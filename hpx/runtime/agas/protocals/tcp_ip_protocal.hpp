////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Bryce Lelbach
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////

#if !defined(HPX_3E01AB13_F49A_4A38_9B19_E18B11F4102D)
#define HPX_3E01AB13_F49A_4A38_9B19_E18B11F4102D

#include <boost/asio/ip/tcp.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/tracking.hpp>

#include <hpx/runtime/agas/local_address.hpp>
#if 0
    #include <hpx/runtime/agas/namespaces/primary.hpp>
    #include <hpx/runtime/agas/namespaces/locality.hpp>
#else
    #include <hpx/runtime/agas/traits.hpp>
#endif

namespace hpx { namespace agas // hpx::agas
{

namespace tag { struct tcp_ip_protocal; }

namespace traits // hpx::agas::traits
{

template <>
struct protocal_name_hook<tag::tcp_ip_protocal>
{
    typedef std::string result_type;

    static result_type call()
    { return "TCP/IP"; }
};

template <>
struct locality_type<tag::tcp_ip_protocal>
{ typedef boost::asio::ip::tcp::endpoint type; };

} // hpx::agas::traits

typedef local_address<tag::tcp_ip_protocal> tcp_ip_local_address;

} // hpx::agas

///////////////////////////////////////////////////////////////////////////////
namespace components { namespace agas // hpx::components::agas
{

#if 0
typedef primary_namespace_type<hpx::agas::tag::tcp_ip_protocal>::type
    tcp_ip_primary_namespace;
typedef locality_namespace_type<hpx::agas::tag::tcp_ip_protocal>::type
    tcp_ip_locality_namespace;
#endif

///////////////////////////////////////////////////////////////////////////////
namespace server // hpx::components::agas::server
{

#if 0
typedef primary_namespace_type<hpx::agas::tag::tcp_ip_protocal>::type
    tcp_ip_primary_namespace;
typedef locality_namespace_type<hpx::agas::tag::tcp_ip_protocal>::type
    tcp_ip_locality_namespace;
#endif

} // hpx::components::agas::stubs

///////////////////////////////////////////////////////////////////////////////
namespace stubs // hpx::components::agas::stubs
{

#if 0
typedef primary_namespace_type<hpx::agas::tag::tcp_ip_protocal>::type
    tcp_ip_primary_namespace;
typedef locality_namespace_type<hpx::agas::tag::tcp_ip_protocal>::type
    tcp_ip_locality_namespace;
#endif

} // hpx::components::agas::stubs

} // hpx::components::agas
} // hpx::components
} // hpx

///////////////////////////////////////////////////////////////////////////////
BOOST_CLASS_VERSION(
    hpx::agas::tcp_ip_local_address,
    hpx::agas::traits::serialization_version<
        hpx::agas::tag::tcp_ip_protocal
    >::value)
BOOST_CLASS_TRACKING(
    hpx::agas::tcp_ip_local_address, boost::serialization::track_never)

#endif // HPX_3E01AB13_F49A_4A38_9B19_E18B11F4102D

