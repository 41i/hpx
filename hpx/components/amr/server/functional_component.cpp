//  Copyright (c) 2007-2008 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx.hpp>
#include <hpx/runtime/components/component_factory.hpp>
#include <hpx/util/portable_binary_iarchive.hpp>
#include <hpx/util/portable_binary_oarchive.hpp>

#include <boost/serialization/version.hpp>
#include <boost/serialization/export.hpp>

#include <hpx/components/amr/server/functional_component.hpp>

///////////////////////////////////////////////////////////////////////////////
typedef 
    hpx::components::amr::server::functional_component 
functional_component_double_type;

///////////////////////////////////////////////////////////////////////////////
// Serialization support for the actions
HPX_REGISTER_ACTION(functional_component_double_type::init_action);
HPX_REGISTER_ACTION(functional_component_double_type::eval_action);
HPX_REGISTER_ACTION(functional_component_double_type::free_action);
HPX_DEFINE_GET_COMPONENT_TYPE(functional_component_double_type);

