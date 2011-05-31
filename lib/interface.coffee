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
    client.on("message", () => console.log(this))
    @nanny.on("program-started", (program) =>
      client.send({ 
        "event": "program-started",
        "program": program.name,
        "state": program.state(),
        "pid": program.pid(),
      })
    )
    @nanny.on("program-exited", (program, reason) =>
      client.send({ 
        "event": "program-exited",
        "program": program.name,
        "state": program.state(),
        "reason": reason
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
