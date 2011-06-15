///////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2011 Dan Kogler
//
//  Distributed under the Boost Software License, Version 1.0.(See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
///////////////////////////////////////////////////////////////////////////////
#ifndef _HPLMATREX_SERVER_HPP
#define _HPLMATREX_SERVER_HPP

/*This is the HPLMatreX class implementation header file.
In order to keep things simple, only operations necessary
to to perform LUP decomposition are declared, which is
basically just constructors, assignment operators,
a destructor, and access operators.
*/

#include <time.h>

#include <hpx/hpx.hpp>
#include <hpx/hpx_fwd.hpp>
#include <hpx/lcos/mutex.hpp>
#include <hpx/lcos/eager_future.hpp>
#include <hpx/runtime/applier/applier.hpp>
#include <hpx/runtime/actions/component_action.hpp>
#include <hpx/runtime/components/component_type.hpp>
#include <hpx/runtime/components/server/simple_component_base.hpp>

#include <boost/foreach.hpp>
#include <boost/random/linear_congruential.hpp>
#include "LUblock.hpp"

#include <boost/date_time/posix_time/posix_time.hpp>
using namespace boost::posix_time;

namespace hpx { namespace components { namespace server
{
    class HPX_COMPONENT_EXPORT HPLMatreX : 
          public simple_component_base<HPLMatreX>
    {
    public:
    //enumerate all of the actions that will(or can) be employed
        enum actions{
            hpl_construct=0,
            hpl_destruct=1,
            hpl_assign=2,
            hpl_partbsub=3,
            hpl_solve=5,
            hpl_swap=6,
            hpl_gmain=7,
            hpl_search=8,
            hpl_check=9
        };

    //constructors and destructor
    HPLMatreX(){}
    int construct(naming::id_type gid, int h, int l, int ab, int bs);
    ~HPLMatreX(){destruct();}
    void destruct();

    double LUsolve();

    private:
    void allocate();
    int assign(int row, int offset, bool complete, int seed);
    void pivot();
    int search_pivots(const int row);
    int swap(const int brow, const int bcol);
    void LU_gauss_manager();
    void LU_gauss_corner(const int iter);
    void LU_gauss_top(const int iter, const int bcol);
    void LU_gauss_left(const int brow, const int iter);
    void LU_gauss_trail(const int brow, const int bcol, const int iter);
//    int LU_mini_trail(const int srow, const int erow,
//                      const int scol, const int ecol, const int iter);
    int LU_gauss_main(const int brow, const int bcol,
                      const int iter, const int type);
    int LUbacksubst();
    int part_bsub(const int brow, const int bcol);
    double checksolve(int row, int offset, bool complete);
    void print();
    void print2();

    int rows;             //number of rows in the matrix
    int brows;            //number of rows of blocks in the matrix
    int columns;          //number of columns in the matrix
    int bcolumns;         //number of columns of blocks in the matrix
    int allocblock;       //reflects amount of allocation done per thread
    int blocksize;        //reflects amount of computation per thread
    int litBlock;         //size of inner loop blocks
    naming::id_type _gid; //the instances gid
    LUblock*** datablock; //stores the data being operated on
    double** factorData;  //stores factors for computations
    double** trueData;    //the original unaltered data
    double** transData;   //transpose of the original data(speeds up pivoting)
    double* solution;     //for storing the solution
    int* pivotarr;        //array for storing pivot elements
                          //(maps original rows to pivoted/reordered rows)
    int* tempivotarr;     //temporary copy of pivotarr
    lcos::mutex mtex;     //mutex

    public:
    //here we define the actions that will be used
    //the construct function
    typedef actions::result_action5<HPLMatreX, int, hpl_construct, 
        naming::id_type, int, int, int, int, &HPLMatreX::construct> 
        construct_action;
    //the destruct function
    typedef actions::action0<HPLMatreX, hpl_destruct,
        &HPLMatreX::destruct> destruct_action;
    //the assign function
    typedef actions::result_action4<HPLMatreX, int, hpl_assign, int,
        int, bool, int, &HPLMatreX::assign> assign_action;
    //the solve function
    typedef actions::result_action0<HPLMatreX, double, hpl_solve,
        &HPLMatreX::LUsolve> solve_action;
    //the search_pivots function
    typedef actions::result_action1<HPLMatreX, int, hpl_search, int,
        &HPLMatreX::search_pivots> search_action;
    //the swap function
    typedef actions::result_action2<HPLMatreX, int, hpl_swap, int,
        int, &HPLMatreX::swap> swap_action;
    //the main gaussian function
    typedef actions::result_action4<HPLMatreX, int, hpl_gmain, int,
        int, int, int, &HPLMatreX::LU_gauss_main> gmain_action;
    //part_bsub function
    typedef actions::result_action2<HPLMatreX, int, hpl_partbsub, int,
        int, &HPLMatreX::part_bsub> partbsub_action;
    //checksolve function
    typedef actions::result_action3<HPLMatreX, double, hpl_check, int,
        int, bool, &HPLMatreX::checksolve> check_action;

    //here begins the definitions of most of the future types that will be used
    //the first of which is for assign action
    typedef lcos::eager_future<server::HPLMatreX::assign_action> assign_future;
    //the search pivots future
    typedef lcos::eager_future<server::HPLMatreX::search_action> search_future;
    //Here is the swap future, which works the same way as the assign future
    typedef lcos::eager_future<server::HPLMatreX::swap_action> swap_future;
    //This future corresponds to the Gaussian elimination functions
    typedef lcos::eager_future<server::HPLMatreX::gmain_action> gmain_future;
    //the backsubst future is used to make sure all computations are complete
    //before returning from LUsolve, to avoid killing processes and erasing the
    //leftdata while it is still being worked on
    typedef lcos::eager_future<server::HPLMatreX::partbsub_action> partbsub_future;
    //the final future type for the class is used for checking the accuracy of
    //the results of the LU decomposition
    typedef lcos::eager_future<server::HPLMatreX::check_action> check_future;

    gmain_future**** main_futures; //A 3D array of pointers to gaussian
                                   //elimination futures declared here
                                   //after gmain_future is defined
    };
///////////////////////////////////////////////////////////////////////////////

