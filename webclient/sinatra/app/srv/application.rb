require 'sinatra'

require_relative 'music_box'

module MusicBox
  IP   = '127.0.0.1'.freeze
  PORT = 8212.freeze

  def self.conn
    @conn ||= EventMachine::connect IP, PORT, MusicBox::Connection, IP, PORT
  end

  def self.call(method, params=nil, timeout=5, &blk)
    MusicBox.conn.invoke(method, params, timeout, &blk)
  end

end

module TestApp

  DEFAULT_CONTENT_TYPE = { "Content-Type" => 'application/json'}.freeze
  IMAGE_CONTENT_TYPE   = { "Content-Type" => 'image/jpeg', 'Cache-Control' => 'public, max-age=3600' }.freeze

  class Main < Sinatra::Base
    set :views, 'app/srv/views'

    get '/' do
      erb :'index.html'
    end

    get '/sources/local' do
      MusicBox.call("db/get/source_local") do |req|
        req.callback { |res| ok(res.to_json) }
        req.errback  { |err| error(err) }
      end
      [-1, {}, []]
    end

    post '/sources/local' do
      MusicBox.call("db/set/source_local", JSON::parse(request.body.read.to_s)) do |req|
        req.callback { |res| ok(res.to_json) }
        req.errback  { |err| error(err) }
      end
      [-1, {}, []]
    end

    post '/sources/local/scan' do
      MusicBox.call("sources/local/scan", nil, 600) do |req|
        req.callback { |res| ok(res.to_json) }
        req.errback  { |err| error(err) }
      end
      [-1, {}, []]
    end

    get '/albums' do
      MusicBox.call("db/get/albums") do |req|
        req.callback { |res| ok(res.to_json) }
        req.errback  { |err| error(err) }
      end
      [-1, {}, []]
    end

    get '/albums/:id/tracks' do |id|
      MusicBox.call("db/get/album/tracks", id) do |req|
        req.callback { |res| ok(res.to_json) }
        req.errback  { |err| error(err) }
      end
      [-1, {}, []]
    end

    get '/albums/:id/cover' do |id|
      MusicBox.call("db/cover", { "album_id" => id }) do |req|
        req.callback { |res| ok(Base64.decode64(res['image_data']), IMAGE_CONTENT_TYPE) }
        req.errback  { |err| not_found(err || "unknown error") }
      end
      [-1, {}, []]
    end

    post '/tracks/:id/queue' do |id|
      MusicBox.call("player/queue", { "id" => id }) do |req|
        req.callback { |res| ok(res.to_json) }
        req.errback  { |err| error(err) }
      end
      [-1, {}, []]
    end

    post '/tracks/:id/tags' do |id|
      MusicBox.call("db/save", { "id" => id, "tags" => JSON::parse(request.body.read.to_s) }) do |req|
        req.callback { |res| ok(res.to_json) }
        req.errback  { |err| error(err) }
      end
      [-1, {}, []]
    end

private

    def ok(body, headers=DEFAULT_CONTENT_TYPE)
      env['async.callback'].call([ 200, headers, body ])
    end

    def not_found(body, headrs={})
      env['async.callback'].call([ 404, headers, body ])
    end

    def error(body, headrs=DEFAULT_CONTENT_TYPE)
      env['async.callback'].call([ 500, headers, body ])
    end

  end
end