// ----------------------------------------------------------------------------
//
//     Filename   : kvstore.h
//
//     Author     : Benny Bach <benny.bach@gmail.com>
//                  Copyright (C) 2014
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#ifndef __dm__kvstore_h__
#define __dm__kvstore_h__

// ----------------------------------------------------------------------------
//#include "collection.h"
#include <json/json.h>

// ----------------------------------------------------------------------------
#include <kchashdb.h>

// ----------------------------------------------------------------------------
#include <string>

// ----------------------------------------------------------------------------
namespace dm
{
  class kvstore
  {
  public:
    kvstore(const std::string filename);
  public:
    ~kvstore();
  public:
    int64_t count();
  public:
    bool set(const std::string& key, const json::value& value);
    bool remove(const std::string& key);
  public:
    json::value get(const std::string& key);
  public:
    void each(std::function<bool(const std::string&)> key_match, std::function<bool(json::value&)> value_cb);
  public:
    std::string create_artist_key();
    std::string create_album_key();
    std::string create_track_key();
  private:
    kyotocabinet::HashDB db_;
  };
}

// ----------------------------------------------------------------------------
#endif // __dm__kvstore_h__