    //the constructor initializes the matrix
    int HPLMatreX::construct(naming::id_type gid, int h, int l,
        int ab, int bs){
// / / /initialize class variables/ / / / / / / / / / / /
    if(ab > std::ceil(((float)h)*.5)){
        allocblock = (int)std::ceil(((float)h)*.5);
    }
    else{ allocblock = ab;}
    if(bs > h){
        blocksize = h;
    }
    else{ blocksize = bs;}
    litBlock = l;
    rows = h;
    brows = (int)std::floor((float)h/blocksize);
    columns = h+1;
    _gid = gid;
// / / / / / / / / / / / / / / / / / / / / / / / / / / /

    int i;          //just counting variables
    int offset = 1; //the initial offset used for the memory handling algorithm
    boost::rand48 gen;     //random generator used for seeding other generators
    gen.seed(time(NULL));
    allocate();    //allocate memory for the elements of the array

    //By making offset a power of two, the assign functions
    //are much simpler than they would be otherwise.
    //The result of the below computation is that offset will be
    //the greatest power of two less than or equal to the number
    //of rows in the matrix
    h = (int)std::ceil(((float)h)*.5);
    while(offset < h){
        offset *= 2;
    }
    //here we initialize the the matrix
    assign_future future(gid,(int)0,offset,false,gen());

    //initialize the pivot array
    for(i=0;i<rows;i++){pivotarr[i]=tempivotarr[i]=i;}
    future.get();
    return 1;
    }

