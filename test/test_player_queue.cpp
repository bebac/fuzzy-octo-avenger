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

  q.push("first", 2);

  REQUIRE( !q.empty() );
  REQUIRE( q.front() == "first" );
}

// ----------------------------------------------------------------------------
TEST_CASE("same priority values should pop in fifo order", "[player_queue]")
{
  player_queue<std::string> q;

  q.push("1", 2);
  q.push("2", 2);

  REQUIRE( !q.empty() );
  REQUIRE( q.pop() == "1" );
  REQUIRE( q.pop() == "2" );
}

// ----------------------------------------------------------------------------
TEST_CASE("highest priority values should pop first in fifo order", "[player_queue]")
{
  player_queue<std::string> q;

  q.push("3", 2);
  q.push("5", 3);
  q.push("1", 1);
  q.push("4", 2);
  q.push("2", 1);

  REQUIRE( !q.empty() );
  REQUIRE( q.pop() == "1" );
  REQUIRE( q.pop() == "2" );
  REQUIRE( q.pop() == "3" );
  REQUIRE( q.pop() == "4" );
  REQUIRE( q.pop() == "5" );
}