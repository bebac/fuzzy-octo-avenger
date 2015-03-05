require 'vienna/template_view'

class TrackView < Vienna::TemplateView
  template :track

  attr_reader :track
  attr_reader :qid

  def initialize(track)
    @track = track
    @qid   = nil
  end

  def tag_name
    :li
  end

  on :click do |evt|
    HTTP.post("/tracks/#{@track.id}/queue") do |response|
      if response.ok?
        @qid = response.body
      else
        puts "queue error!"
      end
      render
      element.find(".action-status").effect(:fade_out, 4000) { qid = nil }
    end
  end

  on :click, '.track-show-details' do |evt|
    element.find(".track-details").toggle
    evt.stop_propagation
  end

  on :click, '.track-details' do |evt|
    evt.stop_propagation
  end

  on :mouseleave do |evt|
    element.find(".track-details").hide
  end

  on :change, '.tag-edit' do |evt|
    tags = evt.target.value.split /,\s*/
    tags.reject! { |t| t.empty? }
    HTTP.post("/tracks/#{@track.id}/tags", payload: tags) do |response|
      if response.ok?
        @track.tags = tags
      else
        puts "set tags error! #{response.json}"
      end
      render
    end
  end

end