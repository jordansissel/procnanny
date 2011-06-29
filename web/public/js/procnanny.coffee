class ProcNanny
  constructor: (name) ->
    #@socketio = new io.Socket()
    @socketio = io.connect()
    @connect()
    @name = name

  dispatch: () ->
    {
      #"program-started": @update_program
      #"program-exited": @update_program
      #"program-stdout": (obj) => @update_output("stdout", obj.data)
      #"program-stderr": (obj) => @update_output("stderr", obj.data)
      "running": (obj) => @update_program(obj),
      "starting": (obj) => @update_program(obj),
      "exited": (obj) => @update_program(obj),
      "new": (obj) => @update_program(obj),
    }

  connect: () ->
    @socketio.on("connect", () => @subscribe())
    @socketio.on("message", (obj) => @receive(obj))
    @socketio.on("disconnect", () => @connect())
  # end connect

  subscribe: () ->
    #console.log("subscribe: " + @name)
    return if !@name?

    @socketio.send({
      "action": "subscribe",
      "program": @name
    })

  receive: (obj) -> 
    callback = @dispatch()[obj.event]
    if callback?
      #console.log(obj)
      callback(obj) 
  # end receive

  update_program: (obj) ->
    console.log(obj.event + " -> " + obj.program + "[" + obj.instance + "]")
    basequery = "#program-" + obj.program
    #$(basequery + " .program-pid").html(obj.pid)
    #$(basequery + " .program-state").html(obj.state)
    #$(basequery + " .program-last-update").html(obj.last_update)
    #$(basequery + " .program-user").html(obj.config.user)
    #$(basequery + " .program-instances").html(obj.config.numprocs)

    # Since 'obj.program' can have spaces and I don't patch that yet in the web
    # ui...
    instance_id = "program-" + obj.program + "-" + obj.instance + "-state"
    instance_el = document.getElementById(instance_id)
    console.log($(instance_el))
    @eventify(instance_el, obj.event)
  # end update_program

  eventify: (element, event) ->
    if event == "starting"
      $(element).removeClass("program-exited")
      $(element).removeClass("program-running")
      $(element).addClass("program-starting")
    else if event == "exited"
      $(element).removeClass("program-starting")
      $(element).removeClass("program-running")
      $(element).addClass("program-exited")
    else if event == "running"
      $(element).removeClass("program-starting")
      $(element).removeClass("program-exited")
      $(element).addClass("program-running")
    # end if
  # end eventify

  update_output: (target, data) ->
    console.log(data)

    #$("pre." + target).get(0).childNodes[0].appendData(data)
    #$("pre.stdout").get(0).childNodes[0].appendData(data)
    el = $("pre.stdout").get(0)
    textnode = el.childNodes[0]
    if !textnode?
      el.appendChild(document.createTextNode(data))
    else
      textnode.appendData(data)
    # end if
  # end update_output
# end class ProcNanny

window.ProcNanny = ProcNanny
