//  Copyright (c) 2007-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <cstring>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <cstdio>
#include <cmath>
#include <sys/time.h>



#include <hpx/hpx.hpp>


#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <boost/thread.hpp>

//#include "nbody/stencil_value.hpp"
#include "init_mpfr.hpp"
#include "nbody/dynamic_stencil_value.hpp"
#include "nbody/functional_component.hpp"
#include "nbody/unigrid_mesh.hpp"
#include "nbody_c/stencil.hpp"
#include "nbody_c/logging.hpp"

#include "nbody_c_test/rand.hpp"

namespace po = boost::program_options;

typedef struct {
    double dtime;
    double eps;
    double tolerance;
    double half_dt;
    double softening_2;
    double inv_tolerance_2;
    int iter;
    int num_bodies;
    int num_iterations;
} crit_vals;

typedef struct {
    double mass;
    double px, py, pz;
    double vx, vy, vz;
    double ax, ay, az;
} body;

using namespace hpx;

///////////////////////////////////////////////////////////////////////////////
// initialize mpreal default precision
namespace hpx { namespace components { namespace nbody 
{
 //   init_mpfr init_(true);
}}}


/*
 * The N-Body tree is composed of two different kinds of particles
 * CELL - intermediate nodes
 * PAR - Terminal nodes which contain the body/particle.
 */
enum 
{
    CELL, PAR
};


///////// Main Class Definitions
class TreeLeaf;
static TreeLeaf **particles; // the n particles

class TreeNode 
{
    public:
        int tag;
        int node_type; /* Each node has a node type which is either NDE or PAR*/
        double mass;   /* Each node has a mass */
        double p[3];   /* position of each node is stored in a vector of type double named p */
};

class IntrTreeNode: public TreeNode /* Internal node inherits from base TreeNode */
{ 
    private:
        IntrTreeNode *edge_link; /* Each Internal node has a pointer to link itself with rest of the tree */
        static IntrTreeNode *tree_head, *free_node_list; /* Each also has two static links to the current root and the free list */
    public:
        static IntrTreeNode *newNode(const double pos_buf[]); /* Function to create a new node, returns an Internal Tree Node */
        void treeNodeInsert(TreeLeaf * const new_particle, const double sub_box_dim); /* Function to insert a particle (tree leaf) node  */
        void calculateCM(int &current_index); /* Recursive function to compute center of mass */
        void tagTree(int &current_node);
        void printTag(int &current_node);


        static void treeReuse()      /* function to recycle tree */
        {
            free_node_list = tree_head; /* point the free List to the root of the tree */
        }
        TreeNode *branch[8];  /* Each Internal node has pointers to 8 branch nodes (since it is an octtree) of type TreeNode*/
};

class TreeLeaf: public TreeNode   /* Terminal / leaf nodes extend intermediate nodes */
{                                 /* in addition to the mass, px, py, pz the internal */
    public:                       /* nodes also have velocity vector of type double named v and */
        double v[3];              /* acceleration vector of type double named a  */
        double a[3];
        TreeLeaf()                /* TreeLeaf Constructor */
        {
            node_type = PAR;     /* Initialize TreeLeaf node to PARTICLE type */
            mass = 0.0;          /* Initialize mass to zero */
            p[0] = 0.0; /* Initialize position vector elements to 0 */
            p[1] = 0.0; /* Initialize position vector elements to 0 */
            p[2] = 0.0; /* Initialize position vector elements to 0 */
            v[0] = 0.0; /* Initialize velocity vector to 0 */
            v[1] = 0.0; /* Initialize velocity vector to 0 */
            v[2] = 0.0; /* Initialize velocity vector to 0 */
            a[0] = 0.0; /* Initialize acceleration vector to 0 */
            a[1] = 0.0; /* Initialize acceleration vector to 0 */
            a[2] = 0.0; /* Initialize acceleration vector to 0 */
        }
        void moveParticles(); /* moves particles according to acceleration and velocity calculated */
        void calculateForce(const IntrTreeNode * const root, const double box_size); /* calculates the acceleration and velocity of each particle in the tree */
        ~TreeLeaf() { }       /* TreeLeaf Destructor */
    private:
        void forceCalcRe(const TreeNode * const n, double box_size_2); /* traverses tree recursively while calculating force for each particle */
};


IntrTreeNode *IntrTreeNode::tree_head = NULL;  /* Initialize the head pointer to NULL */
IntrTreeNode *IntrTreeNode::free_node_list = NULL; /* Initialize the free list pointer to NULL */

