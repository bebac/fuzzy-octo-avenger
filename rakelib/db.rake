require_relative '../webclient/sinatra/source/em-spotihifi-client'

namespace :db do

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

end