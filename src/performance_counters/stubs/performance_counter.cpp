//  Copyright (c) 2007-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx_fwd.hpp>
#include <hpx/performance_counters/stubs/performance_counter.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace performance_counters { namespace stubs
{
    lcos::future_value<counter_info> performance_counter::get_info_async(
        naming::gid_type const& targetgid)
    {
        // Create an eager_future, execute the required action,
        // we simply return the initialized future_value, the caller needs
        // to call get() on the return value to obtain the result
        typedef server::base_performance_counter::get_counter_info_action action_type;
        return lcos::eager_future<action_type, counter_info>(targetgid);
    }

    lcos::future_value<counter_value> performance_counter::get_value_async(
        naming::gid_type const& targetgid)
    {
        // Create an eager_future, execute the required action,
        // we simply return the initialized future_value, the caller needs
        // to call get() on the return value to obtain the result
        typedef server::base_performance_counter::get_counter_value_action action_type;
        return lcos::eager_future<action_type, counter_value>(targetgid);
    }

    counter_info performance_counter::get_info(naming::gid_type const& targetgid)
    {
        return get_info_async(targetgid).get();
    }

    counter_value performance_counter::get_value(naming::gid_type const& targetgid)
    {
        return get_value_async(targetgid).get();
    }
}}}
