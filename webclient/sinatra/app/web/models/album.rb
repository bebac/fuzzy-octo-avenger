class Album < Vienna::Model

  attributes :title
  attributes :id
  attributes :tracks
  attributes :ext_cover_url
  attributes :artist

  def initialize
  end

  def artist_name
    artist ? artist["name"] : "<unknown>"
  end

  def cover_url
    if ext_cover_url
      'https://i.scdn.co/image/d3c9914e2dbd9bdb7c6dd84714a853440417dbad'
    else
      "/albums/#{id}/cover"
    end
  end

end