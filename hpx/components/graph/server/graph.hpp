//  Copyright (c) 2007-2009 Dylan Stark
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_COMPONENTS_SERVER_GRAPH_MAY_17_2008_0731PM)
#define HPX_COMPONENTS_SERVER_GRAPH_MAY_17_2008_0731PM

#include <iostream>

#include <hpx/hpx_fwd.hpp>
#include <hpx/runtime/applier/applier.hpp>
#include <hpx/runtime/threads/thread.hpp>
#include <hpx/runtime/components/component_type.hpp>
#include <hpx/runtime/components/server/managed_component_base.hpp>
#include <hpx/runtime/actions/component_action.hpp>

#include <hpx/components/vertex/vertex.hpp>
#include <hpx/components/vertex_list/vertex_list.hpp>
#include <hpx/components/distributed_set/distributed_set.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace components { namespace server 
{
    ///////////////////////////////////////////////////////////////////////////
    /// The graph is an HPX component. 
    ///
    class graph 
      : public components::detail::managed_component_base<graph> 
    {
    public:
        // parcel action code: the action to be performed on the destination 
        // object (the graph)
        enum actions
        {
            graph_init = 0,
            graph_order = 1,
            graph_size = 2,
            graph_add_edge = 3,
            graph_vertex_name = 4,
            graph_vertices = 5
        };
        
        // Typedefs for graph
        typedef int count_t;
        typedef long int vertex_t;
        typedef long int edge_t;

        typedef vertex_list::result_type result_type;

        // constructor: initialize graph value
        graph()
          : size_(0)
        {
            naming::id_type here = applier::get_applier().get_runtime_support_gid();

            // This is going to be phased out
            using hpx::components::vertex_list;
            vertex_list vertices_(vertex_list::create(here));

            vertex_set_ = vertex_set_stub::create(here);
        }

        ///////////////////////////////////////////////////////////////////////
        // exposed functionality of this component

        /// Initialize the graph
        // This is an opt. for when we know the order a priori
        int init(count_t order)
        {            
            std::cout << "Initializing graph of order " << order << std::endl;
            
            // Get list of all known localities
            std::vector<naming::id_type> locales;
            naming::id_type locale;
            applier::applier& appl = applier::get_applier();
            if (appl.get_remote_prefixes(locales))
            {
                locale = locales[0];
            }
            else
            {
                locale = appl.get_runtime_support_gid();
            }
            locales.push_back(appl.get_runtime_support_gid());
            
            // Create a vertex_list and initialize
            components::component_type vertex_type = components::get_component_type<vertex>();
            vertices_.init(vertex_type, order);

            return 0;
        }

        int order(void)
        {
            return vertices_.size();
        }

        int size(void)
        {
            return size_;
        }

        int add_edge(naming::id_type u_g, naming::id_type v_g, int label)
        {
            hpx::components::stubs::vertex::add_edge(u_g, v_g, label);

        	++size_;

        	return 0;
        }

        naming::id_type vertex_name(int id)
        {
        	return vertices_.at_index(id);
        }

        result_type vertices(void)
        {
            return vertices_.list();
        }

        ///////////////////////////////////////////////////////////////////////
        // Each of the exposed functions needs to be encapsulated into an action
        // type, allowing to generate all required boilerplate code for threads,
        // serialization, etc.
        typedef hpx::actions::result_action1<
            graph, int, graph_init, count_t, &graph::init
        > init_action;

        typedef hpx::actions::result_action0<
            graph, int, graph_order, &graph::order
        > order_action;

        typedef hpx::actions::result_action0<
            graph, int, graph_size, &graph::size
        > size_action;

        typedef hpx::actions::result_action3<
			graph, int, graph_add_edge, naming::id_type, naming::id_type, int, &graph::add_edge
		> add_edge_action;

        typedef hpx::actions::result_action1<
			graph, naming::id_type, graph_vertex_name, int, &graph::vertex_name
		> vertex_name_action;

        typedef hpx::actions::result_action0<
            graph, result_type, graph_vertices, &graph::vertices
        > vertices_action;

    private:
        typedef hpx::components::stubs::distributed_set<
            hpx::components::vertex
        > vertex_set_stub;

        count_t block_size_;
        std::vector<naming::id_type> blocks_;

        int size_;

        vertex_list vertices_;

        naming::id_type vertex_set_;
    };

}}}

#endif
