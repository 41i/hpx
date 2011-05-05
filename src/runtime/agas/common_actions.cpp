////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Bryce Lelbach
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////

#include <vector>

#include <boost/fusion/include/vector.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/export.hpp>

#include <hpx/hpx.hpp>
#include <hpx/runtime/naming/name.hpp>
#include <hpx/runtime/components/component_factory.hpp>
#include <hpx/runtime/actions/continuation_impl.hpp>
#include <hpx/util/portable_binary_iarchive.hpp>
#include <hpx/util/portable_binary_oarchive.hpp>
#include <hpx/lcos/base_lco.hpp>

using hpx::components::component_base_lco_with_value;
using hpx::lcos::base_lco_with_value;
using hpx::naming::gid_type;
using boost::fusion::vector2;
using boost::fusion::vector4;

HPX_REGISTER_ACTION_EX(
    base_lco_with_value<bool>::set_result_action,
    set_result_action_bool);
HPX_DEFINE_GET_COMPONENT_TYPE_STATIC(
    base_lco_with_value<bool>,
    component_base_lco_with_value);

// component_ and primary_namespace's prefixes_type
HPX_REGISTER_ACTION_EX(
    base_lco_with_value<std::vector<boost::uint32_t> >::set_result_action,
    set_result_action_agas_prefixes_type);
HPX_DEFINE_GET_COMPONENT_TYPE_STATIC(
    base_lco_with_value<std::vector<boost::uint32_t> >,
    component_base_lco_with_value);

typedef vector4<gid_type, gid_type, gid_type, bool> agas_binding_type; 

// primary_namespaces's binding_type
HPX_REGISTER_ACTION_EX(
    base_lco_with_value<agas_binding_type>::set_result_action,
    set_result_action_agas_binding_type);
HPX_DEFINE_GET_COMPONENT_TYPE_STATIC(
    base_lco_with_value<agas_binding_type>,
    component_base_lco_with_value);

typedef vector2<boost::uint64_t, boost::uint32_t> agas_decrement_type;
 
// primary_namespaces's decrement_type
HPX_REGISTER_ACTION_EX(
    base_lco_with_value<agas_decrement_type>::set_result_action,
    set_result_action_agas_decrement_type);
HPX_DEFINE_GET_COMPONENT_TYPE_STATIC(
    base_lco_with_value<agas_decrement_type>,
    component_base_lco_with_value);

