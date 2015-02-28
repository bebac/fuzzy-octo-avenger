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
      auto& tag_member = params["tag"];

      if ( tag_member.is_string() )
      {
        auto tag = tag_member.as_string();

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
        auto track = dm::track::find_by_id(id.as_string());

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
        auto cover = dm::album_cover::find_by_album_id(album_id.as_string());

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
  json_rpc_response import_tracks(const json_rpc_request& request)
  {
    json_rpc_response response{request};

    if ( request.params().is_array() )
    {
      auto& jtracks = request.params().as_array();

      for ( auto& jt : jtracks )
      {
        if ( jt.is_object() )
        {
          auto& track_obj = jt.as_object();

          auto& jartist = track_obj["artist"];
          auto& jalbum  = track_obj["album"];

          if ( !jartist.is_object() ) {
            throw std::runtime_error("invalid artist");
          }

          if ( !jalbum.is_object() ) {
            throw std::runtime_error("invalid album");
          }

          auto& artist_obj   = jartist.as_object();
          auto& album_obj    = jalbum.as_object();
          auto& jtrack_title = track_obj["title"];
          auto& jartist_name = artist_obj["name"];
          auto& jalbum_title = album_obj["title"];

          if ( !jtrack_title.is_string() ) {
            throw std::runtime_error("invalid track title");
          }

          if ( !jartist_name.is_string() ) {
            throw std::runtime_error("invalid artist name");
          }

          if ( !jalbum_title.is_string() ) {
            throw std::runtime_error("invalid album title");
          }

          auto track_title = jtrack_title.as_string();
          auto artist_name = jartist_name.as_string();
          auto album_title = jalbum_title.as_string();

          auto artist = dm::artist::find_by_name(artist_name);

          if ( artist.is_null() )
          {
            // Create artist.
            artist.name(artist_name);
            artist.save();
            std::cerr << "created artist " << artist.name() << " id " << artist.id() << std::endl;
          }

          auto album = artist.find_album_by_title(album_title);

          if ( album.is_null() )
          {
            // Create album.
            album.title(album_title);
            album.save();
            std::cerr << "created album " << album.title() << " id " << album.id() << std::endl;
            // Add album to artist albums.
            artist.add_album(album);
            artist.save();
          }

          auto& tn = track_obj["tn"];

          if ( !tn.is_number() ) {
            throw std::runtime_error("invalid track_number");
          }

          auto track = album.find_track_by_title_and_number(track_title, tn.as_number());

          track.title(track_title);
          track.track_number(tn.as_number());

          auto& dn = track_obj["dn"];
          if ( dn.is_number() )
          {
            track.disc_number(dn.as_number());
          }

          auto& duration = track_obj["duration"];
          if ( duration.is_number() )
          {
            track.duration(duration.as_number());
          }

          auto& cover_obj = track_obj["cover"];
          if ( cover_obj.is_object() )
          {
            auto cover = dm::album_cover::find_by_album_id(album.id());

            if ( cover.is_null() )
            {
              cover.data(std::move(cover_obj.as_object()));
              cover.save();
            }
            else
            {
            }
          }
          else
          {
            std::cerr << "json no cover!" << std::endl;
          }

          track.artist(artist);
          track.album(album);

          auto& source_obj = track_obj["source"];
          if ( source_obj.is_object() )
          {
            track.source(std::move(source_obj.as_object()));
          }
          else
          {
            //ERROR!
          }

          auto& spotify = album_obj["spotify"];
          if ( !spotify.is_null() )
          {
            album.member("spotify", std::move(spotify));
          }

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
  json_rpc_response continuous_playback(player& player, const json_rpc_request& request)
  {
    json_rpc_response response{request};

    if ( request.params().is_object() )
    {
      player.set_continuous_playback(std::move(request.params().as_object()));
      response.set_result("ok");
    }
    else
    {
      response.invalid_params();
    }

    return response;
  }

} // namespace json_rpc