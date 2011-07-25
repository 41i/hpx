////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Hartmut Kaiser, Katelyn Kufahl & Bryce Lelbach
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////

#if !defined(HPX_894FCD94_A2A4_413D_AD50_088A9178DE77)
#define HPX_894FCD94_A2A4_413D_AD50_088A9178DE77

#include <hpx/hpx_fwd.hpp>
#include <hpx/runtime/parcelset/parcel.hpp>
#include <hpx/runtime/parcelset/parcelhandler.hpp>
#include <hpx/runtime/parcelset/parcelhandler_queue_base.hpp>

#include <boost/assert.hpp>
#include <boost/lockfree/fifo.hpp>

namespace hpx { namespace parcelset { namespace policies
{

struct global_parcelhandler_queue : parcelhandler_queue_base
{
    global_parcelhandler_queue()
      : ph_(0)
    {}

    ~global_parcelhandler_queue()
    {
        parcel p;
        while (get_parcel(p)) {}
    }

    void add_parcel(parcel const& p)
    {
        parcel* tmp = new parcel(p);
        parcels_.enqueue(tmp);

        // do some work (notify event handlers)
        BOOST_ASSERT(ph_ != 0);
        notify_(*ph_, tmp->get_destination_addr()); 
    }

    bool get_parcel(parcel& p)
    {
        parcel* tmp;

        if (parcels_.dequeue(&tmp))
        {
            p = *tmp;
            delete tmp; 
            return true;
        }

        else
            return false;
    }

    bool register_event_handler(callback_type const& sink) 
    { 
        return notify_.connect(sink).connected();
    }

    bool register_event_handler(callback_type const& sink, 
        connection_type& conn) 
    {
        return (conn = notify_.connect(sink)).connected();
    } 

    void set_parcelhandler(parcelhandler* ph)
    {
        BOOST_ASSERT(ph_ == 0);
        ph_ = ph;
    }

    boost::int64_t get_queue_length() const 
    {
        return -1;
    }

  private:
    boost::lockfree::fifo<parcel*> parcels_;

    parcelhandler* ph_;

    boost::signals2::signal_type<
        void(parcelhandler&, naming::address const&)
      , boost::signals2::keywords::mutex_type<lcos::mutex>
    >::type notify_;
};

}}}

#endif // HPX_894FCD94_A2A4_413D_AD50_088A9178DE77

