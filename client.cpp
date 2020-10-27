#include <string>
#include <stdio.h>

#include <zmq.h>
#include "zmq_proto.h"
#include "protocol.pb.h"

int main()
{
  zmq_proto_context context{1};
  zmq_proto_socket<ZMQ_PROTO_REQ> socket(context, "tcp://localhost:5555");

  while (1)
  {
    protocol::msg_t msg;
    msg.set_a(435);
    msg.set_b(219);

    google::protobuf::Any req;
    req.PackFrom(msg);

    int sent = zmq_proto_send(socket, req);

    if (sent >= 0)
    {
      printf("send %d %d\n", msg.a(), msg.b());


      google::protobuf::Any rep;
      const int received = zmq_proto_recv(socket, rep);

      if (received >= 0)
        printf("recv ack\n");
    }
  }

  return 0;
}
