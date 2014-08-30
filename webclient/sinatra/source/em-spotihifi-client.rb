require 'rubygems'
require 'json'
require 'eventmachine'

module SpotiHifi

  class Request

    include EventMachine::Deferrable

    attr_accessor :method
    attr_accessor :params

    def initialize(method, params)
      @method = method
      @params = params
    end

    def encode
      buf = { :jsonrpc => "2.0", :method => method, :params => params, :id => self.object_id.to_s }.to_json
      buf = buf.unpack("C*")
      buf = buf.pack("C*") + "\0"
      buf
    end

    def set_response(res)
      if res.has_key?("result")
        succeed(res["result"])
      else
        fail(res["error"])
      end
    end

  end

  class Client < EventMachine::Connection

    def initialize(ip, port)
      puts "init client #{ip}:#{port}"
      @ip = ip
      @port = port
      @events = EventMachine::Channel.new
    end

    def invoke(method, params, timeout=5, &blk)
      req = Request.new(method, params)
      if block_given?
        req.timeout(timeout)
        yield req
        send_data(req.encode)
      end
      req
    end

    def subscribe(*args, &blk)
      @events.subscribe(*args, &blk)
    end

    def unsubscribe(*args, &blk)
      @events.unsubscribe(*args, &blk)
    end

    def post_init
      @data = String.new
    end

    def receive_data(data)
      @data << data

      while (msg = @data.slice!(/[^\0]*\0/))
        # Remove zero terminator.
        msg.chop!

        resp = JSON::parse(msg)

        if resp.has_key?("jsonrpc") and resp.has_key?("id")
          if (req = find_request(resp["id"]))
            req.set_response(resp)
          else
            fail "received response to an unknown request"
          end
        else
          @events.push(resp)
        end
      end
    end

    def find_request(id)
      ObjectSpace._id2ref(id.to_i)
    end

    def connection_completed
    end

    def unbind
      EventMachine.next_tick {
        EventMachine.add_timer(1) { reconnect @ip, @port }
      }
    end

  end

end
