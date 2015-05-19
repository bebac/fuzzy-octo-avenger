// ----------------------------------------------------------------------------
//
//     Filename   : dripcore/acceptor.h
//
//     Author     : Benny Bach <benny.bach@gmail.com>
//                  Copyright (C) 2015
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#ifndef __dripcore__acceptor_h__
#define __dripcore__acceptor_h__

// ----------------------------------------------------------------------------
#include <dripcore/eventable.h>
#include <dripcore/context.h>
#include <dripcore/socket.h>

// ----------------------------------------------------------------------------
#include <iostream>
#include <cstring>

// ----------------------------------------------------------------------------
namespace dripcore
{
  class acceptor : public dripcore::eventable
  {
    using connection_func = std::function<void(socket socket)>;
  public:
    acceptor(const char* ip, unsigned port, connection_func connection_cb)
      :
      eventable(),
      socket_(ipv4::tcp::socket()),
      connection_cb_(connection_cb)
    {
      struct sockaddr_in sock_addr;

      std::memset(&sock_addr, 0, sizeof(sock_addr));

      sock_addr.sin_family = AF_INET;
      sock_addr.sin_port = htons(port);

      auto res = inet_pton(AF_INET, ip, &sock_addr.sin_addr);

      if ( res == 0 ) {
        throw std::runtime_error("invalid ip address");
      }
      else if ( res < 0 ) {
        throw std::system_error(errno, std::system_category());
      }

      socket_.nonblocking(true);
      socket_.reuseaddr(true);
      socket_.bind((struct sockaddr *)&sock_addr, sizeof(sock_addr));
      socket_.listen(5);

      set_rd_handler(std::bind(&acceptor::accept_handler, this));
   }
  protected:
    virtual void stopped(dripcore::loop* loop)
    {
      socket_.close();
      dripcore::eventable::stopped(loop);
    }
  protected:
    void accept_handler()
    {
      bool done = false;

      do
      {
        auto client = socket_.accept(0, 0);

        if ( client.ok() )
        {
          client.nonblocking(true);

          connection_cb_(std::move(client));
        }
        else if ( client.last_error() == EWOULDBLOCK ||
                  client.last_error() == EAGAIN )
        {
          done = true;
        }
        else
        {
          // TODO: Handle error.
          std::cerr << "accept error " << strerror(errno) << std::endl;
          done = true;
        }
      }
      while ( !done );
    }
  public:
    int get_os_handle() const
    {
      return socket_.get_os_handle();
    }
  public:
    context& get_context()
    {
      return context_;
    }
  private:
    context context_;
    socket socket_;
    connection_func connection_cb_;
  };
} // dripcore

#endif // __dripcore__acceptor_h__
