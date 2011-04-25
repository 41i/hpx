//  Copyright (c) 2007-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_LCOS_QUEUE_FEB_10_2011_1232PM)
#define HPX_LCOS_QUEUE_FEB_10_2011_1232PM

#include <hpx/runtime/components/client_base.hpp>
#include <hpx/lcos/stubs/queue.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace lcos 
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename ValueType, typename RemoteType = ValueType>
    class queue 
      : public components::client_base<queue<ValueType, RemoteType>, 
            lcos::stubs::queue<ValueType, RemoteType> >
    {
        typedef components::client_base<
            queue, lcos::stubs::queue<ValueType, RemoteType> > base_type;

    public:
        queue() 
        {}

        /// Create a client side representation for the existing
        /// \a server#queue instance with the given global id \a gid.
        queue(naming::id_type gid) 
          : base_type(gid)
        {}

        ///////////////////////////////////////////////////////////////////////
        // exposed functionality of this component

        lcos::future_value<ValueType, RemoteType> 
        get_value_async()
        {
            BOOST_ASSERT(gid_);
            return this->base_type::get_value_async(gid_);
        }

        lcos::future_value<void> 
        set_value_async(RemoteType const& val)
        {
            BOOST_ASSERT(gid_);
            return this->base_type::set_value_async(gid_, val);
        }

        lcos::future_value<void, util::unused_type> 
        abort_pending_async(boost::exception_ptr const& e)
        {
            BOOST_ASSERT(gid_);
            return this->base_type::abort_pending_async(gid_, e);
        }

        ///////////////////////////////////////////////////////////////////////
        ValueType get_value_sync()
        {
            BOOST_ASSERT(gid_);
            return this->base_type::get_value_sync(gid_);
        }

        void set_value_sync(RemoteType const& val)
        {
            BOOST_ASSERT(gid_);
            this->base_type::set_value_sync(gid_, val);
        }

        void abort_pending_sync(boost::exception_ptr const& e)
        {
            this->base_type::abort_pending_sync(gid_, e);
        }

        ///////////////////////////////////////////////////////////////////////
        void set_value(RemoteType const& val)
        {
            this->base_type::set_value(gid_, val);
        }

        void abort_pending()
        {
            try {
                HPX_THROW_EXCEPTION(no_success, "queue::set_error", 
                    "interrupt all pending requests");
            }
            catch (...) {
                this->base_type::abort_pending(gid_, boost::current_exception());
            }
        }
    };
}}

#endif

