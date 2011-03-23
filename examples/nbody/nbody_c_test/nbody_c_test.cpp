//  Copyright (c) 2007-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx.hpp>
#include <cmath>

//#include "../nbody_c/stencil.hpp"
#include "../nbody_c/stencil_data.hpp"
#include "../nbody_c/stencil_functions.hpp"
#include "../had_config.hpp"
#include <stdio.h>

#include <boost/scoped_array.hpp>

#define UGLIFY 1

///////////////////////////////////////////////////////////////////////////////
// windows needs to initialize MPFR in each shared library
#if defined(BOOST_WINDOWS) 

#include "../init_mpfr.hpp"

namespace hpx { namespace components { namespace nbody 
{
    // initialize mpreal default precision
    init_mpfr init_;
}}}
#endif

///////////////////////////////////////////////////////////////////////////////
// local functions
inline int floatcmp(had_double_type const& x1, had_double_type const& x2) 
{
  // compare to floating point numbers
  static had_double_type const epsilon = 1.e-8;
  if ( x1 + epsilon >= x2 && x1 - epsilon <= x2 ) {
    // the numbers are close enough for coordinate comparison
    return 1;
  } else {
    return 0;
  }
}

///////////////////////////////////////////////////////////////////////////
int generate_initial_data(stencil_data* val, int item, int maxitems, int row,
    Par const& par)
{
//     // provide initial data for the given data value 
//     val->x = item;
//     val->y = item + 100; 
//     val->z = item + 1000;
// 
//     val->vx = 0.0;
//     val->vy = 0.0;
//     val->vz = 0.0;
// 
//     val->ax = 0.0;
//     val->ay = 0.0;
//     val->az = 0.0;
//     
//     val->row = row;
//     val->column = item;
    val->node_type = par.bodies[item].node_type;
    val->x = par.bodies[item].px;
    val->y = par.bodies[item].py; 
    val->z = par.bodies[item].pz;

    val->vx = par.bodies[item].vx;
    val->vy = par.bodies[item].vy;
    val->vz = par.bodies[item].vz;

    val->ax = 0.0;
    val->ay = 0.0;
    val->az = 0.0;
//     std::cout << "Row: " << row << " item: " << item << " x: " << val->x << " y: " << val->y << " z: " << val->z << std::endl;
//     std::cout << " Maxitems " << maxitems << std::endl;
    val->row = row;
    val->column = item;


    return 1;
}

int rkupdate(hpx::memory::default_vector< nodedata* >::type const& vecval, stencil_data* result, 
             int compute_index, Par const& par)
{
  return 1;
}
