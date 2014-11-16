// ----------------------------------------------------------------------------
#include <dm/kvstore.h>
#include <dm/artist.h>
#include <dm/album.h>
#include <dm/dm.h>

// ----------------------------------------------------------------------------
#include "catch.hpp"

// ----------------------------------------------------------------------------
TEST_CASE("kvstore-create")
{
  dm::kvstore kvstore("test.kdb");

  REQUIRE(kvstore.count() == 2);
}

// ----------------------------------------------------------------------------
TEST_CASE("kvstore-create-artist-key")
{
  dm::kvstore kvstore("test.kdb");

  REQUIRE( kvstore.create_artist_key() == "ar0001" );
  REQUIRE( kvstore.create_artist_key() == "ar0002" );
}

// ----------------------------------------------------------------------------
TEST_CASE("kvstore-artist")
{
  dm::kvstore kvstore("test.kdb");

  REQUIRE(kvstore.count() == 2);

  dm::artist::init(&kvstore);
  dm::album::init(&kvstore);

  dm::artist artist;

  artist.name("Bruce Springsteen");
  artist.save();

  dm::artist a1 = dm::artist::find_by_id(artist.id());
  REQUIRE( !a1.is_null() );

  dm::artist a2 = dm::artist::find_by_name(artist.name());
  REQUIRE( !a2.is_null() );

  dm::album album;

  album.title("Born In The U.S.A.");
  album.save();

  CAPTURE( album.id() );
  REQUIRE( album.id() != "" );

  a2.add_album(album);
  a2.save();

  auto album_ids = a2.album_ids();

  REQUIRE( album_ids.size() == 1 );
}

// ----------------------------------------------------------------------------
TEST_CASE("kvstore-dm-init")
{
  dm::init();

  std::string artist_name("Bruce Springsteen");

  dm::artist artist = dm::artist::find_by_name(artist_name);

  if ( artist.is_null() )
  {
    artist.name("Bruce Springsteen");
    artist.save();

    WARN( "created atist '" << artist.name() << "'" );
  }
  else
  {
    WARN("found atist '" << artist.name() << "' by name");
  }

  dm::artist a1 = dm::artist::find_by_id(artist.id());
  REQUIRE( !a1.is_null() );
}
