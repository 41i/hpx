//  Copyright (c) 2007-2011 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/lcos/eager_future.hpp>
#include <hpx/runtime/applier/apply.hpp>
#include <hpx/runtime/components/console_error_sink.hpp>
#include <hpx/runtime/components/server/console_error_sink.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace components
{
    // Stub function which applies the console_error_sink action.
    void console_error_sink(naming::id_type const& dst,
        boost::exception_ptr const& e)
    {
        // Report the error only if the threadmanager is up. 
        if (threads::threadmanager_is(running))
        {
            if (threads::get_self_ptr())
            {
                lcos::eager_future<server::console_error_sink_action> f(dst, e);
                f.get();
            }
            else
            {
                // FIXME: This should use a sync_put_parcel.
                applier::apply<server::console_error_sink_action>(dst, e);
            }
        }
    }
}}

