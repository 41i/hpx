//  Copyright (c) 2007-2008 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <string>

#include <boost/lexical_cast.hpp>

#include <hpx/hpx.hpp>

///////////////////////////////////////////////////////////////////////////////
//  This get's called whenever a parcel was received, it just sets a flag
bool received = false;

void received_parcel(hpx::parcelset::parcelport& ps)
{
    hpx::parcelset::parcel p;
    if (ps.get_parcel(p))
    {
        std::cout << "Received parcel: " << std::hex << p.get_parcel_id() 
                  << std::endl;
        received = true;
    }
}

int main(int argc, char* argv[])
{
    try {
        // Check command line arguments.
        std::string ps_host, remote_ps_host, gas_host;
        unsigned short ps_port, remote_ps_port, gas_port;
        
        // Check command line arguments.
        if (argc != 7) 
        {
            std::cerr << "Using default settings: localhost:7913 localhost:7912 localhost:7911" 
                      << std::endl;
            std::cerr << "Possible arguments: <HPX address> <HPX port> <DGAS address> <DGAS port> <remote HPX address> <remote HPX port>"
                      << std::endl;

            ps_host = "localhost";
            ps_port = 7913;
            gas_host = "localhost";
            gas_port = 7912;
            remote_ps_host = "localhost";
            remote_ps_port = 7911;
        }
        else
        {
            ps_host = argv[1];
            ps_port = boost::lexical_cast<unsigned short>(argv[2]);
            gas_host = argv[3];
            gas_port  = boost::lexical_cast<unsigned short>(argv[4]);
            remote_ps_host = argv[5];
            remote_ps_port = boost::lexical_cast<unsigned short>(argv[6]);
        }

        // Start ParalleX services
        hpx::naming::resolver_client dgas_c(gas_host, gas_port);
        hpx::parcelset::parcelport ps(dgas_c, hpx::naming::locality(ps_host, ps_port));

        // sleep for a second to give parcelset server a chance to startup
        boost::xtime xt;
        boost::xtime_get(&xt, boost::TIME_UTC);
        xt.sec += 1;
        boost::thread::sleep(xt);
                            
        // retrieve prefix for remote locality
        boost::uint64_t remote_prefix = 0;
        hpx::naming::locality remote_l(remote_ps_host, remote_ps_port);
        dgas_c.get_prefix(remote_l, remote_prefix);
        
        ps.run(false);

        std::cout << "Parcelset (client) listening at port: " << ps_port 
                  << std::flush << std::endl;

        
        // send parcel to remote locality        
        hpx::parcelset::parcel p(remote_prefix);
        hpx::parcelset::parcel_id id = ps.sync_put_parcel(p);

        std::cout << "Successfully sent parcel: " << std::hex << id << std::endl;

        ps.register_event_handler(received_parcel);

        ps.stop();
    }
    catch (std::exception& e) {
        std::cerr << "std::exception caught: " << e.what() << "\n";
        return -1;
    }
    catch (...) {
        std::cerr << "unexpected exception caught\n";
        return -2;
    }
    return 0;
}

