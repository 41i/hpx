//  Copyright (c) 2007-2008 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_COMPONENTS_WRAPPER_JUN_04_2008_0901PM)
#define HPX_COMPONENTS_WRAPPER_JUN_04_2008_0901PM

#include <boost/throw_exception.hpp>
#include <boost/noncopyable.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/detail/atomic_count.hpp>
#include <boost/type_traits/is_same.hpp>

#include <hpx/exception.hpp>
#include <hpx/util/static.hpp>
#include <hpx/components/server/wrapper_heap.hpp>
#include <hpx/components/server/wrapper_heap_list.hpp>

namespace hpx { namespace components 
{
    namespace detail
    {
        ///////////////////////////////////////////////////////////////////////
        struct this_type {};

        ///////////////////////////////////////////////////////////////////////
        template <typename HasUseCount>
        struct wrapper_use_count;

        template <>
        struct wrapper_use_count<boost::mpl::false_> : boost::noncopyable
        {
        };

        template <>
        struct wrapper_use_count<boost::mpl::true_> : boost::noncopyable
        {
            wrapper_use_count() 
              : use_count_(0) 
            {}

            boost::detail::atomic_count use_count_;
        };
    }

    ///////////////////////////////////////////////////////////////////////////
    /// The wrapper template is used as a indirection layer for components 
    /// allowing to gracefully handle the access to non-existing components.
    template <
        typename Component, typename Derived = detail::this_type, 
        typename HasUseCount = boost::mpl::false_
    >
    class wrapper : public detail::wrapper_use_count<HasUseCount>
    {
    private:
        typedef typename boost::mpl::if_<
                boost::is_same<Derived, detail::this_type>, wrapper, Derived
            >::type derived_type;

    public:
        typedef Component wrapped_type;
        
        wrapper() 
          : component_(0) 
        {}

        // takes ownership of the passed pointer
        explicit wrapper(Component* c) 
          : component_(c) 
        {}

        ~wrapper()
        {
            delete component_;
        }

        Component* get()
        {
            if (0 == component_)
                boost::throw_exception(hpx::exception(invalid_status));
            return component_;
        }
        Component const* get() const
        {
            if (0 == component_)
                boost::throw_exception(hpx::exception(invalid_status));
            return component_;
        }

    protected:
        // the memory for the wrappers is managed by a one_size_heap_list
        typedef 
            detail::wrapper_heap_list<detail::fixed_wrapper_heap<wrapper> > 
        heap_type;

        struct wrapper_heap_tag {};

        static heap_type& get_heap()
        {
            // ensure thread-safe initialization
            static util::static_<heap_type, wrapper_heap_tag> heap;
            return heap.get();
        }

    public:
        /// The memory for wrapper objects is managed by a class specific 
        /// allocator 
        static void* operator new(std::size_t size)
        {
            if (size > sizeof(wrapper))
                return ::operator new(size);
            return get_heap().alloc();
        }
        static void operator delete(void* p, std::size_t size)
        {
            if (NULL == p) 
                return;     // do nothing if given a NULL pointer

            if (size != sizeof(wrapper)) {
                ::operator delete(p);
                return;
            }
            get_heap().free(p);
        }

        // Overload placement operators as well (the global placement operators
        // are hidden because of the new/delete overloads above)
        static void* operator new(std::size_t, void *p)
        {
            return p;
        }
        // delete if placement new fails
        static void operator delete(void*, void*)
        {}

        // The function create() is used for allocation and initialization of 
        // arrays of wrappers
        static wrapper* create(std::size_t count)
        {
            // allocate the memory
            wrapper* p = get_heap().alloc(count);
            if (1 == count)
                return new (p) derived_type;

            // call constructors
            std::size_t succeeded = 0;
            try {
                derived_type* curr = static_cast<derived_type*>(p);
                for (std::size_t i = 0; i < count; ++i, ++curr) {
                    new (curr) derived_type;     // call placement new, might throw
                    ++succeeded;
                }
            }
            catch (...) {
                // call destructors for successfully constructed objects
                derived_type* curr = static_cast<derived_type*>(p);
                for (std::size_t i = 0; i < succeeded; ++i)
                    curr->~derived_type();
                get_heap().free(p, count);     // free memory
                throw;      // rethrow
            }
            return p;
        }
        // The function destroy() is used for deletion and de-allocation of 
        // wrappers
        static void destroy(derived_type* p, std::size_t count)
        {
            if (NULL == p || 0 == count) 
                return;     // do nothing if given a NULL pointer

            if (1 == count) {
                p->~derived_type();
            }
            else {
                // call destructors for all wrapper instances
                derived_type* curr = static_cast<derived_type*>(p);
                for (std::size_t i = 0; i < count; ++i)
                    curr->~derived_type();
            }

            // free memory itself
            get_heap().free(p, count);
        }

    public:
        ///
        naming::id_type 
        get_gid(applier::applier& appl) const
        {
            return get_heap().get_gid(appl, const_cast<wrapper*>(this));
        }

        ///////////////////////////////////////////////////////////////////////
        // The wrapper behaves just like the wrapped object
        Component* operator-> ()
        {
            if (0 == component_)
                boost::throw_exception(hpx::exception(invalid_status));
            return component_;
        }

        Component const* operator-> () const
        {
            if (0 == component_)
                boost::throw_exception(hpx::exception(invalid_status));
            return component_;
        }

        ///////////////////////////////////////////////////////////////////////
        Component& operator* ()
        {
            if (0 == component_)
                boost::throw_exception(hpx::exception(invalid_status));
            return *component_;
        }

        Component const& operator* () const
        {
            if (0 == component_)
                boost::throw_exception(hpx::exception(invalid_status));
            return *component_;
        }

        /// \brief Return the type of the embedded component
        static components::component_type get_type() 
        {
            return static_cast<components::component_type>(Component::value);
        }

    private:
        Component* component_;
    };

    // support for boost::intrusive_ptr<wrapper<...> >
    template <typename Component, typename Derived, typename HasUseCount>
    inline void
    intrusive_ptr_add_ref(wrapper<Component, Derived, HasUseCount>* p)
    {
        ++p->use_count_;
    }

    template <typename Component, typename Derived, typename HasUseCount>
    inline void
    intrusive_ptr_release(wrapper<Component, Derived, HasUseCount>* p)
    {
        if (--p->use_count_ == 0)
            delete p;
    }

}}

#endif


