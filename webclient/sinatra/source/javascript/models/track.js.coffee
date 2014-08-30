class @Track extends Backbone.Model
    defaults:
        title: "test"
        duration: "0:00"

    number: ->
        tn = @get('tn')
        if tn < 10
          "0#{tn}"
        else
          tn



class @Tracks extends Backbone.Collection
    model: Track

    comparator: (track) -> (track.get('dn')<<8)+track.get('tn')
