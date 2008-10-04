//  Copyright (c) 2007-2008 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_NAMING_SERVER_RESOLVER_SERVER_MAR_24_2008_1005AM)
#define HPX_NAMING_SERVER_RESOLVER_SERVER_MAR_24_2008_1005AM

#include <string>
#include <vector>

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

#include <hpx/config.hpp>
#include <hpx/util/io_service_pool.hpp>
#include <hpx/runtime/naming/server/connection.hpp>
#include <hpx/runtime/naming/server/request_handler.hpp>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace naming 
{
    /// \class resolver_server resolver_server.hpp hpx/runtime/naming/resolver_server.hpp
    /// 
    /// The \a resolver_server implements the top-level class of the DGAS server.
    /// It can be used to instantiate a DGAS server listening at the given 
    /// network address.
    class HPX_EXPORT resolver_server : private boost::noncopyable
    {
    public:
        /// Construct the server to listen on the specified TCP address and 
        /// port, and serve up requests to the address translation service.
        ///
        /// \param io_service_pool 
        ///                 [in] The pool of networking threads to use to serve 
        ///                 outgoing requests
        /// \param address  [in] This is the address (IP address or 
        ///                 host name) of the locality this DGAS server 
        ///                 instance is listening on. If this value is not 
        ///                 specified the actual address will be taken from 
        ///                 the configuration file (hpx.ini).
        /// \param port     [in] This is the port number this DGAS server
        ///                 instance is listening on. If this value is not 
        ///                 specified the actual address will be taken from 
        ///                 the configuration file (hpx.ini).
        /// \param start_asynchronously 
        ///                 [in] This parameter allows to start the resolver 
        ///                 server instance immediately. The default is to 
        ///                 start immediately (the constructor blocks
        ///                 until the \a resolver_server#stop function has 
        ///                 been called). If this parameter is \a false the 
        ///                 server instance needs to be started explicitly 
        ///                 using a call to the \a resolver_server#run function.
        resolver_server (util::io_service_pool& io_service_pool, 
            std::string const& address = "", unsigned short port = -1, 
            bool start_asynchronously = true);

        /// Construct the server to listen to the endpoint given by the 
        /// locality and serve up requests to the address translation service.
        ///
        /// \param io_service_pool 
        ///                 [in] The pool of networking threads to use to serve 
        ///                 outgoing requests
        /// \param l        [in] This is the locality this DGAS server instance
        ///                 is running on.
        /// \param start_asynchronously 
        ///                 [in] This parameter allows to start the resolver 
        ///                 server instance immediately. The default is to 
        ///                 start immediately (the constructor blocks
        ///                 until the \a resolver_server#stop function has 
        ///                 been called). If this parameter is \a false the 
        ///                 server instance needs to be started explicitly 
        ///                 using a call to the \a resolver_server#run function.
        resolver_server (util::io_service_pool& io_service_pool, locality l, 
            bool start_asynchronously = false);

        /// Destruct the object. Stops the service if it has not been stopped
        /// already.
        ~resolver_server();

        /// Run the server's io_service loop.
        ///
        /// \param blocking [in] This allows to control whether this call 
        ///                 blocks until the resolver server system has been 
        ///                 stopped. 
        void run (bool blocking = true);

        /// \brief Stop the resolver server.
        void stop();

    private:
        /// Handle completion of an asynchronous accept and read operations.
        void handle_accept(boost::system::error_code const& e);
        void handle_completion(boost::system::error_code const& e);

        /// The pool of io_service objects used to perform asynchronous operations.
        util::io_service_pool& io_service_pool_;

        /// Acceptor used to listen for incoming connections.
        boost::asio::ip::tcp::acceptor acceptor_;

        /// The next connection to be accepted.
        server::connection_ptr new_connection_;

        /// The handler for all incoming requests.
        server::request_handler request_handler_;

        /// this represents the locality this server is running on
        naming::locality here_;
    };

///////////////////////////////////////////////////////////////////////////////
}}  // namespace hpx::naming

#include <hpx/config/warnings_suffix.hpp>

#endif 
