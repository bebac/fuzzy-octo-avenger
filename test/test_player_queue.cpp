// ----------------------------------------------------------------------------
#include "catch.hpp"
// ----------------------------------------------------------------------------
#include <player_queue.h>

// ----------------------------------------------------------------------------
TEST_CASE("initially the queue should be empty", "[player_queue]")
{
  player_queue<std::string> q;

  REQUIRE( q.empty() );
}

// ----------------------------------------------------------------------------
TEST_CASE("push value on empty queue", "[player_queue]")
{
  player_queue<std::string> q;

  auto pos = q.push("first", 2);

  REQUIRE( !q.empty() );
  REQUIRE( q.front() == "first" );
  REQUIRE( pos == 1 );
}

// ----------------------------------------------------------------------------
TEST_CASE("same priority values should pop in fifo order", "[player_queue]")
{
  player_queue<std::string> q;

  REQUIRE( q.push("1", 2) == 1 );
  REQUIRE( q.push("2", 2) == 2 );

  REQUIRE( !q.empty() );
  REQUIRE( q.pop() == "1" );
  REQUIRE( q.pop() == "2" );
}

// ----------------------------------------------------------------------------
TEST_CASE("highest priority values should pop first in fifo order", "[player_queue]")
{
  player_queue<std::string> q;

  REQUIRE( q.push("3", 2) == 1 );
  REQUIRE( q.push("5", 3) == 2 );
  REQUIRE( q.push("1", 1) == 1 );
  REQUIRE( q.push("4", 2) == 3 );
  REQUIRE( q.push("2", 1) == 2 );

  REQUIRE( !q.empty() );
  REQUIRE( q.pop() == "1" );
  REQUIRE( q.pop() == "2" );
  REQUIRE( q.pop() == "3" );
  REQUIRE( q.pop() == "4" );
  REQUIRE( q.pop() == "5" );
}

// ----------------------------------------------------------------------------
TEST_CASE("erase specified priority from queue", "[player_queue]")
{
  player_queue<std::string> q;

  REQUIRE( q.push("1", 2) == 1 );
  REQUIRE( q.push("2", 3) == 2 );
  REQUIRE( q.push("3", 3) == 3 );
  REQUIRE( q.size() == 3 );

  q.erase_priority(3);

  REQUIRE( q.size() == 1 );
}