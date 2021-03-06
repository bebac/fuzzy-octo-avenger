// ----------------------------------------------------------------------------
//
//     Filename   : json_error.h
//
//     Author     : Benny Bach <benny.bach@gmail.com>
//                  Copyright (C) 2014
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#ifndef __json__json_error_h__
#define __json__json_error_h__

// ----------------------------------------------------------------------------
#include <stdexcept>

// ----------------------------------------------------------------------------
namespace json
{
  class error : public std::runtime_error
  {
  public:
    error(const std::string& msg) : std::runtime_error(msg) {}
  };
}

// ----------------------------------------------------------------------------
#endif // __json__json_error_h__
