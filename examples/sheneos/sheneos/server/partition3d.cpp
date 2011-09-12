//  Copyright (c) 2007-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx_fwd.hpp>
#include <hpx/runtime/actions/continuation_impl.hpp>
#include <hpx/runtime/components/component_factory.hpp>

#include <boost/serialization/vector.hpp>
#include <hpx/util/portable_binary_iarchive.hpp>
#include <hpx/util/portable_binary_oarchive.hpp>

#include <cmath>
#include <memory>

#include <boost/assert.hpp>

#include "partition3d.hpp"
#include "../read_values.hpp"

///////////////////////////////////////////////////////////////////////////////
namespace sheneos { namespace server
{
    partition3d::partition3d()
      : energy_shift_(0)
    {
        std::memset(min_value_, 0, sizeof(min_value_));
        std::memset(max_value_, 0, sizeof(max_value_));
        std::memset(delta_, 0, sizeof(delta_));
    }

    inline void 
    partition3d::init_dimension(std::string const& datafilename, int d, 
        dimension const& dim, char const* name, boost::scoped_array<double>& values)
    { 
        // store all parameters
        dim_[d] = dim;

        // Account for necessary overlap on the right hand end of the data 
        // interval (the interpolation algorithm used does not go beyond the
        // left hand end).
        std::size_t count = dim.count_;
        if (dim.offset_ + dim.count_ < dim.size_-2) {
            dim_[d].count_ += 2;
            ++count;
        }

        // read the full data range
        values.reset(new double[dim_[d].count_]);
        extract_data(datafilename, name, values.get(), dim.offset_, dim_[d].count_);

        // extract range (without ghost-zones)
        min_value_[d] = values[0];
        max_value_[d] = values[count-1];
        delta_[d] = values[1] - values[0];
    }

    inline void 
    partition3d::init_data(std::string const& datafilename, 
        char const* name, boost::scoped_array<double>& values, 
        std::size_t array_size)
    {
        values.reset(new double[array_size]);
        extract_data(datafilename, name, values.get(), dim_[dimension::ye], 
            dim_[dimension::temp], dim_[dimension::rho]);
    }

    void partition3d::init(std::string const& datafilename, 
        dimension const& dimx, dimension const& dimy, dimension const& dimz)
    {
        init_dimension(datafilename, dimension::ye, dimx, "ye", ye_values_);
        init_dimension(datafilename, dimension::temp, dimy, "logtemp", logtemp_values_);
        init_dimension(datafilename, dimension::rho, dimz, "logrho", logrho_values_);

        // init energy shift
        extract_data(datafilename, "energy_shift", &energy_shift_, 0, 1);

        // read the slice of our data
        std::size_t array_size = dim_[dimension::ye].count_ * 
            dim_[dimension::temp].count_ * dim_[dimension::rho].count_;

        init_data(datafilename, "logpress", logpress_values_, array_size);
        init_data(datafilename, "logenergy", logenergy_values_, array_size);
        init_data(datafilename, "entropy", entropy_values_, array_size);
        init_data(datafilename, "munu", munu_values_, array_size);
        init_data(datafilename, "cs2", cs2_values_, array_size);
        init_data(datafilename, "dedt", dedt_values_, array_size);
        init_data(datafilename, "dpdrhoe", dpdrhoe_values_, array_size);
        init_data(datafilename, "dpderho", dpderho_values_, array_size);
#if SHENEOS_SUPPORT_FULL_API
        init_data(datafilename, "muhat", muhat_values_, array_size);
        init_data(datafilename, "mu_e", mu_e_values_, array_size);
        init_data(datafilename, "mu_p", mu_p_values_, array_size);
        init_data(datafilename, "mu_n", mu_n_values_, array_size);
        init_data(datafilename, "Xa", xa_values_, array_size);
        init_data(datafilename, "Xh", xh_values_, array_size);
        init_data(datafilename, "Xp", xp_values_, array_size);
        init_data(datafilename, "Xn", xn_values_, array_size);
        init_data(datafilename, "Abar", abar_values_, array_size);
        init_data(datafilename, "Zbar", zbar_values_, array_size);
        init_data(datafilename, "gamma", gamma_values_, array_size);
#endif
    }

    // do the actual interpolation
    inline std::size_t 
    partition3d::get_index(int d, double value)
    {
        if (value < min_value_[d] || value > max_value_[d]) {
            HPX_THROW_EXCEPTION(hpx::bad_parameter, "verify_value", 
                "argument out of range");
            return 0;
        }

        std::size_t index = (value - min_value_[d]) / delta_[d];

        // Either the index has to be inside bounds or the requested value 
        // corresponds to the right end edge of the managed data range.
        BOOST_ASSERT(index < dim_[d].count_ || 
            (index == dim_[d].count_ && value == max_value_[d]));

        return index;
    }

    ///////////////////////////////////////////////////////////////////////////
    // get index of a given value in the one-dimensional array-slice
    inline std::size_t 
    index(std::size_t x, std::size_t y, std::size_t z, dimension const* dim)
    {
        std::size_t idx = z + (y + x * dim[dimension::temp].count_) * 
            dim[dimension::rho].count_;

        BOOST_ASSERT(idx < dim[dimension::ye].count_ * 
            dim[dimension::temp].count_ * dim[dimension::rho].count_);

        return idx;
    }

