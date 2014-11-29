// ----------------------------------------------------------------------------
#include "catch.hpp"

// ----------------------------------------------------------------------------
#include <local_source.h>

// ----------------------------------------------------------------------------
TEST_CASE("test file system extension")
{
  auto ext = file_system::extension("test/test.flac");

  REQUIRE( ext == "flac" );
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
