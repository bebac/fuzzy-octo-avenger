require 'sinatra'
require 'json'
require 'coffee-script'

require_relative 'em-spotihifi-client'

module Player

  IP   = '127.0.0.1'.freeze
  PORT = 8212.freeze

  def self.api
    @client ||= EventMachine::connect IP, PORT, SpotiHifi::Client, IP, PORT
  end

  def self.call(method, params, env)
    Player.api.invoke(method, params) do |req|
      req.callback do |result|
        env['async.callback'].call([ 200, { "Content-Type" => 'application/json' }, result.to_json ])
      end
      req.errback do |error|
        puts "call error #{error.inspect}"
        env['async.callback'].call([ 500, {"Content-Type" => 'application/json'}, error || "unknown error" ])
      end
    end
    [-1, {}, []]
  end

end

module TestApp
  class Main < Sinatra::Base
    set :views, 'source/views'

    get '/' do
      erb :'index.html'
    end

    get '/api/index' do
      Player.call("db/index", [], env)
    end

    post '/api/queue' do
      #Player.call("player/queue", { "id" => params['id'].to_i }, env)
      Player.call("player/queue", { "id" => params['id'] }, env)
    end

    post '/api/tags' do
      tags = params['tags'] || []
      Player.call("db/save", { "id" => params['id'], "tags" => tags }, env)
    end

    post '/api/play' do
      Player.call("player/play", nil, env)
    end

    post '/api/skip' do
      Player.call("player/skip", [], env)
    end

    post '/api/stop' do
      Player.call("player/stop", [], env)
    end

    get '/api/state' do
      Player.call("player/state", [], env)
    end

    get '/api/cover' do
      if params.key?("album_id")
        rpc_params = { "album_id" => params['album_id'] }
      else
        rpc_params = { "track_id" => params['track_id'] }
      end

      Player.api.invoke("db/cover", rpc_params) do |req|

        req.callback do |result|
          env['async.callback'].call([ 200, { "Content-Type" => 'image/jpeg', 'Cache-Control' => 'public, max-age=3600' }, Base64.decode64(result['image_data']) ])
        end

        req.errback do |error|
          env['async.callback'].call([ 404, {}, error || "unknown error" ])
        end

      end
      [-1, {}, []]
    end

    get '/api/events', provides: 'text/event-stream' do
      stream(:keep_open) do |out|
        EM.next_tick {
          # Keep connection alive.
          timer = EM.add_periodic_timer(10) {
            out << "data: #{{}.to_json}\n\n"
          }

          # Subscribe to player/event
          sid = Player.api.subscribe do |msg|
            out << "data: #{msg.to_json}\n\n"
          end

          out.callback do
            p "unsubscribe sid=#{sid}"
            Player.api.unsubscribe(sid)
            timer.cancel
          end
        }

        stream.errback do
          logger.info "subscribe failed!"
        end
      end
    end
  end
end
