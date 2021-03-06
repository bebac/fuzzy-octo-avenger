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
player_ctbp_selector::player_ctbp_selector()
  :
  track_ids_()
{
}

// ----------------------------------------------------------------------------
void player_ctbp_selector::init()
{
  track_ids_.clear();

  dm::track::each([&](dm::track& track) -> bool
  {
    track_ids_.push_back(track.id());
    return true;
  });

  rg_.reset(new random_generator(0, track_ids_.size()-1));

  std::cout << "ctpb selector init track size " << track_ids_.size() << std::endl;
}

// ----------------------------------------------------------------------------
void player_ctbp_selector::init_by_tag(std::string tag)
{
  track_ids_.clear();

  dm::track::each([&](dm::track& track) -> bool
  {
    if ( track.has_tag(tag) )
    {
      track_ids_.push_back(track.id());
    }
    return true;
  });

  rg_.reset(new random_generator(0, track_ids_.size()-1));

  std::cout << "ctpb selector init track size " << track_ids_.size() << std::endl;
}

// ----------------------------------------------------------------------------
dm::track player_ctbp_selector::next()
{
  auto next = track_ids_[rg_->next()];
  auto track = dm::track::find_by_id(next);

  return track;
}

// ----------------------------------------------------------------------------
player::player(const std::string& audio_device)
  :
  audio_device_(audio_device),
  audio_output_(),
  play_queue_(),
  continuous_playback_(true),
  ctpb_selector_(),
  state_{stopped},
  running_(true),
  command_queue_(),
  thr_{&player::loop, this}
{
}

// ----------------------------------------------------------------------------
player::~player()
{
  // Wait for player thread to stop.
  thr_.join();
}

// ----------------------------------------------------------------------------
void player::shutdown()
{
  command_queue_.push([=]()
  {
    // Request thread to stop.
    running_ = false;
  });
}

// ----------------------------------------------------------------------------
int player::queue(dm::track track)
{
  auto promise = std::make_shared<std::promise<int>>();
  command_queue_.push(std::bind(&player::queue_handler, this, std::move(track), promise));
  return promise->get_future().get();
}

// ----------------------------------------------------------------------------
std::string player::play()
{
  auto promise = std::make_shared<std::promise<std::string>>();
  command_queue_.push([=]()
  {
    if ( state_.state == stopped )
    {
      if ( !play_queue_.empty() )
      {
        play_from_queue();
        promise->set_value("ok");
      }
      else if ( continuous_playback_ )
      {
        // Reinitialize continous playback selector.
        ctpb_selector_.init();
        // Select track and start playback.
        play_queue_.push(ctpb_selector_.next());
        play_from_queue();
        promise->set_value("ok");
      }
      else
      {
        promise->set_value("queue is empty");
      }
    }
    else
    {
      promise->set_value("ok");
    }
  });
  return promise->get_future().get();
}

// ----------------------------------------------------------------------------
std::string player::play_tag(std::string tag)
{
  auto promise = std::make_shared<std::promise<std::string>>();
  command_queue_.push([=]()
  {
    // Reinitialize continous playback selector.
    ctpb_selector_.init_by_tag(tag);

    play_queue_.clear();
    play_queue_.push(ctpb_selector_.next());

    play_stop();
    play_from_queue();

    promise->set_value("ok");
  });
  return promise->get_future().get();
}

// ----------------------------------------------------------------------------
#if 0
int player::play(int id, const std::string& source_name)
{
  auto promise = std::make_shared<std::promise<int>>();
  command_queue_.push(std::bind(&player::play_id_handler, this, id, source_name, promise));
  return promise->get_future().get();
}
#endif

// ----------------------------------------------------------------------------
#if 0
int player::queue(int id, const std::string& source_name)
{
  auto promise = std::make_shared<std::promise<int>>();
  command_queue_.push(std::bind(&player::queue_id_handler, this, id, source_name, promise));
  return promise->get_future().get();
}
#endif

// ----------------------------------------------------------------------------
void player::skip()
{
  command_queue_.push([this]()
  {
    play_stop();
    play_from_queue();
  });
}

