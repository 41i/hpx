//  Copyright (c) 2010-2011 Dylan Stark
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>

#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>

#include <hpx/runtime/actions/plain_action.hpp>
#include <hpx/runtime/components/plain_component_factory.hpp>

#include <boost/program_options.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/export.hpp>

using namespace hpx;
namespace po = boost::program_options;

///////////////////////////////////////////////////////////////////////////////
// Helpers
typedef hpx::naming::id_type id_type;
typedef hpx::naming::gid_type gid_type;

// For a multi-locality run, this returns some locality that is not L0 
// (where the main thread was spawned). This guarantees that the "get()"
// of the factorial value at the tail end of the recursion is remote.
inline gid_type find_somewhere_but_home(int i)
{
  std::vector<gid_type> locales;
  hpx::applier::get_applier().get_agas_client().get_prefixes(locales);

  int location = i%locales.size();
  if (location == 0 && locales.size() > 1)
    location = 1;

  return locales[location];
}

///////////////////////////////////////////////////////////////////////////////
// More helpers
template<typename Action, typename Arg0>
inline void apply(Arg0 arg0, id_type k, id_type h)
{
  naming::id_type 
      somewhere(find_somewhere_but_home(arg0), naming::id_type::unmanaged);
  hpx::applier::apply<Action>(somewhere, arg0, k, h);
}

template<typename Action, typename Arg0, typename Arg1>
inline void apply(Arg0 arg0, Arg1 arg1, id_type k, id_type h)
{
  naming::id_type 
      somewhere(find_somewhere_but_home(arg0), naming::id_type::unmanaged);
  hpx::applier::apply<Action>(somewhere, arg0, arg1, k, h);
}

template<typename Arg0>
inline void trigger(id_type k, Arg0 arg0)
{
  typedef typename
      lcos::template base_lco_with_value<Arg0>::set_result_action set_action;
  hpx::applier::apply<set_action>(k,arg0);
}

template<typename Value>
inline Value get(id_type k)
{
  typedef typename
      lcos::template base_lco_with_value<Value>::get_value_action get_action;
  return lcos::eager_future<get_action>(k).get();
}

///////////////////////////////////////////////////////////////////////////////
// The following CPS example was adapted from 
// http://en.wikipedia.org/wiki/Continuation-passing_style (Jun 30, 2010).
//
// def factorial(n, k):
//     factorial_aux(n, 1, k)
//
// def factorial_aux(n, a, k):
//     if n == 0:
//         k(a)
//     else:
//         factorial_aux(n-1, n*a, k)
//
// >>> k = dataflow_variable()
// >>> factorial(3, k)
//
// >>> print k
// 6

///////////////////////////////////////////////////////////////////////////////
void factorial_get_aux(int, int, id_type, id_type);
typedef actions::plain_action4<int, int, id_type, id_type, factorial_get_aux> 
    factorial_get_aux_action;
HPX_REGISTER_PLAIN_ACTION(factorial_get_aux_action);

void factorial_get(int, id_type, id_type);
typedef actions::plain_action3<int, id_type, id_type, factorial_get> 
    factorial_get_action;
HPX_REGISTER_PLAIN_ACTION(factorial_get_action);

void factorial_get_aux(int n, int a, id_type k, id_type h)
{
  if (n == 0)
  {
    std::cout << "Triggering k(" << a << ")" << std::endl;
    trigger<int>(k, a);

    std::cout << "Getting value of k" << std::endl;
    get<int>(k);

    std::cout << "Triggering h(0)" << std::endl;
    trigger<int>(h, 0);
  }
  else
  {
    std::cout << "Applying factorial_aux(" << n-1 
              << ", " << n*a << ", k, h)" << std::endl;
    apply<factorial_get_aux_action>(n-1, n*a, k, h);
  }
}

void factorial_get(int n, id_type k, id_type h)
{
  std::cout << "Applying factorial_aux(" << n << ", 1, k, h)" << std::endl;
  apply<factorial_get_aux_action>(n, 1, k, h);
}

///////////////////////////////////////////////////////////////////////////////
typedef hpx::lcos::dataflow_variable<int,int> dataflow_int_type;

///////////////////////////////////////////////////////////////////////////////
int hpx_main(po::variables_map &vm)
{
    int n = 0;
    if (vm.count("value"))
      n = vm["value"].as<int>();

    // Create DFV to guard against premature termination of main thread
    dataflow_int_type halt;

    // Create DFV for storing final value
    std::cout << ">>> kn = dataflow()" << std::endl;
    dataflow_int_type kn;

    if (n >= 0)
    {
      std::cout << ">>> factorial(" << n << ", kn, halt)" << std::endl;
      std::cout << "Applying factorial(" << n << ", kn, halt)" << std::endl;
      apply<factorial_get_action>(n, kn.get_gid(), halt.get_gid());

      std::cout << ">>> print kn" << std::endl;
      std::cout << kn.get() << std::endl;
    }

    std::cout << "Getting value of halt" << std::endl;
    halt.get();

    // initiate shutdown of the runtime systems on all localities
    components::stubs::runtime_support::shutdown_all();

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
  po::options_description 
      desc_commandline ("Usage: factorial_get [hpx_options] [options]");
  desc_commandline.add_options()
      ("value,v", po::value<int>(), 
       "the number to be used as the argument to factorial (default is 0)")
      ;

  int retcode = hpx_init(desc_commandline, argc, argv);
  return retcode;
}

