//  Copyright (c) 2007-2009 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_COMPONENTS_SIMPLE_COMPONENT_BASE_JUL_18_2008_0948PM)
#define HPX_COMPONENTS_SIMPLE_COMPONENT_BASE_JUL_18_2008_0948PM

#include <hpx/runtime/naming/name.hpp>
#include <hpx/runtime/naming/address.hpp>
#include <hpx/runtime/naming/full_address.hpp>

namespace hpx { namespace components 
{
    ///////////////////////////////////////////////////////////////////////////
    /// \class simple_component_base simple_component_base.hpp hpx/runtime/components/server/simple_component_base.hpp
    ///
    template <typename Component = detail::this_type>
    class simple_component_base : public detail::simple_component_tag
    {
    private:
        typedef typename boost::mpl::if_<
                boost::is_same<Component, detail::this_type>, 
                simple_component_base, Component
            >::type this_component_type;

        static component_type value;

    public:
        typedef this_component_type wrapped_type;

        /// \brief Construct an empty simple_component
        simple_component_base() 
        {}

        /// \brief Destruct a simple_component
        ~simple_component_base()
        {
            if (gid_)
                hpx::applier::get_applier().get_agas_client().unbind(gid_);
        }

        /// \brief finalize() will be called just before the instance gets 
        ///        destructed
        ///
        /// \param self [in] The PX \a thread used to execute this function.
        /// \param appl [in] The applier to be used for finalization of the 
        ///             component instance. 
        void finalize() {}

        // This is the component id. Every component needs to have an embedded
        // enumerator 'value' which is used by the generic action implementation
        // to associate this component with a given action.
        static component_type get_component_type()
        {
            return value;
        }
        static void set_component_type(component_type type)
        {
            value = type;
        }

        /// \brief Create a new GID (if called for the first time), assign this 
        ///        GID to this instance of a component and register this gid 
        ///        with the AGAS service
        ///
        /// \param appl   The applier instance to be used for accessing the 
        ///               AGAS service.
        ///
        /// \returns      The global id (GID)  assigned to this instance of a 
        ///               component
        naming::id_type const&
        get_gid() const
        {
            if (!gid_) 
            {
                applier::applier& appl = hpx::applier::get_applier();
                naming::address addr(appl.here(), 
                    this_component_type::get_component_type(), 
                    boost::uint64_t(static_cast<this_component_type const*>(this)));
                gid_ = appl.get_parcel_handler().get_next_id();
                if (!appl.get_agas_client().bind(gid_, addr))
                {
                    HPX_OSSTREAM strm;
                    strm << gid_;

                    gid_ = naming::id_type();   // invalidate GID

                    HPX_THROW_EXCEPTION(duplicate_component_address,
                        "simple_component_base<Component>::get_gid", 
                        HPX_OSSTREAM_GETSTRING(strm));
                }
            }
            return gid_;
        }

        /// \brief Create a new GID (if called for the first time), assign this 
        ///        GID to this instance of a component and register this gid 
        ///        with the AGAS service
        ///
        /// \param fa     [out] On return this will hold the full address of 
        ///               instance (global id (GID), local virtual address)
        ///
        /// \returns      This returns \a true if the para,eter \a fa is valid.
        bool get_full_address(naming::full_address& fa) const
        {
            fa.gid() = get_gid();
            applier::applier& appl = hpx::applier::get_applier();
            naming::address& addr = fa.addr();
            addr.locality_ = appl.here();
            addr.type_ = this_component_type::get_component_type();
            addr.address_ = 
                boost::uint64_t(static_cast<this_component_type const*>(this));
            return true;
        }

        /// \brief  The function \a get_factory_properties is used to 
        ///         determine, whether instances of the derived component can 
        ///         be created in blocks (i.e. more than one instance at once). 
        ///         This function is used by the \a distributing_factory to 
        ///         determine a correct allocation strategy
        static factory_property get_factory_properties()
        {
            // components derived from this template have to be allocated one
            // at a time
            return factory_none;
        }

    private:
        mutable naming::id_type gid_;
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename Component>
    component_type simple_component_base<Component>::value = component_invalid;


    ///////////////////////////////////////////////////////////////////////////
    /// \class simple_component simple_component.hpp hpx/runtime/components/server/simple_component_base.hpp
    ///
    template <typename Component>
    class simple_component : public Component
    {
    public:
        typedef Component type_holder;

        /// \brief  The function \a create is used for allocation and 
        ///         initialization of instances of the derived components.
        static Component* create(std::size_t count)
        {
            // simple components can be created individually only
            BOOST_ASSERT(1 == count);
            return new Component();
        }

        /// \brief  The function \a destroy is used for destruction and 
        ///         de-allocation of instances of the derived components.
        static void destroy(Component* p, std::size_t count = 1)
        {
            // simple components can be deleted individually only
            BOOST_ASSERT(1 == count);
            p->finalize();
            delete p;
        }

    };

}}

#endif
