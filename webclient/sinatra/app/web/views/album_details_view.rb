require 'vienna/template_view'

class AlbumDetailsView < Vienna::TemplateView
  template :album_details

  element '#album-details-container'

  attr_reader :album

  def initialize(album)
    @album = album
    @tracks = []
    load_tracks
    show
  end

  def render
    render_template
    render_tracks
  end

  def render_template
    if template = Template[self.class.template]
      Element.find('#album-details').html = _render_template(template)
    else
      fail("failed to locate template '#{self.class.template}'")
    end
  end

  def render_tracks
    dn = 1
    tracks.each do |track|
      if track.dn > dn
        dn = track.dn
        Element.find('.track-list') << "<li class=\"disc-header\">Disc #{dn}</li>"
      end
      view = TrackView.new(track)
      view.render
      Element.find('.track-list') << view.element
    end
  end

  def tracks
    @tracks.sort { |x,y| x.index <=> y.index }
  end

  def show
    element.show
  end

  def load_tracks
    HTTP.get("/albums/#{@album.id}/tracks") do |response|
      if response.ok?
        response.json.each do |attrs|
          @tracks << Track.load(attrs)
        end
      else
        puts "error loading tracks"
      end
      self.render
    end
  end

  on :click do |evt|
    element.hide if evt.target.id == "album-details-container"
  end

end