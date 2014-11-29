#= require jquery-1.11.0.js
#= require underscore
#= require backbone.js
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
        tags = tags.filter (t) -> t.length > 0
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
    #tagName: 'div'

    events:
        'click .cover': "show"

    render: ->
        @template = @template or _.template($("#album-view").html())
        @model.set_cover_path()
        @$el.html @template(@model.attributes)
        @

    show: ->
        #$('body').toggleClass('hide-overflow')
        view = new AlbumDetailsView model: @model



class AlbumsView extends Backbone.View
    #el: '#albums-container'

    initialize: (collection) ->
        @collection = collection
        console.log(@collection)
        @render()

    append_album: (view) ->
        @$("#albums-view").append(view.el)

    render: ->
        @template = @template or _.template($("#albums-view").html())
        @$el.html @template()
        # Render albums.
        for artist in @collection.models
            for album in artist.get("albums").models
                album.set("artist", artist.get("name"))
                @render_album(album)
        @

    render_album: (album) ->
        view = new AlbumView model: album
        @append_album(view.render())



###############################################################################
# Now Playing
#

class StateModel extends Backbone.Model
    defaults:
        id:     null
        title:  ""
        artist: { id: null, name: "" }
        album:  { id: null, title: "" }
        action: 'play'

    initialize: ->
        @load_player_state()
        @subscribe()

    load_player_state: ->
        req =
            type: "GET"
            url:  "/api/state"
            data: []
        $.ajax(req).done (msg) =>
            console.log(msg)
            @set({ state: msg.state, source: msg.source, action: @action(msg.state) })
            @update_track(msg.track) if msg.track

    subscribe: ->
        es = new EventSource("/api/events");
        es.onmessage = (event) =>
            msg = JSON.parse(event.data)
            if msg.method == "player/event"
                if msg.params.state == "playing"
                    @update_track(msg.params.track)
                else
                    @clear_track()
                @set({ state: msg.params.state, source: msg.params.source, action: @action(msg.params.state) })
        es.onerror = (error) =>
            console.log(error)

    clear_track: ->
        @id     = ""
        @title  = ""
        @artist = { id: null, name: "" }
        @album  = { id: null, title: "" }

    update_track: (track) ->
        @set({ id: track.track_id, title: track.title, artist: track.artist, album: track.album })

    action: (state) ->
        switch state
            when "playing" then "stop"
            when "stopped" then "play"
            else "stop"




class NowPlayingView extends Backbone.View

    initialize: (model) ->
        @model = model
        @model.on 'change', => @render()
        @render()

    events:
        'click #center-action': "do_action"

    render: ->
        @template = @template or _.template($("#now-playing-view").html())
        @$el.html @template(@model.attributes)
        @

    do_action: ->
        req =
            type: "POST"
            url:  "/api/#{@model.get('action')}"
            data: null
        $.ajax(req)




class MainView extends Backbone.View
    el: '#view-container'

    initialize: (model) ->
        @state = new StateModel()
        @db = new Database
        @db.fetch()

        $('#show-playing').on    'click', => @show_playing();
        $('#show-albums-all').on 'click', => @show_albums();

        @show_playing()
        #@show_albums()
        @render()

    render: ->
        @$el.append @view.el

    show_playing: ->
        @view.remove() if @view
        @view = new NowPlayingView(@state)
        @render()

    show_albums: ->
        @view.remove() if @view
        @view = new AlbumsView(@db.get('artists').sort())
        @render()



$ ->
    main_view = new MainView()
