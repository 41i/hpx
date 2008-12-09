//  Copyright (c) 2007-2008 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_UTIL_VALUE_LOGGER_DEC_08_0548PM)
#define HPX_UTIL_VALUE_LOGGER_DEC_08_0548PM

#include <fstream>
#include <boost/version.hpp>
#include <boost/cstdint.hpp>
#include <boost/lockfree/primitives.hpp>

#include <hpx/util/logging.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace util
{
    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        struct value_logger_ref_time
        {
            value_logger_ref_time()
            {
                start_ = boost::lockfree::hrtimer_ticks();
            }
            boost::uint64_t start_;
        };

        value_logger_ref_time const value_logger_ref_time_;
    }

    ///////////////////////////////////////////////////////////////////////////
    /// The \a block_profiler class can be used to collect timings for a block
    /// of code. It measures the execution time for each of the executions and
    /// collects the number of invocations, the average, and the variance of 
    /// the measured execution times.
    template <typename T>
    class value_logger
    {
    private:
        enum { hpx_initial_times_size = 64000 };
        typedef std::vector<std::pair<boost::uint64_t, T> > values_type;

    public:
        value_logger(char const* const description)
          : description_(description)
        {
            if (LTIM_ENABLED(fatal)) 
                values_.reserve(hpx_initial_times_size);
        }

        ~value_logger()
        {
            if (!LTIM_ENABLED(fatal)) 
                return;     // generate output only if logging is enabled

            std::string name(description_);
            name += "." + boost::lexical_cast<std::string>(getpid());
            name += ".csv";

            std::ofstream out(name.c_str());

            typename values_type::iterator eit = values_.end();
            typename values_type::iterator bit = values_.begin();
            for (typename values_type::iterator it = bit; it != eit; ++it)
            {
                boost::uint64_t t = (*it).first - detail::value_logger_ref_time_.start_;
                out << t << "," << (*it).second << std::endl;
            }
        }

        void snapshot(T const& t)
        {
            if (LTIM_ENABLED(fatal)) 
                values_.push_back(std::make_pair(boost::lockfree::hrtimer_ticks(), t));
        }

    private:
        char const* const description_;
        values_type values_;
    };

}}

#endif
