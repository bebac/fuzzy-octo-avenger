require 'vienna/template_view'

class PlayerView < Vienna::TemplateView
  template :player

  element '#nav-player'

  def initialize
  end

  on :click, ".action" do |evt|
    puts "clicked player action"
    if defined?(`window.EventSource`)
      puts "Browser supports EventSource"
    end
  end
end