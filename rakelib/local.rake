require_relative '../webclient/sinatra/source/em-spotihifi-client'

namespace :local do

  desc "Scan local source directories"
  task :scan, [ :ip ] do |t, args|
    ip = args[:ip] || fail("ip address required")

    EventMachine.run {
      client = EventMachine::connect ip, 8212, SpotiHifi::Client, ip, 8212

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