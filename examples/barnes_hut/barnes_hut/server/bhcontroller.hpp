///////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2011 Daniel Kogler
//
//  Distributed under the Boost Software License, Version 1.0.(See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
///////////////////////////////////////////////////////////////////////////////
#ifndef _BHCONTROLLER_SERVER_HPP
#define _BHCONTROLLER_SERVER_HPP

/*This is the bhcontroller class implementation header file.
*/

#include <time.h>

#include <hpx/hpx.hpp>
#include <hpx/hpx_fwd.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/lcos/mutex.hpp>
#include <hpx/lcos/eager_future.hpp>
#include <hpx/runtime/applier/applier.hpp>
#include <hpx/runtime/actions/component_action.hpp>
#include <hpx/runtime/components/component_type.hpp>
#include <hpx/runtime/components/server/simple_component_base.hpp>

#include <boost/foreach.hpp>
#include <boost/random/linear_congruential.hpp>
#include "../bhnode.hpp"

#include <boost/date_time/posix_time/posix_time.hpp>

using namespace boost::posix_time;
using hpx::applier::get_applier;

namespace hpx { namespace components { namespace server
{
    class HPX_COMPONENT_EXPORT bhcontroller : 
          public simple_component_base<bhcontroller>
    {
    public:
    //enumerate all of the actions that will(or can) be employed
    enum actions{
        hpl_construct,
        hpl_run
    };

    //constructors and destructor
    bhcontroller(){}
    int construct(id_type gid, std::string input);
    int run_simulation();
    ~bhcontroller(){destruct();}
    void destruct();

    private:
    void build_init();
    void create_region_nodes(region_path path);

    //data members
    id_type _gid;
    int numBodies;
    int numIters;
    double dtime, eps, tolerance;
    double halfdt, softening, invTolerance;
    double** particlePositions;
    double*  particleMasses;
    components::bhnode treeRoot;
    components::bhnode* particle;
    vector<components::bhnode> regions;

    public:
    //here we define the actions that will be used
    typedef actions::result_action2<bhcontroller, int, hpl_construct,
        id_type, std::string, &bhcontroller::construct> constructAction;
    typedef actions::result_action0<bhcontroller, int, hpl_run,
        &bhcontroller::run_simulation> runAction;

    //here begins the definitions of most of the future types that will be used

    };
///////////////////////////////////////////////////////////////////////////////

    //the constructor initializes the matrix
    int bhcontroller::construct(naming::id_type gid, std::string inputFile){
    std::vector<bhnode::constFuture> futures;
    std::ifstream infile;
    infile.open(inputFile.c_str());
    if(!infile){
        std::cerr<<"Can't open input file "<<inputFile<<std::endl;
        return 1;
    }
    try{
        double dat[7];
        double bounds[6] = {-1.7e308,-1.7e308,-1.7e308,1.7e308,1.7e308,1.7e308};
        infile>>numBodies>>numIters>>dtime>>eps>>tolerance;
        halfdt = .5 * dtime;
        softening = eps * eps;
        invTolerance = 1 / (tolerance * tolerance);
        treeRoot.create(get_applier().get_runtime_support_gid());
        particle = new components::bhnode[numBodies];
        particlePositions = new double*[numBodies];
        particleMasses = new double[numBodies];
        for(int i = 0; i < numBodies; i++){
            particlePositions[i] = new double[3];
            particle[i].create(get_applier().get_runtime_support_gid());
            infile>>dat[0]>>dat[1]>>dat[2]>>dat[3]>>dat[4]>>dat[5]>>dat[6];
            futures.push_back(particle[i].construct_node(dat));
            for(int j = 0; j < 3; j++){
                if(bounds[j] < dat[j+1]){bounds[j] = dat[j+1];}
                if(bounds[j+3] > dat[j+1]){bounds[j+3] = dat[j+1];}
                particlePositions[i][j] = dat[j+1];
            }
            particleMasses[i] = dat[0];
        }
        id_type treeGid = treeRoot.get_gid();
        treeRoot.set_boundaries(treeGid, bounds);
        regions.push_back(treeGid);
        for(int i = 0; i < numBodies; i++){futures[i].get();}
    }
    catch(...){
        std::cerr<<"An error occurred while reading the file "<<inputFile<<"\n";
        infile.close();
        return 1;
    }
    infile.close();
    _gid = gid;
    return 0;
    }

    //the destructor frees the memory
    void bhcontroller::destruct(){
        treeRoot.free();
    }

    int bhcontroller::run_simulation(){
        build_init();
        return 0;
    }

    void bhcontroller::build_init(){
        for(int i = 0; i < numBodies; i++){
            region_path path;
            vector<double> pos(particlePositions[i],
                               particlePositions[i]+3);
            path = treeRoot.insert_node(pos, particleMasses[i],
                                        particle[i].get_gid());
            if(path.isPath){create_region_nodes(path);}
        }
    }

    //this creates all region nodes needed after inserting a new particle node
    //onto the octtree
    void bhcontroller::create_region_nodes(region_path path){
        components::bhnode firstNode;
        vector<bhnode::cnstFuture2> newNodeFutures;
        id_type nextInsertPoint = path.insertPoint;
        firstNode.create(get_applier().get_runtime_support_gid());
        bhnode::childFuture chFuture(path.insertPoint,
            path.octants[0], firstNode.get_gid());
        regions.push_back(firstNode);

        for(int i = 0; i < (int)path.subboundaries.size()-1; i++){
            components::bhnode nextNode;
            vector<double> tempPosition;
            for(int j = 0; j < 3; j++){
                tempPosition.push_back((path.subboundaries[i+1][j] +
                                        path.subboundaries[i+1][j+3])*.5);
            }
            nextNode.create(get_applier().get_runtime_support_gid());
            newNodeFutures.push_back(firstNode.construct_node(nextInsertPoint,
                path.subboundaries[i], nextNode.get_gid(), path.childData,
                path.octants[i], tempPosition));
            regions.push_back(nextNode);
            nextInsertPoint = firstNode.get_gid();
            firstNode = nextNode;
        }
        
        newNodeFutures.push_back(firstNode.construct_node(nextInsertPoint,
            path.subboundaries[path.subboundaries.size()-1], path.child,
            path.childData, path.octants));

        chFuture.get();
        for(int i = 0; i < (int)newNodeFutures.size(); i++){
            newNodeFutures[i].get();
        }
    }
}}}

#endif
