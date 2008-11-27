//  Copyright (c) 2008-2009 Chirag Dekate, Hartmut Kaiser, Anshul Tandon
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx_fwd.hpp>
#include <hpx/runtime/components/component_type.hpp>
#include <hpx/runtime/threads/thread.hpp>
#include <boost/coroutine/detail/coroutine_impl_impl.hpp>

#include <boost/assert.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace threads { namespace detail
{
    components::component_type thread::get_component_type()
    {
        return components::component_thread;
    }

    void thread::set_component_type(components::component_type type)
    {
        BOOST_ASSERT(false);    // shouldn't be called, ever
    }

}}}

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace threads 
{
    thread_self& get_self()
    {
        return *thread_self::impl_type::get_self();
    }

    thread_self* get_self_ptr()
    {
        return thread_self::impl_type::get_self();
    }

}}

///////////////////////////////////////////////////////////////////////////////
// explicit instantiation of the function thread_self::set_self
template void hpx::threads::thread_self::impl_type::set_self(hpx::threads::thread_self*);

