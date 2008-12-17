//  Copyright (c) 2007-2008 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_UTIL_AGAS_LOGGING_APR_10_2008_1032AM)
#define HPX_UTIL_AGAS_LOGGING_APR_10_2008_1032AM

#include <string>
#include <hpx/hpx_fwd.hpp>

#include <boost/logging/format/named_write_fwd.hpp>
#include <boost/logging/format_fwd.hpp>

BOOST_LOG_FORMAT_MSG(optimize::cache_string_one_str<>)

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace util 
{
    ///////////////////////////////////////////////////////////////////////////
    HPX_API_EXPORT std::string levelname(int level);

    ///////////////////////////////////////////////////////////////////////////
    typedef boost::logging::named_logger<>::type logger_type;
    typedef boost::logging::level::holder filter_type;

    ///////////////////////////////////////////////////////////////////////////
    HPX_EXPORT BOOST_DECLARE_LOG_FILTER(agas_level, filter_type)
    HPX_EXPORT BOOST_DECLARE_LOG(agas_logger, logger_type)

    #define LAGAS_(lvl)                                                       \
        BOOST_LOG_USE_LOG_IF_LEVEL(hpx::util::agas_logger(),                  \
            hpx::util::agas_level(), lvl)                                     \
        << hpx::util::levelname(::boost::logging::level::lvl) << " "          \
    /**/

    #define LAGAS_ENABLED(lvl)                                                \
        hpx::util::agas_level()->is_enabled(::boost::logging::level::lvl)     \
    /**/

    ///////////////////////////////////////////////////////////////////////////
    HPX_EXPORT BOOST_DECLARE_LOG_FILTER(timing_level, filter_type)
    HPX_EXPORT BOOST_DECLARE_LOG(timing_logger, logger_type)

    #define LTIM_(lvl)                                                        \
        BOOST_LOG_USE_LOG_IF_LEVEL(hpx::util::timing_logger(),                \
            hpx::util::timing_level(), lvl)                                   \
        << hpx::util::levelname(::boost::logging::level::lvl) << " "          \
    /**/

    #define LTIM_ENABLED(lvl)                                                 \
        hpx::util::timing_level()->is_enabled(::boost::logging::level::lvl)   \
    /**/

    ///////////////////////////////////////////////////////////////////////////
    HPX_EXPORT BOOST_DECLARE_LOG_FILTER(hpx_level, filter_type)
    HPX_EXPORT BOOST_DECLARE_LOG(hpx_logger, logger_type)

    #define LHPX_(lvl, cat)                                                   \
        BOOST_LOG_USE_LOG_IF_LEVEL(hpx::util::hpx_logger(),                   \
            hpx::util::hpx_level(), lvl)                                      \
        << hpx::util::levelname(::boost::logging::level::lvl)                 \
        << (cat)                                                              \
    /**/

    #define LHPX_ENABLED(lvl)                                                 \
        hpx::util::hpx_level()->is_enabled(::boost::logging::level::lvl)      \
    /**/

    ///////////////////////////////////////////////////////////////////////////
    // specific logging
    #define LTM_(lvl)   LHPX_(lvl, "  [TM] ")   /* thread manager */
    #define LRT_(lvl)   LHPX_(lvl, "  [RT] ")   /* runtime support */
    #define LOSH_(lvl)  LHPX_(lvl, " [OSH] ")   /* one size heap */
    #define LERR_(lvl)  LHPX_(lvl, " [ERR] ")   /* exceptions */
    #define LPT_(lvl)   LHPX_(lvl, "  [PT] ")   /* parcel transport */
    #define LAUX_(lvl)  LHPX_(lvl, " [AUX] ")   /* auxiliary */

    ///////////////////////////////////////////////////////////////////////////
    // errors are logged in a special manner (always to cerr and additionally,
    // if enabled to 'normal' logging destination as well)
    HPX_EXPORT BOOST_DECLARE_LOG_FILTER(hpx_error_level, filter_type)
    HPX_EXPORT BOOST_DECLARE_LOG(hpx_error_logger, logger_type)

    #define LFATAL_                                                           \
        BOOST_LOG_USE_LOG_IF_LEVEL(hpx::util::hpx_error_logger(),             \
            hpx::util::hpx_error_level(), fatal)                              \
        << hpx::util::levelname(::boost::logging::level::fatal)               \
        << (" [ERR] ")                                                        \
    /**/

    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        // get the data to use to pre-fill the runtime_configuration instance
        // with logging specific data
        std::vector<std::string> const& get_logging_data();

        // the init_logging type will be used for initialization purposes only as
        // well
        struct init_logging
        {
            init_logging(runtime_configuration& ini, bool isconsole, 
                naming::resolver_client& agas_client, 
                naming::id_type const& prefix);
        };
    }

///////////////////////////////////////////////////////////////////////////////
}}

#endif


