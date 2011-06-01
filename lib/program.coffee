child_process = require('child_process')
EventEmitter = require("events").EventEmitter


# Program:
#   name
#   command+args
#   user
#   environment
#   directory
#   ulimits
#   log output
#   numprocs
#   nice
#   autostart
#   events like upstart?
#   callbacks
#   history

class Program extends EventEmitter
  constructor: (options) ->
    @setMaxListeners(100)

    @name = options.name
    @command = options.args[0]
    @args = options.args.slice(1)

    @user = options.user or "root"
    @directory = options.directory
    @ulimits = options.ulimit
    @numprocs = options.numprocs or 1
    @nice = options.nice or 0

    @environment = {}
    for name,value of process.env
      @environment[name] = value

    for name,value of options.environment
      @environment[name] = value

    @environment["PROC_USER"] = @user if @user?
    @environment["PROC_DIRECTORY"] = @directory if @directory?
    @environment["PROC_NICE"] = @nice

    for type, value of @ulimits
      @environment["PROC_ULIMIT_" + type.toUpperCase()] = value

    @state("new")
  # end constructor
  
  start: () ->
    return if @child?
    # TODO(sissel): Track history
    clearTimeout(@start_timer) if @start_timer?
    # TODO(sissel): Start N procs according to @numprocs
    runner = __dirname + "/shell/runner.sh"
    args = [runner, @command].concat(@args)

    @child = child_process.spawn("sh", args, {
      env: @environment 
    })

    @child.on("exit", (code, signal) => @exited(code, signal))

    @child.stdout.setEncoding("utf8")
    @child.stderr.setEncoding("utf8")

    @child.stdout.on("data", (data) => @stdout(data))
    @child.stderr.on("data", (data) => @stderr(data))

    @state("started")
    @emit("started")
  # end start
  
  stdout: (data) -> @emit("stdout", data)
  stderr: (data) -> @emit("stderr", data)

  signal: (signal) -> @child.kill(signal) if @child?

  stop: () ->
    # State moves:
    #   if @child is running, move to "stopping"
    #   else, move to "stopped"
    # TODO(sissel): Track history
    console.log("Stopping " + @name)

    if @child?
      @signal("SIGTERM")
      @state("stopping")
    else
      @state("stopped")
    # end if

    clearTimeout(@start_timer) if @start_timer?
  # end stop
  
  restart: () -> 
    console.log("Restarting " + @name)

    if @child?
      @signal("SIGTERM")
      @state("restarting")
    else
      @start()

  exited: (code, signal) ->
    # TODO(sissel): Track history
    console.log(@name + " exited: " + code + "/" + signal)
    if @state() != "stopping"
      @state("died/restarting")
      callback = () => @start()
      @start_timer = setTimeout(callback, 1000)
    else
      @state("stopped")
    # end if
    @child = undefined

    if code?
      @emit("exited", code)
    else
      @emit("exited", signal)

  # end exited

  state: (state) ->
    if state?
      @last_change = (new Date())
      @_state = state
    # end if 
    @_state
  # end state

  pid: () -> @child.pid if @child?
#end class Program

exports.Program = Program