IntrTreeNode *IntrTreeNode::newNode(const double pos_buf[]) 
{
    register IntrTreeNode *temp_node; /* Declare the temporary node of type TreeNodeI */

    if (free_node_list == NULL)           /* if the list of free nodes is Null then create */
    {                                     /* a new node and link it to the previous node of the tree */
        temp_node = new IntrTreeNode();   /* create a new node of type Tree Node internal and store it in temp_node */
        temp_node->edge_link = tree_head; /* initialize the link to point to the previous node of the tree */
        tree_head = temp_node;            /* set new subroot of the tree to be temp_node */
    } 
    else 
    { 
        temp_node = free_node_list;       /* get a node from the free list of nodes ... re use previously allocated */
        free_node_list = free_node_list->edge_link; /* memory space. Helps in reuse of space */
    }

    temp_node->node_type = CELL;  /* Set the node type to CELL (since an intermediate node is being created */
    temp_node->mass = 0.0;        /* intermediate node have no mass. Center of Mass is populated here later*/
    temp_node->p[0] = pos_buf[0]; /* Set the position of the cell to the position passed in*/
    temp_node->p[1] = pos_buf[1];
    temp_node->p[2] = pos_buf[2];
    for (int i = 0; i < 8; ++i)        /* Intermediate nodes in a BHT can have upto 8 branch.  */
        temp_node->branch[i] = NULL;   /* Initialized to NULL */

    return temp_node;
}

/* Insert New Particle: This function inserts a new particle from the specified level 
 * values passed into the function are:
 *     the new particle to be inserted into the tree named new_particle (of type TreeLeaf)
 *     the value representing dimension of the sub_box  helps determine the 
 *     position of intermediate node
 */
void IntrTreeNode::treeNodeInsert(TreeLeaf * const new_particle, const double sub_box_dim) // builds the tree
{
    register int i = 0;                        /* Initialize stores the index  */ 
    register double temp[3] ;                  /* Initialize buffer position to 0.0 */
    temp[0] = 0.0;                    /* bufPos stores the value which helps derive*/
    temp[1] = 0.0;                    /* bufPos stores the value which helps derive*/
    temp[2] = 0.0;                    /* bufPos stores the value which helps derive
                                                * the coordinates of a new intermediate node
                                                * if one has to be created.
                                                */
    /*
     * If the curr.x > new.x & curr.y > new.y & curr.z > new.z then store at i=0 buf.x=0, buf.y=0, buf.z=0
     * If the curr.x < new.x & curr.y > new.y & curr.z > new.z then store at i=1 buf.x=sub_box, buf.y=0, buf.z=0
     * If the curr.x > new.x & curr.y < new.y & curr.z > new.z then store at i=2 buf.x=0, buf.y=sub_box, buf.z=0
     * If the curr.x < new.x & curr.y < new.y & curr.z > new.z then store at i=3 buf.x=sub_box, buf.y=sub_box, buf.z=0
     * If the curr.x > new.x & curr.y > new.y & curr.z < new.z then store at i=4 buf.x=0, buf.y=0, buf.z=sub_box
     * If the curr.x < new.x & curr.y > new.y & curr.z < new.z then store at i=5 buf.x=sub_box, buf.y=0, buf.z=sub_box
     * If the curr.x > new.x & curr.y < new.y & curr.z < new.z then store at i=6 buf.x=0, buf.y=sub_box, buf.z=sub_box
     * If the curr.x < new.x & curr.y < new.y & curr.z < new.z then store at i=7 buf.x=sub_box, buf.y=sub_box, buf.z=sub_box
     */ 
    if (p[0] < new_particle->p[0]) 
    {
        i = 1;
        temp[0] = sub_box_dim;
    }
    if (p[1] < new_particle->p[1]) 
    {
        i += 2;
        temp[1] = sub_box_dim;
    }
    if (p[2] < new_particle->p[2]) 
    {
        i += 4;
        temp[2] = sub_box_dim;
    }

    /* if the branch node at i as determined above is empty then
     * store current body at that position */
    if (branch[i] == NULL) 
    {
        branch[i] = new_particle;
    } 
    /* if the branch node at the current i is a CELL then
     * traverse to the branch nodes to see where the body 
     * can be inserted */ 
    else if (branch[i]->node_type == CELL) 
    {
        ((IntrTreeNode *) (branch[i]))->treeNodeInsert(new_particle, 0.5 * sub_box_dim);
    } 

    /* if all branch node at the current position is a LEAF 
     * then create a node at the current position 
     * insert the current branch node into the subtree
     * insert the new node into the subtree */
    else 
    {
        /* since the cell that we are inserting in, is already a terminal node
         * we split the cell and create an intermediate node. In order to do this
         * we need to allocate an intermediate node */
        /* Since we are creating a new intermediate node we divide sub_box by 2 */
        register const double new_sub_box_dim = 0.5 * sub_box_dim;
        /* The coordinates of the intermediate which will point to the two branch nodes 
         * can be calculated 
         * the X coordinate of the intermediate node is p[0] - sub_box2 + bufPos[0]
         * the Y coordinate of the intermediate node is p[1] - sub_box2 + bufPos[1]
         * the Z coordinate of the intermediate node is p[2] - sub_box2 + bufPos[2]
         **/
        register const double pos_buf[] = { p[0] - new_sub_box_dim + temp[0], p[1] - new_sub_box_dim + temp[1], p[2] - new_sub_box_dim + temp[2] };
        register IntrTreeNode * const cell = newNode(pos_buf);
        /* Insert the new node at the new intermediate node */
        cell->treeNodeInsert(new_particle, new_sub_box_dim);
        /* Insert the current branch node at the new intermediate node */
        cell->treeNodeInsert((TreeLeaf *) (branch[i]), new_sub_box_dim);
        /* Set the current branch[i] to the new intermediate node that we just created */
        branch[i] = cell;
    }
}

