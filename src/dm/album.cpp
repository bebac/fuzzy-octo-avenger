// ----------------------------------------------------------------------------
//
//     Filename   : album.cpp
//
//     Author     : Benny Bach <benny.bach@gmail.com>
//                  Copyright (C) 2014
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------

#include "album.h"
#include "track.h"

// ----------------------------------------------------------------------------
#include <base64.h>

// ----------------------------------------------------------------------------
namespace dm
{
  kvstore* album_cover::kvstore_ = nullptr;
  kvstore* album::kvstore_       = nullptr;

  const std::string id_member      = "id";
  const std::string title_member   = "title";
  const std::string tracks_member  = "tracks";
  const std::string no_disc_id     = "";

  album_cover::album_cover(const album& album)
    :
    key_(album.id()+"/cover")
  {
  }

  album_cover::album_cover(const std::string& key, json::object&& data)
    :
    key_(key)
  {
    data_ = std::move(data);
  }

  album_cover::album_cover(const album_cover& other) : data_(other.data_)
  {
  }

  album_cover::album_cover(album_cover&& other) : data_(std::move(other.data_))
  {
  }

  album_cover& album_cover::operator=(dm::album_cover& rhs)
  {
    data_ = rhs.data_;
    return *this;
  }

  album_cover& album_cover::operator=(dm::album_cover&& rhs)
  {
    data_ = std::move(rhs.data_);
    return *this;
  }

  bool album_cover::is_null()
  {
    return data_.empty();
  }

  bool album::id_is_null()
  {
    return data_["id"].is_null();
  }

  void album_cover::format(const std::string& v)
  {
    data_["image_format"] = v;
  }

  void album_cover::data(const char* v, size_t len)
  {
    data_["image_data"] = base64::encode(v, len);
  }

  void album_cover::data(json::object&& data)
  {
    data_ = std::move(data);
  }

  void album_cover::save()
  {
    kvstore_->set(key_, data_);
  }

  void album_cover::erase()
  {
    kvstore_->remove(key_);
  }

  album_cover album_cover::find_by_album_id(const std::string& id)
  {
    auto key  = id+"/cover";
    auto data = kvstore_->get(key);

    if ( !data.is_null() ) {
      return album_cover(key, std::move(data.as_object()));
    }
    else {
      return album_cover(key, json::object{});
    }
  }

  void album::init(kvstore* store)
  {
    album_cover::kvstore_ = store;
    album::kvstore_       = store;
  }

  album::album()
  {
  }

  album::album(const album& other) : data_(other.data_)
  {
  }

  album::album(album&& other) : data_(std::move(other.data_))
  {
  }

  album& album::operator=(dm::album& rhs)
  {
    data_ = rhs.data_;
    return *this;
  }

  album& album::operator=(dm::album&& rhs)
  {
    data_ = std::move(rhs.data_);
    return *this;
  }

  album::album(json::object&& data)
  {
    data_ = std::move(data);
  }

  bool album::is_null()
  {
    return data_.empty();
  }

  const std::string& album::id() const
  {
    return data_.at(id_member).as_string();
  }

  const std::string& album::title() const
  {
    return data_.at(title_member).as_string();
  }

  void album::title(const std::string& v)
  {
    data_[title_member] = v;
  }

  album::track_id_list album::track_ids() const
  {
    track_id_list result;

    if ( data_.has_member(tracks_member) )
    {
      auto& tracks = data_.at(tracks_member);

      assert(tracks.is_array());

      for ( auto& track : tracks.as_array() ) {
        result.push_back(track);
      }
    }
    return result;
  }

#if 0
  album_cover album::cover() const
  {
    return album_cover(*this);
  }
#endif

  void album::add_track(const track& track)
  {
    auto& jtracks = data_[tracks_member];

    if ( jtracks.is_array() )
    {
      auto& tracks = jtracks.as_array();
      for ( auto& jtrack : tracks )
      {
        if ( jtrack.as_string() == track.id() )
          return;
      }
      tracks.push_back(track.id());
    }
    else
    {
      jtracks = json::array{ track.id() };
    }
  }

  void album::remove_track(const track& track)
  {
    auto& jtracks = data_[tracks_member];

    if ( jtracks.is_array() )
    {
      auto new_tracks = json::array();

      auto& tracks = jtracks.as_array();
      for ( auto& jtrack : tracks )
      {
        if ( jtrack.as_string() != track.id() ) {
          new_tracks.push_back(jtrack);
        }
      }
      jtracks = std::move(new_tracks);
    }
  }

  void album::member(const std::string& key, json::value&& value)
  {
    data_[key] = std::move(value);
  }

  void album::save()
  {
    auto& id = data_[id_member];

    if ( id.is_null() ) {
      id = album::kvstore_->create_album_key();
    }

    kvstore_->set(id.as_string(), data_);
  }

  void album::data(json::object&& data)
  {
    data_ = std::move(data);
  }

  void album::erase()
  {
    // Make sure to remove album references from artists before calling
    // album::erase.

    auto& id = data_[id_member];

    if ( !id.is_null() )
    {
      kvstore_->remove(id);
    }
  }

  track album::find_track_by_disc_and_track_number(unsigned disc_number, unsigned track_number)
  {
    for ( auto& id : track_ids() )
    {
      auto track = dm::track::find_by_id(id);

      if ( !track.is_null() )
      {
        if ( track.disc_number() == disc_number && track.track_number() == track_number ) {
          return track;
        }
      }
      else
      {
        // TODO: ERROR!
        std::cerr << "invalid track! album id=" << this->id() << std::endl;
      }
    }
    return track();
  }

  track album::find_track_by_title_and_number(const std::string& title, unsigned track_number)
  {
    for ( auto& id : track_ids() )
    {
      auto track = dm::track::find_by_id(id);

      if ( !track.is_null() )
      {
        if ( track.title() == title && track.track_number() == track_number ) {
          return track;
        }
      }
      else
      {
        // TODO: ERROR!
        std::cerr << "invalid track!" << std::endl;
      }
    }
    return track();
  }

  void album::each_track(std::function<bool(track& track)> value_cb)
  {
    for ( auto& id : track_ids() )
    {
      auto track = dm::track::find_by_id(id);

      if ( !track.is_null() ) {
        value_cb(track);
      }
    }
  }

  json::array album::find_all()
  {
    json::array res;

    album::each([&](json::value& value) -> bool
    {
      res.push_back(std::move(value));
      return true;
    });

    return std::move(res);
  }

  album album::find_by_id(const std::string& id)
  {
    auto data = kvstore_->get(id);

    if ( !data.is_null() ) {
      return album(std::move(data.as_object()));
    }
    else {
      return album();
    }
  }

  void album::each(std::function<bool(json::value& value)> value_cb)
  {
    kvstore_->each(
      [](const std::string& key) -> bool
      {
        if ( key.length() == 6 && key[0] == 'a' && key[1] == 'l' ) {
          return true;
        }
        else {
          return false;
        }
      },
      [&](json::value& value) -> bool
      {
        if ( value.is_object() ) {
          return value_cb(value);
        }
        else {
          return true;
        }
      }
    );

  }

  void album::each(std::function<bool(album& album)> value_cb)
  {
    each([&](json::value& value) -> bool
    {
      auto v = album(std::move(value.as_object()));
      return value_cb(v);
    });
  }
}