// ----------------------------------------------------------------------------
//
//     Filename   : player.h
//
//     Author     : Benny Bach <benny.bach@gmail.com>
//                  Copyright (C) 2014
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#ifndef __player_h__
#define __player_h__

// ----------------------------------------------------------------------------
#include "cmdque.h"
#include "database.h"
#include "audio_output_alsa.h"
#include "source_base.h"
#include "player_queue.h"

// ----------------------------------------------------------------------------
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <future>
#include <queue>
#include <map>

// ----------------------------------------------------------------------------
const int player_status_ok              = 0;
const int player_status_track_not_found = 1;

// ----------------------------------------------------------------------------
struct player_state_info
{
  std::string         state;
  database::track_ptr track;
  std::string         source;
};

// ----------------------------------------------------------------------------
class player
{
public:
  player(const std::string& audio_device);
public:
  ~player();
public:
  void set_state_info_callback(std::function<void(const player_state_info& state_info)> cb)
  {
    state_info_cb_ = cb;
  }
public:
  void add_source(std::string name, std::shared_ptr<source_base> source)
  {
    sources_.emplace(std::move(name), std::move(source));
  }
public:
  std::string play();
  int  play(int id, const std::string& source_name="");
  //int  play(track_ptr track);
  int  queue(int id, const std::string& source_name="");
  //int  queue(track_ptr track);
  void skip();
  void stop();
public:
  player_state_info get_state_info() const;
public:
  json::value get_cover_by_album_id(int album_id) const;
  json::value get_cover_by_track_id(int track_id) const;
public:
  json::value database_index();
  json::value database_save(json::object track_json);
  std::string database_delete_track(int track_id);
  std::string database_delete_album(int album_id);
  json::value database_tags();
  json::value database_export_tracks();
  json::value database_import_tracks(json::array tracks);
public:
  void source_scan(const std::string& source_name);
public:
  void set_continuous_playback(json::object value);
private:
  void init();
  void loop();
private:
  void play_id_handler(int id, const std::string& source_name, std::shared_ptr<std::promise<int>> promise);
  //void play_track_handler(track_ptr track, std::shared_ptr<std::promise<int>> promise);
  void queue_id_handler(int id, const std::string& source_name, std::shared_ptr<std::promise<int>> promise);
  //void queue_track_handler(track_ptr track, std::shared_ptr<std::promise<int>> promise);
private:
  void get_cover_by_album_id_handler(int album_id, std::shared_ptr<std::promise<json::value>> promise) const;
private:
  void database_index_handler(std::shared_ptr<std::promise<json::value>> promise);
  void database_save_handler(json::object track_json, std::shared_ptr<std::promise<json::value>> promise);
  void database_delete_track_handler(int id, std::shared_ptr<std::promise<std::string>> promise);
  void database_delete_album_handler(int id, std::shared_ptr<std::promise<std::string>> promise);
  void database_tags_handler(std::shared_ptr<std::promise<json::value>> promise);
  void database_export_tracks_handler(std::shared_ptr<std::promise<json::value>> promise);
  void database_import_tracks_handler(json::array tracks, std::shared_ptr<std::promise<json::value>> promise);
private:
  void source_scan_handler(const std::string& source_name);
private:
  void start_of_track_handler();
  void end_of_track_handler();
private:
  void play_stop();
  void play_from_queue();
  void open_audio_output();
  void close_audio_output();
  void source_play(const track_source& source);
  void queue_continuous_playback_tracks();
private:
  database::index  db_;
private:
  std::function<void(const player_state_info& state_info)> state_info_cb_;
private:
  std::map<std::string, std::shared_ptr<source_base>> sources_;
  std::string audio_device_;
  std::shared_ptr<audio_output_t> audio_output_;
  player_queue<database::track_ptr> play_queue_;
  bool continuous_playback_;
  json::object continuous_playback_filter_;
  player_state_info state_;
private:
  std::atomic<bool> running_;
  mutable cmdque_t  command_queue_;
  std::thread       thr_;
};

// ----------------------------------------------------------------------------
#endif // __player_h__
