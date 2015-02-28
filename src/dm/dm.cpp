// ----------------------------------------------------------------------------
//
//     Filename   : dm.h
//
//     Author     : Benny Bach <benny.bach@gmail.com>
//                  Copyright (C) 2014
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------

#include "dm.h"

// ----------------------------------------------------------------------------
namespace dm
{
  std::unique_ptr<kvstore> kvstore_;

  void init()
  {
    kvstore_.reset(new kvstore("mboxd.kdb"));

    dm::artist::init(kvstore_.get());
    dm::album::init(kvstore_.get());
    dm::track::init(kvstore_.get());
    dm::source_local::init(kvstore_.get());
  }
}