    //allocate() allocates memory space for the matrix
    void HPLMatreX::allocate(){
    datablock = (LUblock***) std::malloc(brows*sizeof(LUblock**));
    factorData = (double**) std::malloc(rows*sizeof(double*));
    transData = (double**) std::malloc(columns*sizeof(double*));
    trueData = (double**) std::malloc(rows*sizeof(double*));
    pivotarr = (int*) std::malloc(rows*sizeof(int));
    tempivotarr = (int*) std::malloc(rows*sizeof(int));
    for(int i = 0;i < rows;i++){
        trueData[i] = (double*) std::malloc(columns*sizeof(double));
        transData[i] = (double*) std::malloc(rows*sizeof(double));
    }
    transData[rows] = (double*) std::malloc(rows*sizeof(double));
    for(int i = 0;i < brows;i++){
        datablock[i] = (LUblock**)std::malloc(brows*sizeof(LUblock*));
    }
    main_futures = (gmain_future****) 
        std::malloc(brows*sizeof(gmain_future***));
    for(int i = 0; i < brows; i++){
        main_futures[i] = (gmain_future***) std::malloc(brows*sizeof(gmain_future**));
        for(int j = 0; j < brows; j++){
            main_futures[i][j] = (gmain_future**) 
                std::malloc(std::min(i+1,j+1)*sizeof(gmain_future*));
    }   }
    }

    /*assign gives values to the empty elements of the array.
    The idea behind this algorithm is that the initial thread produces
    threads with offsets of each power of 2 less than (or equal to) the
    number of rows in the matrix.  Each of the generated threads repeats
    the process using its assigned row as the base and the new set of 
    offsets is each power of two that will not overlap with other threads'
    assigned rows. After each thread has produced all of its child threads,
    that thread initializes the data of its assigned row and waits for the
    child threads to complete before returning.*/
    int HPLMatreX::assign(int row, int offset, bool complete, int seed){
        //futures is used to allow this thread to continue spinning off new
        //threads while other threads work, and is checked at the end to make
        //certain all threads are completed before returning.
        std::vector<assign_future> futures;
        boost::rand48 gen;
        gen.seed(seed);

        //create multiple futures which in turn create more futures
        while(!complete){
        //there is only one more child thread left to produce
            if(offset <= allocblock){
                if(row + offset < rows){
                    futures.push_back(assign_future(_gid,
                       row+offset,offset,true,gen()));
                }
                complete = true;
            }
            //there are at least two more child threads to produce
            else{
                if(row + offset < rows){
                    futures.push_back(assign_future(_gid,row+offset,
            (int)(offset*.5),false,gen()));
                }
                offset = (int)(offset*.5);
            }
        }
    //initialize the assigned row
    int temp = std::min((int)offset, (int)(rows - row));
    int location;
    for(int i=0;i<temp;i++){
        location = row+i;
        for(int j=0;j<columns;j++){
            transData[j][location] = trueData[location][j] = (double) (gen() % 1000);
        }
    }
    //once all spun off futures are complete we are done
    BOOST_FOREACH(assign_future af, futures){
        af.get();
    }
    return 1;
    }

    //the destructor frees the memory
    void HPLMatreX::destruct(){
    int i;
    for(i=0;i<rows;i++){
        free(trueData[i]);
    }
    for(i=0;i<brows;i++){
        for(int j=0;j<brows;j++){
            for(int k=0;k<std::min(i+1,j+1);k++)
                delete main_futures[i][j][k];
            delete datablock[i][j];
            free(main_futures[i][j]);
        }
        free(main_futures[i]);
        free(datablock[i]);
    }
    free(main_futures);
    free(datablock);
    free(pivotarr);
    free(factorData);
    free(trueData);
    free(solution);
    }

//DEBUGGING FUNCTIONS/////////////////////////////////////////////////
    //print out the matrix
    void HPLMatreX::print(){
    for(int i = 0; i < brows; i++){
        for(int j = 0; j < datablock[i][0]->rows; j++){
        for(int k = 0; k < brows; k++){
            for(int l = 0; l < datablock[i][k]->columns; l++){
            std::cout<<datablock[i][k]->data[j][l]<<" ";
        }   }
        std::cout<<std::endl;
    }   }
    std::cout<<std::endl;
    }
    void HPLMatreX::print2(){
    for(int i = 0;i < rows; i++){
        for(int j = 0;j < columns; j++){
        std::cout<<trueData[i][j]<<" ";
        }
        std::cout<<std::endl;
    }
/*    std::cout<<std::endl;
    for(int i = 0;i < columns; i++){
        for(int j = 0;j < rows; j++){
        std::cout<<transData[i][j]<<" ";
        }
        std::cout<<std::endl;
    }
    std::cout<<std::endl;
*/    }
//END DEBUGGING FUNCTIONS/////////////////////////////////////////////

