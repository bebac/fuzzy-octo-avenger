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
  service::service()
  {
  }

  service::~service()
  {
  }

  json_rpc_response service::execute(const json_rpc_request& request)
  {
    auto it = methods_.find(request.method());

    if ( it != end(methods_) )
    {
      return (*it).second(request);
    }
    else
    {
      json_rpc_response response{request};
      response.method_not_found();
      return response;
    }
  }

  void service::send_notification(json_rpc_notification notification)
  {
    lock_guard lock(mutex_);

    for ( auto& ptr : connections_ )
    {
      auto connection = ptr.lock();

      if ( connection )
      {
        connection->send_notification(notification);
      }
    }
  }

  void service::attach_connection(std::shared_ptr<server::connection> connection)
  {
    lock_guard lock(mutex_);
    connections_.insert(connection);
  }

  void service::detach_connection(std::shared_ptr<server::connection> connection)
  {
    lock_guard lock(mutex_);
    connections_.erase(connection);
  }
}
