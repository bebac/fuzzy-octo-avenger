// ----------------------------------------------------------------------------
//
//     Filename   : dripcore/context.h
//
//     Author     : Benny Bach <benny.bach@gmail.com>
//                  Copyright (C) 2015
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#ifndef __dripcore__context_h__
#define __dripcore__context_h__

// ----------------------------------------------------------------------------
#include <mutex>

// ----------------------------------------------------------------------------
namespace dripcore
{
  class context
  {
  public:
    using lock_guard = std::lock_guard<context>;
  public:
    void lock()   { mutex_.lock(); }
    void unlock() { mutex_.unlock(); }
  private:
    std::mutex mutex_;
  };
}

// ----------------------------------------------------------------------------
#endif // __dripcore__context_h__
