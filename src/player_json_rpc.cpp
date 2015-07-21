// ----------------------------------------------------------------------------
//
//     Filename   : player_json_rpc.cpp
//
//     Author     : Benny Bach <benny.bach@gmail.com>
//                  Copyright (C) 2014
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------

#include "player.h"
#include "player_json_rpc.h"
#include "local_source.h"
#include "dm/dm.h"

// ----------------------------------------------------------------------------
namespace json_rpc
{
  // --------------------------------------------------------------------------
  json_rpc_response play(player& player, const json_rpc_request& request)
  {
    json_rpc_response response{request};

    // TODO: Handle play by id.

    if ( request.params().is_object() )
    {
      auto& params = request.params().as_object();
      auto& tag = params["tag"];

      if ( tag.is_string() )
      {
        auto msg = player.play_tag(tag);

        if ( msg == "ok" ) {
          response.set_result(msg);
        }
        else {
          response.error(1, msg);
        }
      }
      else  {
        response.invalid_params();
      }
    }
    else if ( request.params().is_null() )
    {
      auto msg = player.play();

      if ( msg == "ok" ) {
        response.set_result(msg);
      }
      else {
        response.error(1, msg);
      }
    }
    else {
      response.invalid_params();
    }

    return response;
  }

  // --------------------------------------------------------------------------
  json_rpc_response queue(player& player, const json_rpc_request& request)
  {
    json_rpc_response response{request};

    if ( request.params().is_object() )
    {
      auto& params = request.params().as_object();
      auto& id     = params["id"];

      if ( id.is_string() )
      {
        auto track = dm::track::find_by_id(id);

        if ( !track.is_null() )
        {
          auto res = player.queue(std::move(track));
          response.set_result(std::to_string(res));
        }
        else
        {
          response.error(1, "track not found");
        }
      }
      else
      {
        response.invalid_params();
      }
    }
    else
    {
      response.invalid_params();
    }

    return response;
  }

  // --------------------------------------------------------------------------
  json_rpc_response skip(player& player, const json_rpc_request& request)
  {
    json_rpc_response response{request};

    player.skip();

    response.set_result("ok");

    return response;
  }

  // --------------------------------------------------------------------------
  json_rpc_response stop(player& player, const json_rpc_request& request)
  {
    json_rpc_response response{request};

    player.stop();

    response.set_result("ok");

    return response;
  }

  // --------------------------------------------------------------------------
  json_rpc_response state(player& player, const json_rpc_request& request)
  {
    json_rpc_response response{request};

    auto info = player.get_state_info();

    json::object params
    {
      { "state",  info.state },
      { "track",  info.track.to_json() },
      { "source", info.source }
    };

    response.set_result(params);

    return response;
  }

  // --------------------------------------------------------------------------
  json_rpc_response cover(const json_rpc_request& request)
  {
    json_rpc_response response{request};

    if ( request.params().is_object() )
    {
      auto& params = request.params().as_object();
      auto& album_id = params["album_id"];

      if ( album_id.is_string() )
      {
        auto cover = dm::album_cover::find_by_album_id(album_id);

        if ( !cover.is_null() ) {
          response.set_result(cover.data());
        }
        else {
          response.error(1, "not found");
        }
      }
      else
      {
        response.invalid_params();
      }
    }
    else
    {
      response.invalid_params();
    }

    return response;
  }

  // --------------------------------------------------------------------------
  json_rpc_response index(const json_rpc_request& request)
  {
    json_rpc_response response{request};

    json::object index{
      { "artists", json::array() }
    };

    auto& artists = index["artists"].as_array();
    dm::artist::each([&](dm::artist& artist) -> bool
    {
      json::object jartist{
        { "id",     artist.id() },
        { "name",   artist.name() },
        { "albums", json::array() }
      };

      auto& albums = jartist["albums"].as_array();
      artist.each_album([&](dm::album& album) -> bool
      {
        json::object jalbum{
          { "id",     album.id() },
          { "title",  album.title() },
          { "tracks", json::array() }
        };

        auto& jtracks = jalbum["tracks"].as_array();
        album.each_track([&](dm::track& track) -> bool
        {
          json::object jtrack{
            { "id",       track.id() },
            { "title",    track.title() },
            { "tn",       track.track_number() },
            { "dn",       track.disc_number() },
            { "duration", track.duration() },
            //{ "rating",   -1 }
            { "tags",     track.tags() }
          };

          jtracks.push_back(jtrack);
          return true;
        });

        albums.push_back(jalbum);
        return true;
      });

      // Don't include artists with no albums.
      if ( albums.size() > 0 )
      {
        artists.push_back(jartist);
      }

      return true;
    });

    response.set_result(std::move(index));

    return response;
  }

