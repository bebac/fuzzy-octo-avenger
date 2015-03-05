#\ -p 8213

require 'bundler'
Bundler.require

opal = Opal::Server.new {|s|
  s.append_path 'app/web'
  s.append_path 'app/web/css'
  s.append_path 'app/web/javascript'
  s.main = 'application'
  #s.debug = true
}

map opal.source_maps.prefix do
  run opal.source_maps
end

map '/assets' do
  run opal.sprockets
end

require './app/srv/application'

#run Sinatra::Application
map '/' do
  run TestApp::Main
end