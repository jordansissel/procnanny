zeromq = require("zeromq")
msgpack = require("msgpack-0.4")
events = require("events")
Interface = require("./lib/interface.js")

class ProcNanny
  constructor: () ->
    @eventstream = zeromq.createSocket("sub")
    @eventstream.connect("tcp://127.0.0.1:3344")
    @eventstream.subscribe("")
    @emitter = new events.EventEmitter()
    @eventstream.on("message", (data) => @event(data))
    @eventstream.on("error", (error) => @error(error))

    @interface = new Interface(this)
    @interface.run()

  # end constructor
  
  event: (data) ->
    obj = msgpack.unpack(data)
    console.log(obj)
    @emitter.emit("update", obj)
  # end event
  
  error: (error) ->
    console.log("Error: " + error)
  # end error

  # Get all program data. Returns an object can be bound
  #
  programs: (callback) ->
    socket = zeromq.createSocket("req")
    socket.connect("tcp://127.0.0.1:3333")
    socket.send(msgpack.pack({
      "request": "status"
    }))
    socket.on("message", (data) => 
      obj = msgpack.unpack(data)
      callback(obj)
    )
    # TODO(sissel): what about on error?
  # end programs
# end class ProcNanny

pn = new ProcNanny()
