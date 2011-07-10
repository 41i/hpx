///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Bryce Lelbach 
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
///////////////////////////////////////////////////////////////////////////////

#include <hpx/include/iostreams.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/runtime/actions/plain_action.hpp>
#include <hpx/runtime/components/plain_component_factory.hpp>
#include <hpx/lcos/eager_future.hpp>

using boost::program_options::variables_map;
using boost::program_options::options_description;

using hpx::cout;
using hpx::endl;

using hpx::init;
using hpx::finalize;

///////////////////////////////////////////////////////////////////////////////
void startup_()
{
    cout() << "startup function called" << endl;
}

///////////////////////////////////////////////////////////////////////////////
void shutdown_()
{
    cout() << "shutdown function called" << endl;
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(variables_map& vm)
{
    // Do nothing.
    finalize();
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    // Configure application-specific options
    options_description
       desc_commandline("usage: " HPX_APPLICATION_STRING " [options]");

    std::vector<boost::function<void()> > startup_functions, shutdown_functions;

    startup_functions.push_back(boost::function<void()>(startup_));
    shutdown_functions.push_back(boost::function<void()>(shutdown_));

    // Initialize and run HPX
    return init
        (desc_commandline, argc, argv, startup_functions, shutdown_functions);
}

