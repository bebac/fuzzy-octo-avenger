// ----------------------------------------------------------------------------
//
//     Filename   : json_rpc.h
//
//     Author     : Benny Bach <benny.bach@gmail.com>
//                  Copyright (C) 2014
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#ifndef __json_rpc_h__
#define __json_rpc_h__

// ----------------------------------------------------------------------------
#include <dripcore/loop.h>
#include <dripcore/eventable.h>
#include <json/json.h>

// ----------------------------------------------------------------------------
#include <string>
#include <cstring>
#include <thread>
#include <functional>
#include <map>
#include <queue>
#include <condition_variable>
#include <atomic>

// ----------------------------------------------------------------------------
#include <unistd.h>
#include <sys/eventfd.h>

// ----------------------------------------------------------------------------
class json_rpc_request
{
public:
  json_rpc_request()
    :
    error_code_(0)
  {
  }
public:
  const std::string& version() const { return version_; }
  const std::string& method()  const { return method_;  }
  const json::value& params()  const { return params_;  }
  const json::value& id()      const { return id_;      }
public:
  bool is_valid() const { return error_code_ == 0; }
public:
  int error_code() const { return error_code_; }
public:
  static json_rpc_request from_json(json::value& v)
  {
    json_rpc_request self;

    if ( v.is_object() )
    {
      auto& o = v.as_object();

      if ( !o["jsonrpc"].is_string() ) {
        self.error_code_ = -32600;
      }

      if ( !o["method"].is_string() ) {
        self.error_code_ = -32600;
      }

      if ( !o["params"].is_null() ) {
        self.params_ = o["params"];
      }
      else {
        self.params_ = json::value();
      }

      if ( !o["id"].is_null() ) {
        self.id_ = o["id"];
      }

      if ( self.is_valid() )
      {
        self.version_ = o["jsonrpc"].as_string();
        self.method_ = o["method"].as_string();
      }
    }
    else
    {
      self.error_code_ = -32600;
    }
    return std::move(self);
  }
private:
  std::string version_;
  std::string method_;
  json::value params_;
  json::value id_;
  // If error != 0 none of the above will be valid.
  int error_code_;
};

// ----------------------------------------------------------------------------
class json_rpc_response
{
  friend std::string to_string(const json_rpc_response& response);
public:
  json_rpc_response(const json_rpc_request& request)
    :
    object_{ { "jsonrpc", "2.0" }, { "id", request.id() } }
  {
    if ( !request.is_valid() )
    {
      error(request.error_code(), "Invalid Request");
    }
  }
public:
  void set_result(json::value value)
  {
    object_["result"] = std::move(value);
  }
public:
  void error(int error_code, std::string error_message)
  {
    object_["error"] = json::object{
      { "code", error_code },
      { "message", std::move(error_message) }
    };
  }
public:
  void method_not_found()
  {
    error(-32601, "Method not found");
  }
public:
  void invalid_params()
  {
    error(-32602, "Invalid params");
  }
private:
  json::object object_;
};

// ----------------------------------------------------------------------------
inline std::string to_string(const json_rpc_response& response)
{
  return to_string(response.object_);
}

// ----------------------------------------------------------------------------
class json_rpc_notification
{
  friend std::string to_string(const json_rpc_notification& notification);
public:
  json_rpc_notification(const std::string& method, json::value params)
    :
    object_{ { "jsonrpc", "2.0" }, { "method", method }, { "params", params } }
    {
    }
private:
  json::object object_;
};

// ----------------------------------------------------------------------------
inline std::string to_string(const json_rpc_notification& notification)
{
  return to_string(notification.object_);
}

// ----------------------------------------------------------------------------
namespace jsonrpc
{
  namespace server
  {
    class connection;
  }

  class service
  {
    using method_func = std::function<json_rpc_response(const json_rpc_request& request)>;
  public:
    service();
  public:
    ~service();
  public:
    void add_method(const std::string& name, method_func method)
    {
      methods_.emplace(std::move(name), std::move(method));
    }
  public:
    json_rpc_response execute(const json_rpc_request& request);
  public:
    void send_notification(json_rpc_notification notification);
  public:
    void attach_connection(std::shared_ptr<server::connection> connection);
    void detach_connection(std::shared_ptr<server::connection> connection);
  private:
    using method_map_t = std::map<std::string, method_func>;
    using connection_ptr = std::weak_ptr<server::connection>;
    using connection_container = std::set<connection_ptr, std::owner_less<connection_ptr>>;
  private:
    method_map_t             methods_;
    connection_container     connections_;
  };
}

// ----------------------------------------------------------------------------
#endif // __json_rpc_h__
