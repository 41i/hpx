//  Copyright (c) 2007-2011 Hartmut Kaiser
//  Copyright (c) 2011 Bryce Lelbach and Katelyn Kufahl
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_PARCELSET_PARCELPORT_CONNECTION_MAY_20_2008_1132PM)
#define HPX_PARCELSET_PARCELPORT_CONNECTION_MAY_20_2008_1132PM

#include <sstream>
#include <vector>

#include <hpx/runtime/parcelset/server/parcelport_queue.hpp>
#include <hpx/util/connection_cache.hpp>

#include <boost/atomic.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/bind.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/integer/endian.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace parcelset 
{
    /// Represents a single parcelport_connection from a client.
    class parcelport_connection
      : public boost::enable_shared_from_this<parcelport_connection>,
        private boost::noncopyable
    {
    public:
        

        /// Construct a sending parcelport_connection with the given io_service.
        parcelport_connection(boost::asio::io_service& io_service,
                naming::locality const& l, 
                util::connection_cache<parcelport_connection>& cache,
                boost::atomic<std::size_t>& started,
                boost::atomic<std::size_t>& completed)
          : socket_(io_service), there_(l), connection_cache_(cache),
            sends_started_(started), sends_completed_(completed)
        {
        }

        void set_parcel (parcel const& p);

        /// Get the socket associated with the parcelport_connection.
        boost::asio::ip::tcp::socket& socket() { return socket_; }

        /// Asynchronously write a data structure to the socket.
        template <typename Handler>
        void async_write(Handler handler)
        {
            // After data structure is written, increment reference to total sends started
            ++sends_started_;

            // Write the serialized data to the socket. We use "gather-write" 
            // to send both the header and the data in a single write operation.
            std::vector<boost::asio::const_buffer> buffers;
            buffers.push_back(boost::asio::buffer(&out_size_, sizeof(out_size_)));
            buffers.push_back(boost::asio::buffer(out_buffer_));

            // this additional wrapping of the handler into a bind object is 
            // needed to keep  this parcelport_connection object alive for the whole
            // write operation
            void (parcelport_connection::*f)(boost::system::error_code const&, std::size_t,
                    boost::tuple<Handler>)
                = &parcelport_connection::handle_write<Handler>;

            boost::asio::async_write(socket_, buffers,
                boost::bind(f, shared_from_this(), 
                    boost::asio::placeholders::error, _2, 
                    boost::make_tuple(handler)));
        }

 
    protected:
        /// handle completed write operation
        template <typename Handler>
        void handle_write(boost::system::error_code const& e, std::size_t bytes,
            boost::tuple<Handler> handler)
        {
            // if there is an error sending a parcel it's likely logging will not 
            // work anyways, so don't log the error
//             if (e) {
//                 LPT_(error) << "parcelhandler: put parcel failed: " 
//                             << e.message();
//             }
//             else {
//                 LPT_(info) << "parcelhandler: put parcel succeeded";
//             }

            // just call initial handler
            boost::get<0>(handler)(e, bytes);

            // now we can give this connection back to the cache
            out_buffer_.clear();
            out_size_ = 0;
            connection_cache_.add(there_, shared_from_this());
            ++sends_completed_;
        }

    private:
        /// Socket for the parcelport_connection.
        boost::asio::ip::tcp::socket socket_;

        /// buffer for outgoing data
        boost::integer::ulittle64_t out_size_;
        std::vector<char> out_buffer_;

        /// the other (receiving) end of this connection
        naming::locality there_;

        /// The connection cache for sending connections
        util::connection_cache<parcelport_connection>& connection_cache_;

        boost::atomic<std::size_t>& sends_started_;
        boost::atomic<std::size_t>& sends_completed_;
    };

    typedef boost::shared_ptr<parcelport_connection> parcelport_connection_ptr;

///////////////////////////////////////////////////////////////////////////////
}}

#endif
