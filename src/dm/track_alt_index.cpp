// ----------------------------------------------------------------------------
//
//     Filename   : kvstore.cpp
//
//     Author     : Benny Bach <benny.bach@gmail.com>
//                  Copyright (C) 2014
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#include "track_alt_index.h"

// ----------------------------------------------------------------------------
#include <stdexcept>

// ----------------------------------------------------------------------------
namespace dm
{
  track_alt_index::track_alt_index()
  {
    using namespace kyotocabinet;

    if (!db_.open()) {
      throw std::runtime_error("track_alt_index open error");
    }
  }

  track_alt_index::~track_alt_index()
  {
    db_.close();
  }

  bool track_alt_index::set(const std::string& key, const std::string& track_id)
  {
    return db_.set(key, track_id);
  }

  bool track_alt_index::get(const std::string& key, std::string *value)
  {
    return db_.get(key, value);
  }
}