    //LUsolve is simply a wrapper function for LUfactor and LUbacksubst
    double HPLMatreX::LUsolve(){
    //first perform partial pivoting
    ptime starttime = ptime(microsec_clock::local_time());
    pivot();
    ptime temp = ptime(microsec_clock::local_time());
    std::cout<<"pivoting over "<<temp-starttime<<std::endl;

    //to initiate the Gaussian elimination
    LU_gauss_manager();
    ptime temp2 = ptime(microsec_clock::local_time());
    std::cout<<"finished gaussian "<<temp2 - temp<<std::endl;

    //allocate memory space to store the solution
    solution = (double*) std::malloc(rows*sizeof(double));
    //perform back substitution
    LUbacksubst();
    ptime endtime = ptime(microsec_clock::local_time());
    std::cout<<"bsub done "<<endtime-temp2<<std::endl;
    std::cout<<"total LU time: "<<endtime-starttime<<std::endl;

    int h = (int)std::ceil(((float)rows)*.5);
    int offset = 1;
    while(offset < h){offset *= 2;}
    //create a future to check the accuracy of the generated solution
    check_future chk(_gid,0,offset,false);
    return chk.get();
    }

    //pivot() finds the pivot element of each column and stores it. All
    //pivot elements are found before any swapping takes place so that
    //the swapping is simplified
    void HPLMatreX::pivot(){
    double max, temp;
    int maxRow, temp_piv, outer;
    int i=0,j;
    bool good;
    int guessedPivots[rows];
    std::vector<swap_future> futures;
    std::vector<search_future> searches;

    for(outer=0;outer<=brows;outer++){
        if(outer > 0 && outer <= brows/2){
            searches[outer-1].get();
            temp = (outer-1)*blocksize;
            for(j=(int)temp;j<rows;j++){
                guessedPivots[j] = tempivotarr[j];
                tempivotarr[j] = pivotarr[j];
            }
        }
        if(outer < (brows/2)){
            searches.push_back(search_future(_gid,(outer+1)*blocksize));
        }
        for(i=i;i<std::min((outer+1)*blocksize,rows-1);i++){
            good = true;
            if(outer > 0 && outer <= brows/2){
                for(j=i-blocksize;j<i;j++){
                    if(pivotarr[j] == guessedPivots[i]){
                        good = false;
                        break;
            }   }   }
            else{good = false;}
            if(good){
                temp_piv = pivotarr[i];
                pivotarr[i] = guessedPivots[i];
                for(j=i-blocksize+1;j<rows;j++){
                    if(pivotarr[j] == pivotarr[i]){
                        pivotarr[j] = temp_piv;
                        break;
            }   }   }
            else{
                maxRow = i;
                max = fabs(transData[i][pivotarr[i]]);
                temp_piv = pivotarr[i];
                for(j=i+1;j<rows;j++){
                    temp = fabs(transData[i][pivotarr[j]]);
                    if(temp > max){
                        max = temp;
                        maxRow = j;
                }   }
                pivotarr[i] = pivotarr[maxRow];
                pivotarr[maxRow] = temp_piv;
            }
        }
        //here we begin swapping portions of the matrix we have finished
        //finding the pivot values for
        if(outer<brows-1){futures.push_back(swap_future(_gid,outer,0));}
        else if(outer==brows){futures.push_back(swap_future(_gid,outer-1,0));}
    }
    //transData is no longer needed so free the memory and allocate
    //space for factorData
    for(i=0;i<rows;i++){
        free(transData[i]);
        factorData[i] = (double*) std::malloc(i*sizeof(double));
    }
    free(transData[rows]);
    free(transData);

    //ensure that all pivoting is complete
    BOOST_FOREACH(swap_future sf, futures){
        sf.get();
    }
    free(tempivotarr);
    }

