require 'opal'
require 'jquery'
require 'opal-jquery'
require 'vienna'

require 'views/application_view'
require 'views/albums_view'
require 'views/album_thumb_view'
require 'views/album_details_view'
require 'views/track_view'
require 'views/settings_view'
require 'views/player_view'

require 'templates/albums'
require 'templates/album_thumb'
require 'templates/album_details'
require 'templates/track'
require 'templates/settings'
require 'templates/player'

require 'models/album'
require 'models/track'
require 'models/source_local'

class Application

  def run
    @view = ApplicationView.new
    router.update
  end

  def router
    @router ||= Vienna::Router.new.tap do |router|
      router.route('/albums')   { |params| @view.show_albums }
      router.route('/settings') { |params| @view.show_settings }
    end
  end

end

Document.ready? do
  Application.new.run
end

