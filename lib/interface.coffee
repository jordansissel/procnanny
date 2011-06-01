express = require("express")
socketio = require("socket.io")

Controllers = {
  Program: require("../controllers/program.js")
}

class Interface
  constructor: (nanny) ->
    @server = express.createServer()
    @socketio = socketio.listen(@server)

    @server.use(express.static(__dirname + "../public"))
    @server.use(express.logger(":method :url (:response-time ms)"))
    @server.use(express.bodyParser())
    @server.use(express.methodOverride())
    @server.use(express.cookieParser())
    @server.use(express.session({ secret: "keyboard cat" }))
    @server.use(@server.router)
    @server.use(express.static(__dirname + "/../public"))
    @server.use(express.errorHandler({ dumpExceptions: true, showStack: true }))

    @socketio.on("connection", (client) => @new_socketio_client(client))

    @nanny = nanny
    @routes()
  # end constructor

  new_socketio_client: (client) ->
    client.on("message", (msg) => 
      if msg.action == "subscribe"
        client._only_program = msg.program
    )

    @nanny.on("program-started", (program) =>
      if client._only_program?
        return if client._only_program != program.name
      # end if

      client.send({ 
        event: "program-started",
        program: program.name,
        state: program.state(),
        last_update: program.last_change.toISOString(),
        pid: program.pid(),
      })
    )

    @nanny.on("program-exited", (program, reason) =>
      if client._only_program?
        return if client._only_program != program.name
      # end if

      client.send({ 
        event: "program-exited",
        program: program.name,
        state: program.state(),
        last_update: program.last_change.toISOString(),
        reason: reason
      })
    )

    @nanny.on("program-stdout", (program, data) =>
      # Only ship stdout if we are watching one program.
      return if client._only_program != program.name

      client.send({ 
        event: "program-stdout",
        program: program.name,
        state: program.state(),
        last_update: program.last_change.toISOString(),
        data: data
      })
    )

    @nanny.on("program-stderr", (program, data) =>
      # Only ship stdout if we are watching one program.
      return if client._only_program != program.name

      client.send({ 
        event: "program-stderr",
        program: program.name,
        state: program.state(),
        last_update: program.last_change.toISOString(),
        data: data
      })
    )
  # end new_socketio_client

  run: () -> @server.listen(3000)

  shutdown: () -> 
    try
      @server.close()
      @server = undefined
    catch error
      console.log("While shutting down server: " + error)
      # Ignore
  # end shutdown

  routes: () ->
    @server.get("/", (req, res) => @index(req,res))
    @server.get("/test", (req, res) =>
      res.send("Hello"))

    for name, controller of Controllers
      new controller(@server, @nanny)
  # end routes

  index: (request, response) ->
    response.redirect("/programs", 302)
  # end index
# end class Interface

exports.Interface = Interface
