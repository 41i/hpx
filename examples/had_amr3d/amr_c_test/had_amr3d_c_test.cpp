//  Copyright (c) 2007-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx.hpp>
#include <cmath>

//#include "../amr_c/stencil.hpp"
#include "../amr_c/stencil_data.hpp"
#include "../amr_c/stencil_functions.hpp"
#include "../had_config.hpp"
#include <stdio.h>

#define UGLIFY 1

///////////////////////////////////////////////////////////////////////////////
// windows needs to initialize MPFR in each shared library
#if defined(BOOST_WINDOWS) 

#include "../init_mpfr.hpp"

namespace hpx { namespace components { namespace amr 
{
    // initialize mpreal default precision
    init_mpfr init_;
}}}
#endif

void calcrhs(struct nodedata * rhs,
             std::vector< nodedata* > const& vecval,
             int flag, had_double_type const& dx,
             bool boundary,int i,int j,int k, Par const& par);

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

inline had_double_type initial_chi(had_double_type const& r,Par const& par) 
{
  return par.amp*exp( -(r-par.R0)*(r-par.R0)/(par.delta*par.delta) );   
}

inline had_double_type initial_Phi(had_double_type const& r,Par const& par) 
{
  // Phi is the r derivative of chi
  static had_double_type const c_m2 = -2.;
  return par.amp*exp( -(r-par.R0)*(r-par.R0)/(par.delta*par.delta) ) * ( c_m2*(r-par.R0)/(par.delta*par.delta) );
}

///////////////////////////////////////////////////////////////////////////
int generate_initial_data(stencil_data* val, int item, int maxitems, int row,
    Par const& par)
{
    // provide initial data for the given data value 
    val->max_index_ = maxitems;
    val->index_ = item;
    val->timestep_ = 0;
    val->cycle_ = 0;

    val->granularity = par.granularity;
    val->x_.resize(par.granularity);
    val->y_.resize(par.granularity);
    val->z_.resize(par.granularity);
    val->value_.resize(par.granularity*par.granularity*par.granularity);

    //number of values per stencil_data
    nodedata node;

    val->level_= 0;
    had_double_type dx = par.dx0;

    int tmp_index = item/par.nx0;
    int c = tmp_index/par.nx0;
    int b = tmp_index%par.nx0;
    int a = item - par.nx0*(b+c*par.nx0);
    BOOST_ASSERT(item == a + par.nx0*(b+c*par.nx0));

    static had_double_type const c_0 = 0.0;
    static had_double_type const c_7 = 7.0;
    static had_double_type const c_6 = 6.0;
    static had_double_type const c_8 = 8.0;

    for (int i=0;i<par.granularity;i++) {
      had_double_type x = par.minx0 + a*dx*par.granularity + i*dx;
      had_double_type y = par.minx0 + b*dx*par.granularity + i*dx;
      had_double_type z = par.minx0 + c*dx*par.granularity + i*dx;

      val->x_[i] = x;
      val->y_[i] = y;
      val->z_[i] = z;
    }

    for (int k=0;k<par.granularity;k++) {
    for (int j=0;j<par.granularity;j++) {
    for (int i=0;i<par.granularity;i++) {
      had_double_type x = val->x_[i];
      had_double_type y = val->y_[j];
      had_double_type z = val->z_[k];

      had_double_type r = sqrt(x*x+y*y+z*z);

      if ( pow(r-par.R0,2) <= par.delta*par.delta && r > 0 ) {
        had_double_type Phi = par.amp*pow((r-par.R0)*(r-par.R0)
                             -par.delta*par.delta,4)/pow(par.delta,8)/r;
        had_double_type D1Phi = par.amp*pow((r-par.R0)*(r-par.R0)-par.delta*par.delta,3)*
                                 (c_7*r*r-c_6*r*par.R0+par.R0*par.R0+par.delta*par.delta)*
                                 x/(r*r)/pow(par.delta,8);
        had_double_type D2Phi  = par.amp*pow((r-par.R0)*(r-par.R0)-par.delta*par.delta,3)*
                                 (c_7*r*r-c_6*r*par.R0+par.R0*par.R0+par.delta*par.delta)*
                                 y/(r*r)/pow(par.delta,8); 
        had_double_type D3Phi = par.amp*pow((r-par.R0)*(r-par.R0)-par.delta*par.delta,3)*
                                 (c_7*r*r-c_6*r*par.R0+par.R0*par.R0+par.delta*par.delta)*
                                 z/(r*r)/pow(par.delta,8);
        had_double_type D4Phi = par.amp_dot*pow((r-par.R0)*(r-par.R0)-par.delta*par.delta,3)*
                                c_8*(par.R0-r)/pow(par.delta,8)/r; 
  
        node.phi[0][0] = Phi;
        node.phi[0][1] = D1Phi;
        node.phi[0][2] = D2Phi;
        node.phi[0][3] = D3Phi;
        node.phi[0][4] = D4Phi;
      } else {
        node.phi[0][0] = c_0;
        node.phi[0][1] = c_0;
        node.phi[0][2] = c_0;
        node.phi[0][3] = c_0;
        node.phi[0][4] = c_0;
      }

      val->value_[i+par.granularity*(j+k*par.granularity)] = node;
    }}}

    return 1;
}

