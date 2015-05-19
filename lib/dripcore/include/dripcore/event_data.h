// ----------------------------------------------------------------------------
//
//     Filename   : dripcore/event_data.h
//
//     Author     : Benny Bach <benny.bach@gmail.com>
//                  Copyright (C) 2015
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#ifndef __dripcore__event_data_h__
#define __dripcore__event_data_h__

// ----------------------------------------------------------------------------
#include <memory>

// ----------------------------------------------------------------------------
namespace dripcore
{
  class eventable;

  class event_data
  {
  public:
    event_data(std::shared_ptr<eventable> eventable_ptr);
  public:
    void dispatch(unsigned events);
  public:
    std::weak_ptr<eventable> eventable_ptr_;
  };
}

// ----------------------------------------------------------------------------
#endif // __dripcore__event_data_h__
