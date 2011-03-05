#include <hpx/hpx_fwd.hpp>
#include <hpx/config.hpp>
#include <hpx/runtime/applier/applier.hpp>
#include <hpx/runtime/threads/thread.hpp>
#include <hpx/runtime/components/component_type.hpp>
#include <hpx/runtime/components/server/managed_component_base.hpp>
#include <hpx/runtime/actions/component_action.hpp>

#include "node.hpp"

namespace hpx { namespace components { namespace node { namespace server {
    
        node::node()
        {
            node_type = 1;
            mass = 0.0;
            p[0] = 0.0;
            p[1] = 0.0;
            p[2] = 0.0;
            v[0] = 0.0;
            v[1] = 0.0;
            v[2] = 0.0;
            a[0] = 0.0;
            a[1] = 0.0;
            a[2] = 0.0;
        }
        
        void node::set_mass(double mass_tmp)
        {
            mass = mass_tmp;
        }
        
        void node::set_pos(double px, double py, double pz)
        {
            p[0] = px;
            p[1] = py;
            p[2] = pz;
        }
        
        double node::get_mass()
        {
            return mass;
        }
        
        std::vector<double> node::get_pos()
        {
            std::vector<double> bPos;
            bPos.push_back(p[0]);
            bPos.push_back(p[1]);
            bPos.push_back(p[2]);    
            return bPos;
        }
        void node::set_type(int type_var)
        {
            node_type = type_var;
        }

        int node::get_type()
        {
            return node_type;
        }
        
        void node::set_vel(double vx, double vy, double vz)
        {
            v[0] = vx;
            v[1] = vy;
            v[2] = vz;
        }        
        
        void node::set_acc(double ax, double ay, double az)
        {
            a[0] = ax;
            a[1] = ay;
            a[2] = az;
        }        
                
        void node::print()
        {
            applier::applier& appl = applier::get_applier();
            std::cout << appl.get_runtime_support_gid() 
            << " pos > " << p[0] << " " << p[1] << " " 
            << p[2] << " " << std::flush << std::endl;   
            std::cout << appl.get_runtime_support_gid() 
            << " vel > " << v[0] << " " << v[1] << " " 
            << v[2] << " " << std::flush << std::endl;     
        }
        
}}}}