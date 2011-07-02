//  Copyright (c) 1998-2011 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_041EF599_BA27_47ED_B1F0_2691B28966B3)
#define HPX_041EF599_BA27_47ED_B1F0_2691B28966B3

#include <list>
#include <string>

#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/atomic.hpp>

#include <hpx/hpx_fwd.hpp>
#include <hpx/state.hpp>
#include <hpx/config.hpp>
#include <hpx/lcos/local_shared_mutex.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace util 
{

template<typename Heap>
class one_size_heap_list 
{
  public:
    typedef Heap heap_type;

    typedef typename heap_type::allocator_type allocator_type;
    typedef typename heap_type::value_type value_type;

    typedef std::list<boost::shared_ptr<heap_type> > list_type;
    typedef typename list_type::iterator iterator;
    typedef typename list_type::const_iterator const_iterator;

    enum
    { 
        heap_step = heap_type::heap_step,   // default grow step
        heap_size = heap_type::heap_size    // size of the object
    };

    typedef lcos::local_shared_mutex mutex_type;
    typedef boost::shared_lock<mutex_type> shared_lock_type;
    typedef boost::upgrade_lock<mutex_type> upgrade_lock_type;
    typedef boost::upgrade_to_unique_lock<mutex_type> upgraded_lock_type;
    typedef boost::unique_lock<mutex_type> unique_lock_type;

    explicit one_size_heap_list(char const* class_name = "")
        : class_name_(class_name)
        , alloc_count_(0L)
        , free_count_(0L)
        , heap_count_(0L)
        , max_alloc_count_(0L)
    {
        BOOST_ASSERT(sizeof(typename heap_type::storage_type) == heap_size);
    }

    explicit one_size_heap_list(std::string const& class_name)
        : class_name_(class_name)
        , alloc_count_(0L)
        , free_count_(0L)
        , heap_count_(0L)
        , max_alloc_count_(0L)
    {
        BOOST_ASSERT(sizeof(typename heap_type::storage_type) == heap_size);
    }

    ~one_size_heap_list()
    {
        LOSH_(info) 
            << "one_size_heap_list (" 
            << (!class_name_.empty() ? class_name_.c_str() : "<Unknown>")
            << "): releasing heap_list: max count: " << max_alloc_count_ 
            << " (in " << heap_count_ << " heaps), alloc count: " 
            << alloc_count_ << ", free count: " << free_count_ << ".";

        if (alloc_count_ != free_count_) 
        {
            LOSH_(warning) 
                << "one_size_heap_list (" 
                << (!class_name_.empty() ? class_name_.c_str() : "<Unknown>")
                << "): releasing heap_list with " << alloc_count_-free_count_ 
                << " allocated object(s)!";
        }
    }

    // operations
    value_type* alloc(std::size_t count = 1)
    {
        if (0 == count)
            throw std::bad_alloc();   // this doesn't make sense for us

        std::size_t size = 0; 
        value_type *p = NULL;
        {
            shared_lock_type guard(mtx_);

            size = heap_list_.size();

            for (iterator it = heap_list_.begin(); it != heap_list_.end(); ++it) 
            {
                if ((*it)->alloc(&p, count)) {
                    // update statistics
                    alloc_count_ += (int)count;
                    if (alloc_count_-free_count_ > max_alloc_count_)
                        max_alloc_count_.store(alloc_count_-free_count_);

                    return p;
                }
                else {
                    LOSH_(info) 
                        << "one_size_heap_list (" 
                        << (!class_name_.empty() ? class_name_.c_str() : "<Unknown>")
                        << "): failed to allocate from heap (" << (*it)->heap_count_ 
                        << "), allocated: " << (*it)->size() << ", free'd: " 
                        << (*it)->free_size() << ".";
                }
            }
        }

        bool did_create = false;
        // create new heap
        {
            // REVIEW: should we acquire upgrade ownership, and then upgrade to
            // unique if necessary?

            // acquire exclusive access
            unique_lock_type ul(mtx_); 

            if (size == heap_list_.size())
            {
                iterator itnew = heap_list_.insert(heap_list_.begin(),
                    typename list_type::value_type(
                        new heap_type(class_name_.c_str(), false, true, 
                                heap_count_+1, heap_step
                            )
                    ));
    
                if (itnew == heap_list_.end())
                    throw std::bad_alloc();   // insert failed
    
                bool result = (*itnew)->alloc(&p, count);
                if (!result || NULL == p)
                    throw std::bad_alloc();   // snh 

                did_create = true;
            }
        }

        if (did_create)
        {
            ++heap_count_;
            LOSH_(info) 
                << "one_size_heap_list (" 
                << (!class_name_.empty() ? class_name_.c_str() : "<Unknown>")
                << "): creating new heap (" << heap_count_ 
                << "), size of heap_list: " << heap_list_.size() << ".";
            return p;
        }

        // try again, we just got a new heap, so we should be good.
        else
            return alloc(count);
    }

    heap_type* alloc_heap()
    {
        return new heap_type(class_name_.c_str(),
            false, true, 0, heap_step);
    }

    void add_heap(heap_type* p)
    {
        BOOST_ASSERT(p);

        // acquire exclusive access
        unique_lock_type ul (mtx_); 

        p->heap_count_ = heap_count_; 

        iterator it = heap_list_.insert(heap_list_.begin(),
            typename list_type::value_type(p));

        if (it == heap_list_.end())
            throw std::bad_alloc();   // insert failed

        ++heap_count_;
    }

    void free(void* p, std::size_t count = 1)
    {
        if (NULL == p && threads::threadmanager_is(running))
            return;

        shared_lock_type guard (mtx_);

        // find heap which allocated this pointer
        iterator it = heap_list_.begin();
        for (/**/; it != heap_list_.end(); ++it) {
            if ((*it)->did_alloc(p)) {
                (*it)->free(p, count);

                free_count_ += (int)count;

                if ((*it)->is_empty()) {
                    LOSH_(info) 
                        << "one_size_heap_list (" 
                        << (!class_name_.empty() ? class_name_.c_str() : "<Unknown>")
                        << "): freeing empty heap (" << (*it)->heap_count_ << ").";

                    // acquire exclusive access
                    guard.unlock();

                    unique_lock_type ul (mtx_); 
                    heap_list_.erase (it);
                }
                return;
            }
        }
        BOOST_ASSERT(false);   // no heap found
    }

    bool did_alloc(void* p) const
    {
        shared_lock_type guard (mtx_);
        for (iterator it = heap_list_.begin(); it != heap_list_.end(); ++it) 
        {
            if ((*it)->did_alloc(p)) 
                return true;
        }
        return false;
    }

  protected:
    mutex_type mtx_;
    list_type heap_list_;

  public:
    std::string const class_name_;
    boost::atomic<unsigned long> alloc_count_;
    boost::atomic<unsigned long> free_count_;
    boost::atomic<unsigned long> heap_count_;
    boost::atomic<unsigned long> max_alloc_count_;
};

}} // namespace hpx::util

#endif // HPX_041EF599_BA27_47ED_B1F0_2691B28966B3

