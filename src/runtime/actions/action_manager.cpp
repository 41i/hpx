//  Copyright (c) 2008 Anshul Tandon
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx_fwd.hpp>

#include <hpx/runtime/actions/action_manager.hpp>
#include <hpx/runtime/parcelset/parcelhandler.hpp>
#include <hpx/runtime/applier/applier.hpp>
#include <hpx/runtime/actions/action.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace actions
{
    // Call-back function for parcelHandler to call when new parcels are received
    void action_manager::fetch_parcel(
        parcelset::parcelhandler& parcel_handler_, naming::address const& dest)
    {
        parcelset::parcel p;
        if (parcel_handler_.get_parcel(p))  // if new parcel is found
        {
        // write this parcel to the log
            LPT_(info) << "action_manager: fetch_parcel: " << p;

        // decode the local virtual address of the parcel
            naming::address addr = p.get_destination_addr();
            naming::address::address_type lva = addr.address_;

            if (0 == lva) {
            // a zero address references the local runtime support component
                lva = applier_.get_runtime_support_gid().get_lsb();
            }

        // decode the action-type in the parcel
            action_type act = p.get_action();
            continuation_type cont = p.get_continuation();

        // make sure the component_type of the action matches the component
        // type in the destination address
            BOOST_ASSERT(dest.type_ == act->get_component_type());

        // dispatch action, register work item either with or without 
        // continuation support
            if (!cont) {
            // no continuation is to be executed, register the plain action 
            // and the local-virtual address with the TM only
                register_work(applier_, 
                    act->get_thread_function(applier_, lva),
                    act->get_action_name());
            }
            else {
            // this parcel carries a continuation, register a wrapper which
            // first executes the original thread function as required by 
            // the action and triggers the continuations afterwards
                register_work(applier_, 
                    act->get_thread_function(cont, applier_, lva),
                    act->get_action_name());
            }
        }
    }

    // Invoked by the Thread Manager when it is running out of work-items 
    // and needs something to execute on a specific starving resources 
    // specified as the argument
    void action_manager::fetch_parcel (naming::id_type resourceID)
    {

    }
    
///////////////////////////////////////////////////////////////////////////////
}}
