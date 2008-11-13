//  Copyright (c) 2007-2008 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This examples illustrates the usage of the generic_component template and 
// related facilities.

#if !defined(HPX_COMPONENTS_GENERIC_RESULT_OCT_13_2008_1017AM)
#define HPX_COMPONENTS_GENERIC_RESULT_OCT_13_2008_1017AM

#include <hpx/hpx.hpp>
#include <hpx/runtime/components/server/generic_component.hpp>

// This is the function to wrap into the component. Its purpose is to generate
// a floating point number, returning it to the caller
HPX_COMPONENT_EXPORT double 
generate_number (hpx::threads::thread_self&, hpx::applier::applier&);

// We use generic_component0 here because the function 
// generate_number() takes no additional argument. The number of additional
// arguments N needs to be reflected in the name of the generic_componentN.
typedef 
    hpx::components::server::generic_component0<double, generate_number> 
generate_number_wrapper;

#endif

