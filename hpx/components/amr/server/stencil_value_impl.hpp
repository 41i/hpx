//  Copyright (c) 2007-2008 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_COMPONENTS_AMR_STENCIL_VALUE_IMPL_OCT_17_2008_0848AM)
#define HPX_COMPONENTS_AMR_STENCIL_VALUE_IMPL_OCT_17_2008_0848AM

#include <hpx/components/amr/server/stencil_value.hpp>
#include <hpx/components/amr/functional_component.hpp>
#include <hpx/components/amr/logging_component.hpp>

#include <boost/bind.hpp>
#include <boost/assert.hpp>
#include <boost/assign/std/vector.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace components { namespace amr { namespace server 
{
    ///////////////////////////////////////////////////////////////////////////
    template <int N>
    struct eval_helper;

    template <>
    struct eval_helper<3>
    {
        template <typename Adaptor>
        static bool
        call(threads::thread_self& self, applier::applier& appl, 
            naming::id_type const& gid, naming::id_type const& value_gid, 
            Adaptor* in)
        {
            using namespace boost::assign;

            std::vector<naming::id_type> input_gids;
            input_gids += in[0]->get(self), in[1]->get(self), in[2]->get(self);

            return components::amr::stubs::functional_component::eval(
                self, appl, gid, value_gid, input_gids);
        }
    };

    template <>
    struct eval_helper<5>
    {
        template <typename Adaptor>
        static bool
        call(threads::thread_self& self, applier::applier& appl, 
            naming::id_type const& gid, naming::id_type const& value_gid, 
            Adaptor* in)
        {
            using namespace boost::assign;

            std::vector<naming::id_type> input_gids;
            input_gids += 
                in[0]->get(self), in[1]->get(self), 
                in[2]->get(self), in[3]->get(self), 
                in[4]->get(self);

            return components::amr::stubs::functional_component::eval(
                self, appl, gid, value_gid, input_gids);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    inline naming::id_type 
    alloc_helper(threads::thread_self& self, applier::applier& appl, 
        naming::id_type const& gid)
    {
        return components::amr::stubs::functional_component::alloc_data(
            self, appl, gid);
    }

    inline void free_helper(applier::applier& appl, naming::id_type const& fgid,
        naming::id_type& gid)
    {
        components::amr::stubs::functional_component::free_data(appl, fgid, gid);
        gid = naming::invalid_id;
    }

    inline void free_helper_sync(threads::thread_self& self, 
        applier::applier& appl, naming::id_type const& fgid, naming::id_type& gid)
    {
        components::amr::stubs::functional_component::free_data_sync(self,
            appl, fgid, gid);
        gid = naming::invalid_id;
    }

    ///////////////////////////////////////////////////////////////////////////
    inline void send_value (threads::thread_self& self, applier::applier& appl, 
        naming::id_type const& logging, naming::id_type const& memblock)
    {
        // synchronously clone the result
        naming::id_type clone = components::stubs::memory_block::clone(self, 
            appl, memblock);
        amr::stubs::logging_component::logentry(appl, logging, clone);
    }

    ///////////////////////////////////////////////////////////////////////////
    template <int N>
    stencil_value<N>::stencil_value(threads::thread_self& self, applier::applier& appl)
      : driver_thread_(0), sem_in_(N), sem_out_(0), sem_result_(0), 
        value_gid_(naming::invalid_id), backup_value_gid_(naming::invalid_id),
        functional_gid_(naming::invalid_id), logging_gid_(naming::invalid_id),
        is_called_(false)
    {
        // create adaptors
        for (std::size_t i = 0; i < N; ++i)
        {
            in_[i].reset(new in_adaptor_type(appl));
            out_[i].reset(new out_adaptor_type(self, appl));
            out_[i]->get()->set_callback(
                boost::bind(&stencil_value::get_value, this, _1, _2));
        }
        // the threads driving the computation are created in 
        // set_functional_component only (see below)
    }

    template <int N>
    stencil_value<N>::~stencil_value()
    {
    }

    template <int N>
    void stencil_value<N>::finalize(threads::thread_self& self, 
        applier::applier& appl) 
    {
        if (naming::invalid_id != value_gid_) 
            free_helper_sync(self, appl, functional_gid_, value_gid_);
    }

    ///////////////////////////////////////////////////////////////////////////
    // The call action is used for the first time step only. It sets the 
    // initial value and waits for the whole evolution to finish, returning the
    // result
    template <int N>
    threads::thread_state  
    stencil_value<N>::call(threads::thread_self& self, applier::applier& appl, 
        naming::id_type* result, naming::id_type const& initial)
    {
        // remember that this instance is used as the first (and last) step in
        // the computation
        is_called_ = true;

        // sem_in_ is pre-initialized to N, so we need to reset it
        sem_in_.wait(self, N);

        // set new current value
        BOOST_ASSERT(value_gid_ == naming::invalid_id);   // shouldn't be initialized yet
        value_gid_ = initial;

        // signal all output threads it's safe to read value
        sem_out_.signal(self, N);

        // wait for final result 
        sem_result_.wait(self, 1);

        // return the final value computed to the caller
        *result = value_gid_;
        value_gid_ = naming::invalid_id;

        return threads::terminated;
    }

    ///////////////////////////////////////////////////////////////////////////
    // The main thread function loops through all operations of the time steps
    // to be handled by this instance:
    // - get values from previous time step
    // - calculate current result
    // - set the result for the next time step
    template <int N>
    threads::thread_state  
    stencil_value<N>::main(threads::thread_self& self, applier::applier& appl)
    {
        // ask functional component to create the local data value
        backup_value_gid_ = alloc_helper(self, appl, functional_gid_);

        // this is the main loop of the computation, gathering the values
        // from the previous time step, computing the result of the current
        // time step and storing the computed value in the memory_block 
        // referenced by value_gid_
        bool is_last = false;
        while (!is_last) {
            // start acquire operations on input ports
            for (std::size_t i = 0; i < N; ++i)
                in_[i]->aquire_value(appl);         // non-blocking!

            // at this point all gid's have to be initialized
            BOOST_ASSERT(naming::invalid_id != functional_gid_);
            BOOST_ASSERT(naming::invalid_id != backup_value_gid_);

            // Compute the next value, store it in backup_value_gid_
            // The eval action returns true for the last time step.
            is_last = eval_helper<N>::call(self, appl, functional_gid_, 
                backup_value_gid_, in_);

            // if the computation finished in an instance which has been used
            // as the target for the initial call_action we can't exit right
            // away, because all other instances need to be executed one more 
            // time allowing them to free all their resources
            if (is_last && is_called_) {
                is_called_ = false;
                is_last = false;
            }

            // Wait for all output threads to have read the current value.
            // On the first time step the semaphore is preset to allow 
            // to immediately set the value.
            sem_in_.wait(self, N);

            // set new current value, allocate space for next current value
            // if needed (this may happen for all time steps except the first 
            // one, where the first gets it's initial value during the 
            // call_action)
            if (naming::invalid_id == value_gid_) 
                value_gid_ = alloc_helper(self, appl, functional_gid_);
            std::swap(value_gid_, backup_value_gid_);

            // if a logging element is defined, take a snapshot of the result 
            // and forward it there
            if (logging_gid_ != naming::invalid_id) 
                send_value(self, appl, logging_gid_, value_gid_);

            // signal all output threads it's safe to read value
            sem_out_.signal(self, N);
        }

        sem_result_.signal(self, 1);        // final result has been set

        free_helper(appl, functional_gid_, backup_value_gid_);
        return threads::terminated;
    }

    ///////////////////////////////////////////////////////////////////////////
    /// The function get will be called by the out-ports whenever 
    /// the current value has been requested.
    template <int N>
    void
    stencil_value<N>::get_value(threads::thread_self& self, 
        naming::id_type* result)
    {
        sem_out_.wait(self, 1);     // wait for the current value to be valid
        *result = value_gid_;       // acquire the current value
        sem_in_.signal(self, 1);    // signal to have read the value
    }

    ///////////////////////////////////////////////////////////////////////////
    template <int N>
    threads::thread_state 
    stencil_value<N>::get_output_ports(threads::thread_self& self, 
        applier::applier& appl, std::vector<naming::id_type> *gids)
    {
        gids->clear();
        for (std::size_t i = 0; i < N; ++i)
            gids->push_back(out_[i]->get_gid(appl));
        return threads::terminated;
    }

    ///////////////////////////////////////////////////////////////////////////
    template <int N>
    threads::thread_state 
    stencil_value<N>::connect_input_ports(threads::thread_self& self, 
        applier::applier& appl, std::vector<naming::id_type> const& gids)
    {
        if (gids.size() < N) {
            HPX_THROW_EXCEPTION(bad_parameter, 
                "insufficient number of gid's supplied");
            return threads::terminated;
        }
        for (std::size_t i = 0; i < N; ++i)
            in_[i]->connect(gids[i]);

        // if the functional component already has been set we need to start 
        // the driver thread
        if (functional_gid_ != naming::invalid_id && 0 == driver_thread_) {
            // run the thread which collects the input, executes the provided
            // functional element and sets the value for the next time step
            driver_thread_ = applier::register_work(appl, 
                boost::bind(&stencil_value<N>::main, this, _1, boost::ref(appl)), 
                "stencil_value::main");
        }
        return threads::terminated;
    }

    ///////////////////////////////////////////////////////////////////////////
    template <int N>
    threads::thread_state 
    stencil_value<N>::set_functional_component(threads::thread_self& self, 
        applier::applier& appl, naming::id_type const& gid, 
        naming::id_type const& logging)
    {
        // store gid of functional component
        functional_gid_ = gid;
        logging_gid_ = logging;

        // if all inputs have been bound already we need to start the driver 
        // thread
        bool inputs_bound = true;
        for (std::size_t i = 0; i < N && inputs_bound; ++i)
            inputs_bound = in_[i]->is_bound();

        if (inputs_bound && 0 == driver_thread_) {
            // run the thread which collects the input, executes the provided
            // functional element and sets the value for the next time step
            driver_thread_ = applier::register_work(appl, 
                boost::bind(&stencil_value<N>::main, this, _1, boost::ref(appl)), 
                "stencil_value::main");
        }

        return threads::terminated;
    }

}}}}

#endif
