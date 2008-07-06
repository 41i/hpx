//  Copyright (c) 2007-2008 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx_fwd.hpp>

#include <boost/config.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include <hpx/include/runtime.hpp>

///////////////////////////////////////////////////////////////////////////////
// Make sure the system gets properly shut down while handling Ctrl-C and other
// system signals
#if defined(BOOST_WINDOWS)

static boost::function0<void> console_ctrl_function;

BOOL WINAPI console_ctrl_handler(DWORD ctrl_type)
{
    switch (ctrl_type) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        console_ctrl_function();
        return TRUE;
        
    default:
        return FALSE;
    }
}

#else

#include <pthread.h>
#include <signal.h>

#endif

///////////////////////////////////////////////////////////////////////////////
namespace hpx 
{
    ///////////////////////////////////////////////////////////////////////////
    runtime::runtime(std::string const& dgas_address, unsigned short dgas_port,
            std::string const& address, unsigned short port) 
      : dgas_pool_(), parcel_pool_(), timer_pool_(),
        dgas_client_(dgas_pool_, dgas_address, dgas_port),
        parcel_port_(parcel_pool_, address, port),
        thread_manager_(timer_pool_),
        parcel_handler_(dgas_client_, parcel_port_, &thread_manager_),
        applier_(dgas_client_, parcel_handler_, thread_manager_, 
            runtime_support_, memory_),
        action_manager_(applier_)
    {}

    ///////////////////////////////////////////////////////////////////////////
    runtime::runtime(naming::locality dgas_address, naming::locality address) 
      : dgas_pool_(), parcel_pool_(), timer_pool_(),
        dgas_client_(dgas_pool_, dgas_address),
        parcel_port_(parcel_pool_, address),
        thread_manager_(timer_pool_),
        parcel_handler_(dgas_client_, parcel_port_, &thread_manager_),
        applier_(dgas_client_, parcel_handler_, thread_manager_, 
            runtime_support_, memory_),
        action_manager_(applier_)
    {}

    ///////////////////////////////////////////////////////////////////////////
    runtime::~runtime()
    {
        // stop all services
        parcel_port_.stop();      // stops parcel_pool_ as well
        thread_manager_.stop();   // stops timer_pool_ as well
        dgas_pool_.stop();
    }

#if (defined(_WIN32) ||defined(_WIN64)) && defined (_DEBUG)
#include <io.h>
#endif

    ///////////////////////////////////////////////////////////////////////////
    void runtime::start(boost::function<hpx_main_function_type> func, 
        std::size_t num_threads, bool blocking)
    {
#if (defined(_WIN32) ||defined(_WIN64)) && defined (_DEBUG)
        // needs to be called to avoid problems at system startup
        // see: http://connect.microsoft.com/VisualStudio/feedback/ViewFeedback.aspx?FeedbackID=100319
        _isatty(0);
#endif

        // start services (service threads)
        thread_manager_.run(num_threads);   // start the thread manager, timer_pool_ as well
        parcel_port_.run(false);            // starts parcel_pool_ as well

        // register the runtime_support and memory instances with the DGAS 
        dgas_client_.bind(applier_.get_runtime_support_gid(), 
            naming::address(parcel_port_.here(), 
                components::server::runtime_support::value, &runtime_support_));

        dgas_client_.bind(applier_.get_memory_gid(), 
            naming::address(parcel_port_.here(), 
                components::server::memory::value, &memory_));

        // register the given main function with the thread manager
        if (!func.empty())
        {
            thread_manager_.register_work(
                boost::bind(func, _1, boost::ref(applier_)));
        }

        // block if required
        if (blocking) 
            wait();     // wait for the shutdown_action to be executed
    }

    ///////////////////////////////////////////////////////////////////////////
    static void wait_helper(components::server::runtime_support& rts)
    {
        rts.wait();
    }

    void runtime::wait()
    {
#if defined(BOOST_WINDOWS)
        // Set console control handler to allow server to be stopped.
        console_ctrl_function = boost::bind(&runtime::stop, this, true);
        SetConsoleCtrlHandler(console_ctrl_handler, TRUE);

        // wait for the shutdown action to be executed
        wait_helper(runtime_support_);
#else
        // Block all signals for background thread.
        sigset_t new_mask;
        sigfillset(&new_mask);
        sigset_t old_mask;
        pthread_sigmask(SIG_BLOCK, &new_mask, &old_mask);

        // start the wait_helper in a separate thread
        boost::thread t (
            boost::bind(&wait_helper, boost::ref(runtime_support_)));

        // Restore previous signals.
        pthread_sigmask(SIG_SETMASK, &old_mask, 0);

        // Wait for signal from waiting thread indicating time to shut down.
        sigset_t wait_mask;
        sigemptyset(&wait_mask);
        sigaddset(&wait_mask, SIGINT);
        sigaddset(&wait_mask, SIGQUIT);
        sigaddset(&wait_mask, SIGTERM);
        pthread_sigmask(SIG_BLOCK, &wait_mask, 0);

        // block main thread, this will exit as soon as Ctrl-C has been issued 
        // or any other signal has been received
        int sig = 0;
        sigwait(&wait_mask, &sig);

        t.join();
#endif
    }

    ///////////////////////////////////////////////////////////////////////////
    void runtime::stop(bool blocking)
    {
        try {
            // unregister the runtime_support and memory instances from the DGAS 
            naming::id_type factoryid (parcel_handler_.get_prefix().get_msb()+1, 0);
            dgas_client_.unbind(factoryid);
            dgas_client_.unbind(parcel_handler_.get_prefix());
        }
        catch(hpx::exception const&) {
            ; // ignore errors during system shutdown (DGAS might be down already)
        }

        // stop runtime services (threads)
        thread_manager_.stop(blocking);
        parcel_port_.stop(blocking);    // stops parcel_pool_ as well
        dgas_pool_.stop();
        runtime_support_.stop();        // re-activate main thread 
    }

    ///////////////////////////////////////////////////////////////////////////
    void runtime::run(boost::function<hpx_main_function_type> func,
        std::size_t num_threads)
    {
        start(func, num_threads);
        wait();
        stop();
    }

    ///////////////////////////////////////////////////////////////////////////
    void runtime::run(std::size_t num_threads)
    {
        start(boost::function<hpx_main_function_type>(), num_threads);
        wait();
        stop();
    }

}

