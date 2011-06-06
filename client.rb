require "rubygems"
require "msgpack"
require "socket"
require "ap"

host = ARGV[0]

sockargs = []
sockargs << Socket::AF_INET6 if host =~ /:/
socket = UDPSocket.new(*sockargs)
#socket.send(MessagePack.pack("Hello world"), 0, "localhost", 3000);

p MessagePack.pack([ "foo", { "testing" => 33.4 }])

1.upto(1) do |i|
  #socket.send(MessagePack.pack({ "request" => "dance", "testing" => 33.4 }), 0, ARGV[0], ARGV[1].to_i);
  #socket.send(MessagePack.pack({ "request" => "restart", "program" => "hello world", "instance" => 53}), 0, ARGV[0], ARGV[1].to_i);
  socket.send(MessagePack.pack({ "request" => "restart", "program" => "hello world"}), 0, ARGV[0], ARGV[1].to_i);
  data = socket.sysread(4096)
  r = MessagePack.unpack(data)
  ap r
end
