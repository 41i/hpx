//  Copyright (c) 2007-2008 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <cstring>
#include <iostream>

#include <hpx/hpx.hpp>
#include <hpx/components/distributing_factory/distributing_factory.hpp>
#include <hpx/components/amr/stencil_value.hpp>
#include <hpx/components/amr_test/stencil.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <boost/assign/std/vector.hpp>

namespace po = boost::program_options;

using namespace hpx;
using namespace std;

///////////////////////////////////////////////////////////////////////////////
// Create functional components, one for each data point, use those to 
// initialize the stencil value instances
void init_stencils(applier::applier& appl,
    components::distributing_factory::result_type const& values,
    components::distributing_factory::result_type const& stencils)
{
    // values are allocated in blocks, stencil_values are allocated separately
    BOOST_ASSERT(values[0].count_ == stencils.size());
    for (std::size_t i = 0; i < stencils.size(); ++i) 
    {
        BOOST_ASSERT(1 == stencils[i].count_);
        components::amr::stubs::stencil_value<3>::set_functional_component(
            appl, values[0].first_gid_ + i, stencils[i].first_gid_);
    }
}

///////////////////////////////////////////////////////////////////////////////
// Get gids of output ports of all stencils
void get_output_ports(threads::thread_self& self, applier::applier& appl,
    components::distributing_factory::result_type const& values,
    std::vector<std::vector<naming::id_type> >& outputs)
{
    typedef components::distributing_factory::result_type result_type;
    typedef 
        std::vector<lcos::future_value<std::vector<naming::id_type> > >
    lazyvals_type;

    // start an asynchronous operation for each of the stencil value instances
    lazyvals_type lazyvals;
    for (std::size_t i = 0; i < values[0].count_; ++i) 
    {
        lazyvals.push_back(components::amr::stubs::stencil_value<3>::
            get_output_ports_async(appl, values[0].first_gid_ + i));
    }

    // now wait for the results
    lazyvals_type::iterator lend = lazyvals.end();
    for (lazyvals_type::iterator lit = lazyvals.begin(); lit != lend; ++lit) 
    {
        outputs.push_back((*lit).get(self));
    }
}

///////////////////////////////////////////////////////////////////////////////
// Connect the given output ports with the correct input ports, creating the 
// required static dataflow structure.
//
// Currently we have exactly one stencil_value instance per data point, where 
// the output ports of a stencil_value are connected to the input ports of the 
// direct neighbors of itself.
inline std::size_t mod(int idx, std::size_t maxidx = 3)
{
    return (idx < 0) ? (idx + maxidx) % maxidx : idx % maxidx;
}

void connect_input_ports(applier::applier& appl,
    components::distributing_factory::result_type const& values,
    std::vector<std::vector<naming::id_type> > const& outputs)
{
    typedef components::distributing_factory::result_type result_type;

    BOOST_ASSERT(outputs.size() == values[0].count_);
    for (int i = 0; i < (int)values[0].count_; ++i) 
    {
        using namespace boost::assign;

        std::vector<naming::id_type> output_ports;
        output_ports += 
                outputs[mod(i-1)][2],    // sw input is connected to the ne output of the w element
                outputs[i][1],           // s input is connected to the n output of the element itself
                outputs[mod(i+1)][0]     // se input is connected to the nw output of the e element
            ;

        components::amr::stubs::stencil_value<3>::
            connect_input_ports(appl, values[0].first_gid_ + i, output_ports);
    }
}

