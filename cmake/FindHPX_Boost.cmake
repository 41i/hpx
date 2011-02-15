# Copyright (c) 2011 Bryce Lelbach
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying 
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

option(BOOST_DEBUG "Enable debugging for the FindBoost (default: OFF)." OFF)
    
option(BOOST_USE_MULTITHREADED "Set to true if multi-threaded boost libraries should be used (default: ON)." ON)
  
if(NOT BOOST_VERSION_FOUND)
  include(FindBoostVersion)
endif()

macro(get_boost_compiler_version)
  if(NOT BOOST_COMPILER_VERSION_FOUND)
  set(BOOST_COMPILER_VERSION_FOUND ON CACHE INTERNAL "Boost compiler version was found.")

  exec_program(${CMAKE_CXX_COMPILER}
    ARGS ${CMAKE_CXX_COMPILER_ARG1} -dumpversion
    OUTPUT_VARIABLE BOOST_COMPILER_VERSION)

  string(REGEX REPLACE "([0-9])\\.([0-9])(\\.[0-9])?" "\\1\\2"
    BOOST_COMPILER_VERSION ${BOOST_COMPILER_VERSION})

  endif()
endmacro()

macro(build_boost_libname BOOST_RAW_NAME)
  if(NOT BOOST_VERSION_FOUND)
    get_boost_version()
  endif()

  # Attempt to guess the compiler suffix
  if(NOT BOOST_COMPILER_VERSION_FOUND)
    get_boost_compiler_version()
  endif()
    
  set(BOOST_COMPILER_VERSION "")
  set(BOOST_LIB_SUFFIX "")

  if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel"
      OR "${CMAKE_CXX_COMPILER}" MATCHES "icl" 
      OR "${CMAKE_CXX_COMPILER}" MATCHES "icpc")
    if(WIN32)
      set(BOOST_COMPILER_VERSION "-iw")
    else()
      set(BOOST_COMPILER_VERSION "-il")
    endif()
  elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"
          OR "${CMAKE_CXX_COMPILER}" MATCHES "clang") 
    set(BOOST_COMPILER_VERSION "-clang")
  elseif(MSVC90)
    set(BOOST_COMPILER_VERSION "-vc90")
  elseif(MSVC10)
    set(BOOST_COMPILER_VERSION "-vc100")
  elseif(MSVC80)
    set(BOOST_COMPILER_VERSION "-vc80")
  elseif(MSVC71)
    set(BOOST_COMPILER_VERSION "-vc71")
  elseif(MSVC70) # Good luck!
    set(BOOST_COMPILER_VERSION "-vc7") 
  elseif(MSVC60) # Good luck!
    set(BOOST_COMPILER_VERSION "-vc6") 
  elseif(BORLAND)
    set(BOOST_COMPILER_VERSION "-bcb")
  elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "SunPro")
    set(BOOST_COMPILER_VERSION "-sw")
  elseif(MINGW)
    if(${BOOST_MAJOR_VERSION}.${BOOST_MINOR_VERSION} VERSION_LESS 1.34)
      set(BOOST_COMPILER_VERSION "-mgw") # no GCC version encoding prior to 1.34
    else()
      set(BOOST_COMPILER_VERSION "-mgw${BOOST_COMPILER_VERSION}")
    endif()
  elseif(UNIX AND CMAKE_COMPILER_IS_GNUCXX)
    if(${BOOST_MAJOR_VERSION}.${BOOST_MINOR_VERSION} VERSION_LESS 1.34)
      set(BOOST_COMPILER_VERSION "-gcc") # no GCC version encoding prior to 1.34
    else()
      # Determine which version of GCC we have.
      if(APPLE)
        if(BOOST_MINOR_VERSION)
          if(${BOOST_MINOR_VERSION} GREATER 35)
            # In Boost 1.36.0 and newer, the mangled compiler name used
            # on Mac OS X/Darwin is "xgcc".
            set(BOOST_COMPILER_VERSION "-xgcc${BOOST_COMPILER_VERSION}")
          else(${BOOST_MINOR_VERSION} GREATER 35)
            # In Boost <= 1.35.0, there is no mangled compiler name for the Mac OS X/Darwin version of GCC.
            set(BOOST_COMPILER_VERSION "")
          endif(${BOOST_MINOR_VERSION} GREATER 35)
        else(BOOST_MINOR_VERSION)
          # We don't know the Boost version, so assume it's pre-1.36.0.
          set(BOOST_COMPILER_VERSION "")
        endif()
      else()
        set(BOOST_COMPILER_VERSION "-gcc${BOOST_COMPILER_VERSION}")
      endif()
    endif()
  endif()

  set(BOOST_COMPILER_VERSION ${BOOST_COMPILER_VERSION} 
    CACHE STRING "Boost compiler version (default: guessed).")

  if(BOOST_SUFFIX)
    # user suffix
    set(BOOST_SUFFIX ${BOOST_SUFFIX} CACHE STRING
      "Boost library suffix (default: none).")
    set(BOOST_LIB_SUFFIX -${BOOST_SUFFIX})
  endif()

  if(BOOST_USE_MULTITHREADED)
    set(BOOST_LIB_SUFFIX -mt${BOOST_LIB_SUFFIX})
  endif()

  set(BOOST_LIBNAMES
      boost_${BOOST_RAW_NAME}${BOOST_COMPILER_VERSION}${BOOST_LIB_SUFFIX}-${BOOST_VERSION}
      libboost_${BOOST_RAW_NAME}${BOOST_COMPILER_VERSION}${BOOST_LIB_SUFFIX}-${BOOST_VERSION}
      boost_${BOOST_RAW_NAME}${BOOST_COMPILER_VERSION}-${BOOST_VERSION}
      libboost_${BOOST_RAW_NAME}${BOOST_COMPILER_VERSION}-${BOOST_VERSION}
      boost_${BOOST_RAW_NAME}${BOOST_LIB_SUFFIX}
      libboost_${BOOST_RAW_NAME}${BOOST_LIB_SUFFIX}
      boost_${BOOST_RAW_NAME}
      libboost_${BOOST_RAW_NAME})
