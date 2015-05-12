require 'open-uri'

require_relative 'music_box'

module Spotify

  class SavedTracksImporter

    def initialize(user)
      @user = user
    end

    def run(queue, ignore_uris)
      puts "SAVED TRACKS IMPORTER RUN!!!!!"
      offset = 0
      begin
        tracks = @user.saved_tracks(limit: 50, offset: offset)
        tracks.each do |track|
          unless ignore_uris.include?(track.uri)
            queue << make_mbox_track(track)
          end
        end
        offset += tracks.size
      end while tracks.size == 50
      puts "SAVED TRACKS IMPORTER END!!!!!"
      queue << nil
    end

    def make_mbox_track(track)
      {
        :title    => track.name,
        :artist   => {
          :name       => track.artists[0].name,
          :spotify_id => track.artists[0].id },
        :album    => {
          :title      => track.album.name,
          :spotify_id => track.album.id,
          :artist     => {
            :name => track.album.artists[0].name,
            :spotify_id => track.album.artists[0].id
          }
        },
        :tn       => track.track_number,
        :dn       => track.disc_number,
        :duration => track.duration_ms/1000,
        :source   => {
          :name => "spotify",
          :uri => track.uri
        },
        :cover    => load_cover(track.album.images[0]['url'])
      }
    end

    def load_cover(uri)
      cover = Hash.new
      open(uri) { |f|
        cover["image_data"]   = Base64.strict_encode64(f.read)
        cover["image_format"] = "jpg"
      }
      cover
    end

  end

  def self.saved_tracks_importer(user)
    @saved_tracks_importer ||= SavedTracksImporter.new(user)
  end

end