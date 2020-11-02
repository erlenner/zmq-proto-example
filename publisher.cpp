#include <stdio.h>

#include "zmq_proto.h"
#include "protocol.pb.h"

int main() 
{
  zmq_proto::context_t context{1};
  zmq_proto::socket_t<zmq_proto::PUB> socket(context, "tcp://*:5555");

  while (1)
  {
    printf("i\n");
    protocol::msg_t msg;
    msg.set_a(435);
    msg.set_b(219);

    int sent = zmq_proto::send(msg, socket, "some_target");

    if (sent >= 0)
      printf("send %d %d\n", msg.a(), msg.b());

    zmq_proto::send(::google::protobuf::Empty(), socket, "some_target");
    zmq_proto::send(::google::protobuf::Empty(), socket, "some_target");
  }

  return 0;
}
