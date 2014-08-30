// ----------------------------------------------------------------------------
//
//     Filename   : source_base.h
//
//     Author     : Benny Bach <benny.bach@gmail.com>
//                  Copyright (C) 2014
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#ifndef __source_base_h__
#define __source_base_h__

// ----------------------------------------------------------------------------
#include <json/json.h>

// ----------------------------------------------------------------------------
#include "audio_output_alsa.h"

// ----------------------------------------------------------------------------
class source_base
{
  using audio_output_ptr = std::weak_ptr<audio_output_t>;
public:
  virtual void play(const std::string& uri, audio_output_ptr audio_output) = 0;
public:
  virtual json::value get_cover(const std::string& uri) = 0;
public:
  bool is_scanable()
  {
    return false;
  }
public:
  virtual json::array scan()
  {
    return json::array();
  }
};

// ----------------------------------------------------------------------------
#endif // __source_base_h__
