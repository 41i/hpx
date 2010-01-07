//  Copyright (c) 2007-2009 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_COMPONENTS_CLIENT_BASE_OCT_31_2008_0424PM)
#define HPX_COMPONENTS_CLIENT_BASE_OCT_31_2008_0424PM

#include <hpx/hpx_fwd.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace components 
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Derived, typename Stub>
    class client_base : public Stub
    {
    protected:
        typedef Stub stub_type;

    public:
        client_base(naming::id_type gid, bool freeonexit = false)
          : gid_(gid), freeonexit_(freeonexit)
        {}

        ~client_base() 
        {
            if (freeonexit_ && gid_)
                this->stub_type::free(gid_);
        }

        ///////////////////////////////////////////////////////////////////////
        /// Create a new instance of an distributing_factory on the locality as 
        /// given by the parameter \a targetgid
        static Derived 
        create(naming::id_type const& targetgid, component_type type,
            std::size_t count = 1, bool freeonexit = false)
        {
            return Derived(stub_type::create(targetgid, type, count), freeonexit);
        }

        static Derived 
        create(naming::id_type const& targetgid, std::size_t count = 1, 
            bool freeonexit = false)
        {
            return Derived(stub_type::create(targetgid, count), freeonexit);
        }

        void free(component_type type)
        {
            if (gid_) {
                this->stub_type::free(type, gid_);
                gid_ = naming::invalid_id;
            }
        }

        void free()
        {
            if (gid_) {
                this->stub_type::free(gid_);
                gid_ = naming::invalid_id;
            }
        }

        ///////////////////////////////////////////////////////////////////////
        naming::id_type const& get_gid() const
        {
            return gid_;
        }

        naming::id_type detach() 
        {
            naming::id_type g = gid_;
            gid_ = naming::invalid_id;
            return g;
        }

    protected:
        naming::id_type gid_;
        bool freeonexit_;
    };

}}

#endif

