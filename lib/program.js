(function() {
  var EventEmitter, Program, child_process;
  var __hasProp = Object.prototype.hasOwnProperty, __extends = function(child, parent) {
    for (var key in parent) { if (__hasProp.call(parent, key)) child[key] = parent[key]; }
    function ctor() { this.constructor = child; }
    ctor.prototype = parent.prototype;
    child.prototype = new ctor;
    child.__super__ = parent.prototype;
    return child;
  }, __bind = function(fn, me){ return function(){ return fn.apply(me, arguments); }; };
  child_process = require('child_process');
  EventEmitter = require("events").EventEmitter;
  Program = (function() {
    __extends(Program, EventEmitter);
    function Program(options) {
      this.name = options.name;
      this.command = options.args[0];
      this.args = options.args.slice(1);
      this.numprocs = options.numprocs || 1;
      this.nice = options.nice || 0;
      this.state("new");
    }
    Program.prototype.start = function() {
      if (this.start_timer != null) {
        clearTimeout(this.start_timer);
      }
      this.emit("started");
      this.state("started");
      this.child = child_process.spawn(this.command, this.args);
      return this.child.on("exit", __bind(function(code, signal) {
        return this.exited(code, signal);
      }, this));
    };
    Program.prototype.signal = function(signal) {
      if (this.child != null) {
        return this.child.kill(signal);
      }
    };
    Program.prototype.stop = function() {
      console.log("Stopping " + this.name);
      if (this.child != null) {
        this.signal("SIGTERM");
        this.state("stopping");
      } else {
        this.state("stopped");
      }
      if (this.start_timer != null) {
        return clearTimeout(this.start_timer);
      }
    };
    Program.prototype.restart = function() {
      console.log("Restarting " + this.name);
      this.signal("SIGTERM");
      return this.state("restarting");
    };
    Program.prototype.exited = function(code, signal) {
      var callback;
      console.log(this.name + " exited: " + code + "/" + signal);
      if (code != null) {
        this.emit("exited", code);
      } else {
        this.emit("exited", signal);
      }
      if (this.state() !== "stopping") {
        this.state("died/restarting");
        callback = __bind(function() {
          return this.start();
        }, this);
        this.start_timer = setTimeout(callback, 1000);
      } else {
        this.state("stopped");
      }
      return this.child = void 0;
    };
    Program.prototype.state = function(state) {
      if (state != null) {
        this._state = state;
      }
      return this._state;
    };
    Program.prototype.pid = function() {
      if (this.child != null) {
        return this.child.pid;
      }
    };
    return Program;
  })();
  exports.Program = Program;
}).call(this);
