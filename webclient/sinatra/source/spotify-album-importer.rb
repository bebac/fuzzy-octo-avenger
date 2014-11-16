require 'open-uri'
require 'json'
require 'ostruct'
require 'base64'

require_relative './em-spotihifi-client'

module Spotify

  class Track < OpenStruct
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

  end

  class Artist < OpenStruct
  end

  class AlbumImporter

    def initialize(ip)
      @ip  = ip
    end

    def import(uri)
      lookup_album(uri) do |album|
      end
    end

    def load_cover(uri)
      cover = Hash.new
      open(uri) { |f|
        cover["image_data"]   = Base64.encode64(f.read)
        cover["image_format"] = "jpg"
      }
      return cover
    end

    def lookup_album(uri, &blk)
      tracks = []
      open("https://api.spotify.com/v1/albums/#{uri}") { |f|
        json  = JSON.parse(f.read)

        album  = Album.new(json)
        artist = Artist.new(album.artists[0])
        cover  = load_cover(album.images[0]["url"])

        album.tracks["items"].each do |item|
          track = Track.new(item)

          track_json = {
            :title    => track.name,
            :artist   => { :name => artist.name },
            :album    => { :title => album.name },
            :tn       => track.track_number,
            :dn       => track.disc_number,
            :duration => track.duration_ms/1000,
            :source   => { :name => "spotify", :uri => track.uri },
            :cover    => cover
          }

          tracks << track_json
        end
      }
      fail("tracks is empty") if tracks.empty?
      save_track(tracks)
    end

    def save_track(track)
      EventMachine.run {
        client = EventMachine::connect @ip, 8212, SpotiHifi::Client, @ip, 8212

        client.invoke("db/import-tracks", track) do |req|
          req.timeout 60

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