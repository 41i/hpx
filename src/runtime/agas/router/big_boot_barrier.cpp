////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Bryce Lelbach
//  Copyright (c) 2011 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////

#if HPX_AGAS_VERSION > 0x10

#include <hpx/config.hpp>
#include <hpx/hpx.hpp>
#include <hpx/runtime/actions/plain_action.hpp>
#include <hpx/runtime/components/plain_component_factory.hpp>
#include <hpx/runtime/components/server/managed_component_base.hpp>
#include <hpx/util/portable_binary_iarchive.hpp>
#include <hpx/util/container_device.hpp>
#include <hpx/util/stringstream.hpp>
#include <hpx/util/static.hpp>
#include <hpx/util/uintptr_t.hpp>
#include <hpx/runtime/actions/action_support.hpp>
#include <hpx/runtime/parcelset/parcel.hpp>
#include <hpx/runtime/parcelset/parcelport.hpp>
#include <hpx/runtime/parcelset/parcelport_connection.hpp>
#include <hpx/runtime/naming/resolver_client.hpp>
#include <hpx/runtime/agas/router/big_boot_barrier.hpp>

#include <boost/thread.hpp>
#include <boost/assert.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

namespace hpx { namespace agas
{

typedef lcos::eager_future<
    naming::resolver_client::primary_namespace_server_type::bind_locality_action,
    naming::resolver_client::response_type
> allocate_response_future_type;

typedef lcos::eager_future<
    naming::resolver_client::primary_namespace_server_type::bind_gid_action,
    naming::resolver_client::response_type
> bind_response_future_type;

typedef components::heap_factory<
    lcos::detail::future_value<
        naming::resolver_client::response_type
      , naming::resolver_client::response_type
      , 1
    >
  , components::managed_component<
        lcos::detail::future_value<
            naming::resolver_client::response_type
          , naming::resolver_client::response_type
          , 1
        >
    >
> response_heap_type; 

void early_parcel_sink(
    parcelset::parcelport&
  , boost::shared_ptr<std::vector<char> > const& parcel_data
) { // {{{
    parcelset::parcel p;

    typedef util::container_device<std::vector<char> > io_device_type;
    boost::iostreams::stream<io_device_type> io (*parcel_data.get());

    // De-serialize the parcel data
    #if HPX_USE_PORTABLE_ARCHIVES != 0
        util::portable_binary_iarchive archive(io);
    #else
        boost::archive::binary_iarchive archive(io);
    #endif
    
    archive >> p;

    // decode the local virtual address of the parcel
    naming::address addr = p.get_destination_addr();

    // decode the action-type in the parcel
    actions::action_type act = p.get_action();

    // early parcels should only be plain actions
    BOOST_ASSERT
        (actions::base_action::plain_action == act->get_action_type());

    // early parcels can't have continuations 
    BOOST_ASSERT(!p.get_continuation());

    act->get_thread_function(0)
        (threads::thread_state_ex(threads::wait_signaled));
} // }}}

void early_write_handler(
    boost::system::error_code const& e 
  , std::size_t size
) {
    hpx::util::osstream strm;
    strm << e.message() << " (read " 
         << size << " bytes)";
    HPX_THROW_EXCEPTION(network_error, 
        "agas::early_write_handler", 
        hpx::util::osstream_get_string(strm));
}

// {{{ early action forwards
void register_console(
    boost::uint64_t count
  , naming::address const& baseaddr
  , hpx::uintptr_t offset
  , hpx::uintptr_t heap_ptr
);

void notify_console(
    naming::gid_type const& prefix
  , boost::uint64_t count
  , naming::address const& baseaddr
  , hpx::uintptr_t heap_ptr
);

void register_worker(
    boost::uint64_t count
  , naming::address const& baseaddr
  , hpx::uintptr_t offset
  , hpx::uintptr_t heap_ptr
);

void notify_worker(
    naming::gid_type const& prefix
  , boost::uint64_t count
  , naming::address const& baseaddr
  , hpx::uintptr_t heap_ptr
);
// }}}

// {{{ early action types
typedef actions::plain_action4<
    boost::uint64_t, naming::address const&, hpx::uintptr_t, hpx::uintptr_t
  , register_console
> register_console_action;

typedef actions::plain_action4<
    naming::gid_type const&, boost::uint64_t, naming::address const&
  , hpx::uintptr_t, notify_console
> notify_console_action;

typedef actions::plain_action4<
    boost::uint64_t, naming::address const&, hpx::uintptr_t, hpx::uintptr_t
  , register_worker
> register_worker_action;

typedef actions::plain_action4<
    naming::gid_type const&, boost::uint64_t, naming::address const&
  , hpx::uintptr_t, notify_worker
> notify_worker_action;
// }}}

// {{{ early action definitions
// remote call to AGAS
void register_console(
    boost::uint64_t count
  , naming::address const& baseaddr
  , hpx::uintptr_t offset
  , hpx::uintptr_t heap_ptr
) {
    naming::resolver_client& agas_client = get_runtime().get_agas_client();

    if (HPX_UNLIKELY(agas_client.state() != router_state_launching))
    {
        hpx::util::osstream strm;
        strm << "AGAS server has already launched, can't register "
             << baseaddr.locality_; 
        HPX_THROW_EXCEPTION(internal_server_error, 
            "agas::register_console", 
            hpx::util::osstream_get_string(strm));
    }

    naming::gid_type prefix, lower, upper;

    agas_client.get_prefix(baseaddr.locality_, prefix, true, false); 
    agas_client.get_id_range(baseaddr.locality_, count, lower, upper);
    agas_client.bind_range(lower, count, baseaddr, offset); 
    agas_client.registerid("/locality(console)", prefix);

    if (HPX_UNLIKELY((prefix + 1) != lower))
    {
        HPX_THROW_EXCEPTION(internal_server_error,
            "agas::register_console",
            "bad initial GID range allocation");
    }

    get_big_boot_barrier().notify();

    get_big_boot_barrier().apply(naming::address(baseaddr.locality_),
        new notify_console_action(prefix, count, baseaddr, heap_ptr));
}

// TODO: callback must finishing installing new heap
// TODO: callback must set up future pool 
// AGAS callback to client
void notify_console(
    naming::gid_type const& prefix // TODO: this could be sent as a uint32_t
  , boost::uint64_t count
  , naming::address const& baseaddr
  , hpx::uintptr_t heap_ptr
) {
    naming::resolver_client& agas_client = get_runtime().get_agas_client();

    if (HPX_UNLIKELY(agas_client.state() != router_state_launching))
    {
        hpx::util::osstream strm;
        strm << "locality "
             << baseaddr.locality_
             << " has launched early"; 
        HPX_THROW_EXCEPTION(internal_server_error, 
            "agas::notify_console", 
            hpx::util::osstream_get_string(strm));
    }

    // set our prefix
    agas_client.local_prefix(prefix);

    // assign the initial gid range to the unique id range allocator that our
    // response heap is using
    response_heap_type::get_heap().set_range(prefix + 1, prefix + 1 + count); 

    // finish setting up the first heap. 
    response_heap_type::block_type* p
        = reinterpret_cast<response_heap_type::block_type*>(heap_ptr);

    // set the base gid that we bound to this heap
    p->set_gid(prefix + 1); 

    // push the heap onto the OSHL
    response_heap_type::get_heap().add_heap(p); 

    // set up the future pools
    naming::resolver_client::allocate_response_pool_type&
        allocate_pool = agas_client.get_allocate_response_pool();
    naming::resolver_client::bind_response_pool_type&
        bind_pool = agas_client.get_bind_response_pool();

    util::runtime_configuration const& ini_ = get_runtime().get_config();

    const std::size_t allocate_size =
        ini_.get_agas_allocate_response_pool_size();
    const std::size_t bind_size =
        ini_.get_agas_bind_response_pool_size();

    for (std::size_t i = 0; i < allocate_size; ++i)
        allocate_pool.enqueue(new allocate_response_future_type);     

    for (std::size_t i = 0; i < bind_size; ++i)
        bind_pool.enqueue(new bind_response_future_type);     
}

// remote call to AGAS
void register_worker(
    boost::uint64_t count
  , naming::address const& baseaddr
  , hpx::uintptr_t offset
  , hpx::uintptr_t heap_ptr
) {
    naming::resolver_client& agas_client = get_runtime().get_agas_client();

    if (HPX_UNLIKELY(agas_client.state() != router_state_launching))
    {
        hpx::util::osstream strm;
        strm << "AGAS server has already launched, can't register "
             << baseaddr.locality_; 
        HPX_THROW_EXCEPTION(internal_server_error, 
            "agas::register_worker", 
            hpx::util::osstream_get_string(strm));
    }

    naming::gid_type prefix, lower, upper;

    agas_client.get_prefix(baseaddr.locality_, prefix, true, false); 
    agas_client.get_id_range(baseaddr.locality_, count, lower, upper);
    agas_client.bind_range(lower, count, baseaddr, offset); 

    if (HPX_UNLIKELY((prefix + 1) != lower))
    {
        HPX_THROW_EXCEPTION(internal_server_error,
            "agas::register_worker",
            "bad initial GID range allocation");
    }

    get_big_boot_barrier().notify();

    get_big_boot_barrier().apply(naming::address(baseaddr.locality_),
        new notify_worker_action(prefix, count, baseaddr, heap_ptr));
}

// TODO: callback must finishing installing new heap
// TODO: callback must set up future pool 
// AGAS callback to client
void notify_worker(
    naming::gid_type const& prefix // TODO: this could be sent as a uint32_t
  , boost::uint64_t count
  , naming::address const& baseaddr
  , hpx::uintptr_t heap_ptr
) {
    naming::resolver_client& agas_client = get_runtime().get_agas_client();

    if (HPX_UNLIKELY(agas_client.state() != router_state_launching))
    {
        hpx::util::osstream strm;
        strm << "locality "
             << baseaddr.locality_
             << " has launched early"; 
        HPX_THROW_EXCEPTION(internal_server_error, 
            "agas::notify_worker", 
            hpx::util::osstream_get_string(strm));
    }

    // set our prefix
    agas_client.local_prefix(prefix);

    // assign the initial gid range to the unique id range allocator that our
    // response heap is using
    response_heap_type::get_heap().set_range(prefix + 1, prefix + 1 + count); 

    // finish setting up the first heap. 
    response_heap_type::block_type* p
        = reinterpret_cast<response_heap_type::block_type*>(heap_ptr);

    // set the base gid that we bound to this heap
    p->set_gid(prefix + 1); 

    // push the heap onto the OSHL
    response_heap_type::get_heap().add_heap(p); 

    // set up the future pools
    naming::resolver_client::allocate_response_pool_type&
        allocate_pool = agas_client.get_allocate_response_pool();
    naming::resolver_client::bind_response_pool_type&
        bind_pool = agas_client.get_bind_response_pool();

    util::runtime_configuration const& ini_ = get_runtime().get_config();

    const std::size_t allocate_size =
        ini_.get_agas_allocate_response_pool_size();
    const std::size_t bind_size =
        ini_.get_agas_bind_response_pool_size();

    for (std::size_t i = 0; i < allocate_size; ++i)
        allocate_pool.enqueue(new allocate_response_future_type);     

    for (std::size_t i = 0; i < bind_size; ++i)
        bind_pool.enqueue(new bind_response_future_type);     
}
// }}}

}}

