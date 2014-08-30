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
#include "database.h"
#include "audio_output_alsa.h"
#include "source_base.h"

// ----------------------------------------------------------------------------
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

// ----------------------------------------------------------------------------
namespace file_system
{
  enum type
  {
    block_device,
    char_device,
    directory,
    fifo,
    symlink,
    file,
    socket,
    unknown
  };

  file_system::type file_type(const std::string& pathname);

  void scan_dir(const std::string& dirname, std::function<void(const std::string& filename)> callback);

  std::string extension(const std::string& filename);
}

// ----------------------------------------------------------------------------
class local_source : public source_base
{
  using audio_output_ptr = std::weak_ptr<audio_output_t>;
public:
  local_source(const std::string& dirname)
    :
    dirname_(dirname)
  {
  }
public:
  virtual void play(const std::string& uri, audio_output_ptr audio_output);
public:
  json::value get_cover(const std::string& uri);
public:
  bool is_scanable()
  {
    return true;
  }
public:
  virtual json::array scan();
private:
  std::string dirname_;
};

// ----------------------------------------------------------------------------
#endif // __local_source_h__