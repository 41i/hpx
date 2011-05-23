////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Bryce Lelbach
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////

#if !defined(HPX_BDD56092_8F07_4D37_9987_37D20A1FEA21)
#define HPX_BDD56092_8F07_4D37_9987_37D20A1FEA21

#include <boost/optional.hpp>
#include <boost/preprocessor/stringize.hpp>

#include <boost/fusion/include/vector.hpp>
#include <boost/fusion/include/at_c.hpp>

#include <hpx/config.hpp>
#include <hpx/hpx_fwd.hpp>
#include <hpx/exception.hpp>
#include <hpx/util/serialize_sequence.hpp>
#include <hpx/runtime/components/component_type.hpp>
#include <hpx/runtime/components/server/fixed_component_base.hpp>
#include <hpx/runtime/components/server/simple_component_base.hpp>
#include <hpx/runtime/agas/traits.hpp>
#include <hpx/runtime/agas/database/table.hpp>
#include <hpx/runtime/agas/network/gva.hpp>

namespace hpx { namespace agas { namespace server
{

template <typename Database, typename Protocol>
struct HPX_COMPONENT_EXPORT primary_namespace
{
    // {{{ nested types
    typedef typename traits::database::mutex_type<Database>::type
        database_mutex_type;

    typedef typename traits::network::endpoint_type<Protocol>::type
        endpoint_type;

    typedef gva<Protocol> gva_type;
    typedef typename gva_type::count_type count_type;
    typedef typename gva_type::offset_type offset_type;
    typedef boost::uint32_t component_type;
    typedef boost::uint32_t prefix_type;
    typedef std::vector<prefix_type> prefixes_type;

    typedef boost::fusion::vector2<count_type, component_type>
        decrement_type;

    typedef boost::fusion::vector2<prefix_type, naming::gid_type>
        partition_type;

    typedef boost::fusion::vector4<
        naming::gid_type, naming::gid_type, naming::gid_type, bool
    > binding_type;

    typedef boost::optional<gva_type> unbinding_type;

    typedef boost::fusion::vector2<naming::gid_type, gva_type>
        locality_type;

    typedef table<Database, naming::gid_type, gva_type>
        gva_table_type; 

    typedef table<Database, endpoint_type, partition_type>
        partition_table_type;
    
    typedef table<Database, naming::gid_type, count_type>
        refcnt_table_type;
    // }}}
 
  private:
    database_mutex_type mutex_;
    gva_table_type gvas_;
    partition_table_type partitions_;
    refcnt_table_type refcnts_;
  
  public:
    primary_namespace_base(std::string const& name)
      : mutex_(),
        gvas_(std::string("hpx.agas.") + name + ".gva"),
        partitions_(std::string("hpx.agas.") + name + ".partition"),
        refcnts_(std::string("hpx.agas.") + name +".refcnt")
    { traits::initialize_mutex(mutex_); }

