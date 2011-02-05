//  Copyright (c) 2007-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_THREADMANAGER_THREAD_QUEUE_AUG_25_2009_0132PM)
#define HPX_THREADMANAGER_THREAD_QUEUE_AUG_25_2009_0132PM

#include <map>
#include <memory>

#include <hpx/config.hpp>
#include <hpx/util/logging.hpp>
#include <hpx/util/block_profiler.hpp>
#include <hpx/runtime/threads/thread.hpp>

#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>
#include <boost/bind.hpp>
#include <boost/atomic.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/lockfree/fifo.hpp>
#include <boost/ptr_container/ptr_map.hpp>

#ifdef HPX_ACCEL_QUEUING
#   include "hpx/runtime/threads/policies/accel_fifo.hpp"
#endif

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace threads { namespace policies
{
    struct add_new_tag {};

    ///////////////////////////////////////////////////////////////////////////
#ifdef HPX_ACCEL_QUEUING
    // hardware accelerated queuing
    typedef accel::fifo<thread *> work_item_queue_type;

    inline void 
    enqueue(work_item_queue_type& work_items, thread* thrd, 
        std::size_t num_thread)
    {
        //printf("enqueue invoked by thread %ld\n", num_thread);
        work_items.enqueue(thrd, num_thread);
    }

    inline bool 
    dequeue(work_item_queue_type& work_items, thread** thrd, 
        std::size_t num_thread)
    {
        return work_items.dequeue(thrd, num_thread);
    }

    inline bool 
    empty(work_item_queue_type& work_items, std::size_t num_thread)
    {
        return work_items.empty(num_thread);
    }

#else
    // software queuing
    typedef boost::lockfree::fifo<thread*> work_item_queue_type;


    inline void 
    enqueue(work_item_queue_type& work_items, thread* thrd, 
        std::size_t num_thread)
    {
        work_items.enqueue(thrd);
    }

    inline bool 
    dequeue(work_item_queue_type& work_items, thread** thrd, 
        std::size_t num_thread)
    {
        return work_items.dequeue(thrd);
    }

    inline bool 
    empty(work_item_queue_type& work_items, std::size_t num_thread)
    {
        return work_items.empty();
    }

#endif

    ///////////////////////////////////////////////////////////////////////////
    template <typename Fifo>
    inline void log_fifo_statistics(Fifo const& q, char const* const desc)
    {
        // FIXME
    }

    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        ///////////////////////////////////////////////////////////////////////
        // This try_lock_wrapper is essentially equivalent to the template 
        // boost::thread::detail::try_lock_wrapper with the one exception, that
        // the lock() function always calls base::try_lock(). This allows us to 
        // skip lock acquisition while exiting the condition variable.
        template<typename Mutex>
        class try_lock_wrapper
          : public boost::detail::try_lock_wrapper<Mutex>
        {
            typedef boost::detail::try_lock_wrapper<Mutex> base;

        public:
            explicit try_lock_wrapper(Mutex& m):
                base(m, boost::try_to_lock)
            {}

            void lock()
            {
                base::try_lock();       // this is different
            }
        };

        ///////////////////////////////////////////////////////////////////////
        // debug helper function, logs all suspended threads
        // this returns true if all threads in the map are currently suspended
        template <typename Map>
        inline bool dump_suspended_threads(std::size_t num_thread,
            Map& tm, std::size_t& idle_loop_count)
        {
            if (idle_loop_count++ < HPX_IDLE_LOOP_COUNT_MAX)
                return false;

            // reset idle loop count
            idle_loop_count = 0;

            bool result = false;
            bool collect_suspended = true;

            bool logged_headline = false;
            typename Map::const_iterator end = tm.end();
            for (typename Map::const_iterator it = tm.begin(); it != end; ++it)
            {
                threads::thread const* thrd = (*it).second;
                threads::thread_state state = thrd->get_state();
                threads::thread_state marked_state = thrd->get_marked_state();

                if (state != marked_state) {
                    // log each thread only once
                    if (!logged_headline) {
                        LTM_(error) << "Listing suspended threads while queue ("
                                    << num_thread << ") is empty:";
                        logged_headline = true;
                    }

                    LTM_(error) << "queue(" << num_thread << "): "
                                << get_thread_state_name(state) 
                                << "(" << std::hex << std::setw(8) 
                                    << std::setfill('0') << (*it).first 
                                << "." << std::hex << std::setw(2) 
                                    << std::setfill('0') << thrd->get_thread_phase() 
                                << "/" << std::hex << std::setw(8) 
                                    << std::setfill('0') << thrd->get_component_id()
                                << ") P" << std::hex << std::setw(8) 
                                    << std::setfill('0') << thrd->get_parent_thread_id() 
                                << ": " << thrd->get_description()
                                << ": " << thrd->get_lco_description();
                    thrd->set_marked_state(state);
                }

                // result should be true if we found only suspended threads
                if (collect_suspended) {
                    switch(state.get_state()) {
                    case threads::suspended:
                        result = true;    // at least one is suspended
                        break;
                    case threads::active:
                        result = false;   // one is active, no deadlock (yet)
                        collect_suspended = false;
                        break;
                    }
                }
            }
            return result;
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    template <bool Global>
    class thread_queue
    {
    private:
        // we use a simple mutex to protect the data members for now
        typedef boost::mutex mutex_type;

        // Add this number of threads to the work items queue each time the 
        // function \a add_new() is called if the queue is empty.
        enum { 
            min_add_new_count = 100, 
            max_add_new_count = 100,
            max_delete_count = 100
        };

        // this is the type of the queues of new or pending threads
        typedef work_item_queue_type work_items_type;

        // this is the type of a map holding all threads (except depleted ones)
        typedef boost::ptr_map<
            thread_id_type, thread, std::less<thread_id_type>, heap_clone_allocator
        > thread_map_type;

        // this is the type of the queue of new tasks not yet converted to
        // threads
        typedef boost::tuple<thread_init_data, thread_state_enum> task_description;
        typedef boost::lockfree::fifo<task_description const*> task_items_type;

        typedef boost::lockfree::fifo<thread_id_type> thread_id_queue_type;

    protected:
        ///////////////////////////////////////////////////////////////////////
        // add new threads if there is some amount of work available
        std::size_t add_new(long add_count, thread_queue* addfrom, 
            std::size_t num_thread)
        {
            if (0 == add_count)
                return 0;

            long added = 0;
            task_description const* task = 0;
            while (add_count-- && addfrom->new_tasks_.dequeue(&task)) 
            {
                --addfrom->new_tasks_count_;

                // measure thread creation time
                util::block_profiler_wrapper<add_new_tag> bp(add_new_logger_);

                // create the new thread
                thread_state_enum state = boost::get<1>(*task);
                std::auto_ptr<threads::thread> thrd (
                    new (memory_pool_) threads::thread(
                        boost::get<0>(*task), memory_pool_, state));

                delete task;

                // add the new entry to the map of all threads
                thread_id_type id = thrd->get_thread_id();
                std::pair<thread_map_type::iterator, bool> p =
                    thread_map_.insert(id, thrd.get());

                if (!p.second) {
                    HPX_THROW_EXCEPTION(hpx::no_success, 
                        "threadmanager::add_new", 
                        "Couldn't add new thread to the map of threads");
                    return false;
                }

                // transfer ownership to map
                threads::thread* t = thrd.release();

                // only insert the thread into the work-items queue if it is in 
                // pending state
                if (state == pending) {
                    // pushing the new thread into the pending queue of the
                    // specified thread_queue
                    ++added;
                    enqueue(work_items_, t, num_thread);
                    ++work_items_count_;
                    do_some_work();         // wake up sleeping threads
                }
            }

            if (added) {
                LTM_(info) << "add_new: added " << added << " tasks to queues";
            }
            return added;
        }

        ///////////////////////////////////////////////////////////////////////
        bool add_new_if_possible(std::size_t& added, thread_queue* addfrom,
            std::size_t num_thread)
        {
//             if (0 == addfrom->new_tasks_count_) 
//                 return false;

            // create new threads from pending tasks (if appropriate)
            long add_count = -1;                  // default is no constraint

            // if the map doesn't hold max_count threads yet add some
            if (max_count_) {
                std::size_t count = thread_map_.size();
                if (max_count_ >= count + min_add_new_count) {
                    add_count = max_count_ - count;
                    if (add_count < min_add_new_count)
                        add_count = min_add_new_count;
                }
                else {
                    return false;
                }
            }

            std::size_t addednew = add_new(add_count, addfrom, num_thread);
            added += addednew;
            return addednew != 0;
        }

        ///////////////////////////////////////////////////////////////////////
        bool add_new_always(std::size_t& added, thread_queue* addfrom,
            std::size_t num_thread)
        {
//             if (0 == addfrom->new_tasks_count_) 
//                 return false;

            // create new threads from pending tasks (if appropriate)
            long add_count = -1;                  // default is no constraint

            // if we are desperate (no work in the queues), add some even if the 
            // map holds more than max_count
            if (max_count_) {
                std::size_t count = thread_map_.size();
                if (max_count_ >= count + min_add_new_count) {
                    add_count = max_count_ - count;
                    if (add_count < min_add_new_count)
                        add_count = min_add_new_count;
                    if (add_count > max_add_new_count)
                        add_count = max_add_new_count;
                }
                else if (empty(work_items_, num_thread)) {
                    add_count = min_add_new_count;    // add this number of threads
                    max_count_ += min_add_new_count;  // increase max_count
                }
                else {
                    return false;
                }
            }

            std::size_t addednew = add_new(add_count, addfrom, num_thread);
            added += addednew;
            return addednew != 0;
        }

        /// This function makes sure all threads which are marked for deletion
        /// (state is terminated) are properly destroyed
        bool cleanup_terminated()
        {
            {
                long delete_count = max_delete_count;   // delete only this much threads
                thread_id_type todelete;
                while (delete_count && terminated_items_.dequeue(&todelete)) 
                {
                    if (thread_map_.erase(todelete))
                        --delete_count;
                }
            }
            return thread_map_.empty();
        }

    public:
        // The maximum number of active threads this thread manager should
        // create. This number will be a constraint only as long as the work
        // items queue is not empty. Otherwise the number of active threads 
        // will be incremented in steps equal to the \a min_add_new_count
        // specified above.
        enum { max_thread_count = 1000 };

        thread_queue(std::size_t max_count = max_thread_count)
          : work_items_(/*"work_items"*/), work_items_count_(0),
            terminated_items_(/*"terminated_items"*/), 
            max_count_((0 == max_count) ? max_thread_count : max_count),
            add_new_logger_("thread_queue::add_new"),
            new_tasks_count_(0)
        {}

        void set_max_count(std::size_t max_count = max_thread_count)
        {
            max_count_ = (0 == max_count) ? max_thread_count : max_count;
        }

        ///////////////////////////////////////////////////////////////////////
        // This returns the current length of the queues (work items and new items)
        boost::int64_t get_queue_lengths() const
        {
            return work_items_count_ + new_tasks_count_;
        }

        ///////////////////////////////////////////////////////////////////////
        // create a new thread and schedule it if the initial state is equal to 
        // pending
        thread_id_type create_thread(thread_init_data& data, 
            thread_state_enum initial_state, bool run_now, 
            std::size_t num_thread, error_code& ec)
        {
            if (run_now) {
                mutex_type::scoped_lock lk(mtx_);

                std::auto_ptr<threads::thread> thrd (
                    new (memory_pool_) threads::thread(
                        data, memory_pool_, initial_state));

                // add a new entry in the map for this thread
                thread_id_type id = thrd->get_thread_id();
                std::pair<thread_map_type::iterator, bool> p =
                    thread_map_.insert(id, thrd.get());

                if (!p.second) {
                    HPX_THROWS_IF(ec, hpx::no_success, 
                        "threadmanager::register_thread", 
                        "Couldn't add new thread to the map of threads");
                    return invalid_thread_id;
                }

                // push the new thread in the pending queue thread
                if (initial_state == pending) 
                    schedule_thread(thrd.get(), num_thread);

                do_some_work();       // try to execute the new work item
                thrd.release();       // release ownership to the map

                // return the thread_id of the newly created thread
                return id;
            }

            // do not execute the work, but register a task description for 
            // later thread creation
            ++new_tasks_count_;
            new_tasks_.enqueue(new task_description(data, initial_state));
            return invalid_thread_id;     // thread has not been created yet
        }

        /// Return the next thread to be executed, return false if non is 
        /// available
        bool get_next_thread(threads::thread** thrd, std::size_t num_thread)
        {
            if (dequeue(work_items_, thrd, num_thread)) {
                --work_items_count_;
                return true;
            }
            return false;
        }

        /// Schedule the passed thread
        void schedule_thread(threads::thread* thrd, std::size_t num_thread)
        {
            enqueue(work_items_, thrd, num_thread);
            ++work_items_count_;
            do_some_work();         // wake up sleeping threads
        }

        /// Destroy the passed thread as it has been terminated
        bool destroy_thread(threads::thread* thrd)
        {
            if (thrd->is_created_from(&memory_pool_)) {
                thread_id_type id = thrd->get_thread_id();
                terminated_items_.enqueue(id);
                return true;
            }
            return false;
        }

        /// Return the number of existing threads, regardless of their state
        std::size_t get_thread_count() const
        {
            return thread_map_.size();
        }

        /// This is a function which gets called periodically by the thread 
        /// manager to allow for maintenance tasks to be executed in the 
        /// scheduler. Returns true if the OS thread calling this function
        /// has to be terminated (i.e. no more work has to be done).
        bool wait_or_add_new(std::size_t num_thread, bool running, 
            std::size_t& idle_loop_count, std::size_t& added,
            thread_queue* addfrom_ = 0)
        {
            thread_queue* addfrom = addfrom_ ? addfrom_ : this;

            // only one dedicated OS thread is allowed to acquire the 
            // lock for the purpose of inserting the new threads into the 
            // thread-map and deleting all terminated threads
            if (Global && 0 == num_thread) {
                // first clean up terminated threads
                detail::try_lock_wrapper<mutex_type> lk(mtx_);
                if (lk) {
                    // no point in having a thread waiting on the lock 
                    // while another thread is doing the maintenance
                    cleanup_terminated();

                    // now, add new threads from the queue of task descriptions
                    add_new_if_possible(added, addfrom, num_thread);    // calls notify_all
                }
            }

            bool terminate = false;
            while (0 == work_items_count_) {
                // No obvious work has to be done, so a lock won't hurt too much
                // but we lock only one of the threads, assuming this thread
                // will do the maintenance
                //
                // We prefer to exit this while loop (some kind of very short 
                // busy waiting) to blocking on this lock. Locking fails either
                // when a thread is currently doing thread maintenance, which
                // means there might be new work, or the thread owning the lock 
                // just falls through to the wait below (no work is available)
                // in which case the current thread (which failed to acquire 
                // the lock) will just retry to enter this loop.
                detail::try_lock_wrapper<mutex_type> lk(mtx_);
                if (!lk)
                    break;            // avoid long wait on lock

                // this thread acquired the lock, do maintenance and finally
                // call wait() if no work is available
                LTM_(info) << "tfunc(" << num_thread << "): queues empty"
                           << ", threads left: " << thread_map_.size();

                // stop running after all PX threads have been terminated
                bool added_new = add_new_always(added, addfrom, num_thread);
                if (!added_new && !running) {
                    // Before exiting each of the OS threads deletes the 
                    // remaining terminated PX threads 
                    if (cleanup_terminated()) {
                        // we don't have any registered work items anymore
                        do_some_work();       // notify possibly waiting threads
                        terminate = true;
                        break;                // terminate scheduling loop
                    }

                    LTM_(info) << "tfunc(" << num_thread 
                               << "): threadmap not empty";
                }
                else {
                    cleanup_terminated();
                }

                if (!Global)
                    break;

                if (added_new) {
                    // dump list of suspended threads once a second
                    if (LHPX_ENABLED(error) && addfrom->new_tasks_.empty())
                        detail::dump_suspended_threads(num_thread, thread_map_, idle_loop_count);

                    break;    // we got work, exit loop
                }

                // Wait until somebody needs some action (if no new work 
                // arrived in the meantime).
                // Ask again if queues are empty to avoid race conditions (we 
                // needed to lock anyways...), this way no notify_all() gets lost
                if (0 == work_items_count_) {
                    LTM_(info) << "tfunc(" << num_thread 
                               << "): queues empty, entering wait";

                    // dump list of suspended threads once a second
                    if (LHPX_ENABLED(error) && addfrom->new_tasks_.empty())
                        detail::dump_suspended_threads(num_thread, thread_map_, idle_loop_count);

                    namespace bpt = boost::posix_time;
                    bool timed_out = !cond_.timed_wait(lk, bpt::microseconds(10*idle_loop_count));
                        
                    LTM_(info) << "tfunc(" << num_thread << "): exiting wait";

                    // make sure all pending new threads are properly queued
                    // but do that only if the lock has been acquired while 
                    // exiting the condition.wait() above
                    if ((lk && add_new_always(added, addfrom, num_thread)) || timed_out)
                        break;
                }
            }
            return terminate;
        }

        /// This function gets called by the threadmanager whenever new work
        /// has been added, allowing the scheduler to reactivate one or more of
        /// possibly idleing OS threads
        void do_some_work()
        {
            if (Global)
                cond_.notify_all();
        }

        ///////////////////////////////////////////////////////////////////////
        bool dump_suspended_threads(std::size_t num_thread
          , std::size_t& idle_loop_count)
        {
            mutex_type::scoped_lock lk(mtx_);
            return detail::dump_suspended_threads(num_thread, thread_map_
              , idle_loop_count);
        }

        ///////////////////////////////////////////////////////////////////////
        void on_start_thread(std::size_t num_thread) {}
        void on_stop_thread(std::size_t num_thread)
        {
            if (0 == num_thread) {
                // print queue statistics
                log_fifo_statistics(work_items_, "thread_queue");
                log_fifo_statistics(terminated_items_, "thread_queue");
                log_fifo_statistics(new_tasks_, "thread_queue");
            }
        }
        void on_error(std::size_t num_thread, boost::exception_ptr const& e) {}

    private:
        friend class local_queue_scheduler;

        mutable mutex_type mtx_;            ///< mutex protecting the members
        boost::condition cond_;             ///< used to trigger some action

        thread_map_type thread_map_;        ///< mapping of thread id's to PX-threads
        work_items_type work_items_;        ///< list of active work items
        boost::atomic<long> work_items_count_;    ///< count of active work items
        thread_id_queue_type terminated_items_;   ///< list of terminated threads

        std::size_t max_count_;             ///< maximum number of existing PX-threads
        task_items_type new_tasks_;         ///< list of new tasks to run
        boost::atomic<long> new_tasks_count_;     ///< count of new tasks to run

        threads::thread_pool memory_pool_;  ///< OS thread local memory pools for
                                            ///< PX-threads 

        util::block_profiler<add_new_tag> add_new_logger_;
    };

}}}

#endif
