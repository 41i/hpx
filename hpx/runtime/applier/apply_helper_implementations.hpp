//  Copyright (c) 2007-2008 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PP_IS_ITERATING

#if !defined(HPX_APPLIER_APPLY_HELPER_IMPLEMENTATIONS_JUN_26_2008_0150PM)
#define HPX_APPLIER_APPLY_HELPER_IMPLEMENTATIONS_JUN_26_2008_0150PM

#include <boost/preprocessor/iterate.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>

#define BOOST_PP_ITERATION_PARAMS_1                                           \
    (3, (2, HPX_COMPONENT_ARGUMENT_LIMIT,                                     \
    "hpx/runtime/applier/apply_helper_implementations.hpp"))                  \
    /**/
    
#include BOOST_PP_ITERATE()

#endif

///////////////////////////////////////////////////////////////////////////////
//  Preprocessor vertical repetition code
///////////////////////////////////////////////////////////////////////////////
#else // defined(BOOST_PP_IS_ITERATING)

#define N BOOST_PP_ITERATION()

    ///////////////////////////////////////////////////////////////////////
    template <
        typename Action,
        BOOST_PP_ENUM_PARAMS(N, typename Arg),
        typename DirectExecute = typename Action::direct_execution
    >
    struct BOOST_PP_CAT(apply_helper, N);

    template <typename Action, BOOST_PP_ENUM_PARAMS(N, typename Arg)>
    struct BOOST_PP_CAT(apply_helper, N)<
        Action, BOOST_PP_ENUM_PARAMS(N, Arg), boost::mpl::false_
    >
    {
        static void 
        call (threadmanager::threadmanager& tm, applier& appl, 
            naming::address::address_type lva,
            BOOST_PP_ENUM_BINARY_PARAMS(N, Arg, const& arg))
        {
            tm.register_work(Action::construct_thread_function(
                appl, lva, BOOST_PP_ENUM_PARAMS(N, arg)));
        }

        static void 
        call (components::continuation_type& c, 
            threadmanager::threadmanager& tm, applier& appl, 
            naming::address::address_type lva,
            BOOST_PP_ENUM_BINARY_PARAMS(N, Arg, const& arg))
        {
            tm.register_work(Action::construct_thread_function(
                c, appl, lva, BOOST_PP_ENUM_PARAMS(N, arg)));
        }
    };

    template <typename Action, BOOST_PP_ENUM_PARAMS(N, typename Arg)>
    struct BOOST_PP_CAT(apply_helper, N)<
        Action, BOOST_PP_ENUM_PARAMS(N, Arg), boost::mpl::true_
    >
    {
        // If local and to be directly executed, just call the function
        static void
        call (threadmanager::threadmanager&, applier& appl, 
            naming::address::address_type addr,
            BOOST_PP_ENUM_BINARY_PARAMS(N, Arg, const& arg))
        {
            Action::execute_function(appl, addr, BOOST_PP_ENUM_PARAMS(N, arg));
        }

        static typename Action::result_type  
        call (components::continuation_type& c, threadmanager::threadmanager&, 
            applier& appl, naming::address::address_type addr,
            BOOST_PP_ENUM_BINARY_PARAMS(N, Arg, const& arg))
        {
            return Action::execute_function(appl, addr, 
                BOOST_PP_ENUM_PARAMS(N, arg));
        }
    };

#undef N

#endif
