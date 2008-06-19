//  Copyright (c) 1998-2008 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_UTIL_WRAPPER_HEAP_JUN_12_2008_0903AM)
#define HPX_UTIL_WRAPPER_HEAP_JUN_12_2008_0903AM

#include <new>
#include <memory>
#include <string>

#include <boost/noncopyable.hpp>
#include <hpx/config.hpp>
#include <hpx/util/logging.hpp>
#include <hpx/runtime/naming/name.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace components { namespace detail 
{
    ///////////////////////////////////////////////////////////////////////////////
    template<typename T, typename Allocator>
    class wrapper_heap : private boost::noncopyable
    {
    public:
        typedef T value_type;
        typedef Allocator allocator_type;
        
        struct data {
            unsigned char data_[sizeof(T)];   // object place
        };

        enum { 
            heap_step = 1024,               // default grow step
            heap_size = sizeof(data)        // size of one element in the heap
        };

    public:
        explicit wrapper_heap(char const* class_name, bool, bool, int step = -1)
          : pool_(NULL), first_free_(NULL), step_(step), size_(0), free_size_(0)
#if defined(HPX_DEBUG_ONE_SIZE_HEAP)
          , class_name_(class_name)
#endif
        {
            BOOST_ASSERT(sizeof(data) == heap_size);

        // adjust step to reasonable value
            if (step_ < heap_step) 
                step_ = heap_step;
            else 
                step_ = ((step_ + heap_step - 1)/heap_step)*heap_step;
        }
        
        wrapper_heap()
          : pool_(NULL), first_free_(NULL), 
            step_(heap_step), size_(0), free_size_(0) 
        {
            BOOST_ASSERT(sizeof(data) == heap_size);
        }
        
        ~wrapper_heap()
        {
            if (pool_ != NULL) {
#if defined(HPX_DEBUG_ONE_SIZE_HEAP)
                if (free_size_ != size_) {
                    LOSH_(error) 
                        << "wrapper_heap " 
                        << (!class_name_.empty() ? class_name_.c_str() : "<Unknown>")
                        << ": releasing heap (" << std::hex << pool_ << ")" 
                        << " with " << size_-free_size_ << " allocated object(s)!";
                }
#endif
                Allocator::free(pool_);
            }
        }

        int size() const { return size_ - free_size_; }
        int free_size() const { return free_size_; }
        bool is_empty() const { return NULL == pool_; }

        T* alloc()
        {
            if (!ensure_pool())
                return NULL;

            T *p = reinterpret_cast<T*>(&first_free_->data_);
            BOOST_ASSERT(p != NULL);

            ++first_free_;
            --free_size_;

            return p;
        }
        
        void free (void *p)
        {
            data* p1 = (data *)(unsigned char *)p;

            BOOST_ASSERT(NULL != pool_ && p1 >= pool_);
            BOOST_ASSERT(NULL != pool_ && p1 <  pool_ + size_);
            BOOST_ASSERT(first_free_ == NULL || p1 != first_free_);

            using namespace std;
            memset(&p1->data_, 0, sizeof(data));
            free_size_++;
            
            // release the pool if this one was the last allocated item
            test_release();
        }
        bool did_alloc (void *p) const
        {
            return NULL != pool_ && NULL != p && pool_ <= p && p < pool_ + size_;
        }

        naming::id_type get_gid(void* p) const
        {
            return naming::invalid_id;
        }
        
    protected:
        bool test_release()
        {
            if (pool_ == NULL || free_size_ < size_)
                return false;
            BOOST_ASSERT(free_size_ == size_);

            Allocator::free(pool_);
            pool_ = first_free_ = NULL;
            size_ = free_size_ = 0;
            return true;
        }

        bool ensure_pool()
        {
            if (NULL == pool_ && !init_pool())
                return false;
            if (first_free_ == pool_+size_) 
                return false;
            return true;
        }

        bool init_pool()
        {
            BOOST_ASSERT(size_ == 0);
            BOOST_ASSERT(first_free_ == NULL);

            std::size_t s = std::size_t(step_ * heap_size);
            pool_ = (data *)Allocator::alloc(s);
            if (NULL == pool_) 
                return false;

#if defined(HPX_DEBUG_ONE_SIZE_HEAP)
            LOSH_(info) 
                << "wrapper_heap " 
                << (!class_name_.empty() ? class_name_.c_str() : "<Unknown>")
                << ": init_pool (" << std::hex << pool_ << ")" 
                << " size: " << s << ".";
#endif
            s /= heap_size;
            first_free_ = pool_;
            size_ = s;
            free_size_ = size_;
            return true;
        }

    private:
        data* pool_;
        data* first_free_;
        int step_;
        int size_;
        int free_size_;

#if defined(HPX_DEBUG_ONE_SIZE_HEAP)
        std::string class_name_;
#endif
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
                bool f1 = false, bool f2 = false, int step = -1)
          : base_type(class_name, f1, f2, step) 
        {}
    };

}}} // namespace hpx::components::detail

#endif
