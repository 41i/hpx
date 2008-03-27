//  Copyright (c) 2007-2008 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <string>

#include <boost/lexical_cast.hpp>

#include <hpx/hpx.hpp>

///////////////////////////////////////////////////////////////////////////////
#if defined(BOOST_WINDOWS)

#include <boost/function.hpp>

boost::function0<void> console_ctrl_function;

BOOL WINAPI console_ctrl_handler(DWORD ctrl_type)
{
    switch (ctrl_type) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        console_ctrl_function();
        return TRUE;
        
    default:
        return FALSE;
    }
}

#else

#include <pthread.h>
#include <signal.h>

#endif

///////////////////////////////////////////////////////////////////////////////
//  This get's called whenever a parcel was received, it just sends back any 
//  received parcel
void received_parcel(hpx::parcelset::parcelport& ps)
{
    hpx::parcelset::parcel p;
    if (ps.get_parcel(p))
    {
        std::cout << "Received parcel: " << p.get_parcel_id() << std::endl;
        p.set_destination(p.get_source());
        p.set_source(0);
        ps.sync_put_parcel(p);
    }
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    try {
        std::string ps_host, gas_host;
        unsigned short ps_port, gas_port;
        std::size_t num_threads;
        
        // Check command line arguments.
        if (argc != 6) 
        {
            std::cerr << "Using default settings: localhost:7911 localhost:7912 threads:1" 
                      << std::endl;
            std::cerr << "Possible arguments: <HPX address> <HPX port> <DGAS address> <DGAS port> <num_threads>"
                      << std::endl;

            ps_host = "localhost";
            ps_port = 7911;
            gas_host = "localhost";
            gas_port = 7912;
            num_threads = 1;
        }
        else
        {
            ps_host = argv[1];
            ps_port = boost::lexical_cast<unsigned short>(argv[2]);
            gas_host = argv[3];
            gas_port  = boost::lexical_cast<unsigned short>(argv[4]);
            num_threads = boost::lexical_cast<std::size_t>(argv[5]);
        }

#if defined(BOOST_WINDOWS)
        // Run the ParalleX services
        hpx::naming::resolver_server dgas_s(gas_host, gas_port, true, num_threads);
        hpx::naming::resolver_client dgas_c(gas_host, gas_port);
        hpx::parcelset::parcelport ps(dgas_c, hpx::naming::locality(ps_host, ps_port));

        // Set console control handler to allow server to be stopped.
        console_ctrl_function = 
            boost::bind(&hpx::parcelset::parcelport::stop, &ps);
        SetConsoleCtrlHandler(console_ctrl_handler, TRUE);

        // Run the server until stopped.
        std::cout << "Parcelset (server) listening at port: " << ps_port 
                  << std::endl;
        std::cout << "GAS server listening at port: " << gas_port 
                  << std::flush << std::endl;

        ps.register_event_handler(received_parcel);
        
        ps.run();
        dgas_s.stop();
#else
        // Block all signals for background thread.
        sigset_t new_mask;
        sigfillset(&new_mask);
        sigset_t old_mask;
        pthread_sigmask(SIG_BLOCK, &new_mask, &old_mask);
        
        // Run the ParalleX services in a background thread
        hpx::naming::resolver_server dgas_s(gas_host, gas_port, true, num_threads);
        hpx::naming::resolver_client dgas_c(gas_host, gas_port);
        hpx::parcelset::parcelport ps(dgas_c, hpx::naming::locality(ps_host, ps_port));

        boost::thread t1(boost::bind(&hpx::parcelset:parcelport::run, &ps, true));

        // print a message stating readiness
        std::cout << "Parcelset (server) listening at port: " << ps_port 
                  << std::endl;
        std::cout << "GAS server listening at port: " << gas_port 
                  << std::flush << std::endl;

        // Restore previous signals.
        pthread_sigmask(SIG_SETMASK, &old_mask, 0);

        // Wait for signal indicating time to shut down.
        sigset_t wait_mask;
        sigemptyset(&wait_mask);
        sigaddset(&wait_mask, SIGINT);
        sigaddset(&wait_mask, SIGQUIT);
        sigaddset(&wait_mask, SIGTERM);
        pthread_sigmask(SIG_BLOCK, &wait_mask, 0);
        int sig = 0;
        sigwait(&wait_mask, &sig);

        // Stop the servers.
        ps.stop();
        dgas_s.stop();
        
        t1.join();
#endif
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

