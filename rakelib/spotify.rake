require_relative '../webclient/sinatra/source/spotify-album-importer'

namespace :spotify do

  desc "Import spotify album"
  task :import, [ :ip, :uri ] do |t, args|
    ip  = args[:ip]  || fail("ip address required")
    uri = args[:uri] || fail("spotify uri required")
    Spotify::AlbumImporter.new(ip).import(uri)
  end

end