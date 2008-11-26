//  Copyright (c) 2007-2008 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_LCOS_FUTURE_VALUE_JUN_12_2008_0654PM)
#define HPX_LCOS_FUTURE_VALUE_JUN_12_2008_0654PM

#include <hpx/hpx_fwd.hpp>
#include <hpx/runtime/applier/applier.hpp>
#include <hpx/runtime/threads/thread.hpp>
#include <hpx/runtime/actions/component_action.hpp>
#include <hpx/runtime/components/component_type.hpp>
#include <hpx/runtime/components/server/managed_component_base.hpp>
#include <hpx/lcos/base_lco.hpp>
#include <hpx/util/full_empty_memory.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/variant.hpp>
#include <boost/static_assert.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace lcos { namespace detail 
{
    /// A future_value can be used by a single thread to invoke a (remote) 
    /// action and wait for the result. 
    template <typename Result, int N>
    class future_value : public lcos::base_lco_with_value<Result>
    {
    private:
        // make sure N is in a reasonable range
        BOOST_STATIC_ASSERT(N > 0);   // N must be greater than zero

    private:
        typedef Result result_type;
        typedef std::pair<boost::system::error_code, std::string> error_type;
        typedef boost::variant<result_type, error_type> data_type;

    public:
        // This is the component id. Every component needs to have an embedded
        // enumerator 'value' which is used by the generic action implementation
        // to associate this component with a given action.
        enum { value = components::component_future };

        future_value()
        {
        }

        /// Reset the future_value to allow to restart an asynchronous 
        /// operation. Allows any subsequent set_data operation to succeed.
        void reset()
        {
            data_->set_empty();
        }

        /// Get the result of the requested action. This call blocks (yields 
        /// control) if the result is not ready. As soon as the result has been 
        /// returned and the waiting thread has been re-scheduled by the thread
        /// manager the function \a lazy_future#get will return.
        ///
        /// \param slot   [in] The number of the slot the value has to be 
        ///               returned for. This number must be positive, but 
        ///               smaller than the template parameter \a N.
        /// \param self   [in] The \a thread which will be unconditionally
        ///               blocked (yielded) while waiting for the result. 
        ///
        /// \note         If there has been an error reported (using the action
        ///               \a base_lco#set_error), this function will throw an
        ///               exception encapsulating the reported error code and 
        ///               error description.
        Result get_data(int slot) 
        {
            if (slot < 0 || slot >= N) {
                HPX_THROW_EXCEPTION(bad_parameter, "slot index out of range");
                return Result();
            }

            // yields control if needed
            data_type d;
            data_[slot].read(d);

            // the thread has been re-activated by one of the actions 
            // supported by this future_value (see \a future_value::set_event
            // and future_value::set_error).
            if (1 == d.which())
            {
                // an error has been reported in the meantime, throw 
                error_type e = boost::get<error_type>(d);
                HPX_THROW_EXCEPTION_EX(
                    boost::system::system_error, e.first, e.second);
            }

            // no error has been reported, return the result
            return boost::get<result_type>(d);
        };

        // helper functions for setting data (if successful) or the error (if
        // non-successful)
        void set_data(int slot, Result const& result)
        {
            // set the received result, reset error status
            if (slot < 0 || slot >= N) {
                HPX_THROW_EXCEPTION(bad_parameter, "slot index out of range");
                return;
            }

            // store the value
            data_[slot].set(data_type(result));
        }

        // trigger the future with the given error condition
        void set_error (int slot, hpx::error code, std::string const& msg)
        {
            if (slot < 0 || slot >= N) {
                HPX_THROW_EXCEPTION(bad_parameter, "slot index out of range");
                return;
            }

            // store the error code
            data_[slot].set(data_type(error_type(make_error_code(code), msg)));
        }

        ///////////////////////////////////////////////////////////////////////
        // exposed functionality of this component

        // trigger the future, set the result
        threads::thread_state 
        set_result (applier::applier& appl, Result const& result)
        {
            // set the received result, reset error status
            set_data(0, result);

            // this thread has nothing more to do
            return threads::terminated;
        }

        // trigger the future with the given error condition
        threads::thread_state set_error (applier::applier& appl, hpx::error code, 
            std::string const& msg)
        {
            set_error(0, code, msg);

            // this thread has nothing more to do
            return threads::terminated;
        }

    private:
        util::full_empty<data_type> data_[N];
    };

    /// A future_value can be used by a single thread to invoke a (remote) 
    /// action and wait for the operation to finish. No result is expected.
    template <int N>
    class future_value<void, N> : public lcos::base_lco
    {
    private:
        // make sure N is in a reasonable range
        BOOST_STATIC_ASSERT(N > 0);   // N must be greater than zero

    private:
        struct void_type 
        {
            void_type() {}
        };

        typedef void_type result_type;
        typedef std::pair<boost::system::error_code, std::string> error_type;
        typedef boost::variant<result_type, error_type> data_type;

    public:
        // This is the component id. Every component needs to have an embedded
        // enumerator 'value' which is used by the generic action implementation
        // to associate this component with a given action.
        enum { value = components::component_future };

        future_value()
        {
        }

        /// Reset the future_value to allow to restart an asynchronous 
        /// operation. Allows any subsequent set_data operation to succeed.
        void reset()
        {
            data_->set_empty();
        }

        /// Get the result of the requested action. This call blocks (yields 
        /// control) if the result is not ready. As soon as the result has been 
        /// returned and the waiting thread has been re-scheduled by the thread
        /// manager the function \a lazy_future#get will return.
        ///
        /// \param slot   [in] The number of the slot the value has to be 
        ///               returned for. This number must be positive, but 
        ///               smaller than the template parameter \a N.
        /// \param self   [in] The \a thread which will be unconditionally
        ///               blocked (yielded) while waiting for the result. 
        ///
        /// \note         If there has been an error reported (using the action
        ///               \a base_lco#set_error), this function will throw an
        ///               exception encapsulating the reported error code and 
        ///               error description.
        void get_data(int slot) 
        {
            if (slot < 0 || slot >= N) {
                HPX_THROW_EXCEPTION(bad_parameter, "slot index out of range");
                return;
            }

            // yields control if needed
            data_type d;
            data_[slot].read(d);

            // the thread has been re-activated by one of the actions 
            // supported by this future_value (see \a future_value::set_event
            // and future_value::set_error).
            if (1 == d.which())
            {
                // an error has been reported in the meantime, throw 
                error_type e = boost::get<error_type>(d);
                HPX_THROW_EXCEPTION_EX(
                    boost::system::system_error, e.first, e.second);
            }

            // no error has been reported, just return 
        };

        // helper functions for setting data (if successful) or the error (if
        // non-successful)
        void set_data(int slot)
        {
            // set the received result, reset error status
            if (slot < 0 || slot >= N) {
                HPX_THROW_EXCEPTION(bad_parameter, "slot index out of range");
                return;
            }

            // store the value
            data_[slot].set(data_type(result_type()));
        }

        // trigger the future with the given error condition
        void set_error (int slot, hpx::error code, std::string const& msg)
        {
            if (slot < 0 || slot >= N) {
                HPX_THROW_EXCEPTION(bad_parameter, "slot index out of range");
                return;
            }

            // store the error code
            data_[slot].set(data_type(error_type(make_error_code(code), msg)));
        }

        ///////////////////////////////////////////////////////////////////////
        // exposed functionality of this component

        // trigger the future, set the result
        threads::thread_state set_event (applier::applier& appl)
        {
            // set the received result, reset error status
            set_data(0);

            // this thread has nothing more to do
            return threads::terminated;
        }

        // trigger the future with the given error condition
        threads::thread_state set_error (applier::applier& appl,
            hpx::error code, std::string const& msg)
        {
            set_error(0, code, msg);

            // this thread has nothing more to do
            return threads::terminated;
        }

    private:
        util::full_empty<data_type> data_[N];
    };

}}}

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace lcos 
{
    ///////////////////////////////////////////////////////////////////////////
    /// \class future_value future_value.hpp hpx/lcos/future_value.hpp
    ///
    /// A future_value can be used by a single \a thread to invoke a 
    /// (remote) action and wait for the result. The result is expected to be 
    /// sent back to the future_value using the LCO's set_event action
    ///
    /// A future_value is one of the simplest synchronization primitives 
    /// provided by HPX. It allows to synchronize on a eager evaluated remote
    /// operation returning a result of the type \a Result. The \a future_value
    /// allows to synchronize exactly one \a thread (the one passed during 
    /// construction time).
    ///
    /// \code
    ///     // Create the future_value (the expected result is a id_type)
    ///     lcos::future_value<naming::id_type> f;
    ///
    ///     // initiate the action supplying the future_value as a 
    ///     // continuation
    ///     applier_.appy<some_action>(new continuation(f.get_gid()), ...);
    ///
    ///     // Wait for the result to be returned, yielding control 
    ///     // in the meantime.
    ///     naming::id_type result = f.get(thread_self);
    ///     // ...
    /// \endcode
    ///
    /// \tparam Result   The template parameter \a Result defines the type this 
    ///                  future_value is expected to return from 
    ///                  \a future_value#get.
    ///
    /// \note            The action executed using the future_value as a 
    ///                  continuation must return a value of a type convertible 
    ///                  to the type as specified by the template parameter 
    ///                  \a Result
    template <typename Result, int N>
    class future_value 
    {
    protected:
        typedef detail::future_value<Result, N> wrapped_type;
        typedef components::managed_component<wrapped_type> wrapping_type;

        /// Construct a new \a future instance. The supplied 
        /// \a thread will be notified as soon as the result of the 
        /// operation associated with this future instance has been 
        /// returned.
        /// 
        /// \note         The result of the requested operation is expected to 
        ///               be returned as the first parameter using a 
        ///               \a base_lco#set_result action. Any error has to be 
        ///               reported using a \a base_lco::set_error action. The 
        ///               target for either of these actions has to be this 
        ///               future instance (as it has to be sent along 
        ///               with the action as the continuation parameter).
        future_value()
          : impl_(new wrapping_type(new wrapped_type()))
        {}

        /// \brief Return the global id of this \a future instance
        naming::id_type get_gid(applier::applier& appl) const
        {
            return impl_->get_gid(appl);
        }

        /// Reset the future_value to allow to restart an asynchronous 
        /// operation. Allows any subsequent set_data operation to succeed.
        void reset()
        {
            (*impl_)->reset();
        }

    public:
        ~future_value()
        {}

        /// Get the result of the requested action. This call blocks (yields 
        /// control) if the result is not ready. As soon as the result has been 
        /// returned and the waiting thread has been re-scheduled by the thread
        /// manager the function \a eager_future#get will return.
        ///
        /// \param self   [in] The \a thread which will be unconditionally
        ///               blocked (yielded) while waiting for the result. 
        ///
        /// \note         If there has been an error reported (using the action
        ///               \a base_lco#set_error), this function will throw an
        ///               exception encapsulating the reported error code and 
        ///               error description.
        Result get(int slot = 0) const
        {
            return (*impl_)->get_data(slot);
        }

    protected:
        boost::shared_ptr<wrapping_type> impl_;
    };

}}

#endif
