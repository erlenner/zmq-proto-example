#include <stdio.h>

#include "zmq_proto.h"
#include "protocol.pb.h"

int main() 
{
  zmq_proto_context context{1};
  zmq_proto_socket<ZMQ_PROTO_PUB> socket(context, "tcp://*:5555");

  while (1)
  {
    printf("i\n");
    protocol::msg_t msg;
    msg.set_a(435);
    msg.set_b(219);

    int sent = zmq_proto_send(socket, msg);

    if (sent >= 0)
      printf("send %d %d\n", msg.a(), msg.b());

    zmq_proto_send(socket, ::google::protobuf::Empty());
    zmq_proto_send(socket, ::google::protobuf::Empty());
  }

  return 0;
}