endmacro()

macro(find_boost_library TARGET_LIB)
  if(NOT BOOST_VERSION_FOUND)
    get_boost_version()
  endif()
 
  # Non-system Boost tree (tarball, SCM checkout, etc)
  if(NOT BOOST_USE_SYSTEM)
    # Locate libraries 
    build_boost_libname(${TARGET_LIB}) 

    foreach(BOOST_TARGET ${BOOST_LIBNAMES})
      if(BOOST_DEBUG)
        message(STATUS "Looking for ${BOOST_TARGET} in ${BOOST_LIB_DIR}...")
      endif()

      find_library(BOOST_${TARGET_LIB}_LIBRARY NAMES ${BOOST_TARGET} PATHS ${BOOST_LIB_DIR} NO_DEFAULT_PATH)

      if(BOOST_${TARGET_LIB}_LIBRARY)
        if(BOOST_DEBUG)
          message(STATUS "Found for ${BOOST_TARGET} in ${BOOST_LIB_DIR}...")
        endif()
        break()
      endif()
    endforeach()
    
    if(NOT BOOST_${TARGET_LIB}_LIBRARY)
      message(WARNING "Could not locate Boost ${TARGET_LIB} shared library in ${BOOST_LIB_DIR}. Now searching the system.")
      unset(BOOST_${TARGET_LIB}_LIBRARY)
   
      build_boost_libname(${TARGET_LIB}) 
    
      foreach(BOOST_TARGET ${BOOST_LIBNAMES})
        if(BOOST_DEBUG)
          message(STATUS "Looking for ${BOOST_TARGET} in system path...")
        endif()

        find_library(BOOST_${TARGET_LIB}_LIBRARY NAMES ${BOOST_TARGET})

        if(BOOST_${TARGET_LIB}_LIBRARY)
          if(BOOST_DEBUG)
            message(STATUS "Found for ${BOOST_TARGET} in system path...")
          endif()
          break()
        endif()
      endforeach()
    
      if(NOT BOOST_${TARGET_LIB}_LIBRARY)
        set(BOOST_ERROR_MSG "Failed to locate Boost ${TARGET_LIB} shared library in ${BOOST_LIB_DIR} or in the default path.")
        message(FATAL_ERROR "${BOOST_ERROR_MSG}")
        unset(BOOST_${TARGET_LIB}_LIBRARY)
      else()
        set(BOOST_${TARGET_LIB}_LIBRARY ${BOOST_${TARGET_LIB}_LIBRARY}
          CACHE FILEPATH "Boost ${TARGET_LIB} shared library.")
        message(STATUS "Using ${BOOST_${TARGET_LIB}_LIBRARY} as Boost ${TARGET_LIB} shared library.")
      endif()
    else()
      set(BOOST_${TARGET_LIB}_LIBRARY ${BOOST_${TARGET_LIB}_LIBRARY}
        CACHE FILEPATH "Boost ${TARGET_LIB} shared library.")
      message(STATUS "Using ${BOOST_${TARGET_LIB}_LIBRARY} as Boost ${TARGET_LIB} shared library.")
    endif()
    
    list(APPEND BOOST_FOUND_LIBRARIES ${BOOST_${TARGET_LIB}_LIBRARY})
  
  # System Boost installation (deb, rpm, etc)
  else()
    # Locate libraries 
    build_boost_libname(${TARGET_LIB}) 
    
    foreach(BOOST_TARGET ${BOOST_LIBNAMES})
      if(BOOST_DEBUG)
        message(STATUS "Looking for ${BOOST_TARGET} in system path...")
      endif()

      find_library(BOOST_${TARGET_LIB}_LIBRARY NAMES ${BOOST_TARGET})

      if(BOOST_${TARGET_LIB}_LIBRARY)
        if(BOOST_DEBUG)
          message(STATUS "Found for ${BOOST_TARGET} in system path...")
        endif()
        break()
      endif()
    endforeach()
    
    if(NOT BOOST_${TARGET_LIB}_LIBRARY)
      message(FATAL_ERROR "Failed to locate Boost ${TARGET_LIB} shared library in the system path.")
      set(BOOST_FOUND OFF)
      unset(BOOST_${TARGET_LIB}_LIBRARY)
    else()
      set(BOOST_${TARGET_LIB}_LIBRARY ${BOOST_${TARGET_LIB}_LIBRARY}
        CACHE FILEPATH "Boost ${TARGET_LIB} shared library.")
      message(STATUS "Using ${BOOST_${TARGET_LIB}_LIBRARY} as Boost ${TARGET_LIB} shared library.")
    endif()
  
    list(APPEND BOOST_FOUND_LIBRARIES ${BOOST_${TARGET_LIB}_LIBRARY})
  endif()    
endmacro()

foreach(BOOST_LIB ${BOOST_LIBRARIES})
  find_boost_library(${BOOST_LIB})
endforeach()

set(BOOST_FOUND_LIBRARIES ${BOOST_FOUND_LIBRARIES} CACHE STRING "Boost shared libraries found by CMake (default: none).")

