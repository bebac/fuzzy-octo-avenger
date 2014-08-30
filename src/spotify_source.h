// ----------------------------------------------------------------------------
//
//     Filename   : spotify_source.h
//
//     Author     : Benny Bach <benny.bach@gmail.com>
//                  Copyright (C) 2014
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#ifndef __spotify_source_h__
#define __spotify_source_h__

// ----------------------------------------------------------------------------
#include "cmdque.h"
#include "audio_output_alsa.h"
#include "source_base.h"

// ----------------------------------------------------------------------------
#include <thread>
#include <atomic>
#include <future>
#include <set>

// ----------------------------------------------------------------------------
#include <libspotify/api.h>

// ----------------------------------------------------------------------------
extern const uint8_t g_appkey[];
extern const size_t  g_appkey_size;

// ----------------------------------------------------------------------------
class spotify_error : public std::runtime_error
{
private:
  const sp_error& error;
public:
  spotify_error(const sp_error& error)
    : std::runtime_error(sp_error_message(error)), error(error)
  {
  }
};

// ----------------------------------------------------------------------------
struct spotify_source_config
{
  std::string username;
  std::string password;
};

// ----------------------------------------------------------------------------
class spotify_cover_loader;

// ----------------------------------------------------------------------------
class spotify_source : public source_base
{
  friend class spotify_cover_loader;
private:
  using audio_output_ptr = std::weak_ptr<audio_output_t>;
public:
  spotify_source(const spotify_source_config& config);
public:
  ~spotify_source();
public:
  void play(const std::string& uri, audio_output_ptr audio_output);
public:
  json::value get_cover(const std::string& uri);
private:
  void init();
  void loop();
private:
  void process_events_handler();
private:
  void play_handler(const std::string& uri, audio_output_ptr audio_output);
  void stop_handler();
  void track_loaded_handler();
private:
  void get_cover_handler(const std::string& uri, std::shared_ptr<std::promise<json::value>> promise);
private:
  // session callbacks.
  static void logged_in_cb(sp_session *session, sp_error error);
  static void logged_out_cb(sp_session *session);
  static void metadata_updated_cb(sp_session *session);
  static void connection_error_cb(sp_session *session, sp_error error);
  static void message_to_user_cb(sp_session *session, const char* message);
  static void notify_main_thread_cb(sp_session *session);
  static int music_delivery(sp_session *session, const sp_audioformat *format, const void *frames, int num_frames);
  static void play_token_lost_cb(sp_session *session);
  static void log_message_cb(sp_session *session, const char* data);
  static void end_of_track_cb(sp_session *session);
  static void stream_error_cb(sp_session *session, sp_error error);
  static void user_info_updated_cb(sp_session *session);
  static void start_playback_cb(sp_session *session);
  static void stop_playback_cb(sp_session *session);
  static void get_audio_buffer_stats_cb(sp_session *session, sp_audio_buffer_stats *stats);
  static void offline_status_updated_cb(sp_session *session);
  static void offline_error_cb(sp_session *session, sp_error error);
  static void credentials_blob_updated_cb(sp_session *session, const char* blob);
private:
  sp_session*           session_;
  int                   session_next_timeout_;
private:
  sp_track*             track_;
  std::atomic<bool>     track_playing_;
  audio_output_ptr      audio_output_;
private:
  spotify_source_config config_;
private:
  std::set<std::shared_ptr<spotify_cover_loader>> cover_loaders_;
private:
  std::atomic<bool>     running_;
  cmdque_t              command_queue_;
  std::thread           thr_;
};


// ----------------------------------------------------------------------------
#endif // __spotify_source_h__