using hpx::agas::register_console_action;
using hpx::agas::notify_console_action;
using hpx::agas::register_worker_action;
using hpx::agas::notify_worker_action;

HPX_REGISTER_PLAIN_ACTION(register_console_action);
HPX_REGISTER_PLAIN_ACTION(notify_console_action);
HPX_REGISTER_PLAIN_ACTION(register_worker_action);
HPX_REGISTER_PLAIN_ACTION(notify_worker_action);
  
namespace hpx { namespace agas
{

void big_boot_barrier::spin()
{
    boost::unique_lock<boost::mutex> lock(mtx);
    while (connected)
        cond.wait(lock);
}

big_boot_barrier::big_boot_barrier(
    parcelset::parcelport& pp_ 
  , util::runtime_configuration const& ini_
  , runtime_mode runtime_type_
):
    pp(pp_)
  , connection_cache_(pp_.get_connection_cache())
  , io_service_pool_(pp_.get_io_service_pool())
  , router_type(ini_.get_agas_router_mode())
  , runtime_type(runtime_type_)
  , bootstrap_agas(ini_.get_agas_locality())
  , cond()
  , mtx()
  , connected( (router_mode_bootstrap == router_type)
             ? (ini_.get_num_localities() - 1)
             : 1) 
{
    pp_.register_event_handler(boost::bind(&early_parcel_sink, _1, _2));
}

void big_boot_barrier::apply(
    naming::address const& addr
  , actions::base_action* act 
) { // {{{
    parcelset::parcel p(addr, act);

    parcelset::parcelport_connection_ptr client_connection
        (connection_cache_.get(addr.locality_));

    if (!client_connection)
    {
        // The parcel gets serialized inside the connection constructor, no 
        // need to keep the original parcel alive after this call returned.
        client_connection.reset(new parcelset::parcelport_connection
            (io_service_pool_.get_io_service(), addr.locality_,
                connection_cache_)); 
        client_connection->set_parcel(p);

        // Connect to the target locality, retry if needed
        boost::system::error_code error = boost::asio::error::try_again;

        for (int i = 0; i < HPX_MAX_NETWORK_RETRIES; ++i)
        {
            try 
            { // {{{
                naming::locality::iterator_type
                    it = addr.locality_.connect_begin
                        (io_service_pool_.get_io_service()), 
                    end = addr.locality_.connect_end();

                for (; it != end; ++it)
                {
                    client_connection->socket().close();
                    client_connection->socket().connect(*it, error);

                    if (!error) 
                        break;
                }

                if (!error) 
                    break;

                // wait for a really short amount of time (usually 100 ms)
                boost::this_thread::sleep(boost::get_system_time() + 
                    boost::posix_time::microseconds
                        (HPX_NETWORK_RETRIES_SLEEP));
            } // }}}

            catch (boost::system::error_code const& e)
            {
                HPX_THROW_EXCEPTION(network_error, 
                    "big_boot_barrier::get_connection", e.message());
            }
        }

        if (error)
        { // {{{ 
            client_connection->socket().close();

            hpx::util::osstream strm;
            strm << error.message() << " (while trying to connect to: " 
                 << addr.locality_ << ")";
            HPX_THROW_EXCEPTION(network_error, "parcelport::get_connection", 
                hpx::util::osstream_get_string(strm));
        } // }}}
    }

    else
        client_connection->set_parcel(p);

    client_connection->async_write(early_write_handler);
} // }}} 

void big_boot_barrier::wait()
{ // {{{
    if (router_mode_bootstrap == router_type)
    {
        // bootstrap, console
        if (runtime_mode_console == runtime_type)
            spin();

        // bootstrap, worker
        else
            // We need to wait for the console to connect to us. The console
            // will send register_console to our parcelport, which will wake us
            // up.
            spin();
    }

    else
    {
        // allocate our first heap
        response_heap_type::block_type* p = response_heap_type::alloc_heap();

        // hosted, console
        if (runtime_mode_console == runtime_type)
        {
            // We need to contact the bootstrap AGAS node, and then wait
            // for it to signal us. We do this by executing register_console
            // on the bootstrap AGAS node, and sleeping on this node. We'll
            // be woken up by notify_console. 

            apply(bootstrap_agas, new register_console_action
                ((boost::uint64_t) response_heap_type::block_type::heap_step,
                    p->get_address(), (boost::uint64_t)
                        response_heap_type::block_type::heap_size,
                            (hpx::uintptr_t) p)); 
            spin();
        }

        // hosted, worker
        else
        {
            // we need to contact the bootstrap AGAS node, and then wait
            // for it to signal us. 

            apply(bootstrap_agas, new register_worker_action
                ((boost::uint64_t) response_heap_type::block_type::heap_step,
                    p->get_address(), (boost::uint64_t)
                        response_heap_type::block_type::heap_size,
                            (hpx::uintptr_t) p)); 
            spin();
        }
    }  
} // }}}

void big_boot_barrier::notify()
{
    boost::mutex::scoped_lock lk(mtx);
    --connected;
    cond.notify_all();
}

struct bbb_tag;

void create_big_boot_barrier(
    parcelset::parcelport& pp_ 
  , util::runtime_configuration const& ini_
  , runtime_mode runtime_type_
) {
    util::static_<boost::shared_ptr<big_boot_barrier>, bbb_tag> bbb;
    if (bbb.get())
    {
        HPX_THROW_EXCEPTION(internal_server_error, 
            "create_big_boot_barrier",
            "create_big_boot_barrier was called more than once");
    }
    bbb.get().reset(new big_boot_barrier(pp_, ini_, runtime_type_));
}

big_boot_barrier& get_big_boot_barrier()
{
    util::static_<boost::shared_ptr<big_boot_barrier>, bbb_tag> bbb;
    if (!bbb.get())
    {
        HPX_THROW_EXCEPTION(internal_server_error, 
            "get_big_boot_barrier",
            "big_boot_barrier has not been created yet");
    }
    return *(bbb.get());
}

}}

#endif
