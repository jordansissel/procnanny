(function() {
  var Controllers, Interface, express, socketio;
  var __bind = function(fn, me){ return function(){ return fn.apply(me, arguments); }; };
  express = require("express");
  socketio = require("socket.io");
  Controllers = {
    Program: require("../controllers/program.js")
  };
  Interface = (function() {
    function Interface(nanny) {
      this.server = express.createServer();
      this.socketio = socketio.listen(this.server);
      this.server.use(express.static(__dirname + "../public"));
      this.server.use(express.logger(":method :url (:response-time ms)"));
      this.server.use(express.bodyParser());
      this.server.use(express.methodOverride());
      this.server.use(express.cookieParser());
      this.server.use(express.session({
        secret: "keyboard cat"
      }));
      this.server.use(this.server.router);
      this.server.use(express.static(__dirname + "/../public"));
      this.server.use(express.errorHandler({
        dumpExceptions: true,
        showStack: true
      }));
      this.socketio.on("connection", __bind(function(client) {
        return this.new_socketio_client(client);
      }, this));
      this.nanny = nanny;
      this.routes();
    }
    Interface.prototype.new_socketio_client = function(client) {
      client.on("message", __bind(function() {
        return console.log(this);
      }, this));
      this.nanny.on("program-started", __bind(function(program) {
        return client.send("Program started: " + program.named);
      }, this));
      return this.nanny.on("program-exited", __bind(function(program, reason) {
        return client.send("Program died: " + program.named + ", " + reason);
      }, this));
    };
    Interface.prototype.run = function() {
      return this.server.listen(3000);
    };
    Interface.prototype.shutdown = function() {
      try {
        this.server.close();
        return this.server = void 0;
      } catch (error) {
        return console.log("While shutting down server: " + error);
      }
    };
    Interface.prototype.routes = function() {
      var controller, name, _results;
      this.server.get("/", __bind(function(req, res) {
        return this.index(req, res);
      }, this));
      this.server.get("/test", __bind(function(req, res) {
        return res.send("Hello");
      }, this));
      _results = [];
      for (name in Controllers) {
        controller = Controllers[name];
        _results.push(new controller(this.server, this.nanny));
      }
      return _results;
    };
    Interface.prototype.index = function(request, response) {
      return response.redirect("/programs", 302);
    };
    return Interface;
  })();
  exports.Interface = Interface;
}).call(this);
