// ----------------------------------------------------------------------------
//
//     Filename   : dripcore.h
//
//     Author     : Benny Bach <benny.bach@gmail.com>
//                  Copyright (C) 2014
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#ifndef __dripcore_h__
#define __dripcore_h__

// ----------------------------------------------------------------------------
#include <set>
#include <cassert>
#include <iostream>
#include <memory>
#include <system_error>

// ----------------------------------------------------------------------------
#include <sys/epoll.h>
#include <fcntl.h>

// ----------------------------------------------------------------------------
namespace dripcore
{
  using os_handle_t = int;

  class eventable;
  class loop;

  class event_data
  {
  public:
    event_data(std::shared_ptr<eventable> eventable_ptr)
      :
      eventable_ptr_(eventable_ptr)
    {
    }
  public:
    void dispatch(loop& loop, unsigned events);
  public:
    std::weak_ptr<eventable> eventable_ptr_;
  };

  class eventable : public std::enable_shared_from_this<eventable>
  {
    friend class loop;
    friend class event_data;
  private:
    using handler_callback = std::function<void()>;
  public:
    eventable() : loop_(nullptr)
    {
    }
  public:
    std::shared_ptr<event_data> event_data_ptr()
    {
      if ( !event_data_ )
      {
        event_data_ = std::make_shared<event_data>(ptr());
      }
      return event_data_;
    }
  public:
    void set_rd_handler(handler_callback callback)
    {
      rd_handler_ = callback;
    }
  public:
    void set_wr_handler(handler_callback callback)
    {
      wr_handler_ = callback;
    }
  public:
    bool has_rd_handler()
    {
      return rd_handler_ ? true : false;
    }
  public:
    bool has_wr_handler()
    {
      return wr_handler_ ? true : false;
    }
  public:
    std::shared_ptr<eventable> ptr()
    {
      return shared_from_this();
    }
  public:
    virtual int get_os_handle() const = 0;
  protected:
    virtual void started(loop* loop)
    {
      loop_ = loop;
    }
  protected:
    virtual void stopped(loop* loop)
    {
    }
  protected:
    dripcore::loop& get_loop()
    {
      assert(loop_);
      return *loop_;
    }
  protected:
    void stop();
  private:
    void call_rd_handler()
    {
      assert(rd_handler_);
      rd_handler_();
    }
  private:
    void call_wr_handler()
    {
      assert(wr_handler_);
      wr_handler_();
    }
  private:
    dripcore::loop*  loop_;
    handler_callback rd_handler_;
    handler_callback wr_handler_;
  private:
    std::shared_ptr<event_data> event_data_;
  };

  class loop
  {
  public:
    loop()
    {
      if ( (os_handle_ = epoll_create1(EPOLL_CLOEXEC)) < 0 ) {
        throw std::system_error(errno, std::system_category());
      }
    }
  public:
    void start(const std::shared_ptr<eventable>& eventable)
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

      eventables_.insert(eventable);
      eventable->started(this);
    }
  public:
    void stop(const std::shared_ptr<eventable>& eventable)
    {
      if ( epoll_ctl(os_handle_, EPOLL_CTL_DEL, eventable->get_os_handle(), nullptr) == -1 ) {
        throw std::system_error(errno, std::system_category());
      }

      eventable->stopped(this);
      eventables_.erase(eventable);
    }
  public:
    void run()
    {
      struct epoll_event events[10];

      while(1)
      {
        int nfds = epoll_wait(os_handle_, events, 10, -1);

        if ( nfds < 0 )
        {
          throw std::system_error(errno, std::system_category());
        }

        for ( int i=0; i<nfds; ++i )
        {
            auto event = reinterpret_cast<event_data *>(events[i].data.ptr);
            if ( event )
            {
                event->dispatch(*this, events[i].events);
            }
        }
      }
    }
  private:
    os_handle_t os_handle_;
    std::set<std::shared_ptr<eventable>> eventables_;
  };

  inline void eventable::stop()
  {
    get_loop().stop(ptr());
  }

  inline void event_data::dispatch(loop& loop, unsigned events)
  {
    if ( !eventable_ptr_.expired() )
    {
      auto eventable = eventable_ptr_.lock();

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
} // namespace dripcore

// ----------------------------------------------------------------------------
#endif // __dripcore_h__