    binding_type bind_locality(endpoint_type const& ep, count_type count)
    { // {{{ bind_locality implementation
        using boost::fusion::at_c;

        typename database_mutex_type::scoped_lock l(mutex_);

        // Load the partition table 
        typename partition_table_type::map_type&
            partition_table = partitions_.get();

        typename partition_table_type::map_type::iterator
            it = partition_table.find(ep),
            end = partition_table.end(); 

        count_type const real_count = (count) ? (count - 1) : (0);

        // If the endpoint is in the table, then this is a resize.
        if (it != end)
        {
            // Just return the prefix
            if (count == 0)
              return binding_type(at_c<1>(it->second), at_c<1>(it->second),
                  naming::get_gid_from_prefix(at_c<0>(it->second)), false);

            // Compute the new allocation.
            naming::gid_type lower(at_c<1>(it->second) + 1),
                             upper(lower + real_count);

            // Check for overflow.
            if (upper.get_msb() != lower.get_msb())
            {
                // Check for address space exhaustion 
                if (HPX_UNLIKELY((lower.get_msb() & ~0xFFFFFFFF) == 0xFFFFFFF))
                {
                    HPX_THROW_IN_CURRENT_FUNC(internal_server_error, 
                        "primary namespace has been exhausted");
                }

                // Otherwise, correct
                lower = naming::gid_type(upper.get_msb(), 0);
                upper = lower + real_count; 
            }
            
            // Store the new upper bound.
            at_c<1>(it->second) = upper;

            // Set the initial credit count.
            naming::set_credit_for_gid(lower, HPX_INITIAL_GLOBALCREDIT);
            naming::set_credit_for_gid(upper, HPX_INITIAL_GLOBALCREDIT); 

            return binding_type(lower, upper,
                naming::get_gid_from_prefix(at_c<0>(it->second)), false);
        }

        // If the endpoint isn't in the table, then we're registering it.
        else
        {
            // Check for address space exhaustion.
            if (HPX_UNLIKELY(partition_table.size() > 0xFFFFFFFE))
            {
                HPX_THROW_IN_CURRENT_FUNC(internal_server_error, 
                    "primary namespace has been exhausted");
            }

            // Compute the locality's prefix
            boost::uint32_t prefix = static_cast<boost::uint32_t>
                (partition_table.size() + 1);
            naming::gid_type id(naming::get_gid_from_prefix(prefix));

            // Start assigning ids with the second block of 64bit numbers only
            naming::gid_type lower_id(id.get_msb() + 1, 0);

            std::pair<typename partition_table_type::map_type::iterator, bool>
                pit = partition_table.insert(typename
                    partition_table_type::map_type::value_type
                        (ep, partition_type(prefix, lower_id)));

            // REVIEW: Should this be an assertion?
            // Check for an insertion failure. If this branch is triggered, then
            // the partition table was updated at some point after we first
            // checked it, which would indicate memory corruption or a locking
            // failure.
            if (HPX_UNLIKELY(!pit.second))
            {
                HPX_THROW_IN_CURRENT_FUNC(internal_server_error, 
                    "insertion failed due to memory corruption or a locking "
                    "error");
            }

            // Load the GVA table. Now that we've inserted the locality into
            // the partition table successfully, we need to put the locality's
            // GID into the GVA table so that parcels can be sent to the memory
            // of a locality.
            typename gva_table_type::map_type& gva_table = gvas_.get();

            std::pair<typename gva_table_type::map_type::iterator, bool>
                git = gva_table.insert(typename
                    gva_table_type::map_type::value_type
                        (id, gva_type
                            (ep, components::component_runtime_support, count)));

            // REVIEW: Should this be an assertion?
            // Check for insertion failure.
            if (HPX_UNLIKELY(!git.second))
            {
                // REVIEW: Is this the right error code to use?
                HPX_THROW_IN_CURRENT_FUNC(no_success, 
                    "insertion failed due to memory corruption or a locking "
                    "error");
            }

            if (count != 0)
            {
                // Generate the requested GID range
                naming::gid_type lower = lower_id + 1;
                naming::gid_type upper = lower + real_count;

                at_c<1>((*pit.first).second) = upper;

                // Set the initial credit count.
                naming::set_credit_for_gid(lower, HPX_INITIAL_GLOBALCREDIT);
                naming::set_credit_for_gid(upper, HPX_INITIAL_GLOBALCREDIT); 

                return binding_type(lower, upper, id, true);
            }

            return binding_type
                (naming::invalid_gid, naming::invalid_gid, id, true);
        }
    } // }}}

