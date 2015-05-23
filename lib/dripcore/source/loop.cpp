// ----------------------------------------------------------------------------
//
//     Filename   : dripcore/loop.h
//
//     Author     : Benny Bach <benny.bach@gmail.com>
//                  Copyright (C) 2015
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#include <dripcore/loop.h>

// ----------------------------------------------------------------------------
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>

// ----------------------------------------------------------------------------
#include <iostream>
#include <system_error>
#include <cassert>
#include <sstream>

// ----------------------------------------------------------------------------
namespace dripcore
{
  loop::loop()
    :
    running_(false)
  {
    if ( (os_handle_ = epoll_create1(EPOLL_CLOEXEC)) < 0 ) {
      throw std::system_error(errno, std::system_category());
    }
  }

  void loop::run()
  {
    if  ( eventables_.size() > 0 )
    {
      running_ = true;

      for ( unsigned i=0; i<4; i++ ) {
        workers_.emplace_back(std::thread{&loop::worker_loop, this});
      }

      for ( auto& thr : workers_ ) {
        thr.join();
      }
    }
  }

  void loop::shutdown()
  {
    stop_all_eventables();
  }

  loop::eventable_container loop::eventables() const
  {
    lock_guard lock(mutex_);
    return eventables_;
  }

  void loop::start(const std::shared_ptr<eventable>& eventable)
  {
    struct epoll_event ev;

    ev.events  = EPOLLET;

    if ( eventable->has_rd_handler() ) {
      ev.events |= EPOLLIN;
    }

    if ( eventable->has_wr_handler() ) {
      ev.events |= EPOLLOUT;
    }

    ev.data.ptr = eventable->event_data_ptr().get();

    if ( epoll_ctl(os_handle_, EPOLL_CTL_ADD, eventable->get_os_handle(), &ev) == -1 ) {
      throw std::system_error(errno, std::system_category());
    }

    lock_guard lock(mutex_);

    eventables_.insert(eventable);
    eventable->started(this);
  }

  void loop::stop(const std::shared_ptr<eventable>& eventable)
  {
#if 0
    std::stringstream ss;
    ss << "loop::stop ptr=" << int64_t(eventable.get()) << ", fd=" << eventable->get_os_handle() << std::endl;
    std::cerr << ss.str();
#endif

    if ( epoll_ctl(os_handle_, EPOLL_CTL_DEL, eventable->get_os_handle(), nullptr) == -1 ) {
      throw std::system_error(errno, std::system_category());
    }

    lock_guard lock(mutex_);

    // Notify the eventable that it was stopped.
    eventable->stopped(this);

    // Install noop handlers. This will avoid calling eventable handlers incase
    // another thread has been notified of events on the eventables os handle.
    eventable->set_rd_handler([]{});
    eventable->set_wr_handler([]{});

    eventables_.erase(eventable);

#if 0
    {
      std::stringstream ss;
      ss << "loop::stop eventables size=" << eventables_.size() << std::endl;
      std::cerr << ss.str();
    }
#endif

    // Shutdown if there are no more eventables.
    if ( eventables_.size() == 0 ) {
      running_ = false;
    }
  }

  void loop::stop_all_eventables()
  {
    for ( auto& eventable : eventables() ) {
      stop(eventable);
    }
  }

  void loop::worker_loop()
  {
    struct epoll_event ev;

    while( running_ )
    {
      int nfds = epoll_wait(os_handle_, &ev, 1, 1000);

      if ( nfds < 0 && errno != EINTR )
      {
        throw std::system_error(errno, std::system_category());
      }
      else if ( nfds > 0 )
      {
        auto event = reinterpret_cast<event_data *>(ev.data.ptr);
        if ( event ) {
          event->dispatch(ev.events);
        }
      }
    }
  }
}
