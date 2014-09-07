require 'open-uri'
require 'json'
require 'ostruct'

require_relative './em-spotihifi-client'

module Spotify

  class Track < OpenStruct

    def artists
      super.collect { |a| OpenStruct.new(a) }
    end

    def track_number
      send(:"track-number").to_i
    end

    def disc_number
      send(:"disc-number").to_i
    end

  end

  class Album < OpenStruct

    def year
      send(:"released").to_i
    end

    def upc
      external_ids = send(:"external-ids")
      if external_ids
        upc = external_ids.select { |id| id["type"] == "upc" }.first
        upc["id"]
      else
        upc = nil
      end
    end

    def tracks
      super.collect { |t| Track.new(t) }
    end

  end

  class AlbumImporter

    def initialize(ip)
      @ip  = ip
    end

    def import(uri)
      lookup_album(uri) do |album|
        album.tracks.each do |t|
          track = {
            :title    => t.name,
            :artist   => album.artist,
            :album    => { :title => album.name, :ids => { :upc => album.upc, :spotify => uri } },
            :tn       => t.track_number,
            :dn       => t.disc_number,
            :duration => t.length,
            :sources  => [ { :name => "spotify", :uri => t.href } ]
          }
          save_track(track)
        end
      end
    end

    def lookup_album(uri, &blk)
      open("http://ws.spotify.com/lookup/1/.json?uri=#{uri}&extras=trackdetail") { |f|
        json = JSON.parse(f.read)
        yield Album.new(json['album'])
      }
    end

    def save_track(track)
      EventMachine.run {
        client = EventMachine::connect @ip, 1100, SpotiHifi::Client, @ip, 1100

        client.invoke("db/save", track) do |req|
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

  end

end