//  Copyright (c) 2007-2008 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/detail/lightweight_test.hpp>

using namespace hpx;

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    // try to access some memory directly
    boost::uint32_t value = 0;

    // store a value to memory
    naming::id_type memid = applier::get_applier().get_memory_gid();
    typedef components::server::memory::store32_action store_action_type;
    applier::apply<store_action_type>(memid, boost::uint64_t(&value), 1);

    BOOST_TEST(value == 1);

    // read the value back from memory (using an eager_future)
    typedef components::server::memory::load32_action load_action_type;
    lcos::eager_future<load_action_type> ef(memid, boost::uint64_t(&value));

    boost::uint32_t result1 = ef.get();
    BOOST_TEST(result1 == value);

    // read the value back from memory (using a lazy_future)
    lcos::lazy_future<load_action_type> lf;

    boost::uint32_t result2 = lf.get(memid, boost::uint64_t(&value));
    BOOST_TEST(result2 == value);

    // initiate shutdown of the runtime system
    components::stubs::runtime_support::shutdown_all();

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    try {
        // Check command line arguments.
        std::string host;
        boost::uint16_t ps_port, agas_port;

        // Check command line arguments.
        if (argc != 4) {
            std::cerr << "Usage: memory_tests hpx_addr hpx_port agas_port" 
                << std::endl;
            return -1;
        }
        else {
            host = argv[1];
            ps_port = boost::lexical_cast<boost::uint16_t>(argv[2]);
            agas_port  = boost::lexical_cast<boost::uint16_t>(argv[3]);
        }

        // initialize the AGAS service
        hpx::util::io_service_pool agas_pool; 
        hpx::naming::resolver_server agas(agas_pool, 
            hpx::naming::locality(host, agas_port));

        // initialize and start the HPX runtime
        hpx::runtime rt(host, ps_port, host, agas_port);
        rt.run(hpx_main, 2);
    }
    catch (std::exception& e) {
        BOOST_TEST(false);
        std::cerr << "std::exception caught: " << e.what() << "\n";
    }
    catch (...) {
        BOOST_TEST(false);
        std::cerr << "unexpected exception caught\n";
    }
    return boost::report_errors();
}
