# Copyright (c) 2007-2011 Hartmut Kaiser
# Copyright (c) 2007-2008 Chirag Dekate
# Copyright (c)      2011 Bryce Lelbach
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying 
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

################################################################################
# project metadata
################################################################################
set(HPX_MAJOR_VERSION 0)
set(HPX_MINOR_VERSION 5)
set(HPX_PATCH_LEVEL   0)
set(HPX_VERSION "${HPX_MAJOR_VERSION}.${HPX_MINOR_VERSION}.${HPX_PATCH_LEVEL}")
set(HPX_SOVERSION ${HPX_MAJOR_VERSION})

################################################################################
# cmake configuration
################################################################################
if(NOT HPX_ROOT)
  set(HPX_ROOT $ENV{HPX_ROOT})
endif()

set(CMAKE_MODULE_PATH ${HPX_ROOT}/share/cmake)

# include additional macro definitions
include(HPX_Utils)

include(HPX_Distclean)

hpx_force_out_of_tree_build("This project requires an out-of-source-tree build. See INSTALL.rst. Clean your CMake cache and CMakeFiles if this message persists.")

# be pedantic and annoying by default
if(NOT HPX_CMAKE_LOGLEVEL)
  set(HPX_CMAKE_LOGLEVEL "WARN")
endif()

################################################################################
# environment detection 
################################################################################
# FIXME: broken for MSVC
execute_process(COMMAND "${HPX_ROOT}/bin/hpx_environment.py" "${CMAKE_CXX_COMPILER}"
                OUTPUT_VARIABLE build_environment
                OUTPUT_STRIP_TRAILING_WHITESPACE)

if(build_environment)
  set(BUILDNAME "${build_environment}" CACHE INTERNAL "A string describing the build environment.")
  hpx_info("environment" "Build environment is ${BUILDNAME}")
else()
  hpx_warn("environment" "Couldn't determine build environment (install python).") 
endif()

################################################################################
# Boost configuration
################################################################################
# Boost.Chrono is in the Boost trunk, but has not been in a Boost release yet

option(HPX_INTERNAL_CHRONO "Use HPX's internal version of Boost.Chrono (default: ON)" ON)

# this cmake module will snag the Boost version we'll be using (which we need
# to know to specify the Boost libraries that we want to look for).
find_package(HPX_BoostVersion)

if(NOT BOOST_VERSION_FOUND)
  hpx_error("boost" "Failed to locate Boost.")
endif()

if(NOT HPX_INTERNAL_CHRONO OR ${BOOST_MINOR_VERSION} GREATER 46)
  set(BOOST_LIBRARIES chrono
                      date_time
                      filesystem
                      program_options
                      regex
                      serialization
                      system
                      signals
                      thread)
else()
  set(BOOST_LIBRARIES date_time
                      filesystem
                      program_options
                      regex
                      serialization
                      system
                      signals
                      thread)
  add_definitions(-DHPX_INTERNAL_CHRONO)
  add_definitions(-DBOOST_CHRONO_NO_LIB)
endif()

# We have a patched version of FindBoost loosely based on the one that Kitware ships
find_package(HPX_Boost)

include_directories(${BOOST_INCLUDE_DIR})
link_directories(${BOOST_LIBRARY_DIR})

# Boost preprocessor definitions
add_definitions(-DBOOST_PARAMETER_MAX_ARITY=7)
add_definitions(-DBOOST_COROUTINE_USE_ATOMIC_COUNT) 
add_definitions(-DBOOST_COROUTINE_ARG_MAX=2)

################################################################################
# search path configuration
################################################################################
include_directories("${HPX_ROOT}/include")
link_directories("${HPX_ROOT}/lib")

################################################################################
# installation configuration
################################################################################
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "RelWithDebInfo")
endif()

# for backwards compatibility
if(CMAKE_PREFIX)
  set(CMAKE_INSTALL_PREFIX "${CMAKE_PREFIX}")
endif()

if(NOT CMAKE_INSTALL_PREFIX)
  if(UNIX)
    # on POSIX, we do a local system install by default 
    set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "Prefix prepended to install directories.")
  else()
    set(CMAKE_INSTALL_PREFIX "C:/Program Files/hpx" CACHE PATH "Prefix prepended to install directories.")
  endif()
endif()

set(CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}"
  CACHE PATH "Where to install ${PROJECT_NAME} (default: /usr/local/ for POSIX, C:/Program Files/hpx for Windows)." FORCE)

hpx_info("install" "Install root is ${CMAKE_INSTALL_PREFIX}")

