require "rubygems"
require "zmq"
require "msgpack"
require "awesome_print"

ctx = ZMQ::Context.new(1)
socket = ctx.socket(ZMQ::REQ);
socket.connect("tcp://localhost:3333")
#socket.setsockopt(ZMQ::HWM, 100);
socket.send(
  #MessagePack.pack({"request" => "jump", "foo" => "bar"})
  #MessagePack.pack({"request" => "dance", "foo" => "bar"})
  MessagePack.pack({"request" => "restart", "program" => "hello world"})
)
response = socket.recv
ap MessagePack.unpack(response)
socket.close
