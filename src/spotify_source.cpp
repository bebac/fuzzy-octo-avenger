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

#include "spotify_source.h"

// ----------------------------------------------------------------------------
#include <memory>

// ----------------------------------------------------------------------------
#include <b64/encode.h>

// ----------------------------------------------------------------------------
class spotify_cover_loader : public std::enable_shared_from_this<spotify_cover_loader>
{
private:
  using promise_ptr = std::shared_ptr<std::promise<json::value>>;
public:
  spotify_cover_loader(spotify_source* spotify, sp_album* album, promise_ptr promise)
    :
    spotify_(spotify),
    album_(album),
    image_(nullptr),
    promise_(promise)
  {
    sp_album_add_ref(album_);
  }
public:
  ~spotify_cover_loader()
  {
    sp_album_release(album_);
    if ( image_ ) {
      sp_image_release(image_);
    }
  }
public:
  std::shared_ptr<spotify_cover_loader> ptr()
  {
    return shared_from_this();
  }
public:
  bool album_is_loaded()
  {
    return sp_album_is_loaded(album_);
  }
public:
  void load_album()
  {
    sp_albumbrowse_create(spotify_->session_, album_, [](sp_albumbrowse *result, void *userdata)
      {
        auto loader = reinterpret_cast<spotify_cover_loader*>(userdata);
        loader->spotify_->command_queue_.push([=]() {
          loader->load_image();
        });
        sp_albumbrowse_release(result);
      },
      this
    );
  }
public:
  void load_image()
  {
    if ( !album_is_loaded() )
    {
      load_album();
      return;
    }

    sp_link* link = sp_link_create_from_album_cover(album_, SP_IMAGE_SIZE_NORMAL);

    if ( link || sp_link_type(link) == SP_LINKTYPE_IMAGE )
    {
      image_ = sp_image_create_from_link(spotify_->session_, link);

      if ( image_ )
      {
        sp_image_add_ref(image_);

        sp_error res = sp_image_add_load_callback(image_, [](sp_image *image, void *userdata)
          {
            auto loader = reinterpret_cast<spotify_cover_loader*>(userdata);
            loader->spotify_->command_queue_.push([=]() {
              loader->image_loaded_handler();
            });
          },
          this
        );

        if ( res != SP_ERROR_OK )
        {
          // TODO: ERROR!
        }
      }
      else
      {
        // TODO: ERROR!
      }
    }
    else
    {
      // TODO: ERROR!
    }
  }
public:
  void image_loaded_handler()
  {
    size_t size;
    const void * image_data;

    image_data = sp_image_data(image_, &size);

    std::string image_data_s(reinterpret_cast<const char*>(image_data), size);

    std::stringstream is;
    std::stringstream os;

    is.str(image_data_s);

    base64::encoder b64;

    b64.encode(is, os);

    json::object result{
      { "image_format", "jpg" },
      { "image_data", os.str() }
    };

    promise_->set_value(result);

    // Remove loader.
    spotify_->command_queue_.push([this]() {
      spotify_->cover_loaders_.erase(ptr());
    });
  }
private:
  spotify_source* spotify_;
  sp_album*       album_;
  sp_image*       image_;
  promise_ptr     promise_;
};

// ----------------------------------------------------------------------------
spotify_source::spotify_source(const spotify_source_config& config)
  :
  track_(nullptr),
  track_playing_(false),
  track_loading_(false),
  config_(config),
  running_(true),
  command_queue_(),
  thr_{&spotify_source::loop, this}
{
}

spotify_source::~spotify_source()
{
  running_ = false;
  thr_.join();
}

// ----------------------------------------------------------------------------
void spotify_source::play(const std::string& uri, audio_output_ptr audio_output)
{
  command_queue_.push([=]() {
    play_handler(uri, audio_output);
  });
}

// ----------------------------------------------------------------------------
json::value spotify_source::get_cover(const std::string& uri)
{
  auto promise = std::make_shared<std::promise<json::value>>();
  command_queue_.push(std::bind(&spotify_source::get_cover_handler, this, uri, promise));
  return promise->get_future().get();
}

