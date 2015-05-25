// ----------------------------------------------------------------------------
//
//     Filename   : dripcore/connection.h
//
//     Author     : Benny Bach <benny.bach@gmail.com>
//                  Copyright (C) 2015
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#ifndef __dripcore__connection_h__
#define __dripcore__connection_h__

// ----------------------------------------------------------------------------
#include <dripcore/socket.h>
#include <dripcore/context.h>
#include <dripcore/eventable.h>

// ----------------------------------------------------------------------------
namespace dripcore
{
  class connection : public eventable
  {
  public:
    connection(dripcore::socket socket);
  public:
    virtual ~connection();
  protected:
    virtual void receive_data(const char* buf, size_t len) = 0;
    virtual void read();
  protected:
    virtual void send(const char* buf, size_t len);
    virtual void write();
  protected:
    // Output buffer management.
    virtual size_t      obuf_len() = 0;
    virtual const char* obuf_ptr() = 0;
    virtual void        obuf_write(const char* buf, size_t len) = 0;
    virtual void        obuf_sent(size_t len) = 0;
  public:
    int get_os_handle() const
    {
      return socket_.get_os_handle();
    }
  public:
    dripcore::context& get_context()
    {
      return context_;
    }
  protected:
    dripcore::context context_;
    dripcore::socket  socket_;
  };
}

// ----------------------------------------------------------------------------
#endif // __dripcore__connection_h__
