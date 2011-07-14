////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Bryce Lelbach
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////

namespace hpx { namespace performance_counters
{
    inline void counter_shutdown(boost::shared_ptr<manage_counter> const& p)
    {
        BOOST_ASSERT(p);
        p->uninstall();
    }

    HPX_EXPORT void install_counter(std::string const& name,
        boost::function<boost::int64_t()> const& f, error_code& ec)
    {
        boost::shared_ptr<manage_counter> p(new manage_counter);

        // Install the counter instance.
        p->install(name, f, ec);  

        // Register the shutdown function which will clean up this counter.
        get_runtime().add_shutdown_function
            (boost::bind(&counter_shutdown, p));
    }
}}