/* This function computes the center of mass
 * of the  tree upto a specified node. 
 * If the root is passed in as an argument
 * this function calculates the Center of Mass 
 * across the entire tree recursively. 
 */

void IntrTreeNode::tagTree(int &current_node)
{
   static int tag_id;
    register TreeNode *temp_branch;
    for (int i = 0; i < 8; ++i) 
    {
        temp_branch = branch[i];
        if (temp_branch != NULL) 
        {
            ++tag_id;
            if(temp_branch->node_type == CELL)
            { // Intermediate Node
                ((IntrTreeNode *) temp_branch)->tagTree(current_node);
            }
            temp_branch->tag = tag_id;

            std::cout << "particleid : " << tag_id << std::endl;
        }
    }
    ++tag_id;
    tag = tag_id;
    std::cout << "particleid : " << tag_id << std::endl;
}

void IntrTreeNode::printTag(int &current_node)
{
    register TreeNode *temp_branch;
    for (int i = 0; i < 8; ++i) 
    {
        temp_branch = branch[i];
        if (temp_branch != NULL) 
        {
            if(temp_branch->node_type == CELL)
            { // Intermediate Node
                ((IntrTreeNode *) temp_branch)->tagTree(current_node);
            }
            std::cout << "Tag : " << temp_branch->tag << std::endl;
        }
    }
    std::cout << "tag : " << tag << std::endl;
}

void IntrTreeNode::calculateCM(int &current_node) // recursively summarizes info about subtrees
{
    /* initialize local buffer mass and position variables to 0 */
    register double m, px = 0.0, py = 0.0, pz = 0.0;
    /* create a buffer branch variable of the type tree node */
    register TreeNode *temp_branch;

    /* initialize var variable to 0 ... helps in cycling through 
     * the array index
     */ 
    
    register int var = 0;
    /* this variable will store the total mass of the branch bodies */
    mass = 0.0;
    /* Cycle through each of the 8 children of the current node
     */
    for (int i = 0; i < 8; ++i) 
    {
        /* copy the current branch into the buffer */
        /* check to see if the current branch is NULL (empty) if so
         * then skip processing and move on to the next branch */
        temp_branch = branch[i];

        if (temp_branch != NULL) 
        {
            /* move the non null children to the front */
            branch[i] = NULL; // move non-NULL children to the front (needed to make other code faster)
            branch[var++] = temp_branch;
            /* if the current node is a leaf node ie contains a particle
             * then copy it into a new position in the particles[]
             * else it is an intermediate node, cycle through the 
             * branch nodes of the current intermediate node by recursively
             * calling the Compute Center of Mass function at the current 
             * intermediate node
             */
            if (temp_branch->node_type == PAR) 
            { // Leaf Node
                particles[current_node++] = (TreeLeaf *) temp_branch; // sort particles in tree order (approximation of putting nearby nodes together for locality)
            } 
            else 
            { // Intermediate Node
                ((IntrTreeNode *) temp_branch)->calculateCM(current_node);
            }
            /* copy mass of the branch node into the temporary variable m */
            m = temp_branch->mass;
            /* Accumulate the mass of the current node into the variable
             * holding the total mass named "mass" */
            mass += m;
            /* Accumulate the position*mass of each branch coordinate 
             * and store the values in the temporary pos vector. 
             */
            px += temp_branch->p[0] * m;
            py += temp_branch->p[1] * m;
            pz += temp_branch->p[2] * m;
            
            std::cout << "Center of Mass Calc : " << mass << px << py << pz << std::endl;
        }
    }
    /* Calculate center of mass for each intermediate node & root node
     * using the formula
     *                        ----- 
     *                 1      \  
     * CM_vector =  --------     m_i * pos_i
     *              tot_mass  /
     *                        -----
     */
    m = 1.0 / mass;
    p[0] = px * m;
    p[1] = py * m;
    p[2] = pz * m;
    std::cout << "Center of Mass Calc : " << m << p[0] << p[0] << p[0] << std::endl;
}

