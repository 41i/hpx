//  Copyright (c) 2007-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx_fwd.hpp>
#include <hpx/util/itt_notify.hpp>

#if defined(HPX_USE_ITT)

#include <ittnotify.h>
#include <internal/ittnotify.h>

///////////////////////////////////////////////////////////////////////////////
#define HPX_INTERNAL_ITT_SYNC_CREATE(obj, type, name)                         \
    if (__itt_sync_createA_ptr) {                                             \
        __itt_sync_createA_ptr(                                               \
            const_cast<void*>(static_cast<volatile void*>(obj)),              \
                type, name, __itt_attr_mutex);                                \
    }                                                                         \
    /**/
#define HPX_INTERNAL_ITT_SYNC(fname, obj)                                     \
    if (__itt_ ## fname ## _ptr) {                                            \
        __itt_ ## fname ## _ptr(                                              \
            const_cast<void*>(static_cast<volatile void*>(obj)));             \
    }                                                                         \
    /**/
#define HPX_INTERNAL_ITT_SYNC_RENAME(obj, name)                               \
    if (__itt_sync_renameA_ptr) {                                             \
        __itt_sync_renameA_ptr(                                               \
            const_cast<void*>(static_cast<volatile void*>(obj)), name);       \
    }                                                                         \
    /**/

#define HPX_INTERNAL_ITT_THREAD_SET_NAME(name)                                \
    if (__itt_thread_set_name_ptr) __itt_thread_set_name_ptr(name)            \
    /**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_INTERNAL_STACK_CREATE()                                           \
    __itt_stack_caller_create_ptr ?                                           \
        __itt_stack_caller_create_ptr() : (__itt_caller)0                     \
    /**/
#define HPX_INTERNAL_STACK_ENTER(ctx)                                         \
    if (__itt_stack_callee_enter_ptr) { __itt_stack_callee_enter_ptr(ctx); }  \
    /**/
#define HPX_INTERNAL_STACK_LEAVE(ctx)                                         \
    if (__itt_stack_callee_leave_ptr) { __itt_stack_callee_leave_ptr(ctx); }  \
    /**/
#define HPX_INTERNAL_STACK_DESTROY(ctx)                                       \
    if (__itt_stack_caller_destroy_ptr && ctx != (__itt_caller)0) {           \
        __itt_stack_caller_destroy_ptr(ctx);                                  \
    }                                                                         \
    /**/

///////////////////////////////////////////////////////////////////////////////
#if defined(BOOST_MSVC) \
    || defined(__BORLANDC__) \
    || (defined(__MWERKS__) && defined(_WIN32) && (__MWERKS__ >= 0x3000)) \
    || (defined(__ICL) && defined(_MSC_EXTENSIONS) && (_MSC_VER >= 1200))

#if HPX_ITT_AMPLIFIER != 0
#pragma comment(lib, "libittnotify_amplifier.lib")
#else
#pragma comment(lib, "libittnotify_inspector.lib")
#endif
#endif

///////////////////////////////////////////////////////////////////////////////
#define HPX_INTERNAL_ITT_SYNC_PREPARE(obj)           HPX_INTERNAL_ITT_SYNC(sync_prepare, obj)
#define HPX_INTERNAL_ITT_SYNC_CANCEL(obj)            HPX_INTERNAL_ITT_SYNC(sync_cancel, obj)
#define HPX_INTERNAL_ITT_SYNC_ACQUIRED(obj)          HPX_INTERNAL_ITT_SYNC(sync_acquired, obj)
#define HPX_INTERNAL_ITT_SYNC_RELEASING(obj)         HPX_INTERNAL_ITT_SYNC(sync_releasing, obj)
#define HPX_INTERNAL_ITT_SYNC_RELEASED(obj)          ((void)0) //HPX_INTERNAL_ITT_SYNC(sync_released, obj)
#define HPX_INTERNAL_ITT_SYNC_DESTROY(obj)           HPX_INTERNAL_ITT_SYNC(sync_destroy, obj)

///////////////////////////////////////////////////////////////////////////////
void itt_sync_create(void *addr, const char* objtype, const char* objname)
{
    HPX_INTERNAL_ITT_SYNC_CREATE(addr, objtype, objname);
}

void itt_sync_rename(void* addr, const char* objname)
{
    HPX_INTERNAL_ITT_SYNC_RENAME(addr, objname);
}

void itt_sync_prepare(void* addr)
{
    HPX_INTERNAL_ITT_SYNC_PREPARE(addr);
}

void itt_sync_acquired(void* addr)
{
    HPX_INTERNAL_ITT_SYNC_ACQUIRED(addr);
}

void itt_sync_cancel(void* addr)
{
    HPX_INTERNAL_ITT_SYNC_CANCEL(addr);
}

void itt_sync_releasing(void* addr)
{
    HPX_INTERNAL_ITT_SYNC_RELEASING(addr);
}

void itt_sync_released(void* addr)
{
    HPX_INTERNAL_ITT_SYNC_RELEASED(addr);
}

void itt_sync_destroy(void* addr)
{
    HPX_INTERNAL_ITT_SYNC_DESTROY(addr);
}

///////////////////////////////////////////////////////////////////////////////
__itt_caller itt_stack_create()
{
    return HPX_INTERNAL_STACK_CREATE();
}

void itt_stack_enter(__itt_caller ctx)
{
    HPX_INTERNAL_STACK_ENTER(ctx);
}

void itt_stack_leave(__itt_caller ctx)
{
    HPX_INTERNAL_STACK_LEAVE(ctx);
}

void itt_stack_destroy(__itt_caller ctx)
{
    HPX_INTERNAL_STACK_DESTROY(ctx);
}

#endif // HPX_USE_ITT

