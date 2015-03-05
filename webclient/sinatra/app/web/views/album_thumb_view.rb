require 'vienna/template_view'

class AlbumThumbView < Vienna::TemplateView
  template :album_thumb

  attr_reader :album

  def initialize(album)
    @album = album
  end

  def class_name
    "album-thumb"
  end

  on :click do |evt|
    @details = AlbumDetailsView.new(@album)
    @details.render
  end

end