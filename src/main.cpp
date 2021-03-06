// ----------------------------------------------------------------------------
//
//     Filename   : main.cpp
//
//     Author     : Benny Bach <benny.bach@gmail.com>
//                  Copyright (C) 2014
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#include <program_options.h>
#include <dripcore/acceptor.h>
#include <connection.h>
#include <player.h>
#include <player_json_rpc.h>
#include <local_source.h>
#include <spotify_source.h>
#include <dm/dm.h>

// ----------------------------------------------------------------------------
#include <iostream>
#include <fstream>

// ----------------------------------------------------------------------------
#include <unistd.h>
#include <signal.h>


// ----------------------------------------------------------------------------
class options : public program_options::container
{
public:
  options() : help(false), foreground(false), conf_filename()
  {
    add('h', "help", "display this message", help);
    add('F', "foreground", "do not detach from starting terminal", foreground);
    add('c', "conf", "configuration filename", conf_filename);
  }
public:
  bool        help;
  bool        foreground;
  std::string conf_filename;

  /////
  // Configuration file options.

  std::string spotify_username;
  std::string spotify_password;
  std::string audio_device;
};

// ----------------------------------------------------------------------------
void print_usage(const options& options)
{
  std::cout
    << "Usage: spotd v0.0.1 [OPTION...] FILE..." << std::endl
    << std::endl
    << "Available option:" << std::endl
    << options << std::endl
    << std::endl;
}

// ----------------------------------------------------------------------------
std::unique_ptr<dripcore::loop> loop_;

// ----------------------------------------------------------------------------
void sig_handler(int signum)
{
  std::cerr << "got signal " << signum << std::endl;
  loop_->shutdown();
}

// ----------------------------------------------------------------------------
void run(const options& options)
{
  dm::init();

  loop_.reset(new dripcore::loop);

  jsonrpc::service service;
  player           player(options.audio_device);

  if ( signal(SIGPIPE, sig_handler) == SIG_ERR ) {
    std::cerr << "Error installing SIGPIPE handler!" << std::endl;
  }

  if ( signal(SIGINT, sig_handler) == SIG_ERR ) {
    std::cerr << "Error installing SIGINT handler!" << std::endl;
  }

  if ( signal(SIGTERM, sig_handler) == SIG_ERR ) {
    std::cerr << "Error installing SIGTERM handler!" << std::endl;
  }

  spotify_source_config sp_cfg{
    options.spotify_username,
    options.spotify_password,
  };

  player.add_source("spotify", std::make_shared<spotify_source>(sp_cfg));
  player.add_source("local", std::make_shared<local_source>());

  using std::placeholders::_1;
  service.add_method("player/play",          std::bind(&json_rpc::play,                std::ref(player), _1));
  service.add_method("player/queue",         std::bind(&json_rpc::queue,               std::ref(player), _1));
  service.add_method("player/skip",          std::bind(&json_rpc::skip,                std::ref(player), _1));
  service.add_method("player/stop",          std::bind(&json_rpc::stop,                std::ref(player), _1));
  service.add_method("player/state",         std::bind(&json_rpc::state,               std::ref(player), _1));
#if 0
  service.add_method("db/tags",          std::bind(&json_rpc::tags,                std::ref(player), _1));
  service.add_method("db/export-tracks", std::bind(&json_rpc::export_tracks,       std::ref(player), _1));
#endif
  service.add_method("db/index",             std::bind(&json_rpc::index,                _1));
  service.add_method("db/save",              std::bind(&json_rpc::save,                 _1));
  service.add_method("db/delete",            std::bind(&json_rpc::erase,                _1));
  service.add_method("db/import-tracks",     std::bind(&json_rpc::import_tracks,        _1));
  service.add_method("db/cover",             std::bind(&json_rpc::cover,                _1));
  service.add_method("db/get/artists",       std::bind(&json_rpc::get_artists,          _1));
  service.add_method("db/get/albums",        std::bind(&json_rpc::get_albums,           _1));
  service.add_method("db/set/album",         std::bind(&json_rpc::set_album,            _1));
  service.add_method("db/get/album/tracks",  std::bind(&json_rpc::get_album_tracks,     _1));
  service.add_method("db/get/tracks",        std::bind(&json_rpc::get_tracks,           _1));
  service.add_method("db/get/source_local",  std::bind(&json_rpc::get_source_local,     _1));
  service.add_method("db/set/source_local",  std::bind(&json_rpc::set_source_local,     _1));
  service.add_method("sources/local/scan",   std::bind(&json_rpc::sources_local_scan,   _1));
  service.add_method("sources/spotify/uris", std::bind(&json_rpc::sources_spotify_uris, _1));

  /////
  // Setup callback to get player state info. Note that the callback is
  // called from player thread context. The service send_notification method
  // will push a send notifition for each connection to be executed in dripcore
  // context.
  //
  player.set_state_info_callback([&](const player_state_info& info)
  {
    json::object params
    {
      { "state",  info.state },
      { "track",  info.track.to_json() },
      { "source", info.source }
    };

    service.send_notification(json_rpc_notification("player/event", params));
  });

  auto acceptor = std::make_shared<dripcore::acceptor>("0.0.0.0", 8212,
    [&](dripcore::socket client) {
      loop_->start(std::make_shared<jsonrpc::server::connection>(service, std::move(client)));
    });

  loop_->start(acceptor);
  loop_->run();

  std::cerr << "shutdown player!" << std::endl;

  player.shutdown();
}

// ----------------------------------------------------------------------------
void load_configuration(const std::string& filename, options& options)
{
  std::ifstream f(filename);

  if ( ! f.good() ) {
    return;
  }

  std::string str((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

  json::value  doc;
  json::parser parser(doc);

  try
  {
    parser.parse(str.c_str(), str.length());

    if ( !doc.is_object() ) {
      throw std::runtime_error("configuration file must be a json object!");
    }

    json::object conf = doc.as_object();

    if ( conf["spotify_username"].is_string() ) {
      options.spotify_username = conf["spotify_username"].as_string();
    }

    if ( conf["spotify_password"].is_string() ) {
      options.spotify_password = conf["spotify_password"].as_string();
    }

    if ( conf["audio_device"].is_string() ) {
      options.audio_device = conf["audio_device"].as_string();
    }
  }
  catch (const std::exception& e)
  {
    std::string err("configuration file parse error: ");
    err += std::string(e.what());
    throw std::runtime_error(err.c_str());
  }
}

// ----------------------------------------------------------------------------
void fork_and_run(const options& options)
{
  pid_t child;

  child = fork();

  if (child == -1) {
    throw std::runtime_error("failed to fork");
  }

  if ( child > 0 ) {
    // Terminate parent process.
    exit(0);
  }

  /////
  // Child process.

  setsid();

  try
  {
    run(options);
  }
  catch(const std::exception& err)
  {
    std::cerr << err.what() << std::endl;
  }
}

// ----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
  options options;

  try
  {
    options.parse(argc, argv);

    if ( options.help ) {
      print_usage(options);
    }
    else if ( options.foreground )
    {
      load_configuration(options.conf_filename, options);
      run(options);
    }
    else
    {
      load_configuration(options.conf_filename, options);
      fork_and_run(options);
    }
  }
  catch(const program_options::error& err) {
    std::cerr << err.what() << std::endl
              << "try: example --help" << std::endl;
  }
  catch(const std::exception& err) {
    std::cerr << err.what() << std::endl;
    return 2;
  }

  return 0;
}