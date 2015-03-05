#!/bin/sh

cd webclient/sinatra

export RACK_ENV=production

bundle exec rackup config.ru

