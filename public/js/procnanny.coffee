class ProcNanny
  constructor: (name) ->
    @socketio = new io.Socket();
    @connect()
    @name = name

  dispatch: () ->
    {
      "program-started": @update_program
      "program-exited": @update_program
      "program-stdout": (obj) => @update_output("stdout", obj.data)
      "program-stderr": (obj) => @update_output("stderr", obj.data)
    }

  connect: () ->
    @socketio.connect();
    @socketio.on("connect", () => @subscribe())
    @socketio.on("message", (obj) => @receive(obj))
    @socketio.on("disconnect", () => @connect())
  # end connect

  subscribe: () ->
    console.log("subscribe: " + @name)
    return if !@name?

    @socketio.send({
      "action": "subscribe",
      "program": @name
    })

  receive: (obj) -> 
    #console.log(obj)
    callback = @dispatch()[obj.event]
    if callback?
      callback(obj)
  # end receive

  update_program: (obj) ->
    console.log(obj.event + " -> " + obj.program + "[" + obj.pid + "]")
    basequery = "#program-" + obj.program
    $(basequery + " .program-pid").html(obj.pid)
    $(basequery + " .program-state").html(obj.state)

    if obj.event == "program-started"
      $(basequery).removeClass("sad")
      $(basequery).addClass("happy")
    else
      $(basequery).removeClass("happy")
      $(basequery).addClass("sad")
    # end if
  # end update_program

  update_output: (target, data) ->
    console.log(data)
    $("pre." + target).get(0).childNodes[0].appendData(data)
  # end update_output
# end class ProcNanny

window.ProcNanny = ProcNanny
