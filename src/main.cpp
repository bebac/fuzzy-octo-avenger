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
#include <acceptor.h>
#include <connection.h>
#include <player.h>
#include <player_json_rpc.h>
#include <database.h>
#include <local_source.h>
#include <spotify_source.h>

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
  std::vector<std::string> local_source_dirs;
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
void sig_handler(int signum)
{
  std::cerr << "got signal " << signum << std::endl;
}

// ----------------------------------------------------------------------------
void run(const options& options)
{
  dripcore::loop   loop;
  jsonrpc::service service(loop);
  player           player(options.audio_device);

#if 0
  if ( signal(SIGPIPE, sig_handler) == SIG_ERR ) {
    std::cerr << "Error installing SIGPIPE handler!" << std::endl;
  }
#endif

  if ( signal(SIGINT, sig_handler) == SIG_ERR ) {
    std::cerr << "Error installing SIGINT handler!" << std::endl;
  }

  if ( signal(SIGTERM, sig_handler) == SIG_ERR ) {
    std::cerr << "Error installing SIGTERM handler!" << std::endl;
  }

  spotify_source_config sp_cfg{
    options.spotify_username,
    options.spotify_password
  };

  player.add_source("spotify", std::make_shared<spotify_source>(sp_cfg));

  if ( options.local_source_dirs.size() > 0 )
  {
    player.add_source("local", std::make_shared<local_source>(options.local_source_dirs[0]));
  }

  using std::placeholders::_1;
  service.add_method("player/play",      std::bind(&json_rpc::play,                std::ref(player), _1));
  service.add_method("player/queue",     std::bind(&json_rpc::queue,               std::ref(player), _1));
  service.add_method("player/skip",      std::bind(&json_rpc::skip,                std::ref(player), _1));
  service.add_method("player/stop",      std::bind(&json_rpc::stop,                std::ref(player), _1));
  service.add_method("player/state",     std::bind(&json_rpc::state,               std::ref(player), _1));
  service.add_method("player/cover",     std::bind(&json_rpc::cover,               std::ref(player), _1));
  service.add_method("player/ctpb",      std::bind(&json_rpc::continuous_playback, std::ref(player), _1));
  service.add_method("db/index",         std::bind(&json_rpc::index,               std::ref(player), _1));
  service.add_method("db/save",          std::bind(&json_rpc::save,                std::ref(player), _1));
  service.add_method("db/delete",        std::bind(&json_rpc::erase,               std::ref(player), _1));
  service.add_method("db/tags",          std::bind(&json_rpc::tags,                std::ref(player), _1));
  service.add_method("db/export-tracks", std::bind(&json_rpc::export_tracks,       std::ref(player), _1));
  service.add_method("db/import-tracks", std::bind(&json_rpc::import_tracks,       std::ref(player), _1));
  service.add_method("local/scan",       std::bind(&json_rpc::local_scan,          std::ref(player), _1));

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
      { "track",  to_json(*info.track) },
      { "source", info.source }
    };

    service.send_notification(json_rpc_notification("player/event", params));
  });

  auto acceptor = std::make_shared<jsonrpc::server::acceptor>([&](basic_socket client) {
      loop.start(std::make_shared<jsonrpc::server::connection>(service, std::move(client)));
    });

  loop.start(acceptor);
  loop.run();
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

    if ( conf["local_source"].is_object() )
    {
      auto local_source = conf["local_source"].as_object();

      if ( local_source["directories"].is_array() )
      {
        auto dirs = local_source["directories"].as_array();

        for ( auto& dir : dirs )
        {
          if ( dir.is_string() ) {
            options.local_source_dirs.push_back(dir.as_string());
          }
          else {
            throw std::runtime_error("local source directory must be a string!");
          }
        }
      }
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