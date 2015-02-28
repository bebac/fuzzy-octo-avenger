// ----------------------------------------------------------------------------
//
//     Filename   : source_local.h
//
//     Author     : Benny Bach <benny.bach@gmail.com>
//                  Copyright (C) 2014
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#ifndef __dm__source_local_h__
#define __dm__source_local_h__

// ----------------------------------------------------------------------------
#include "kvstore.h"

// ----------------------------------------------------------------------------
#include <json/json.h>

// ----------------------------------------------------------------------------
namespace dm
{
  class source_local
  {
  public:
    static void init(kvstore* store);
  public:
    source_local();
  public:
    std::vector<std::string> directories() const;
  public:
    void directories(const std::vector<std::string>& dirs);
  public:
    void scan();
  private:
    dm::track scan_flac_file(const std::string& filename);
  public:
    json::value to_json() const { return data_; }
  private:
    json::object data_;
  private:
    static kvstore* kvstore_;
  };
}

// ----------------------------------------------------------------------------
#endif // __dm__source_local_h__