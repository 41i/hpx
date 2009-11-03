//  Copyright (c) 2009-2010 Dylan Stark
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_COMPONENTS_STUBS_SSCA2_AUG_13_2009_1040AM)
#define HPX_COMPONENTS_STUBS_SSCA2_AUG_13_2009_1040AM

#include <hpx/runtime/naming/name.hpp>
#include <hpx/runtime/applier/applier.hpp>
#include <hpx/runtime/components/stubs/runtime_support.hpp>
#include <hpx/runtime/components/component_type.hpp>
#include <hpx/runtime/components/stubs/stub_base.hpp>
#include <hpx/lcos/eager_future.hpp>

#include "../server/ssca2.hpp"

namespace hpx { namespace components { namespace stubs
{    
    ///////////////////////////////////////////////////////////////////////////
    /// The \a stubs#ssca2 class is the client side representation of all
    /// \a server#ssca2 components
    struct ssca2
      : components::stubs::stub_base<server::ssca2>
    {
        ///////////////////////////////////////////////////////////////////////
        // exposed functionality of this component
        typedef hpx::components::server::distributing_factory::locality_result
            locality_result;

        static int
        read_graph(naming::id_type gid,
                   naming::id_type G,
                   std::string filename)
        {
            typedef server::ssca2::read_graph_action action_type;
            return lcos::eager_future<action_type>(gid, G, filename).get();
        }

        static int
        init_props_map(naming::id_type gid,
                       naming::id_type P,
                       naming::id_type G)
        {
            typedef server::ssca2::init_props_map_action action_type;
            return lcos::eager_future<action_type>(gid, P, G).get();
        }

        static int
        init_props_map_local(naming::id_type gid,
                             naming::id_type local_props,
                             locality_result local_vertices)
        {
            typedef server::ssca2::init_props_map_local_action action_type;
            return lcos::eager_future<action_type>(gid, local_props, local_vertices).get();
        }
    };

}}}

#endif
