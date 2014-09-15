require_relative '../webclient/sinatra/source/em-spotihifi-client'

namespace :db do

  desc "Delete album"
  task :delete_album, [ :ip, :id ] do |t, args|
    ip = args[:ip] || fail("ip address required")
    id = args[:id] || fail("id is required")

    EventMachine.run {
      client = EventMachine::connect ip, 1100, SpotiHifi::Client, ip, 1100

      client.invoke("db/delete", { "album" => { "id" => id.to_i } }) do |req|
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

end