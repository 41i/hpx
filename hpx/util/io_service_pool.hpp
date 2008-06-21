//  Copyright (c) 2007-2008 Hartmut Kaiser
//
//  Parts of this code were taken from the Boost.Asio library
//  Copyright (c) 2003-2007 Christopher M. Kohlhoff (chris at kohlhoff dot com)
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_UTIL_IO_SERVICE_POOL_MAR_26_2008_1218PM)
#define HPX_UTIL_IO_SERVICE_POOL_MAR_26_2008_1218PM

#include <vector>

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace util
{
    /// A pool of io_service objects.
    class io_service_pool : private boost::noncopyable
    {
    public:
        /// Construct the io_service pool.
        /// \param pool_size
        ///                 [in] The number of threads to run to serve incoming
        ///                 requests
        explicit io_service_pool(std::size_t pool_size = 1);

        /// Run all io_service objects in the pool. If join_threads is true
        /// this will also wait for all threads to complete
        bool run(bool join_threads = true);

        /// Stop all io_service objects in the pool.
        void stop();

        /// Join all io_service threads in the pool.
        void join();

        /// Get an io_service to use.
        boost::asio::io_service& get_io_service();

    private:
        typedef boost::shared_ptr<boost::asio::io_service> io_service_ptr;
        typedef boost::shared_ptr<boost::asio::io_service::work> work_ptr;

        /// The pool of io_services.
        std::vector<io_service_ptr> io_services_;
        std::vector<boost::shared_ptr<boost::thread> > threads_;

        /// The work that keeps the io_services running.
        std::vector<work_ptr> work_;

        /// The next io_service to use for a connection.
        std::size_t next_io_service_;

        /// set to true if stopped
        bool stopped_;
    };

///////////////////////////////////////////////////////////////////////////////
}}  // namespace hpx::util

#endif 
