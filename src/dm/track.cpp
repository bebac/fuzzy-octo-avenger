// ----------------------------------------------------------------------------
//
//     Filename   : track.cpp
//
//     Author     : Benny Bach <benny.bach@gmail.com>
//                  Copyright (C) 2014
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------

#include "track.h"

namespace dm
{
  kvstore* track::kvstore_ = nullptr;

  track_source::track_source()
    :
    data_()
  {
  }

  track_source::track_source(json::object data)
    :
    data_(std::move(data))
  {
  }

  const std::string& track_source::name() const
  {
    return data_.at("name").as_string();
  }

  const std::string& track_source::uri() const
  {
    return data_.at("uri").as_string();
  }

  void track::init(kvstore* store)
  {
    track::kvstore_ = store;
  }

  track::track()
  {
  }

  track::track(json::object&& data)
  {
    data_ = std::move(data);
  }

  track& track::operator= (const track& rhs)
  {
    data_ = rhs.data_;
    return *this;
  }

  bool track::is_null()
  {
    return data_.empty();
  }

  bool track::id_is_null()
  {
    return data_["id"].is_null();
  }

  bool track::has_tag(std::string tag)
  {
    if ( data_.has_member("tags") )
    {
      auto& tags = data_["tags"].as_array();

      for ( auto& track_tag : tags ) {
        if ( track_tag.is_string() && track_tag.as_string() == tag ) {
            return true;
        }
      }

      return false;
    }
    else {
      return false;
    }
  }

  const std::string& track::id() const
  {
    return data_.at("id").as_string();
  }

  const std::string& track::title() const
  {
    return data_.at("title").as_string();
  }

  const unsigned track::track_number() const
  {
    return data_.at("tn").as_number();
  }

  const unsigned track::disc_number() const
  {
    return data_.at("dn").as_number();
  }

  const unsigned track::duration() const
  {
    return data_.at("duration").as_number();
  }

  const json::array track::tags() const
  {
    if ( data_.has_member("tags") ) {
      return data_.at("tags").as_array();
    }
    else {
      return json::array();
    }
  }

  void track::title(const std::string& v)
  {
    data_["title"] = v;
  }

  dm::artist track::artist() const
  {
    auto& artist_obj = data_.at("artist").as_object();
    return dm::artist::find_by_id(artist_obj.at("id").as_string());
  }

  dm::album track::album() const
  {
    auto& album_obj = data_.at("album").as_object();
    return dm::album::find_by_id(album_obj.at("id").as_string());
  }

  void track::track_number(unsigned v)
  {
    data_["tn"] = v;
  }

  void track::disc_number(unsigned v)
  {
    data_["dn"] = v;
  }

  void track::duration(unsigned v)
  {
    data_["duration"] = v;
  }

  void track::tags(json::array v)
  {
    data_["tags"] = std::move(v);
  }

  void track::artist(const dm::artist& v)
  {
    data_["artist"] = json::object{ { "id", v.id() }, { "name", v.name() } };
  }

  void track::album(const dm::album& v)
  {
    data_["album"] = json::object{ { "id", v.id() }, { "title", v.title() } };
  }

  void track::source(json::object&& jsource)
  {
    auto& name = jsource["name"];

    if ( name.is_null() ) {
      throw std::runtime_error("track source name is null");
    }

    if ( !name.is_string() ) {
      throw std::runtime_error("track source name must be a string");
    }

    auto& sources = data_["sources"];

    if ( sources.is_array() )
    {
      for ( auto& jobj : sources.as_array() )
      {
        auto& s = jobj.as_object();

        if ( s["name"].as_string() == name.as_string() )
        {
          // Replace source.
          s = std::move(jsource);
          return;
        }
        // Add new source.
        sources.as_array().push_back(std::move(jsource));
      }
    }
    else
    {
      sources = json::array{ std::move(jsource) };
    }
  }

  void track::save()
  {
    auto& id = data_["id"];

    if ( id.is_null() )
    {
      id = track::kvstore_->create_track_key();
    }

    kvstore_->set(id.as_string(), data_);
  }

  void track::erase()
  {
    auto& id = data_["id"];

    if ( !id.is_null() )
    {
      kvstore_->remove(id.as_string());
    }
  }

  track_source track::find_source(const std::string& name)
  {
    auto& jsources = data_["sources"];

    if ( jsources.is_array() )
    {
      auto& sources = jsources.as_array();

      return track_source{sources[0].as_object()};
    }
    else
    {
      return track_source();
    }
  }

  track track::find_by_id(const std::string& id)
  {
    auto data = kvstore_->get(id);

    if ( !data.is_null() )
    {
      return std::move(track(std::move(data.as_object())));
    }
    else
    {
      return std::move(track());
    }
  }

  void track::each(std::function<bool(track& track)> value_cb)
  {
    kvstore_->each(
      [](const std::string& key) -> bool
      {
        if ( key.length() == 6 && key[0] == 't' ) {
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
          auto v = track(std::move(value.as_object()));
          value_cb(v);
        }
        return true;
      }
    );
  }
}