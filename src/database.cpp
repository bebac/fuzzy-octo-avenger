// ----------------------------------------------------------------------------
//
//     Filename   : database.cpp
//
//     Author     : Benny Bach <benny.bach@gmail.com>
//                  Copyright (C) 2014
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------

#include "database.h"

// ----------------------------------------------------------------------------
#include <fstream>
#include <algorithm>

// ----------------------------------------------------------------------------
namespace database
{
  // --------------------------------------------------------------------------
  track_ptr album::find_track(const std::string& title)
  {
    auto it = std::find_if(begin(tracks_), end(tracks_), [&](const track_ptr& t) {
      return t->title() == title;
    });

    if ( it == end(tracks_) ) {
      return track_ptr();
    }
    else {
      return (*it);
    }
  }

  // --------------------------------------------------------------------------
  std::string album::find_id(const std::string& key)
  {
    if ( ids_.has_member(key) && ids_[key].is_string() ) {
      return ids_[key].as_string();
    }
    else {
      return std::string();
    }
  }

  // --------------------------------------------------------------------------
  index::index()
    :
    next_track_id_(1),
    next_album_id_(1)
  {
  }

  // --------------------------------------------------------------------------
  std::vector<track_ptr> index::tracks()
  {
    std::vector<database::track_ptr> tracks;

    for ( auto& t : tracks_ )
    {
      tracks.push_back(t.second);
    }

    return std::move(tracks);
  }

  // --------------------------------------------------------------------------
  track_ptr index::save(json::object track_json)
  {
    auto track = std::make_shared<database::track>();

    if ( track_json["id"].is_null() )
    {
      auto artist = get_artist(track_json["artist"].as_string());
      auto album  = get_artist_album(artist, track_json["album"].as_object());

      auto title = track_json["title"].as_string();

      track = album->find_track(title);

      if ( !track )
      {
        track = std::make_shared<database::track>();

        track->title(title);
        track->track_number(track_json["tn"].as_number());
        track->disc_number(track_json["dn"].as_number());
        track->duration(track_json["duration"].as_number());
        track->artist(artist);
        track->album(album);

        json::array sources = track_json["sources"].as_array();

        for ( auto& s : sources ) {
          track->source_add(s.as_object());
        }

        album->add_track(track);

        track = save(track);
      }
      else
      {
        track->track_number(track_json["tn"].as_number());
        track->disc_number(track_json["dn"].as_number());
        track->duration(track_json["duration"].as_number());
        track->artist(artist);
        track->album(album);

        json::array sources = track_json["sources"].as_array();

        for ( auto& s : sources ) {
          track->source_add(s.as_object());
        }
      }
    }
    else
    {
      if ( track_json["id"].is_number() )
      {
        track = find_track(track_json["id"].as_number());

        if ( track )
        {
          if ( track_json["tags"].is_array() )
          {
            // Replace tags.
            track->tags_clear();

            for ( auto& tag : track_json["tags"].as_array() )
            {
              //std::cerr << "add tag " << to_string(tag) << std::endl;
              track->tag_add(tag.as_string());
            }
          }

          if ( track_json["tags/add"].is_array() )
          {
            // Add new tags.
            for ( auto& tag : track_json["tags/add"].as_array() )
            {
              //std::cerr << "add tag " << to_string(tag) << std::endl;
              track->tag_add(tag.as_string());
            }
          }

          // TODO: Update other track attributes.

        }
        else
        {
          // TODO: ERROR!
        }
      }
      else
      {
        // TODO: ERROR!
      }
    }
    return track;
  }

  // --------------------------------------------------------------------------
  track_ptr index::save(track_ptr track)
  {
    if ( track->track_id() == -1 )
    {
      track->track_id(next_track_id_);

      tracks_[track->track_id()] = track;

      next_track_id_++;
    }
    else
    {
      // TODO: Update track.
    }
    return track;
  }

  // --------------------------------------------------------------------------
  void index::delete_track(track_ptr track)
  {
    track->album_->remove_track(track);
    tracks_.erase(track->track_id());
  }

