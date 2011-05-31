(function() {
  var EventEmitter, Interface, Nanny, Program;
  var __hasProp = Object.prototype.hasOwnProperty, __extends = function(child, parent) {
    for (var key in parent) { if (__hasProp.call(parent, key)) child[key] = parent[key]; }
    function ctor() { this.constructor = child; }
    ctor.prototype = parent.prototype;
    child.prototype = new ctor;
    child.__super__ = parent.prototype;
    return child;
  }, __bind = function(fn, me){ return function(){ return fn.apply(me, arguments); }; };
  Program = require("./program.js").Program;
  Interface = require("./interface.js").Interface;
  EventEmitter = require("events").EventEmitter;
  Nanny = (function() {
    __extends(Nanny, EventEmitter);
    function Nanny() {
      this.config_paths = ["./programs.d", "/etc/procnanny/programs.d"];
      this.programs = {};
      this.sequence = 0;
    }
    Nanny.prototype.run = function(args) {
      this.register_signal_handlers();
      this.parse_config();
      return this.run_interface();
    };
    Nanny.prototype.register_signal_handlers = function() {
      process.on("SIGHUP", __bind(function() {
        return this.reload();
      }, this));
      process.on("SIGINT", __bind(function() {
        return this.shutdown();
      }, this));
      return process.on("SIGTERM", __bind(function() {
        return this.shutdown();
      }, this));
    };
    Nanny.prototype.parse_config = function() {
      this.start("echo", ["echo", "hello world"]);
      this.start("sleep1", ["sleep", "9"]);
      this.start("sleep2", ["sleep", "15"]);
      this.start("sleep3", ["sleep", "33"]);
      this.start("sleep4", ["sleep", "74"]);
      this.start("sleep5", ["sleep", "55"]);
      this.start("sleep6", ["sleep", "40"]);
      return this.start("sleep7", ["sleep", "45"]);
    };
    Nanny.prototype.run_interface = function() {
      this.interface = new Interface(this);
      return this.interface.run();
    };
    Nanny.prototype.start = function(name, args) {
      var program;
      program = new Program({
        name: name,
        args: args
      });
      program.on("started", __bind(function() {
        return this.emit("program-started", program);
      }, this));
      program.on("exited", __bind(function(reason) {
        return this.emit("program-exited", program, reason);
      }, this));
      program.start();
      return this.programs[name] = program;
    };
    Nanny.prototype.reload = function() {
      return console.log("Reload");
    };
    Nanny.prototype.shutdown = function() {
      var name, program, _ref;
      console.log("shutting down");
      _ref = this.programs;
      for (name in _ref) {
        program = _ref[name];
        program.stop();
      }
      if (this.interface != null) {
        return this.interface.shutdown();
      }
    };
    return Nanny;
  })();
  Nanny.run = function(args) {
    var nanny;
    console.log("Starting");
    nanny = new Nanny();
    return nanny.run(args);
  };
  exports.Nanny = Nanny;
  exports.run = Nanny.run;
}).call(this);
