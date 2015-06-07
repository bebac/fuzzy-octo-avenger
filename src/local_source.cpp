// ----------------------------------------------------------------------------
//
//     Filename   : local_source.cpp
//
//     Author     : Benny Bach <benny.bach@gmail.com>
//                  Copyright (C) 2014
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------

#include "local_source.h"
#include "file_system.h"
#include "dm/dm.h"

// ----------------------------------------------------------------------------
#include <taglib/fileref.h>
#include <taglib/flacfile.h>
#include <taglib/xiphcomment.h>
#include <FLAC++/decoder.h>

// ----------------------------------------------------------------------------
#include <iostream>

// ----------------------------------------------------------------------------
struct output_frame
{
    FLAC__int32 l;
    FLAC__int32 r;
};

// ----------------------------------------------------------------------------
class flac_decoder : public FLAC::Decoder::File
{
  using audio_output_ptr = std::weak_ptr<audio_output_t>;
public:
  flac_decoder(const std::string& filename)
    :
    FLAC::Decoder::File(),
    audio_output_()
  {
    auto res = init(filename.c_str());

    if ( FLAC__STREAM_DECODER_INIT_STATUS_OK != res )
    {
      std::stringstream ss;
      ss << "decoder init error " << res;
      throw std::runtime_error(ss.str());
    }
  }
public:
  void play(audio_output_ptr audio_output)
  {
    audio_output_ = audio_output;

    process_until_end_of_stream();

    auto output = audio_output_.lock();
    if ( output.get() )
    {
      output->write_end_marker();
    }
  }
protected:
  FLAC__StreamDecoderReadStatus read_callback(FLAC__byte buffer[], size_t *bytes)
  {
    std::cout << "read_callback size=" << *bytes << std::endl;
    return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
  }
protected:
  FLAC__StreamDecoderWriteStatus write_callback(const FLAC__Frame* frame, const FLAC__int32* const buffer[])
  {
    auto output_buffer = std::unique_ptr<output_frame[]>(new output_frame[frame->header.blocksize]);

    for( size_t i = 0; i < frame->header.blocksize; i++ )
    {
      if ( bits_per_sample_ == 16 )
      {
        FLAC__int16 l16 = (FLAC__int16)buffer[0][i];
        FLAC__int16 r16 = (FLAC__int16)buffer[1][i];

        output_buffer[i].l = ((FLAC__int32)l16<<16);
        output_buffer[i].r = ((FLAC__int32)r16<<16);
      }
      else if ( bits_per_sample_ == 24 )
      {
        FLAC__int32 l24 = (FLAC__int32)buffer[0][i];
        FLAC__int32 r24 = (FLAC__int32)buffer[1][i];

        output_buffer[i].l = ((FLAC__int32)l24<<8);
        output_buffer[i].r = ((FLAC__int32)r24<<8);
      }
      else
      {
        std::cerr << "unhanled bits_per_sample " << bits_per_sample_ << std::endl;
        throw std::runtime_error("unhandled bits_per_sample");
      }
    }

    auto output = audio_output_.lock();
    if ( output.get() )
    {
      output->write_s32_le_i(&output_buffer[0], frame->header.blocksize);
    }

    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
  }
protected:
  void metadata_callback(const ::FLAC__StreamMetadata *metadata)
  {
    if ( metadata->type == FLAC__METADATA_TYPE_STREAMINFO )
    {
      //auto total_samples = metadata->data.stream_info.total_samples;
      auto sample_rate = metadata->data.stream_info.sample_rate;
      //auto channels = metadata->data.stream_info.channels;
      //auto bits_per_sample = metadata->data.stream_info.bits_per_sample;
      bits_per_sample_ = metadata->data.stream_info.bits_per_sample;

      std::cerr << "metadata_callback sample_rate=" << sample_rate << ", bits_per_sample=" << bits_per_sample_ << std::endl;

      auto output = audio_output_.lock();
      if ( output.get() )
      {
        output->set_sample_rate(sample_rate);
        output->write_start_marker();
      }
    }
    else {
      std::cerr << "metadata_callback" << std::endl;
    }
  }
protected:
  void error_callback (::FLAC__StreamDecoderErrorStatus status)
  {
  }
private:
  unsigned         bits_per_sample_;
  audio_output_ptr audio_output_;
};

