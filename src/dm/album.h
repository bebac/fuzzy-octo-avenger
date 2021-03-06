// ----------------------------------------------------------------------------
//
//     Filename   : album.h
//
//     Author     : Benny Bach <benny.bach@gmail.com>
//                  Copyright (C) 2014
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#ifndef __dm__album_h__
#define __dm__album_h__

// ----------------------------------------------------------------------------
#include "kvstore.h"

// ----------------------------------------------------------------------------
#include <json/json.h>

// ----------------------------------------------------------------------------
namespace dm
{
  class track;
  class album;

  class album_cover
  {
    friend class album;
  private:
    album_cover(const album& album);
  private:
    album_cover(const std::string& key, json::object&& data);
  public:
    album_cover(const album_cover& other);
    album_cover(album_cover&& other);
  public:
    album_cover& operator=(dm::album_cover& rhs);
    album_cover& operator=(dm::album_cover&& rhs);
  public:
    bool is_null();
  public:
    // Setters.
    void format(const std::string& v);
    void data(const char* v, size_t len);
    void data(json::object&& data);
  public:
    void save();
    void erase();
  public:
    static album_cover find_by_album_id(const std::string& id);
  public:
    json::value data() { return std::move(data_); }
  private:
    json::object data_;
  private:
    std::string key_;
  private:
    static kvstore* kvstore_;
  };

  class album
  {
    using track_id_list = std::vector<std::string>;
  public:
    static void init(kvstore* store);
  public:
    album();
  public:
    album(const album& other);
    album(album&& other);
  public:
    album& operator=(dm::album& rhs);
    album& operator=(dm::album&& rhs);
  private:
    album(json::object&& data);
  public:
    bool is_null();
    bool id_is_null();
  public:
    // Getters.
    const std::string& id()        const;
    const std::string& title()     const;
    track_id_list      track_ids() const;
    //album_cover        cover()     const;
  public:
    // Setters.
    void title(const std::string& v);
    void add_track(const track& track);
    void remove_track(const track& track);
    void member(const std::string& key, json::value&& value);
    void data(json::object&& data);
  public:
    void save();
    void erase();
  public:
    json::value to_json() { return data_; }
  public:
    track find_track_by_disc_and_track_number(unsigned disc_number, unsigned track_number);
    track find_track_by_title_and_number(const std::string& title, unsigned track_number);
  public:
    void each_track(std::function<bool(track& track)> value_cb);
  public:
    static json::array find_all();
  public:
    static album find_by_id(const std::string& id);
    public:
    static void each(std::function<bool(json::value& value)> value_cb);
    static void each(std::function<bool(album& album)> value_cb);
  private:
    json::object data_;
  private:
    static kvstore* kvstore_;
  };
}

// ----------------------------------------------------------------------------
#endif // __dm__album_h__