  // --------------------------------------------------------------------------
  json_rpc_response save(const json_rpc_request& request)
  {
    json_rpc_response response{request};

    if ( request.params().is_object() )
    {
      auto& params = request.params().as_object();

      auto& id = params["id"];

      if ( !id.is_null() )
      {
        if ( id.is_string() )
        {
          auto track = dm::track::find_by_id(id.as_string());

          if ( !track.is_null() )
          {
            auto& tags = params["tags"];

            if ( !tags.is_null() )
            {
              if ( tags.is_array() )
              {
                track.tags(std::move(tags.as_array()));
              }
              else
              {
                response.error(3, "track tags not an array");
                goto save_error;
              }
            }
            track.save();
            response.set_result("ok");
          }
          else
          {
            response.error(1, "track not found");
          }
        }
        else
        {
          response.invalid_params();
        }
      }
      else
      {
        response.error(2, "not implemented yet");
      }
    }
    else
    {
      response.invalid_params();
    }

save_error:
    return response;
  }

  // --------------------------------------------------------------------------
  json_rpc_response erase(const json_rpc_request& request)
  {
    json_rpc_response response{request};

    // TODO: Delete album / artist

    if ( request.params().is_array() )
    {
      for ( auto& jid : request.params().as_array() )
      {
        if ( jid.is_string() )
        {
          auto id = jid.as_string();

          if ( id.length() == 6 && id[0] == 'a' && id[1] == 'l' )
          {
            auto album = dm::album::find_by_id(id);

            // For now only allow delete if album has no tracks.
            auto tracks = album.track_ids();

            if ( tracks.size() == 0 )
            {
              // Iterate all artists to remove album reference.
              dm::artist::each([&](dm::artist& artist) -> bool
              {
                artist.remove_album(album);
                artist.save();
                return true;
              });
              // Delete album cover
              auto cover = dm::album_cover::find_by_album_id(album.id());
              if ( !cover.is_null() )
              {
                cover.erase();
              }
              // Delete the album.
              album.erase();
            }
            else
            {
              // Error!
            }
          }
          else if ( id.length() == 6 && id[0] == 't' )
          {
            auto track = dm::track::find_by_id(id);

            if ( !track.is_null() )
            {
              auto album = track.album();

              album.remove_track(track);
              album.save();

              track.erase();
            }
          }
          else
          {
            response.invalid_params();
            break;
          }
        }
        else
        {
          response.invalid_params();
          break;
        }
      }
      response.set_result("ok");
    }
    else
    {
      response.invalid_params();
    }

    return response;
  }

  // --------------------------------------------------------------------------
#if 0
  json_rpc_response tags(player& player, const json_rpc_request& request)
  {
    json_rpc_response response{request};

    response.set_result(player.database_tags());

    return response;
  }
#endif

  // --------------------------------------------------------------------------
#if 0
  json_rpc_response export_tracks(player& player, const json_rpc_request& request)
  {
    json_rpc_response response{request};

    response.set_result(player.database_export_tracks());

    return response;
  }
#endif

