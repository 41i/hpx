//  Copyright (c) 2008-2009 Chirag Dekate, Hartmut Kaiser, Anshul Tandon
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_PX_THREAD_MAY_20_2008_0910AM)
#define HPX_PX_THREAD_MAY_20_2008_0910AM

#include <hpx/config.hpp>

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/noncopyable.hpp>
#include <boost/lockfree/atomic_int.hpp>
#include <boost/lockfree/cas.hpp>
#include <boost/lockfree/branch_hints.hpp>
#include <boost/coroutine/coroutine.hpp>

#include <hpx/hpx_fwd.hpp>
#include <hpx/runtime/applier/applier.hpp>
#include <hpx/runtime/components/component_type.hpp>
#include <hpx/runtime/components/server/managed_component_base.hpp>
#include <hpx/lcos/base_lco.hpp>
// #include <hpx/util/one_size_heap.hpp>
// #include <hpx/util/one_size_heap_list.hpp>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx { namespace threads { namespace detail
{
    ///////////////////////////////////////////////////////////////////////////
    // This is the representation of a ParalleX thread
    class thread : public lcos::base_lco, private boost::noncopyable
    {
        typedef boost::function<thread_function_type> function_type;

    public:
        thread(function_type func, thread_id_type id, thread_state newstate,
                char const* const description)
          : coroutine_(func, id), 
            current_state_(newstate), current_state_ex_(wait_signaled),
            description_(description)
        {}

        /// This constructor is provided just for compatibility with the scheme
        /// of component creation. But since threads never get created 
        /// by a factory (runtime_support) instance, we can leave this 
        /// constructor empty
        thread()
          : coroutine_(function_type(), 0), description_("")
        {
            BOOST_ASSERT(false);    // shouldn't ever be called
        }

        ~thread() 
        {}

        // This is the component id. Every component needs to have an embedded
        // enumerator 'value' which is used by the generic action implementation
        // to associate this component with a given action.
        static components::component_type get_component_type();
        static void set_component_type(components::component_type);

        thread_state execute()
        {
            return coroutine_(set_state_ex(wait_signaled));
        }

        thread_state get_state() const 
        {
            boost::lockfree::memory_barrier();
            return static_cast<thread_state>(current_state_);
        }

        thread_state set_state(thread_state newstate)
        {
            using namespace boost::lockfree;
            for (;;) {
                long prev_state = current_state_;
                if (likely(CAS(&current_state_, prev_state, (long)newstate)))
                    return static_cast<thread_state>(prev_state);
            }
        }

        thread_state_ex get_state_ex() const 
        {
            boost::lockfree::memory_barrier();
            return static_cast<thread_state_ex>(current_state_ex_);
        }

        thread_state_ex set_state_ex(thread_state_ex newstate_ex)
        {
            using namespace boost::lockfree;
            for (;;) {
                long prev_state = current_state_ex_;
                if (likely(CAS(&current_state_ex_, prev_state, (long)newstate_ex)))
                    return static_cast<thread_state_ex>(prev_state);
            }
        }

        thread_id_type get_thread_id() const
        {
            return coroutine_.get_thread_id();
        }

        char const* const get_description() const
        {
            return description_;
        }

        // threads use a specialized allocator for fast creation/destruction
        static void *operator new(std::size_t size);
        static void operator delete(void *p, std::size_t size);

    public:
        // action support

        // This is the component id. Every component needs to have an embedded
        // enumerator 'value' which is used by the generic action implementation
        // to associate this component with a given action.
        enum { value = components::component_thread };

        /// 
        void set_event()
        {
            // we need to reactivate the thread itself
            if (suspended == static_cast<thread_state>(current_state_)) {
                hpx::applier::get_applier().get_thread_manager().
                    set_state(get_thread_id(), pending);
            }
            // FIXME: implement functionality required for depleted state
        }

    private:
        coroutine_type coroutine_;
        // the state is stored as a long to allow to use CAS
        long current_state_;
        long current_state_ex_;
        char const* const description_;
    };

///////////////////////////////////////////////////////////////////////////////
}}}

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace threads 
{
    ///////////////////////////////////////////////////////////////////////////
    /// \class thread thread.hpp hpx/runtime/threads/thread.hpp
    ///
    /// A \a thread is the representation of a ParalleX thread. It's a first
    /// class object in ParalleX. In our implementation this is a user level 
    /// thread running on top of one of the OS threads spawned by the \a 
    /// threadmanager.
    ///
    /// A \a thread encapsulates:
    ///  - A thread status word (see the functions \a thread#get_state and 
    ///    \a thread#set_state)
    ///  - A function to execute (the thread function)
    ///  - A frame (in this implementation this is a block of memory used as 
    ///    the threads stack)
    ///  - A block of registers (not implemented yet)
    ///
    /// Generally, \a threads are not created or executed directly. All 
    /// functionality related to the management of \a thread's is 
    /// implemented by the \a threadmanager.
    class thread 
      : public components::managed_component<detail::thread, thread>
    {
    private:
        typedef detail::thread wrapped_type;
        typedef 
            components::managed_component<wrapped_type, thread> 
        base_type;

        // avoid warning about using 'this' in initializer list
        thread* This() { return this; }

    public:
        thread()
        {}

        /// \brief Construct a new \a thread
        ///
        /// \param func     [in] The thread function to execute by this 
        ///                 \a thread.
        /// \param tm       [in] A reference to the thread manager this 
        ///                 \a thread will be associated with.
        /// \param newstate [in] The initial thread state this instance will
        ///                 be initialized with.
        thread(boost::function<thread_function_type> threadfunc, 
               thread_state new_state = init, char const* const desc = "")
          : base_type(new detail::thread(threadfunc, This(), new_state, desc))
        {
            LTM_(debug) << "thread::thread(" << this << "), description(" 
                        << desc << ")";
        }

        ~thread() 
        {
            LTM_(debug) << "~thread(" << this << "), description(" 
                        << get()->get_description() << ")";
        }

        thread_id_type get_thread_id() const
        {
            return const_cast<thread*>(this);
        }

        /// The get_state function allows to query the state of this thread
        /// instance.
        ///
        /// \returns        This function returns the current state of this
        ///                 thread. It will return one of the values as defined 
        ///                 by the \a thread_state enumeration.
        ///
        /// \note           This function will be seldom used directly. Most of 
        ///                 the time the state of a thread will be retrieved
        ///                 by using the function \a threadmanager#get_state.
        thread_state get_state() const 
        {
            return get()->get_state();
        }

        /// The set_state function allows to change the state of this thread 
        /// instance.
        ///
        /// \param newstate [in] The new state to be set for the thread.
        ///
        /// \note           This function will be seldom used directly. Most of 
        ///                 the time the state of a thread will have to be 
        ///                 changed using the threadmanager. Moreover,
        ///                 changing the thread state using this function does
        ///                 not change its scheduling status. It only sets the
        ///                 thread's status word. To change the thread's 
        ///                 scheduling status \a threadmanager#set_state should
        ///                 be used.
        thread_state set_state(thread_state new_state)
        {
            return get()->set_state(new_state);
        }

        /// The get_state_ex function allows to query the extended state of 
        /// this thread instance.
        ///
        /// \returns        This function returns the current extended state of 
        ///                 this thread. It will return one of the values as 
        ///                 defined by the \a thread_state_ex enumeration.
        ///
        /// \note           This function will be seldom used directly. Most of 
        ///                 the time the extended state of a thread will be 
        ///                 retrieved by using the function 
        ///                 \a threadmanager#get_state_ex.
        thread_state_ex get_state_ex() const 
        {
            return get()->get_state_ex();
        }

        /// The set_state function allows to change the extended state of this 
        /// thread instance.
        ///
        /// \param newstate [in] The new extended state to be set for the 
        ///                 thread.
        ///
        /// \note           This function will be seldom used directly. Most of 
        ///                 the time the state of a thread will have to be 
        ///                 changed using the threadmanager. 
        thread_state_ex set_state_ex(thread_state_ex new_state)
        {
            return get()->set_state_ex(new_state);
        }

        /// \brief Execute the thread function
        ///
        /// \returns        This function returns the thread state the thread
        ///                 should be scheduled from this point on. The thread
        ///                 manager will use the returned value to set the 
        ///                 thread's scheduling status.
        thread_state operator()()
        {
            return get()->execute();
        }

        /// \brief Get the (optional) description of this thread
        char const* const get_description() const
        {
            return get()->get_description();
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    thread_id_type const invalid_thread_id = 0;

}}

#include <hpx/config/warnings_suffix.hpp>

#endif
