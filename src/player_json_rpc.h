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
  json_rpc_response stop(player& player, const json_rpc_request& request);
  json_rpc_response state(player& player, const json_rpc_request& request);
  json_rpc_response cover(player& player, const json_rpc_request& request);
  json_rpc_response index(player& player, const json_rpc_request& request);
  json_rpc_response save(player& player, const json_rpc_request& request);
  json_rpc_response erase(player& player, const json_rpc_request& request);
  json_rpc_response export_tracks(player& player, const json_rpc_request& request);
  json_rpc_response import_tracks(player& player, const json_rpc_request& request);
  json_rpc_response local_scan(player& player, const json_rpc_request& request);
} // namespace json_rpc

// ----------------------------------------------------------------------------
#endif // __player_json_rpc_h__