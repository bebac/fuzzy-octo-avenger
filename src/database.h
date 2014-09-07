// ----------------------------------------------------------------------------
//
//     Filename   : database.h
//
//     Author     : Benny Bach <benny.bach@gmail.com>
//                  Copyright (C) 2014
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#ifndef __database_h__
#define __database_h__

// ----------------------------------------------------------------------------
#include "track_base.h"

// ----------------------------------------------------------------------------
#include <json/json.h>

// ----------------------------------------------------------------------------
#include <string>
#include <set>
#include <vector>
#include <algorithm>

// ----------------------------------------------------------------------------
namespace database
{
  class index;
  class track;

  // --------------------------------------------------------------------------
  typedef std::shared_ptr<track> track_ptr;

  // --------------------------------------------------------------------------
  class album
  {
    friend class database::index;
  private:
    friend std::ostream& operator<< (std::ostream &os, const album& album);
  private:
    using track_container = std::vector<track_ptr>;
  public:
    album(const std::string& title)
      :
      id_(-1),
      title_(title)
    {
    }
  public:
    void id(int id)
    {
      id_ = id;
    }
  public:
    int id()
    {
      return id_;
    }
  public:
    const std::string& title()
    {
      return title_;
    }
  public:
    void ids(json::object ids)
    {
      if ( ids_.empty() )
      {
        ids_ = std::move(ids);
      }
      else
      {
        // TODO: merge new ids.
      }
    }
  public:
    void cover(json::object cover)
    {
      cover_ = std::move(cover);
    }
  public:
    json::value cover()
    {
      if ( !cover_.empty() ) {
        return cover_;
      }
      else {
        return json::value();
      }
    }
  public:
    void add_track(track_ptr track)
    {
      tracks_.push_back(track);
    }
  public:
    void remove_track(track_ptr track)
    {
      auto it = std::find(begin(tracks_), end(tracks_), track);

      if ( it != end(tracks_) ) {
        tracks_.erase(it);
      }
    }
  public:
    track_ptr find_track(const std::string& title);
  public:
    std::string find_id(const std::string& key);
  private:
    int             id_;
    std::string     title_;
    json::object    ids_;
    json::object    cover_;
    track_container tracks_;
  };

  // --------------------------------------------------------------------------
  typedef std::shared_ptr<album>  album_ptr;

  // --------------------------------------------------------------------------
  inline std::ostream& operator<< (std::ostream &os, const album& album)
  {
    os << "id=" << album.id_ << ", "
       << "title=" << album.title_ << ", "
       << "ids=" << to_string(album.ids_)
       ;
    return os;
  }

  // --------------------------------------------------------------------------
  class artist
  {
    friend class database::index;
  private:
    using album_container = std::vector<std::shared_ptr<album>>;
  public:
    artist(const std::string& name)
      :
      name_(name)
    {
    }
  public:
    const std::string& name()
    {
      return name_;
    }
  public:
    void add_album(std::shared_ptr<album> album)
    {
      albums_.push_back(album);
    }
  public:
    void remove_album(album_ptr album)
    {
      auto it = std::find(begin(albums_), end(albums_), album);

      if ( it != end(albums_) ) {
        albums_.erase(it);
      }
    }
  private:
    std::string     name_;
    album_container albums_;
  };

  // --------------------------------------------------------------------------
  typedef std::shared_ptr<artist> artist_ptr;

  // --------------------------------------------------------------------------
  class track : public track_base
  {
    friend class database::index;
  public:
    track()
      :
      track_id_(-1)
    {
    }
  public:
    void track_id(int track_id)
    {
      track_id_ = track_id;
    }
  public:
    void artist(artist_ptr artist)
    {
      artist_ = std::move(artist);
    }
  public:
    virtual void album(album_ptr album)
    {
      album_ = std::move(album);
    }
  public:
    int track_id() const
    {
      return track_id_;
    }
  public:
    virtual const std::string& artist() const
    {
      return artist_->name();
    }
  public:
    virtual const std::string& album() const
    {
      return album_->title();
    }
  public:
    int album_id() const
    {
      return album_->id();
    }
  private:
    int        track_id_;
    artist_ptr artist_;
    album_ptr  album_;
  };

  // --------------------------------------------------------------------------
  class index
  {
  private:
    using artist_map_t = std::unordered_map<std::string, artist_ptr>;
    using album_map_t = std::unordered_map<std::string, album_ptr>;
    using track_map_t = std::unordered_map<int, track_ptr>;
  public:
    index();
  public:
    std::vector<track_ptr> tracks();
  public:
    track_ptr save(json::object track_json);
    track_ptr save(track_ptr track);
  public:
    void delete_track(track_ptr track);
    void delete_album(album_ptr album);
  public:
    artist_ptr get_artist(const std::string& name);
  public:
    //album_ptr get_artist_album(artist_ptr artist, const std::string& title);
    album_ptr get_artist_album(artist_ptr artist, json::object& album);
  public:
    track_ptr get_album_track(album_ptr album, const std::string& title);
  public:
    track_ptr find_track(int id);
    album_ptr find_album(int id);
  public:
  #if 0
    std::string find_track_source_uri(int id, const std::string& source);
  #endif
  public:
    json::value export_tracks() const;
    json::value import_tracks(json::array tracks);
  public:
    void save(const std::string& filename) const;
    void load(const std::string& filename);
  public:
    json::value to_json() const;
  private:
    artist_map_t artists_;
    album_map_t  albums_;
    track_map_t  tracks_;
    int          next_track_id_;
    int          next_album_id_;
  };
} // namespace database

// ----------------------------------------------------------------------------
json::value to_json(const database::track& track);
json::value to_json(const database::index& database);

// ----------------------------------------------------------------------------
#endif // __database_h__