    ///////////////////////////////////////////////////////////////////////////
    // tri-linear interpolation routine
    inline double 
    partition3d::interpolate(double* values, 
        std::size_t idx_x, std::size_t idx_y, std::size_t idx_z,
        double delta_ye, double delta_logtemp, double delta_logrho)
    {
        double value000 = values[index(idx_x,   idx_y,   idx_z,   dim_)];
        double value001 = values[index(idx_x,   idx_y,   idx_z+1, dim_)];
        double value010 = values[index(idx_x,   idx_y+1, idx_z,   dim_)];
        double value011 = values[index(idx_x,   idx_y+1, idx_z+1, dim_)];
        double value100 = values[index(idx_x+1, idx_y,   idx_z,   dim_)];
        double value101 = values[index(idx_x+1, idx_y,   idx_z+1, dim_)];
        double value110 = values[index(idx_x+1, idx_y+1, idx_z,   dim_)];
        double value111 = values[index(idx_x+1, idx_y+1, idx_z+1, dim_)];

        double comp_delta_ye = 1. - delta_ye;
        double comp_delta_logtemp = 1. - delta_logtemp;
        double comp_delta_logrho = 1. - delta_logrho;

        return value000 * comp_delta_ye * comp_delta_logtemp * comp_delta_logrho +
               value001 * comp_delta_ye * comp_delta_logtemp * delta_logrho +
               value010 * comp_delta_ye * delta_logtemp      * comp_delta_logrho +
               value011 * comp_delta_ye * delta_logtemp      * delta_logrho +
               value100 * delta_ye      * comp_delta_logtemp * comp_delta_logrho +
               value101 * delta_ye      * comp_delta_logtemp * delta_logrho +
               value110 * delta_ye      * delta_logtemp      * comp_delta_logrho +
               value111 * delta_ye      * delta_logtemp      * delta_logrho;
    }

    ///////////////////////////////////////////////////////////////////////////
    // ye: electron fraction
    // temp: temperature
    // rho: rest mass density of the plasma
    std::vector<double> 
    partition3d::interpolate(double ye, double temp, 
        double rho, boost::uint32_t eosvalues)
    {
        double logrho = std::log10(rho);
        double logtemp = std::log10(temp);

        std::size_t idx_ye = get_index(dimension::ye, ye);
        std::size_t idx_logtemp = get_index(dimension::temp, logtemp);
        std::size_t idx_logrho = get_index(dimension::rho, logrho);

        double delta_ye = (ye - ye_values_[idx_ye]) / delta_[dimension::ye];
        double delta_logtemp = (logtemp - logtemp_values_[idx_logtemp]) / delta_[dimension::temp];
        double delta_logrho = (logrho - logrho_values_[idx_logrho]) / delta_[dimension::rho];

        std::vector<double> results; 
        results.reserve(19);

        // calculate all required values
        if (eosvalues & logpress) {
            double value = interpolate(logpress_values_.get(), 
                idx_ye, idx_logtemp, idx_logrho, 
                delta_ye, delta_logtemp, delta_logrho);
            results.push_back(std::pow(10., value));
        }
        if (eosvalues & logenergy) {
            double value = interpolate(logenergy_values_.get(), 
                idx_ye, idx_logtemp, idx_logrho, 
                delta_ye, delta_logtemp, delta_logrho);
            results.push_back(std::pow(10., value) - energy_shift_);
        }
        if (eosvalues & entropy) {
            results.push_back(interpolate(entropy_values_.get(), 
                idx_ye, idx_logtemp, idx_logrho, 
                delta_ye, delta_logtemp, delta_logrho));
        }
        if (eosvalues & munu) {
            results.push_back(interpolate(munu_values_.get(), 
                idx_ye, idx_logtemp, idx_logrho, 
                delta_ye, delta_logtemp, delta_logrho));
        }
        if (eosvalues & cs2) {
            results.push_back(interpolate(cs2_values_.get(), 
                idx_ye, idx_logtemp, idx_logrho, 
                delta_ye, delta_logtemp, delta_logrho));
        }
        if (eosvalues & dedt) {
            results.push_back(interpolate(dedt_values_.get(), 
                idx_ye, idx_logtemp, idx_logrho, 
                delta_ye, delta_logtemp, delta_logrho));
        }
        if (eosvalues & dpdrhoe) {
            results.push_back(interpolate(dpdrhoe_values_.get(), 
                idx_ye, idx_logtemp, idx_logrho, 
                delta_ye, delta_logtemp, delta_logrho));
        }
        if (eosvalues & dpderho) {
            results.push_back(interpolate(dpderho_values_.get(), 
                idx_ye, idx_logtemp, idx_logrho, 
                delta_ye, delta_logtemp, delta_logrho));
        }

#if SHENEOS_SUPPORT_FULL_API
#endif

        return results;
    }
}}

///////////////////////////////////////////////////////////////////////////////
typedef sheneos::server::partition3d partition3d_type;

///////////////////////////////////////////////////////////////////////////////
// Serialization support for the actions
HPX_REGISTER_ACTION_EX(partition3d_type::init_action, 
    sheneos_partition3d_init_action);
HPX_REGISTER_ACTION_EX(partition3d_type::interpolate_action, 
    sheneos_partition3d_interpolate_action);

HPX_REGISTER_MINIMAL_COMPONENT_FACTORY(
    hpx::components::simple_component<partition3d_type>, 
    sheneos_partition_type);
HPX_DEFINE_GET_COMPONENT_TYPE(partition3d_type);

HPX_REGISTER_ACTION_EX(
    hpx::lcos::base_lco_with_value<std::vector<double> >::set_result_action,
    set_result_action_vector_double);
HPX_DEFINE_GET_COMPONENT_TYPE_STATIC(
    hpx::lcos::base_lco_with_value<std::vector<double> >,
    hpx::components::component_base_lco_with_value);
