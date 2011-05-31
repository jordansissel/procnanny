class ProcNanny
  constructor: () ->
    @socketio = new io.Socket();
    @connect()

  dispatch: () ->
    {
      "program-started": @update_program
      "program-exited": @update_program
    }

  connect: () ->
    @socketio.connect();
    @socketio.on("connect", () => console.log("Connected"))
    @socketio.on("message", (obj) => @receive(obj))
    @socketio.on("disconnect", () => @connect())
  # end connect

  receive: (obj) -> 
    #console.log(obj)
    callback = @dispatch()[obj.event]
    if callback?
      callback(obj)
  # end receive

  update_program: (obj) ->
    console.log(obj.event + " -> " + obj.program + "[" + obj.pid + "]")
    basequery = "#program-" + obj.program
    $(basequery + " td.program-pid").html(obj.pid)
    $(basequery + " td.program-state").html(obj.state)
  # end update_program
# end class ProcNanny

pn = new ProcNanny()
