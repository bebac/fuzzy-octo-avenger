require 'vienna/template_view'

class SettingsView < Vienna::TemplateView
  template :settings

  def initialize
    load_source_local
    load_source_spotify
  end

  def render
    super
    local_source_directories.each do |dir|
      add_directory(dir)
    end
    if @spotify_user
      name = @spotify_user['display_name']
      element.find('#spotify') << "<button id=\"spotify-import\">Import saved tracks</button>Signed in as #{name}"
    else
      element.find('#spotify') << "<a href=\"/auth/spotify\">Sign in with Spotify</a>"
    end
  end

  def class_name
    "settings-container"
  end

  def local_source_directories
    if @source_local
      @source_local.directories
    else
      []
    end
  end

  def add_directory(dir=nil)
    element.find('#directories') << "<li><input type=\"text\" value=\"#{dir}\"></input></li>"
  end

  def load_source_local
    HTTP.get("/sources/local") do |response|
      if response.ok?
        attrs = response.json
        attrs["id"] = 1
        p attrs
        @source_local = SourceLocal.load(attrs)
      else
        puts "error loading albums"
      end
      render
    end
  end

  def load_source_spotify
    HTTP.get("/sources/spotify") do |response|
      if response.ok?
        @spotify_user = response.json
      else
        puts "error source spotify"
      end
      render
    end
  end

  def save_source_local
    HTTP.post("/sources/local", payload: { "directories" => local_source_directories }) do |response|
      if response.ok?
        puts "ok"
      else
        puts "error saving source_local"
      end
    end
  end

  on :keypress, 'input' do |evt|
    if evt.which == 13
      if evt.target.value.empty?
        evt.target.remove
      end
      @source_local.directories = element.find('#directories input').collect { |input| input.value }
      save_source_local
    end
  end

  on :click, '#scan' do |evt|
    evt.target.prop("disabled", true)
    HTTP.post("/sources/local/scan") do |response|
      if response.ok?
        puts "scan ok"
      else
        puts "scan error!"
      end
      render
    end
  end

  on :click, '#spotify-import' do |evt|
    evt.target.prop("disabled", true)
    HTTP.post("/sources/spotify/import", { :timeout => 5*60000 }) do |response|
      p response.json
      if response.ok?
        puts "spotify import ok"
      else
        puts "spotify import error!"
      end
      render
    end
  end

  on :click, '#add' do |evt|
    add_directory
  end

end
