require 'sinatra'

require_relative 'music_box'
require_relative 'spotify'

module TestApp

  class DeferrableBody
    include EventMachine::Deferrable

    def call(body)
      body.each do |chunk|
        @body_callback.call(chunk)
      end
    end

    def each(&blk)
      @body_callback = blk
    end
  end

  DEFAULT_CONTENT_TYPE = { "Content-Type" => 'application/json'}.freeze
  IMAGE_CONTENT_TYPE   = { "Content-Type" => 'image/jpeg', 'Cache-Control' => 'public, max-age=3600' }.freeze

  class Main < Sinatra::Base
    enable  :sessions

    use OmniAuth::Builder do
      provider :spotify, "8392143644964c5d84a51b65451f422c", "afd2e13d03ba4ace92f534bf2928e669",
               scope: 'user-read-email playlist-modify-public user-library-read user-library-modify'
    end

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
        req.errback  { |err| error(err || "unknown error") }
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

    get '/sources/spotify', :provides => :json do
      user = session[:spotify_user]
      if user
        {
          :display_name => user.display_name
        }.to_json
      else
        nil.to_json
      end
    end

    post '/sources/spotify/import' do
      user = session[:spotify_user]

      logger.info("begin import spotify saved tracks")

      if user
        queue = EM::Queue.new
        body  = DeferrableBody.new

        EM.next_tick { env['async.callback'].call [200, {'Content-Type' => 'application/json'}, body] }

        EventMachine.next_tick {
          # Get a list of the spotify uris already known to the music box daemon and
          # pass it on to the saved tracks importer.
          MusicBox.call("sources/spotify/uris") do |req|
            req.callback do |res|
              # Kick off the importer in the background.
              EventMachine.defer(
                proc {
                  Spotify.saved_tracks_importer(user).run(queue, res.to_set)
                },
                proc { |result|
                  puts "saved_tracks_import done"
                }
              )
            end
            # Return an error on failure.
            req.errback  do |err|
              puts "ERROR! #{err.inspect}"
              body.fail
            end
          end
        }
        # Start processing the queue.
        process_saved_tracks_importer(queue, body)
      else
        EM.next_tick { error({ "status" => "not logged in"}) }
      end
      [-1, {}, []]
    end

    get '/auth/spotify/callback' do
      session[:spotify_user] = RSpotify::User.new(request.env['omniauth.auth'])
      redirect '/#/settings'
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

    def process_saved_tracks_importer(queue, body)
      queue.pop { |track|
        if track
          MusicBox.call("db/import-tracks", [ track ]) do |req|
            req.callback { |res|
              puts "imported #{track[:title]} res=#{res}"
              process_saved_tracks_importer(queue, body)
            }
            req.errback  { |err| p err }
          end
        else
          body.call([{"status" => "ok" }.to_json])
          body.succeed
        end
      }
    end

  end
end
