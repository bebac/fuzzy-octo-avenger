#= require 'models/track'
#= require 'models/album'
#= require 'models/artist'
#= require 'models/database'

class AlbumTrackView extends Backbone.View
    tagName: 'li'

    #<span class="tag"><%= tag %></span>

    #HTML: '<%= title %><span class="duration"><%= duration %></span>'
    HTML: """
    <div class="track">
      <div style="display:inline-block;vertical-align:top">
        <%= track.number() %>
      </div>
      <div style="display:inline-block">
        <div>
          <%= track.get("title") %>&nbsp;<span class="track-show-details">+</span>
        </div>
        <div class="track-details" style="display:none">
          <div>
            <input class="tag-edit" type="text" value="<%= track.get("tags") %>">
          </div>
          <!--<div style="font-size:0.66em">
            #<%= track.get("id") %>
          </div>-->
        </div>
      </div>
      <div class="action-status" style="display:inline-block;vertical-align:top">
        <%= status %>
      </div>
      <div class="duration" style="display:inline-block;vertical-align:top">
        <%= track.duration() %>
      </div>
      <div class="tags" style="display:inline-block;vertical-align:top">
        <% _.each(track.get("tags"), function(tag) { %>
          <span class="tag"><%= tag %></span>
        <% }); %>
      </div>
    </div>
    """

    template: _.template(AlbumTrackView::HTML)

    render: ->
        @$el.html @template({ track: @model, status: @status })
        @

    events:
        'click': "play"
        'click .track-show-details' : "show_track_details"
        'click .track-details' : "track_details"
        'mouseleave' : "hide_track_details"
        'change .tag-edit': "tags_save"

    play: ->
        req =
            type: "POST"
            url:  "/api/queue"
            data: { id: @model.get('id') }
        $.ajax(req).done (msg) => @show_action_status(msg)

    show_track_details: (event) ->
        @$('.track-details').toggle()
        event.stopPropagation()

    hide_track_details: (event) ->
        @$('.track-details').hide()

    track_details: (event) ->
        event.stopPropagation()

    show_action_status: (msg) ->
        @status = msg
        @render()
        @$('.action-status').fadeOut(5000)

    tags_save: (event) ->
        tags = @$('.tag-edit').val().split(/,\s*/)
        req =
            type: "POST"
            url:  "/api/tags"
            data: { id: @model.get('id'), tags: tags }
        $.ajax(req).done (msg) =>
            @model.set("tags", tags)
            @render()



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



class AlbumDetailsView extends Backbone.View
    el: '#album-details'

    initialize: ->
        @render()
        $("#album-details-container").show()
        $("#album-details-container").click (event) =>
            if event.target.id == "album-details-container"
                #$('body').toggleClass('hide-overflow')
                $("#album-details-container").hide()


    render: ->
        @template = @template or _.template($("#album-details-view").html())
        @model.set_cover_path()
        @$el.html @template(@model.attributes)
        @tracks_view = new AlbumTrackListView el: @$('.track-list'), collection: @model.get('tracks')
        @tracks_view.render()
        @

    nodefault: (event) ->
        event.preventDefault()



class AlbumView extends Backbone.View
    tagName: 'div'

    events:
        'click': "show"

    render: ->
        @template = @template or _.template($("#album-view").html())
        @model.set_cover_path()
        @$el.html @template(@model.attributes)
        @

    show: ->
        #$('body').toggleClass('hide-overflow')
        view = new AlbumDetailsView model: @model



class AlbumsView extends Backbone.View
    el: '#albums-view'

    initialize: (collection) ->
        @collection = collection
        console.log(@collection)

    append_album: (view) ->
        @$el.append(view.el)

    render: ->
        for artist in @collection.models
            for album in artist.get("albums").models
                album.set("artist", artist.get("name"))
                @render_album(album)
        @

    render_album: (album) ->
        view = new AlbumView model: album
        @append_album(view.render())



class StateModel extends Backbone.Model



class NowPlayingView extends Backbone.View
    el: '#now-playing'

    HTML: """
    [ <%= state %>, <%= source %> ] : <b><%= title %></b> &bull; <%= artist %> &bull; <%= album %>
    """

    template: _.template(NowPlayingView::HTML)

    initialize: (model) ->
        @model = model
        @load_player_state()
        @subscribe()
        @render()

    render: ->
        #@$el.html @template({ state: @state, title: @title, artist: @artist, album: @album })
        @$el.html @template({ state: @model.get("state"), source: @model.get("source"), title: @title, artist: @artist, album: @album })
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
        albums_view = new AlbumsView(db.get('artists').sort())
        albums_view.render()
