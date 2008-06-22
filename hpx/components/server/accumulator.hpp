//  Copyright (c) 2007-2008 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_COMPONENTS_SERVER_ACCUMULATOR_MAY_17_2008_0731PM)
#define HPX_COMPONENTS_SERVER_ACCUMULATOR_MAY_17_2008_0731PM

#include <iostream>

#include <hpx/hpx_fwd.hpp>
#include <hpx/runtime/applier/applier.hpp>
#include <hpx/runtime/threadmanager/px_thread.hpp>
#include <hpx/components/component_type.hpp>
#include <hpx/components/action.hpp>
#include <hpx/components/server/wrapper.hpp>

namespace hpx { namespace components { namespace server { namespace detail
{
    ///////////////////////////////////////////////////////////////////////////
    /// The accumulator is a very simple example of a HPX component. 
    ///
    /// Note that the implementation of the accumulator does not require any 
    /// special data members or virtual functions.
    ///
    class accumulator 
    {
    public:
        // parcel action code: the action to be performed on the destination 
        // object (the accumulator)
        enum actions
        {
            accumulator_init = 0,
            accumulator_add = 1,
            accumulator_query_value = 2,
            accumulator_print = 3
        };

        // This is the component id. Every component needs to have an embedded
        // enumerator 'value' which is used by the generic action implementation
        // to associate this component with a given action.
        enum { value = component_accumulator };

        // constructor: initialize accumulator value
        accumulator()
          : arg_(0)
        {}
        
        ///////////////////////////////////////////////////////////////////////
        // exposed functionality of this component

        /// Initialize the accumulator
        threadmanager::thread_state 
        init (threadmanager::px_thread_self&, applier::applier& appl) 
        {
            arg_ = 0;
            return hpx::threadmanager::terminated;
        }

        /// Add the given number to the accumulator
        threadmanager::thread_state 
        add (threadmanager::px_thread_self&, applier::applier& appl, double arg) 
        {
            arg_ += arg;
            return hpx::threadmanager::terminated;
        }

        /// Return the current value to the caller
        threadmanager::thread_state 
        query (threadmanager::px_thread_self&, applier::applier& appl,
            double* result) 
        {
            // this will be zero if the action got invoked without continuations
            if (result)
                *result = arg_;
            return hpx::threadmanager::terminated;
        }

        /// Print the current value of the accumulator
        threadmanager::thread_state 
        print (threadmanager::px_thread_self&, applier::applier& appl) 
        {
            std::cout << arg_ << std::flush << std::endl;
            return hpx::threadmanager::terminated;
        }

        ///////////////////////////////////////////////////////////////////////
        // Each of the exposed functions needs to be encapsulated into an action
        // type, allowing to generate all required boilerplate code for threads,
        // serialization, etc.
        typedef action0<
            accumulator, accumulator_init, &accumulator::init
        > init_action;

        typedef action1<
            accumulator, accumulator_add, double, &accumulator::add
        > add_action;

        typedef result_action0<
            accumulator, double, accumulator_query_value, &accumulator::query
        > query_action;

        typedef action0<
            accumulator, accumulator_print, &accumulator::print
        > print_action;

        ///
        static accumulator* get_lva(naming::address::address_type lva)
        {
            typedef 
                wrapper<detail::accumulator, server::accumulator> 
            wrapping_type;
            return reinterpret_cast<wrapping_type*>(lva)->get();
        }

    private:
        double arg_;
    };

}}}}

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace components { namespace server 
{
    class accumulator 
      : public wrapper<detail::accumulator, accumulator>
    {
    private:
        typedef detail::accumulator wrapped_type;
        typedef wrapper<wrapped_type, accumulator> base_type;

    public:
        // This is the component id. Every component needs to have an embedded
        // enumerator 'value' which is used by the generic action implementation
        // to associate this component with a given action.
        enum { value = wrapped_type::value };

        accumulator()
          : base_type(new wrapped_type())
        {}

    protected:
        base_type& base() { return *this; }
        base_type const& base() const { return *this; }
    };

}}}

///////////////////////////////////////////////////////////////////////////////
// Serialization support for the accumulator actions
HPX_SERIALIZE_ACTION(hpx::components::server::detail::accumulator::init_action);
HPX_SERIALIZE_ACTION(hpx::components::server::detail::accumulator::add_action);
HPX_SERIALIZE_ACTION(hpx::components::server::detail::accumulator::query_action);
HPX_SERIALIZE_ACTION(hpx::components::server::detail::accumulator::print_action);

#endif
