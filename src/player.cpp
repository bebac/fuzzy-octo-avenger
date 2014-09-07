// ----------------------------------------------------------------------------
//
//     Filename   : player.cpp
//
//     Author     : Benny Bach <benny.bach@gmail.com>
//                  Copyright (C) 2014
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#include "player.h"

// ----------------------------------------------------------------------------
#include <sstream>
#include <chrono>

// ----------------------------------------------------------------------------
static const std::string stopped = "stopped";
static const std::string playing = "playing";

// ----------------------------------------------------------------------------
player::player(database::index& db, const std::string& audio_device)
  :
  db_(db),
  audio_device_(audio_device),
  audio_output_(),
  play_queue_(),
  continuous_playback(true),
  state_{stopped, nullptr},
  running_(true),
  command_queue_(),
  thr_{&player::loop, this}
{
  try
  {
    db_.load("index.json");
  }
  catch(const std::exception& err)
  {
    std::cerr << "faild to load index : " << err.what() << std::endl;
  }
}

// ----------------------------------------------------------------------------
player::~player()
{
  // Stop player thread.
  running_ = false;
  thr_.join();
  // Save index.
  db_.save("index.json");
}

// ----------------------------------------------------------------------------
std::string player::play()
{
  auto promise = std::make_shared<std::promise<std::string>>();
  command_queue_.push([=]()
  {
    if ( !play_queue_.empty() )
    {
      if ( state_.state == stopped ) {
        play_from_queue();
      }
      promise->set_value("");
    }
    else {
      promise->set_value("queue is empty");
    }
  });
  return promise->get_future().get();
}

// ----------------------------------------------------------------------------
int player::play(int id, const std::string& source_name)
{
  auto promise = std::make_shared<std::promise<int>>();
  command_queue_.push(std::bind(&player::play_id_handler, this, id, source_name, promise));
  return promise->get_future().get();
}

// ----------------------------------------------------------------------------
#if 0
int player::play(track_ptr track)
{
  auto promise = std::make_shared<std::promise<int>>();
  command_queue_.push(std::bind(&player::play_track_handler, this, track, promise));
  return promise->get_future().get();
}
#endif

// ----------------------------------------------------------------------------
int player::queue(int id, const std::string& source_name)
{
  auto promise = std::make_shared<std::promise<int>>();
  command_queue_.push(std::bind(&player::queue_id_handler, this, id, source_name, promise));
  return promise->get_future().get();
}

// ----------------------------------------------------------------------------
#if 0
int player::queue(track_ptr track)
{
  auto promise = std::make_shared<std::promise<int>>();
  command_queue_.push(std::bind(&player::queue_track_handler, this, track, promise));
  return promise->get_future().get();
}
#endif

// ----------------------------------------------------------------------------
void player::skip()
{
  command_queue_.push([this]()
  {
    close_audio_output();
    play_stop();
    play_from_queue();
  });
}

// ----------------------------------------------------------------------------
void player::stop()
{
  command_queue_.push([this]()
  {
    close_audio_output();
    play_stop();
  });
}

// ----------------------------------------------------------------------------
player_state_info player::get_state_info() const
{
  auto promise = std::make_shared<std::promise<player_state_info>>();
  command_queue_.push([=]()
  {
    promise->set_value(state_);
  });
  return promise->get_future().get();
}

// ----------------------------------------------------------------------------
json::value player::get_cover_by_album_id(int album_id) const
{
  auto promise = std::make_shared<std::promise<json::value>>();
  command_queue_.push(std::bind(&player::get_cover_by_album_id_handler, this, album_id, promise));
  return promise->get_future().get();
}

// ----------------------------------------------------------------------------
json::value player::get_cover_by_track_id(int track_id) const
{
  auto promise = std::make_shared<std::promise<json::value>>();
  command_queue_.push([=]()
  {
    auto track = db_.find_track(track_id);

    if ( track )
    {
      get_cover_by_album_id_handler(track->album_id(), promise);
    }
    else {
      promise->set_value(json::value("track not found"));
    }
  });
  return promise->get_future().get();
}

// ----------------------------------------------------------------------------
json::value player::database_index()
{
  auto promise = std::make_shared<std::promise<json::value>>();
  command_queue_.push(std::bind(&player::database_index_handler, this, promise));
  return promise->get_future().get();
}

