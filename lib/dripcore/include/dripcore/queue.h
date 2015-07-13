// ----------------------------------------------------------------------------
//
//     Filename   : dripcore/queue.h
//
//     Author     : Benny Bach <benny.bach@gmail.com>
//                  Copyright (C) 2015
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#ifndef __dripcore__queue_h__
#define __dripcore__queue_h__

// ----------------------------------------------------------------------------
#include <dripcore/eventable.h>

// ----------------------------------------------------------------------------
#include <queue>

// ----------------------------------------------------------------------------
#include <unistd.h>
#include <sys/eventfd.h>

// ----------------------------------------------------------------------------
namespace dripcore
{
  class queue : public eventable
  {
  public:
    queue(context& context) : context_(context)
    {
      if ( (fd_ = eventfd(0, EFD_NONBLOCK)) == -1 ) {
        throw std::system_error(errno, std::system_category());
      }
      set_rd_handler(std::bind(&queue::read_handler, this));
    }
  public:
    void push(std::function<void()>&& command)
    {
      std::lock_guard<std::mutex> _(lock_);
      q_.push(std::move(command));
      notify_one();
    }
  public:
    void read_handler()
    {
      uint64_t v;

      ssize_t res = read(fd_, &v, sizeof(uint64_t));

      if ( res == sizeof(uint64_t) )
      {
        std::lock_guard<std::mutex> _(lock_);

        while ( v-- > 0 )
        {
          q_.front()();
          q_.pop();
        }
      }
      else if ( res < 0 && errno == EAGAIN )
      {
      }
      else
      {
        throw std::system_error(errno, std::system_category());
      }
    }
  public:
    int get_os_handle() const
    {
      return fd_;
    }
  public:
    dripcore::context& get_context()
    {
      return context_;
    }
  private:
    void notify_one()
    {
      uint64_t v = 1;
      if ( write(fd_, &v, sizeof(uint64_t)) < 0 )
      {
        throw std::system_error(errno, std::system_category());
      }
    }
  private:
    context& context_;
    std::mutex lock_;
    int fd_;
    std::queue<std::function<void()>> q_;
  };
} // namespace dripcore

// ----------------------------------------------------------------------------
#endif // __dripcore__queue_h__
