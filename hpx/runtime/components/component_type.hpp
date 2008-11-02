//  Copyright (c) 2007-2008 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_COMPONENT_COMPONENT_TYPE_MAR_26_2008_1058AM)
#define HPX_COMPONENT_COMPONENT_TYPE_MAR_26_2008_1058AM

#include <boost/assert.hpp>
#include <boost/lexical_cast.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace components
{
    enum component_type
    {
        component_invalid = -1,
        component_runtime_support = 0,  // runtime support (needed to create components, etc.)
        component_memory,               // general memory address
        component_memory_block,         // general memory block
        component_thread,               // a ParalleX thread

        // LCO's
        component_base_lco,         ///< the base of all LCO's not waiting on a value
        component_base_lco_with_value,  ///< base LCO's blocking on a value
        component_future,           ///< a future executing the action and 
                                    ///< allowing to wait for the result
        component_value_adaptor,    ///< a adaptor to access specific slot of an LCO

        component_last,
        component_first_dynamic = component_last
    };

    /// \brief Return the string representation for a given component type id
    HPX_EXPORT std::string const get_component_type_name(int type);

    ///////////////////////////////////////////////////////////////////////////
    /// This needs to be specialized for each of the components
    template <typename Component>
    HPX_ALWAYS_EXPORT component_type get_component_type();

}}

///////////////////////////////////////////////////////////////////////////////
#define HPX_DEFINE_GET_COMPONENT_TYPE(component)                              \
    namespace hpx { namespace components {                                    \
        template<> HPX_ALWAYS_EXPORT component_type                           \
        get_component_type<component>()                                       \
            { return component::get_component_type(); }                       \
    }}                                                                        \
    /**/

#endif

