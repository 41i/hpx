//  Copyright (c) 2007-2009 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx.hpp>
#include <hpx/lcos/future_wait.hpp>

#include "stencil.hpp"
#include "logging.hpp"
#include "stencil_data.hpp"
#include "stencil_functions.hpp"

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace components { namespace amr 
{
    stencil::stencil()
      : numsteps_(0)
    {}

    ///////////////////////////////////////////////////////////////////////////
    // Implement actual functionality of this stencil
    // Compute the result value for the current time step
    bool stencil::eval(naming::id_type const& result, 
        std::vector<naming::id_type> const& gids, int row, int column)
    {
        BOOST_ASSERT(gids.size() == 3);

        // start asynchronous get operations

        // get all input memory_block_data instances
        access_memory_block<stencil_data> val1, val2, val3, resultval;
        boost::tie(val1, val2, val3, resultval) = 
            wait(components::stubs::memory_block::get_async(gids[0]), 
                 components::stubs::memory_block::get_async(gids[1]), 
                 components::stubs::memory_block::get_async(gids[2]),
                 components::stubs::memory_block::get_async(result));

        // make sure all input data items agree on the time step number
        BOOST_ASSERT(val1->timestep_ == val2->timestep_);
        BOOST_ASSERT(val1->timestep_ == val3->timestep_);

        // the middle point is our direct predecessor
        if (val2->timestep_ < numsteps_) {

            // this is the actual calculation, call provided (external) function
            evaluate_timestep(val1.get_ptr(), val2.get_ptr(), val3.get_ptr(), 
                resultval.get_ptr(), numsteps_);

            if (log_)     // send result to logging instance
                stubs::logging::logentry(log_, resultval.get(), row);
        }
        else {
            // the last time step has been reached, just copy over the data
            resultval.get() = val2.get();
        }

        // set return value to true if this is the last time step
        return resultval->timestep_ >= numsteps_;
    }

    naming::id_type stencil::alloc_data(int item, int maxitems, int row)
    {
        naming::id_type result = components::stubs::memory_block::create(
            applier::get_applier().get_runtime_support_gid(), sizeof(stencil_data));

        if (-1 != item) {
            // provide initial data for the given data value 
            access_memory_block<stencil_data> val(
                components::stubs::memory_block::checkout(result));

            // call provided (external) function
            generate_initial_data(val.get_ptr(), item, maxitems, row);

            if (log_)         // send initial value to logging instance
                stubs::logging::logentry(log_, val.get(), row);
        }
        return result;
    }

    void stencil::init(std::size_t numsteps, naming::id_type const& logging)
    {
        numsteps_ = numsteps;
        log_ = logging;
    }

}}}

