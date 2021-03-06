require 'open-uri'
require 'json'
require 'ostruct'
require 'base64'

require_relative './music_box'

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

    def import(uri, track_numbers=nil)
      tracks = lookup_album(uri)
      if track_numbers
        if track_numbers.length == 1
          save_track([tracks[*track_numbers]])
        else
          save_track(tracks[*track_numbers])
        end
      else
        save_track(tracks)
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

    def make_mbox_track(track, album, cover)
      album_artist = Artist.new(album.artists[0])
      track_artist = Artist.new(track.artists[0])
      {
        :title    => track.name,
        :artist   => {
          :name       => track_artist.name,
          :spotify_id => track_artist.id },
        :album    => {
          :title      => album.name,
          :spotify_id => album.id,
          :artist     => {
            :name => album_artist.name,
            :spotify_id => album_artist.id
          }
        },
        :tn       => track.track_number,
        :dn       => track.disc_number,
        :duration => track.duration_ms/1000,
        :source   => {
          :name => "spotify",
          :uri => track.uri
        },
        :cover    => cover
      }
    end

    def lookup_album(uri)
      tracks = []
      open("https://api.spotify.com/v1/albums/#{uri}") { |f|
        json  = JSON.parse(f.read)

        album  = Album.new(json)
        cover  = load_cover(album.images[0]["url"])

        album.tracks["items"].each do |item|
          tracks << make_mbox_track(Track.new(item), album, cover)
        end

        if album.tracks["next"]
          open(album.tracks["next"]) { |file_next|
            json = JSON.parse(file_next.read)
            json["items"].each do |item|
              tracks << make_mbox_track(Track.new(item), album, cover)
            end
          }
        end
      }
      fail("tracks is empty") if tracks.empty?
      tracks
    end

    def save_track(tracks)
      EventMachine.run {
        client = EventMachine::connect @ip, 8212, MusicBox::Connection, @ip, 8212

        client.invoke("db/import-tracks", tracks) do |req|
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