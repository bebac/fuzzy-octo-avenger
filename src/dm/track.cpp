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
  kvstore*        track::kvstore_ = nullptr;
  track_alt_index track::index_;

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

  bool track_source::is_null()
  {
    return data_.empty();
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

    // Build alt id index.
    track::each([&](json::object& value) -> bool
    {
      auto& alt_ids = value["alt_ids"];

      if ( alt_ids.is_array() )
      {
        for ( auto& alt_id : alt_ids.as_array() )
        {
          index_.set(alt_id, value["id"]);
        }
      }
      return true;
    });
  }

  track::track()
  {
  }

  track::track(const track& other) : data_(other.data_)
  {
  }

  track::track(track&& other) : data_(std::move(other.data_))
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

  track& track::operator= (track&& rhs)
  {
    data_ = std::move(rhs.data_);
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
      return data_.at("tags");
    }
    else {
      return json::array();
    }
  }

  void track::alt_id(const std::string& id)
  {
    auto& alt_ids = data_["alt_ids"];

    if ( alt_ids.is_array() )
    {
      auto& arr = alt_ids.as_array();

      for ( auto& alt_id : arr )
      {
        if ( alt_id.as_string() == id ) {
          return;
        }
      }
      arr.push_back(id);
    }
    else
    {
      data_["alt_ids"] = json::array{id};
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

  void track::source_remove(const std::string& name)
  {
    auto& sources = data_["sources"];

    if ( sources.is_array() )
    {
      json::array new_sources;

      for ( auto& obj : sources.as_array() )
      {
        auto& s = obj.as_object();

        if ( s["name"].as_string() != name )
        {
          new_sources.push_back(s);
        }
      }
      // Replace sources.
      data_["sources"] = new_sources;
      // Save track changes.
      save();
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

      if ( name.length() > 0 )
      {
        for ( auto& source : sources )
        {
          auto& obj = source.as_object();
          if ( !obj.empty() && obj["name"].as_string() == name ) {
            return track_source{obj};
          }
        }
        return track_source();
      }
      else
      {
        return track_source{sources[0].as_object()};
      }
    }
    else
    {
      return track_source();
    }
  }

  json::array track::find_all()
  {
    json::array res;

    track::each([&](json::object& value) -> bool
    {
      res.push_back(std::move(value));
      return true;
    });

    return res;
  }

  track track::find_by_id(const std::string& id)
  {
    auto data = kvstore_->get(id);

    if ( !data.is_null() )
    {
      return track(std::move(data.as_object()));
    }
    else
    {
      return track();
    }
  }

  track track::find_by_alt_id(const std::string& alt_id)
  {
    std::string id;

    if ( index_.get(alt_id, &id) ) {
      return find_by_id(id);
    }
    else {
      return track();
    }
  }

  void track::each(std::function<bool(json::object& value)> value_cb)
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
        if ( value.is_object() ) {
          value_cb(value.as_object());
        }
        return true;
      }
    );

  }

  void track::each(std::function<bool(track& track)> value_cb)
  {
    each([&](json::object& value) -> bool
    {
      auto v = track(std::move(value));
      return value_cb(v);
    });
  }
}