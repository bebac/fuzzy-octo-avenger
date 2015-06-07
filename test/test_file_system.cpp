// ----------------------------------------------------------------------------
#include "catch.hpp"

// ----------------------------------------------------------------------------
#include <local_source.h>
#include <file_system.h>

// ----------------------------------------------------------------------------
TEST_CASE("test file system extension")
{
  auto ext = file_system::extension("test/test.flac");

  REQUIRE( ext == "flac" );
}

// ----------------------------------------------------------------------------
TEST_CASE("test file system status is true when file exists")
{
  auto fstat = file_system::status{"test/music/tone16bit.flac"};

  REQUIRE( fstat == true );
  REQUIRE( fstat.is_file() == true );
  REQUIRE( fstat.is_directory() == false );
  REQUIRE( fstat.is_block_device() == false );
  REQUIRE( fstat.mtime() > std::chrono::system_clock::time_point());

  auto dstat = file_system::status{"test/music"};

  REQUIRE( dstat == true );
  REQUIRE( dstat.is_file() == false );
  REQUIRE( dstat.is_directory() == true );
  REQUIRE( dstat.is_block_device() == false );
  REQUIRE( dstat.mtime() > std::chrono::system_clock::time_point());
}

// ----------------------------------------------------------------------------
TEST_CASE("test file system status is false when file does not exists")
{
  auto stat = file_system::status{"xxx"};

  REQUIRE( stat == false );
  REQUIRE_THROWS_AS(stat.is_directory(), file_system::error);
  REQUIRE_THROWS_AS(stat.is_block_device(), file_system::error);
  REQUIRE_THROWS_AS(stat.mtime(), file_system::error);
}

// ----------------------------------------------------------------------------
TEST_CASE("test file system directory")
{
  auto dir = file_system::directory{"test/music"};

  REQUIRE( dir == true );

  auto count = 0;
  dir.each_file([&](const std::string& filename)
  {
    REQUIRE( filename == "test/music/tone16bit.flac");
    count++;
  });
  REQUIRE( count == 1 );

  dir.rewind();

  count = 0;
  dir.each_file([&](const std::string& filename)
  {
    REQUIRE( filename == "test/music/tone16bit.flac");
    count++;
  });
  REQUIRE( count == 1 );

  auto no_dir = file_system::directory{"xxx"};

  REQUIRE_THROWS_AS(no_dir.each_file([&](const std::string& filename){}), file_system::error);
}


#if 0
// ----------------------------------------------------------------------------
TEST_CASE("read flac tags")
{
  auto source = local_source("test/music");

  auto tracks = source.scan();

  REQUIRE( tracks.size() == 1 );
  REQUIRE( tracks[0].is_object() == true );

  auto track = tracks[0].as_object();

  REQUIRE( track["duration"].is_number() == true );
  REQUIRE( track["duration"].as_number() > 0 );
  REQUIRE( track["sources"].is_array() == true );

  auto& sources = track["sources"].as_array();

  REQUIRE( sources.size() == 1 );

  auto& s  = sources[0].as_object();
  auto& rg = s["replaygain"];

  INFO( "source " << to_string(s) );

  REQUIRE( rg.is_object() == true );
}
#endif
