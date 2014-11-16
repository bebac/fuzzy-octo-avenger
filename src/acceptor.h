// ----------------------------------------------------------------------------
//
//     Filename   : acceptor.h
//
//     Author     : Benny Bach <benny.bach@gmail.com>
//                  Copyright (C) 2014
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#ifndef __acceptor_h__
#define __acceptor_h__

// ----------------------------------------------------------------------------
#include <dripcore.h>
#include <basic_socket.h>

// ----------------------------------------------------------------------------
#include <iostream>
#include <cstring>

// ----------------------------------------------------------------------------
namespace jsonrpc
{
  namespace server
  {
    class acceptor : public dripcore::eventable
    {
      using connection_func = std::function<void(basic_socket socket)>;
    public:
      acceptor(connection_func connection_cb)
        :
        eventable(),
        socket_(inet::ipv4::tcp::socket()),
        connection_cb_(connection_cb)
      {
        struct sockaddr_in sock_addr;

        std::memset(&sock_addr, 0, sizeof(sock_addr));

        sock_addr.sin_family = AF_INET;
        sock_addr.sin_port = htons(8212);
        sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);

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
    private:
      basic_socket socket_;
      connection_func connection_cb_;
    };
  } // namespace server
} // namespace jsonrpc

#endif // __acceptor_h__
