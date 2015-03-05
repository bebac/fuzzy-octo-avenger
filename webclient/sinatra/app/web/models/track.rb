class Track < Vienna::Model

  attributes :title
  attributes :id
  attributes :tn
  attributes :dn
  attributes :duration
  attributes :album
  attributes :sources
  attributes :artist
  attributes :tags

  def initialize
  end

  ###
  # Combine disc number and track number into one number used
  # to sort tracks.

  def index
    (dn << 8) + tn
  end

  ###
  # Format artist.

  def artist_name
    artist ? artist["name"] : "<unknown>"
  end

  ###
  # Format duration.

  def duration_formatted
    min = Integer(duration / 60) % 60;
    sec = Integer(duration % 60);
    "%d:%02d" % [ min, sec ]
  end

end