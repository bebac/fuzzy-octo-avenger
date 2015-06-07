// ----------------------------------------------------------------------------
//
//     Filename   : local_source.h
//
//     Author     : Benny Bach <benny.bach@gmail.com>
//                  Copyright (C) 2014
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#ifndef __local_source_h__
#define __local_source_h__

// ----------------------------------------------------------------------------
#include "audio_output_alsa.h"
#include "source_base.h"

// ----------------------------------------------------------------------------
class local_source : public source_base
{
  using audio_output_ptr = std::weak_ptr<audio_output_t>;
public:
  local_source()
  {
  }
public:
  virtual void play(const std::string& uri, audio_output_ptr audio_output);
public:
  json::value get_cover(const std::string& uri);
};

// ----------------------------------------------------------------------------
#if 0
void import_flac_file(const std::string& filename);
#endif

// ----------------------------------------------------------------------------
#endif // __local_source_h__