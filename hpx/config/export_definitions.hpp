//  Copyright (c) 2007-2008 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_EXPORT_DEFINITIONS_SEPTEMBER_25_2008_0214PM)
#define HPX_EXPORT_DEFINITIONS_SEPTEMBER_25_2008_0214PM

// For symbol import/export macros
#include <boost/config.hpp>

# if defined(BOOST_WINDOWS)
#   define HPX_SYMBOL_EXPORT      __declspec(dllexport)
#   define HPX_SYMBOL_IMPORT      __declspec(dllimport)
#   define HPX_SYMBOL_INTERNAL    /* empty */
# elif defined(HPX_GCC_HAVE_VISIBILITY)
#   define HPX_SYMBOL_EXPORT      __attribute__((visibility("default")))
#   define HPX_SYMBOL_IMPORT      __attribute__((visibility("default")))
#   define HPX_SYMBOL_INTERNAL    __attribute__((visibility("hidden")))
# else
#   define HPX_SYMBOL_EXPORT      /* empty */
#   define HPX_SYMBOL_IMPORT      /* empty */
#   define HPX_SYMBOL_INTERNAL    /* empty */
# endif

///////////////////////////////////////////////////////////////////////////////
// define the export/import helper macros used by the runtime module
# if defined(HPX_EXPORTS)
#   define  HPX_EXPORT             HPX_SYMBOL_EXPORT
#   define  HPX_EXCEPTION_EXPORT   HPX_SYMBOL_EXPORT
# else
#   define  HPX_EXPORT             HPX_SYMBOL_IMPORT
#   define  HPX_EXCEPTION_EXPORT   HPX_SYMBOL_IMPORT
# endif

///////////////////////////////////////////////////////////////////////////////
// define the export/import helper macros to be used for component modules
# if defined(HPX_COMPONENT_EXPORTS)
#   define  HPX_COMPONENT_EXPORT   HPX_SYMBOL_EXPORT
# else
#   define  HPX_COMPONENT_EXPORT   HPX_SYMBOL_IMPORT
# endif

#endif
