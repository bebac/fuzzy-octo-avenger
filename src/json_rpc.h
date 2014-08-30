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
#include <dripcore.h>
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
  //json_rpc_response(json_rpc_request& request)
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
  class request_queue
  {
  public:
    request_queue()
    {
    };
  private:
    request_queue(const request_queue& other) = delete;
  public:
    virtual ~request_queue()
    {
    };
  public:
    void push(std::function<void()>&& command)
    {
      std::lock_guard<std::mutex> _(lock);
      q.push(std::move(command));
      rdy.notify_one();
    }
  public:
    std::function<void()> pop()
    {
      std::unique_lock<std::mutex> _(lock);

      if ( q.empty() ) {
        rdy.wait(_);
      }

      if ( q.empty() ) {
        return []{};
      }
      else
      {
        std::function<void()> c = std::move(q.front());
        q.pop();
        return std::move(c);
      }
    }
  private:
    std::mutex lock;
    std::condition_variable rdy;
    std::queue<std::function<void()>> q;
  };

  class response_queue : public dripcore::eventable
  {
  public:
    response_queue()
    {
      if ( (fd_ = eventfd(0, 0)) == -1 ) {
        throw std::system_error(errno, std::system_category());
      }
      set_rd_handler(std::bind(&response_queue::read_handler, this));
    }
  public:
    void push(std::function<void()>&& command)
    {
      std::lock_guard<std::mutex> _(lock_);
      q_.push(std::move(command));
      notify_one();
    }
  public:
    void read_handler()
    {
      uint64_t v;

      ssize_t res = read(fd_, &v, sizeof(uint64_t));

      if ( res == sizeof(uint64_t) )
      {
        std::lock_guard<std::mutex> _(lock_);

        while ( v-- > 0 )
        {
          q_.front()();
          q_.pop();
        }
      }
      else
      {
        throw std::system_error(errno, std::system_category());
      }
    }
  public:
    int get_os_handle() const
    {
      return fd_;
    }
  private:
    void notify_one()
    {
      uint64_t v = 1;
      if ( write(fd_, &v, sizeof(uint64_t)) < 0 )
      {
        throw std::system_error(errno, std::system_category());
      }
    }
  private:
    std::mutex lock_;
    int fd_;
    std::queue<std::function<void()>> q_;
  };

  namespace server
  {
    class connection;
  }

  class service
  {
    using method_func = std::function<json_rpc_response(const json_rpc_request& request)>;
    //using method_func = std::function<json_rpc_response(json_rpc_request& request)>;
    //using notify_func = std::function<void(json_rpc_notification)>;
  public:
    service(dripcore::loop& loop);
  public:
    ~service();
  public:
    void add_method(const std::string& name, method_func method)
    {
      methods_.emplace(std::move(name), std::move(method));
    }
  public:
    void execute_async(const json_rpc_request& request, std::function<void(json_rpc_response)> completed);
  public:
    void send_notification(json_rpc_notification notification);
  public:
    void attach_connection(std::shared_ptr<server::connection> connection);
    void detach_connection(std::shared_ptr<server::connection> connection);
  private:
    void work_loop();
  private:
    using method_map_t = std::map<std::string, method_func>;
    using response_queue_ptr = std::shared_ptr<response_queue>;
    using connection_ptr = std::weak_ptr<server::connection>;
    using connection_container = std::set<connection_ptr, std::owner_less<connection_ptr>>;
  private:
    request_queue            req_q_;
    response_queue_ptr       res_q_;
    std::vector<std::thread> workers_;
    std::atomic<bool>        running_;
    method_map_t             methods_;
    connection_container     connections_;
  };
}

// ----------------------------------------------------------------------------
#endif // __json_rpc_h__