// ----------------------------------------------------------------------------
void player::stop()
{
  command_queue_.push([this]()
  {
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
void player::init()
{
  ctpb_selector_.init();
}

// ----------------------------------------------------------------------------
void player::loop()
{
  init();
  while ( running_ )
  {
    auto cmd = command_queue_.pop(std::chrono::seconds(5), [this] {});
    cmd();
  }
}

// ----------------------------------------------------------------------------
#if 0
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

      state_.track  = track;
      state_.source = src.name;

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
#endif

// ----------------------------------------------------------------------------
#if 0
void player::queue_id_handler(int id, const std::string& source_name, std::shared_ptr<std::promise<int>> promise)
{
  auto track = db_.find_track(id);

  if ( track )
  {
    promise->set_value(play_queue_.push(track));

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
#endif

// ----------------------------------------------------------------------------
void player::queue_handler(dm::track track, std::shared_ptr<std::promise<int>> promise)
{
  promise->set_value(play_queue_.push(std::move(track)));

  if ( state_.state == stopped ) {
    play_from_queue();
  }
}

// ----------------------------------------------------------------------------
void player::start_of_track_handler()
{
  std::cout << "start playback" << std::endl;

  if ( state_info_cb_ ) {
    state_info_cb_(state_);
  }
}

// ----------------------------------------------------------------------------
void player::end_of_track_handler()
{
  std::cout << "end playback" << std::endl;

  play_from_queue();
}

// ----------------------------------------------------------------------------
void player::audio_output_error(const std::string& error_message)
{
  std::cout
    << "audio outout error '" << error_message << "' "
    << "source=" << state_.source << " "
    << "track=" << state_.track.to_json()
    << std::endl;

  // What an ugly way to handle this!!!
  if ( error_message == "track unavailable" && state_.source == "spotify" )
  {
    state_.track.source_remove(state_.source);
  }

  audio_output_.reset();

  play_from_queue();
}

// ----------------------------------------------------------------------------
void player::play_stop()
{
  if ( state_.state == playing )
  {
    audio_output_close();

    state_.state = stopped;

    if ( state_info_cb_ ){
      state_info_cb_(state_);
    }

    state_.track = dm::track();
    state_.source = std::string();

    std::cout << "player state=" << state_.state << std::endl;
  }
}

// ----------------------------------------------------------------------------
void player::play_from_queue()
{
  if ( play_queue_.size() > 0 )
  {
    auto track = play_queue_.front();
    auto src   = track.find_source();

    if ( !src.is_null() )
    {
      std::cerr << "play_from_queue id=" << track.id() << ", title='" << track.title() << "', source=" << src.name() << std::endl;

      state_.state  = playing;
      state_.track  = std::move(track);
      state_.source = src.name();

      std::cout << "player state=" << state_.state << std::endl;

      if ( !audio_output_ ) {
        audio_output_open();
      }

      play_source(src);
      play_queue_.pop();
    }
    else
    {
      std::cerr << "play_from_queue id=" << track.id() << ", title='" << track.title() << "', NO SOURCE!" << std::endl;
      play_queue_.pop();
      play_from_queue();
    }
  }
  else if ( continuous_playback_ )
  {
    play_queue_.push(ctpb_selector_.next());
    play_from_queue();
  }
  else
  {
    play_stop();
  }
}

// ----------------------------------------------------------------------------
void player::audio_output_open()
{
  audio_output_.reset(new audio_output_t(audio_device_));

  audio_output_->set_start_marker_callback(
    [this]() {
      command_queue_.push(std::bind(&player::start_of_track_handler, this));
    }
  );

  audio_output_->set_end_marker_callback(
    [this]() {
      command_queue_.push(std::bind(&player::end_of_track_handler, this));
    }
  );

  audio_output_->set_error_callback(
    [this](const std::string& error_message) {
      command_queue_.push(std::bind(&player::audio_output_error, this, error_message));
    }
  );
}

// ----------------------------------------------------------------------------
void player::audio_output_close()
{
  audio_output_.reset();
}

// ----------------------------------------------------------------------------
void player::play_source(const dm::track_source& source)
{
  auto it = sources_.find(source.name());
  if ( it != end(sources_) )
  {
    (*it).second->play(source.uri(), audio_output_);
  }
  else
  {
    std::cerr << "no source found! source=" << source.name() << std::endl;
  }
}
