//  Copyright (c) 2010-2011 Phillip LeBlanc, Dylan Stark
//  Copyright (c)      2011 Bryce Lelbach, Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ABC9B037_3A25_4591_BB60_CD166773D61D)
#define ABC9B037_3A25_4591_BB60_CD166773D61D

#include <hpx/hpx_fwd.hpp>
#include <hpx/runtime.hpp>

#include <boost/program_options.hpp>
#include <boost/lexical_cast.hpp>

///////////////////////////////////////////////////////////////////////////////
// This function must be implemented by the application when for the console 
// locality.
int hpx_main(boost::program_options::variables_map& vm); 

///////////////////////////////////////////////////////////////////////////////
namespace hpx
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename T>
    inline void 
    get_option(boost::program_options::variables_map& vm,
               std::string const& name, T& x,
               std::string const& config = "")
    {
        if (vm.count(name)) 
            x = vm[name].as<T>();

        if (!config.empty()) 
            x = boost::lexical_cast<T>
                (get_runtime().get_config().get_entry(config, x));
    }

    template <typename T>
    inline void
    get_option(T& x, std::string const& config)
    {
        if (!config.empty())
            x = boost::lexical_cast<T>
                (get_runtime().get_config().get_entry(config, x));
    }

    ///////////////////////////////////////////////////////////////////////////
    /// This is the main entry point for any HPX application. This function 
    /// (or one of its overloads below) should be called from the users main()
    /// function. It will set up the HPX runtime environment and schedule the
    /// function given by \p f as a HPX thread.
    /// 
    /// \param f            [in] The function to be scheduled as an HPX 
    ///                     thread. Usually this function represents the main
    ///                     entry point of any HPX application.
    /// \param desc_cmdline [in] This parameter may hold the description of 
    ///                     additional command line arguments understood by the 
    ///                     application. These options will be prepended to 
    ///                     the default command line options understood by
    ///                     \a hpx::init (see description below).
    /// \param argc         [in] The number of command line arguments passed
    ///                     in \p argv. This is usually the unchanged value as
    ///                     passed by the operating system (to main()).
    /// \param argv         [in] The command line arguments for this 
    ///                     application, usually that is the value as passed
    ///                     by the operating system (to main()).
    /// \param startup_function [in] A function to be executed inside a HPX
    ///                     thread before \p f is called. If this parameter
    ///                     is not given no function will be executed.
    /// \param shutdown_function [in] A function to be executed inside an HPX 
    ///                     thread while hpx::finalize is executed. If this 
    ///                     parameter is not given no function will be 
    ///                     executed.
    /// \param mode         [in] The mode the created runtime environment 
    ///                     should be initialized in. There has to be exactly 
    ///                     one locality in each HPX application which is 
    ///                     executed in console mode (\a hpx::runtime_mode_console),
    ///                     all other localities have to be run in worker mode
    ///                     (\a hpx::runtime_mode_worker). Normally this is 
    ///                     set up automatically, but sometimes it is necessary
    ///                     to explicitly specify the mode.
    ///
    /// \returns            The function returns the value, which has been 
    ///                     returned from the user supplied \p f.
    ///
    /// \note               If the parameter \p mode is not given (defaulted), 
    ///                     the created runtime system instance will be 
    ///                     executed in console or worker mode depending on the
    ///                     command line arguments passed in argc/argv. 
    ///                     Otherwise it will be executed as specified by the 
    ///                     parameter\p mode.
    int init(int (*f)(boost::program_options::variables_map& vm),
        boost::program_options::options_description& desc_cmdline, 
        int argc, char* argv[],
        boost::function<void()> startup_function = boost::function<void()>(),
        boost::function<void()> shutdown_function = boost::function<void()>(),
        hpx::runtime_mode mode = hpx::runtime_mode_default);

    ///////////////////////////////////////////////////////////////////////////
    /// This is a simplified main entry point, which can be used to set up the
    /// runtime for an HPX application (the runtime system will be set up in
    /// console mode or worker mode depending on the command line settings). 
    ///
    /// \param f            [in] The function to be scheduled as an HPX 
    ///                     thread. Usually this function represents the main
    ///                     entry point of any HPX application.
    /// \param app_name     [in] The name of the application. 
    /// \param argc         [in] The number of command line arguments passed
    ///                     in \p argv. This is usually the unchanged value as
    ///                     passed by the operating system (to main()).
    /// \param argv         [in] The command line arguments for this 
    ///                     application, usually that is the value as passed
    ///                     by the operating system (to main()).
    ///
    /// \returns            The function returns the value, which has been 
    ///                     returned from the user supplied \p f.
    ///
    /// \note               The created runtime system instance will be 
    ///                     executed in console or worker mode depending on the
    ///                     command line arguments passed in argc/argv.
    int init(int (*f)(boost::program_options::variables_map& vm),
        std::string const& app_name, int argc, char* argv[]);

    ///////////////////////////////////////////////////////////////////////////
    /// This is a simplified main entry point, which can be used to set up the
    /// runtime for an HPX application (the runtime system will be set up in
    /// console mode or worker mode depending on the command line settings). 
    /// 
    /// In console mode it will execute the user supplied function hpx_main, 
    /// in worker mode it will execute an empty hpx_main.
    ///
    /// \param desc_cmdline [in] This parameter may hold the description of 
    ///                     additional command line arguments understood by the 
    ///                     application. These options will be prepended to 
    ///                     the default command line options understood by
    ///                     \a hpx::init (see description below).
    /// \param argc         [in] The number of command line arguments passed
    ///                     in \p argv. This is usually the unchanged value as
    ///                     passed by the operating system (to main()).
    /// \param argv         [in] The command line arguments for this 
    ///                     application, usually that is the value as passed
    ///                     by the operating system (to main()).
    /// \param startup_function [in] A function to be executed inside a HPX
    ///                     thread before \p f is called. If this parameter
    ///                     is not given no function will be executed.
    /// \param shutdown_function [in] A function to be executed inside an HPX 
    ///                     thread while hpx::finalize is executed. If this 
    ///                     parameter is not given no function will be 
    ///                     executed.
    /// \param mode         [in] The mode the created runtime environment 
    ///                     should be initialized in. There has to be exactly 
    ///                     one locality in each HPX application which is 
    ///                     executed in console mode (\a hpx::runtime_mode_console),
    ///                     all other localities have to be run in worker mode
    ///                     (\a hpx::runtime_mode_worker). Normally this is 
    ///                     set up automatically, but sometimes it is necessary
    ///                     to explicitly specify the mode.
    ///
    /// \returns            The function returns the value, which has been 
    ///                     returned from hpx_main (or 0 when executed in 
    ///                     worker mode).
    ///
    /// \note               If the parameter \p mode is not given (defaulted), 
    ///                     the created runtime system instance will be 
    ///                     executed in console or worker mode depending on the
    ///                     command line arguments passed in argc/argv. 
    ///                     Otherwise it will be executed as specified by the 
    ///                     parameter\p mode.
    inline int 
    init(boost::program_options::options_description& desc_cmdline, 
        int argc, char* argv[],
        boost::function<void()> startup = boost::function<void()>(),
        boost::function<void()> shutdown = boost::function<void()>(),
        hpx::runtime_mode mode = hpx::runtime_mode_default)
    {
        return init(::hpx_main, desc_cmdline, argc, argv, startup, shutdown, mode);
    }

    ///////////////////////////////////////////////////////////////////////////
    /// This is a simplified main entry point, which can be used to set up the
    /// runtime for an HPX application (the runtime system will be set up in
    /// console mode or worker mode depending on the command line settings). 
    /// 
    /// In console mode it will execute the user supplied function hpx_main, 
    /// in worker mode it will execute an empty hpx_main.
    ///
    /// \param desc_cmdline [in] This parameter may hold the description of 
    ///                     additional command line arguments understood by the 
    ///                     application. These options will be prepended to 
    ///                     the default command line options understood by
    ///                     \a hpx::init (see description below).
    /// \param argc         [in] The number of command line arguments passed
    ///                     in \p argv. This is usually the unchanged value as
    ///                     passed by the operating system (to main()).
    /// \param argv         [in] The command line arguments for this 
    ///                     application, usually that is the value as passed
    ///                     by the operating system (to main()).
    /// \param mode         [in] The mode the created runtime environment 
    ///                     should be initialized in. There has to be exactly 
    ///                     one locality in each HPX application which is 
    ///                     executed in console mode (\a hpx::runtime_mode_console),
    ///                     all other localities have to be run in worker mode
    ///                     (\a hpx::runtime_mode_worker). Normally this is 
    ///                     set up automatically, but sometimes it is necessary
    ///                     to explicitly specify the mode.
    ///
    /// \returns            The function returns the value, which has been 
    ///                     returned from hpx_main (or 0 when executed in 
    ///                     worker mode).
    ///
    /// \note               If the parameter \p mode is runtime_mode_default, 
    ///                     the created runtime system instance will be 
    ///                     executed in console or worker mode depending on the
    ///                     command line arguments passed in argc/argv. 
    ///                     Otherwise it will be executed as specified by the 
    ///                     parameter\p mode.
    inline int 
    init(boost::program_options::options_description& desc_cmdline, int argc, 
        char* argv[], hpx::runtime_mode mode)
    {
        boost::function<void()> const empty;
        return init(::hpx_main, desc_cmdline, argc, argv, empty, empty, mode);
    }

    ///////////////////////////////////////////////////////////////////////////
    /// This is a simplified main entry point, which can be used to set up the
    /// runtime for an HPX application (the runtime system will be set up in
    /// console mode or worker mode depending on the command line settings). 
    /// 
    /// In console mode it will execute the user supplied function hpx_main, 
    /// in worker mode it will execute an empty hpx_main.
    ///
    /// \param app_name     [in] The name of the application. 
    /// \param argc         [in] The number of command line arguments passed
    ///                     in \p argv. This is usually the unchanged value as
    ///                     passed by the operating system (to main()).
    /// \param argv         [in] The command line arguments for this 
    ///                     application, usually that is the value as passed
    ///                     by the operating system (to main()).
    ///
    /// \returns            The function returns the value, which has been 
    ///                     returned from hpx_main (or 0 when executed in 
    ///                     worker mode).
    ///
    /// \note               The created runtime system instance will be 
    ///                     executed in console or worker mode depending on the
    ///                     command line arguments passed in argc/argv.
    inline int 
    init(std::string const& app_name, int argc = 0, char* argv[] = 0)
    {
        return init(::hpx_main, app_name, argc, argv);
    }

    ///////////////////////////////////////////////////////////////////////////
    /// This is a simplified main entry point, which can be used to set up the
    /// runtime for an HPX application (the runtime system will be set up in
    /// console mode or worker mode depending on the command line settings). 
    ///
    /// \param f            [in] The function to be scheduled as an HPX 
    ///                     thread. Usually this function represents the main
    ///                     entry point of any HPX application.
    /// \param app_name     [in] The name of the application. 
    /// \param argc         [in] The number of command line arguments passed
    ///                     in \p argv. This is usually the unchanged value as
    ///                     passed by the operating system (to main()).
    /// \param argv         [in] The command line arguments for this 
    ///                     application, usually that is the value as passed
    ///                     by the operating system (to main()).
    ///
    /// \returns            The function returns the value, which has been 
    ///                     returned from the user supplied function \p f.
    ///
    /// \note               The created runtime system instance will be 
    ///                     executed in console or worker mode depending on the
    ///                     command line arguments passed in argc/argv.
    inline int 
    init(int (*f)(boost::program_options::variables_map& vm),
        std::string const& app_name, int argc, char* argv[])
    {
        using boost::program_options::options_description; 

        options_description desc_commandline(
            "usage: " + app_name +  " [options]");

        if (argc == 0 || argv == 0)
        {
            char *dummy_argv[1] = { const_cast<char*>(app_name.c_str()) };
            return init(desc_commandline, 1, dummy_argv);
        }

        boost::function<void()> const empty;
        return init(f, desc_commandline, argc, argv, empty, empty);
    }

    ///////////////////////////////////////////////////////////////////////////
    void finalize(double shutdown_timeout = -1.0, 
        double localwait = -1.0);

    ///////////////////////////////////////////////////////////////////////////
    void disconnect(double shutdown_timeout = -1.0,
        double localwait = -1.0);
}

#endif // HPX_ABC9B037_3A25_4591_BB60_CD166773D61D

