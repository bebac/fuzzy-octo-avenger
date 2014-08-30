// ----------------------------------------------------------------------------
//
//     Filename   : cmdque.h
//
//     Author     : Benny Bach <benny.bach@gmail.com>
//                  Copyright (C) 2014
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#ifndef __cmdque_h__
#define __cmdque_h__

// ----------------------------------------------------------------------------
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <iostream>

// ----------------------------------------------------------------------------
class cmdque_t
{
public:
  cmdque_t()
  {
  };
private:
  cmdque_t(const cmdque_t& other) = delete;
public:
  virtual ~cmdque_t()
  {
  };
public:
  void push(std::function<void()>&& command)
  {
    std::lock_guard<std::mutex> _(lock_);
    q_.push(std::move(command));
    rdy_.notify_all();
  }
public:
  std::function<void()> pop(std::chrono::milliseconds wait_for, std::function<void()> timeout_cb=[]{})
  //std::function<void()> pop()
  {
    std::unique_lock<std::mutex> _(lock_);

    if ( q_.empty() )
    {
      rdy_.wait_for(_, wait_for);
    }

    if ( q_.empty() )
    {
      return timeout_cb;
    }
    else
    {
      std::function<void()> command = std::move(q_.front());
      q_.pop();
      return std::move(command);
    }
  }
private:
  std::mutex lock_;
  std::condition_variable rdy_;
  std::queue<std::function<void()>> q_;
};

// ----------------------------------------------------------------------------
#endif // __cmdque_h__
