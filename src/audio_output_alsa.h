// ----------------------------------------------------------------------------
//
//     Filename   : audio_output_alsa.h
//
//     Author     : Benny Bach <benny.bach@gmail.com>
//                  Copyright (C) 2014
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#ifndef __audio_output_alsa_h__
#define __audio_output_alsa_h__

// ----------------------------------------------------------------------------
#include "cmdque.h"
//#include <log.h>

// ----------------------------------------------------------------------------
#include <iostream>
#include <memory>
#include <cstring>
#include <thread>
#include <atomic>
#include <string>

// ----------------------------------------------------------------------------
#include <alsa/asoundlib.h>

// ----------------------------------------------------------------------------
struct s16_le_frame
{
    int16_t l;
    int16_t r;
};

// ----------------------------------------------------------------------------
struct s32_le_frame
{
    int32_t l;
    int32_t r;
};

// ----------------------------------------------------------------------------
class audio_output_t
{
public:
  audio_output_t(const std::string& device_name)
    :
    handle_(0),
    sample_rate_(44100),
    queued_frames_(0),
    device_name_(device_name),
    replaygain_(0),
    running_(true),
    command_queue_(),
    thr_{&audio_output_t::main, this}
  {
  }
public:
  ~audio_output_t()
  {
    if ( running_ ) {
      stop();
    }
    thr_.join();
  }
public:
  void set_start_marker_callback(std::function<void()> callback)
  {
    start_marker_cb_ = callback;
  }
public:
  void set_end_marker_callback(std::function<void()> callback)
  {
    end_marker_cb_ = callback;
  }
public:
  void set_error_callback(std::function<void(const std::string& error_message)> callback)
  {
    error_cb_ = callback;
  }
public:
  void write_start_marker()
  {
    // Prepare PCM.
    command_queue_.push([=]
    {
      auto res = snd_pcm_prepare(handle_);
      if ( res < 0 ) {
        std::cerr << "prepare pcm error " << snd_strerror(res) << std::endl;
      }
    });
    // Schedule start marker callback if set.
    if ( start_marker_cb_ )
    {
      auto cb = start_marker_cb_;
      command_queue_.push([=] { cb(); });
    }
  }
public:
  void write_end_marker()
  {
    // Drain PCM.
    command_queue_.push([=]
    {
      auto res = snd_pcm_drain(handle_);
      if ( res < 0 ) {
        std::cerr << "drain pcm error " << snd_strerror(res) << std::endl;
      }
    });
    // Schedule end marker callback if set.
    if ( end_marker_cb_ )
    {
      auto cb = end_marker_cb_;
      command_queue_.push([=] { cb(); });
    }
  }
public:
  void write_error_marker(const std::string& error_message)
  {
    if ( error_cb_ )
    {
      auto cb = error_cb_;
      command_queue_.push([=]{
        cb(error_message);
      });
    }
  }
public:
  void set_sample_rate(unsigned sample_rate)
  {
    command_queue_.push([=]{
      set_sample_rate_handler(sample_rate);
    });
  }
public:
  void set_replaygain(double db)
  {
    replaygain_ = db;
  }
public:
  void write_s16_le_i(const void* frames, size_t num_frames)
  {
    auto ibuf = reinterpret_cast<const s16_le_frame*>(frames);
    auto obuf = std::shared_ptr<s32_le_frame>(new s32_le_frame[num_frames], std::default_delete<s32_le_frame[]>());

    unsigned scale = 16 - gain_factor();

    for( size_t i = 0; i < num_frames; i++ )
    {
      obuf.get()[i].l = ((int32_t)ibuf[i].l<<scale);
      obuf.get()[i].r = ((int32_t)ibuf[i].r<<scale);
    }

    command_queue_.push([=]{
      write_handler(std::move(obuf), num_frames);
    });

    queued_frames_ += num_frames;
  }
public:
  void write_s32_le_i(const void* frames, size_t num_frames)
  {
    auto ibuf = reinterpret_cast<const s32_le_frame*>(frames);
    auto obuf = std::shared_ptr<s32_le_frame>(new s32_le_frame[num_frames], std::default_delete<s32_le_frame[]>());

    unsigned scale = gain_factor();

    for( size_t i = 0; i < num_frames; i++ )
    {
      obuf.get()[i].l = ((int32_t)ibuf[i].l)>>scale;
      obuf.get()[i].r = ((int32_t)ibuf[i].r)>>scale;
    }

    command_queue_.push([=]{
      write_handler(std::move(obuf), num_frames);
    });

    queued_frames_ += num_frames;
  }
public:
  void stop()
  {
    //command_queue_.push([this]() {
      running_ = false;
    //});
  }
public:
  unsigned sample_rate()
  {
    return sample_rate_;
  }
public:
  int queued_frames()
  {
    return queued_frames_;
  }
private:
  void init()
  {
    int err;
    int open_retries = 0;

    while ( open_retries < 10 )
    {
      err = snd_pcm_open( &handle_, device_name_.c_str(), SND_PCM_STREAM_PLAYBACK, 0 );
      if ( err < 0 )
      {
        //_log_(error) << "snd_pcm_open failed! " << snd_strerror(err);
        std::cerr << "snd_pcm_open '" << device_name_ << "' failed! " << snd_strerror(err) << std::endl;
        open_retries++;
        std::this_thread::sleep_for(std::chrono::seconds(1));
      }
      else {
        std::cerr << "opened pcm handle_=" << handle_ << std::endl;
        break;
      }
    }

    err = snd_pcm_set_params(handle_, SND_PCM_FORMAT_S32_LE, SND_PCM_ACCESS_RW_INTERLEAVED, 2, sample_rate_, 0, 100000);
    if ( err < 0 ) {
      //_log_(error) << "snd_pcm_set_params failed! " << snd_strerror(err);
      std::cerr << "snd_pcm_set_params failed! " << snd_strerror(err);
    }
  }
private:
  void set_sample_rate_handler(unsigned sample_rate)
  {
    if ( sample_rate_ != sample_rate )
    {
      if (handle_)
      {
        std::cerr << "closing pcm handle_=" << handle_ << std::endl;
        snd_pcm_close(handle_);
      }

      sample_rate_ = sample_rate;
      // Reinitialize.
      init();
    }
  }
private:
  void write_handler(std::shared_ptr<s32_le_frame> buf, size_t num_frames)
  {
    snd_pcm_sframes_t frames = snd_pcm_writei(handle_, buf.get(), num_frames);

    if ( frames < 0 ) {
      //_log_(warning) << "underrun";
      frames = snd_pcm_recover(handle_, frames, 0);
    }

    if ( frames < 0 ) {
      //_log_(error) << snd_strerror(frames);
      std::cerr << snd_strerror(frames) << std::endl;
    }
    else {
      queued_frames_ -= frames;
    }
  }
public:
  unsigned gain_factor()
  {
    if ( replaygain_ > -3 ) {
      return 0;
    }
    else if ( replaygain_ > -9 ) {
      return 1;
    }
    else if ( replaygain_ > -15 ) {
      return 2;
    }
    else {
      return 3;
    }
  }
private:
  void main()
  {
    init();
    while ( running_ )
    {
      auto cmd = command_queue_.pop(std::chrono::seconds(1));
      cmd();
    }

    if (handle_) {
      snd_pcm_close(handle_);
    }
  }
private:
  std::function<void()> start_marker_cb_;
  std::function<void()> end_marker_cb_;
private:
  std::function<void(const std::string& error_message)> error_cb_;
private:
  snd_pcm_t*            handle_;
  std::atomic<unsigned> sample_rate_;
  std::atomic<int>      queued_frames_;
  std::string           device_name_;
  std::atomic<double>   replaygain_;
private:
  std::atomic<bool>     running_;
  cmdque_t              command_queue_;
  std::thread           thr_;
};

// ----------------------------------------------------------------------------
#endif // __audio_output_alsa_h__
