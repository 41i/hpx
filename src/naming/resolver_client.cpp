//  Copyright (c) 2007-2008 Hartmut Kaiser
//
//  Parts of this code were taken from the Boost.Asio library
//  Copyright (c) 2003-2007 Christopher M. Kohlhoff (chris at kohlhoff dot com)
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <vector>

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/cstdint.hpp>
#include <boost/assert.hpp>
#include <boost/throw_exception.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/integer/endian.hpp>

#include <hpx/exception.hpp>
#include <hpx/naming/locality.hpp>
#include <hpx/naming/address.hpp>
#include <hpx/naming/resolver_client.hpp>
#include <hpx/naming/resolver_client_connection.hpp>
#include <hpx/naming/server/reply.hpp>
#include <hpx/naming/server/request.hpp>
#include <hpx/util/portable_binary_oarchive.hpp>
#include <hpx/util/portable_binary_iarchive.hpp>
#include <hpx/util/container_device.hpp>
#include <hpx/util/asio_util.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace naming 
{
    resolver_client::resolver_client(util::io_service_pool& io_service_pool, 
            std::string const& address, unsigned short port,
            bool start_asynchronously) 
      : io_service_pool_(io_service_pool), 
        socket_(io_service_pool.get_io_service())
    {
        try {
            if (start_asynchronously)
                io_service_pool.run(false);

            using namespace boost::asio::ip;
            
            // try to convert the address string to an IP address directly
            boost::system::error_code error = boost::asio::error::try_again;
            tcp::endpoint ep;
            if (util::get_endpoint(address, port, ep)) {
                socket_.connect(ep, error);
                if (!error) 
                    return;
            }
            
            // resolve the given address
            tcp::resolver resolver(io_service_pool.get_io_service());
            tcp::resolver::query query(address, 
                boost::lexical_cast<std::string>(port));
            
            // Try each endpoint until we successfully establish a connection.
            tcp::resolver::iterator end;
            tcp::resolver::iterator it = resolver.resolve(query);
            for (/**/; it != end; ++it)
            {
                socket_.close();
                socket_.connect(*it, error);
                if (!error) 
                    break;
            }
            if (error) {
                socket_.close();
                boost::throw_exception(
                    hpx::exception(network_error, error.message()));
            }
        }
        catch (boost::system::error_code const& e) {
            boost::throw_exception(hpx::exception(network_error, e.message()));
        }
    }
    
    resolver_client::resolver_client(util::io_service_pool& io_service_pool, 
            locality l, bool start_asynchronously) 
      : io_service_pool_(io_service_pool), 
        socket_(io_service_pool.get_io_service())
    {
        if (start_asynchronously)
            io_service_pool.run(false);

        boost::system::error_code error = boost::asio::error::try_again;
        socket_.connect(l.get_endpoint(), error);
        if (error) {
            socket_.close();
            boost::throw_exception(
                hpx::exception(network_error, error.message()));
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    // synchronous functionality
    bool resolver_client::get_prefix(locality const& l, id_type& prefix)
    {
        // send request
        server::request req (server::command_getprefix, l);
        server::reply rep;            
        execute(req, rep);
        
        hpx::error s = (hpx::error) rep.get_status();
        if (s != success && s != repeated_request)
            boost::throw_exception(hpx::exception((error)s, rep.get_error()));

        prefix = rep.get_prefix();
        return s == success;
    }
    
    bool resolver_client::get_id_range(locality const& l, id_type& lower_bound, 
        id_type& upper_bound)
    {
        // send request
        server::request req (server::command_getidrange, l);
        server::reply rep;            
        execute(req, rep);
        
        hpx::error s = (hpx::error) rep.get_status();
        if (s != success && s != repeated_request)
            boost::throw_exception(hpx::exception((error)s, rep.get_error()));

        lower_bound = rep.get_lower_bound();
        upper_bound = rep.get_upper_bound();
        return s == success;
    }
    
    bool resolver_client::bind_range(id_type id, std::size_t count, 
        address const& addr, std::ptrdiff_t offset)
    {
        // send request
        server::request req (server::command_bind_range, id, count, addr, offset);
        server::reply rep;            
        execute(req, rep);
        
        hpx::error s = (hpx::error) rep.get_status();
        if (s != success && s != no_success)
            boost::throw_exception(hpx::exception((error)s, rep.get_error()));

        return s == success;
    }

    bool resolver_client::unbind_range(id_type id, std::size_t count)
    {
        // send request
        server::request req (server::command_unbind_range, id, count);
        server::reply rep;            
        execute(req, rep);

        hpx::error s = (hpx::error) rep.get_status();
        if (s != success && s != no_success)
            boost::throw_exception(hpx::exception((error)s, rep.get_error()));

        return s == success;
    }

    bool resolver_client::resolve(id_type id, address& addr)
    {
        // send request
        server::request req (server::command_resolve, id);
        server::reply rep;            
        execute(req, rep);
        
        hpx::error s = (hpx::error) rep.get_status();
        if (s != success && s != no_success)
            boost::throw_exception(hpx::exception((error)s, rep.get_error()));

        addr = rep.get_address();
        return s == success;
    }

    bool resolver_client::registerid(std::string const& ns_name, id_type id)
    {
        // send request
        server::request req (server::command_registerid, ns_name, id);
        server::reply rep;            
        execute(req, rep);
        
        hpx::error s = (hpx::error) rep.get_status();
        if (s != success && s != no_success)
            boost::throw_exception(hpx::exception((error)s, rep.get_error()));

        return s == success;
    }

    bool resolver_client::unregisterid(std::string const& ns_name)
    {
        // send request
        server::request req (server::command_unregisterid, ns_name);
        server::reply rep;            
        execute(req, rep);

        hpx::error s = (hpx::error) rep.get_status();
        if (s != success && s != no_success)
            boost::throw_exception(hpx::exception((error)s, rep.get_error()));

        return s == success;
    }

    bool resolver_client::queryid(std::string const& ns_name, id_type& id)
    {
        // send request
        server::request req (server::command_queryid, ns_name);
        server::reply rep;            
        execute(req, rep);
        
        hpx::error s = (hpx::error) rep.get_status();
        if (s != success && s != no_success)
            boost::throw_exception(hpx::exception((error)s, rep.get_error()));

        id = rep.get_id();
        return s == success;
    }

    bool resolver_client::get_statistics_count(std::vector<std::size_t>& counts)
    {
        // send request
        server::request req (server::command_statistics_count);
        server::reply rep;            
        execute(req, rep);
        
        hpx::error s = (hpx::error) rep.get_status();
        if (s != success && s != no_success)
            boost::throw_exception(hpx::exception((error)s, rep.get_error()));

        counts.clear();
        for (std::size_t i = 0; i < server::command_lastcommand; ++i)
            counts.push_back(std::size_t(rep.get_statictics(i)));
            
        return s == success;
    }

    bool resolver_client::get_statistics_mean(std::vector<double>& timings)
    {
        // send request
        server::request req (server::command_statistics_mean);
        server::reply rep;            
        execute(req, rep);
        
        hpx::error s = (hpx::error) rep.get_status();
        if (s != success && s != no_success)
            boost::throw_exception(hpx::exception((error)s, rep.get_error()));

        timings.clear();
        for (std::size_t i = 0; i < server::command_lastcommand; ++i)
            timings.push_back(rep.get_statictics(i));
            
        return s == success;
    }

    bool resolver_client::get_statistics_moment2(std::vector<double>& timings)
    {
        // send request
        server::request req (server::command_statistics_moment2);
        server::reply rep;            
        execute(req, rep);
        
        hpx::error s = (hpx::error) rep.get_status();
        if (s != success && s != no_success)
            boost::throw_exception(hpx::exception((error)s, rep.get_error()));

        timings.clear();
        for (std::size_t i = 0; i < server::command_lastcommand; ++i)
            timings.push_back(rep.get_statictics(i));
            
        return s == success;
    }

    ///////////////////////////////////////////////////////////////////////////
    bool resolver_client::read_completed(boost::system::error_code const& err, 
        std::size_t bytes_transferred, boost::uint32_t size)
    {
        return bytes_transferred >= size;
    }
    
    void resolver_client::execute(server::request const &req, 
        server::reply& rep)
    {
        typedef util::container_device<std::vector<char> > io_device_type;

        try {
            // connect socket
            std::vector<char> buffer;
            {
                // serialize the request
                boost::iostreams::stream<io_device_type> io(buffer);
                util::portable_binary_oarchive archive(io);
                archive << req;
            }

            // send the data
            boost::integer::little32_t size = (boost::uint32_t)buffer.size();
            std::vector<boost::asio::const_buffer> buffers;
            buffers.push_back(boost::asio::buffer(&size, sizeof(size)));
            buffers.push_back(boost::asio::buffer(buffer));
            if (buffer.size() + sizeof(size) != boost::asio::write(socket_, buffers))
            {
                boost::throw_exception(
                    hpx::exception(network_error, "network write failed"));
            }
            
            // wait for response
            boost::system::error_code err = boost::asio::error::fault;

            // first read the size of the message 
            std::size_t reply_length = boost::asio::read(socket_,
                boost::asio::buffer(&size, sizeof(size)));
            if (reply_length != sizeof(size)) {
                boost::throw_exception(
                    hpx::exception(network_error, "network read failed"));
            }
            
            // now read the rest of the message
            boost::uint32_t native_size = size;
            buffer.resize(native_size);
            reply_length = boost::asio::read(socket_, boost::asio::buffer(buffer), 
                boost::bind(&resolver_client::read_completed, _1, _2, native_size), 
                err);

            if (err) {
                boost::throw_exception(
                    hpx::exception(network_error, err.message()));
            }
            if (reply_length != native_size) {
                boost::throw_exception(
                    hpx::exception(network_error, "network read failed"));
            }
            
            // De-serialize the data
            {
                boost::iostreams::stream<io_device_type> io(buffer);
                util::portable_binary_iarchive archive(io);
                archive >> rep;
            }
        }
        catch (boost::system::error_code const& e) {
            boost::throw_exception(hpx::exception(network_error, e.message()));
        }        
        catch (std::exception const& e) {
            boost::throw_exception(hpx::exception(network_error, e.what()));
        }
        catch(...) {
            boost::throw_exception(hpx::exception(no_success, 
                "unexpected error"));
        } 
    }

    ///////////////////////////////////////////////////////////////////////////
    // asynchronous API
    util::unique_future<bool> 
    resolver_client::bind_range_async(id_type lower_id, std::size_t count, 
        address const& addr, std::ptrdiff_t offset)
    {
        typedef resolver_client_connection<bool> connection_type;
        
        // prepare request
        server::request req(server::command_bind_range, lower_id, count, addr, offset);
        connection_type* conn = new connection_type(socket_, req);
        boost::shared_ptr<connection_type> client_conn(conn);
        
        conn->execute();
        return conn->get_future();
    }

    util::unique_future<bool> 
    resolver_client::unbind_range_async(id_type lower_id, std::size_t count)
    {
        typedef resolver_client_connection<bool> connection_type;
        
        // prepare request
        server::request req(server::command_unbind_range, lower_id, count);
        connection_type* conn = new connection_type(socket_, req);
        boost::shared_ptr<connection_type> client_conn(conn);

        conn->execute();
        return conn->get_future();
    }

    util::unique_future<std::pair<bool, address> >  
    resolver_client::resolve_async(id_type id)
    {
        typedef 
            resolver_client_connection<std::pair<bool, address> > 
        connection_type;
        
        // prepare request
        server::request req(server::command_resolve, id);
        connection_type* conn = new connection_type(socket_, req);
        boost::shared_ptr<connection_type> client_conn(conn);
        
        conn->execute();
        return conn->get_future();
    }

///////////////////////////////////////////////////////////////////////////////
}}  // namespace hpx::naming

