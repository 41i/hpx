//  Copyright (c) 2007-2008 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_RUNTIME_ACTIONS_CONTINUATION_JUN_13_2008_1031AM)
#define HPX_RUNTIME_ACTIONS_CONTINUATION_JUN_13_2008_1031AM

#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/serialization/serialization.hpp>

#include <hpx/hpx_fwd.hpp>
#include <hpx/exception.hpp>
#include <hpx/runtime/naming/name.hpp>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace actions
{
    // parcel continuations are simply lists of global ids of LCO's to call 
    // set_event on
    class HPX_EXPORT continuation
    {
    public:
        continuation()
        {}

        explicit continuation(naming::id_type const& gid)
        {
            gids_.push_back(gid);
        }

        explicit continuation(std::vector<naming::id_type> const& gids)
          : gids_(gids)
        {}

        virtual ~continuation() {}

        bool empty() const
        {
            return gids_.empty();
        }

        ///
        void trigger_all(applier::applier& app);

        ///
        template <typename Arg0>
        void trigger_all(applier::applier& app, Arg0 const& arg0);

        ///
        void trigger_error(applier::applier& app, hpx::exception const& e);

    private:
        // serialization support    
        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive& ar, const unsigned int /*version*/)
        {
            ar & gids_;
        }

        std::vector<naming::id_type> gids_;
    };

}}

#include <hpx/config/warnings_suffix.hpp>

#endif
