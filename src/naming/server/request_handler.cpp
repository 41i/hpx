//  Copyright (c) 2007-2008 Hartmut Kaiser
//
//  Parts of this code were taken from the Boost.Asio library
//  Copyright (c) 2003-2007 Christopher M. Kohlhoff (chris at kohlhoff dot com)
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <string>
#include <boost/lexical_cast.hpp>

#include <hpx/util/dgas_logging.hpp>
#include <hpx/naming/server/reply.hpp>
#include <hpx/naming/server/request.hpp>
#include <hpx/naming/server/request_handler.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace naming { namespace server 
{
    ///////////////////////////////////////////////////////////////////////////
    //  Handle conversion to/from prefix
    inline id_type get_id_from_prefix(boost::uint16_t prefix)
    {
        return boost::uint64_t(prefix) << 48;
    }
    
    inline boost::uint16_t get_prefix_from_id(id_type id)
    {
        return boost::uint16_t(id >> 48);
    }
    
    inline bool is_prefix_only(id_type id)
    {
        return (id & 0xFFFFFFFFFFFF) ? false : true;
    }
    
    ///////////////////////////////////////////////////////////////////////////
    request_handler::request_handler()
      : totals_(command_lastcommand)
    {
    }

    request_handler::~request_handler()
    {
    }
    
    ///////////////////////////////////////////////////////////////////////////
    void request_handler::handle_getprefix(request const& req, reply& rep)
    {
        try {
            mutex_type::scoped_lock l(mtx_);
            site_prefix_map_type::iterator it = 
                site_prefixes_.find(req.get_site());
            if (it != site_prefixes_.end()) {
                // The real prefix has to be used as the 16 most 
                // significant bits of global id's
                
                // existing entry
                rep = reply(repeated_request, command_getprefix, 
                    get_id_from_prefix((*it).second.first)); 
            }
            else {
                // insert this prefix as being mapped to the given locality
                boost::uint16_t prefix = (boost::uint16_t)site_prefixes_.size() + 1;
                id_type id = get_id_from_prefix(prefix);
                site_prefixes_.insert(
                    site_prefix_map_type::value_type(req.get_site(), 
                        std::make_pair(prefix, id)));

                // now, bind this prefix to the locality address allowing to 
                // send parcels to a locality
                registry_type::iterator it = registry_.find(id);
                if (it != registry_.end()) {
                    // this shouldn't happen
                    rep = reply(command_getprefix, no_success, 
                        "prefix is already bound to local address");
                }
                else {
                    registry_.insert(
                        registry_type::value_type(id, address(req.get_site())));
                }

                // The real prefix has to be used as the 16 most 
                // significant bits of global id's
                
                // created new entry
                rep = reply(success, command_getprefix, id);
            }
        }
        catch (std::bad_alloc) {
            rep = reply(command_getprefix, out_of_memory);
        }            
        catch (...) {
            rep = reply(command_getprefix, internal_server_error);
        }            
    }

    void request_handler::handle_getidrange(request const& req, reply& rep)
    {
        try {
            mutex_type::scoped_lock l(mtx_);
            site_prefix_map_type::iterator it = 
                site_prefixes_.find(req.get_site());
            if (it != site_prefixes_.end()) {
                // The real prefix has to be used as the 16 most 
                // significant bits of global id's
                
                // generate the new id range
                boost::uint64_t lower = (*it).second.second + 1;
                boost::uint64_t upper = lower + range_delta;
                
                // store the new lower bound
                (*it).second.second = upper;

                // existing entry
                rep = reply(success, command_getidrange, lower, upper); 
            }
            else {
                // insert this prefix as being mapped to the given locality
                boost::uint16_t prefix = (boost::uint16_t)site_prefixes_.size() + 1;
                id_type id = get_id_from_prefix(prefix);
                std::pair<site_prefix_map_type::iterator, bool> p=
                    site_prefixes_.insert(
                        site_prefix_map_type::value_type(req.get_site(), 
                            std::make_pair(prefix, id)));

                // now, bind this prefix to the locality address allowing to 
                // send parcels to a locality
                registry_type::iterator it = registry_.find(id);
                if (it != registry_.end()) {
                    // this shouldn't happen
                    rep = reply(command_getprefix, no_success, 
                        "prefix is already bound to local address");
                }
                else {
                    registry_.insert(
                        registry_type::value_type(id, address(req.get_site())));
                }

                // generate the new id range
                boost::uint64_t lower = id + 1;
                boost::uint64_t upper = lower + range_delta;
                
                // store the new lower bound
                (*p.first).second.second = upper;
                
                // created new entry
                rep = reply(success, command_getidrange, lower, upper);
            }
        }
        catch (std::bad_alloc) {
            rep = reply(command_getprefix, out_of_memory);
        }            
        catch (...) {
            rep = reply(command_getprefix, internal_server_error);
        }            
    }

    void request_handler::handle_bind(request const& req, reply& rep)
    {
        try {
            error s = no_success;
            {
                mutex_type::scoped_lock l(mtx_);
                registry_type::iterator it = registry_.find(req.get_id());
                if (it != registry_.end())
                    (*it).second = req.get_address();
                else {
                    registry_.insert(registry_type::value_type(
                        req.get_id(), req.get_address()));
                    s = success;    // created new entry
                }
            }
            rep = reply(command_bind, s);
        }
        catch (std::bad_alloc) {
            rep = reply(command_bind, out_of_memory);
        }            
        catch (...) {
            rep = reply(command_bind, internal_server_error);
        }            
    }

    void request_handler::handle_unbind(request const& req, reply& rep)
    {
        try {
            error s = no_success;
            {
                mutex_type::scoped_lock l(mtx_);
                registry_type::iterator it = registry_.find(req.get_id());
                if (it != registry_.end()) {
                    registry_.erase(it);
                    s = success;
                }
            }
            rep = reply(command_unbind, s);
        }
        catch (std::bad_alloc) {
            rep = reply(command_unbind, out_of_memory);
        }            
        catch (...) {
            rep = reply(command_unbind, internal_server_error);
        }            
    }

    void request_handler::handle_resolve(request const& req, reply& rep)
    {
        try {
            mutex_type::scoped_lock l(mtx_);
            registry_type::iterator it = registry_.find(req.get_id());
            if (it != registry_.end()) {
                rep = reply(command_resolve, (*it).second);
            }
            else {
                rep = reply(command_resolve, no_success);
            }
        }
        catch (std::bad_alloc) {
            rep = reply(command_resolve, out_of_memory);
        }            
        catch (...) {
            rep = reply(command_resolve, internal_server_error);
        }            
    }

    void request_handler::handle_queryid(request const& req, reply& rep)
    {
        try {
            mutex_type::scoped_lock l(mtx_);
            ns_registry_type::iterator it = ns_registry_.find(req.get_name());
            if (it != ns_registry_.end()) 
                rep = reply(command_queryid, (*it).second);
            else
                rep = reply(command_queryid, no_success);
        }
        catch (std::bad_alloc) {
            rep = reply(command_queryid, out_of_memory);
        }            
        catch (...) {
            rep = reply(command_queryid, internal_server_error);
        }            
    }

    void request_handler::handle_registerid(request const& req, reply& rep)
    {
        try {
            error s = no_success;
            {
                mutex_type::scoped_lock l(mtx_);
                ns_registry_type::iterator it = ns_registry_.find(req.get_name());
                if (it != ns_registry_.end())
                    (*it).second = req.get_id();
                else {
                    ns_registry_.insert(ns_registry_type::value_type(
                        req.get_name(), req.get_id()));
                    s = success;    // created new entry
                }
            }
            rep = reply(command_registerid, s);
        }
        catch (std::bad_alloc) {
            rep = reply(command_registerid, out_of_memory);
        }            
        catch (...) {
            rep = reply(command_registerid, internal_server_error);
        }            
    }

    void request_handler::handle_unregisterid(request const& req, reply& rep)
    {
        try {
            error s = no_success;
            {
                mutex_type::scoped_lock l(mtx_);
                ns_registry_type::iterator it = ns_registry_.find(req.get_name());
                if (it != ns_registry_.end()) {
                    ns_registry_.erase(it);
                    s = success;
                }
            }            
            rep = reply(command_unregisterid, s);
        }
        catch (std::bad_alloc) {
            rep = reply(command_unregisterid, out_of_memory);
        }            
        catch (...) {
            rep = reply(command_unregisterid, internal_server_error);
        }            
    }

    void request_handler::handle_statistics(request const& req, reply& rep)
    {
        try {
            rep = reply(command_statistics, totals_);
        }
        catch (std::bad_alloc) {
            rep = reply(command_statistics, out_of_memory);
        }            
        catch (...) {
            rep = reply(command_statistics, internal_server_error);
        }            
    }

    ///////////////////////////////////////////////////////////////////////////
    void request_handler::handle_request(request const& req, reply& rep)
    {
        LDGAS_(info) << "request: " << req;
        switch (req.get_command()) {
        case command_getprefix:
            handle_getprefix(req, rep);
            break;
            
        case command_getidrange:
            handle_getidrange(req, rep);
            break;
            
        case command_bind:
            handle_bind(req, rep);
            break;
            
        case command_unbind:
            handle_unbind(req, rep);
            break;
            
        case command_resolve:
            handle_resolve(req, rep);
            break;
            
        case command_queryid:
            handle_queryid(req, rep);
            break;
            
        case command_registerid:
            handle_registerid(req, rep);
            break;
            
        case command_unregisterid:
            handle_unregisterid(req, rep);
            break;
            
        case command_statistics:
            handle_statistics(req, rep);
            break;
            
        default:
            rep = reply(bad_request);
            break;
        }
        
        if (rep.get_status() != success && rep.get_status() != repeated_request) {
            LDGAS_(error) << "response: " << rep;
        }
        else {
            LDGAS_(info) << "response: " << rep;
        }
    }

}}}  // namespace hpx::naming::server

