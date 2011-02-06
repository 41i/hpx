//  Copyright (c) 2007-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_COMPONENT_COMPONENT_TYPE_MAR_26_2008_1058AM)
#define HPX_COMPONENT_COMPONENT_TYPE_MAR_26_2008_1058AM

#include <boost/assert.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/cstdint.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace components
{
    enum component_type
    {
        component_invalid = -1,
        component_runtime_support = 0,  // runtime support (needed to create components, etc.)
        component_memory = 1,           // general memory address
        component_memory_block = 2,     // general memory block

        // LCO's
        component_base_lco = 3,         ///< the base of all LCO's not waiting on a value
        component_last,

        component_base_lco_with_value = 
            ((4 << 16) | component_base_lco),
                                        ///< base LCO's blocking on a value
        component_future =              ///< a future executing the action and 
                                        ///< allowing to wait for the result
            ((5 << 16) | component_base_lco),
        component_value_adaptor = 5,    ///< an adaptor to access specific slot of an LCO
        component_barrier =             ///< a LCO implementing a barrier
            ((6 << 16) | component_base_lco),
        component_thread =              ///< a ParalleX thread
            ((7 << 16) | component_base_lco),
        component_dataflow_variable =   ///< a LCO implementing dataflow var.
            ((8 << 16) | component_base_lco),
        component_thunk =               ///< a LCO implementing a thunk
            ((9 << 16) | component_base_lco_with_value),

        component_dataflow_block = 10,  ///< a dataflow block
        component_first_dynamic = component_last
    };

    /// \brief Return the string representation for a given component type id
    HPX_EXPORT std::string const get_component_type_name(int type);

    /// The lower short word of the component type is the type of the component 
    /// exposing the actions.
    inline component_type get_base_type(boost::int64_t t)
    {
        return component_type(t & 0xFFFF);
    }

    /// The upper short word of the component is the actual component type
    inline component_type get_derived_type(boost::int64_t t)
    {
        return component_type((t >> 16) & 0xFFFF);
    }

    /// A component derived from a base component exposing the actions needs to
    /// have a specially formatted component type. 
    inline component_type 
    derived_component_type(boost::int64_t derived, boost::int64_t base)
    {
        return component_type(derived << 16 | base);
    }

    /// \brief Verify the two given component types are matching (compatible)
    inline bool types_are_compatible(boost::int64_t lhs, boost::int64_t rhs)
    {
        // don't compare types if one of them is unknown
        if (component_invalid == rhs || component_invalid == lhs)
            return true;    // no way of telling, so we assume the best :-P

        // don't compare types if one of them is component_runtime_support
        if (component_runtime_support == rhs || component_runtime_support == lhs)
            return true;

        return get_base_type(lhs) == get_base_type(rhs);
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Component, typename Enable = void>
    struct extract_component_type_from_component
    {
        typedef component_type type;

        static type call()
        {
            return Component::get_component_type();
        }
    }; 

    template <typename Component, typename Enable = void>
    struct assign_component_type_to_component
    {
        typedef void type;

        static void call(component_type type)
        {
            return Component::set_component_type(type);
        }
    }; 
 
    template <typename Component>
    HPX_ALWAYS_EXPORT typename
    extract_component_type_from_component<Component>::type
    get_component_type()
    {
        return extract_component_type_from_component<Component>::call();
    }

    template <typename Component>
    HPX_ALWAYS_EXPORT typename
    assign_component_type_to_component<Component>::type
    set_component_type(component_type type)
    {
        return assign_component_type_to_component<Component>::call(type);
    }

    ///////////////////////////////////////////////////////////////////////////
    enum factory_property
    {
        factory_invalid = -1,
        factory_none = 0,                   ///< The factory has no special properties
        factory_is_multi_instance = 1,      ///< The factory can be used to 
                                            ///< create more than one component 
                                            ///< at the same time
        factory_instance_count_is_size = 2, ///< The component count will be 
                                            ///< interpreted as the component
                                            ///< size instead
    };
}}

///////////////////////////////////////////////////////////////////////////////
// This macro is deprecated now.
#define HPX_DEFINE_GET_COMPONENT_TYPE(component)

#endif

