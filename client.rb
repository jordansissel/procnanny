require "rubygems"
require "zmq"
require "msgpack"
require "awesome_print"

ctx = ZMQ::Context.new(1)

message = {
  "request" => "create",
  "program" => "newprogram",
  "command" => "date",
  "args" => [ "Today is: +%Y-%m-%d" ]
}

socket = ctx.socket(ZMQ::REQ);
socket.connect("tcp://localhost:3333")
socket.send(MessagePack.pack(message))
response = socket.recv
ap MessagePack.unpack(response)
socket.close