int rkupdate(std::vector< nodedata* > const& vecval, stencil_data* result, 
  bool boundary,
  int *bbox, int compute_index, 
  had_double_type const& dt, had_double_type const& dx, had_double_type const& timestep,
  int level, Par const& par)
{
  // allocate some temporary arrays for calculating the rhs
  nodedata rhs;
  std::vector<nodedata> work,work2;
  std::vector<nodedata* > pwork, pwork2;
  static had_double_type const c_0_75 = 0.75;
  static had_double_type const c_0_25 = 0.25;
  static had_double_type const c_2_3 = had_double_type(2.)/had_double_type(3.);
  static had_double_type const c_1_3 = had_double_type(1.)/had_double_type(3.);

  work.resize(vecval.size());
  work2.resize(vecval.size());

  int n = 3*par.granularity;
  int n2 = par.granularity;

  // -------------------------------------------------------------------------
  // iter 0
    std::size_t start,end;
    start = par.granularity;
    end = 2*par.granularity;

    for (int k=start; k<end;k++) {
    for (int j=start; j<end;j++) {
    for (int i=start; i<end;i++) {
      calcrhs(&rhs,vecval,0,dx,boundary,i,j,k,par); 
      for (int ll=0;ll<num_eqns;ll++) {
        work[i+n*(j+n*k)].phi[0][ll] = vecval[i+n*(j+n*k)]->phi[0][ll];
        work[i+n*(j+n*k)].phi[1][ll] = vecval[i+n*(j+n*k)]->phi[0][ll] + rhs.phi[0][ll]*dt; 
      }
    }}}

    std::vector<nodedata>::iterator n_iter;
    for (n_iter=work.begin();n_iter!=work.end();++n_iter) pwork.push_back( &(*n_iter) );

  // -------------------------------------------------------------------------
  // iter 1
    for (int k=start; k<end;k++) {
    for (int j=start; j<end;j++) {
    for (int i=start; i<end;i++) {
      calcrhs(&rhs,pwork,1,dx,boundary,i,j,k,par); 
      for (int ll=0;ll<num_eqns;ll++) {
        work2[i+n*(j+n*k)].phi[1][ll] = c_0_75*work[i+n*(j+n*k)].phi[0][ll] + 
                                        c_0_25*work[i+n*(j+n*k)].phi[1][ll] + 
                                        c_0_25*rhs.phi[0][ll]*dt;
      }
    }}}

    for (n_iter=work2.begin();n_iter!=work2.end();++n_iter) pwork2.push_back( &(*n_iter) );

  // -------------------------------------------------------------------------
  // iter 2
    int ii,jj,kk;
    for (int k=n2; k<2*n2;k++) {
    for (int j=n2; j<2*n2;j++) {
    for (int i=n2; i<2*n2;i++) {
      calcrhs(&rhs,pwork2,1,dx,boundary,i,j,k,par); 

      ii = i - n2;
      jj = j - n2;
      kk = k - n2;
      for (int ll=0;ll<num_eqns;ll++) {
        result->value_[ii + n2*(jj + n2*kk)].phi[0][ll] = 
                                   c_1_3*work[i+n*(j+n*k)].phi[0][ll]  
                                 + c_2_3*(work2[i+n*(j+n*k)].phi[1][ll] + rhs.phi[0][ll]*dt);
      }
    }}}

    result->timestep_ = timestep + 1.0/pow(2.0,level);

  return 1;
}

// This is a pointwise calculation: compute the rhs for point result given input values in array phi
void calcrhs(struct nodedata * rhs,
               std::vector< nodedata* > const& vecval,
                int flag, had_double_type const& dx,
                bool boundary,int i,int j,int k, Par const& par)
{

  int n = 3*par.granularity;

  if ( !boundary ) {
    rhs->phi[0][0] = vecval[i+n*(j+n*k)]->phi[flag][4]; 
    rhs->phi[0][1] = - 0.5*(vecval[i+1+n*(j+n*k)]->phi[flag][4] - vecval[i-1+n*(j+n*k)]->phi[flag][4])/dx;
    rhs->phi[0][2] = - 0.5*(vecval[i+n*(j+1+n*k)]->phi[flag][4] - vecval[i+n*(j+1+n*k)]->phi[flag][4])/dx;
    rhs->phi[0][3] = - 0.5*(vecval[i+n*(j+n*(k+1))]->phi[flag][4] - vecval[i+n*(j+n*(k-1))]->phi[flag][4])/dx;
    rhs->phi[0][4] = - 0.5*(vecval[i+1+n*(j+n*k)]->phi[flag][1] - vecval[i-1+n*(j+n*k)]->phi[flag][1])/dx
                - 0.5*(vecval[i+n*(j+1+n*k)]->phi[flag][2] - vecval[i+n*(j-1+n*k)]->phi[flag][2])/dx
                - 0.5*(vecval[i+n*(j+n*(k+1))]->phi[flag][3] - vecval[i+n*(j+n*(k-1))]->phi[flag][3])/dx;
  } else {
    // dirichlet
    rhs->phi[0][0] = 0.0;
    rhs->phi[0][1] = 0.0;
    rhs->phi[0][2] = 0.0;
    rhs->phi[0][3] = 0.0;
    rhs->phi[0][4] = 0.0;
  }
  return;
}