    //search_pivots guesses where pivots will be
    int HPLMatreX::search_pivots(const int row){
    int i, j, maxRow, temp_piv;
    double max, temp;

    for(i=row;i<std::min(row+blocksize,rows);i++){
        maxRow = i-blocksize;
        max = fabs(transData[i][tempivotarr[i-blocksize]]);
        temp_piv = tempivotarr[i];
        for(j=i-blocksize+1;j<rows;j++){
            temp = fabs(transData[i][tempivotarr[j]]);
            if(temp > max){
                max = temp;
                maxRow = j;
        }   }
        tempivotarr[i] = tempivotarr[maxRow];
        tempivotarr[maxRow] = temp_piv;
    }
    return 1;
    }

    //swap() creates the datablocks and reorders the original
    //trueData matrix when assigning the initial values to the datablocks
    //according to the pivotarr data.  trueData itself remains unchanged
    int HPLMatreX::swap(const int brow, const int bcol){
    const int temp = rows/blocksize;
    int numrows = blocksize, numcols = blocksize;
    int i,j,k;
    for(k=0;k<brows;k++){
    if(brow == brows-1){numrows = rows - (temp-1)*blocksize;}
    if(k == brows-1){numcols = columns - (temp-1)*blocksize;}
    datablock[brow][k] = new LUblock(numrows,numcols);
    for(i=0;i<numrows;i++){
        for(j=0;j<numcols;j++){
        datablock[brow][k]->data[i][j] =
            trueData[pivotarr[brow*blocksize+i]][k*blocksize+j];
    }   }
    }
    return 1;
    }

    //LU_gauss_manager creates futures as old futures finish their computations
    void HPLMatreX::LU_gauss_manager(){
    int i,j;

    LU_gauss_corner(0);
    for(i = 0; i < brows-1; i++){
        main_futures[i+1][i][i] = new gmain_future(_gid,i+1,i,i,3);
        main_futures[i][i+1][i] = new gmain_future(_gid,i,i+1,i,2);
        for(j = 1; i+j < brows; j++){
            main_futures[i+j][i+j][i] = new gmain_future(_gid,i+j,i+j,i,1);
        }
        main_futures[i+1][i+1][i]->get();
        LU_gauss_corner(i+1);
    }
    }

    //LUgaussmain is a wrapper function which is used so that only one type of
    //action is needed instead of four types of actions
    int HPLMatreX::LU_gauss_main(const int brow,const int bcol,const int iter,
        const int type){
    if(iter > 0){
//        while(main_futures[brow][bcol][iter-1]==NULL){sleep(1);}
        main_futures[brow][bcol][iter-1]->get();
        if(type == 1){
//            while(main_futures[brow][iter][iter]==NULL){sleep(1);}
            main_futures[brow][iter][iter]->get();
//            while(main_futures[iter][bcol][iter]==NULL){sleep(1);}
            main_futures[iter][bcol][iter]->get();
    }   }
    if(type == 1){
        if(brow >= bcol && brow < brows - 1)
            main_futures[brow+1][bcol][iter] =
                new gmain_future(_gid,brow+1,bcol,iter,1);
        if(brow <= bcol && bcol < brows - 1)
            main_futures[brow][bcol+1][iter] =
                new gmain_future(_gid,brow,bcol+1,iter,1);
        LU_gauss_trail(brow,bcol,iter);
    }
    else if(type == 2){
       if(bcol < brows - 1)
           main_futures[brow][bcol+1][iter] =
               new gmain_future(_gid,brow,bcol+1,iter,2);
       LU_gauss_top(iter,bcol);
    }
    else{
        if(brow < brows - 1)
            main_futures[brow+1][bcol][iter] =
                new gmain_future(_gid,brow+1,bcol,iter,3);
        LU_gauss_left(brow,iter);
    }
    return 1;
    }

    //LUgausscorner peforms gaussian elimination on the topleft corner block
    //of data that has not yet completed all of it's gaussian elimination
    //computations. Once complete, this block will need no further computations
    void HPLMatreX::LU_gauss_corner(const int iter){
    int i, j, k;
    const int offset = iter*blocksize;        //factorData index offset
    double fFactor;
    double factor;

    for(i=0;i<datablock[iter][iter]->rows;i++){
        if(datablock[iter][iter]->data[i][i] == 0){
            std::cerr<<"Warning: divided by zero\n";
        }
        fFactor = 1/datablock[iter][iter]->data[i][i];
        for(j=i+1;j<datablock[iter][iter]->rows;j++){
            factor = fFactor*datablock[iter][iter]->data[j][i];
            factorData[j+offset][i+offset] = factor;
            for(k=i+1;k<datablock[iter][iter]->columns;k++){
            datablock[iter][iter]->data[j][k] -= 
                factor*datablock[iter][iter]->data[i][k];
    }   }   }
    }

