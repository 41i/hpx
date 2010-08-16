#ifndef _HPLMATREX3_HPP
#define _HPLMATREX3_HPP

/*This is the HPLMatreX3 interface header file.
In order to keep things simple, only operations necessary
to to perform LUP decomposition are declared, which is
basically just constructors, assignment operators, 
a destructor, and access operators.
*/

#include <hpx/runtime.hpp>
#include <hpx/runtime/components/client_base.hpp>

#include "stubs/hplmatrex3.hpp"

namespace hpx { namespace components
{
    class HPLMatreX3 : public client_base<HPLMatreX3, stubs::HPLMatreX3>
    {
	typedef
	    client_base<HPLMatreX3, stubs::HPLMatreX3> base_type;

    public:
	//constructors and destructor
	HPLMatreX3(){}
	HPLMatreX3(naming::id_type gid) : base_type(gid){}
	void destruct(){
		BOOST_ASSERT(gid_);
		return this->base_type::destruct(gid_);
	}

	//initialization function
	int construct(unsigned int h, unsigned int w, unsigned int ab, unsigned int bs){
		BOOST_ASSERT(gid_);
		return this->base_type::construct(gid_,h,w,ab,bs);
	}

	//operators for assignment and data access
	double get(unsigned int row, unsigned int col) const{
		BOOST_ASSERT(gid_);
		return this->base_type::get(gid_,row,col);
	}
	void set(unsigned int row, unsigned int col, double val){
		BOOST_ASSERT(gid_);
		return this->base_type::set(gid_,row,col,val);
	}

	//functions for manipulating the matrix
	double LUsolve(){
		BOOST_ASSERT(gid_);
		return this->base_type::LUsolve(gid_);
	}
    };
}}

#endif
