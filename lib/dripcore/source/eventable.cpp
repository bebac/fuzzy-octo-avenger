// ----------------------------------------------------------------------------
//
//     Filename   : dripcore/eventable.cpp
//
//     Author     : Benny Bach <benny.bach@gmail.com>
//                  Copyright (C) 2015
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#include <dripcore/eventable.h>
#include <dripcore/loop.h>

// ----------------------------------------------------------------------------
#include <cassert>

// ----------------------------------------------------------------------------
namespace dripcore
{
  void eventable::call_rd_handler()
  {
    assert(rd_handler_);
    rd_handler_();
  }

  void eventable::call_wr_handler()
  {
    assert(wr_handler_);
    wr_handler_();
  }

  void eventable::stop()
  {
    get_loop().stop(ptr());
  }

  void eventable::started(loop* loop)
  {
    loop_ = loop;
  }

  void eventable::stopped(loop* loop)
  {
    loop_ = nullptr;
  }

  dripcore::loop& eventable::get_loop()
  {
    assert(loop_);
    return *loop_;
  }

  std::shared_ptr<event_data> eventable::event_data_ptr()
  {
    if ( !event_data_ )
    {
      event_data_ = std::make_shared<event_data>(ptr());
    }
    return event_data_;
  }
}