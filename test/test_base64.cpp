// ----------------------------------------------------------------------------
#include "catch.hpp"

// ----------------------------------------------------------------------------
#include <base64.h>

// ----------------------------------------------------------------------------
TEST_CASE("test base64 encode")
{
  std::string test1 = "The quick brown fox";
  std::string test2 = "The quick brown fox ";
  std::string test3 = "The quick brown fox j";
  std::string res;

  res = base64::encode(test1.data(), test1.length());
  REQUIRE( res == "VGhlIHF1aWNrIGJyb3duIGZveA==" );

  res = base64::encode(test2.data(), test2.length());
  REQUIRE( res == "VGhlIHF1aWNrIGJyb3duIGZveCA=" );

  res = base64::encode(test3.data(), test3.length());
  REQUIRE( res == "VGhlIHF1aWNrIGJyb3duIGZveCBq" );
}

TEST_CASE("test base64 decode")
{
  std::string test1 = "VGhlIHF1aWNrIGJyb3duIGZveA==";
  std::string test2 = "VGhlIHF1aWNrIGJyb3duIGZveCA=";
  std::string test3 = "VGhlIHF1aWNrIGJyb3duIGZveCBq";
  std::string test4 = "MDEyMzQ1Njc4OTAxMjM0NTY3ODkwMTIzNDU2Nzg5MDEyMzQ1Njc4OTAxMjM0\nNTY3ODkwMTIzNDU2Nzg5\n";
  std::string res;

  res = base64::decode(test1.data(), test1.length());
  REQUIRE( res == "The quick brown fox" );

  res = base64::decode(test2.data(), test2.length());
  REQUIRE( res == "The quick brown fox " );

  res = base64::decode(test3.data(), test3.length());
  REQUIRE( res == "The quick brown fox j" );

  res = base64::decode(test4.data(), test4.length());
  REQUIRE( res == "012345678901234567890123456789012345678901234567890123456789" );
}

