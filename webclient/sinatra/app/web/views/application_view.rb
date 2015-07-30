class ApplicationView < Vienna::View
  element '#application'

  def initialize
    render
    show_albums
  end

  def render
    super
  end

  def show_albums
    show('albums')
  end

  def show_settings
    show('settings')
  end

  def show(name)
    @subview = create_view(name)
    @subview.render
    element.find('#view-container').css( :top => "#{top}px" )
    element.find('#menu').css( :top => "#{top}px" )
    element.find('#view-container').html = @subview.element
    set_selected(name)
  end

  def set_selected(href)
    links = Element.find '#nav li a'
    links.remove_class 'selected'
    links.filter("a[href=\"#/#{href}\"]").add_class 'selected'
  end

  def create_view(name)
    case name
    when 'albums'
      AlbumsView.new
    when 'settings'
      SettingsView.new
    else
      fail("unknown view")
    end
  end

  def top
    Document.find('#nav-container').height
  end

  on :click, '#menu-toggle' do |evt|
    menu = element.find('#menu')
    cont = Document.find('#view-container')
    menu.toggle(150, :swing, -> {
      if menu.visible?
        cont.css( :"width" => "#{cont.width-menu.width}px", :"left" => "#{menu.width}px" )
      else
        cont.css( :"width" => "100%", :"left" => "0" )
      end
    })
    evt.stop_propagation
  end

end