// ----------------------------------------------------------------------------
void spotify_source::init()
{
  sp_session_callbacks callbacks = {
    &logged_in_cb,
    &logged_out_cb,
    &metadata_updated_cb,
    &connection_error_cb,
    &message_to_user_cb,
    &notify_main_thread_cb,
    &music_delivery,
    &play_token_lost_cb,
    &log_message_cb,
    &end_of_track_cb,
    &stream_error_cb,
    0, //&user_info_updated_cb,
    &start_playback_cb,
    &stop_playback_cb,
    &get_audio_buffer_stats_cb,
    0, //&offline_status_updated_cb,
    0, //&offline_error_cb,
    &credentials_blob_updated_cb
  };

  sp_session_config config;

  config.callbacks = &callbacks;
  config.api_version = SPOTIFY_API_VERSION;
  //config.cache_location = m_cache_dir.c_str();
  config.cache_location = "/tmp";
  //config.settings_location = m_cache_dir.c_str();
  config.settings_location = "/tmp";
  config.application_key = g_appkey;
  config.application_key_size = g_appkey_size;
  config.user_agent = "spotd";
  config.userdata = this;
  config.compress_playlists = false;
  config.dont_save_metadata_for_playlists = true;
  config.initially_unload_playlists = false;
  config.device_id = 0;
  config.proxy = "";
  config.proxy_username = 0;
  config.proxy_password = 0;
  config.ca_certs_filename = 0;
  config.tracefile = 0;

  sp_error error = sp_session_create(&config, &session_);
  if ( SP_ERROR_OK != error ) {
    throw spotify_error(error);
  }

  error = sp_session_preferred_bitrate(session_, SP_BITRATE_320k);
  if ( SP_ERROR_OK != error ) {
    throw spotify_error(error);
  }

  error = sp_session_set_volume_normalization(session_, false);
  if ( SP_ERROR_OK != error ) {
    throw spotify_error(error);
  }

  std::cerr << "spotify source - init" << std::endl;

  sp_session_login(session_, config_.username.c_str(), config_.password.c_str(), 0, 0);
}

// ----------------------------------------------------------------------------
void spotify_source::loop()
{
    init();
    while ( running_ )
    {
      auto cmd = command_queue_.pop(std::chrono::seconds(1));
      cmd();
    }
}

// ----------------------------------------------------------------------------
void spotify_source::process_events_handler()
{
  do
  {
    sp_session_process_events(session_, &session_next_timeout_);
  } while (session_next_timeout_ == 0);
}

// ----------------------------------------------------------------------------
void spotify_source::play_handler(const std::string& uri, std::weak_ptr<audio_output_t> audio_output)
{
  // If currently playing a track, stop it before starting load of the new track.
  stop_handler();

  audio_output_ = audio_output;

  std::cerr << "play_handler audio_output=" << audio_output.lock().get() << std::endl;

  sp_link* link = sp_link_create_from_string(uri.c_str());

  if ( link )
  {
    if ( sp_link_type(link) == SP_LINKTYPE_TRACK )
    {
      sp_track_add_ref(track_ = sp_link_as_track(link));

      track_loading_ = true;
      if (sp_track_error(track_) == SP_ERROR_OK) {
        track_loaded_handler();
      }
    }
    else {
      std::cerr << "'" << uri.c_str() << "' is not a track";
    }

    sp_link_release(link);
  }
  else {
    std::cerr << "failed to create link from '" << uri.c_str() << "'";
  }
}

// ----------------------------------------------------------------------------
void spotify_source::start_handler()
{
  if ( !audio_output_.expired() )
  {
    track_playing_ = true;
    auto audio_output = audio_output_.lock();
    audio_output->write_start_marker();
  }
}

// ----------------------------------------------------------------------------
void spotify_source::stop_handler()
{
  std::cerr << "spotify source - stop_handler track_playing_=" << track_playing_ << std::endl;

  if ( track_playing_ )
  {
    sp_session_player_unload(session_);

    if ( track_ )
    {
      sp_track_release(track_);
      track_ = 0;
    }

    if ( !audio_output_.expired() )
    {
      auto audio_output = audio_output_.lock();
      audio_output->write_end_marker();
    }

    track_playing_ = false;
  }
}

// ----------------------------------------------------------------------------
void spotify_source::track_loaded_handler()
{
  if ( track_loading_ )
  {
    sp_error err;

    if ( (err=sp_session_player_load(session_, track_)) != SP_ERROR_OK ) {
      //_log_(error) << "sp_session_player_load error " << err;
    }

    if ( (err=sp_session_player_play(session_, 1)) != SP_ERROR_OK ) {
      //_log_(error) << "sp_session_player_play error " << err;
    }

    track_loading_ = false;
  }
}

// ----------------------------------------------------------------------------
void spotify_source::get_cover_handler(const std::string& uri, std::shared_ptr<std::promise<json::value>> promise)
{
  sp_link* link = sp_link_create_from_string(uri.c_str());

  if ( link && sp_link_type(link) == SP_LINKTYPE_ALBUM )
  {
    sp_album* album = sp_link_as_album(link);

    auto loader = std::make_shared<spotify_cover_loader>(this, album, promise);

    cover_loaders_.insert(loader);

    if ( loader->album_is_loaded() )
    {
      loader->load_image();
    }
    else
    {
      loader->load_album();
    }
  }
  else
  {
    promise->set_value(json::value("cover not found"));
  }
}

