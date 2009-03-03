//  Copyright (c) 2007-2009 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx_fwd.hpp>
#include <hpx/runtime/actions/continuation_impl.hpp>
#include <hpx/performance_counters/base_performance_counter.hpp>

#include <hpx/util/portable_binary_iarchive.hpp>
#include <hpx/util/portable_binary_oarchive.hpp>

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/export.hpp>

///////////////////////////////////////////////////////////////////////////////
// Serialization support for the performance counter  actions
HPX_REGISTER_ACTION_EX(
    hpx::performance_counters::base_performance_counter::get_counter_info_action, 
    performance_counter_get_counter_info_action);
HPX_REGISTER_ACTION_EX(
    hpx::performance_counters::base_performance_counter::get_counter_value_action, 
    performance_counter_get_counter_value_action);

HPX_REGISTER_ACTION_EX(
    hpx::lcos::base_lco_with_value<hpx::performance_counters::counter_info>::set_result_action, 
    set_result_action_counter_info);
HPX_REGISTER_ACTION_EX(
    hpx::lcos::base_lco_with_value<hpx::performance_counters::counter_value>::set_result_action, 
    set_result_action_counter_value);

///////////////////////////////////////////////////////////////////////////////
HPX_DEFINE_GET_COMPONENT_TYPE(hpx::performance_counters::base_performance_counter);
HPX_DEFINE_GET_COMPONENT_TYPE(hpx::lcos::base_lco_with_value<hpx::performance_counters::counter_info>);
HPX_DEFINE_GET_COMPONENT_TYPE(hpx::lcos::base_lco_with_value<hpx::performance_counters::counter_value>);

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace performance_counters 
{
    /// \brief Create a full name of a counter from the contents of the given 
    ///        \a counter_path_elements instance.
    counter_status get_counter_name(counter_path_elements const& path, 
        std::string& result)
    {
        result = "/";
        if (!path.objectname_.empty())
            result += path.objectname_;

        if (!path.parentinstancename_.empty() || !path.instancename_.empty())
        {
            result += "(";
            if (!path.parentinstancename_.empty())
            {
                result += path.parentinstancename_;
                result += "/";
            }
            if (path.instancename_.empty())
                return status_invalid_data;

            result += path.instancename_;
            if (path.instanceindex_ != 0)
            {
                result += "#";
                result += boost::lexical_cast<std::string>(path.instanceindex_);
            }
            result += ")";
        }
        if (path.countername_.empty())
            return status_invalid_data;

        result += "/";
        result += path.countername_;

        return status_valid_data;
    }

    /// \brief Fill the given \a counter_path_elements instance from the given 
    ///        full name of a counter
    ///
    ///    /objectname(parentinstancename/instancename#instanceindex)/countername
    counter_status get_counter_path_elements(std::string const& name, 
        counter_path_elements& path)
    {
        if (name.empty() || name[0] != '/')
            return status_invalid_data;

        std::string::size_type p = name.find_first_of("(/", 1);
        if (p == std::string::npos)
            return status_invalid_data;

        // object name is the first part of the full name
        path.objectname_ = name.substr(1, p-1);
        if (path.objectname_.empty())
            return status_invalid_data;

        if (name[p] == '(') {
            std::string::size_type p1 = name.find_first_of(")", p);
            if (p1 == std::string::npos || p1 >= name.size()) 
                return status_invalid_data;     // unmatched parenthesis

            // instance name is in between p and p1
            std::string::size_type p2 = name.find_last_of("/#", p1);
            if (p2 == std::string::npos) 
                return status_invalid_data;

            if (name[p2] == '#') {
                // instance index
                try {
                    path.instanceindex_ = 
                        boost::lexical_cast<boost::uint32_t>(name.substr(p2+1, p1-p2-1));
                }
                catch (boost::bad_lexical_cast const& /*e*/) {
                    return status_invalid_data;
                }

                std::string::size_type p3 = name.find_last_of("/", p2);
                if (p3 != std::string::npos && p3 > p) {
                    // instance name and parent instance name
                    path.parentinstancename_ = name.substr(p+1, p3-p-1);
                    if (path.parentinstancename_.empty())
                        return status_invalid_data;
                    path.instancename_ = name.substr(p3+1, p2-p3-1);
                    if (path.instancename_.empty())
                        return status_invalid_data;
                }
                else {
                    // instance name only
                    path.instancename_ = name.substr(p+1, p2-p-1);
                    if (path.instancename_.empty())
                        return status_invalid_data;
                }
            }
            else if (p2 > p) {
                // instance name and parent instance name
                path.parentinstancename_ = name.substr(p+1, p2-p-1);
                if (path.parentinstancename_.empty())
                    return status_invalid_data;
                path.instancename_ = name.substr(p2+1, p1-p2-1);
                if (path.instancename_.empty())
                    return status_invalid_data;
            }
            else {
                // just instance name
                path.instancename_ = name.substr(p+1, p1-p-1);
                if (path.instancename_.empty())
                    return status_invalid_data;
            }
            p = p1+1;
        }

        // counter name is always the last part of the full name
        path.countername_ = name.substr(p+1);
        if (path.countername_.empty())
            return status_invalid_data;
        if (path.countername_.find_first_of("/") != std::string::npos)
            return status_invalid_data;

        return status_valid_data;
    }

}}

