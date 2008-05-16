//  Copyright (c) 2007-2008 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define MAX_ITERATIONS 100

#include <iostream>
#include <string>
#include <hpx/hpx.hpp>
#include <boost/cstdint.hpp>
#include <boost/assert.hpp>
#include <boost/lexical_cast.hpp>
#include <hpx/util/high_resolution_timer.hpp>

int main(int argc, char* argv[])
{
    // Check command line arguments.
    std::string host;
    unsigned short port;
    if (argc != 3)
    {
        std::cerr << "Using default settings: localhost:7911" << std::endl;
        std::cerr << "Possible arguments: <DGAS address> <DGAS port>" << std::endl;

        host = "localhost";
        port = 7911;
    }
    else
    {
        host = argv[1];
        port = boost::lexical_cast<unsigned short>(argv[2]);
    }

    try {
        using namespace hpx::naming;

        std::vector<double> timings;

        // this is our locality
        locality here("localhost", HPX_PORT);
        resolver_client resolver(host, port);
        
        id_type last_lowerid;
        
#if defined(MAX_ITERATIONS)
        for (int i = 0; i < MAX_ITERATIONS; ++i)
        {
#endif
        // retrieve the id prefix of this site
        id_type prefix1;
        if (resolver.get_prefix(here, prefix1))
            last_lowerid = prefix1;
                  
        // test get_id_range
        id_type lower1, upper1;
        
        // bind an arbitrary address
        for(int a=1;a<1000;a++)
        {
            resolver.bind(id_type(a), address(here, 1, 2));
        }
                 
        // registerid() associate this id with a namespace name
        // registerid() accepts char const*, to convert string into that we use c_str() function.
        std::string s;
        for(int a=1;a<1000;a++)
        {
            s="/test/foo/";
            s+= boost::lexical_cast<std::string>(a);//type conversion
            const char* b = s.c_str();
            resolver.registerid(b, id_type(a));
        }          
            
        // resolve this address
        address addr;
        hpx::util::high_resolution_timer t;		
        for(int i =1;i<1000;i++)
        {
            resolver.resolve(id_type(i), addr);
        }
        std::cout << "Measure_resolve:"<< t.elapsed() << std::endl << std::flush;
        
        resolver.get_statistics(timings);
        std::cout << " Time taken by get_prefix is: " << timings[0] <<  std::endl <<std::flush;
        std::cout << " Time taken by get_range  is: " << timings[1] <<  std::endl <<std::flush;
        std::cout << " Time taken by bind       is: " << timings[2] <<  std::endl <<std::flush;
        std::cout << " Time taken by registerid is: " << timings[6] <<  std::endl <<std::flush;
        std::cout << " Time taken by resolve    is: " << timings[4] <<  std::endl <<std::flush;
        
#if defined(MAX_ITERATIONS)
        }
#endif
    }
    catch (std::exception& e) {
        std::cerr << "std::exception caught: " << e.what() << "\n";
        return -1;
    }
    catch (...) {
        std::cerr << "unexpected exception caught\n";
        return -1;
    }
    return 0;
}

