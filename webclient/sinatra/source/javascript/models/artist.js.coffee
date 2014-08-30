class @Artist extends Backbone.Model
    defaults:
        name: ''
        albums: new Albums

    parse: (data) ->
        #console.log("parse artist: #{data}")
        if data.albums? then data.albums = new Albums(data.albums, { parse: true })
        data



class @Artists extends Backbone.Collection
    model: Artist

    comparator: (artist) -> artist.get('name')
