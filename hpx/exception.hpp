//  Copyright (c) 2007-2009 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_EXCEPTION_MAR_24_2008_0929AM)
#define HPX_EXCEPTION_MAR_24_2008_0929AM

#include <exception>
#include <string>

#include <hpx/hpx_fwd.hpp>
#include <hpx/runtime/threads/thread_helpers.hpp>
#include <hpx/util/logging.hpp>
#include <hpx/util/util.hpp>

#include <boost/assert.hpp>
#include <boost/throw_exception.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx  
{
    ///////////////////////////////////////////////////////////////////////////
    enum error
    {
        success = 0,
        no_success = 1,
        not_implemented = 2,
        out_of_memory = 3,
        bad_action_code = 4,
        bad_component_type = 5,
        network_error = 6,
        version_too_new = 7,
        version_too_old = 8,
        version_unknown = 9,
        unknown_component_address = 10,
        duplicate_component_address = 11,
        invalid_status = 12,
        bad_parameter = 13,
        internal_server_error = 14,
        service_unavailable = 15,
        bad_request = 16,
        repeated_request = 17,
        lock_error = 18,
        duplicate_console = 19,
        no_registered_console = 20,
        last_error
    };

    char const* const error_names[] = 
    {
        "success",
        "no_success",
        "not_implemented",
        "out_of_memory",
        "bad_action_code",
        "bad_component_type",
        "network_error",
        "version_too_new",
        "version_too_old",
        "version_unknown",
        "unknown_component_address",
        "duplicate_component_address",
        "invalid status",
        "bad parameter",
        "internal_server_error",
        "service_unavailable",
        "bad_request",
        "repeated_request",
        "lock_error",
        "duplicate_console",
        "no_registered_console",
        ""
    };

    namespace detail 
    {
        class hpx_category : public boost::system::error_category
        {
        public:
            const char* name() const
            {
                return "HPX";
            }

            std::string message(int value) const
            {
                switch (value) {
                case success:
                case no_success:
                case not_implemented:
                case out_of_memory:
                case bad_action_code:
                case bad_component_type:
                case network_error:
                case version_too_new:
                case version_too_old:
                case unknown_component_address:
                case duplicate_component_address:
                case invalid_status:
                case bad_parameter:
                case repeated_request:
                case lock_error:
                case duplicate_console:
                case no_registered_console:
                    return std::string("HPX(") + error_names[value] + ")";

                default:
                    break;
                }
                return "HPX(Unknown error)";
            }
        };

        // this doesn't add any text to the exception what() message
        class hpx_category_rethrow : public boost::system::error_category
        {
        public:
            const char* name() const
            {
                return "";
            }

            std::string message(int value) const
            {
                return "";
            }
        };

    } // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    //  Define the HPX error category
    inline boost::system::error_category const& get_hpx_category()
    {
        static detail::hpx_category instance;
        return instance;
    }

    inline boost::system::error_category const& get_hpx_rethrow_category()
    {
        static detail::hpx_category_rethrow instance;
        return instance;
    }

    ///////////////////////////////////////////////////////////////////////////
    enum throwmode 
    {
        plain = 0,
        rethrow = 1,
    };

    inline boost::system::error_code 
    make_error_code(error e, throwmode mode = plain)
    {
        return boost::system::error_code(static_cast<int>(e), 
            mode == rethrow ? get_hpx_rethrow_category() : get_hpx_category());
    }

    ///////////////////////////////////////////////////////////////////////////
    inline boost::system::error_condition 
        make_error_condition(error e, throwmode mode)
    {
        return boost::system::error_condition(static_cast<int>(e), 
            mode == rethrow ? get_hpx_rethrow_category() : get_hpx_category());
    }

    ///////////////////////////////////////////////////////////////////////////
    class error_code : public boost::system::error_code
    {
    public:
        explicit error_code(throwmode mode = plain)
          : boost::system::error_code(make_error_code(success, mode))
        {}

        explicit error_code(error e, char const* msg = "", throwmode mode = plain)
          : boost::system::error_code(make_error_code(e, mode))
          , message_(msg)
        {}

        error_code(error e, std::string const& msg, throwmode mode = plain)
          : boost::system::error_code(make_error_code(e, mode))
          , message_(msg)
        {}

        std::string const& get_message() const { return message_; }

    private:
        std::string message_;
    };

    inline error_code 
    make_error_code(error e, char const* msg, throwmode mode = plain)
    {
        return error_code(e, msg, mode);
    }

    inline error_code 
    make_error_code(error e, std::string const& msg, throwmode mode = plain)
    {
        return error_code(e, msg, mode);
    }

    inline error_code make_success_code(throwmode mode = plain)
    {
        return error_code(success, "success", mode);
    }

    //  predefined error_code object used as "throw on error" tag
    HPX_EXCEPTION_EXPORT extern error_code throws;

    ///////////////////////////////////////////////////////////////////////////
    class HPX_EXCEPTION_EXPORT exception : public boost::system::system_error
    {
    public:
        explicit exception(error e) 
          : boost::system::system_error(make_error_code(e, plain))
        {
            BOOST_ASSERT(e >= success && e < last_error);
            LERR_(error) << "created exception: " << this->what();
        }
        explicit exception(boost::system::system_error e) 
          : boost::system::system_error(e)
        {
            LERR_(error) << "created exception: " << this->what();
        }
        exception(error e, char const* msg, throwmode mode = plain) 
          : boost::system::system_error(make_error_code(e, mode), msg)
        {
            BOOST_ASSERT(e >= success && e < last_error);
            LERR_(error) << "created exception: " << this->what();
        }
        exception(error e, std::string const& msg, throwmode mode = plain) 
          : boost::system::system_error(make_error_code(e, mode), msg)
        {
            BOOST_ASSERT(e >= success && e < last_error);
            LERR_(error) << "created exception: " << this->what();
        }
        ~exception (void) throw() 
        {
        }

        error get_error() const throw() 
        { 
            return static_cast<error>(
                this->boost::system::system_error::code().value());
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    // types needed to add additional information to the thrown exceptions
    namespace detail
    {
        struct tag_throw_function {};
        struct tag_throw_thread_name {};
        struct tag_throw_file {};
        struct tag_throw_line {};

        typedef boost::error_info<struct tag_throw_function, std::string> throw_function;
        typedef boost::error_info<struct tag_throw_thread_name, std::string> throw_thread_name;
        typedef boost::error_info<struct tag_throw_file, std::string> throw_file;
        typedef boost::error_info<struct tag_throw_line, int> throw_line;

        template <typename Exception>
        void throw_exception(Exception const& e, char const* func, 
            std::string const& file, int line)
        {
            threads::thread_self* self = threads::get_self_ptr();
            if (NULL != self) {
                threads::thread_id_type id = self->get_thread_id();
                throw boost::enable_current_exception(
                    boost::enable_error_info(e) 
                        << boost::throw_function(func) 
                        << throw_thread_name(threads::get_thread_description(id))
                        << throw_file(file) << throw_line(line));
            }
            else {
                throw boost::enable_current_exception(
                    boost::enable_error_info(e) 
                        << boost::throw_function(func) 
                        << throw_file(file) << throw_line(line));
            }
        }

        template <typename Exception>
        void throw_exception(Exception const& e, std::string const& func, 
            std::string const& file, int line)
        {
            threads::thread_self* self = threads::get_self_ptr();
            if (NULL != self) {
                threads::thread_id_type id = self->get_thread_id();
                throw boost::enable_current_exception(
                    boost::enable_error_info(e) 
                        << throw_function(func) 
                        << throw_thread_name(threads::get_thread_description(id))
                        << throw_file(file) << throw_line(line));
            }
            else {
                throw boost::enable_current_exception(
                    boost::enable_error_info(e) 
                        << throw_function(func) 
                        << throw_file(file) << throw_line(line));
            }
        }
    }

///////////////////////////////////////////////////////////////////////////////
}

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace system
{
    // make sure our errors get recognized by the Boost.System library
    template<> struct is_error_code_enum<hpx::error>
    {
        static const bool value = true;
    };

    template<> struct is_error_condition_enum<hpx::error>
    { 
        static const bool value = true; 
    };

}}

///////////////////////////////////////////////////////////////////////////////
// helper macro allowing to prepend file name and line number to a generated 
// exception
#define HPX_THROW_EXCEPTION_EX(except, errcode, func, msg, mode)              \
    {                                                                         \
        std::string __s;                                                      \
        boost::filesystem::path __p(__FILE__, boost::filesystem::native);     \
        if (LHPX_ENABLED(debug))                                              \
        {                                                                     \
            __s = hpx::util::leaf(__p);                                       \
            __s += std::string("(") + BOOST_PP_STRINGIZE(__LINE__) + "): ";   \
        }                                                                     \
        __s += msg;                                                           \
        hpx::detail::throw_exception(except((hpx::error)errcode, __s, mode),  \
            func, __p.string(), __LINE__);                                    \
    }                                                                         \
    /**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_THROW_EXCEPTION(errcode, f, msg)                                  \
    HPX_THROW_EXCEPTION_EX(hpx::exception, errcode, f, msg, hpx::plain)       \
    /**/

#define HPX_RETHROW_EXCEPTION(errcode, f, msg)                                \
    HPX_THROW_EXCEPTION_EX(hpx::exception, errcode, f, msg, hpx::rethrow)     \
    /**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_THROWS_IF(ec, errcode, f, msg)                                    \
    {                                                                         \
        if (&ec == &hpx::throws)                                              \
            HPX_THROW_EXCEPTION(errcode, f, msg);                             \
        ec = make_error_code((hpx::error)errcode, msg);                       \
    }
    /**/

#define HPX_RETHROWS_IF(ec, errcode, f, msg)                                  \
    {                                                                         \
        if (&ec == &hpx::throws)                                              \
            HPX_RETHROW_EXCEPTION(errcode, f, msg);                           \
        ec = make_error_code((hpx::error)errcode, msg);                       \
    }
    /**/

#include <hpx/config/warnings_suffix.hpp>

#endif



