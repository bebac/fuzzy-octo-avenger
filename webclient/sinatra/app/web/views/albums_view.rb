require 'vienna/template_view'

class AlbumsView < Vienna::TemplateView
  template :albums

  def initialize
    @albums = []
    load_albums
  end

  def render
    super
    render_albums_by_artist
  end

  def render_albums_by_artist
    @albums.sort! { |x,y| x.artist_name <=> y.artist_name }.each do |album|
      add_album_thumb(album)
    end
  end

  def add_album_thumb(album)
    view = AlbumThumbView.new(album)
    view.render
    element.find('#album-thumbs') << view.element
  end

  def load_albums
    HTTP.get("/albums") do |response|
      if response.ok?
        response.json.each do |attrs|
          @albums << Album.load(attrs)
        end
      else
        puts "error loading albums"
      end
      self.render
    end
  end

end