add_definitions(-DHPX_PREFIX=\"${CMAKE_INSTALL_PREFIX}\")

################################################################################
# rpath configuration
################################################################################
set(CMAKE_SKIP_BUILD_RPATH TRUE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_RPATH} "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

################################################################################
# Warning configuration
################################################################################
option(HPX_DISABLE_WARNINGS "Disable all warnings (default: OFF)" OFF)

if(NOT HPX_DISABLE_WARNINGS)
  add_definitions(-Wall)
endif()

################################################################################
# Windows specific configuration 
################################################################################
if(MSVC)
  if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    add_definitions(-DDEBUG)
  endif()
  
  set(CMAKE_DEBUG_POSTFIX "d")

  # we auto-link on Windows
  set(BOOST_FOUND_LIBRARIES "")

  add_definitions(-D_WINDOWS)
  add_definitions(-DBOOST_USE_WINDOWS_H)
  add_definitions(-D_WIN32_WINNT=0x0501)
  add_definitions(-D_SCL_SECURE_NO_WARNINGS)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  add_definitions(-D_SCL_SECURE_NO_DEPRECATE)
  add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
  add_definitions(-D_CRT_NONSTDC_NO_WARNINGS)
  add_definitions(-DBOOST_ALL_DYN_LINK)

  # suppress certain warnings
  add_definitions(-wd4251 -wd4231 -wd4275 -wd4660 -wd4094 -wd4267 -wd4180 -wd4244)

  if(CMAKE_CL_64)
    add_definitions(-DBOOST_COROUTINE_USE_FIBERS)
  endif()
  
  # multiproccessor build
  add_definitions(-MP) 
  
  # TODO: implement
  #hpx_check_for_msvc_128bit_interlocked(hpx_HAVE_MSVC_128BIT_INTERLOCKED
  #  DEFINITIONS HPX_HAVE_CMPXCHG16B
  #              BOOST_ATOMIC_HAVE_MSVC_128BIT_INTERLOCKED # for the msvc code
  #              BOOST_ATOMIC_HAVE_CMPXCHG16B)             # for the fallback 

################################################################################
# POSIX specific configuration 
################################################################################
else()
  set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG"
    CACHE STRING "Debug flags (C++)" FORCE)
  set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG"
    CACHE STRING "MinSizeRel flags (C++)" FORCE)
  set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG"
    CACHE STRING "Release flags (C++)" FORCE)
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g -DNDEBUG"
    CACHE STRING "RelWithDebInfo flags (C++)" FORCE)

  if(NOT HPX_DISABLE_WARNINGS)
    add_definitions(-Wno-strict-aliasing -Wsign-promo)
  endif()

  ##############################################################################
  # GNU specific configuration
  ##############################################################################
  option(HPX_ELF_HIDDEN_VISIBILITY
    "Use -fvisibility=hidden for Release, MinSizeRel and RelWithDebInfo builds (GCC only, default: ON)" ON)
  if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    # Show the flags that toggle each warning
    add_definitions(-fdiagnostics-show-option)

    # I'm aware that __sync_fetch_and_nand changed semantics
    add_definitions(-Wno-sync-nand)

    if(HPX_ELF_HIDDEN_VISIBILITY)
      add_definitions(-DHPX_ELF_HIDDEN_VISIBILITY)
      add_definitions(-DBOOST_COROUTINE_GCC_HAVE_VISIBILITY)
      add_definitions(-DBOOST_PLUGIN_GCC_HAVE_VISIBILITY)
      set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -fvisibility=hidden")
      set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fvisibility=hidden")
    endif()
  ##############################################################################
  # Intel specific configuration
  ##############################################################################
  elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
    # warning #191: type qualifier is meaningless on cast type
    add_definitions(-diag-disable 191) 
    
    # warning #279: controlling expression is constant 
    add_definitions(-diag-disable 279) 
    
    # warning #68: integer conversion resulted in a change of sign 
    add_definitions(-diag-disable 68) 
    
    # warning #858: type qualifier on return type is meaningless 
    add_definitions(-diag-disable 858) 
    
    # warning #1125: virtual function override intended 
    add_definitions(-diag-disable 1125) 

    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -ipo")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -ipo")
  endif()

  ##############################################################################
  # POSIX configuration tests 
  ##############################################################################
  # GCC's -march=native will automatically tune generated code for the host
  # environment. This is available on newish versions of GCC only (4.3ish). If
  # this flag is used, the generated binaries will be less portable. This is why
  # we define the HPX_COMPILER_AUTO_TUNED macro.
  hpx_check_for_compiler_auto_tune(HPX_COMPILER_AUTO_TUNE
    ROOT ${HPX_ROOT}
    DEFINITIONS HPX_COMPILER_AUTO_TUNED) 
  
  if(HPX_COMPILER_AUTO_TUNE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
  endif() 

  # cmpxchg16b is an x86-64 extension present on most newer x86-64 machines.
  # It allows us to do double quadword (128bit) atomic compare and swap
  # operations, which is AWESOME. Note that early x86-64 processors do lack
  # this instruction.
  hpx_check_for_gnu_mcx16(HPX_GNU_MCX16
    ROOT ${HPX_ROOT}
    DEFINITIONS HPX_HAVE_GNU_SYNC_16
                BOOST_ATOMIC_HAVE_GNU_SYNC_16) # for the gnu code

  if(HPX_GNU_MCX16)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcx16")
  endif() 

  # __attribute__ ((aligned(16))) should align a variable to a 16-byte, however,
  # GCC sets an upper limit on the maximum alignment (__BIGGEST_ALIGNMENT__)
  # and some versions don't warn if you ask for an alignment above said limit.
  # Instead, they'll just silently use the maximum, which can be problematical. 
  hpx_check_for_gnu_aligned_16(HPX_GNU_ALIGNED_16
    ROOT ${HPX_ROOT}
    DEFINITIONS HPX_HAVE_GNU_ALIGNED_16
                BOOST_ATOMIC_HAVE_GNU_ALIGNED_16) # for the gnu code

  # __uint128_t and __int128_t are a nifty, albeit undocumented, GNU extension
  # that's been supported in GCC (4.1ish and up) and clang-linux for awhile
  # (strangely, intel-linux doesn't support this). This is particularly useful
  # for use with cmpxchg16b
  hpx_check_for_gnu_128bit_integers(HPX_GNU_128BIT_INTEGERS
    ROOT ${HPX_ROOT}
    DEFINITIONS HPX_HAVE_GNU_128BIT_INTEGERS
                BOOST_ATOMIC_HAVE_GNU_128BIT_INTEGERS) # for integral casts
 
  # we use movdqa for atomic 128bit loads and stores 
  hpx_cpuid("sse2" HPX_SSE2
    ROOT ${HPX_ROOT}
    DEFINITIONS HPX_HAVE_SSE2
                BOOST_ATOMIC_HAVE_SSE2)
  
  if(HPX_SSE2)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse2")
  endif() 

  # rdtsc is an x86 instruction that reads the value of a CPU time stamp
  # counter. rdtscp is an extension to rdtsc. The difference is that rdtscp is
  # a serializing instruction.
  hpx_cpuid("rdtsc" HPX_RDTSC
    ROOT ${HPX_ROOT}
    DEFINITIONS HPX_HAVE_RDTSC)
  hpx_cpuid("rdtscp" HPX_RDTSCP
    ROOT ${HPX_ROOT}
    DEFINITIONS HPX_HAVE_RDTSCP)

  hpx_check_for_pthread_affinity_np(HPX_PTHREAD_AFFINITY_NP
    ROOT ${HPX_ROOT}
    DEFINITIONS HPX_HAVE_PTHREAD_AFFINITY_NP)

  add_definitions(-D_GNU_SOURCE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

  set(hpx_LIBRARIES ${hpx_LIBRARIES} dl rt)

  set(HPX_MALLOC "tcmalloc" CACHE STRING
    "HPX malloc allocator (default: tcmalloc)" FORCE)

  find_package(HPX_TCMalloc)
  find_package(HPX_Jemalloc)

  if("${HPX_MALLOC}" MATCHES "tcmalloc|TCMalloc|TCMALLOC" AND NOT TCMALLOC_FOUND)
    hpx_warn("malloc" "tcmalloc allocator not found.")
  endif()

  if("${HPX_MALLOC}" MATCHES "jemalloc|Jemalloc|JEMALLOC" AND NOT JEMALLOC_FOUND)
    hpx_warn("malloc" "jemalloc allocator not found.")
  endif()
  
  set(hpx_MALLOC_LIBRARY "")

  if(NOT "${HPX_MALLOC}" MATCHES "system|System|SYSTEM")
    if(TCMALLOC_FOUND OR JEMALLOC_FOUND)
      if("${HPX_MALLOC}" MATCHES "tcmalloc|TCMalloc|TCMALLOC" OR NOT JEMALLOC_FOUND)
        hpx_info("malloc" "Using tcmalloc allocator.")
        set(hpx_MALLOC_LIBRARY ${TCMALLOC_LIBRARY})
        set(hpx_LIBRARIES ${hpx_LIBRARIES} ${TCMALLOC_LIBRARY})
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-builtin-cfree")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-builtin-pvalloc")
        add_definitions(-DHPX_TCMALLOC)
      else()
        hpx_info("malloc" "Using jemalloc allocator.")
        set(hpx_MALLOC_LIBRARY ${JEMALLOC_LIBRARY})
        set(hpx_LIBRARIES ${hpx_LIBRARIES} ${JEMALLOC_LIBRARY})
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-builtin-cfree")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-builtin-pvalloc")
        add_definitions(-DHPX_JEMALLOC)
      endif()
 
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-builtin-malloc")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-builtin-free")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-builtin-calloc")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-builtin-realloc")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-builtin-valloc")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-builtin-memalign")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-builtin-posix_memalign")
    else()
      hpx_info("malloc" "Using system allocator.")
      hpx_warn("malloc" "HPX will perform poorly without tcmalloc.")
    endif()
  else()
    hpx_info("malloc" "Using system allocator.")
    hpx_warn("malloc" "HPX will perform poorly without tcmalloc.")
  endif()
endif()

################################################################################
# Mac OS X specific configuration 
################################################################################
if("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
  add_definitions(-D_XOPEN_SOURCE=1) # for some reason Darwin whines without this
endif()

if(MSVC)
  set(hpx_LIBRARIES ${hpx_LIBRARIES} hpx hpx_serialization)
else()
  set(hpx_LIBRARIES ${hpx_LIBRARIES} hpx hpx_serialization hpx_asio hpx_ini)
endif()