    bool bind_gid(naming::gid_type const& gid, gva_type const& gva)
    { // {{{ bind_gid implementation
        using boost::fusion::at_c;

        // TODO: Implement and use a non-mutating version of
        // strip_credit_from_gid()
        naming::gid_type id = gid;
        naming::strip_credit_from_gid(id); 

        typename database_mutex_type::scoped_lock l(mutex_);

        // Load the GVA table 
        typename gva_table_type::map_type& gva_table = gvas_.get();

        typename gva_table_type::map_type::iterator
            it = gva_table.lower_bound(id),
            begin = gva_table.begin(),
            end = gva_table.end();

        if (it != end)
        {
            // If we got an exact match, this is a request to update an existing
            // locality binding (e.g. move semantics).
            if (it->first == id)
            {
                // Check for count mismatch (we can't change block sizes of
                // existing bindings).
                if (it->second.count != gva.count)
                {
                    // REVIEW: Is this the right error code to use?
                    HPX_THROW_IN_CURRENT_FUNC(bad_parameter, 
                        "cannot change block size of existing binding");
                }

                // Store the new endpoint and offset
                it->second.endpoint = gva.endpoint;
                it->second.type = gva.type;
                it->second.lva(gva.lva());
                it->second.offset = gva.offset;
                return false;
            }

            // We're about to decrement it, so make sure it's safe to do so.
            else if (it != begin)
            {
                --it;

                // Check that a previous range doesn't cover the new id.
                if ((it->first + it->second.count) > id)
                {
                    // REVIEW: Is this the right error code to use?
                    HPX_THROW_IN_CURRENT_FUNC(bad_parameter, 
                        "the new GID is contained in an existing range");
                }
            }
        }            

        else if (HPX_LIKELY(!gva_table.empty()))
        {
            --it;
            // Check that a previous range doesn't cover the new id.
            if ((it->first + it->second.count) > id)
            {
                // REVIEW: Is this the right error code to use?
                HPX_THROW_IN_CURRENT_FUNC(bad_parameter, 
                    "the new GID is contained in an existing range");
            }
        }

        naming::gid_type upper_bound(id + (gva.count - 1));

        if (HPX_UNLIKELY(id.get_msb() != upper_bound.get_msb()))
        {
            HPX_THROW_IN_CURRENT_FUNC(internal_server_error, 
                "MSBs of lower and upper range bound do not match");
        }

        std::pair<typename gva_table_type::map_type::iterator, bool>
            p = gva_table.insert(typename gva_table_type::map_type::value_type
                (id, gva));
        
        // REVIEW: Should this be an assertion?
        // Check for an insertion failure. 
        if (HPX_UNLIKELY(!p.second))
        {
            HPX_THROW_IN_CURRENT_FUNC(internal_server_error, 
                "insertion failed due to memory corruption or a locking "
                "error");
        }

        return true;
    } // }}}

    locality_type resolve_locality(endpoint_type const& ep) 
    { // {{{ resolve_endpoint implementation
        using boost::fusion::at_c;

        typename database_mutex_type::scoped_lock l(mutex_);

        // Load the partition table 
        typename partition_table_type::map_type const&
            partition_table = partitions_.get();

        typename partition_table_type::map_type::const_iterator
            pit = partition_table.find(ep),
            pend = partition_table.end(); 

        if (pit != pend)
        {
            naming::gid_type id
                = naming::get_gid_from_prefix(at_c<0>(pit->second));
                
            // Load the GVA table 
            typename gva_table_type::map_type const& gva_table = gvas_.get();
    
            typename gva_table_type::map_type::const_iterator
                git = gva_table.lower_bound(id),
                gbegin = gva_table.begin(),
                gend = gva_table.end();
    
            if (git != gend)
            {
                // Check for exact match
                if (git->first == id)
                    return locality_type(git->first, git->second);
    
                // We need to decrement the iterator, check that it's safe to do
                // so.
                else if (git != gbegin)
                {
                    --git;
    
                    // Found the GID in a range
                    if ((git->first + git->second.count) > id)
                    {
                        if (HPX_UNLIKELY(id.get_msb() != git->first.get_msb()))
                        {
                            HPX_THROW_IN_CURRENT_FUNC(internal_server_error, 
                                "MSBs of lower and upper range bound do not match");
                        }
     
                        // Calculation of the lva address occurs in gva<>::resolve()
                        return locality_type
                            (git->first, git->second.resolve(id, git->first));
                    }
                }
            }
    
            else if (HPX_LIKELY(!gva_table.empty()))
            {
                --git;
    
                // Found the GID in a range
                if ((git->first + git->second.count) > id)
                {
                    if (HPX_UNLIKELY(id.get_msb() != git->first.get_msb()))
                    {
                        HPX_THROW_IN_CURRENT_FUNC(internal_server_error, 
                            "MSBs of lower and upper range bound do not match");
                    }
     
                    // Calculation of the local address occurs in gva<>::resolve()
                    return locality_type
                        (git->first, git->second.resolve(id, git->first));
                }
            }
        }

        return locality_type();
    } // }}}

