//  Copyright (c) 2005-2008 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx_fwd.hpp>
#include <hpx/util/runtime_configuration.hpp>
#include <hpx/util/init_ini_data.hpp>

#include <boost/assign/std/vector.hpp>
#include <boost/preprocessor/stringize.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace util
{
    ///////////////////////////////////////////////////////////////////////////
    runtime_configuration::runtime_configuration()
    {
        // pre-initialize entries with compile time based values
        using namespace boost::assign;
        std::vector<std::string> lines; 
        lines +=
                "[hpx]",
                "location = " HPX_PREFIX,
                "ini_path = $[hpx.location]/share/hpx/ini",
                "dgas_address = ${HPX_DGAS_SERVER_ADRESS:" HPX_NAME_RESOLVER_ADDRESS "}",
                "dgas_port = ${HPX_DGAS_SERVER_PORT:" BOOST_PP_STRINGIZE(HPX_NAME_RESOLVER_PORT) "}"
            ;
        this->parse("static defaults", lines);

        // try to build default ini structure from shared libraries in default 
        // installation location, this allows to install simple components
        // without the need to install an ini file
        util::init_ini_data_default(HPX_DEFAULT_COMPONENT_PATH, *this);

        // add explicit configuration information if its provided
        if (util::init_ini_data_base(*this)) {
            // merge all found ini files of all components
            util::merge_component_inis(*this);

            // read system and user ini files _again_, to allow the user to 
            // overwrite the settings from the default component ini's. 
            util::init_ini_data_base(*this);
        }
    }

    // DGAS configuration information has to be stored in the global HPX 
    // configuration section:
    // 
    //    [hpx]
    //    dgas_address=<ip address>   # this defaults to HPX_NAME_RESOLVER_ADDRESS
    //    dgas_port=<ip port>         # this defaults to HPX_NAME_RESOLVER_PORT
    //
    naming::locality runtime_configuration::get_dgas_locality()
    {
        // load all components as described in the configuration information
        if (has_section("hpx")) {
            util::section* sec = get_section("hpx");
            if (NULL != sec) {
                std::string cfg_port(
                    sec->get_entry("dgas_port", HPX_NAME_RESOLVER_PORT));

                return naming::locality(
                    sec->get_entry("dgas_address", HPX_NAME_RESOLVER_ADDRESS),
                    boost::lexical_cast<unsigned short>(cfg_port));
            }
        }
        return naming::locality(HPX_NAME_RESOLVER_ADDRESS, HPX_NAME_RESOLVER_PORT);
    }

    naming::locality runtime_configuration::get_dgas_locality(
        std::string default_address, unsigned short default_port)
    {
        // load all components as described in the configuration information
        if (has_section("hpx")) {
            util::section* sec = get_section("hpx");
            if (NULL != sec) {
                // read fall back values from cfg file, if needed
                if (default_address.empty()) {
                    default_address = 
                        sec->get_entry("dgas_address", HPX_NAME_RESOLVER_ADDRESS);
                }
                if (-1 == default_port) {
                    default_port = boost::lexical_cast<unsigned short>(
                        sec->get_entry("dgas_port", HPX_NAME_RESOLVER_PORT));
                }
                return naming::locality(default_address, default_port);
            }
        }
        return naming::locality(HPX_NAME_RESOLVER_ADDRESS, HPX_NAME_RESOLVER_PORT);
    }

}}
