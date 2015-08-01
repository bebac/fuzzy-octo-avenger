#\ -p 8213

require 'bundler'
Bundler.require

opal = Opal::Server.new {|s|
  s.append_path 'app/web'
  s.append_path 'app/web/images'
  s.append_path 'app/web/css'
  s.append_path 'app/web/javascript'
  s.main = 'application'
  s.debug = true
}

sprockets   = opal.sprockets
maps_prefix = '/__OPAL_SOURCE_MAPS__'
maps_app    = Opal::SourceMapServer.new(sprockets, maps_prefix)

# Monkeypatch sourcemap header support into sprockets
::Opal::Sprockets::SourceMapHeaderPatch.inject!(maps_prefix)

map maps_prefix do
  run maps_app
end

map '/assets' do
  run opal.sprockets
end

#require './app/srv/application'
require_relative 'app/srv/application'

#run Sinatra::Application
map '/' do
  run TestApp::Main.new(nil, sprockets: sprockets)
end