// ----------------------------------------------------------------------------
//
//     Filename   : artist.cpp
//
//     Author     : Benny Bach <benny.bach@gmail.com>
//                  Copyright (C) 2014
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------

#include "artist.h"
#include "album.h"

namespace dm
{
  kvstore* artist::kvstore_ = nullptr;

  void artist::init(kvstore* store)
  {
    artist::kvstore_ = store;
  }

  artist::artist()
  {
  }

  artist::artist(json::object&& data)
  {
    data_ = std::move(data);
  }

  bool artist::is_null()
  {
    return data_.empty();
  }

  const std::string& artist::id() const
  {
    return data_.at("id").as_string();
  }

  const std::string& artist::name() const
  {
    return data_.at("name").as_string();
  }

  artist::album_id_list artist::album_ids() const
  {
    album_id_list result;

    if ( data_.has_member("albums") )
    {
      auto& albums = data_.at("albums");

      if ( albums.is_array() )
      {
        for ( auto& album : albums.as_array() ) {
          result.push_back(album.as_string());
        }
      }
    }
    return std::move(result);
  }

  void artist::name(const std::string& v)
  {
    data_["name"] = v;
  }

  void artist::add_album(const album& album)
  {
    auto& jalbums = data_["albums"];

    if ( jalbums.is_array() )
    {
      auto& albums = jalbums.as_array();

      for ( auto& jalbum : albums )
      {
        if ( jalbum.as_string() == album.id() ) {
          return;
        }
      }
      albums.push_back(album.id());
    }
    else
    {
      jalbums = json::array{ album.id() };
    }
  }

  void artist::save()
  {
    auto& id = data_["id"];

    if ( id.is_null() ) {
      id = artist::kvstore_->create_artist_key();
    }

    kvstore_->set(id.as_string(), data_);
  }

  album artist::find_album_by_title(const std::string& title)
  {
    for ( auto& id : album_ids() )
    {
      auto album = dm::album::find_by_id(id);

      if ( !album.is_null() )
      {
        if ( album.title() == title ) {
          return album;
        }
      }
      else
      {
        // TODO: ERROR!
        std::cerr << "invalid album!" << std::endl;
      }
    }
    return album();
  }

  void artist::each_album(std::function<bool(album& album)> value_cb)
  {
    for ( auto& id : album_ids() )
    {
      auto album = dm::album::find_by_id(id);
      value_cb(album);
    }
  }

  artist artist::find_by_id(const std::string& id)
  {
    auto data = kvstore_->get(id);

    if ( !data.is_null() )
    {
      return std::move(artist(std::move(data.as_object())));
    }
    else
    {
      return std::move(artist());
    }
  }

  artist artist::find_by_name(const std::string& name)
  {
    artist result;

    kvstore_->each(
      [](const std::string& key) -> bool
      {
        if ( key.length() == 6 && key[0] == 'a' && key[1] == 'r' ) {
          return true;
        }
        else {
          return false;
        }
      },
      [&](json::value& value) -> bool
      {
        if ( value.is_object() )
        {
          auto& a           = value.as_object();
          auto& artist_name = a["name"];

          if ( artist_name.as_string() == name )
          {
            result = std::move(a);
          }
        }

        return true;
      }
    );

    return std::move(result);
  }

  void artist::each(std::function<bool(artist& artist)> value_cb)
  {
    kvstore_->each(
      [](const std::string& key) -> bool
      {
        if ( key.length() == 6 && key[0] == 'a' && key[1] == 'r' ) {
          return true;
        }
        else {
          return false;
        }
      },
      [&](json::value& value) -> bool
      {
        if ( value.is_object() )
        {
          auto v = artist(std::move(value.as_object()));
          value_cb(v);
        }
        return true;
      }
    );
  }
}