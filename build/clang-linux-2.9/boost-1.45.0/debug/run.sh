#! /usr/bin/env bash
#
# Copyright (c) 2011 Bryce Lelbach
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying 
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

CFE_VER=2.9
BOOST_VER=1.45.0

cmake -DCMAKE_C_COMPILER="/usr/bin/clang-${CFE_VER}"                \
      -DCMAKE_CXX_COMPILER="/usr/bin/clang++-${CFE_VER}"            \
      -DCMAKE_BUILD_TYPE=Debug                                      \
      -DBOOST_DEBUG=ON                                              \
      -DBOOST_ROOT="/opt/boost-${BOOST_VER}"                        \
      -DBOOST_LIB_DIR="/opt/boost-${BOOST_VER}/clang-${CFE_VER}/lib"\
      -DCMAKE_PREFIX="."                                            \
      ../../../..

