////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Bryce Lelbach
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////

#if !defined(HPX_E5F37926_4705_48AE_AE28_844E5ED05B9F)
#define HPX_E5F37926_4705_48AE_AE28_844E5ED05B9F

#include <hpx/hpx_fwd.hpp>
#include <hpx/config.hpp>
#include <hpx/runtime/applier/applier.hpp>
#include <hpx/runtime/naming/name.hpp>
#include <hpx/runtime/naming/address.hpp>
#include <hpx/runtime/naming/locality.hpp>
#include <hpx/runtime/agas/network/backend/tcpip.hpp>
#include <hpx/runtime/agas/namespace/bootstrap_component.hpp>
#include <hpx/runtime/agas/namespace/bootstrap_primary.hpp>
#include <hpx/runtime/agas/namespace/bootstrap_symbol.hpp>
#include <hpx/runtime/agas/client/legacy/resolver_cache.hpp>

namespace hpx { namespace agas { namespace legacy
{

// TODO: pass error codes once they're implemented in AGAS.
template <typename Database>
struct bootstrap_base : resolver_cache<tag::network::tcpip>
{
  protected:
    // {{{ types
    typedef resolver_cache<tag::network::tcpip> base_type;

    typedef bootstrap_primary_namespace<Database, tag::network::tcpip>
        primary_namespace_type;

    typedef bootstrap_component_namespace<Database> component_namespace_type;
    typedef bootstrap_symbol_namespace<Database> symbol_namespace_type;

    typedef typename primary_namespace_type::server_type
        primary_namespace_server_type; 

    typedef typename component_namespace_type::server_type
        component_namespace_server_type; 

    typedef typename symbol_namespace_type::server_type
        symbol_namespace_server_type; 

    typedef base_type::gva_cache_key gva_cache_key;
    typedef base_type::gva_type::endpoint_type endpoint_type;
    // }}}

    primary_namespace_type primary_ns_;
    component_namespace_type component_ns_;
    symbol_namespace_type symbol_ns_;

    bootstrap_base(util::runtime_configuration const& ini_, runtime_mode mode)
        : base_type(ini_),
          primary_ns_server(new primary_namespace_server_type),
          component_ns_server(new component_namespace_server_type), 
          symbol_ns_server(new symbol_namespace_server_type)
    {
        using boost::asio::ip::address;

        naming::locality l = ini_.get_agas_locality();

        address addr = address::from_string(l.get_address());
        endpoint_type ep(addr, l.get_port()); 

        gva_type primary_gva(ep,
            primary_namespace_server_type::get_component_type(), 1U,
                static_cast<void*>(primary_ns_server.get()));
        gva_type component_gva(ep,
            component_namespace_server_type::get_component_type(), 1U,
                static_cast<void*>(component_ns_server.get()));
        gva_type symbol_gva(ep,
            symbol_namespace_server_type::get_component_type(), 1U,
                static_cast<void*>(symbol_ns_server.get()));

        gva_cache_key primary_key(primary_ns_server->get_base_gid(), 1U);
        gva_cache_key component_key(component_ns_server->get_base_gid(), 1U);
        gva_cache_key symbol_key(symbol_ns_server->get_base_gid(), 1U);

        this->locality_cache_.insert(l, 
            naming::get_gid_from_prefix(HPX_AGAS_BOOTSTRAP_PREFIX));
        this->symbol_ns_server->bind("/locality(agas#0)",
            naming::get_gid_from_prefix(HPX_AGAS_BOOTSTRAP_PREFIX)); 
        this->primary_ns_server->bind_locality(ep, 3);

        this->gva_cache_.insert(primary_key, primary_gva);
        this->gva_cache_.insert(component_key, component_gva); 
        this->gva_cache_.insert(symbol_key, symbol_gva);
        this->primary_ns_server->bind_gid
            (primary_ns_server->get_base_gid(), primary_gva);
        this->primary_ns_server->bind_gid
            (component_ns_server->get_base_gid(), component_gva);
        this->primary_ns_server->bind_gid
            (symbol_ns_server->get_base_gid(), symbol_gva);

        if (mode == runtime_mode_console)
        { 
            this->console_cache_.store(HPX_AGAS_BOOTSTRAP_PREFIX);
            this->symbol_ns_server->bind("/locality(console)",
                naming::get_gid_from_prefix(HPX_AGAS_BOOTSTRAP_PREFIX)); 
        }
    }

  private:
    template <typename SchedulingPolicy, typename NotificationPolicy> 
    friend class runtime_impl;

    boost::shared_ptr<primary_namespace_server_type> primary_ns_server;
    boost::shared_ptr<component_namespace_server_type> component_ns_server;
    boost::shared_ptr<symbol_namespace_server_type> symbol_ns_server;
};

}}}

#endif // HPX_E5F37926_4705_48AE_AE28_844E5ED05B9F

