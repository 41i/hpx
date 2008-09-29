//  Copyright (c) 2007-2008 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_COMPONENTS_STUBS_SIMPLE_ACCUMULATOR_JUL_18_2008_1125AM)
#define HPX_COMPONENTS_STUBS_SIMPLE_ACCUMULATOR_JUL_18_2008_1125AM

#include <hpx/runtime/naming/name.hpp>
#include <hpx/runtime/applier/applier.hpp>
#include <hpx/runtime/components/stubs/runtime_support.hpp>
#include <hpx/runtime/components/server/simple_accumulator.hpp>
#include <hpx/lcos/eager_future.hpp>

namespace hpx { namespace components { namespace stubs
{
    ///////////////////////////////////////////////////////////////////////////
    /// The \a stubs#simple_accumulator class is the client side representation of all
    /// \a server#simple_accumulator components
    class simple_accumulator
    {
    public:
        /// Create a client side representation for any existing
        /// \a server#simple_accumulator instance.
        simple_accumulator(applier::applier& appl) 
          : app_(appl)
        {}

        ~simple_accumulator() 
        {}

        /// Asynchronously create a new instance of an simple_accumulator
        static lcos::simple_future<naming::id_type>
        create_async(applier::applier& appl, naming::id_type const& targetgid)
        {
            return stubs::runtime_support::create_component_async(
                appl, targetgid, 
                (components::component_type)server::simple_accumulator::value);
        }

        /// Create a new instance of an simple_accumulator
        static naming::id_type 
        create(threads::thread_self& self, applier::applier& appl, 
            naming::id_type const& targetgid)
        {
            return stubs::runtime_support::create_component(
                self, appl, targetgid, 
                (components::component_type)server::simple_accumulator::value);
        }

        /// Delete an existing component
        static void
        free(applier::applier& appl, naming::id_type const& targetgid, 
            naming::id_type const& gid)
        {
            stubs::runtime_support::free_component(appl, targetgid, 
                (components::component_type)server::simple_accumulator::value, gid);
        }

        /// Query the current value of the server#simple_accumulator instance 
        /// with the given \a gid. This is a non-blocking call. The caller 
        /// needs to call \a simple_future#get_result on the return value of 
        /// this function to obtain the result as returned by the simple_accumulator.
        static lcos::simple_future<double> query_async(
            applier::applier& appl, naming::id_type gid) 
        {
            // Create an eager_future, execute the required action,
            // we simply return the initialized eager_future, the caller needs
            // to call get_result() on the return value to obtain the result
            typedef server::simple_accumulator::query_action action_type;
            return lcos::eager_future<action_type, double>(appl, gid);
        }

        /// Query the current value of the server#simple_accumulator instance 
        /// with the given \a gid. Block for the current simple_accumulator value to 
        /// be returned.
        static double query(threads::thread_self& self, 
            applier::applier& appl, naming::id_type gid) 
        {
            // The following get_result yields control while the action above 
            // is executed and the result is returned to the simple_future
            return query_async(appl, gid).get_result(self);
        }

        ///////////////////////////////////////////////////////////////////////
        // exposed functionality of this component

        /// Initialize the simple_accumulator value of the server#simple_accumulator instance 
        /// with the given \a gid
        void init(naming::id_type gid) 
        {
            app_.apply<server::simple_accumulator::init_action>(gid);
        }

        /// Add the given number to the server#simple_accumulator instance 
        /// with the given \a gid
        void add (naming::id_type gid, double arg) 
        {
            app_.apply<server::simple_accumulator::add_action>(gid, arg);
        }

        /// Print the current value of the server#simple_accumulator instance 
        /// with the given \a gid
        void print(naming::id_type gid) 
        {
            app_.apply<server::simple_accumulator::print_action>(gid);
        }

        /// Query the current value of the server#simple_accumulator instance 
        /// with the given \a gid. This is a non-blocking call. The 
        /// caller needs to call \a simple_future#get_result on the return 
        /// value of this function to obtain the result as returned by the 
        /// simple_accumulator.
        lcos::simple_future<double> query_async(naming::id_type gid) 
        {
            return query_async(app_, gid);
        }

        /// Query the current value of the server#simple_accumulator instance 
        /// with the given \a gid. Block for the current simple_accumulator 
        /// value to be returned.
        double query(threads::thread_self& self,naming::id_type gid) 
        {
            return query(self, app_, gid);
        }

    protected:
        applier::applier& app_;
    };
    
}}}

#endif
