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

// ----------------------------------------------------------------------------
#include <taglib/fileref.h>
#include <taglib/flacfile.h>
#include <FLAC++/decoder.h>
#include <b64/encode.h>

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
namespace file_system
{
  file_system::type file_type(const std::string& pathname)
  {
    struct stat st;

    stat(pathname.c_str(), &st);

    switch (st.st_mode & S_IFMT)
    {
    case S_IFBLK:  return block_device;
    case S_IFCHR:  return char_device;
    case S_IFDIR:  return directory;
    case S_IFIFO:  return fifo;
    case S_IFLNK:  return symlink;
    case S_IFREG:  return file;
    case S_IFSOCK: return socket;
    default:       return unknown;
    }
  }

  void scan_dir(const std::string& dirname, std::function<void(const std::string& filename)> callback)
  {
    DIR *dir;
    struct dirent *dp;

    if ((dir = opendir(dirname.c_str())) == NULL)
    {
        perror ("Cannot open .");
        exit (1);
    }

    while ((dp = readdir(dir)) != NULL)
    {
      auto entry = std::string(dp->d_name);

      if ( ! (entry == "." || entry == "..") )
      {
        auto path = dirname+"/"+entry;
        auto type = file_type(path);

        if ( type == directory ) {
          scan_dir(path, callback);
        }
        else if ( type == file ) {
          callback(path);
        }
        else {
          // Ignore.
        }
      }
    }

    closedir(dir);
  }

  std::string extension(const std::string& filename)
  {
    size_t pos = filename.rfind('.');

    if ( pos != std::string::npos ) {
      return filename.substr(pos+1, std::string::npos);
    }
    else {
      return std::string();
    }
  }
}

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

// ----------------------------------------------------------------------------
json::array local_source::scan()
{
  json::array tracks;

  file_system::scan_dir(dirname_, [&](const std::string& filename)
  {
    TagLib::FileRef f(filename.c_str());

    if ( !f.isNull() )
    {
      auto tag = f.tag();

      json::object track{
        { "title",    tag->title().to8Bit(true) },
        { "artist",   tag->artist().to8Bit(true) },
        { "album",    json::object{ { "title", tag->album().to8Bit(true) } } },
        { "tn",       tag->track() },
        { "dn",       1 },
        { "duration", 0 },
        { "sources",  json::array{ json::object{ { "name", "local" }, { "uri", filename } } } }
      };

      if ( file_system::extension(filename) == "flac" )
      {
        TagLib::FLAC::File file(filename.c_str());

        const TagLib::List<TagLib::FLAC::Picture*>& images = file.pictureList();

        if ( images.size() > 0 )
        {
          TagLib::FLAC::Picture* image = images[0];

          if ( image->mimeType() == "image/jpeg" )
          {
            std::string image_data_s(reinterpret_cast<const char*>(image->data().data()), image->data().size());

            std::stringstream is;
            std::stringstream os;

            is.str(image_data_s);

            base64::encoder b64;

            b64.encode(is, os);

            json::object cover {
              { "image_format", "jpg" },
              { "image_data", os.str() }
            };

            auto& album = track["album"].as_object();

            album["cover"] = std::move(cover);
          }
          else
          {
            std::cerr << "unhandled image mime type - " << filename << " images=" << images.size() << ", mime type " << image->mimeType() << std::endl;
          }
        }
      }

      tracks.push_back(track);
    }
  });

  return std::move(tracks);
}