// ----------------------------------------------------------------------------
json::value player::database_save(json::object track_json)
{
  auto promise = std::make_shared<std::promise<json::value>>();
  command_queue_.push(std::bind(&player::database_save_handler, this, std::move(track_json), promise));
  return promise->get_future().get();
}

// ----------------------------------------------------------------------------
std::string player::database_delete_track(int track_id)
{
  auto promise = std::make_shared<std::promise<std::string>>();
  command_queue_.push(std::bind(&player::database_delete_track_handler, this, track_id, promise));
  return promise->get_future().get();
}

// ----------------------------------------------------------------------------
std::string player::database_delete_album(int album_id)
{
  auto promise = std::make_shared<std::promise<std::string>>();
  command_queue_.push(std::bind(&player::database_delete_album_handler, this, album_id, promise));
  return promise->get_future().get();
}

// ----------------------------------------------------------------------------
json::value player::database_export_tracks()
{
  auto promise = std::make_shared<std::promise<json::value>>();
  command_queue_.push(std::bind(&player::database_export_tracks_handler, this, promise));
  return promise->get_future().get();
}

// ----------------------------------------------------------------------------
json::value player::database_import_tracks(json::array tracks)
{
  auto promise = std::make_shared<std::promise<json::value>>();
  command_queue_.push(std::bind(&player::database_import_tracks_handler, this, tracks, promise));
  return promise->get_future().get();
}

// ----------------------------------------------------------------------------
void player::source_scan(const std::string& source_name)
{
  command_queue_.push(std::bind(&player::source_scan_handler, this, source_name));
}

// ----------------------------------------------------------------------------
void player::init()
{
}

// ----------------------------------------------------------------------------
void player::loop()
{
    init();
    while ( running_ )
    {
      auto cmd = command_queue_.pop(std::chrono::seconds(5), [this]
        {
          if ( continuous_playback ) {
            queue_continuous_playback_tracks();
          }
        });
      cmd();
    }
}

// ----------------------------------------------------------------------------
void player::play_id_handler(int id, const std::string& source_name, std::shared_ptr<std::promise<int>> promise)
{
  auto track = db_.find_track(id);

  std::cerr << "found track" << std::endl;

  if ( track )
  {
    auto src = track->find_source(source_name);

    if ( src.uri.length() > 0 )
    {
      promise->set_value(player_status_ok);

      state_.track = track;

      open_audio_output();
      source_play(src);
    }
    else {
      promise->set_value(player_status_track_not_found);
    }
  }
  else
  {
    promise->set_value(player_status_track_not_found);
  }
}

// ----------------------------------------------------------------------------
#if 0
void player::play_track_handler(track_ptr track, std::shared_ptr<std::promise<int>> promise)
{
  auto uri = track->find_source_uri("");

  if ( uri.length() > 0 )
  {
    promise->set_value(player_status_ok);
    open_audio_output();
    play_flac(uri);
  }
  else
  {
    promise->set_value(player_status_track_not_found);
  }
}
#endif

// ----------------------------------------------------------------------------
void player::queue_id_handler(int id, const std::string& source_name, std::shared_ptr<std::promise<int>> promise)
{
  auto track = db_.find_track(id);

  if ( track )
  {
    play_queue_.push(track);

    promise->set_value(play_queue_.size());

    if ( state_.state == stopped ) {
      play_from_queue();
    }
  }
  else
  {
    //promise->set_value(player_status_track_not_found);
    promise->set_value(-1);
  }
}

// ----------------------------------------------------------------------------
#if 0
void player::queue_track_handler(track_ptr track, std::shared_ptr<std::promise<int>> promise)
{
  if ( track )
  {
    promise->set_value(player_status_ok);
    play_queue_.push(track);
  }
  else
  {
    promise->set_value(player_status_track_not_found);
  }
}
#endif

// ----------------------------------------------------------------------------
void player::get_cover_by_album_id_handler(int album_id, std::shared_ptr<std::promise<json::value>> promise) const
{
  auto album = db_.find_album(album_id);

  if ( album )
  {
    json::value result;
    json::value cover = album->cover();

    if ( cover.is_null() )
    {
      for ( auto& source : sources_ )
      {
        auto id = album->find_id(source.first);

        if ( !id.empty() ) {
          cover = source.second->get_cover(id);
        }
      }
    }

    if ( cover.is_null() ) {
      cover = json::value("cover not found");
    }

    promise->set_value(std::move(cover));
  }
  else {
    promise->set_value(json::value("album not found"));
  }
}

