require_relative '../webclient/sinatra/app/srv/music_box'

namespace :player do

  def spotihifi_call(ip, method, params=nil)
    EventMachine.run {
      client = EventMachine::connect ip, 8212, MusicBox::Connection, ip, 8212

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

=begin no longer works!
  desc """Continuous playback - Leave out tags to clear filter. To
  include multiple tags use a quoted string with tags separated by
  space. Example: rake player:ctpb[127.0.0.1,\"pop rock\"]
  """
  task :ctpb_enable, [ :ip, :tags ] do |t, args|
    ip   = args[:ip]   || fail("ip address required")
    tags = args[:tags]

    if tags
      tags = tags.split
    else
      tags = nil
    end

    params = {
      "enable" => true, "filter" => { "tags" => tags }
    }

    spotihifi_call(ip, "player/ctpb", params)
  end

  desc "Continuous playback - Disable"
  task :ctpb_disable, [ :ip ] do |t, args|
    ip  = args[:ip] || fail("ip address required")

    params = {
      "enable" => false, "filter" => { "tags" => nil }
    }

    spotihifi_call(ip, "player/ctpb", params)
  end
=end
end