static inline void computeRootPos(const int num_bodies, double &box_dim, double center_position[]) 
{
 /* Initialize the min and max position vectors to extreme values */
    double minPos[3];
    minPos[0] = 1.0E90;
    minPos[1] = 1.0E90;
    minPos[2] = 1.0E90;
    double maxPos[3];
    maxPos[0] = -1.0E90;
    maxPos[1] = -1.0E90;
    maxPos[2] = -1.0E90;
    
    /* loop through the all the bodies
     * compare each x, y, z and store
     * the max and min values for each coordinate
     * in the maxPos and minPos vector
     */
    for (int i = 0; i < num_bodies; ++i)
    {
//        std::cout << " x "<< particles[i]->p[0] <<" y " << particles[i]->p[1] << " z " << particles[i]->p[2] << std::endl;  
        if (minPos[0] > particles[i]->p[0])
            minPos[0] = particles[i]->p[0];
        if (minPos[1] > particles[i]->p[1])
            minPos[1] = particles[i]->p[1];
        if (minPos[2] > particles[i]->p[2])
            minPos[2] = particles[i]->p[2];
        if (maxPos[0] < particles[i]->p[0])
            maxPos[0] = particles[i]->p[0];
        if (maxPos[1] < particles[i]->p[1])
            maxPos[1] = particles[i]->p[1];
        if (maxPos[2] < particles[i]->p[2])
            maxPos[2] = particles[i]->p[2];
    }
    /* Maximum of the difference between 
     * max.X and min.X, max.Y and min.Y
     * and max.Z and min.Z 
     * is stored in the box_dim
     * The pass by reference box_dim variable helps determine
     * the bounding box of the particles 
     */
    box_dim = maxPos[0] - minPos[0];
    if (box_dim < (maxPos[1] - minPos[1]))
        box_dim = maxPos[1] - minPos[1];
    if (box_dim < (maxPos[2] - minPos[2]))
        box_dim = maxPos[2] - minPos[2];
       
    /* Calculate the center position as the average of
     * maxPos and minPos for each of the x, y and z 
     * coordinates and store it in a vector 
     */
    center_position[0] = (maxPos[0] + minPos[0]) / 2;
    center_position[1] = (maxPos[1] + minPos[1]) / 2;
    center_position[2] = (maxPos[2] + minPos[2]) / 2;
}



