require 'sprockets'
require './source/application'

map '/assets' do
  environment = Sprockets::Environment.new do |env|
    env.append_path 'source/javascript'
    env.append_path 'source/css'
    env.append_path 'source/images'
  end
  run environment
end

map '/' do
  run TestApp::Main
end
