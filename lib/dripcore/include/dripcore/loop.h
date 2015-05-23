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
#ifndef __dripcore__loop_h__
#define __dripcore__loop_h__

// ----------------------------------------------------------------------------
#include <dripcore/eventable.h>

// ----------------------------------------------------------------------------
#include <memory>
#include <set>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

// ----------------------------------------------------------------------------
namespace dripcore
{
  using os_handle_t = int;

  class loop
  {
    using eventable_container = std::set<std::shared_ptr<eventable>>;
    using lock_guard = std::lock_guard<std::mutex>;
  public:
    loop();
  public:
    void run();
    void shutdown();
  public:
    eventable_container eventables() const;
  public:
    void start(const std::shared_ptr<eventable>& eventable);
    void stop(const std::shared_ptr<eventable>& eventable);
    void stop_all_eventables();
  private:
    void worker_loop();
  private:
    os_handle_t os_handle_;
  private:
    eventable_container eventables_;
  private:
    std::atomic<bool>  running_;
    mutable std::mutex mutex_;
  private:
    std::vector<std::thread> workers_;
  };
} // dripcore

// ----------------------------------------------------------------------------
#endif // __dripcore__loop_h__
