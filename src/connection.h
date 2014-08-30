// ----------------------------------------------------------------------------
//
//     Filename   : connection.h
//
//     Author     : Benny Bach <benny.bach@gmail.com>
//                  Copyright (C) 2014
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#ifndef __connection_h__
#define __connection_h__

// ----------------------------------------------------------------------------
#include <dripcore.h>
#include <basic_socket.h>
#include <json/json.h>
#include <json_rpc.h>

// ----------------------------------------------------------------------------
#include <iostream>
#include <sstream>
#include <cstring>

// ----------------------------------------------------------------------------
namespace jsonrpc
{
  namespace server
  {
    class connection : public dripcore::eventable
    {
    public:
      connection(jsonrpc::service& service, basic_socket socket)
        :
        eventable(),
        service_(service),
        socket_(std::move(socket)),
        value_(),
        parser_(value_)
      {
        std::cout << "connection " << this << std::endl;
        set_rd_handler(std::bind(&connection::read, this));
        set_wr_handler(std::bind(&connection::write, this));
      }
    protected:
      virtual void started(dripcore::loop* loop)
      {
        dripcore::eventable::started(loop);
        service_.attach_connection(std::dynamic_pointer_cast<connection>(ptr()));
      }
    protected:
      virtual void stopped(dripcore::loop* loop)
      {
        service_.detach_connection(std::dynamic_pointer_cast<connection>(ptr()));
        dripcore::eventable::stopped(loop);
      }
    public:
      ~connection()
      {
        socket_.close();
        std::cout << "~connection " << this << std::endl;
      }
    public:
      void send_notification(json_rpc_notification notification)
      {
        obuf_.append(to_string(notification));
        obuf_.append(1, '\0');
        write();
      }
    protected:
      void read()
      {
        std::array<char, 1024> buf;

        //std::cout << "read_handler" << std::endl;

        ssize_t res;
        do
        {
          res = socket_.recv(buf.data(), buf.size(), 0);

          if ( res > 0 )
          {
            process_data(buf.data(), res);
          }
          else if ( res < 0 )
          {
            // TODO: Handle error.
            std::cerr << "connection read error " << strerror(errno) << std::endl;
          }
          else
          {
            stop();
          }
        }
        while ( res != 0 && res == buf.size() );
      }
    protected:
      void process_data(const char* data, size_t len)
      {
        size_t consumed = 0;
        do
        {
          consumed += parser_.parse(data+consumed, len-consumed);

          if ( parser_.complete() )
          {
            if ( data[consumed] == '\0' ) {
              consumed += 1; // Discard zero terminator.
            }

            process_json_value(std::move(value_));

            // Start on a new value.
            parser_.reset();
          }
        }
        while ( consumed < len );
      }
    protected:
      void process_json_value(json::value value)
      {
        auto request = json_rpc_request::from_json(value);

        //std::cerr << "method=" << req.method() << ", params=" << req.params() << std::endl;

        service_.execute_async(request, [this](json_rpc_response response) {
            obuf_.append(to_string(response));
            obuf_.append(1, '\0');
            write();
          });
      }
    protected:
      void write()
      {
        //std::cout << "write obuf_ len=" << obuf_.length() << ", capacity=" << obuf_.capacity() << std::endl;

        if ( obuf_.length() == 0 )
          return;

        ssize_t sent = socket_.send(obuf_.data(), obuf_.length(), 0);

        //std::cerr << "sent=" << sent << std::endl;

        if ( sent >= 0 )
        {
          /////
          // NOTE
          // I assume erase(0, obuf_.length()) is as efficient as clear. If not
          // maybe it's better to check if sent == obuf_.length and call clear.

          obuf_.erase(0, sent);
          obuf_.reserve(1024);
        }
        else
        {
          // TODO: Handle send error, EWOULDBLOCK, EAGAIN etc.
          std::cerr << "connection write error " << strerror(errno) << std::endl;
        }
      }
    public:
      int get_os_handle() const
      {
        return socket_.get_os_handle();
      }
    private:
      jsonrpc::service& service_;
    private:
      basic_socket socket_;
      json::value  value_;
      json::parser parser_;
      std::string  obuf_;
    };
  } // namespace server
} // namespace jsonrpc

#endif // __acceptor_h__
