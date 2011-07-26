////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Katelyn Kufahl
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////

#if !defined(HPX_76311D67_43DA_4B3A_8A2A_14B8A1A266D9)
#define HPX_76311D67_43DA_4B3A_8A2A_14B8A1A266D9

#include <hpx/runtime/naming/name.hpp>

#include <boost/cstdint.hpp>

namespace hpx { namespace performance_counters { namespace parcels
{

class data_point {

    friend class parcelset::parcelport;
    friend class parcelset::parcelport_connection;

    public:

    data_point(boost::int64_t start_, boost::int64_t end_, std::size_t bytes_,
        naming::gid_type const& parcel_):

        bytes(bytes_)
      , parcel(parcel_)
      , start(start_)
      , end(end_)
      {}

    data_point():

        bytes(0)
      , parcel(0)
      , start(0)
      , end(0)
      {}

    data_point(data_point const& x):

        bytes(x.bytes)
      , parcel(x.parcel)
      , start(x.start)
      , end(x.end)
      {}

    boost::int64_t elapsed() const
    {
        return end - start;
    }

    std::size_t bytes;
    naming::gid_type parcel;

    boost::int64_t start;
    boost::int64_t end;
};

}}}

#endif // HPX_76311D67_43DA_4B3A_8A2A_14B8A1A266D9

