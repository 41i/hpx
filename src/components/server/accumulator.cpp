//  Copyright (c) 2007-2008 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/serialization/export.hpp>
#include <hpx/components/server/accumulator.hpp>

///////////////////////////////////////////////////////////////////////////////
// enable serialization support (these need to be in a single compilation unit 
// and in the global namespace)
BOOST_CLASS_EXPORT(hpx::components::server::accumulator::init_action);
BOOST_CLASS_EXPORT(hpx::components::server::accumulator::add_action);
BOOST_CLASS_EXPORT(hpx::components::server::accumulator::print_action);

