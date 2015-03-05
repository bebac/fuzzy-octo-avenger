require_relative '../webclient/sinatra/source/em-spotihifi-client'

namespace :db do

  class MusicBoxTrack < OpenStruct

    def spotify_source
      sources.each do |src|
        if src["name"] == "spotify"
          return src
        end
      end
      nil
    end

  end

  class MusicBoxAlbum < OpenStruct
  end

  class MusicBox

    def initialize(ip, port=8212)
      @ip = ip
      @port = port
    end

    def artists
      res = []
      EventMachine.run {
        client = EventMachine::connect @ip, @port, SpotiHifi::Client, @ip, @port

        client.invoke("db/get/artists", nil) do |req|
          req.timeout 2

          req.callback do |result|
            res = result
            EventMachine.stop
          end

          req.errback do |error|
            EventMachine.stop
            raise error
          end
        end
      }
      res
    end

    def albums
      res = []
      EventMachine.run {
        client = EventMachine::connect @ip, @port, SpotiHifi::Client, @ip, @port

        client.invoke("db/get/albums", nil) do |req|
          req.timeout 2

          req.callback do |result|
            res = result
            EventMachine.stop
          end

          req.errback do |error|
            EventMachine.stop
            raise error
          end
        end
      }
      res
    end

    def album_tracks(album_id)
      res = []
      EventMachine.run {
        client = EventMachine::connect @ip, @port, SpotiHifi::Client, @ip, @port

        client.invoke("db/get/album/tracks", album_id) do |req|
          req.timeout 2

          req.callback do |result|
            result.each do |track|
              res << MusicBoxTrack.new(track)
            end
            EventMachine.stop
          end

          req.errback do |error|
            EventMachine.stop
            raise error
          end
        end
      }
      res
    end

    def save_album(album)
      res = nil
      EventMachine.run {
        client = EventMachine::connect @ip, @port, SpotiHifi::Client, @ip, @port

        client.invoke("db/set/album", album) do |req|
          req.timeout 2

          req.callback do |result|
            res = result
            EventMachine.stop
          end

          req.errback do |error|
            EventMachine.stop
            res = error
          end
        end
      }
      res
    end

    def tracks
      res = nil
      EventMachine.run {
        client = EventMachine::connect @ip, @port, SpotiHifi::Client, @ip, @port

        client.invoke("db/get/tracks", nil) do |req|
          req.timeout 2

          req.callback do |result|
            res = result
            EventMachine.stop
          end

          req.errback do |error|
            res = error
            EventMachine.stop
          end
        end
      }
      res
    end

    def delete(id)
      res = nil
      EventMachine.run {
        client = EventMachine::connect @ip, @port, SpotiHifi::Client, @ip, @port

        client.invoke("db/delete", [ id ]) do |req|
          req.timeout 2

          req.callback do |result|
            res = result
            EventMachine.stop
          end

          req.errback do |error|
            res = error
            EventMachine.stop
          end
        end
      }
      res
    end

  end

  def spotihifi_call(ip, method, params=nil)
    EventMachine.run {
      client = EventMachine::connect ip, 8212, SpotiHifi::Client, ip, 8212

      client.invoke(method, params) do |req|
        req.timeout 2

        req.callback do |result|
          p result
          EventMachine.stop
        end

        req.errback do |error|
          p error
          EventMachine.stop
        end
      end
    }
  end

  desc "Get index"
  task :index, [ :ip ] do |t, args|
    ip = args[:ip] || fail("ip address required")
    spotihifi_call(ip, "db/index")
  end

=begin
  desc "Delete album"
  task :delete_album, [ :ip, :id ] do |t, args|
    ip = args[:ip] || fail("ip address required")
    id = args[:id] || fail("id is required")
    spotihifi_call(ip, "db/delete", { "album" => { "id" => id.to_i } })
  end
=end

  desc "Delete track"
  task :delete_track, [ :ip, :id ] do |t, args|
    ip = args[:ip] || fail("ip address required")
    id = args[:id] || fail("id is required")
    spotihifi_call(ip, "db/delete", [ id ])
  end

=begin
  desc "Get tags"
  task :tags, [ :ip ] do |t, args|
    ip = args[:ip] || fail("ip address required")
    spotihifi_call(ip, "db/tags")
  end
=end

=begin
  desc "Add tag to track"
  task :add_tag, [ :ip, :id, :tag ] do |t, args|
    ip  = args[:ip]  || fail("ip address required")
    id  = args[:id]  || fail("track id is required")
    tag = args[:tag] || fail("tag required")
    spotihifi_call(ip, "db/save", { "id" => id.to_i, "tags/add" => [ tag ] })
  end
=end

  desc "Get artists"
  task :artists, [ :ip ] do |t, args|
    ip = args[:ip] || fail("ip address required")

    artists = MusicBox.new(ip).artists

    artists.each do |artist|
      p artist
    end
  end

  desc "Get albums"
  task :albums, [ :ip ] do |t, args|
    ip = args[:ip] || fail("ip address required")

    albums = MusicBox.new(ip).albums

    albums.each do |album|
      p album
    end
  end

  desc "Get albums"
  task :album_tracks, [ :ip, :id ] do |t, args|
    ip = args[:ip] || fail("ip address required")
    id = args[:id] || fail("album id required")

    tracks = MusicBox.new(ip).album_tracks(id)

    tracks.each do |track|
      p track
    end
  end


  desc "Get tracks"
  task :tracks, [ :ip ] do |t, args|
    ip = args[:ip] || fail("ip address required")

    tracks = MusicBox.new(ip).tracks

    tracks.each do |track|
      p track
    end
  end

end