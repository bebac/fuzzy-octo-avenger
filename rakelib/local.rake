require_relative '../webclient/sinatra/app/srv/music_box'

namespace :local do

  desc "Scan local source directories"
  task :scan, [ :ip ] do |t, args|
    ip = args[:ip] || fail("ip address required")

    EventMachine.run {
      client = EventMachine::connect ip, 8212, MusicBox::Connection, ip, 8212

      client.invoke("sources/local/scan", []) do |req|
        req.timeout 240

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