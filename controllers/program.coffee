class ProgramController
  constructor: (server, nanny) ->
    @nanny = nanny
    server.get("/programs", (req, res) => @index(req,res))
    server.get("/programs/:name", (req, res) => @show(req,res))
    server.get("/programs/:name/stop", (req, res) => @stop(req,res))
    server.get("/programs/:name/start", (req, res) => @start(req,res))
    server.get("/programs/:name/restart", (req, res) => @restart(req,res))
    server.get("/programs/:name/log", (req, res) => @log(req,res))
  # end constructor

  index: (request, response) ->
    console.log("Hello")
    response.render("programs/index.jade", {
      programs: @nanny.programs
    })
  # end index

  show: (request, response) ->
    response.render("programs/show.jade", {
      program: @nanny.programs[request.params.name]
    })
  # end show

  stop: (request, response) -> 
    @nanny.programs[request.params.name].stop()
    response.send("OK\n")

  start: (request, response) ->
    @nanny.programs[request.params.name].start()
    response.send("OK\n")

  restart: (request, response) -> 
    @nanny.programs[request.params.name].restart()
    response.send("OK\n")

  log: (request, response) -> null
# end class ProgramController

exports = module.exports = ProgramController
