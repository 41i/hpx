//  Copyright (c) 2008-2009 Chirag Dekate, Hartmut Kaiser, Anshul Tandon
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_RUNTIME_THREAD_AFFINITY_NOV_11_2008_0711PM)
#define HPX_RUNTIME_THREAD_AFFINITY_NOV_11_2008_0711PM

#include <hpx/hpx.hpp>
#include <hpx/hpx_fwd.hpp>

#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace threads
{
#if defined(BOOST_WINDOWS)

    inline std::size_t least_significant_bit(std::size_t mask)
    {
        if (mask) {
            int c = 0;    // will count v's trailing zero bits

            // Set mask's trailing 0s to 1s and zero rest
            mask = (mask ^ (mask - 1)) >> 1;
            for (/**/; mask; ++c)
                mask >>= 1;

            return std::size_t(1) << c;
        }
        return std::size_t(1);
    }

    bool set_affinity(boost::thread& thrd, std::size_t num_thread, 
        bool numa_sensitive)
    {
        unsigned int num_of_cores = boost::thread::hardware_concurrency();
        if (0 == num_of_cores)
            num_of_cores = 1;     // assume one core
        std::size_t affinity = num_thread % num_of_cores;

        DWORD_PTR process_affinity = 0, system_affinity = 0;
        if (GetProcessAffinityMask(GetCurrentProcess(), &process_affinity, 
              &system_affinity))
        {
            ULONG numa_nodes = 0;
            if (numa_sensitive && GetNumaHighestNodeNumber(&numa_nodes)) {
                ++numa_nodes;

                ULONG numa_node = affinity % numa_nodes;
                DWORD_PTR node_affinity = 0;
                if (!GetNumaNodeProcessorMask(numa_node, &node_affinity))
                    return false;

                DWORD_PTR mask = least_significant_bit(node_affinity) << (affinity / numa_nodes);
                while (!(mask & node_affinity)) {
                    mask <<= 1LL;
                    if (0 == mask)
                        mask = 0x01LL;
                }
                return SetThreadAffinityMask(thrd.native_handle(), mask) != 0;
            }
            else {
                DWORD_PTR mask = least_significant_bit(process_affinity) << affinity;
                while (!(mask & process_affinity)) {
                    mask <<= 1LL;
                    if (0 == mask)
                        mask = 0x01LL;
                }
                return SetThreadAffinityMask(thrd.native_handle(), mask) != 0;
            }
        }
        return false;
    }
    inline bool set_affinity(std::size_t affinity, bool numa_sensitive)
    {
        return true;
    }

#elif defined(__APPLE__)

    // the thread affinity code is taken from the example:
    // http://www.opensource.apple.com/darwinsource/projects/other/xnu-1228.3.13/tools/tests/affinity/pool.c

    #include <AvailabilityMacros.h>
    #include <mach/mach.h>
    #include <mach/mach_error.h>
#ifdef AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER
    #include <mach/thread_policy.h>
#endif

    inline bool set_affinity(boost::thread& thrd, std::size_t affinity, 
        bool numa_sensitive)
    {
        return true;
    }
    inline bool set_affinity(std::size_t num_thread, bool numa_sensitive)
    {
#ifdef AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER
        thread_extended_policy_data_t epolicy;
        epolicy.timeshare = FALSE;

        kern_return_t ret = thread_policy_set(mach_thread_self(), 
            THREAD_EXTENDED_POLICY, (thread_policy_t) &epolicy,
            THREAD_EXTENDED_POLICY_COUNT);

        if (ret != KERN_SUCCESS)
            return false;

        thread_affinity_policy_data_t policy;
        policy.affinity_tag = num_thread + 1;   // 1...N

        ret = thread_policy_set(mach_thread_self(), 
            THREAD_AFFINITY_POLICY, (thread_policy_t) &policy, 
            THREAD_AFFINITY_POLICY_COUNT);

        return ret == KERN_SUCCESS;
#else
        return true;
#endif
    }

#else

    #include <pthread.h>
    #include <sched.h>    // declares the scheduling interface
    #include <sys/syscall.h>
    #include <sys/types.h>

    inline bool set_affinity(boost::thread& thrd, std::size_t num_thread, 
        bool numa_sensitive)
    {
        return true;
    }
    bool set_affinity(std::size_t num_thread, bool numa_sensitive)
    {
        std::size_t num_of_cores = boost::thread::hardware_concurrency();
        if (0 == num_of_cores)
            num_of_cores = 1;     // assume one core

        cpu_set_t cpu;
        CPU_ZERO(&cpu);

        // limit this thread to one of the cores
        std::size_t num_blocks = 1;
        std::size_t num_cores = num_of_cores;

        num_blocks = boost::lexical_cast<std::size_t>(
            get_runtime().get_config().get_entry("system_topology.num_blocks",
                                                 num_blocks));
        num_cores = boost::lexical_cast<std::size_t>(
            get_runtime().get_config().get_entry("system_topology.num_cores",
                                                 num_cores));

        // Check sanity
        assert(num_blocks * num_cores == num_of_cores);

        // Choose thread mapping function and determine affinity
        std::string thread_map = "linear";
        thread_map = get_runtime().get_config().get_entry(
            "system_topology.thread_map", thread_map);

        std::size_t affinity;
        if (thread_map == "linear")
        {
          affinity = (num_thread) % num_of_cores;
        }
        else if (thread_map == "block_striped")
        {
          affinity = (num_thread/num_blocks)
                      + (num_cores * (num_thread % num_blocks));
        }
        else
        {
          assert(0);
        }

        CPU_SET(affinity, &cpu);
#if defined(HAVE_PTHREAD_SETAFFINITY_NP)
        if (0 == pthread_setaffinity_np(pthread_self(), sizeof(cpu), &cpu))
#else
        if (0 == sched_setaffinity(syscall(SYS_gettid), sizeof(cpu), &cpu))
#endif
        {
            sleep(0);   // allow the OS to pick up the change
            return true;
        }
        return false;
    }

#endif

}}

#endif