// ----------------------------------------------------------------------------
void local_source::play(const std::string& uri, audio_output_ptr audio_output)
{
  flac_decoder stream(uri);
  stream.play(audio_output);
}

// ----------------------------------------------------------------------------
json::value local_source::get_cover(const std::string& uri)
{
  return json::value("cover not found");
}

// --------------------------------------------------------------------------
#if 0
void import_flac_file(const std::string& filename)
{
  //std::cerr << "import_flac_file " << filename << std::endl;

  TagLib::FLAC::File file(filename.c_str());

  auto tag = file.tag();

  auto artist_name = tag->artist().to8Bit(true);
  auto album_title = tag->album().to8Bit(true);
  auto track_title = tag->title().to8Bit(true);

  auto artist = dm::artist::find_by_name(artist_name);

  if ( artist.is_null() )
  {
    // Create artist.
    artist.name(artist_name);
    artist.save();
  }

  auto album = artist.find_album_by_title(album_title);

  if ( album.is_null() )
  {
    // Create album.
    album.title(album_title);
    album.member("artist", json::object{ { "id", artist.id() }, { "name", artist.name() } });
    album.save();

    // Add album to artist albums.
    artist.add_album(album);
    artist.save();
  }

  auto track = album.find_track_by_title_and_number(track_title, tag->track());

  // Set/update track attributes.

  track.title(track_title);
  track.track_number(tag->track());
  track.disc_number(1);

  // Create track source object.
  json::object source{ { "name", "local" }, { "uri", filename } };

  TagLib::FLAC::Properties* properties = file.audioProperties();
  if ( properties )
  {
    track.duration(properties->length());
  }

  TagLib::Ogg::XiphComment* xiph_comment = file.xiphComment();
  if ( xiph_comment )
  {
    auto field_map = xiph_comment->fieldListMap();

    json::object replaygain;

    for ( auto& field : field_map )
    {
      if ( field.first == "TRACK NUMBER" || field.first == "TRACKNUMBER" )
      {
        if ( field.second.size() > 0 ) {
          track.track_number(std::stoi(field.second[0].to8Bit()));
        }
        else {
          std::cerr << "field='" << field.first << "' string list size=" << field.second.size() << std::endl;
        }
      }
      else if ( field.first == "DISC NUMBER" || field.first == "DISCNUMBER" )
      {
        if ( field.second.size() > 0 ) {
          track.disc_number(std::stoi(field.second[0].to8Bit()));
        }
        else {
          std::cerr << "field='" << field.first << "' string list size=" << field.second.size() << std::endl;
        }
      }
      else if ( field.first == "REPLAYGAIN_REFERENCE_LOUDNESS" )
      {
        auto ref_loudness = std::stod(field.second[0].to8Bit());
        replaygain["reference_loudness"] = ref_loudness;
      }
      else if ( field.first == "REPLAYGAIN_TRACK_GAIN" )
      {
        auto gain = std::stod(field.second[0].to8Bit());
        replaygain["track_gain"] = gain;
      }
    }

    if ( !replaygain.empty() ) {
      source["replaygain"] = replaygain;
    }
  }

  const TagLib::List<TagLib::FLAC::Picture*>& images = file.pictureList();

  if ( images.size() > 0 )
  {
    TagLib::FLAC::Picture* image = images[0];

    if ( image->mimeType() == "image/jpeg" )
    {
      auto cover = dm::album_cover::find_by_album_id(album.id());

      if ( cover.is_null() )
      {
        cover.format("jpg");
        cover.data(reinterpret_cast<const char*>(image->data().data()), image->data().size());
        cover.save();
      }
    }
    else
    {
      std::cerr << "unhandled image mime type - " << filename << " images=" << images.size() << ", mime type " << image->mimeType() << std::endl;
    }
  }

  track.artist(artist);
  track.album(album);
  track.source(std::move(source));

  if ( track.id_is_null() )
  {
    // Create track id.
    track.save();
    // Add new track to album.
    album.add_track(track);
    album.save();
  }
  else
  {
    track.save();
  }
}
#endif