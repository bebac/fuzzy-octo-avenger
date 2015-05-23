// ----------------------------------------------------------------------------
//
//     Filename   : dripcore/eventable.h
//
//     Author     : Benny Bach <benny.bach@gmail.com>
//                  Copyright (C) 2015
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#ifndef __dripcore__eventable_h__
#define __dripcore__eventable_h__

// ----------------------------------------------------------------------------
#include <dripcore/event_data.h>
#include <dripcore/context.h>

// ----------------------------------------------------------------------------
#include <memory>
#include <mutex>

// ----------------------------------------------------------------------------
namespace dripcore
{
  class loop;

  class eventable : public std::enable_shared_from_this<eventable>
  {
    friend class loop;
    friend class event_data;
  public:
    using callback = std::function<void()>;
  public:
    eventable() : loop_(nullptr) {}
  public:
    std::shared_ptr<eventable> ptr() { return shared_from_this(); }
  public:
    virtual int get_os_handle() const = 0;
  public:
    virtual context& get_context() = 0;
  public:
    bool has_rd_handler() const noexcept { return rd_handler_ ? true : false; }
    bool has_wr_handler() const noexcept { return wr_handler_ ? true : false; }
  public:
    void set_rd_handler(callback callback) { rd_handler_ = callback; }
    void set_wr_handler(callback callback) { wr_handler_ = callback; }
  private:
    void call_rd_handler();
    void call_wr_handler();
  public:
    void stop();
  protected:
    virtual void started(loop* loop);
    virtual void stopped(loop* loop);
  protected:
    loop& get_loop();
  public:
    std::shared_ptr<event_data> event_data_ptr();
  private:
    loop* loop_;
  private:
    callback rd_handler_;
    callback wr_handler_;
  private:
    std::shared_ptr<event_data> event_data_;
  };
}

// ----------------------------------------------------------------------------
#endif // __dripcore__eventable_h__