    gva_type resolve_gid(naming::gid_type const& gid) 
    { // {{{ resolve_gid implementation 
        // TODO: Implement and use a non-mutating version of
        // strip_credit_from_gid()
        naming::gid_type id = gid;
        naming::strip_credit_from_gid(id); 
            
        typename database_mutex_type::scoped_lock l(mutex_);
        
        // Load the GVA table 
        typename gva_table_type::map_type const& gva_table = gvas_.get();

        typename gva_table_type::map_type::const_iterator
            it = gva_table.lower_bound(id),
            begin = gva_table.begin(),
            end = gva_table.end();

        if (it != end)
        {
            // Check for exact match
            if (it->first == id)
                return it->second;

            // We need to decrement the iterator, check that it's safe to do
            // so.
            else if (it != begin)
            {
                --it;

                // Found the GID in a range
                if ((it->first + it->second.count) > id)
                {
                    if (HPX_UNLIKELY(id.get_msb() != it->first.get_msb()))
                    {
                        HPX_THROW_IN_CURRENT_FUNC(internal_server_error, 
                            "MSBs of lower and upper range bound do not match");
                    }
 
                    // Calculation of the lva address occurs in gva<>::resolve()
                    return it->second.resolve(id, it->first);
                }
            }
        }

        else if (HPX_LIKELY(!gva_table.empty()))
        {
            --it;

            // Found the GID in a range
            if ((it->first + it->second.count) > id)
            {
                if (HPX_UNLIKELY(id.get_msb() != it->first.get_msb()))
                {
                    HPX_THROW_IN_CURRENT_FUNC(internal_server_error, 
                        "MSBs of lower and upper range bound do not match");
                }
 
                // Calculation of the local address occurs in gva<>::resolve()
                return it->second.resolve(id, it->first);
            }
        }

        return gva_type();
    } // }}}

    unbinding_type unbind(naming::gid_type const& gid, count_type count)
    { // {{{ unbind implementation
        // TODO: Implement and use a non-mutating version of
        // strip_credit_from_gid()
        naming::gid_type id = gid;
        naming::strip_credit_from_gid(id); 
        
        typename database_mutex_type::scoped_lock l(mutex_);
        
        // Load the GVA table 
        typename gva_table_type::map_type& gva_table = gvas_.get();

        typename gva_table_type::map_type::iterator
            it = gva_table.find(id),
            end = gva_table.end();

        if (it != end)
        {
            if (it->second.count != count)
            {
                HPX_THROW_IN_CURRENT_FUNC(bad_parameter, 
                    "block sizes must match");
            }

            unbinding_type ep(it->second);
            gva_table.erase(it);
            return ep;
        }

        else
            return unbinding_type();       
    } // }}}

    count_type increment(naming::gid_type const& gid, count_type credits)
    { // {{{ increment implementation
        // TODO: Implement and use a non-mutating version of
        // strip_credit_from_gid()
        naming::gid_type id = gid;
        naming::strip_credit_from_gid(id); 
        
        typename database_mutex_type::scoped_lock l(mutex_);

        typename refcnt_table_type::map_type& refcnt_table = refcnts_.get();

        typename refcnt_table_type::map_type::iterator
            it = refcnt_table.find(id),
            end = refcnt_table.end();

        // See if this is the first increment request for this GID
        if (it == end)
        {
            std::pair<typename refcnt_table_type::map_type::iterator, bool>
                p = refcnt_table.insert(typename
                    refcnt_table_type::map_type::value_type
                        (id, HPX_INITIAL_GLOBALCREDIT));

            if (HPX_UNLIKELY(!p.second))
            {
                HPX_THROW_IN_CURRENT_FUNC(internal_server_error, 
                    "insertion failed due to memory corruption or a locking "
                    "error");
            }

            it = p.first;
        }
       
        // Add the requested amount and return the new total 
        return (it->second += credits);
    } // }}}
    