// ----------------------------------------------------------------------------
void player::database_index_handler(std::shared_ptr<std::promise<json::value>> promise)
{
  promise->set_value(to_json(db_));
}

// ----------------------------------------------------------------------------
void player::database_save_handler(json::object track_json, std::shared_ptr<std::promise<json::value>> promise)
{
  std::cerr << "database_save_handler track=" << track_json << std::endl;

  auto track = db_.save(std::move(track_json));

  promise->set_value(to_json(*track));
}

// ----------------------------------------------------------------------------
void player::database_delete_track_handler(int id, std::shared_ptr<std::promise<std::string>> promise)
{
  std::cerr << "database_delete_track_handler track_id=" << id << std::endl;

  auto track = db_.find_track(id);

  if ( track )
  {
    db_.delete_track(track);
    promise->set_value("");
  }
  else
  {
    promise->set_value("track not found");
  }
}

// ----------------------------------------------------------------------------
void player::database_delete_album_handler(int id, std::shared_ptr<std::promise<std::string>> promise)
{
  std::cerr << "database_delete_album_handler album_id=" << id << std::endl;

  auto album = db_.find_album(id);

  if ( album )
  {
    db_.delete_album(album);
    promise->set_value("");
  }
  else
  {
    promise->set_value("album not found");
  }
}

// ----------------------------------------------------------------------------
void player::database_export_tracks_handler(std::shared_ptr<std::promise<json::value>> promise)
{
  promise->set_value(db_.export_tracks());
}

// ----------------------------------------------------------------------------
void player::database_import_tracks_handler(json::array tracks, std::shared_ptr<std::promise<json::value>> promise)
{
  db_.import_tracks(std::move(tracks));

  promise->set_value(json::value(0));
}

// ----------------------------------------------------------------------------
void player::source_scan_handler(const std::string& source_name)
{
  auto it = sources_.find(source_name);

  if ( it != end(sources_) )
  {
    auto source = (*it).second;

    db_.import_tracks(std::move(source->scan()));
  }
  else
  {
    // TODO!
  }
}

// ----------------------------------------------------------------------------
void player::start_of_track_handler()
{
  std::cout << "start playback" << std::endl;

  state_.state = playing;

  if ( state_info_cb_ ) {
    state_info_cb_(state_);
  }
}

// ----------------------------------------------------------------------------
void player::end_of_track_handler()
{
  std::cout << "end playback" << std::endl;

  play_stop();
  play_from_queue();
}

// ----------------------------------------------------------------------------
void player::play_stop()
{
  if ( state_.state == playing )
  {
    state_.state = stopped;

    if ( state_info_cb_ ){
      state_info_cb_(state_);
    }

    state_.track.reset();
  }
}

// ----------------------------------------------------------------------------
void player::play_from_queue()
{
  if ( play_queue_.size() > 0 )
  {
    auto track = play_queue_.front();
    //auto track = play_queue_.pop();
    auto src   = track->find_source("");

    state_.track = track;

    if ( !audio_output_ ) {
      open_audio_output();
    }

    source_play(src);

    play_queue_.pop();
  }
  else
  {
    close_audio_output();
  }
}

// ----------------------------------------------------------------------------
void player::open_audio_output()
{
  audio_output_.reset(new audio_output_t(audio_device_));

  audio_output_->set_start_marker_callback([this]() {
      command_queue_.push(std::bind(&player::start_of_track_handler, this));
    });

  audio_output_->set_end_marker_callback([this]() {
      command_queue_.push(std::bind(&player::end_of_track_handler, this));
    });
}

// ----------------------------------------------------------------------------
void player::close_audio_output()
{
  audio_output_.reset();
}

// ----------------------------------------------------------------------------
void player::source_play(const track_source& source)
{
  auto it = sources_.find(source.name);
  if ( it != end(sources_) )
  {
    (*it).second->play(source.uri, audio_output_);
  }
  else
  {
    std::cerr << "no source found! source=" << source.name << std::endl;
  }
}

// ----------------------------------------------------------------------------
void player::queue_continuous_playback_tracks()
{
  if ( play_queue_.size() < 3 )
  {
    auto tracks = db_.tracks();

    std::random_device rd;
    std::mt19937       g(rd());

    std::shuffle(tracks.begin(), tracks.end(), g);

    while ( play_queue_.size() < 5 && !tracks.empty())
    {
      play_queue_.push(tracks.back(), 3);
      tracks.pop_back();
    }
  }
}