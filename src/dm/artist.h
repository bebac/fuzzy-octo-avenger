// ----------------------------------------------------------------------------
//
//     Filename   : artist.h
//
//     Author     : Benny Bach <benny.bach@gmail.com>
//                  Copyright (C) 2014
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#ifndef __dm__artist_h__
#define __dm__artist_h__

// ----------------------------------------------------------------------------
#include "kvstore.h"

// ----------------------------------------------------------------------------
#include <json/json.h>

// ----------------------------------------------------------------------------
namespace dm
{
  class album;

  class artist
  {
    using album_id_list = std::vector<std::string>;
  public:
    static void init(kvstore* store);
  public:
    artist();
  private:
    artist(json::object&& data);
  public:
    bool is_null();
  public:
    // Getters.
    const std::string& id()        const;
    const std::string& name()      const;
    album_id_list      album_ids() const;
  public:
    // Setters.
    void name(const std::string& v);
    void add_album(const album& album);
  public:
    void save();
  public:
    album find_album_by_title(const std::string& title);
  public:
    void each_album(std::function<bool(album& album)> value_cb);
  public:
    static artist find_by_id(const std::string& id);
    static artist find_by_name(const std::string& name);
  public:
    static void each(std::function<bool(artist& artist)> value_cb);
  private:
    json::object data_;
  private:
    static kvstore* kvstore_;
  };
}

// ----------------------------------------------------------------------------
#endif // __dm__collection_h__
