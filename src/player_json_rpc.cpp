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

// ----------------------------------------------------------------------------
namespace json_rpc
{
  // --------------------------------------------------------------------------
  json_rpc_response play(player& player, const json_rpc_request& request)
  {
    json_rpc_response response{request};

    if ( request.params().is_object() )
    {
      auto& params = request.params().as_object();

      if ( params["id"].is_number() )
      {
        auto res = player.play(params["id"].as_number());

        if ( res == player_status_ok ) {
          response.set_result("ok");
        }
        else {
          response.error(1, "Track not found");
        }
      }
  #if 0
      else if ( params["track"].is_object() )
      {
        auto track = track_t::from_json(params["track"].as_object());
        auto res = player.play(track);

        if ( res == player_status_ok ) {
          response.set_result("ok");
        }
        else {
          response.error(1, "Track not found");
        }
      }
  #endif
      else {
        response.invalid_params();
      }
    }
    else if ( request.params().is_null() )
    {
      auto msg = player.play();

      if ( msg.empty() ) {
        response.set_result("ok");
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

      if ( params["id"].is_number() )
      {
        auto res = player.queue(params["id"].as_number());

        if ( res > 0 ) {
          response.set_result(std::to_string(res));
        }
        else {
          response.error(1, "Track not found");
        }
      }
  #if 0
      else if ( params["track"].is_object() )
      {
        auto track = track_t::from_json(params["track"].as_object());
        auto res = player.queue(track);

        if ( res == player_status_ok ) {
          response.set_result("ok");
        }
        else {
          response.error(1, "Track not found");
        }
      }
  #endif
      else {
        response.invalid_params();
      }
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
      { "track",  info.track ? to_json(*info.track) : json::value{} },
      { "source", info.source }
    };

    response.set_result(params);

    return response;
  }

  // --------------------------------------------------------------------------
  json_rpc_response cover(player& player, const json_rpc_request& request)
  {
    json_rpc_response response{request};

    if ( request.params().is_object() )
    {
      auto& params = request.params().as_object();

      if ( params["album_id"].is_number() )
      {
        auto res = player.get_cover(params["album_id"].as_number());

        if ( res.is_object() ) {
          response.set_result(res);
        }
        else if ( res.is_string() ) {
          response.error(1, res.as_string());
        }
        else {
          response.error(2, "unknown error");
        }
      }
      else if ( params["track_id"].is_number() )
      {
        auto res = player.get_cover_by_track_id(params["track_id"].as_number());

        if ( res.is_object() ) {
          response.set_result(res);
        }
        else if ( res.is_string() ) {
          response.error(1, res.as_string());
        }
        else {
          response.error(2, "unknown error");
        }
      }

      else {
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
  json_rpc_response index(player& player, const json_rpc_request& request)
  {
    json_rpc_response response{request};

    response.set_result(player.database_index());

    return response;
  }

  // --------------------------------------------------------------------------
  json_rpc_response save(player& player, const json_rpc_request& request)
  {
    json_rpc_response response{request};

    if ( request.params().is_object() )
    {
      auto& params = request.params().as_object();

      response.set_result(player.database_save(params));
    }
    else
    {
      response.invalid_params();
    }

    //response.set_result(player.database_index());

    return response;
  }

  // --------------------------------------------------------------------------
  json_rpc_response erase(player& player, const json_rpc_request& request)
  {
    json_rpc_response response{request};

    if ( request.params().is_object() )
    {
      auto& params = request.params().as_object();

      if ( params["track"].is_object() )
      {
        auto track = params["track"].as_object();

        if ( track["id"].is_number() )
        {
          auto msg = player.database_delete_track(track["id"].as_number());

          if ( msg.empty() ) {
            response.set_result("ok");
          }
          else {
            response.error(1, msg);
          }
        }
        else {
          response.invalid_params();
        }
      }
      else if ( params["album"].is_object() )
      {
        auto album = params["album"].as_object();

        if ( album["id"].is_number() )
        {
          auto msg = player.database_delete_album(album["id"].as_number());

          if ( msg.empty() ) {
            response.set_result("ok");
          }
          else {
            response.error(1, msg);
          }
        }
        else {
          response.invalid_params();
        }
      }
      else {
        response.invalid_params();
      }
    }

    return response;
  }

  // --------------------------------------------------------------------------
  json_rpc_response export_tracks(player& player, const json_rpc_request& request)
  {
    json_rpc_response response{request};

    response.set_result(player.database_export_tracks());

    return response;
  }

  // --------------------------------------------------------------------------
  json_rpc_response import_tracks(player& player, const json_rpc_request& request)
  {
    json_rpc_response response{request};

    if ( request.params().is_array() )
    {
      auto tracks = request.params().as_array();
      response.set_result(player.database_import_tracks(std::move(tracks)));
    }
    else
    {
      response.invalid_params();
    }

    return response;
  }

  // --------------------------------------------------------------------------
  json_rpc_response local_scan(player& player, const json_rpc_request& request)
  {
    json_rpc_response response{request};

    player.source_scan("local");

    response.set_result("ok");

    return response;
  }
} // namespace json_rpc