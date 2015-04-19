// ----------------------------------------------------------------------------
//
//     Filename   : track_alt_index.h
//
//     Author     : Benny Bach <benny.bach@gmail.com>
//                  Copyright (C) 2014
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#ifndef __dm__track_alt_index_h__
#define __dm__track_alt_index_h__

// ----------------------------------------------------------------------------
#include <kcdbext.h>

// ----------------------------------------------------------------------------
#include <string>

// ----------------------------------------------------------------------------
namespace dm
{
  class track_alt_index
  {
  public:
    track_alt_index();
  public:
    ~track_alt_index();
  public:
    int64_t count();
  public:
    bool set(const std::string& key, const std::string& track_id);
    //bool remove(const std::string& key);
  public:
    bool get(const std::string& key, std::string* value);
  public:
  private:
    kyotocabinet::IndexDB db_;
  };
}

// ----------------------------------------------------------------------------
#endif // __dm__track_alt_index_h__
