# Data given to MM viz:
# {'participants': [<participantId, participantId, ...],
#  'transitions': <Number Of transitions in interval>,
#  'turns': [{'participant': <participantId>,
#             'turns': <Percent of turns in interval by this participant>}, ...]

  class @MM
    constructor: (data, localParticipant, width, height) ->

      console.log "constructing MM with data:", data
      @localParticipant = localParticipant
      @fontFamily = "Futura,Helvetica Neue,Helvetica,Arial,sans-serif"
      @margin = {top: 0, right: 0, bottom: 0, left: 0}
      @width = width - @margin.right - @margin.left
      @height = height - @margin.bottom - @margin.top

      # radius of network as a whole
      @radius = 115

      @data = data

      # determines positions for participant avatars
      # use [115, 435] to keep local participant node at the top
      # of the visual.
      @angle = d3.scale.ordinal()
        .domain @data.participants
        .rangePoints [0, 360], 1 # 90 to make it start at top? still goes to left...

      # determines thickness of edges
      @linkStrokeScale = d3.scale.linear()
        .domain [0, 1]
        .range [3, 15]

      # color scale for sphere in the middle
      @sphereColorScale = d3.scale.linear()
        .domain [0, data.participants.length * 3]
        .range ['#C8E6C9', '#2E7D32']
        .clamp true

      # create initial node data
      @nodes = ({'participant': p, 'name':@data.names[i]} for p, i in @data.participants)
      @nodes.push({'participant': 'energy'}) # keep the energy ball in the list of nodes

      @nodeTransitionTime = 500
      @linkTransitionTime = 500

      @createLinks()

    nodeRadius: (d) =>
      if (d.participant == "energy")
        30
      else
        20


    render: (id="#meeting-mediator") ->
      @chart = d3.select id
        .append "svg"
        .attr "class", "meeting-mediator"
        .attr "width", @width + @margin.left + @margin.right
        .attr "height", @height + @margin.top + @margin.bottom
        .append "g"
        .attr "transform", "translate(" + @width / 2 + "," + @height / 2 + ")"

      @chartBody = @chart.append "g"
        .attr "width", @width
        .attr "height", @height

      @graphG = @chart.append "g"
        .attr "width", @width
        .attr "height", @height

      @outline = @chartBody.append "g"
        .attr "id", "outline"
        .append "circle"
        .style "stroke", "#AFAFAF"
        .attr "stroke-width", 3
        .style "stroke-dasharray", ("10, 5")
        .attr "fill", "transparent"
        .attr "r", @radius + 20 + 2 # + @nodeRadius + 2.5

      # put links / nodes in a separate group
      @linksG = @graphG.append "g"
        .attr "id", "links"
      @nodesG = @graphG.append "g"
        .attr "id", "nodes"

      @renderNodes()
      @renderLinks()
      # keeps user node at top
      @graphG.transition().duration(250)
        .attr "transform", @constantRotation()

    # a little complicated, since we want to be able to put text
    # and prettier stuff on nodes in the future (maybe).
    # We create a group for each node, and do a selection for moving them around.
    renderNodes: () =>
      @node = @nodesG.selectAll ".node"
        .data @nodes, (d) -> d.participant

      @nodeG = @node.enter().append "g"
        .attr "class", "node"
        .attr "id", (d) -> d.participant

      @nodeG.append "circle"
        .attr "class", "nodeCircle"
        .attr "fill", @nodeColor
        .attr "r", @nodeRadius

      @nodeG.append "circle"
        .attr "class", "nodeFill"
        .attr "fill", "#FFFFFF"
        .attr "r", (d) =>
          if (d.participant == 'energy' or d.participant == @localParticipant)
            0
          else
            @nodeRadius(d) - 3

      @nodeG.append "text"
        .attr "text-anchor", "middle"
        .attr "font-size", "24px"
        .attr "dy", ".35em"
        .attr "transform", (d) =>
          if (d.participant == 'energy')
            ""
          else
            "rotate(" + (-1 * (@constantRotationAngle() + @angle(d.participant))) + ")"
        .attr "fill", (d) =>
          if (d.participant == @localParticipant)
            "#FFFFFF"
          else
            "#000000"
        .text (d) -> d.name

      @nodesG.selectAll(".node").transition().duration(500)
        .attr "transform", @nodeTransform
        .select('circle') # change circle color
        .attr "fill", @nodeColor

      # remove nodes that have left
      @node.exit().remove()

    # different colors for different types of nodes...
    nodeColor: (d) =>
      if (d.participant == 'energy')
        @sphereColorScale(@data.transitions)
      else if d.participant == @localParticipant
        '#092070'
      else
        '#3AC4C5'

    # we have different kinds of nodes, so this just abstracts
    # out the transform function.
    nodeTransform: (d) =>
      if (d.participant == "energy")
        @sphereTranslation()
      else
        "rotate(" + @angle(d.participant) + ")translate(" + @radius + ",0)"

    # a translatoin between the angle rotation for nodes
    # and the raw x/y positions. Used for computing link endpoints.
    getNodeCoords: (id) =>
      transformText = @nodeTransform({'participant': id})
      coords = d3.transform(transformText).translate
      return {'x': coords[0], 'y': coords[1]}


    renderLinks: () =>
      @link = @linksG.selectAll "line.link"
        .data @links

      @link.enter()
        .append "line"
        .attr "class", "link"
        .attr "stroke", "#646464"
        .attr "fill", "none"
        .attr "stroke-opacity", 0.8
        .attr "stroke-width", 7
        .attr "x1", (d) => @getNodeCoords(d.source)['x']
        .attr "y1", (d) => @getNodeCoords(d.source)['y']
        .attr "x2", (d) => @getNodeCoords(d.target)['x']
        .attr "y2", (d) => @getNodeCoords(d.target)['y']

      @link.transition().duration(@linkTransitionTime)
        .attr "stroke-width", (d) => @linkStrokeScale d.weight

      @link
        .attr "x1", (d) => @getNodeCoords(d.source)['x']
        .attr "y1", (d) => @getNodeCoords(d.source)['y']
        .attr "x2", (d) => @getNodeCoords(d.target)['x']
        .attr "y2", (d) => @getNodeCoords(d.target)['y']

      @link.exit().remove()


    # translation / position for "energy" ball.
    # Moves closer (just based on weighting) to nodes.
    sphereTranslation: () =>
      x = 0
      y = 0

      for turn in @data.turns
        coords = @getNodeCoords(turn.participant)
        # get coordinates of this node & distance from ball
        node_x = coords['x']
        node_y = coords['y']
        xDist = (node_x - x)
        yDist = (node_y - y)

        # transform x and y proportional to the percentage of turns
        # (and use dist/2 to prevent collision)
        x += turn.turns * (xDist / 2)
        y += turn.turns * (yDist / 2)
      return "translate(" + x + "," + y + ")"

    # create links, give it a 0 default (all nodes should be linked to
    # ball)
    createLinks: () =>
      @links = ({'source': turn.participant, 'target': 'energy', 'weight': turn.turns} for turn in @data.turns)
      for participant in @data.participants
        if !_.find(@links, (link) => link.source == participant)
          @links.push({'source': participant, 'target': 'energy', 'weight': 0})


    # we want the users's node always at the top
    # This returns a translation string to rotate the _entire_
    # "graph group" to keep the user's node at the top.
    constantRotationAngle: () =>
      mod = (a, n) -> a - Math.floor(a/n) * n
      angle = @angle(@localParticipant) || 0
      targetAngle = -90
      a = targetAngle - angle
      a = (a + 180) % 360 - 180
      if (angle != -90)
        return a
      else
        return 0

    constantRotation: () =>
        return "rotate(" + @constantRotationAngle() + ")"


    updateData: (data) =>
      console.log "updating MM viz with data:", data
      # if we're not updating participants, don't redraw everything.
      if data.participants.length == @data.participants.length
        @data = data
        @createLinks()

        @renderLinks()
        @renderNodes()
      else
        # Create nodes again
        @data = data
        @nodes = ({'participant': p} for p in @data.participants)
        @nodes.push({'participant': 'energy'}) # keep the energy ball in the list of nodes

        # recompute the color scale for the sphere and angle domain
        @sphereColorScale.domain [0, data.participants.length * 5]
        @angle.domain @data.participants

        # recompute links
        @link = @linksG.selectAll "line.link"
        .data []
        .exit().remove()
        # Re-render. Do it on a delay to make sure links get rendered after nodes.
        # After links, rotate entire graph so user is at top.
        @renderNodes()
        setTimeout((() =>
          @renderLinks()
          @graphG.transition().duration(100)
            .attr "transform", @constantRotation()
          ), @nodeTransitionTime + 100)
