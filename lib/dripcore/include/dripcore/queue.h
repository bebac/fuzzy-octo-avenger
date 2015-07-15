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
namespace dripcore
{
  class queue : public eventable
  {
    using container  = std::queue<std::function<void()>>;
    using mutex      = std::mutex;
    using lock_guard = std::lock_guard<mutex>;
  public:
    queue(context& context);
  public:
    void push(std::function<void()>&& command);
  public:
    int get_os_handle() const final;
  public:
    dripcore::context& get_context() final { return context_; }
  private:
    void read_handler();
    void notify_one();
  private:
    context&  context_;
    mutex     lock_;
    int       fd_;
    container q_;
  };
} // namespace dripcore

// ----------------------------------------------------------------------------
#endif // __dripcore__queue_h__
