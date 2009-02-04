//  Copyright (c) 2007-2009 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_NAMING_ADDRESS_MAR_24_2008_0949AM)
#define HPX_NAMING_ADDRESS_MAR_24_2008_0949AM

#include <boost/cstdint.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/version.hpp>

#include <hpx/runtime/naming/locality.hpp>
#include <hpx/runtime/components/component_type.hpp>
#include <hpx/util/safe_bool.hpp>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
//  address serialization format version
#define HPX_ADDRESS_VERSION 0x20

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace naming
{
    /// \class address address.hpp hpx/runtime/naming/address.hpp
    ///
    /// 
    struct HPX_EXPORT address : public util::safe_bool<address>
    {
        typedef boost::int64_t component_type;
        typedef boost::uint64_t address_type;

        ///////////////////////////////////////////////////////////////////////
        address()
          : type_(components::component_invalid), address_(0)
        {}

        address(locality l)
          : locality_(l), type_(components::component_invalid), address_(0) 
        {}

        address(locality l, component_type t, void* lva)
          : locality_(l), type_(t), 
            address_(reinterpret_cast<address_type>(lva)) 
        {}

        address(locality l, component_type t, address_type a)
          : locality_(l), type_(t), address_(a) 
        {}

        // this gets called from the safe_bool base class 
        bool operator_bool() const 
        { 
            return components::component_invalid != type_ || 0 != address_; 
        }

        friend bool operator==(address const& lhs, address const& rhs)
        {
            return lhs.locality_ == rhs.locality_ &&
                   lhs.type_ == rhs.type_ && lhs.address_ == rhs.address_;
        }

        locality locality_;     /// locality: ip4 address/port number
        component_type type_;   /// component type this address is referring to
        address_type address_;  /// address (sequence number)

    private:
        friend std::ostream& operator<< (std::ostream&, address const&);

        // serialization support    
        friend class boost::serialization::access;

        template<class Archive>
        void save(Archive & ar, const unsigned int version) const
        {
            ar << locality_ << type_ << address_; 
        }

        template<class Archive>
        void load(Archive & ar, const unsigned int version)
        {
            if (version > HPX_ADDRESS_VERSION) {
                throw exception(version_too_new, 
                    "trying to load address with unknown version");
            }
            ar >> locality_ >> type_ >> address_; 
        }

        BOOST_SERIALIZATION_SPLIT_MEMBER()
    };

    inline std::ostream& operator<< (std::ostream& os, address const& addr)
    {
        os << "(" << addr.locality_ << ":" 
           << components::get_component_type_name((int)addr.type_) 
           << ":0x" << std::hex << addr.address_ << ")"; 
        return os;
    }

///////////////////////////////////////////////////////////////////////////////
}}

///////////////////////////////////////////////////////////////////////////////
// this is the current version of the address serialization format
// this definition needs to be in the global namespace
BOOST_CLASS_VERSION(hpx::naming::address, HPX_ADDRESS_VERSION)
BOOST_CLASS_TRACKING(hpx::naming::address, boost::serialization::track_never)

#include <hpx/config/warnings_suffix.hpp>

#endif
