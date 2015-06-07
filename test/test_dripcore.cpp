// ----------------------------------------------------------------------------
#include "catch.hpp"

// ----------------------------------------------------------------------------
#include <dripcore/loop.h>
#include <dripcore/acceptor.h>

// ----------------------------------------------------------------------------
#include <thread>
#include <atomic>

// ----------------------------------------------------------------------------
void run_dripcore_with_no_eventables(std::atomic<bool>& stopped)
{
  dripcore::loop loop;

  loop.run();

  stopped = true;
}

// ----------------------------------------------------------------------------
void run_dripcore_with_eventable(std::shared_ptr<dripcore::eventable> eventable, std::atomic<bool>& stopped)
{
  dripcore::loop loop;

  loop.start(eventable);
  loop.run();

  stopped = true;
}

// ----------------------------------------------------------------------------
void run_dripcore(dripcore::loop& loop, std::atomic<bool>& stopped)
{
  loop.run();
  stopped = true;
}

// ----------------------------------------------------------------------------
TEST_CASE("should return immediately when started with no eventables")
{
  std::atomic<bool> stopped;

  std::thread dripcore(run_dripcore_with_no_eventables, std::ref(stopped));

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  REQUIRE( stopped == true );
  REQUIRE( dripcore.joinable() == true );

  dripcore.join();
}

// ----------------------------------------------------------------------------
TEST_CASE("should shot down when there is noo more eventables")
{
  std::atomic<bool> stopped;

  auto acceptor = std::make_shared<dripcore::acceptor>("127.0.0.1", 8212, [&](dripcore::socket client) {});

  std::thread dripcore(run_dripcore_with_eventable, acceptor, std::ref(stopped));

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  acceptor->stop();

  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  REQUIRE( stopped == true );
  REQUIRE( dripcore.joinable() == true );

  dripcore.join();
}

// ----------------------------------------------------------------------------
TEST_CASE("should shot down on request")
{
  std::atomic<bool> stopped;

  dripcore::loop loop;

  auto acceptor = std::make_shared<dripcore::acceptor>("127.0.0.1", 8212, [&](dripcore::socket client) {});

  loop.start(acceptor);

  std::thread dripcore(run_dripcore, std::ref(loop), std::ref(stopped));

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  loop.shutdown();

  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  REQUIRE( stopped == true );
  REQUIRE( dripcore.joinable() == true );

  dripcore.join();
}