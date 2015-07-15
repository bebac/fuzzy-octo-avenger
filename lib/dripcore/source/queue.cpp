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
#include <dripcore/queue.h>

// ----------------------------------------------------------------------------
#include <unistd.h>
#include <sys/eventfd.h>

// ----------------------------------------------------------------------------
namespace dripcore
{
  queue::queue(context& context) : context_(context)
  {
    if ( (fd_ = eventfd(0, EFD_NONBLOCK)) == -1 ) {
      throw std::system_error(errno, std::system_category());
    }
    set_rd_handler(std::bind(&queue::read_handler, this));
  }

  void queue::push(std::function<void()>&& command)
  {
    lock_guard _(lock_);
    q_.push(std::move(command));
    notify_one();
  }

  void queue::read_handler()
  {
    uint64_t v;

    ssize_t res = read(fd_, &v, sizeof(uint64_t));

    if ( res == sizeof(uint64_t) )
    {
      lock_guard _(lock_);

      while ( v-- > 0 )
      {
        q_.front()();
        q_.pop();
      }
    }
    else if ( res < 0 && errno == EAGAIN ) {
    }
    else {
      throw std::system_error(errno, std::system_category());
    }
  }

  int queue::get_os_handle() const
  {
    return fd_;
  }

  void queue::notify_one()
  {
    uint64_t v = 1;
    if ( write(fd_, &v, sizeof(uint64_t)) < 0 ) {
      throw std::system_error(errno, std::system_category());
    }
  }
} // namespace dripcore
