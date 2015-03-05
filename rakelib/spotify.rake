require 'json'

require_relative '../webclient/sinatra/app/srv/spotify-album-importer'

namespace :spotify do

  desc "Import spotify album"
  task :import, [ :ip, :uri, *:a..:z ] do |t, args|
    ip     = args[:ip]  || fail("ip address required")
    uri    = args[:uri] || fail("spotify uri required")
    tracks = args.values_at(*(:a..:z)).collect { |i| i.to_i if i }.compact!
    tracks = tracks.length > 0 ? tracks : nil
    id = uri.sub(/spotify:album:/, '')
    Spotify::AlbumImporter.new(ip).import(id, tracks)
  end

  desc "Import spotify albums from json list"
  task :import_albums, [ :ip, :filename ] do |t, args|
    ip       = args[:ip]       || fail("ip address required")
    filename = args[:filename] || fail("spotify uri filename required")

    uris = File.open(filename) { |file| JSON.parse(file.read).reject{ |uri| uri.nil? } }

    uris.each do |uri|
      puts "import #{uri}"
      id = uri.sub(/spotify:album:/, '')
      Spotify::AlbumImporter.new(ip).import(id)
      sleep(2)
    end
  end

  desc "Get album cover"
  task :get_cover, [ :uri ] do |t, args|
    uri = args[:uri] || fail("spotify uri required")
    id  = uri.sub(/spotify:album:/, '')
    open("https://api.spotify.com/v1/albums/#{id}") { |f|
      json  = JSON.parse(f.read)
      cover_url = json["images"][0]["url"]
      p cover_url
      open(cover_url) { |f|
        File.open("cover.jpg", "w+") { |o| o.write(f.read) }
      }
    }
  end

end