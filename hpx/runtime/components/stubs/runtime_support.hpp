//  Copyright (c) 2007-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_COMPONENTS_STUBS_RUNTIME_SUPPORT_JUN_09_2008_0503PM)
#define HPX_COMPONENTS_STUBS_RUNTIME_SUPPORT_JUN_09_2008_0503PM

#include <boost/bind.hpp>

#include <hpx/runtime/applier/apply.hpp>
#include <hpx/runtime/components/server/runtime_support.hpp>
#include <hpx/lcos/eager_future.hpp>
#include <hpx/util/ini.hpp>

namespace hpx { namespace components { namespace stubs
{
    ///////////////////////////////////////////////////////////////////
    // The \a runtime_support class is the client side representation of a 
    // \a server#runtime_support component
    struct runtime_support
    {
        ///////////////////////////////////////////////////////////////////////
        // exposed functionality of this component

        /// \brief  The function \a get_factory_properties is used to 
        ///         determine, whether instances of the derived component can 
        ///         be created in blocks (i.e. more than one instance at once). 
        ///         This function is used by the \a distributing_factory to 
        ///         determine a correct allocation strategy
        static lcos::future_value<int> get_factory_properties_async(
            naming::id_type const& targetgid, components::component_type type) 
        {
            // Create an eager_future, execute the required action,
            // we simply return the initialized future_value, the caller needs
            // to call get() on the return value to obtain the result
            typedef 
                server::runtime_support::factory_properties_action 
            action_type;
            return lcos::eager_future<action_type>(targetgid, type);
        }

        static int get_factory_properties(naming::id_type const& targetgid, 
            components::component_type type) 
        {
            // The following get yields control while the action above 
            // is executed and the result is returned to the eager_future
            return get_factory_properties_async(targetgid, type).get();
        }

        ///////////////////////////////////////////////////////////////////////
        /// Create a new component \a type using the runtime_support with the 
        /// given \a targetgid. This is a non-blocking call. The caller needs 
        /// to call \a future_value#get on the result of this function 
        /// to obtain the global id of the newly created object.
        static lcos::future_value<naming::id_type, naming::gid_type> 
        create_component_async(
            naming::gid_type const& gid, components::component_type type, 
            std::size_t count = 1) 
        {
            // Create an eager_future, execute the required action,
            // we simply return the initialized future_value, the caller needs
            // to call get() on the return value to obtain the result
            typedef server::runtime_support::create_component_action action_type;
            return lcos::eager_future<action_type, naming::id_type>(gid, type, count);
        }

        static lcos::future_value<naming::id_type, naming::gid_type> 
        create_component_async(
            naming::id_type const& gid, components::component_type type, 
            std::size_t count = 1) 
        {
            // Create an eager_future, execute the required action,
            // we simply return the initialized future_value, the caller needs
            // to call get() on the return value to obtain the result
            typedef server::runtime_support::create_component_action action_type;
            return lcos::eager_future<action_type, naming::id_type>(
                gid.get_gid(), type, count);
        }

        /// Create a new component \a type using the runtime_support with the 
        /// given \a targetgid. Block for the creation to finish.
        static naming::id_type create_component(
            naming::gid_type const& gid, components::component_type type, 
            std::size_t count = 1) 
        {
            // The following get yields control while the action above 
            // is executed and the result is returned to the eager_future
            return create_component_async(gid, type, count).get();
        }

        static naming::id_type create_component(
            naming::id_type const& gid, components::component_type type, 
            std::size_t count = 1) 
        {
            // The following get yields control while the action above 
            // is executed and the result is returned to the eager_future
            return create_component_async(gid.get_gid(), type, count).get();
        }

        ///////////////////////////////////////////////////////////////////////
        /// Create a new component \a type using the runtime_support with the 
        /// given \a targetgid. This is a non-blocking call. The caller needs 
        /// to call \a future_value#get on the result of this function 
        /// to obtain the global id of the newly created object. Pass one
        /// generic argument to the constructor.
        template <typename Arg0>
        static lcos::future_value<naming::id_type, naming::gid_type> 
        create_one_component_async(
            naming::gid_type const& gid, components::component_type type, 
            Arg0 const& arg0) 
        {
            // Create an eager_future, execute the required action,
            // we simply return the initialized future_value, the caller needs
            // to call get() on the return value to obtain the result
            typedef server::runtime_support::create_one_component_action 
                action_type;
            return lcos::eager_future<action_type, naming::id_type>(
                gid, type, components::constructor_argument(arg0));
        }

        template <typename Arg0>
        static lcos::future_value<naming::id_type, naming::gid_type> 
        create_one_component_async(
            naming::id_type const& gid, components::component_type type, 
            Arg0 const& arg0) 
        {
            // Create an eager_future, execute the required action,
            // we simply return the initialized future_value, the caller needs
            // to call get() on the return value to obtain the result
            typedef server::runtime_support::create_one_component_action 
                action_type;
            return lcos::eager_future<action_type, naming::id_type>(
                gid.get_gid(), type, components::constructor_argument(arg0));
        }

        /// Create a new component \a type using the runtime_support with the 
        /// given \a targetgid. Block for the creation to finish. Pass one
        /// generic argument to the constructor.
        template <typename Arg0>
        static naming::id_type create_one_component(
            naming::id_type const& targetgid, components::component_type type, 
            Arg0 const& arg0) 
        {
            // The following get yields control while the action above 
            // is executed and the result is returned to the eager_future
            return create_one_component_async(targetgid, type, arg0).get();
        }

        template <typename Arg0>
        static naming::id_type create_one_component(
            naming::gid_type const& gid, components::component_type type, 
            Arg0 const& arg0) 
        {
            // The following get yields control while the action above 
            // is executed and the result is returned to the eager_future
            return create_one_component_async(gid, type, arg0).get();
        }

