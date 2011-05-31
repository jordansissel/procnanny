(function() {
  var ProgramController, exports;
  var __bind = function(fn, me){ return function(){ return fn.apply(me, arguments); }; };
  ProgramController = (function() {
    function ProgramController(server, nanny) {
      this.nanny = nanny;
      server.get("/programs", __bind(function(req, res) {
        return this.index(req, res);
      }, this));
      server.get("/programs/:name", __bind(function(req, res) {
        return this.show(req, res);
      }, this));
      server.get("/programs/:name/stop", __bind(function(req, res) {
        return this.stop(req, res);
      }, this));
      server.get("/programs/:name/start", __bind(function(req, res) {
        return this.start(req, res);
      }, this));
      server.get("/programs/:name/restart", __bind(function(req, res) {
        return this.restart(req, res);
      }, this));
      server.get("/programs/:name/log", __bind(function(req, res) {
        return this.log(req, res);
      }, this));
    }
    ProgramController.prototype.index = function(request, response) {
      console.log("Hello");
      return response.render("programs/index.jade", {
        programs: this.nanny.programs
      });
    };
    ProgramController.prototype.show = function(request, response) {
      return response.render("programs/show.jade", {
        program: this.nanny.programs[request.params.name]
      });
    };
    ProgramController.prototype.stop = function(request, response) {
      return null;
    };
    ProgramController.prototype.start = function(request, response) {
      return null;
    };
    ProgramController.prototype.restart = function(request, response) {
      this.nanny.programs[request.params.name].restart();
      return response.send("OK\n");
    };
    ProgramController.prototype.log = function(request, response) {
      return null;
    };
    return ProgramController;
  })();
  exports = module.exports = ProgramController;
}).call(this);