  // --------------------------------------------------------------------------
  void import_json_track(json::object& track_obj)
  {
    /////
    // Validate track json

    if ( !track_obj["artist"].is_object() ) {
      throw std::runtime_error("track artist must be object");
    }

    if ( !track_obj["album"].is_object() ) {
      throw std::runtime_error("track album must be object");
    }

    if ( !track_obj["title"].is_string() ) {
      throw std::runtime_error("track title must be string");
    }

    if ( !track_obj["tn"].is_number() ) {
      throw std::runtime_error("track tn must be number");
    }

    if ( !track_obj["dn"].is_number() ) {
      throw std::runtime_error("track dn must be number");
    }

    if ( !track_obj["duration"].is_number() ) {
      throw std::runtime_error("track duration must be number");
    }

    if ( !track_obj["source"].is_object() ) {
      throw std::runtime_error("track source must be object");
    }

    auto& track_source_obj = track_obj["source"].as_object();

    if ( !track_source_obj["name"].is_string() ) {
      throw std::runtime_error("track source name must be string");
    }

    if ( !track_source_obj["uri"].is_string() ) {
      throw std::runtime_error("track source uri must be string");
    }

    auto& track_artist_obj = track_obj["artist"].as_object();

    if ( !track_artist_obj["name"].is_string() ) {
      throw std::runtime_error("track artist name must be string");
    }

    auto& album_obj = track_obj["album"].as_object();

    if ( !album_obj["artist"].is_object() ) {
      throw std::runtime_error("album artist must be object");
    }

    if ( !album_obj["title"].is_string() ) {
      throw std::runtime_error("album title must be string");
    }

    auto& album_artist_obj = album_obj["artist"].as_object();

    if ( !album_artist_obj["name"].is_string() ) {
      throw std::runtime_error("album artist name must be string");
    }

    auto& track_artist_name = track_artist_obj["name"].as_string();
    auto& album_artist_name = album_artist_obj["name"].as_string();

    /////
    // Update / create album artist.

    auto album_artist = dm::artist::find_by_name(album_artist_name);

    if ( album_artist.is_null() )
    {
      // Create artist.
      album_artist.name(album_artist_name);
      album_artist.save();
      std::cerr << "created artist " << album_artist.name() << " id " << album_artist.id() << std::endl;
    }

    /////
    // Update / create track artist.

    auto track_artist = dm::artist::find_by_name(track_artist_name);

    if ( track_artist.is_null() )
    {
      // Create artist.
      track_artist.name(track_artist_name);
      track_artist.save();
      std::cerr << "created artist " << track_artist.name() << " id " << track_artist.id() << std::endl;
    }

    auto& album_title = album_obj["title"].as_string();

    /////
    // Update / Create album.

    auto album = album_artist.find_album_by_title(album_title);

    // Set / update album artist.
    album.member("artist", json::object{
      { "id",   album_artist.id() },
      { "name", album_artist.name() }
    });

    // Set / update spotify id.
    if ( album_obj["spotify_id"].is_string() ) {
      album.member("spotify_id", album_obj["spotify_id"].as_string());
    }

    if ( album.id_is_null() )
    {
      // Create album.
      album.title(album_title);
      album.save();
      std::cerr << "created album " << album.title() << " id " << album.id() << std::endl;
      // Add album to artist albums.
      album_artist.add_album(album);
      album_artist.save();
    }
    else
    {
      album.save();
    }

    auto track_title    = track_obj["title"].as_string();
    auto track_tn       = track_obj["tn"].as_number();
    auto track_dn       = track_obj["dn"].as_number();
    auto track_duration = track_obj["duration"].as_number();

    /////
    // Update / create track.

    auto track = album.find_track_by_disc_and_track_number(track_dn, track_tn);

    if ( track.id_is_null() )
    {
      //
      // Only set the title for new tracks.
      //
      track.title(track_title);
    }
    else
    {
      //
      // Found the track, but is it the right one? Not sure how to verify it. For
      // now just log a warning if the titles don't match.
      //
      if ( track.title() != track_title )
      {
        std::cerr
          << "title mismatch track id=" << track.id() << " '"
          << track.title() << "' != '" << track_title << "'"  << std::endl;
      }
    }

    track.track_number(track_tn);
    track.disc_number(track_dn);
    track.duration(track_duration);
    track.artist(track_artist);
    track.album(album);
    track.source(std::move(track_source_obj));

    if ( track.id_is_null() )
    {
      // Create track id.
      track.save();
      // Add new track to album.
      album.add_track(track);
      album.save();

      std::cout << album.to_json() << std::endl;
    }
    else
    {
      track.save();
    }

    /////
    // Update / create cover.

    auto& cover_obj = track_obj["cover"];

    if ( cover_obj.is_object() )
    {
      auto cover = dm::album_cover::find_by_album_id(album.id());

      if ( cover.is_null() )
      {
        cover.data(std::move(cover_obj.as_object()));
        cover.save();
      }
    }
    else
    {
      std::cerr << "json no cover!" << std::endl;
    }
  }

  // --------------------------------------------------------------------------
  json_rpc_response import_tracks(const json_rpc_request& request)
  {
    json_rpc_response response{request};

    if ( request.params().is_array() )
    {
      auto& tracks_arr = request.params().as_array();

      for ( auto& item : tracks_arr )
      {
        if ( item.is_object() )
        {
          import_json_track(item.as_object());
        }
        else
        {
          // ERROR!
        }
      }
      response.set_result("ok");
    }
    else
    {
      response.invalid_params();
    }

    return response;
  }

