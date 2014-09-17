class @Track extends Backbone.Model
    defaults:
        title: "test"
        duration: ".:.."

    number: ->
        tn = @get('tn')
        if tn < 10
          "0#{tn}"
        else
          tn

    duration: ->
        duration = @get("duration")
        if typeof duration == 'string'
            duration
        else
            min = parseInt( duration / 60 ) % 60;
            sec = duration % 60;
            if sec < 10 then "#{min}:0#{sec}" else "#{min}:#{sec}"




class @Tracks extends Backbone.Collection
    model: Track

    comparator: (track) -> (track.get('dn')<<8)+track.get('tn')
