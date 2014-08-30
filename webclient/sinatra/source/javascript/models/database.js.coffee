class @Database extends Backbone.Model
    url: 'api/index'
    defaults:
        artists: new Artists

    parse: (data) ->
        if data.artists? then data.artists = new Artists(data.artists, { parse: true })
        console.log(data)
        data
