//  Copyright (c) 2007-2008 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_COMPONENTS_STUBS_DISTRIBUTING_FACTORY_OCT_19_2008_0452PM)
#define HPX_COMPONENTS_STUBS_DISTRIBUTING_FACTORY_OCT_19_2008_0452PM

#include <hpx/hpx_fwd.hpp>
#include <hpx/lcos/eager_future.hpp>
#include <hpx/components/distributing_factory/server/distributing_factory.hpp>

namespace hpx { namespace components { namespace stubs
{
    ///////////////////////////////////////////////////////////////////
    // The \a runtime_support class is the client side representation of a 
    // \a server#runtime_support component
    class distributing_factory
    {
    public:
        /// Create a client side representation for any existing 
        /// \a server#runtime_support instance
        distributing_factory(applier::applier& app) 
          : app_(app)
        {}

        ~distributing_factory() 
        {}

        ///////////////////////////////////////////////////////////////////////
        // exposed functionality of this component
        typedef server::distributing_factory::result_type result_type;

        /// Create a number of new components of the given \a type distributed
        /// evenly over all available localities. This is a non-blocking call. 
        /// The caller needs to call \a future_value#get_result on the result 
        /// of this function to obtain the global ids of the newly created 
        /// objects.
        static lcos::future_value<result_type> create_components_async(
            applier::applier& appl, naming::id_type const& targetgid, 
            components::component_type type, std::size_t count) 
        {
            // Create an eager_future, execute the required action,
            // we simply return the initialized future_value, the caller needs
            // to call get_result() on the return value to obtain the result
            typedef server::distributing_factory::create_component_action action_type;
            return lcos::eager_future<action_type, result_type>(appl, 
                targetgid, type, count);
        }

        /// Create a number of new components of the given \a type distributed
        /// evenly over all available localities. Block for the creation to finish.
        static result_type create_components(
            threads::thread_self& self, applier::applier& appl, 
            naming::id_type const& targetgid, components::component_type type,
            std::size_t count = 1) 
        {
            // The following get_result yields control while the action above 
            // is executed and the result is returned to the eager_future
            return create_components_async(appl, targetgid, type, count)
                .get_result(self);
        }

        ///
        lcos::future_value<result_type> create_components_async(
            naming::id_type const& targetgid, components::component_type type,
            std::size_t count = 1) 
        {
            return create_components_async(app_, targetgid, type, count);
        }

        /// 
        result_type create_components(threads::thread_self& self,
            naming::id_type const& targetgid, components::component_type type,
            std::size_t count = 1) 
        {
            return create_components(self, app_, targetgid, type, count);
        }

    protected:
        applier::applier& app_;
    };

}}}

#endif
