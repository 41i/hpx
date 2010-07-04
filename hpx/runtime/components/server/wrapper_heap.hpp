//  Copyright (c) 1998-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_UTIL_WRAPPER_HEAP_JUN_12_2008_0904AM)
#define HPX_UTIL_WRAPPER_HEAP_JUN_12_2008_0904AM

#include <new>
#include <memory>
#include <string>

#include <boost/noncopyable.hpp>
#include <boost/aligned_storage.hpp>
#include <boost/type_traits/alignment_of.hpp>

#include <hpx/config.hpp>
#include <hpx/util/logging.hpp>
#include <hpx/runtime/naming/name.hpp>
#include <hpx/util/generate_unique_ids.hpp>
#if !defined(_DEBUG)
#include <hpx/util/spinlock_pool.hpp>
#endif

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace components { namespace detail 
{
#if HPX_DEBUG_WRAPPER_HEAP != 0
    namespace debug
    {
        ///////////////////////////////////////////////////////////////////////
        // Test memory area for being filled as expected
        inline 
        bool test_fill_bytes (void *p, unsigned char c, std::size_t cnt)
        {
            unsigned char* uc = (unsigned char*)p;
            for (std::size_t i = 0; i < cnt; ++i) {
                if (*uc++ != c)
                    return false;
            }
            return true;
        }

        ///////////////////////////////////////////////////////////////////////
        // Fill memory area
        inline 
        void fill_bytes (void *p, unsigned char c, int cnt)
        {
            using namespace std;    // some systems have memset in namespace std
            memset (p, c, cnt);
        }

    } // namespace debug
#endif

    ///////////////////////////////////////////////////////////////////////////////
    template<typename T, typename Allocator>
    class wrapper_heap : private boost::noncopyable
    {
    public:
        typedef T value_type;
        typedef Allocator allocator_type;

#if HPX_DEBUG_WRAPPER_HEAP != 0
        enum guard_value {
            initial_value = 0xcc,           // memory has been initialized
            freed_value = 0xdd,             // memory has been freed
        };
#endif

#if !defined(_DEBUG)
        struct tag {};
        typedef hpx::util::spinlock_pool<tag> mutex_type;
        typedef typename mutex_type::scoped_lock scoped_lock;
#else
        typedef boost::mutex mutex_type;
#endif

        typedef boost::aligned_storage<sizeof(value_type),
            boost::alignment_of<value_type>::value> storage_type;
//         storage_type data;

        enum { 
            heap_step = 0xFFFF,               // default initial number of elements
            heap_size = sizeof(storage_type)  // size of one element in the heap
        };

    public:
        explicit wrapper_heap(char const* class_name, bool, bool, 
                int count, std::size_t step = (std::size_t)-1)
          : pool_(NULL), first_free_(NULL), step_(step), size_(0), free_size_(0),
            base_gid_(naming::invalid_gid), get_agas_client_(NULL),
            class_name_(class_name), alloc_count_(0), free_count_(0),
            heap_count_(count)
        {
            BOOST_ASSERT(sizeof(storage_type) == heap_size);

        // adjust step to reasonable value
            if ((std::size_t)(-1) == step_ || step_ < heap_step) 
                step_ = heap_step;
            else 
                step_ = ((step_ + heap_step - 1)/heap_step)*heap_step;

            if (!init_pool())
                throw std::bad_alloc();
        }

        wrapper_heap()
          : pool_(NULL), first_free_(NULL), 
            step_(heap_step), size_(0), free_size_(0),
            base_gid_(naming::invalid_gid), get_agas_client_(NULL),
            alloc_count_(0), free_count_(0), heap_count_(0)
        {
            BOOST_ASSERT(sizeof(storage_type) == heap_size);
            if (!init_pool())
                throw std::bad_alloc();
        }

        ~wrapper_heap()
        {
            tidy();
        }

        int size() const { return int(size_ - free_size_); }
        int free_size() const { return free_size_; }
        bool is_empty() const { return NULL == pool_; }
        bool has_allocatable_slots() const { return first_free_ < pool_+size_; }

        bool alloc(T** result, std::size_t count = 1)
        {
#if !defined(_DEBUG)
            scoped_lock l(this);
#else
            mutex_type::scoped_lock l(mtx_);
#endif
            if (!ensure_pool(count))
                return false;

            alloc_count_ += (int)count;

            value_type* p = static_cast<value_type*>(first_free_->address());
            BOOST_ASSERT(p != NULL);

            first_free_ += count;
            free_size_ -= (int)count;

#if HPX_DEBUG_WRAPPER_HEAP != 0
            // init memory blocks
            debug::fill_bytes(p, initial_value, count*sizeof(storage_type));
#endif
            BOOST_ASSERT(free_size_ >= 0);

            *result = p;
            return true;
        }

        void free(void *p, std::size_t count = 1)
        {
            BOOST_ASSERT(did_alloc(p));

#if !defined(_DEBUG)
            scoped_lock l(this);
#else
            mutex_type::scoped_lock l(mtx_);
#endif
            storage_type* p1 = static_cast<storage_type*>(p);

            BOOST_ASSERT(NULL != pool_ && p1 >= pool_);
            BOOST_ASSERT(NULL != pool_ && p1 + count <= pool_ + size_);
            BOOST_ASSERT(first_free_ == NULL || p1 != first_free_);
            BOOST_ASSERT((std::size_t)free_size_ + count <= size_);
#if HPX_DEBUG_WRAPPER_HEAP != 0
            // make sure this has not been freed yet
            BOOST_ASSERT(!debug::test_fill_bytes(p1->address(), freed_value, 
                count*sizeof(storage_type)));

            // give memory back to pool
            debug::fill_bytes(p1->address(), freed_value, sizeof(storage_type));
#endif
            free_count_ += (int)count;
            free_size_ += (int)count;

            // release the pool if this one was the last allocated item
            test_release();
        }
        bool did_alloc (void *p) const
        {
            // no lock is necessary here as all involved variables are immutable
            return NULL != pool_ && NULL != p && pool_ <= p && p < pool_ + size_;
        }

        /// \brief Get the global id of the managed_component instance 
        ///        given by the parameter \a p. 
        ///
        ///
        /// \note  The pointer given by the parameter \a p must have been 
        ///        allocated by this instance of a \a wrapper_heap
        template <typename Mutex>
        naming::gid_type 
        get_gid(util::unique_ids<Mutex>& ids, void* p) 
        {
            BOOST_ASSERT(did_alloc(p));

            value_type* addr = static_cast<value_type*>(pool_->address());
            if (!base_gid_) {
#if !defined(_DEBUG)
                scoped_lock l(this);
#else
                mutex_type::scoped_lock l(mtx_);
#endif

                // store a pointer to the AGAS client
                hpx::applier::applier& appl = hpx::applier::get_applier();
                get_agas_client_ = &appl.get_agas_client();

                // this is the first call to get_gid() for this heap - allocate 
                // a sufficiently large range of global ids
                base_gid_ = ids.get_id(appl.here(), *get_agas_client_, step_);

                // register the global ids and the base address of this heap
                // with the AGAS
                if (!get_agas_client_->bind_range(base_gid_, step_, 
                      naming::address(appl.here(), value_type::get_component_type(), addr),
                      sizeof(value_type))) 
                {
                    return naming::invalid_gid;
                }
            }
            return base_gid_ + (static_cast<value_type*>(p) - addr);
        }

        /// \brief Get the global id of the managed_component instance 
        ///        given by the parameter \a p. 
        ///
        ///
        /// \note  The pointer given by the parameter \a p must have been 
        ///        allocated by this instance of a \a wrapper_heap
//         template <typename Mutex>
//         bool get_full_address(util::unique_ids<Mutex>& ids, void* p, 
//             naming::full_address& fa) 
//         {
//             BOOST_ASSERT(did_alloc(p));
// 
//             hpx::applier::applier& appl = hpx::applier::get_applier();
//             value_type* addr = static_cast<value_type*>(pool_->address());
//             naming::address& localaddr = fa.addr();
// 
//             localaddr.locality_ = appl.here();
//             localaddr.type_ = value_type::get_component_type();
//             localaddr.address_ = 
//                 reinterpret_cast<naming::address::address_type>(addr);
// 
//             if (!base_gid_) {
//                 // store a pointer to the AGAS client
//                 get_agas_client_ = &appl.get_agas_client();
// 
//                 // this is the first call to get_gid() for this heap - allocate 
//                 // a sufficiently large range of global ids
//                 base_gid_ = ids.get_id(appl.here(), *get_agas_client_, step_);
// 
//                 // register the global ids and the base address of this heap
//                 // with the AGAS
//                 if (!get_agas_client_->bind_range(
//                         base_gid_, step_, localaddr, sizeof(value_type))) 
//                 {
//                     return false;
//                 }
//             }
// 
//             localaddr.address_ = reinterpret_cast<naming::address::address_type>(p);
//             fa.gid() = base_gid_ + (static_cast<value_type*>(p) - addr);
//             return true;
//         }

    protected:
        bool test_release()
        {
            if (pool_ == NULL || (std::size_t)free_size_ < size_ || first_free_ < pool_+size_)
                return false;
            BOOST_ASSERT(free_size_ == size_);

            // unbind in AGAS service 
            if (base_gid_) {
                BOOST_ASSERT(NULL != get_agas_client_);
                get_agas_client_->unbind_range(base_gid_, step_);
                base_gid_ = naming::invalid_gid;
            }

            tidy();
            return true;
        }

        bool ensure_pool(std::size_t count)
        {
            if (NULL == pool_)
                return false;
            if (first_free_ + count > pool_+size_) 
                return false;
            return true;
        }

        bool init_pool()
        {
            BOOST_ASSERT(size_ == 0);
            BOOST_ASSERT(first_free_ == NULL);

            std::size_t s = step_ * heap_size;
            pool_ = (storage_type*)Allocator::alloc(s);
            if (NULL == pool_) 
                return false;

            first_free_ = pool_;
            size_ = s / heap_size;
            free_size_ = (int)size_;

            LOSH_(info) 
                << "wrapper_heap (" 
                << (!class_name_.empty() ? class_name_.c_str() : "<Unknown>")
                << "): init_pool (" << std::hex << pool_ << ")" 
                << " size: " << s << ".";

            return true;
        }

        void tidy()
        {
            if (pool_ != NULL) {
                LOSH_(debug) 
                    << "wrapper_heap (" 
                    << (!class_name_.empty() ? class_name_.c_str() : "<Unknown>")
                    << "): releasing heap: alloc count: " << alloc_count_ 
                    << ", free count: " << free_count_ << ".";

                if (free_size_ != size_ || alloc_count_ != free_count_) {
                    LOSH_(warning) 
                        << "wrapper_heap (" 
                        << (!class_name_.empty() ? class_name_.c_str() : "<Unknown>")
                        << "): releasing heap (" << std::hex << pool_ << ")" 
                        << " with " << size_-free_size_ << " allocated object(s)!";
                }
                Allocator::free(pool_);
                pool_ = first_free_ = NULL;
                size_ = free_size_ = 0;
            }
        }

    private:
        storage_type* pool_;
        storage_type* first_free_;
        std::size_t step_;
        std::size_t size_;
        int free_size_;

        // these values are used for AGAS registration of all elements of this
        // managed_component heap
        naming::gid_type base_gid_;
        naming::resolver_client const* get_agas_client_;

#if defined(_DEBUG)
        mutable mutex_type mtx_;
#endif

    public:
        std::string const class_name_;
        int alloc_count_;
        int free_count_;
        int heap_count_;
    };

    ///////////////////////////////////////////////////////////////////////////
    namespace one_size_heap_allocators
    {
        ///////////////////////////////////////////////////////////////////////
        // simple allocator which gets the memory from malloc, but which does
        // not reallocate the heap (it doesn't grow)
        struct fixed_mallocator  
        {
            static void* alloc(std::size_t& size) 
            { 
                return malloc(size); 
            }
            static void free(void* p) 
            { 
                ::free(p); 
            }
            static void* realloc(std::size_t &size, void *p)
            { 
                // normally this should return ::realloc(p, size), but we are 
                // not interested in growing the allocated heaps, so we just 
                // return NULL
                return NULL;
            }
        };
    }

    ///////////////////////////////////////////////////////////////////////////
    // heap using malloc and friends
    template<typename T>
    class fixed_wrapper_heap : 
        public wrapper_heap<T, one_size_heap_allocators::fixed_mallocator>
    {
    private:
        typedef 
            wrapper_heap<T, one_size_heap_allocators::fixed_mallocator> 
        base_type;

    public:
        explicit fixed_wrapper_heap(char const* class_name = "<Unknown>", 
                bool f1 = false, bool f2 = false, int count = 0, 
                std::size_t step = (std::size_t)-1)
          : base_type(class_name, f1, f2, count, step) 
        {}
    };

}}} // namespace hpx::components::detail

#endif
