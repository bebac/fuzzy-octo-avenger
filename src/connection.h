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
#include <dripcore/loop.h>
#include <dripcore/eventable.h>
#include <dripcore/socket.h>
#include <dripcore/context.h>
#include <dripcore/connection.h>
#include <dripcore/queue.h>
#include <json/json.h>
#include <json_rpc.h>

// ----------------------------------------------------------------------------
#include <iostream>

// ----------------------------------------------------------------------------
namespace jsonrpc
{
  namespace server
  {
    class connection : public dripcore::connection
    {
    public:
      connection(jsonrpc::service& service, dripcore::socket socket)
        :
        dripcore::connection(std::move(socket)),
        service_(service),
        value_(),
        parser_(value_),
        queue_(std::make_shared<dripcore::queue>(context_))
      {
        std::cout << "connection " << this << std::endl;
      }
    protected:
      virtual void started(dripcore::loop* loop)
      {
        dripcore::connection::started(loop);
        get_loop().start(queue_);
        service_.attach_connection(std::dynamic_pointer_cast<connection>(ptr()));
      }
    protected:
      virtual void stopped(dripcore::loop* loop)
      {
        service_.detach_connection(std::dynamic_pointer_cast<connection>(ptr()));
        get_loop().stop(queue_);
        dripcore::connection::stopped(loop);
      }
    public:
      ~connection()
      {
        std::cout << "~connection " << this << std::endl;
      }
    public:
      void send_notification(const json_rpc_notification& notification)
      {
        queue_->push([=]()
        {
          auto buf = to_string(notification).append(1, '\0');
          send(buf.data(), buf.length());
        });
      }
    protected:
      void receive_data(const char* data, size_t len)
      {
        size_t consumed = 0;
        do
        {
          // Skip leftover zero terminator(s) and stop processing if we only
          // got zeros.
          while ( data[consumed] == '\0' )
          {
            if ( ++consumed >= len ) {
              return;
            }
          }

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

        auto response = service_.execute(request);
        auto buf = to_string(response).append(1, '\0');

        send(buf.data(), buf.length());
      }
    private:
      size_t obuf_len()
      {
        return obuf_.length();
      }
    private:
      const char* obuf_ptr()
      {
        return obuf_.data();
      }
    private:
      void obuf_write(const char* buf, size_t len)
      {
        obuf_.append(buf, len);
        std::cerr << "connection obuf_write len=" << len << std::endl;
      }
    private:
      void obuf_sent(size_t len)
      {
        obuf_.erase(0, len);
      }
    private:
      jsonrpc::service& service_;
      json::value       value_;
      json::parser      parser_;
      std::string       obuf_;
    private:
      std::shared_ptr<dripcore::queue> queue_;
    };
  } // namespace server
} // namespace jsonrpc

#endif // __connection_h__