  // --------------------------------------------------------------------------
  void index::delete_album(album_ptr album)
  {
    // Delete all album tracks.
    for ( auto& track : album->tracks_ ) {
      tracks_.erase(track->track_id());
    }

    album->tracks_.clear();

    // Delete album.
    auto it = std::find_if(begin(albums_), end(albums_), [=](const std::pair<std::string, album_ptr>& it) {
      return it.second == album;
    });

    if ( it != end(albums_) )  {
      std::cerr << "delete album " << album->title() << std::endl;
      albums_.erase((*it).first);
    }

    for ( auto& it : artists_ ) {
      it.second->remove_album(album);
    }
  }

  // --------------------------------------------------------------------------
  json::value index::tags()
  {
    // TODO: Return a list of all tags.
    return json::array{};
  }

  // --------------------------------------------------------------------------
  artist_ptr index::get_artist(const std::string& name)
  {
    auto it = artists_.find(name);

    if ( it == end(artists_) )
    {
      auto artist = std::make_shared<database::artist>(name);

      artists_[name] = artist;

      return artist;
    }
    else
    {
      return (*it).second;
    }
  }

  // --------------------------------------------------------------------------
  //album_ptr index::get_artist_album(artist_ptr artist, const std::string& title)
  album_ptr index::get_artist_album(artist_ptr artist, json::object& album_json)
  {
    // TODO: Validate album.
    auto title = album_json["title"].as_string();
    auto key = artist->name()+"/"+title;
    auto it  = albums_.find(key);

    if ( it == end(albums_) )
    {
      auto album = std::make_shared<database::album>(title);

      album->id(next_album_id_);

      if ( album_json["ids"].is_object() ) {
        album->ids(album_json["ids"].as_object());
      }

      if ( album_json["cover"].is_object() ) {
        album->cover(album_json["cover"].as_object());
      }

      albums_[key] = album;

      next_album_id_++;

      artist->add_album(album);

      return album;
    }
    else
    {
      auto album = (*it).second;

      if ( album_json["ids"].is_object() ) {
        album->ids(album_json["ids"].as_object());
      }

      if ( album_json["cover"].is_object() ) {
        album->cover(album_json["cover"].as_object());
      }

      return album;
    }
  }

  // --------------------------------------------------------------------------
  track_ptr index::get_album_track(album_ptr album, const std::string& title)
  {
    auto track = album->find_track(title);

    if ( !track )
    {
      return track_ptr();
    }
    else
    {
      return track;
    }
  }

  // --------------------------------------------------------------------------
  track_ptr index::find_track(int id)
  {
    auto it = tracks_.find(id);

    if ( it != end(tracks_) )
    {
      return (*it).second;
    }
    else
    {
      return track_ptr();
    }
  }

  // --------------------------------------------------------------------------
  album_ptr index::find_album(int id)
  {
    auto it = std::find_if(begin(albums_), end(albums_), [=](const std::pair<std::string, album_ptr>& it) {
      return it.second->id() == id;
    });

    if ( it != end(albums_) )
    {
      return (*it).second;
    }
    else
    {
      return album_ptr();
    }
  }

  // --------------------------------------------------------------------------
  json::value index::export_tracks() const
  {
    json::array tracks;

    for ( auto& p : tracks_ )
    {
      auto track = p.second;

      json::object o {
        { "title",    track->title() },
        { "tn",       track->track_number() },
        { "dn",       track->disc_number() },
        { "duration", track->duration() },
        { "rating",   track->rating() },
        { "artist",   track->artist() },
        { "album",    json::object{ { "title", track->album() }, { "ids", track->album_->ids_ } } }
      };

      json::array sources;

      for ( auto& s : track->sources() ) {
        sources.push_back(s);
      }

      o["sources"] = std::move(sources);

      tracks.push_back(std::move(o));
    }

    return std::move(tracks);
  }

