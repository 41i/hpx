//  Copyright (c) 2007-2009 Dylan Stark
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_COMPONENTS_GRAPH_MAY_18_2008_0822AM)
#define HPX_COMPONENTS_GRAPH_MAY_18_2008_0822AM

#include <hpx/runtime.hpp>
#include <hpx/runtime/components/client_base.hpp>

#include <hpx/components/graph/stubs/graph.hpp>

namespace hpx { namespace components 
{
    ///////////////////////////////////////////////////////////////////////////
    /// The \a graph class is the client side representation of a 
    /// specific \a server#graph component
    class graph 
      : public client_base<graph, stubs::graph>
    {
        typedef client_base<graph, stubs::graph> base_type;

    public:
        /// Create a client side representation for the existing
        /// \a server#graph instance with the given global id \a gid.
        graph(naming::id_type gid, bool freeonexit = true) 
          : base_type(gid, freeonexit)
        {}

        ///////////////////////////////////////////////////////////////////////
        // exposed functionality of this component

        /// Initialize the graph
        void init() 
        {
            BOOST_ASSERT(gid_);
            this->base_type::init(gid_);
        }
    };
    
}}

#endif