    //LUgausstop performs gaussian elimination on the topmost row of blocks
    //that have not yet finished all gaussian elimination computation.
    //Once complete, these blocks will no longer need further computations
    void HPLMatreX::LU_gauss_top(const int iter, const int bcol){
    int i,j,k;
    const int offset = iter*blocksize;        //factorData index offset
    double factor;

    for(i=0;i<datablock[iter][bcol]->rows;i++){
        for(j=i+1;j<datablock[iter][bcol]->rows;j++){
            factor = factorData[j+offset][i+offset];
            for(k=0;k<datablock[iter][bcol]->columns;k++){
            datablock[iter][bcol]->data[j][k] -= 
                factor*datablock[iter][bcol]->data[i][k];
    }   }   }
    }

    //LUgaussleft performs gaussian elimination on the leftmost column of
    //blocks that have not yet finished all gaussian elimination computation.
    //Upon completion, no further computations need be done on these blocks.
    void HPLMatreX::LU_gauss_left(const int brow, const int iter){
    int i,j,k;
    const int offset = brow*blocksize;
    const int offsetCol = iter*blocksize;    //factorData offset
    double fFactor[datablock[brow][iter]->columns];
    double factor;

    for(i=0;i<datablock[brow][iter]->columns;i++){
        fFactor[i] = 1/datablock[iter][iter]->data[i][i];
        factor = fFactor[i]*datablock[brow][iter]->data[0][i];
        factorData[offset][i+offsetCol] = factor;
        for(k=i+1;k<datablock[brow][iter]->columns;k++){
            datablock[brow][iter]->data[0][k] -= 
                factor*datablock[iter][iter]->data[i][k];
    }   }
    for(j=1;j<datablock[brow][iter]->rows;j++){
        for(i=0;i<datablock[brow][iter]->columns;i++){
            factor = fFactor[i]*datablock[brow][iter]->data[j][i];
            factorData[j+offset][i+offsetCol] = factor;
            for(k=i+1;k<datablock[brow][iter]->columns;k++){
            datablock[brow][iter]->data[j][k] -= 
                factor*datablock[iter][iter]->data[i][k];
    }    }   }
    }