        ///////////////////////////////////////////////////////////////////////
        /// Create a new memory block using the runtime_support with the 
        /// given \a targetgid. This is a non-blocking call. The caller needs 
        /// to call \a future_value#get on the result of this function 
        /// to obtain the global id of the newly created object.
        template <typename T>
        static lcos::future_value<naming::id_type, naming::gid_type> 
        create_memory_block_async(
            naming::gid_type const& gid, std::size_t count,
            hpx::actions::manage_object_action<T> const& act) 
        {
            // Create an eager_future, execute the required action,
            // we simply return the initialized future_value, the caller needs
            // to call get() on the return value to obtain the result
            typedef server::runtime_support::create_memory_block_action action_type;
            return lcos::eager_future<action_type, naming::id_type>(gid, count, act);
        }

        template <typename T>
        static lcos::future_value<naming::id_type, naming::gid_type> 
        create_memory_block_async(
            naming::id_type const& gid, std::size_t count,
            hpx::actions::manage_object_action<T> const& act) 
        {
            // Create an eager_future, execute the required action,
            // we simply return the initialized future_value, the caller needs
            // to call get() on the return value to obtain the result
            typedef server::runtime_support::create_memory_block_action action_type;
            return lcos::eager_future<action_type, naming::id_type>(
                gid.get_gid(), count, act);
        }

        /// Create a new memory block using the runtime_support with the 
        /// given \a targetgid. Block for the creation to finish.
        template <typename T>
        static naming::gid_type create_memory_block(
            naming::id_type const& gid, std::size_t count,
            hpx::actions::manage_object_action<T> const& act) 
        {
            // The following get yields control while the action above 
            // is executed and the result is returned to the eager_future
            return create_memory_block_async(gid, count, act).get();
        }

        template <typename T>
        static naming::id_type create_memory_block(
            naming::id_type const& gid, std::size_t count,
            hpx::actions::manage_object_action<T> const& act) 
        {
            // The following get yields control while the action above 
            // is executed and the result is returned to the eager_future
            return create_memory_block_async(gid.get_gid(), count, act).get();
        }

        /// Destroy an existing component
//         static void free_component(components::component_type type, 
//             naming::gid_type const& gid) 
//         {
//             typedef server::runtime_support::free_component_action action_type;
// 
//             // Determine whether the gid of the component to delete is local or 
//             // remote
//             naming::address addr;
//             applier::applier& appl = hpx::applier::get_applier();
//             naming::resolver_client const& agas = appl.get_agas_client();
//             if (agas.is_smp_mode() || appl.address_is_local(gid, addr)) {
//                 // apply locally
//                 applier::detail::apply_helper2<
//                     action_type, components::component_type, naming::gid_type
//                 >::call(appl.get_runtime_support_raw_gid().get_lsb(), type, gid);
//             }
//             else {
//                 // apply remotely
//                 // zero address will be interpreted as a reference to the 
//                 // remote runtime support object
//                 addr.address_ = 0;
//                 addr.type_ = components::component_runtime_support;
//                 hpx::applier::apply_r<action_type>(addr, naming::invalid_id, type, gid);
//             }
//         }

        static void free_component_sync(components::component_type type, 
            naming::gid_type const& gid) 
        {
            typedef server::runtime_support::free_component_action action_type;

            // Determine whether the gid of the component to delete is local or
            // remote
            naming::address addr;
            applier::applier& appl = hpx::applier::get_applier();
            naming::resolver_client const& agas = appl.get_agas_client();
            if (agas.is_smp_mode() || appl.address_is_local(gid, addr)) {
                // apply locally
                applier::detail::apply_helper2<
                    action_type, components::component_type, naming::gid_type
                >::call(appl.get_runtime_support_raw_gid().get_lsb(), type, gid);
            }
            else {
                // apply remotely
                naming::gid_type prefix;
                appl.get_agas_client().get_prefix(addr.locality_, prefix, false);
                lcos::eager_future<action_type, void>(prefix, type, gid).get();
            }
        }

        /// \brief Shutdown the given runtime system
        static lcos::future_value<int>  
        shutdown_async(naming::id_type const& targetgid)
        {
            // Create an eager_future, execute the required action,
            // we simply return the initialized future_value, the caller needs
            // to call get() on the return value to obtain the result
            typedef server::runtime_support::shutdown_action action_type;
            return lcos::eager_future<action_type>(targetgid);
        }

        static int shutdown(naming::id_type const& targetgid)
        {
            // The following get yields control while the action above 
            // is executed and the result is returned to the eager_future
            return shutdown_async(targetgid).get();
        }

        /// \brief Shutdown the runtime systems of all localities
        static void 
        shutdown_all(naming::id_type const& targetgid)
        {
            hpx::applier::apply<server::runtime_support::shutdown_all_action>(targetgid);
        }

        static void shutdown_all()
        {
            hpx::applier::apply<server::runtime_support::shutdown_all_action>(
                hpx::naming::id_type(
                    hpx::applier::get_applier().get_runtime_support_raw_gid(),
                    hpx::naming::id_type::unmanaged));
        }

        /// \brief Retrieve configuration information
        static lcos::future_value<util::section> get_config_async(
            naming::id_type const& targetgid) 
        {
            // Create an eager_future, execute the required action,
            // we simply return the initialized future_value, the caller needs
            // to call get() on the return value to obtain the result
            typedef server::runtime_support::get_config_action action_type;
            return lcos::eager_future<action_type>(targetgid);
        }

        static void get_config(naming::id_type const& targetgid, util::section& ini)
        {
            // The following get yields control while the action above 
            // is executed and the result is returned to the eager_future
            ini = get_config_async(targetgid).get();
        }
    };

}}}

#endif
