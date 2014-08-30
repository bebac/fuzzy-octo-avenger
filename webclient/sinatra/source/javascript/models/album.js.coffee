class @Album extends Backbone.Model
    defaults:
        title: "test"
        tracks: new Tracks

    parse: (data) ->
        #console.log("parse album: #{data}")
        if data.tracks? then data.tracks = new Tracks(data.tracks, { parse: true })
        data

    set_cover_path: ->
      #@set("cover_path", "api/cover?track_id="+@get("tracks").at(0).get("id"))
      @set("cover_path", "api/cover?album_id="+@get("id"))

class @Albums extends Backbone.Collection
    model: Album