  // --------------------------------------------------------------------------
  json_rpc_response get_artists(const json_rpc_request& request)
  {
    json_rpc_response response{request};

    if ( request.params().is_null() )
    {
      response.set_result(dm::artist::find_all());
    }
    else
    {
      response.invalid_params();
    }

    return response;
  }

  // --------------------------------------------------------------------------
  json_rpc_response get_albums(const json_rpc_request& request)
  {
    json_rpc_response response{request};

    if ( request.params().is_null() )
    {
      response.set_result(dm::album::find_all());
    }
    else
    {
      response.invalid_params();
    }

    return response;
  }

  // --------------------------------------------------------------------------
  json_rpc_response set_album(const json_rpc_request& request)
  {
    json_rpc_response response{request};

    if ( request.params().is_object() )
    {
      auto& params = request.params().as_object();
      auto& id = params["id"];

      if ( id.is_string() )
      {
        auto album = dm::album::find_by_id(id.as_string());

        // TODO: Validate!

        album.data(std::move(params));
        album.save();

        response.set_result("ok");
      }
      else
      {
        response.invalid_params();
      }
    }
    else
    {
      response.invalid_params();
    }

    return response;
  }

  // --------------------------------------------------------------------------
  json_rpc_response get_album_tracks(const json_rpc_request& request)
  {
    json_rpc_response response{request};

    if ( request.params().is_string() )
    {
      auto album = dm::album::find_by_id(request.params().as_string());

      if ( !album.is_null() )
      {
        json::array res;

        album.each_track([&](dm::track& track) -> bool
        {
          res.push_back(track.to_json());
          return true;
        });

        response.set_result(std::move(res));
      }
      else
      {
        // TODO: Report error;
        response.invalid_params();
      }
    }
    else
    {
      response.invalid_params();
    }

    return response;
  }

  // --------------------------------------------------------------------------
  json_rpc_response get_tracks(const json_rpc_request& request)
  {
    json_rpc_response response{request};

    if ( request.params().is_null() )
    {
      response.set_result(dm::track::find_all());
    }
    else
    {
      response.invalid_params();
    }

    return response;
  }

  // --------------------------------------------------------------------------
  json_rpc_response get_source_local(const json_rpc_request& request)
  {
    json_rpc_response response{request};

    if ( request.params().is_null() )
    {
      dm::source_local source_local;
      response.set_result(source_local.to_json());
    }
    else
    {
      response.invalid_params();
    }

    return response;
  }

  // --------------------------------------------------------------------------
  json_rpc_response set_source_local(const json_rpc_request& request)
  {
    json_rpc_response response{request};

    if ( request.params().is_object() )
    {
      dm::source_local source_local;

      auto& params = request.params().as_object();
      auto& directories = params["directories"];

      std::cerr << "set_source_local directories=" << directories << std::endl;

      if ( directories.is_array() )
      {
        std::vector<std::string> new_dirs;

        for ( auto& jsdir : directories.as_array() )
        {
          if ( jsdir.is_string() )
          {
            new_dirs.push_back(jsdir.as_string());
          }
          else
          {
            response.invalid_params();
            return response;
          }
        }

        source_local.directories(new_dirs);

        response.set_result(source_local.to_json());
      }
      else
      {
        response.invalid_params();
      }
    }
    else
    {
      response.invalid_params();
    }

    return response;
  }

  // --------------------------------------------------------------------------
  json_rpc_response sources_local_scan(const json_rpc_request& request)
  {
    json_rpc_response response{request};

    dm::source_local source_local;

    std::cerr << "sources_local_scan begin" << std::endl;

    source_local.scan();

    std::cerr << "sources_local_scan end" << std::endl;

    response.set_result("ok");

    return response;
  }

  // --------------------------------------------------------------------------
  json_rpc_response sources_spotify_uris(const json_rpc_request& request)
  {
    json_rpc_response response{request};
    json::array uris;

    dm::track::each([&](dm::track& track) -> bool
    {
      auto src = track.find_source("spotify");

      if ( !src.is_null() )
      {
        uris.push_back(src.uri());
      }
      return true;
    });

    response.set_result(uris);

    return response;
  }

} // namespace json_rpc