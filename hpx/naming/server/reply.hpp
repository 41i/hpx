//  Copyright (c) 2007-2008 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_NAMING_SERVER_REPLY_MAR_24_2008_0940AM)
#define HPX_NAMING_SERVER_REPLY_MAR_24_2008_0940AM

#include <vector>
#include <boost/asio.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/serialization.hpp>

#include <hpx/exception.hpp>
#include <hpx/naming/address.hpp>
#include <hpx/naming/server/request.hpp>

///////////////////////////////////////////////////////////////////////////////
///  version of GAS reply structure
#define HPX_REPLY_VERSION   0x30

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace naming { namespace server 
{
    ///////////////////////////////////////////////////////////////////////////
    namespace status_strings 
    {
        char const* const success = 
            "hpx: dgas: success";
        char const* const repeated_request = 
            "hpx: dgas: success (repeated request)";
        char const* const no_success = 
            "hpx: dgas: no success";
        char const* const internal_server_error = 
            "hpx: dgas: internal server error";
        char const* const service_unavailable = 
            "hpx: dgas: service unavailable";
        char const* const invalid_status = 
            "hpx: dgas: corrupted internal status";
        char const* const bad_parameter = 
            "hpx: dgas: bad parameter";
        char const* const bad_request = 
            "hpx: dgas: ill formatted request or unknown command";
        char const* const out_of_memory = 
            "hpx: dgas: internal server error (out of memory)";
        char const* const unknown_version = 
            "hpx: dgas: ill formatted request (unknown version)";

        inline char const* const get_error_text(error status)
        {
            switch (status) {
            case hpx::success:               return success;
            case hpx::no_success:            return no_success;
            case hpx::service_unavailable:   return service_unavailable;
            case hpx::invalid_status:        return invalid_status;
            case hpx::bad_parameter:         return bad_parameter;
            case hpx::bad_request:           return bad_request;
            case hpx::out_of_memory:         return out_of_memory;
            case hpx::version_unknown:       return unknown_version;
            case hpx::repeated_request:      return repeated_request;
            default:
                break;
            }
            return internal_server_error;
        }
    }
    
    ///////////////////////////////////////////////////////////////////////////
    /// a reply to be sent to a client
    class reply
    {
    public:
        reply (error s = no_success)
          : command_(command_unknown), status_(s),
            error_(status_strings::get_error_text(s)),
            lower_bound_(0), upper_bound_(0)
        {}

        reply (error s, char const* what)
          : command_(command_unknown), status_(s),
            error_(status_strings::get_error_text(s)),
            lower_bound_(0), upper_bound_(0)
        {
            error_ += std::string(": ") + what;
        }

        reply (name_server_command command, error s = success,
                char const* what = 0)
          : command_(command), status_(s), 
            error_(status_strings::get_error_text(s)),
            lower_bound_(0), upper_bound_(0)
        {
            if (0 != what)
                error_ += std::string(": ") + what;
        }

        reply (name_server_command command, naming::id_type const& id, 
               error s = success)
          : command_(command), status_(s),
            error_(status_strings::get_error_text(success)),
            lower_bound_(0), upper_bound_(0), id_(id)
        {
            BOOST_ASSERT(s == success || s == no_success);
            BOOST_ASSERT(command == command_queryid || 
                         command == command_registerid);
        }

        template <typename Container, typename F>
        reply (name_server_command command, Container const& totals, F f,
               error s = success)
          : command_(command), status_(s),
            error_(status_strings::get_error_text(success)),
            lower_bound_(0), upper_bound_(0)
        {
            BOOST_ASSERT(s == success || s == no_success);
            BOOST_ASSERT(command == command_statistics_count ||
                         command == command_statistics_mean ||
                         command == command_statistics_moment2);
            
            for (std::size_t i = 0; i < command_lastcommand; ++i)
                statistics_[i] = f(totals[i]);
        }

        reply (name_server_command command, naming::address addr)
          : command_(command), status_(success), 
            error_(status_strings::get_error_text(success)),
            address_(addr),
            lower_bound_(0), upper_bound_(0)
        {
            BOOST_ASSERT(command == command_resolve ||
                         command == command_unbind_range);
        }

        reply (error s, name_server_command command, 
                naming::id_type prefix)
          : command_(command_getprefix), status_(s),
            error_(status_strings::get_error_text(s)),
            lower_bound_(prefix), upper_bound_(0)
        {
            BOOST_ASSERT(s == success || s == repeated_request);
            BOOST_ASSERT(command == command_getprefix);
        }

        reply (error s, name_server_command command, 
                naming::id_type lower_bound, naming::id_type upper_bound)
          : command_(command_getidrange), status_(s),
            error_(status_strings::get_error_text(s)),
            lower_bound_(lower_bound), upper_bound_(upper_bound)
        {
            BOOST_ASSERT(s == success || s == repeated_request);
            BOOST_ASSERT(command == command_getidrange);
        }

        ///////////////////////////////////////////////////////////////////////
        error get_status() const
        {
            return (error)status_;
        }
        std::string get_error() const
        {
            return error_;
        }
        naming::address get_address() const
        {
            return address_;
        }
        naming::id_type const& get_prefix() const
        {
            return lower_bound_;
        }
        naming::id_type const& get_lower_bound() const
        {
            return lower_bound_;
        }
        naming::id_type const& get_upper_bound() const
        {
            return upper_bound_;
        }
        naming::id_type get_id() const
        {
            return id_;
        }
        
        double get_statictics(std::size_t i) const
        {
            if (i >= command_lastcommand)
                boost::throw_exception(hpx::exception(bad_parameter));
            return statistics_[i];
        }
        
    private:
        friend std::ostream& operator<< (std::ostream& os, reply const& rep);
        
        // serialization support    
        friend class boost::serialization::access;
    
        template<class Archive>
        void save(Archive & ar, const unsigned int version) const
        {
            ar << command_;
            ar << status_;
            ar << error_;
            
            switch (command_) {
            case command_resolve:
            case command_unbind_range:
                ar << address_.locality_;
                ar << address_.type_;
                ar << address_.address_;
                break;

            case command_getidrange:
                ar << upper_bound_;
                // fall through
            case command_getprefix:
                ar << lower_bound_;
                break;
                
            case command_queryid:
                ar << id_;
                break;

            case command_statistics_count:
            case command_statistics_mean:
            case command_statistics_moment2:
                for (std::size_t i = 0; i < command_lastcommand; ++i)
                    ar << statistics_[i];
                break;
                
            case command_bind_range:
            case command_registerid: 
            case command_unregisterid: 
            default:
                break;  // nothing additional to be sent
            }
        }

        template<class Archive>
        void load(Archive & ar, const unsigned int version)
        {
            if (version > HPX_REPLY_VERSION) {
                throw exception(version_too_new, 
                    "trying to load reply with unknown version");
            }
    
            ar >> command_;
            ar >> status_;
            ar >> error_;
            
            switch (command_) {
            case command_resolve:
            case command_unbind_range:
                ar >> address_.locality_;
                ar >> address_.type_;
                ar >> address_.address_;
                break;

            case command_getidrange:
                ar >> upper_bound_;
                // fall through
            case command_getprefix:
                ar >> lower_bound_;
                break;
                
            case command_queryid:
                ar >> id_;
                break;
                
            case command_statistics_count:
            case command_statistics_mean:
            case command_statistics_moment2:
                for (std::size_t i = 0; i < command_lastcommand; ++i)
                    ar >> statistics_[i];
                break;
                
            case command_bind_range:
            case command_registerid: 
            case command_unregisterid: 
            default:
                break;  // nothing additional to be received
            }
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER()

    private:
        boost::uint8_t command_;        /// the command this is a reply for
        boost::uint8_t status_;         /// status of requested operation
        std::string error_;             /// descriptive error message
        naming::address address_;       /// address (for resolve only)
        naming::id_type lower_bound_;   /// lower bound of id range (for get_idrange only) 
                                        /// or the prefix for the given locality (get_prefix only)
        naming::id_type upper_bound_;   /// upper bound of id range (for get_idrange only)
        naming::id_type id_;            /// global id (for queryid only)
        double statistics_[command_lastcommand];       /// gathered statistics
    };

    inline std::ostream& operator<< (std::ostream& os, reply const& rep)
    {
        os << get_command_name(rep.command_) << ": ";

        if (rep.status_ != success && rep.status_ != repeated_request) {
            os << "status(" << rep.error_ << ") ";
        }
        else {
            switch (rep.command_) {
            case command_resolve:
            case command_unbind_range:
                os << "addr(" << rep.address_ << ") ";
                break;

            case command_getidrange:
                os << "range(" << std::hex << rep.lower_bound_ << ", "
                   << rep.upper_bound_ << ") ";
                break;
                
                // fall through
            case command_getprefix:
                os << "prefix(" << std::hex<< rep.lower_bound_ << ") ";
                break;
                
            case command_queryid:
                os << "id" << std::hex << rep.id_ << " ";
                break;
                
            case command_statistics_count:
            case command_statistics_mean:
            case command_statistics_moment2:
                break;
                
            case command_bind_range:
            case command_registerid: 
            case command_unregisterid: 
            default:
                break;  // nothing additional to be received
            }
            if (rep.status_ == repeated_request)
                os << " (" << error_names[repeated_request] << ")";
        }
        return os;
    }

///////////////////////////////////////////////////////////////////////////////
}}}  // namespace hpx::naming::server

///////////////////////////////////////////////////////////////////////////////
// this is the current version of the parcel serialization format
// this definition needs to be in the global namespace
BOOST_CLASS_VERSION(hpx::naming::server::reply, HPX_REPLY_VERSION)

#endif 
