class ProcNanny
  constructor: () ->
    @socketio = new io.Socket();
    @connect()

  connect: () ->
    @socketio.connect();
    @socketio.on("connect", () => console.log("Connected"))
    @socketio.on("message", (obj) => @receive(obj))
    @socketio.on("disconnect", () => @connect())

  receive: (obj) -> console.log(obj)
# end class ProcNanny

pn = new ProcNanny()
