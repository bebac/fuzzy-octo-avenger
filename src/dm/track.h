// ----------------------------------------------------------------------------
//
//     Filename   : track.h
//
//     Author     : Benny Bach <benny.bach@gmail.com>
//                  Copyright (C) 2014
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#ifndef __dm__track_h__
#define __dm__track_h__

// ----------------------------------------------------------------------------
#include "kvstore.h"
#include "artist.h"
#include "album.h"

// ----------------------------------------------------------------------------
#include <json/json.h>

// ----------------------------------------------------------------------------
namespace dm
{
  class track_source
  {
  public:
    track_source();
    track_source(json::object data);
  public:
    const std::string& name() const;
    const std::string& uri() const;
  private:
    json::object data_;
  };

  class track
  {
  public:
    static void init(kvstore* store);
  public:
    track();
  private:
    track(json::object&& data);
  public:
    track& operator= (const track& rhs);
  public:
    bool is_null();
    bool id_is_null();
  public:
    // Getters.
    const std::string& id() const;
    const std::string& title() const;
    const unsigned track_number() const;
    const unsigned disc_number() const;
    const unsigned duration() const;
    const json::array tags() const;
    dm::artist artist() const;
    dm::album album() const;
  public:
    // Setters.
    void title(const std::string& v);
    void track_number(unsigned v);
    void disc_number(unsigned v);
    void duration(unsigned v);
    void tags(json::array v);
    void artist(const dm::artist& v);
    void album(const dm::album& v);
    void source(json::object&& jsource);
  public:
    void save();
    void erase();
  public:
    json::value to_json() const { return data_; }
  public:
    track_source find_source(const std::string& name="");
  public:
    static track find_by_id(const std::string& id);
  public:
    static void each(std::function<bool(track& track)> value_cb);
  private:
    json::object data_;
  private:
    static kvstore* kvstore_;
  };
}

// ----------------------------------------------------------------------------
#endif // __dm__track_h__