///////////////////////////////////////////////////////////////////////////////
int hpx_main(std::size_t numvals, std::size_t numsteps,bool do_logging,
             components::nbody::Parameter const& par)
{
    std::string input_file;
    crit_vals cv;

    // get component types needed below
    components::component_type function_type = 
        components::get_component_type<components::nbody::stencil>();
    components::component_type logging_type = 
        components::get_component_type<components::nbody::server::logging>();

    {
        naming::id_type here = applier::get_applier().get_runtime_support_gid();

        if ( par->loglevel > 0 ) {
          // over-ride a false command line argument
          do_logging = true;
        }

        hpx::util::high_resolution_timer t;

        components::nbody::unigrid_mesh unigrid_mesh;
        unigrid_mesh.create(here);
        
        input_file = par->input_file;
        if(input_file.size() == 0)
        {
            //hpx::finalize();
            return 0;
        }
            
        std::ifstream infile;
        infile.open(input_file.c_str());
        if (!infile)                                /* if there is a problem opening file */
        {                                           /* exit gracefully */
            std::cerr << "Can't open input file " << input_file << std::endl;
            exit (1);
        }
        infile >> cv.num_bodies;                   
        infile >> cv.num_iterations;               
        infile >> cv.dtime;                        
        infile >> cv.eps;                          
        infile >> cv.tolerance;                  
        cv.half_dt = 0.5 * cv.dtime;
        cv.softening_2 = cv.eps * cv.eps;
        cv.inv_tolerance_2 = 1.0 / (cv.tolerance * cv.tolerance);    
        
        std::cout << "Num Bodies " << cv.num_bodies << std::endl;
//         hpx::memory::default_vector<body>::type bodies;
//         hpx::memory::default_vector<body>::type::iterator bod_iter;
//         bodies.resize(cv.num_bodies);
        if (particles == NULL) 
        {
            /* create a n array container for particles each of type treeleaf */
            particles = new TreeLeaf*[cv.num_bodies];
            /* cycle through the n array container and allocate memory space for each
            * particle */
            for (int i = 0; i < cv.num_bodies; ++i)
                particles[i] = new TreeLeaf();
        }
        
        for (int i =0; i < cv.num_bodies ; ++i)
        {
            double dat[7] = {0,0,0,0,0,0,0};
            infile >> dat[0] >> dat[1] >> dat[2] >> dat[3] >> dat[4] >> dat[5] >> dat[6];
            particles[i]->mass = dat[0];
            particles[i]->p[0] = dat[1];
            particles[i]->p[1] = dat[2];
            particles[i]->p[2] = dat[3];
            particles[i]->v[0] = dat[4];
            particles[i]->v[1] = dat[5];
            particles[i]->v[2] = dat[6];
        }
        
        for (int i =0; i < cv.num_bodies ; ++i)
        {
            std::cout << "body : "<< i << " : " << particles[i]->mass << " : " <<
            particles[i]->p[0] << " : " << particles[i]->p[1] << " : " << particles[i]->p[2] << std::endl;
            std::cout <<"           " << " : " << particles[i]->v[0] << " : " << particles[i]->v[1] 
            << " : " << particles[i]->v[2] << std::endl;
        }

        infile.close();  
        for (cv.iter = 0; cv.iter < cv.num_iterations; ++cv.iter)
        {
            double box_size, cPos[3];
            computeRootPos(cv.num_bodies, box_size, cPos);
            
            IntrTreeNode *bht_root = IntrTreeNode::newNode(cPos);
            const double sub_box_size = box_size * 0.5;
            for (int i = 0; i < cv.num_bodies; ++i) 
            {
                bht_root->treeNodeInsert(particles[i], sub_box_size); // grow the tree by inserting each body
            }
            
            register int current_index = 0;
            bht_root->calculateCM(current_index);
            bht_root->tagTree(current_index);
            bht_root->printTag(current_index);

            std::cout << "Center Position : " << cPos[0] << " " << cPos[1] << " " << cPos[2] << std::endl;
            std::cout << bht_root->tag << std::endl;
            
        }

        // for loop for iteration

        // recompute tree/mass
        
        std::vector<naming::id_type> result_data =
            unigrid_mesh.init_execute(function_type, numvals, numsteps,
                do_logging ? logging_type : components::component_invalid,par);

        // end for loop

        std::cout << "Elapsed time: " << t.elapsed() << " [s]" << std::endl;

        for (std::size_t i = 0; i < result_data.size(); ++i)
            components::stubs::memory_block::free(result_data[i]);
    }   // nbody_mesh needs to go out of scope before shutdown

    // initiate shutdown of the runtime systems on all localities
    components::stubs::runtime_support::shutdown_all();

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
bool parse_commandline(int argc, char *argv[], po::variables_map& vm)
{
    try {
        po::options_description desc_cmdline ("Usage:nbody_client [options]");
        desc_cmdline.add_options()
            ("input_file,i", po::value<std::string>(), 
            "Input File")
            ("help,h", "print out program usage (this message)")
            ("run_agas_server,r", "run AGAS server as part of this runtime instance")
            ("worker,w", "run this instance in worker (non-console) mode")
            ("agas,a", po::value<std::string>(), 
                "the IP address the AGAS server is running on (default taken "
                "from hpx.ini), expected format: 192.168.1.1:7912")
            ("hpx,x", po::value<std::string>(), 
                "the IP address the HPX parcelport is listening on (default "
                "is localhost:7910), expected format: 192.168.1.1:7913")
            ("threads,t", po::value<int>(), 
                "the number of operating system threads to spawn for this "
                "HPX locality")
            ("parfile,p", po::value<std::string>(), 
                "the parameter file")
            ("verbose,v", "print calculated values after each time step")
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
        std::cerr << "nbody_client: exception caught: " << e.what() << std::endl;
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
        std::cerr << "nbody_client: illegal port number given: " << v.substr(p+1) << std::endl;
        std::cerr << "                  using default value instead: " << port << std::endl;
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
    ~agas_server_helper()
    {
        agas_.stop();
    }

private:
    hpx::util::io_service_pool agas_pool_; 
    hpx::naming::resolver_server agas_;
};

///////////////////////////////////////////////////////////////////////////////
// this is the runtime type we use in this application
typedef hpx::runtime_impl<hpx::threads::policies::global_queue_scheduler> global_runtime_type;
typedef hpx::runtime_impl<hpx::threads::policies::local_queue_scheduler> local_runtime_type;

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
        hpx::runtime::mode mode = hpx::runtime::console;    // default is console mode
        bool do_logging = false;

        // extract IP address/port arguments
        if (vm.count("agas")) 
            split_ip_address(vm["agas"].as<std::string>(), agas_host, agas_port);

        if (vm.count("hpx")) 
            split_ip_address(vm["hpx"].as<std::string>(), hpx_host, hpx_port);

        if (vm.count("threads"))
            num_threads = vm["threads"].as<int>();

        if (vm.count("worker"))
            mode = hpx::runtime::worker;

        if (vm.count("verbose"))
            do_logging = true;

        // initialize and run the AGAS service, if appropriate
        std::auto_ptr<agas_server_helper> agas_server;
        if (vm.count("run_agas_server"))  // run the AGAS server instance here
            agas_server.reset(new agas_server_helper(agas_host, agas_port));

        std::size_t numvals;

        components::nbody::Parameter par;

        // default pars
        par->loglevel    = 2;
        par->output      = 1.0;
        par->output_stdout = 1;
        par->rowsize =  4;
        par->input_file="5_file";

        int scheduler = 1;  // 0: global scheduler
                            // 1: parallel scheduler
        std::string parfile;
        if (vm.count("parfile")) {
            parfile = vm["parfile"].as<std::string>();
            hpx::util::section pars(parfile);

            if ( pars.has_section("nbody") ) {
              hpx::util::section *sec = pars.get_section("nbody");
              if ( sec->has_entry("loglevel") ) {
                std::string tmp = sec->get_entry("loglevel");
                par->loglevel = atoi(tmp.c_str());
              }
              if ( sec->has_entry("output") ) {
                std::string tmp = sec->get_entry("output");
                par->output = atof(tmp.c_str());
              }
              if ( sec->has_entry("output_stdout") ) {
                std::string tmp = sec->get_entry("output_stdout");
                par->output_stdout = atoi(tmp.c_str());
              }
              if ( sec->has_entry("thread_scheduler") ) {
                std::string tmp = sec->get_entry("thread_scheduler");
                scheduler = atoi(tmp.c_str());
                BOOST_ASSERT( scheduler == 0 || scheduler == 1 );
              }
              if ( sec->has_entry("input_file") ) {
                std::string tmp = sec->get_entry("input_file");
                par->input_file = tmp;
              }
            }
        }

        
        // figure out the number of points for row 0
        numvals = par->rowsize;

        // initialize and start the HPX runtime
        std::size_t executed_threads = 0;

        int numsteps = 1;

        if (scheduler == 0) {
          global_runtime_type rt(hpx_host, hpx_port, agas_host, agas_port, mode);
          if (mode == hpx::runtime::worker) 
              rt.run(num_threads);
          else 
              rt.run(boost::bind(hpx_main, numvals, numsteps, do_logging, par), num_threads);

          executed_threads = rt.get_executed_threads();
        } 
        else if (scheduler == 1) {
          std::pair<std::size_t, std::size_t> init(/*vm["local"].as<int>()*/num_threads, 0);
          local_runtime_type rt(hpx_host, hpx_port, agas_host, agas_port, mode, init);
          if (mode == hpx::runtime::worker) 
              rt.run(num_threads);
          else 
              rt.run(boost::bind(hpx_main, numvals, numsteps, do_logging, par), num_threads);

          executed_threads = rt.get_executed_threads();
        } 
        else {
          BOOST_ASSERT(false);
        }

        std::cout << "Executed number of PX threads: " << executed_threads << std::endl;
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

