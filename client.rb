require "rubygems"
require "zmq"
require "msgpack"
require "awesome_print"

if ARGV.length != 1
  puts "Usage: #{$0} <request|restarT|dance>"
  exit 1
end

ctx = ZMQ::Context.new(1)
socket = ctx.socket(ZMQ::REQ);
socket.connect("tcp://localhost:3333")
#socket.setsockopt(ZMQ::HWM, 100);

message = {
  "request" => ARGV[0]
}

socket.send(MessagePack.pack(message))
response = socket.recv
ap MessagePack.unpack(response)
socket.close