    //LUgausstrail performs gaussian elimination on the trailing submatrix of
    //the blocks operated on during the current iteration of the Gaussian
    //elimination computations. These blocks will still require further 
    //computations to be performed in future iterations.
    void HPLMatreX::LU_gauss_trail(const int brow,const int bcol,const int iter){
    int i,j,k,jj;
    const int offset = brow*blocksize;    //factorData row offset
    const int offsetCol = iter*blocksize;    //factorData column offset
    double factor,temp;

    //outermost loop: iterates over the fFactors of the most recent corner 
    //block (fFactors are used indirectly through factorData)
    //middle loop: iterates over the rows of the current block
    //inner loop: iterates across the columns of the current block
    for(jj=0;jj<datablock[brow][bcol]->rows;jj+=litBlock){
    for(j=jj;j<std::min(jj+litBlock,datablock[brow][bcol]->rows);j++){
        for(i=0;i<datablock[iter][iter]->rows;i++){
            factor = factorData[j+offset][i+offsetCol];
            for(k=0;k<datablock[brow][bcol]->columns;k++){
            temp = factor*datablock[iter][bcol]->data[i][k];
            datablock[brow][bcol]->data[j][k] -= temp;
    }   }   }
    }}

/*    int HPLMatreX::LU_mini_trail(const int srow,const int erow,const int scol,
                                 const int ecol,const int iter){
    const int brow = std::min(srow/blocksize,brows-1);
    const int bcol = std::min(scol/blocksize,brows-1);
    const int startrow = srow-brow*blocksize;
    const int endrow = erow-brow*blocksize;
    const int startcol = scol-bcol*blocksize;
    const int endcol = ecol-bcol*blocksize;
    int i, j, k;
    double factor;

    for(j=startrow;j<endrow;j++){
        for(i=0;i<blocksize;i++){
            factor = factorData[j+srow][i+scol];
            for(k=startcol;k<endcol;k++){
                datablock[brow][bcol]->data[j][k] -=
                    factor*datablock[iter][bcol]->data[i][k];
    }   }   }
    return 1;
    }
*/
    //this is an implementation of back substitution modified for use on
    //multiple datablocks instead of a single large data structure
    int HPLMatreX::LUbacksubst(){
    int i,k,l,row,temp;
    int neededFuture[brows-1];
    std::vector<partbsub_future> futures;

    for(i=0;i<brows;i++){
        temp = i*blocksize;
        for(k=0;k<datablock[i][0]->rows;k++){
            solution[temp+k] = 
            datablock[i][brows-1]->data[k][datablock[i][brows-1]->columns-1];
    }   }

    i = brows-1;
    row = i*blocksize;
    for(k=datablock[i][i]->columns-2;k>=0;k--){
        temp = row+k;
        solution[temp]/=datablock[i][i]->data[k][k];
        for(l=k-1;l>=0;l--){
            solution[row+l] -= datablock[i][i]->data[l][k]*solution[temp];
    }   }
    neededFuture[0] = futures.size();
    for(k=brows-2;k>=0;k--){futures.push_back(partbsub_future(_gid,k,i));}
    for(i=brows-2;i>=0;i--){
        row = i*blocksize;
        for(k=0;k<brows-i-1;k++){
            futures[neededFuture[k]].get();
            neededFuture[k]+=1;
        }
        for(k=blocksize-1;k>=0;k--){
            temp = row+k;
            solution[temp]/=datablock[i][i]->data[k][k];
            for(l=k-1;l>=0;l--){
                solution[row+l] -= datablock[i][i]->data[l][k]*solution[temp];
        }   }
        neededFuture[brows-i-1] = futures.size();
        for(k=i-1;k>=0;k--){futures.push_back(partbsub_future(_gid,k,i));}
    }
    return 1;
    }

    //part_bsub performs backsubstitution on a single block of data
    //the function is designed to both take advantage of cache locality
    //and to allow fine grained parallelism of the back substitution
    int HPLMatreX::part_bsub(const int brow, const int bcol){
        const int row = brow*blocksize, col = bcol*blocksize;
        int cols, i, j;
        if(bcol == brows-1){cols = datablock[bcol][bcol]->columns-1;}
        else{cols = blocksize;}

        for(i=0;i<blocksize;i++){
            for(j=0;j<cols;j++){
                solution[row+i]-=datablock[brow][bcol]->data[i][j]*solution[col+j];
        }   }
        return 1;
    }

    //finally, this function checks the accuracy of the LU computation a few
    //rows at a time
    double HPLMatreX::checksolve(int row, int offset, bool complete){
    double toterror = 0;    //total error from all checks
        //futures is used to allow this thread to continue spinning off new 
        //thread while other threads work, and is checked at the end to make
        // certain all threads are completed before returning.
        std::vector<check_future> futures;

        //start spinning off work to do
        while(!complete){
            if(offset <= allocblock){
                if(row + offset < rows){
                    futures.push_back(check_future(_gid,
                        row+offset,offset,true));
                }
                complete = true;
            }
            else{
                if(row + offset < rows){
            futures.push_back(check_future(_gid,row+offset,
                (int)(offset*.5),false));
                }
                offset = (int)(offset*.5);
            }
        }

    //accumulate the total error for a subset of the solutions
        int temp = std::min((int)offset, (int)(rows - row));
        int i,j;
        double sum;
        for(i=0;i<temp;i++){
        sum = 0;
            for(j=0;j<rows;j++){
                sum += trueData[pivotarr[row+i]][j] * solution[j];
            }
        toterror += std::fabs(sum-trueData[pivotarr[row+i]][rows]);
        }

        //collect the results and add them up
        BOOST_FOREACH(check_future cf, futures){
                toterror += cf.get();
        }
        return toterror;
    }
}}}

#endif
