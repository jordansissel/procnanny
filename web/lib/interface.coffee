express = require("express")
socketio = require("socket.io")
msgpack = require("msgpack-0.4")

Controllers = {
  Program: require("../controllers/program.js")
}

class Interface
  constructor: (procnanny) ->
    @server = express.createServer()
    @socketio = socketio.listen(@server)
    @procnanny = procnanny

    @server.use(express.static(__dirname + "/../public"))
    @server.use(express.logger(":method :url (:response-time ms)"))
    @server.use(express.bodyParser())
    @server.use(express.methodOverride())
    @server.use(express.cookieParser())
    @server.use(express.session({ secret: "keyboard cat" }))
    @server.use(@server.router)
    @server.use(express.static(__dirname + "/../public"))
    @server.use(express.errorHandler({ dumpExceptions: true, showStack: true }))

    @socketio.sockets.on("connection", (client) => @new_socketio_client(client))
    @routes()
  # end constructor

  new_socketio_client: (client) ->
    #client.on("message", (msg) => 
      #if msg.action == "subscribe"
        #client._only_program = msg.program
    #)

    @procnanny.emitter.on("update", (data) =>
      client.json.send(data)
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
    @server.get("/test", (req, res) => res.send("Hello"))

    for name, controller of Controllers
      new controller(@server, @procnanny)
  # end routes

  index: (request, response) ->
    response.redirect("/programs", 302)
  # end index
# end class Interface

exports = module.exports = Interface
