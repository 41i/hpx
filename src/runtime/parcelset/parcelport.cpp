//  Copyright (c) 2007-2008 Hartmut Kaiser
//  Copyright (c) 2007 Richard D Guidry Jr
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <string>

#include <hpx/hpx_fwd.hpp>

#include <boost/version.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include <hpx/exception_list.hpp>
#include <hpx/runtime/naming/locality.hpp>
#include <hpx/runtime/parcelset/parcelport.hpp>
#include <hpx/util/io_service_pool.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace parcelset
{
    parcelport::parcelport(util::io_service_pool& io_service_pool, 
            naming::locality here)
      : io_service_pool_(io_service_pool),
        acceptor_(io_service_pool_.get_io_service()),
        parcels_(This()),
        connection_cache_(HPX_MAX_CONNECTION_CACHE_SIZE), here_(here)
    {
        // initialize network
        using boost::asio::ip::tcp;
        
        int tried = 0;
        exception_list errors;
        naming::locality::iterator_type end = here.accept_end();
        for (naming::locality::iterator_type it = 
                here.accept_begin(io_service_pool_.get_io_service()); 
             it != end; ++it, ++tried)
        {
            try {
                server::parcelport_connection_ptr conn(
                    new server::parcelport_connection(
                        io_service_pool_.get_io_service(), parcels_)
                );

                tcp::endpoint ep = *it;
                acceptor_.open(ep.protocol());
                acceptor_.set_option(tcp::acceptor::reuse_address(true));
                acceptor_.bind(ep);
                acceptor_.listen();
                acceptor_.async_accept(conn->socket(),
                    boost::bind(&parcelport::handle_accept, this,
                        boost::asio::placeholders::error, conn));
            }
            catch (boost::system::system_error const& e) {
                errors.add(e);   // store all errors
                continue;
            }
        }

        if (errors.get_error_count() == tried) {
            // all attempts failed
            HPX_THROW_EXCEPTION(network_error, errors.get_message());
        }
    }

    bool parcelport::run(bool blocking)
    {
        return io_service_pool_.run(blocking);
    }

    void parcelport::stop(bool blocking)
    {
        io_service_pool_.stop();
        if (blocking)
            io_service_pool_.join();
    }
    
    /// accepted new incoming connection
    void parcelport::handle_accept(boost::system::error_code const& e,
        server::parcelport_connection_ptr conn)
    {
        if (!e) {
        // handle this incoming parcel
            conn->async_read(
                boost::bind(&parcelport::handle_read_completion, this,
                boost::asio::placeholders::error));

        // create new connection waiting for next incoming parcel
            conn.reset(new server::parcelport_connection(
                io_service_pool_.get_io_service(), parcels_));
            acceptor_.async_accept(conn->socket(),
                boost::bind(&parcelport::handle_accept, this,
                    boost::asio::placeholders::error, conn));
        }
    }

    /// Handle completion of a read operation.
    void parcelport::handle_read_completion(boost::system::error_code const& e)
    {
        if (e && e != boost::asio::error::operation_aborted)
        {
            // FIXME: add error handling
            std::cerr << "Error: " << e.message() << std::endl;
        }
    }

///////////////////////////////////////////////////////////////////////////////
}}
