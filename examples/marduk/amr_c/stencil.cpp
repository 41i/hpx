//  Copyright (c) 2007-2011 Hartmut Kaiser
//  Copyright (c)      2011 Matthew Anderson
//  Copyright (c)      2011 Bryce Lelbach
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx.hpp>
#include <hpx/lcos/future_wait.hpp>

#include <boost/foreach.hpp>

#include <math.h>

#include "stencil.hpp"
#include "logging.hpp"
#include "stencil_data.hpp"
#include "stencil_functions.hpp"
#include "stencil_data_locking.hpp"
#include "../mesh/unigrid_mesh.hpp"

#if defined(RNPL_FOUND)
#include <sdf.h>
#endif

bool intersection(double xmin,double xmax,
                  double ymin,double ymax,
                  double zmin,double zmax,
                  double xmin2,double xmax2,
                  double ymin2,double ymax2,
                  double zmin2,double zmax2);
double max(double,double);
double min(double,double);

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace components { namespace amr 
{
    ///////////////////////////////////////////////////////////////////////////
    stencil::stencil()
      : numsteps_(0)
    {
    }

    inline bool 
    stencil::floatcmp(double_type const& x1, double_type const& x2) {
      // compare two floating point numbers
      static double_type const epsilon = 1.e-8;
      if ( x1 + epsilon >= x2 && x1 - epsilon <= x2 ) {
        // the numbers are close enough for coordinate comparison
        return true;
      } else {
        return false;
      }
    }

    inline bool 
    stencil::floatcmp_le(double_type const& x1, double_type const& x2) {
      // compare two floating point numbers
      static double_type const epsilon = 1.e-8;

      if ( x1 < x2 ) return true;

      if ( x1 + epsilon >= x2 && x1 - epsilon <= x2 ) {
        // the numbers are close enough for coordinate comparison
        return true;
      } else {
        return false;
      }
    }

    inline bool 
    stencil::floatcmp_ge(double_type const& x1, double_type const& x2) {
      // compare two floating point numbers
      static double_type const epsilon = 1.e-8;

      if ( x1 > x2 ) return true;

      if ( x1 + epsilon >= x2 && x1 - epsilon <= x2 ) {
        // the numbers are close enough for coordinate comparison
        return true;
      } else {
        return false;
      }
    }

    ///////////////////////////////////////////////////////////////////////////
    // Implement actual functionality of this stencil
    // Compute the result value for the current time step
    int stencil::eval(naming::id_type const& result, 
        std::vector<naming::id_type> const& gids, std::size_t row, std::size_t column,
        parameter const& par)
    {
        // make sure all the gids are looking valid
        if (result == naming::invalid_id)
        {
            HPX_THROW_EXCEPTION(bad_parameter,
                "stencil::eval", "result gid is invalid");
            return -1;
        }


        // this should occur only after result has been delivered already
        BOOST_FOREACH(naming::id_type gid, gids)
        {
            if (gid == naming::invalid_id)
                return -1;
        }

        // get all input and result memory_block_data instances
        std::vector<access_memory_block<stencil_data> > val;
        access_memory_block<stencil_data> resultval = 
            get_memory_block_async(val, gids, result);

        // lock all user defined data elements, will be unlocked at function exit
        scoped_values_lock<lcos::mutex> l(resultval, val); 

        resultval.get() = val[0].get();
        resultval->level_ = val[0]->level_;

        // Check if this is a prolongation/restriction step
        if ( (row+5)%3 == 0 || ( par->allowedl == 0 && row == 0 ) ) {
          // This is a prolongation/restriction step
          resultval->timestep_ = val[0]->timestep_;
        } else {
          resultval->timestep_ = val[0]->timestep_ + 1.0/pow(2.0,int(val[0]->level_));
        }

#if defined(RNPL_FOUND)
        // Output
        if (par->loglevel > 1 && fmod(resultval->timestep_, par->output) < 1.e-6) {
          std::vector<double> x,y,z,phi,d1phi,d2phi,d3phi,d4phi;   
          applier::applier& appl = applier::get_applier();
          naming::id_type this_prefix = appl.get_runtime_support_gid();
          int locality = get_prefix_from_id( this_prefix );
          double datatime;

          int gi = par->item2gi[column];
          int nx = par->gr_nx[gi];
          int ny = par->gr_ny[gi];
          int nz = par->gr_nz[gi];
          for (int i=0;i<nx;i++) {
            x.push_back(par->gr_minx[gi] + par->gr_h[gi]*i);
          }
          for (int i=0;i<ny;i++) {
            x.push_back(par->gr_miny[gi] + par->gr_h[gi]*i);
          }
          for (int i=0;i<nz;i++) {
            x.push_back(par->gr_minz[gi] + par->gr_h[gi]*i);
          }

          //datatime = resultval->timestep_*par->gr_h[gi]*par->lambda;
          datatime = resultval->timestep_;
          for (int k=0;k<nz;k++) {
          for (int j=0;j<ny;j++) {
          for (int i=0;i<nx;i++) {
            phi.push_back(resultval->value_[i+nx*(j+ny*k)].phi[0][0]);
            d1phi.push_back(resultval->value_[i+nx*(j+ny*k)].phi[0][1]);
            d2phi.push_back(resultval->value_[i+nx*(j+ny*k)].phi[0][2]);
            d3phi.push_back(resultval->value_[i+nx*(j+ny*k)].phi[0][3]);
            d4phi.push_back(resultval->value_[i+nx*(j+ny*k)].phi[0][4]);
          } } }
          int shape[3];
          char cnames[80] = { "x|y|z" };
          char phi_name[80];
          sprintf(phi_name,"%dphi",locality);
          char phi1_name[80];
          sprintf(phi1_name,"%dd1phi",locality);
          char phi2_name[80];
          sprintf(phi2_name,"%dd2phi",locality);
          char phi3_name[80];
          sprintf(phi3_name,"%dd3phi",locality);
          char phi4_name[80];
          sprintf(phi4_name,"%dd4phi",locality);
          shape[0] = nx;
          shape[1] = ny;
          shape[2] = nz;
          gft_out_full(phi_name,datatime,shape,cnames,3,&*x.begin(),&*phi.begin());
          gft_out_full(phi1_name,datatime,shape,cnames,3,&*x.begin(),&*d1phi.begin());
          gft_out_full(phi2_name,datatime,shape,cnames,3,&*x.begin(),&*d2phi.begin());
          gft_out_full(phi3_name,datatime,shape,cnames,3,&*x.begin(),&*d3phi.begin());
          gft_out_full(phi4_name,datatime,shape,cnames,3,&*x.begin(),&*d4phi.begin());
        }
#endif

        //std::cout << " TEST row " << row << " column " << column << " timestep " << resultval->timestep_ << std::endl;
        if ( val[0]->timestep_ >= par->nt0-2 ) {
          return 0;
        }
        return 1;
    }

    hpx::actions::manage_object_action<stencil_data> const manage_stencil_data =
        hpx::actions::manage_object_action<stencil_data>();

    ///////////////////////////////////////////////////////////////////////////
    naming::id_type stencil::alloc_data(int item, int maxitems, int row,
                           std::vector<naming::id_type> const& interp_src_data,
                           double time,
                           parameter const& par)
    {
        naming::id_type here = applier::get_applier().get_runtime_support_gid();
        naming::id_type result = components::stubs::memory_block::create(
            here, sizeof(stencil_data), manage_stencil_data);

        if (-1 != item) {
            // provide initial data for the given data value 
            access_memory_block<stencil_data> val(
                components::stubs::memory_block::checkout(result));

            if ( time < 1.e-8 ) {
              // call provided (external) function
              generate_initial_data(val.get_ptr(), item, maxitems, row, *par.p);
            } else {
              // data is generated from interpolation using interp_src_data
              // find the bounding box for this item
              double minx = par->gr_minx[par->item2gi[item]];
              double miny = par->gr_miny[par->item2gi[item]];
              double minz = par->gr_minz[par->item2gi[item]];
              double maxx = par->gr_maxx[par->item2gi[item]];
              double maxy = par->gr_maxy[par->item2gi[item]];
              double maxz = par->gr_maxz[par->item2gi[item]];
              double h = par->gr_h[par->item2gi[item]];
              int nx0 = par->gr_nx[par->item2gi[item]];
              int ny0 = par->gr_ny[par->item2gi[item]];
              int nz0 = par->gr_nz[par->item2gi[item]];

              val->max_index_ = maxitems;
              val->index_ = item;
              val->timestep_ = 0;
              val->value_.resize(nx0*ny0*nz0);
              for (int step=0;step<par->prev_gi.size();step++) {
                // see if the new gi is the same as the old
                int gi = par->prev_gi[step];
                if ( floatcmp(minx,par->gr_minx[gi]) && 
                     floatcmp(miny,par->gr_miny[gi]) && 
                     floatcmp(minz,par->gr_minz[gi]) && 
                     floatcmp(maxx,par->gr_maxx[gi]) && 
                     floatcmp(maxy,par->gr_maxy[gi]) && 
                     floatcmp(maxz,par->gr_maxz[gi]) && 
                     floatcmp(h,par->gr_h[gi]) 
                   ) 
                {
                  // This has the same data -- copy it over
                  access_memory_block<stencil_data> prev_val(
                    components::stubs::memory_block::checkout(interp_src_data[par->prev_gi2item[gi]]));
                  val.get() = prev_val.get();

                  val->max_index_ = maxitems;
                  val->index_ = item;
                  val->timestep_ = 0;
                  break;
                } else if (
                  intersection(minx,maxx,
                               miny,maxy,
                               minz,maxz,
                               par->gr_minx[gi],par->gr_maxx[gi],
                               par->gr_miny[gi],par->gr_maxy[gi],
                               par->gr_minz[gi],par->gr_maxz[gi])
                  && floatcmp(h,par->gr_h[gi]) 
                          ) 
                {
                  access_memory_block<stencil_data> prev_val(
                    components::stubs::memory_block::checkout(interp_src_data[par->prev_gi2item[gi]]));
                  // find the intersection index
                  double x1 = max(minx,par->gr_minx[gi]); 
                  double x2 = min(maxx,par->gr_maxx[gi]); 
                  double y1 = max(miny,par->gr_miny[gi]); 
                  double y2 = min(maxy,par->gr_maxy[gi]); 
                  double z1 = max(minz,par->gr_minz[gi]); 
                  double z2 = min(maxz,par->gr_maxz[gi]);

                  int isize = (int) ( (x2-x1)/h );
                  int jsize = (int) ( (y2-y1)/h );
                  int ksize = (int) ( (z2-z1)/h );

                  int lnx = par->gr_nx[gi]; 
                  int lny = par->gr_ny[gi]; 
                  int lnz = par->gr_nz[gi]; 

                  int istart_dst = (int) ( (x1 - minx)/h );
                  int jstart_dst = (int) ( (y1 - miny)/h );
                  int kstart_dst = (int) ( (z1 - minz)/h );

                  int istart_src = (int) ( (x1 - par->gr_minx[gi])/h );
                  int jstart_src = (int) ( (y1 - par->gr_miny[gi])/h );
                  int kstart_src = (int) ( (z1 - par->gr_minz[gi])/h );

                  val->level_ = prev_val->level_;
                  for (int kk=0;kk<=ksize;kk++) {
                  for (int jj=0;jj<=jsize;jj++) {
                  for (int ii=0;ii<=isize;ii++) {
                    int i = ii + istart_dst;
                    int j = jj + jstart_dst;
                    int k = kk + kstart_dst;

                    int si = ii + istart_src;
                    int sj = jj + jstart_src;
                    int sk = kk + kstart_src;
                    BOOST_ASSERT(i+nx0*(j+ny0*k) < val->value_.size());
                    BOOST_ASSERT(si+lnx*(sj+lny*sk) < prev_val->value_.size());
                    val->value_[i+nx0*(j+ny0*k)] = prev_val->value_[si+lnx*(sj+lny*sk)];
                  } } }

                }
              }
              //std::cout << " TEST bbox in gen init data " << xmin << " " << xmax << std::endl;
              //std::cout << "                            " << ymin << " " << ymax << std::endl;
              //std::cout << "                            " << zmin << " " << zmax << std::endl;

              // TEST
              generate_initial_data(val.get_ptr(), item, maxitems, row, *par.p);
               
            }

            if (log_ && par->loglevel > 1)         // send initial value to logging instance
                stubs::logging::logentry(log_, val.get(), row,0, par);
        }
        return result;
    }

    void stencil::init(std::size_t numsteps, naming::id_type const& logging)
    {
        numsteps_ = numsteps;
        log_ = logging;
    }

}}}

