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