  // --------------------------------------------------------------------------
  json::value index::import_tracks(json::array tracks)
  {
    for ( auto& track : tracks )
    {
      if ( track.is_object() ) {
        save(std::move(track.as_object()));
      }
    }
    return json::value(static_cast<int>(tracks.size()));
  }

  // --------------------------------------------------------------------------
  void index::save(const std::string& filename) const
  {
    json::object index;
    json::array  artists;

    for ( auto& ar : artists_ )
    {
      json::array albums;

      for ( auto& al : ar.second->albums_ )
      {
        json::array  tracks;

        for ( auto& tr : al->tracks_ )
        {
          json::object track{
            { "id",       tr->track_id() },
            { "title",    tr->title() },
            { "tn",       tr->track_number() },
            { "dn",       tr->disc_number() },
            { "duration", tr->duration() },
            { "rating",   tr->rating() }
          };

          // Build tags array.
          json::array tags;
          for ( auto& t : tr->tags() ) {
            tags.push_back(t);
          }
          track["tags"] = std::move(tags);

          // Build sources array.
          json::array sources;
          for ( auto& s : tr->sources() ) {
            sources.push_back(s);
          }
          track["sources"] = std::move(sources);

          tracks.push_back(track);
        }

        json::object album{
          { "id",     al->id() },
          { "title",  al->title() },
          { "tracks", std::move(tracks) },
          { "ids",    al->ids_ },
          { "cover",  al->cover_ }
        };

        albums.push_back(album);
      }

      json::object artist{
        { "name",   ar.second->name_ },
        { "albums", std::move(albums) }
      };

      artists.push_back(artist);
    }

    std::cerr << "saving " << artists.size() << " artists" << std::endl;

    index["artists"] = std::move(artists);

    std::ofstream os;

    os.open(filename);
    os << to_string(index);
    os.close();

    std::cerr << "saved " << filename << std::endl;
  }

