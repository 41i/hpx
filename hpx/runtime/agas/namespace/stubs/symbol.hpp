////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Bryce Lelbach
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////

#if !defined(HPX_28443929_CB68_43ED_B134_F60602A344DD)
#define HPX_28443929_CB68_43ED_B134_F60602A344DD

#include <hpx/hpx_fwd.hpp>
#include <hpx/lcos/eager_future.hpp>
#include <hpx/runtime/components/stubs/stub_base.hpp>
#include <hpx/runtime/agas/traits.hpp>
#include <hpx/runtime/agas/namespace/server/symbol.hpp>

namespace hpx { namespace agas { namespace stubs
{

template <typename Server>
struct symbol_namespace_base : components::stubs::stub_base<Server>
{
    // {{{ nested types
    typedef components::stubs::stub_base<Server> base_type;
    typedef Server server_type; 

    typedef typename server_type::symbol_type symbol_type;
    // }}}

    // {{{ bind dispatch
    static lcos::future_value<bool>
    bind_async(naming::id_type const& gid, symbol_type const& key,
               naming::gid_type const& value)
    {
        typedef typename server_type::bind_action action_type;
        return lcos::eager_future<action_type, bool>(gid, key, value);
    }

    static bool bind(naming::id_type const& gid, symbol_type const& key,
                     naming::gid_type const& value)
    { return bind_async(gid, key, value).get(); } 
    // }}}
    
    // {{{ rebind dispatch
    static lcos::future_value<naming::gid_type>
    rebind_async(naming::id_type const& gid, symbol_type const& key,
                 naming::gid_type const& value)
    {
        typedef typename server_type::rebind_action action_type;
        return lcos::eager_future<action_type, naming::gid_type>  
            (gid, key, value);
    }

    static naming::gid_type
    rebind(naming::id_type const& gid, symbol_type const& key,
           naming::gid_type const& value)
    { return rebind_async(gid, key, value).get(); } 
    // }}}

    // {{{ resolve dispatch 
    static lcos::future_value<naming::gid_type>
    resolve_async(naming::id_type const& gid, symbol_type const& key)
    {
        typedef typename server_type::resolve_action action_type;
        return lcos::eager_future<action_type, naming::gid_type>(gid, key);
    }
    
    static naming::gid_type
    resolve(naming::id_type const& gid, symbol_type const& key)
    { return resolve_async(gid, key).get(); } 
    // }}}

    // {{{ unbind dispatch 
    static lcos::future_value<bool>
    unbind_async(naming::id_type const& gid, symbol_type const& key)
    {
        typedef typename server_type::unbind_action action_type;
        return lcos::eager_future<action_type, bool>(gid, key);
    }
    
    static bool unbind(naming::id_type const& gid, symbol_type const& key)
    { return unbind_async(gid, key).get(); } 
    // }}}
};            

template <typename Database> 
struct symbol_namespace : symbol_namespace_base<
    server::symbol_namespace<Database>
> { };

template <typename Database> 
struct bootstrap_symbol_namespace : symbol_namespace_base<
    server::bootstrap_symbol_namespace<Database>
> { };

}}}

#endif // HPX_28443929_CB68_43ED_B134_F60602A344DD

