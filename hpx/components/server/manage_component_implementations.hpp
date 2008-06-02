//  Copyright (c) 2007-2008 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PP_IS_ITERATING

#if !defined(HPX_COMPONENTS_SERVER_MANAGE_COMPONENT_IMPLEMENTATIONS_JUN_02_2008_0237PM)
#define HPX_COMPONENTS_SERVER_MANAGE_COMPONENT_IMPLEMENTATIONS_JUN_02_2008_0237PM

#include <boost/preprocessor/iterate.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>

#define BOOST_PP_ITERATION_PARAMS_1                                           \
    (3, (3, HPX_COMPONENT_ARGUMENT_LIMIT,                                     \
    "hpx/components/server/manage_component_implementations.hpp"))            \
    /**/
    
#include BOOST_PP_ITERATE()

#endif

///////////////////////////////////////////////////////////////////////////////
//  Preprocessor vertical repetition code
///////////////////////////////////////////////////////////////////////////////
#else // defined(BOOST_PP_IS_ITERATING)

#define N BOOST_PP_ITERATION()

    ///////////////////////////////////////////////////////////////////////////
    template <typename Component, BOOST_PP_ENUM_PARAMS(N, typename Arg)>
    Component* create (naming::resolver_client const& dgas, naming::id_type gid,
        BOOST_PP_ENUM_BINARY_PARAMS(N, Arg, const& arg))
    {
        Component* c = new Component(BOOST_PP_ENUM_PARAMS(N, arg));
        if (!dgas.bind(gid, naming::address(dgas.here(), Component::value, c))) 
        {
            delete c;
            boost::throw_exception(
                hpx::exception(hpx::duplicate_component_address,
                    "global id is already bound to a different "
                    "component instance"));
        }
        return c;
    }
    
#undef N

#endif
