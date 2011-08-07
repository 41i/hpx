//  Copyright (c) 2007-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_INTERPOLATE1D_AUG_04_2011_0340PM)
#define HPX_INTERPOLATE1D_AUG_04_2011_0340PM

#include <hpx/hpx_fwd.hpp>
#include <hpx/lcos/future_value.hpp>
#include <hpx/components/distributing_factory/distributing_factory.hpp>

#include <vector>

#include "stubs/partition3d.hpp"

///////////////////////////////////////////////////////////////////////////////
namespace interpolate3d 
{
    // This class encapsulates N partitions and dispatches requests based on 
    // the given values to get interpolated results for.
    class HPX_COMPONENT_EXPORT interpolate3d 
    {
    public:
        // Create a new interpolation instance and initialize it synchronously.
        // Passing -1 as the second argument creates exactly one partition 
        // instance on each available locality.
        interpolate3d(std::string const& datafilename, 
            std::size_t num_instances = std::size_t(-1));

        // Return the interpolated  function value for the given argument. This
        // function dispatches to the proper partition for the actual 
        // interpolation.
        hpx::lcos::future_value<double>
        interpolate_async(double value_x, double value_y, double value_z)
        {
            return stubs::partition3d::interpolate_async(
                get_gid(value_x, value_y, value_z), value_x, value_y, value_z);
        }

        double interpolate(double value_x, double value_y, double value_z)
        {
            return stubs::partition3d::interpolate(
                get_gid(value_x, value_y, value_z), value_x, value_y, value_z);
        }

    private:
        // map the given value to the gid of the partition responsible for the
        // interpolation
        hpx::naming::id_type get_gid(double value_x, double value_y, double value_z);

        // initialize the partitions and store the mappings
        typedef hpx::components::distributing_factory distributing_factory;
        typedef distributing_factory::async_create_result_type 
            async_create_result_type;

        void fill_partitions(std::string const& datafilename,
            async_create_result_type future);
        std::size_t get_index(int d, double value);

    private:
        std::vector<hpx::naming::id_type> partitions_;
        double minval_[dimension::dim];
        double delta_[dimension::dim];
        std::size_t num_values_[dimension::dim];
        std::size_t num_partitions_per_dim_;
    };
}

#endif


