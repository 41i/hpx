////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Bryce Lelbach
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////

#if !defined(HPX_38369627_57D4_4BCA_BA3B_F9E121FF10F8)
#define HPX_38369627_57D4_4BCA_BA3B_F9E121FF10F8

#include <boost/io/ios_state.hpp>
#include <boost/cstdint.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/version.hpp>

#include <hpx/util/safe_bool.hpp>
#include <hpx/runtime/naming/name.hpp>

///////////////////////////////////////////////////////////////////////////////
// partition serialization format version
#define HPX_PARTITION_VERSION 0x10

namespace hpx { namespace agas
{

struct partition
{
    typedef boost::uint32_t prefix_type;
    typedef naming::gid_type upper_bound_type;

    partition(prefix_type prefix, upper_bound_type const& upper)
      : prefix_(prefix), upper_(upper) {}

    bool operator==(partition const& rhs) const
    {
        return prefix_ == rhs.prefix_
            && upper_  == rhs.upper_; 
    }

    prefix_type get_prefix() const
    { return prefix_; }

    void set_prefix(prefix_type prefix)
    { prefix_ = prefix; }

    upper_bound_type get_upper_bound() const
    { return upper_; }

    void set_upper_bound(upper_bound_type upper_bound)
    { upper_ = upper_bound; }

  private:
    prefix_type prefix_;
    upper_bound_type upper_;

    friend class boost::serialization::access;

    template<class Archive>
    void save(Archive& ar, const unsigned int version) const
    { ar << prefix_ << upper_; }

    template<class Archive>
    void load(Archive& ar, const unsigned int version)
    {
        if (version > HPX_PARTITION_VERSION) {
            throw exception(version_too_new, 
                "trying to load partition with unknown version");
        }
        ar >> prefix_ >> upper_; 
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()
};

template <typename Char, typename Traits>
inline std::basic_ostream<Char, Traits>&
operator<< (std::basic_ostream<Char, Traits>& os, partition const& part)
{
    boost::io::ios_flags_saver ifs(os); 
    os << std::showbase << std::hex << "(" 
       << part.get_prefix() << " "
       << part.get_upper_bound() << ")";
    return os;
} 

}}

BOOST_CLASS_VERSION(hpx::agas::partition, HPX_PARTITION_VERSION)
BOOST_CLASS_TRACKING(hpx::agas::partition, boost::serialization::track_never)

#endif // HPX_38369627_57D4_4BCA_BA3B_F9E121FF10F8

