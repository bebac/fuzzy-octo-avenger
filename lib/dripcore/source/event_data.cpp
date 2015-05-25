// ----------------------------------------------------------------------------
//
//     Filename   : dripcore/event_data.cpp
//
//     Author     : Benny Bach <benny.bach@gmail.com>
//                  Copyright (C) 2015
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#include <dripcore/event_data.h>
#include <dripcore/eventable.h>
#include <dripcore/loop.h>

// ----------------------------------------------------------------------------
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>

// ----------------------------------------------------------------------------
#include <iostream>
#include <sstream>

// ----------------------------------------------------------------------------
namespace dripcore
{
  event_data::event_data(std::shared_ptr<eventable> eventable_ptr)
    :
    eventable_ptr_(eventable_ptr)
  {
  }

  void event_data::dispatch(unsigned events)
  {
    auto eventable = eventable_ptr_.lock();

    if ( eventable )
    {
      // TODO: Locking the context is not optimal. Need another way
      //       to schedule events.

      context::lock_guard lock(eventable->get_context());

      if ( (events & EPOLLIN) == EPOLLIN ) {
        eventable->call_rd_handler();
      }

      if ( (events & EPOLLOUT) == EPOLLOUT ) {
        eventable->call_wr_handler();
      }
    }
    else
    {
      throw std::runtime_error("dispatch on an expired eventable");
    }
  }
}