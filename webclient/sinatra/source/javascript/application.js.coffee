class StateModel extends Backbone.Model
    defaults:
        artist: { id: null, name: "" }
        album: { id: null, title: "" }



class NowPlayingView extends Backbone.View
    el: '#now-playing'

    #HTML: """
    #[ <%= state %>, <%= source %> ] : <b><%= title %></b> &bull; <%= artist %> &bull; <%= album %>
    #"""

    #template: _.template(NowPlayingView::HTML)

    initialize: (model) ->
        @model = model
        @clear_track()
        @load_player_state()
        @subscribe()
        @render()

    render: ->
        @template = @template or _.template($("#now-playing-view").html())
        track =
            id:     @id
            title:  @title
            artist: @artist
            album:  @album
        @$el.html @template({ track: track })
        @

    clear_track: ->
        @id     = ""
        @title  = ""
        @artist = { id: null, name: "" }
        @album  = { id: null, title: "" }

    update_track: (track) ->
        @id     = track.track_id
        @title  = track.title
        @artist = track.artist
        @album  = track.album

    load_player_state: ->
        req =
            type: "GET"
            url:  "/api/state"
            data: []
        $.ajax(req).done (msg) =>
            console.log(msg)
            @model.set("state", msg.state)
            @model.set("source", msg.source)
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
                @model.set("source", msg.params.source)
                @render()
        es.onerror = (error) =>
            console.log(error)



class ControlsView extends Backbone.View
    el: '#control-container'

    HTML: """
    <img src="assets/<%= action %>.png" id="center-action">
    """

    template: _.template(ControlsView::HTML)

    initialize: (model) ->
        model.on "change:state", (model, value) => @set_actions(value)
        @render()

    events:
        'click #center-action': "do_action"
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