  // --------------------------------------------------------------------------
  void index::load(const std::string& filename)
  {
    std::ifstream is(filename);

    if ( is.good() )
    {
      std::string str((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());

      json::value  doc;
      json::parser parser(doc);

      parser.parse(str.c_str(), str.length());

      if ( !doc.is_object() ) {
        throw std::runtime_error("index file is not a json object!");
      }

      auto index = doc.as_object();

      if ( !index["artists"].is_array() ) {
        throw std::runtime_error("index file does not contain an artists array!");
      }

      auto artists_json = index["artists"].as_array();

      for ( auto& art_json : artists_json )
      {
        if ( art_json.is_object() )
        {
          auto obj = art_json.as_object();

          if ( !obj["name"].is_string() ) {
            throw std::runtime_error("index file - artist name must be a string!");
          }

          if ( !obj["albums"].is_array() ) {
            throw std::runtime_error("index file - artist albums must be an array!");
          }

          auto name   = obj["name"].as_string();
          auto albums = obj["albums"].as_array();

          auto art    = std::make_shared<artist>(name);

          for ( auto& alb_json : albums )
          {
            if ( alb_json.is_object() )
            {
              auto obj = alb_json.as_object();

              auto id     = obj["id"].as_number();
              auto title  = obj["title"].as_string();
              auto tracks = obj["tracks"].as_array();
              auto key    = art->name() + "/" + title;

              auto alb = std::make_shared<album>(title);

              alb->id(id);

              if ( obj.has_member("ids") ) {
                alb->ids_ = std::move(obj["ids"].as_object());
              }

              if ( obj.has_member("cover") ) {
                alb->cover_ = std::move(obj["cover"].as_object());
              }

              for ( auto& trk_json : tracks )
              {
                if ( trk_json.is_object() )
                {
                  auto obj = trk_json.as_object();

                  auto id       = obj["id"].as_number();
                  auto title    = obj["title"].as_string();
                  auto tn       = obj["tn"].as_number();
                  auto dn       = obj["dn"].as_number();
                  auto duration = obj["duration"].as_number();
                  auto rating   = obj["rating"].as_number();
                  auto sources  = obj["sources"].as_array();

                  auto trk = std::make_shared<track>();

                  trk->track_id(id);
                  trk->title(title);
                  trk->track_number(tn);
                  trk->disc_number(dn);
                  trk->duration(duration);
                  trk->rating(rating);
                  trk->artist(art);
                  trk->album(alb);

                  if ( obj["tags"].is_array() )
                  {
                    for ( auto& tag_json : obj["tags"].as_array() )
                    {
                      if ( tag_json.is_string() ) {
                        trk->tag_add(tag_json.as_string());
                      }
                      else {
                        throw std::runtime_error("index file - track tag must be a string!");
                      }
                    }
                  }

                  for ( auto& src_json : sources )
                  {
                    if ( src_json.is_object() ) {
                      trk->source_add(src_json.as_object());
                    }
                    else {
                      throw std::runtime_error("index file - track source must be an object!");
                    }
                  }

                  alb->add_track(trk);

                  next_track_id_ = std::max(int(id), next_track_id_);
                  tracks_[id]    = trk;
                }
                else {
                  throw std::runtime_error("index file - track must be an object!");
                }
              }

              art->add_album(alb);

              next_album_id_ = std::max(int(id), next_album_id_);
              albums_[key]   = alb;
            }
            else {
              throw std::runtime_error("index file - album must be an object!");
            }
          }

          artists_[name] = art;
        }
        else
        {
          throw std::runtime_error("index file - artist must be an object!");
        }
      }

      next_album_id_++;
      next_track_id_++;
    }
    else
    {
      throw std::runtime_error("index file - failed to open file!");
    }
  }

  // --------------------------------------------------------------------------
  json::value index::to_json() const
  {
    json::object index;
    json::array  artists;

    for ( auto& ar : artists_ )
    {
      json::array  albums;

      for ( auto& al : ar.second->albums_ )
      {
        json::array  tracks;

        for ( auto& tr : al->tracks_ )
        {
          json::object track{
            { "id",       tr->track_id() },
            { "title",    tr->title() },
            { "tn",       tr->track_number() },
            { "dn",       tr->disc_number() },
            { "duration", tr->duration() },
            { "rating",   tr->rating() }
          };

          // Build tags array.
          json::array tags;
          for ( auto& t : tr->tags() ) {
            tags.push_back(t);
          }
          track["tags"] = std::move(tags);

          tracks.push_back(track);
        }

        json::object album{
          { "id",     al->id() },
          { "title",  al->title() },
          { "tracks", std::move(tracks) }
        };

        albums.push_back(album);
      }

      json::object artist{
        { "name", ar.second->name_ },
        { "albums", std::move(albums) }
      };

      artists.push_back(artist);
    }

    index["artists"] = std::move(artists);

    return index;
  }

  // --------------------------------------------------------------------------
  #if 0
  std::string database::find_track_source_uri(int id, const std::string& source)
  {
    auto it = tracks_.find(id);

    if ( it != end(tracks_) ) {
      return (*it).second->find_source_uri(source);
    }
    else {
      return std::string();
    }
  }
  #endif

} // namespace database

// ----------------------------------------------------------------------------
json::value to_json(const database::track& track)
{
  json::object o {
    { "track_id", track.track_id() },
    { "title",    track.title() },
    { "tn",       track.track_number() },
    { "dn",       track.disc_number() },
    { "duration", track.duration() },
    { "rating",   track.rating() },
    { "artist",   track.artist() },
    { "album",    track.album() }
  };

  // Build tags array.
  json::array tags;
  for ( auto& t : track.tags() ) {
    tags.push_back(t);
  }
  o["tags"] = std::move(tags);

  // Build sources array.
  json::array sources;
  for ( auto& s : track.sources() ) {
    sources.push_back(s);
  }
  o["sources"] = std::move(sources);

  return std::move(o);
}

// ----------------------------------------------------------------------------
json::value to_json(const database::index& index)
{
  return std::move(index.to_json());
}
