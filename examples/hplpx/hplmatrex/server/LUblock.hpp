////////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2011 Dan Kogler
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////
#ifndef _LUBLOCK_SERVER_HPP
#define _LUBLOCK_SERVER_HPP

/*This is the LUblock class implementation header file.
This is mostly just for storing data, but I hope to move
the Gaussian elimination functions here eventually.
*/

class LUblock
{
    public:
    //constructors and destructor
    LUblock(){}
    LUblock(unsigned int h, unsigned int w);
    ~LUblock();

    //functions for assignment and data access
    inline int getrows(){return rows;}
    inline int getcolumns(){return columns;}
    inline double get(unsigned int row, unsigned int col);
    inline void set(unsigned int row, unsigned int col, double val);

//    private://data members
    int rows;
    int columns;
    double** data;
};
//////////////////////////////////////////////////////////////////////////////////////

//the constructor initializes the matrix
LUblock::LUblock(unsigned int h, unsigned int w){
    rows = h;
    columns = w;

    data = (double**) std::malloc(h*sizeof(double*));
    for(int i=0;i<rows;i++){data[i]=(double*) std::malloc(w*sizeof(double));}
}

//the destructor frees the memory
LUblock::~LUblock(){
    int i;
    for(i=0;i<rows;i++){
        free(data[i]);
    }
    free(data);
}

//get() gives back an element in the original matrix
double LUblock::get(unsigned int row, unsigned int col){
    return data[row][col];
}

//set() assigns a value to an element in all matrices
void LUblock::set(unsigned int row, unsigned int col, double val){
    data[row][col] = val;
}

#endif
