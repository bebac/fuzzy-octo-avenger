#= require 'models/track'
#= require 'models/album'
#= require 'models/artist'
#= require 'models/database'

class AlbumTrackView extends Backbone.View
    tagName: 'li'

    #HTML: '<%= title %><span class="duration"><%= duration %></span>'
    HTML: '<%= track.number() %>&nbsp;&nbsp;&nbsp;<%= track.get("title") %> <span class="action-status"><%= status %></span>'

    template: _.template(AlbumTrackView::HTML)

    render: ->
        @$el.html @template({ track: @model, status: @status })
        @

    events:
        'click': "play"

    play: ->
        req =
            type: "POST"
            url:  "/api/queue"
            data: { id: @model.get('id') }
        $.ajax(req).done (msg) => @show_action_status(msg)

    show_action_status: (msg) ->
        @status = msg
        @render()
        @$('.action-status').fadeOut(5000)



class AlbumTrackListView extends Backbone.View

    append_track: (view) ->
        @$el.append(view.el)

    append_disc_header: (disc_number) ->
        @$el.append("<li class=\"disc-header\">Disc #{disc_number}</li>")

    render: ->
        dn = 1
        for track in @collection.models
            track_dn = track.get("dn")
            if track_dn > dn
                dn = track_dn
                @append_disc_header(dn)
            @render_track(track)
        #@render_track(track) for track in @collection.models
        @$('li:odd').addClass('odd')
        @

    render_track: (track) ->
        view = new AlbumTrackView model: track
        @append_track(view.render())



class AlbumView extends Backbone.View
    tagName: 'li'

    render: ->
        @template = @template or _.template($("#album_details").html())
        @model.set_cover_path()
        @$el.html @template(@model.attributes)
        tracks_view = new AlbumTrackListView el: @$('.album-tracks > ol'), collection: @model.get('tracks')
        tracks_view.render()
        @



class ArtistAlbumsView extends Backbone.View

    append_album: (view) ->
        @$el.append(view.el)

    render: ->
        @render_album(album) for album in @collection.models
        @

    render_album: (album) ->
        view = new AlbumView model: album
        @append_album(view.render())



class ArtistView extends Backbone.View
    tagName: 'article'

    render: ->
        @template = @template or _.template($("#artists_tpl").html())
        @$el.html @template(@model.attributes)
        albums_view = new ArtistAlbumsView el: @$('ul'), collection: @model.get('albums')
        albums_view.render()
        @



class ArtistsView extends Backbone.View
    el: '#artists'

    initialize: (collection) ->
        @collection = collection
        #@collection.on("reset", @render, @);
        #@collection.on("changed", @render, @);
        console.log(@collection)

    append_artist: (view) ->
        @$el.append(view.el)

    render: ->
        @render_artist(artist) for artist in @collection.models
        @

    render_artist: (artist) ->
        view = new ArtistView model: artist
        @append_artist(view.render())

class StateModel extends Backbone.Model



class NowPlayingView extends Backbone.View
    el: '#now-playing'

    HTML: """
    [ <%= state %> ] : <b><%= title %></b> &bull; <%= artist %> &bull; <%= album %>
    """

    template: _.template(NowPlayingView::HTML)

    initialize: (model) ->
        @model = model
        @load_player_state()
        @subscribe()
        @render()

    render: ->
        #@$el.html @template({ state: @state, title: @title, artist: @artist, album: @album })
        @$el.html @template({ state: @model.get("state"), title: @title, artist: @artist, album: @album })
        @

    clear_track: ->
        @title  = ""
        @artist = ""
        @album  = ""

    update_track: (track) ->
        @title  = track.title
        @artist = track.artist
        @album  = track.album

    load_player_state: ->
        req =
            type: "GET"
            url:  "/api/state"
            data: []
        $.ajax(req).done (msg) =>
            @model.set("state", msg.state)
            @update_track(msg.track) if msg.track
            @render()

    subscribe: ->
        es = new EventSource("/api/events");
        es.onmessage = (event) =>
            msg = JSON.parse(event.data)
            if msg.method == "player/event"
                if msg.params.state == "playing"
                    @update_track(msg.params.track)
                else
                    @clear_track()
                @model.set("state", msg.params.state)
                @render()
        es.onerror = (error) =>
            console.log(error)



class ControlsView extends Backbone.View
    el: '#controls'

    HTML: """
    <div id="play_or_stop_action"><%= action %></div> | <div id="skip_action">skip</div>
    """

    template: _.template(ControlsView::HTML)

    initialize: (model) ->
        model.on "change:state", (model, value) => @set_actions(value)
        @render()

    events:
        'click #play_or_stop_action': "do_action"
        'click #skip_action': "do_skip"

    render: ->
        @$el.html @template({ action: @action })
        @

    set_actions: (player_state) ->
        @action = switch player_state
            when "playing" then "stop"
            when "stopped" then "play"
            else "hmm"
        @render()

    do_action: ->
        req =
            type: "POST"
            url:  "/api/#{@action}"
            data: null
        $.ajax(req)

    do_skip: ->
        req =
            type: "POST"
            url:  "/api/skip"
            data: null
        $.ajax(req)


$ ->
    state = new StateModel()
    now_playing_view = new NowPlayingView(state)
    controls = new ControlsView(state)

    db = new Database

    db.fetch success: ->
        artists_view = new ArtistsView(db.get('artists').sort())
        artists_view.render()
