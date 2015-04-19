// ----------------------------------------------------------------------------
//
//     Filename   : json_rpc.cpp
//
//     Author     : Benny Bach <benny.bach@gmail.com>
//                  Copyright (C) 2014
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#include <json_rpc.h>
#include <connection.h>

// ----------------------------------------------------------------------------
#include <iostream>

// ----------------------------------------------------------------------------
namespace jsonrpc
{
  service::service(dripcore::loop& loop)
    :
    res_q_(new response_queue),
    running_(true)
  {
    // Start response queue.
    loop.start(res_q_);
    // Start worker threads.
    workers_.emplace_back(std::thread{&service::work_loop, this});
    workers_.emplace_back(std::thread{&service::work_loop, this});
  }

  service::~service()
  {
    running_ = false;

    // Wake up the workers.
    for ( size_t i=0; i<workers_.size(); i++ ) {
      req_q_.push([]{});
    }

    // Wait for workers to end.
    for ( auto& thr : workers_ ) {
      thr.join();
    }
  }

  void service::execute_async(const json_rpc_request& request, std::function<void(json_rpc_response)> completed)
  {
    auto it = methods_.find(request.method());

    if ( it != end(methods_) )
    {
      req_q_.push([=] {
        res_q_->push(std::bind(std::move(completed), (*it).second(std::move(request))));
      });
    }
    else
    {
      json_rpc_response response{request};
      response.method_not_found();
      completed(response);
    }
  }

  void service::send_notification(json_rpc_notification notification)
  {
    for ( auto& conn : connections_ )
    {
      if ( !conn.expired() )
      {
        res_q_->push(std::bind(&server::connection::send_notification, conn.lock(), notification));
      }
    }
  }

  void service::attach_connection(std::shared_ptr<server::connection> connection)
  {
    connections_.insert(connection);
  }

  void service::detach_connection(std::shared_ptr<server::connection> connection)
  {
    connections_.erase(connection);
  }

  void service::work_loop()
  {
    while ( running_ )
    {
      auto cmd = req_q_.pop();
      cmd();
    }
  }
}
