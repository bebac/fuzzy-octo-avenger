// ----------------------------------------------------------------------------
//
//     Filename   : source_local.cpp
//
//     Author     : Benny Bach <benny.bach@gmail.com>
//                  Copyright (C) 2014
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------

#include "dm.h"
#include "source_local.h"

#include "../local_source.h"
#include "../file_system.h"

#include <taglib/fileref.h>
#include <taglib/flacfile.h>
#include <taglib/xiphcomment.h>

namespace dm
{
  kvstore* source_local::kvstore_ = nullptr;

  const std::string source_local_key = "source/local";
  const std::string directories_key  = "directories";

  void source_local::init(kvstore* store)
  {
    source_local::kvstore_ = store;
  }

  source_local::source_local()
  {
    auto data = kvstore_->get(source_local_key);

    if ( data.is_null() ) {
      data_ = json::object{ { directories_key, json::array() } };
    }
    else if ( data.is_object() ) {
      data_ = std::move(data.as_object());
    }
    else {
      // TODO: Error!
    }
  }

  std::vector<std::string> source_local::directories() const
  {
    std::vector<std::string> res;

    for ( auto& dir : data_.at(directories_key).as_array() )
    {
      if ( dir.is_string() ) {
        res.push_back(dir);
      }
      else {
        // TODO: Error!
      }
    }

    return std::move(res);
  }

  void source_local::directories(const std::vector<std::string>& dirs)
  {
    json::array new_dirs;

    for ( auto& dir : dirs ) {
      new_dirs.push_back(dir);
    }

    data_[directories_key] = new_dirs;

    kvstore_->set(source_local_key, data_);
  }

  void source_local::scan()
  {
    /////
    // Create a list of all existing local tracks.

    std::map<std::string, bool> local_track_ids;

    dm::track::each([&](dm::track& track) -> bool
    {
      auto source = track.find_source("local");

      if ( !source.is_null() ) {
        local_track_ids[track.id()] = false;
      }

      return true;
    });

    /////
    // Scan local source directories.

    for ( auto& dirname : directories() )
    {
      auto dir = file_system::directory{dirname};

      dir.each_file([&](const std::string& filename)
      {
        if ( file_system::extension(filename) == "flac" )
        {
          auto track = scan_flac_file(filename);

          if ( !track.is_null() )
          {
            local_track_ids[track.id()] = true;
          }
          else
          {
            // Invalid file?
          }
        }
      });
    }

    /////
    // Remove local source from the track ids remaining in the
    // local_tracks list.

    for ( auto& id : local_track_ids )
    {
      if ( id.second == false )
      {
        auto track = dm::track::find_by_id(id.first);

        std::cerr << "remove source 'local' from track " << track.to_json() << std::endl;

        track.source_remove("local");
      }
    }
  }

  dm::track source_local::scan_flac_file(const std::string& filename)
  {
    TagLib::FLAC::File file(filename.c_str());

    if ( file.isValid() )
    {
      int         tag_tn = 1;
      int         tag_dn = 1;
      std::string tag_title;
      std::string tag_artist;
      std::string tag_album;
      std::string tag_album_artist;
      std::string tag_disc_id;
      std::string alt_id;

      json::object replaygain;

      /////
      // Scan tags.

      TagLib::Ogg::XiphComment* xiph_comment = file.xiphComment();

      if ( xiph_comment )
      {
        for ( auto& field : xiph_comment->fieldListMap() )
        {
          if ( field.first == "TITLE" && field.second.size() > 0 ) {
            tag_title = field.second[0].to8Bit(true);
          }
          else if ( field.first == "ARTIST" && field.second.size() > 0 ) {
            tag_artist = field.second[0].to8Bit(true);
          }
          else if ( field.first == "ALBUM" && field.second.size() > 0 ) {
            tag_album = field.second[0].to8Bit(true);
          }
          else if ( (field.first == "ALBUM ARTIST" || field.first == "ALBUMARTIST") && field.second.size() > 0 ) {
            tag_album_artist = field.second[0].to8Bit(true);
          }
          else if ( (field.first == "TRACK NUMBER" || field.first == "TRACKNUMBER") && field.second.size() > 0 ) {
            tag_tn = std::stoi(field.second[0].to8Bit());
          }
          else if ( (field.first == "DISC NUMBER" || field.first == "DISCNUMBER") && field.second.size() > 0 ) {
            tag_dn = std::stoi(field.second[0].to8Bit());
          }
          else if ( field.first == "DISCID" && field.second.size() > 0 ) {
            tag_disc_id = field.second[0].to8Bit(true);
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
      }

      dm::track  track;
      dm::artist artist;
      dm::artist album_artist;
      dm::album  album;

      /////
      // Find track by DISCID/DISCNUMBER/TRACKNUMBER

      if ( tag_disc_id.length() > 0 )
      {
        alt_id = tag_disc_id+"/"+to_string(tag_dn)+"/"+to_string(tag_tn);
        track = track.find_by_alt_id(alt_id);
      }

      if ( !track.is_null() )
      {
        artist = track.artist();
        album = track.album();
      }
      else
      {
        /////
        // Find/create artist.

        if ( tag_album_artist.length() > 0 )
        {
          album_artist = dm::artist::find_by_name(tag_album_artist);

          if ( album_artist.is_null() )
          {
            // Create artist.
            album_artist.name(tag_album_artist);
            album_artist.save();
          }
        }

        if ( tag_artist.length() > 0 )
        {
          artist = dm::artist::find_by_name(tag_artist);

          if ( artist.is_null() )
          {
            // Create artist.
            artist.name(tag_artist);
            artist.save();
          }
        }

        if ( artist.is_null() )
        {
          // TODO: Error?
        }

        /////
        // Find/create/update album.

        if ( !album_artist.is_null() ) {
          album = album_artist.find_album_by_title(tag_album);
        }
        else {
          album = artist.find_album_by_title(tag_album);
        }

        album.title(tag_album);

        if ( !album_artist.is_null() )
        {
          album.member("artist", json::object{
            { "id",   album_artist.id() },
            { "name", album_artist.name() }
          });
        }
        else
        {
          album.member("artist", json::object{
            { "id",   artist.id() },
            { "name", artist.name() }
          });
        }

        if ( album.id_is_null() )
        {
          album.save();

          if ( !album_artist.is_null() )
          {
            album_artist.add_album(album);
            album_artist.save();
          }
          else
          {
            artist.add_album(album);
            artist.save();
          }
        }
        else
        {
          album.save();
        }

        /////
        // Create/update track.

        track = album.find_track_by_title_and_number(tag_title, tag_tn);

        if ( alt_id.length() > 0 ) {
          track.alt_id(alt_id);
        }
      }

      track.title(tag_title);
      track.track_number(tag_tn);
      track.disc_number(tag_dn);
      track.artist(artist);
      track.album(album);

      json::object source{ { "name", "local" }, { "uri", filename } };

      if ( !replaygain.empty() ) {
        source["replaygain"] = replaygain;
      }

      track.source(std::move(source));

      TagLib::FLAC::Properties* properties = file.audioProperties();
      if ( properties )
      {
        track.duration(properties->length());
      }

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

      /////
      // Create/update album cover.

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

      return std::move(track);
    }
    else
    {
      // TODO: Error!
      return track();
    }
  }
}