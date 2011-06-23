require "rubygems"
require "zmq"
require "msgpack"
require "awesome_print"

ctx = ZMQ::Context.new(1)
socket = ctx.socket(ZMQ::SUB);
p :connect => socket.connect("tcp://127.0.0.1:3344")
p :setsockopt => socket.setsockopt(ZMQ::SUBSCRIBE, "")

while true
  msg = MessagePack.unpack(socket.recv)
  p msg["event"] => msg
end