    decrement_type 
    decrement(naming::gid_type const& gid, count_type credits)
    { // {{{ decrement implementation
        // TODO: Implement and use a non-mutating version of
        // strip_credit_from_gid()
        naming::gid_type id = gid;
        naming::strip_credit_from_gid(id); 

        if (HPX_UNLIKELY(credits <= HPX_INITIAL_GLOBALCREDIT))
        {
            HPX_THROW_IN_CURRENT_FUNC(bad_parameter, 
                "cannot decrement more than "
                BOOST_PP_STRINGIZE(HPX_INITIAL_GLOBALCREDIT)
                " credits");
        } 
        
        typename database_mutex_type::scoped_lock l(mutex_);
        
        // Load the reference count table    
        typename refcnt_table_type::map_type& refcnt_table = refcnts_.get();

        typename refcnt_table_type::map_type::iterator
            it = refcnt_table.find(id),
            end = refcnt_table.end();

        if (it != end)
        {
            if (HPX_UNLIKELY(it->second < credits))
            {
                HPX_THROW_IN_CURRENT_FUNC(bad_parameter, 
                    "bogus credit encountered while decrement global reference "
                    "count");
            }

            count_type cnt = (it->second -= credits);
            
            if (0 == cnt)
            {
                refcnt_table.erase(it);

                // Load the GVA table 
                typename gva_table_type::map_type& gva_table = gvas_.get();

                typename gva_table_type::map_type::iterator
                    git = gva_table.lower_bound(id),
                    gbegin = gva_table.begin(),
                    gend = gva_table.end();

                if (git != gend)
                {
                    // Did we get an exact match?
                    if (git->first == id)
                        return decrement_type(cnt, git->second.type);

                    // Check if we can safely decrement the iterator.
                    else if (git != gbegin)
                    {
                        --git;

                        // See if the previous range covers this GID
                        if ((git->first + git->second.count) > id)
                        {
                            // Make sure that the msbs match
                            if (id.get_msb() == git->first.get_msb())
                                return decrement_type
                                    (cnt, git->second.type);
                        } 
                    }
                }
                
                else if (HPX_LIKELY(!gva_table.empty()))
                {
                    --git;

                    // See if the previous range covers this GID
                    if ((git->first + git->second.count) > id)
                    {
                        // Make sure that the msbs match
                        if (id.get_msb() == git->first.get_msb())
                            return decrement_type(cnt, git->second.type);
                    } 
                }

                // If we didn't find anything, we've got a problem
                HPX_THROW_IN_CURRENT_FUNC(bad_parameter, 
                    "unknown component type encountered while decrement global "
                    "reference count");
            }

            else
                return decrement_type(cnt, components::component_invalid);
        }
        
        // We need to insert a new reference count entry. We assume that
        // binding has already created a first reference + credits.
        BOOST_ASSERT(credits < HPX_INITIAL_GLOBALCREDIT);

        std::pair<typename refcnt_table_type::map_type::iterator, bool>
            p = refcnt_table.insert(typename
                refcnt_table_type::map_type::value_type
                    (id, HPX_INITIAL_GLOBALCREDIT));

        if (HPX_UNLIKELY(!p.second))
        {
            HPX_THROW_IN_CURRENT_FUNC(internal_server_error, 
                "insertion failed due to memory corruption or a locking "
                "error");
        }
        
        it = p.first;
        
        if (HPX_UNLIKELY(it->second < credits))
        {
            HPX_THROW_IN_CURRENT_FUNC(bad_parameter, 
                "bogus credit encountered while decrement global reference "
                "count");
        }
        
        count_type cnt = (it->second -= credits);
        
        BOOST_ASSERT(0 != cnt);
 
        return decrement_type(cnt, components::component_invalid);
    } // }}}
 
    prefixes_type localities()
    { // {{{ localities implementation
        using boost::fusion::at_c;

        prefixes_type prefixes;

        typename database_mutex_type::scoped_lock l(mutex_);

        // Load the partition table 
        typename partition_table_type::map_type const&
            partition_table = partitions_.get();

        prefixes.reserve(partition_table.size());

        typename partition_table_type::map_type::const_iterator
            it = partition_table.begin(),
            end = partition_table.end(); 

        for (; it != end; ++it)
            prefixes.push_back(at_c<0>(it->second));

        return prefixes;
    } // }}}

    enum actions 
    { // {{{ action enum
        namespace_bind_locality,
        namespace_bind_gid,
        namespace_resolve_locality,
        namespace_resolve_gid,
        namespace_unbind,
        namespace_increment,
        namespace_decrement,
        namespace_localities
    }; // }}}

