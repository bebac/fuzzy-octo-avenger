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
#include "track_alt_index.h"
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
    bool is_null();
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
  public:
    track(const track& other);
    track(track&& other);
  private:
    track(json::object&& data);
  public:
    track& operator= (const track& rhs);
    track& operator= (track&& rhs);
  public:
    bool is_null();
    bool id_is_null();
  public:
    bool has_tag(std::string tag);
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
    void alt_id(const std::string& id);
    void title(const std::string& v);
    void track_number(unsigned v);
    void disc_number(unsigned v);
    void duration(unsigned v);
    void tags(json::array v);
    void artist(const dm::artist& v);
    void album(const dm::album& v);
    void source(json::object&& jsource);
    void source_remove(const std::string& name);
  public:
    void save();
    void erase();
  public:
    json::value to_json() const { return data_; }
  public:
    track_source find_source(const std::string& name="");
  public:
    static json::array find_all();
  public:
    static track find_by_id(const std::string& id);
    static track find_by_alt_id(const std::string& alt_id);
  public:
    static void each(std::function<bool(json::object& value)> value_cb);
    static void each(std::function<bool(track& track)> value_cb);
  private:
    json::object data_;
  private:
    static kvstore*        kvstore_;
    static track_alt_index index_;
  };
}

// ----------------------------------------------------------------------------
#endif // __dm__track_h__
