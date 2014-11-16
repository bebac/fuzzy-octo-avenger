require 'json'

require_relative '../webclient/sinatra/source/spotify-album-importer'

namespace :spotify do

  desc "Import spotify album"
  task :import, [ :ip, :uri ] do |t, args|
    ip  = args[:ip]  || fail("ip address required")
    uri = args[:uri] || fail("spotify uri required")
    id = uri.sub(/spotify:album:/, '')
    Spotify::AlbumImporter.new(ip).import(id)
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

end