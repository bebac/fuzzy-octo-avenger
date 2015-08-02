require 'vienna/template_view'

class EventSource
  def self.supported?
    Browser.supports? :EventSource
  end

  include Native
  #include Event::Target

  # HACK! Only do onmessage.
  def on(evt, &block)
    @callback = block
  end

  def dispatch(evt)
     @callback.call(`evt.data`) if @callback
  end

  #target {|value|
  #  EventSource.new(value) if Native.is_a?(value, `window.EventSource`)
  #}

  # Create an {EventSource} on the given path.
  #
  # @param path [String] the path to use as source
  # @yield if the block has no parameters it's instance_exec'd, otherwise it's
  #        called with self
  def initialize(path, &block)
    if native?(path)
      super(path)
    else
      super(`new window.EventSource(path)`)
    end

    %x{
      #@native.onmessage = function(evt) { self.$dispatch(evt) }
    }

    if block.arity == 0
      instance_exec(&block)
    else
      block.call(self)
    end if block
  end

  # @!attribute [r] url
  # @return [String] the URL of the event source
  alias_native :url

  # @!attribute [r] state
  # @return [:connecting, :open, :closed] the state of the event source
  def state
    %x{
      switch (#@native.readyState) {
        case window.EventSource.CONNECTING:
          return "connecting";

        case window.EventSource.OPEN:
          return "open";

        case window.EventSource.CLOSED:
          return "closed";
      }
    }
  end

  # Check if the event source is alive.
  def alive?
    state == :open
  end

  # Close the event source.
  def close
    `#@native.close()`
  end
end

class PlayerView < Vienna::TemplateView
  template :player

  element '#nav-player'

  attr_reader :playing

  def initialize
    load_state
    @events = EventSource.new "/events" do |es|
      es.on :message do |data|
        msg = JSON.parse(data)
        if msg["method"] == "player/event"
          params = msg["params"]
          @playing = (params["state"] == "playing" ? true : false)
          render
        end
      end
    end
  end

  def load_state
    HTTP.get("/player/state") do |response|
      if response.ok?
        params = response.json
        @playing = (params["state"] == "playing" ? true : false)
      else
        puts "error loading player state"
      end
      render
    end
  end

  on :click, ".play" do |evt|
    HTTP.post("/player/play") do |response|
      if response.ok?
        puts "play ok"
      else
        puts "play error!"
      end
    end
  end

  on :click, ".stop" do |evt|
    HTTP.post("/player/stop") do |response|
      if response.ok?
        puts "stop ok"
      else
        puts "stop error!"
      end
    end
  end
end