// ----------------------------------------------------------------------------
void spotify_source::logged_in_cb(sp_session *session, sp_error error)
{
  std::cerr << "spotify source - logged in" << std::endl;
}

// ----------------------------------------------------------------------------
void spotify_source::logged_out_cb(sp_session *session)
{
}

// ----------------------------------------------------------------------------
inline void spotify_source::metadata_updated_cb(sp_session *session)
{
  auto self = reinterpret_cast<spotify_source*>(sp_session_userdata(session));

  if ( self->track_ )
  {
    sp_error err = sp_track_error(self->track_);
    if (err == SP_ERROR_OK) {
      self->command_queue_.push(std::bind(&spotify_source::track_loaded_handler, self));
    }
  }
}

// ----------------------------------------------------------------------------
void spotify_source::connection_error_cb(sp_session *session, sp_error error)
{
  std::cerr << "spotify source - connection error" << std::endl;
}

// ----------------------------------------------------------------------------
void spotify_source::message_to_user_cb(sp_session *session, const char* message)
{
  std::cerr << "spotify source - message to user " << message << std::endl;
}

// ----------------------------------------------------------------------------
void spotify_source::notify_main_thread_cb(sp_session *session)
{
  auto self = reinterpret_cast<spotify_source*>(sp_session_userdata(session));
  self->command_queue_.push(std::bind(&spotify_source::process_events_handler, self));
}

// ----------------------------------------------------------------------------
int spotify_source::music_delivery(sp_session *session, const sp_audioformat *format, const void *frames, int num_frames)
{
  auto self = reinterpret_cast<spotify_source*>(sp_session_userdata(session));

  if ( !self->audio_output_.expired() )
  {
    auto audio_output = self->audio_output_.lock();

    if ( audio_output->sample_rate() != static_cast<unsigned>(format->sample_rate) ) {
      audio_output->set_sample_rate(format->sample_rate);
    }

    audio_output->write_s16_le_i(frames, num_frames);
  }
  else
  {
    self->command_queue_.push(std::bind(&spotify_source::stop_handler, self));
  }

  return num_frames;
}

// ----------------------------------------------------------------------------
void spotify_source::play_token_lost_cb(sp_session *session)
{
  std::cerr << "spotify source - play token lost" << std::endl;
}

// ----------------------------------------------------------------------------
void spotify_source::log_message_cb(sp_session *session, const char* data)
{
  std::cerr << "spotify source - log: " << data;
}

// ----------------------------------------------------------------------------
void spotify_source::end_of_track_cb(sp_session *session)
{
  std::cerr << "spotify source - end_of_track_cb" << std::endl;
  auto self = reinterpret_cast<spotify_source*>(sp_session_userdata(session));

  self->command_queue_.push(std::bind(&spotify_source::stop_handler, self));
}

// ----------------------------------------------------------------------------
void spotify_source::stream_error_cb(sp_session *session, sp_error error)
{
  std::cerr << "spotify source - stream error " << error << std::endl;
}

// ----------------------------------------------------------------------------
void spotify_source::user_info_updated_cb(sp_session *session)
{
}

// ----------------------------------------------------------------------------
void spotify_source::start_playback_cb(sp_session *session)
{
  std::cerr << "spotify source - start_playback_cb" << std::endl;
  auto self = reinterpret_cast<spotify_source*>(sp_session_userdata(session));
  self->command_queue_.push(std::bind(&spotify_source::start_handler, self));
}

// ----------------------------------------------------------------------------
void spotify_source::stop_playback_cb(sp_session *session)
{
  std::cerr << "spotify source - stop_playback_cb" << std::endl;
  auto self = reinterpret_cast<spotify_source*>(sp_session_userdata(session));
  self->command_queue_.push(std::bind(&spotify_source::stop_handler, self));
}

// ----------------------------------------------------------------------------
void spotify_source::get_audio_buffer_stats_cb(sp_session *session, sp_audio_buffer_stats *stats)
{
  auto self = reinterpret_cast<spotify_source*>(sp_session_userdata(session));

  auto audio_output = self->audio_output_.lock();
  if ( audio_output.get() )
  {
    stats->samples = audio_output->queued_frames();
    stats->stutter = 0;
  }
}

// ----------------------------------------------------------------------------
void spotify_source::offline_status_updated_cb(sp_session *session)
{
  std::cerr << "spotify source - offline status update" << std::endl;
}

// ----------------------------------------------------------------------------
void spotify_source::offline_error_cb(sp_session *session, sp_error error)
{
  std::cerr << "spotify source - offline error " << error << std::endl;
}

// ----------------------------------------------------------------------------
void spotify_source::credentials_blob_updated_cb(sp_session *session, const char* blob)
{
}

