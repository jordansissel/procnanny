(function() {
  var ProcNanny, pn;
  var __bind = function(fn, me){ return function(){ return fn.apply(me, arguments); }; };
  ProcNanny = (function() {
    function ProcNanny() {
      this.socketio = new io.Socket();
      this.connect();
    }
    ProcNanny.prototype.connect = function() {
      this.socketio.connect();
      this.socketio.on("connect", __bind(function() {
        return console.log("Connected");
      }, this));
      this.socketio.on("message", __bind(function(obj) {
        return this.receive(obj);
      }, this));
      return this.socketio.on("disconnect", __bind(function() {
        return this.connect();
      }, this));
    };
    ProcNanny.prototype.receive = function(obj) {
      return console.log(obj);
    };
    return ProcNanny;
  })();
  pn = new ProcNanny();
}).call(this);
