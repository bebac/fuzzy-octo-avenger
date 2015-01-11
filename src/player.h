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
#include "audio_output_alsa.h"
#include "source_base.h"
#include "player_queue.h"
#include "dm/dm.h"

// ----------------------------------------------------------------------------
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <future>
#include <queue>
#include <map>
#include <random>

// ----------------------------------------------------------------------------
const int player_status_ok              = 0;
const int player_status_track_not_found = 1;

// ----------------------------------------------------------------------------
struct player_state_info
{
  std::string state;
  dm::track   track;
  std::string source;
};

// ----------------------------------------------------------------------------
class player_ctbp_selector
{
private:
  using engine       = std::default_random_engine;
  using distribution = std::uniform_int_distribution<int>;
  using device       = std::random_device;
public:
  player_ctbp_selector();
public:
  void init();
  void init_by_tag(std::string tag);
public:
  dm::track next();
private:
  device rd_;
  engine re_;
private:
  std::vector<std::string> track_ids_;
};

// ----------------------------------------------------------------------------
class player
{
public:
  player(const std::string& audio_device);
public:
  ~player();
public:
  void shutdown();
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
  int queue(dm::track track);
public:
  std::string play();
  std::string play_tag(std::string tag);
#if 0
  int  play(int id, const std::string& source_name="");
  int  queue(int id, const std::string& source_name="");
#endif
  //int  queue(track_ptr track);
  void skip();
  void stop();
public:
  player_state_info get_state_info() const;
public:
  std::shared_ptr<source_base> find_source(const std::string& source_name);
public:
  void set_continuous_playback(json::object value);
private:
  void init();
  void loop();
private:
#if 0
  void play_id_handler(int id, const std::string& source_name, std::shared_ptr<std::promise<int>> promise);
  void queue_id_handler(int id, const std::string& source_name, std::shared_ptr<std::promise<int>> promise);
#endif
  void queue_handler(dm::track track, std::shared_ptr<std::promise<int>> promise);
private:
  void start_of_track_handler();
  void end_of_track_handler();
private:
  void play_stop();
  void play_from_queue();
  void open_audio_output();
  void close_audio_output();
  void source_play(const dm::track_source& source);
  void queue_continuous_playback_tracks();
private:
  std::function<void(const player_state_info& state_info)> state_info_cb_;
private:
  std::map<std::string, std::shared_ptr<source_base>> sources_;
  std::string audio_device_;
  std::shared_ptr<audio_output_t> audio_output_;
  player_queue<dm::track> play_queue_;
  bool continuous_playback_;
  json::object continuous_playback_filter_;
  player_ctbp_selector ctpb_selector_;
  player_state_info state_;
private:
  std::atomic<bool> running_;
  mutable cmdque_t  command_queue_;
  std::thread       thr_;
};

// ----------------------------------------------------------------------------
#endif // __player_h__