///////////////////////////////////////////////////////////////////////////////
threads::thread_state 
hpx_main(threads::thread_self& self, applier::applier& appl)
{
    // get component types needed below
    components::component_type stencil_type = 
        components::get_component_type<components::amr::stencil>();
    components::component_type stencil_value_type = 
        components::get_component_type<components::amr::server::stencil_value<3> >();

    {
        typedef components::distributing_factory::result_type result_type;

        // create a distributing factory locally
        components::distributing_factory factory(
            components::distributing_factory::create(self, appl, 
                appl.get_runtime_support_gid(), true));

        // create a couple of stencil (functional) components and the same 
        // amount of stencil_value components
        result_type stencils = factory.create_components(self, stencil_type, 3);
        result_type values = factory.create_components(self, stencil_value_type, 3);

        // initialize stencil_values using the stencil (functional) components
        init_stencils(appl, values, stencils);

        // ask stencil instances for their output gids
        std::vector<std::vector<naming::id_type> > outputs;
        get_output_ports(self, appl, values, outputs);

        // connect output gids with corresponding stencil inputs
        connect_input_ports(appl, values, outputs);



        // free all allocated components
//         factory.free_components(values);
//         factory.free_components(stencils);

    }   // distributing_factory needs to go out of scope before shutdown

    // initiate shutdown of the runtime systems on all localities
    components::stubs::runtime_support::shutdown_all(appl);

    return threads::terminated;
}

///////////////////////////////////////////////////////////////////////////////
bool parse_commandline(int argc, char *argv[], po::variables_map& vm)
{
    try {
        po::options_description desc_cmdline ("Usage: hpx_runtime [options]");
        desc_cmdline.add_options()
            ("help,h", "print out program usage (this message)")
            ("run_agas_server,r", "run AGAS server as part of this runtime instance")
            ("agas,a", po::value<std::string>(), 
                "the IP address the AGAS server is running on (default taken "
                "from hpx.ini), expected format: 192.168.1.1:7912")
            ("hpx,x", po::value<std::string>(), 
                "the IP address the HPX parcelport is listening on (default "
                "is localhost:7910), expected format: 192.168.1.1:7913")
            ("threads,t", po::value<int>(), 
                "the number of operating system threads to spawn for this"
                "HPX locality")
        ;

        po::store(po::command_line_parser(argc, argv)
            .options(desc_cmdline).run(), vm);
        po::notify(vm);

        // print help screen
        if (vm.count("help")) {
            std::cout << desc_cmdline;
            return false;
        }
    }
    catch (std::exception const& e) {
        std::cerr << "amr_client: exception caught: " << e.what() << std::endl;
        return false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
inline void 
split_ip_address(std::string const& v, std::string& addr, boost::uint16_t& port)
{
    std::string::size_type p = v.find_first_of(":");
    try {
        if (p != std::string::npos) {
            addr = v.substr(0, p);
            port = boost::lexical_cast<boost::uint16_t>(v.substr(p+1));
        }
        else {
            addr = v;
        }
    }
    catch (boost::bad_lexical_cast const& /*e*/) {
        std::cerr << "amr_client: illegal port number given: " << v.substr(p+1) << std::endl;
        std::cerr << "            using default value instead: " << port << std::endl;
    }
}

///////////////////////////////////////////////////////////////////////////////
// helper class for AGAS server initialization
class agas_server_helper
{
public:
    agas_server_helper(std::string host, boost::uint16_t port)
      : agas_pool_(), agas_(agas_pool_, host, port)
    {
        agas_.run(false);
    }

private:
    hpx::util::io_service_pool agas_pool_; 
    hpx::naming::resolver_server agas_;
};

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    try {
        // analyze the command line
        po::variables_map vm;
        if (!parse_commandline(argc, argv, vm))
            return -1;

        // Check command line arguments.
        std::string hpx_host("localhost"), agas_host;
        boost::uint16_t hpx_port = HPX_PORT, agas_port = 0;
        int num_threads = 1;

        // extract IP address/port arguments
        if (vm.count("agas")) 
            split_ip_address(vm["agas"].as<std::string>(), agas_host, agas_port);

        if (vm.count("hpx")) 
            split_ip_address(vm["hpx"].as<std::string>(), hpx_host, hpx_port);

        if (vm.count("threads"))
            num_threads = vm["threads"].as<int>();

        // initialize and run the AGAS service, if appropriate
        std::auto_ptr<agas_server_helper> agas_server;
        if (vm.count("run_agas_server"))  // run the AGAS server instance here
            agas_server.reset(new agas_server_helper(agas_host, agas_port));

        // initialize and start the HPX runtime
        hpx::runtime rt(hpx_host, hpx_port, agas_host, agas_port);
        rt.run(hpx_main, num_threads);
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

