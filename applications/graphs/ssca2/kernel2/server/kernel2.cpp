//  Copyright (c) 2009-2010 Dylan Stark
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx.hpp>
#include <hpx/lcos/future_wait.hpp>
#include <hpx/runtime/components/component_factory.hpp>

#include <hpx/util/portable_binary_iarchive.hpp>
#include <hpx/util/portable_binary_oarchive.hpp>

#include <boost/serialization/version.hpp>
#include <boost/serialization/export.hpp>
#include <hpx/components/distributing_factory/distributing_factory.hpp>

#include <hpx/components/graph/graph.hpp>
#include <hpx/components/vertex/vertex.hpp>
#include <hpx/components/distributed_list/distributed_list.hpp>
#include <hpx/components/distributed_list/server/distributed_list.hpp>
#include <hpx/components/distributed_list/local_list.hpp>
#include <hpx/components/distributed_list/server/local_list.hpp>

#include <hpx/lcos/eager_future.hpp>
#include <hpx/lcos/reduce_max.hpp>

#include "kernel2.hpp"
#include "../stubs/kernel2.hpp"

///////////////////////////////////////////////////////////////////////////////
typedef hpx::components::server::kernel2 kernel2_type;
typedef hpx::components::server::kernel2::edge_list_type edge_list_type;

///////////////////////////////////////////////////////////////////////////////
// Serialization support for the kernel2 actions
HPX_REGISTER_ACTION_EX(kernel2_type::large_set_action,
                       kernel2_large_set_action);
HPX_REGISTER_ACTION_EX(kernel2_type::large_set_local_action,
                       kernel2_large_set_local_action);
HPX_REGISTER_MINIMAL_COMPONENT_FACTORY(
    hpx::components::simple_component<kernel2_type>, kernel2);
HPX_DEFINE_GET_COMPONENT_TYPE(kernel2_type);

typedef hpx::lcos::base_lco_with_value<
        kernel2_type::edge_list_type
    > create_edge_list_type;

HPX_REGISTER_ACTION_EX(
    create_edge_list_type::set_result_action,
    set_result_action_kernel2_result);
HPX_DEFINE_GET_COMPONENT_TYPE(create_edge_list_type);

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace components { namespace server
{
    kernel2::kernel2() {}
    
    int
    kernel2::large_set(naming::id_type G, naming::id_type dist_edge_list)
    {
        std::cout << "Starting Kernel 2" << std::endl;

        typedef components::distributing_factory::result_type result_type;

        int total_added = 0;
        edge_list_type edge_list;
        std::vector<lcos::future_value<int> > local_searches;

        // Get vertex set of G
        result_type vertices =
            lcos::eager_future<graph::vertices_action>(G).get();

        // Create lcos to reduce max value
        lcos::reduce_max local_max(vertices.size(), 1);
        lcos::reduce_max global_max(1, vertices.size());

        // Spawn searches on local sub lists
        result_type::const_iterator end = vertices.end();
        for (result_type::const_iterator it = vertices.begin();
             it != end; ++it)
        {
            local_searches.push_back(
                lcos::eager_future<large_set_local_action>((*it).prefix_,
                    *it,
                    dist_edge_list,
                    local_max.get_gid(),
                    global_max.get_gid()));
        }

        // Get the (reduced) global maximum, and signal it back to
        // local searches
        typedef hpx::lcos::detail::reduce_max::wait_action wait_action;
        global_max.signal(
            lcos::eager_future<wait_action>(local_max.get_gid()).get());

        // Run through local search results to tally total edges added
        while (local_searches.size() > 0)
        {
            total_added += local_searches.back().get();
            local_searches.pop_back();
        }

        std::cout << "Large set of " << total_added << std::endl;

        return total_added;
    }
    
    int
    kernel2::large_set_local(locality_result local_vertex_list,
                             naming::id_type edge_list,
                             naming::id_type local_max_lco,
                             naming::id_type global_max_lco)
    {
        std::cout << "Processing local set" << std::endl;

        int max = -1;
        int num_added = 0;
        kernel2::edge_list_type edge_list_local;

        // Iterate over local edges
        naming::id_type gid = local_vertex_list.first_gid_;
        for (std::size_t cnt = 0; cnt < local_vertex_list.count_; ++cnt, ++gid)
        {
            // Get incident edges from this vertex
            typedef vertex::partial_edge_list_type partial_type;
            partial_type partials =
                lcos::eager_future<vertex::out_edges_action>(gid).get();

            // Iterate over incident edges
            partial_type::iterator end = partials.end();
            for (partial_type::iterator it = partials.begin(); it != end; ++it)
            {
                if ((*it).label_ > max)
                {
                    edge_list_local.clear();
                    edge_list_local.push_back(
                        edge(gid, (*it).target_, (*it).label_));
                    max = (*it).label_;
                }
                else if ((*it).label_ == max)
                {
                    edge_list_local.push_back(
                        edge(gid, (*it).target_, (*it).label_));
                }
            }
        }

        std::cout << "Max on locality " << local_vertex_list.prefix_ << " "
                  << "is " << max << ", total of " << edge_list_local.size()
                  << std::endl;

        // Signal local maximum
        applier::apply<
            lcos::detail::reduce_max::signal_action
        >(local_max_lco, max);

        std::cout << "Waiting to see max is on locality "
                  << local_vertex_list.prefix_ << std::endl;

        // Wait for global maximum
        int global_max =
            lcos::eager_future<
                hpx::lcos::detail::reduce_max::wait_action
            >(global_max_lco).get();

        // Add local max edge set if it is globally maximal
        if (max == global_max)
        {
            std::cout << "Adding local edge set at "
                      << local_vertex_list.prefix_ << std::endl;

            typedef distributed_list<kernel2::edge_list_type>
                distributed_edge_list_type;
            typedef local_list<kernel2::edge_list_type> local_edge_list_type;

            naming::id_type local_list =
                lcos::eager_future<
                    distributed_edge_list_type::get_local_action
                >(edge_list, local_vertex_list.prefix_).get();

            num_added =
                lcos::eager_future<
                    local_edge_list_type::append_action
                >(local_list, edge_list_local).get();
        }
        else
        {
            std::cout << "Not adding local edge set at "
                      << local_vertex_list.prefix_ << std::endl;
        }

        return num_added;
    }

}}}