    template <typename Derived> 
    struct action_types
    { // {{{ action rebinder
        typedef hpx::actions::result_action2<
            Derived,
            /* return type */ binding_type,
            /* enum value */  namespace_bind_locality,
            /* arguments */   endpoint_type const&, count_type,
            &Derived::bind_locality
        > bind_locality; 
       
        typedef hpx::actions::result_action2<
            Derived,
            /* return type */ bool,
            /* enum value */  namespace_bind_gid,
            /* arguments */   naming::gid_type const&, gva_type const&,
            &Derived::bind_gid
        > bind_gid;
        
        typedef hpx::actions::result_action1<
            Derived,
            /* return type */ locality_type,
            /* enum value */  namespace_resolve_locality,
            /* arguments */   endpoint_type const&,
            &Derived::resolve_locality
        > resolve_locality;
    
        typedef hpx::actions::result_action1<
            Derived,
            /* return type */ gva_type,
            /* enum value */  namespace_resolve_gid,
            /* arguments */   naming::gid_type const&,
            &Derived::resolve_gid
        > resolve_gid;
        
        typedef hpx::actions::result_action2<
            Derived,
            /* return type */ unbinding_type,
            /* enum value */  namespace_unbind,
            /* arguments */   naming::gid_type const&, count_type,
            &Derived::unbind
        > unbind;
        
        typedef hpx::actions::result_action2<
            Derived,
            /* return type */ count_type,  
            /* enum value */  namespace_increment,
            /* arguments */   naming::gid_type const&, count_type,
            &Derived::increment
        > increment;
        
        typedef hpx::actions::result_action2<
            Derived,
            /* return type */ decrement_type,
            /* enum value */  namespace_decrement,
            /* arguments */   naming::gid_type const&, count_type,
            &Derived::decrement
        > decrement;
        
        typedef hpx::actions::result_action0<
            Derived,
            /* return type */ prefixes_type,
            /* enum value */  namespace_localities,
            &Derived::localities
        > localities;
    }; // }}}
};

template <typename Database, typename Protocol>
struct HPX_COMPONENT_EXPORT bootstrap_primary_namespace
  : components::fixed_component_base<
      0x0000000100000001ULL, 0x0000000000000001ULL, // constant GID
      primary_namespace<Database, Protocol> >,
    primary_namespace_base<Database, Protocol>
{
    typedef primary_namespace_base<Database, Protocol> base_type;

    typedef typename base_type::template
      action_types<bootstrap_primary_namespace> bound_action_types;

    typedef typename bound_action_types::bind_locality bind_locality_action;
    typedef typename bound_action_types::bind_gid bind_gid_action;
    typedef typename bound_action_types::resolve_locality resolve_locality_action;
    typedef typename bound_action_types::resolve_gid resolve_gid_action;
    typedef typename bound_action_types::unbind unbind_action;
    typedef typename bound_action_types::increment increment_action;
    typedef typename bound_action_types::decrement decrement_action;
    typedef typename bound_action_types::localities localities_action;

    bootstrap_primary_namespace():
      base_type("bootstrap_primary_namespace") {} 
};

template <typename Database, typename Protocol>
struct HPX_COMPONENT_EXPORT primary_namespace
  : components::simple_component_base<primary_namespace<Database, Protocol> >,
    primary_namespace_base<Database, Protocol>
{
    typedef primary_namespace_base<Database, Protocol> base_type;

    typedef typename base_type::template
      action_types<primary_namespace> bound_action_types;
    
    typedef typename bound_action_types::bind_locality bind_locality_action;
    typedef typename bound_action_types::bind_gid bind_gid_action;
    typedef typename bound_action_types::resolve_locality resolve_locality_action;
    typedef typename bound_action_types::resolve_gid resolve_gid_action;
    typedef typename bound_action_types::unbind unbind_action;
    typedef typename bound_action_types::increment increment_action;
    typedef typename bound_action_types::decrement decrement_action;
    typedef typename bound_action_types::localities localities_action;

    primary_namespace(): base_type("primary_namespace") {} 
};

}}}

#endif // HPX_BDD56092_8F07_4D37_9987_37D20A1FEA21

