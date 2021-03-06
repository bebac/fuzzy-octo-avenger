// ----------------------------------------------------------------------------
//
//     Filename   : player_json_rpc.h
//
//     Author     : Benny Bach <benny.bach@gmail.com>
//                  Copyright (C) 2014
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#ifndef __player_json_rpc_h__
#define __player_json_rpc_h__

// ----------------------------------------------------------------------------
#include <json_rpc.h>

// ----------------------------------------------------------------------------
namespace json_rpc
{
  json_rpc_response play(player& player, const json_rpc_request& request);
  json_rpc_response queue(player& player, const json_rpc_request& request);
  json_rpc_response skip(player& player, const json_rpc_request& request);
  json_rpc_response stop(player& player, const json_rpc_request& request);
  json_rpc_response state(player& player, const json_rpc_request& request);
  json_rpc_response cover(const json_rpc_request& request);
  json_rpc_response index(const json_rpc_request& request);
  json_rpc_response save(const json_rpc_request& request);
  json_rpc_response erase(const json_rpc_request& request);
  json_rpc_response import_tracks(const json_rpc_request& request);
  json_rpc_response get_artists(const json_rpc_request& request);
  json_rpc_response get_albums(const json_rpc_request& request);
  json_rpc_response set_album(const json_rpc_request& request);
  json_rpc_response get_album_tracks(const json_rpc_request& request);
  json_rpc_response get_tracks(const json_rpc_request& request);
  json_rpc_response get_source_local(const json_rpc_request& request);
  json_rpc_response set_source_local(const json_rpc_request& request);
  json_rpc_response sources_local_scan(const json_rpc_request& request);
  json_rpc_response sources_spotify_uris(const json_rpc_request& request);
#if 0
  json_rpc_response tags(player& player, const json_rpc_request& request);
  json_rpc_response export_tracks(player& player, const json_rpc_request& request);
  json_rpc_response import_tracks(player& player, const json_rpc_request& request);
#endif
} // namespace json_rpc

// ----------------------------------------------------------------------------
#endif // __player_json_rpc_h__