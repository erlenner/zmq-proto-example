#include <string>
#include <stdio.h>

#include "zmq_proto.h"
#include "protocol.pb.h"

int main()
{
  zmq_proto::context_t context{1};
  zmq_proto::socket_t<zmq_proto::REQ> socket(context, "tcp://localhost:5555");

  while (1)
  {
    const char endpoint_id[] = "some_target";

    protocol::msg_t msg;
    msg.set_a(435);
    msg.set_b(219);

    int sent = zmq_proto::send(msg, socket, endpoint_id);

    if (sent >= 0)
    {
      printf("send %d %d\n", msg.a(), msg.b());


      google::protobuf::Empty rep;
      const int received = zmq_proto::recv(rep, socket, endpoint_id);

      if (received >= 0)
        printf("recv ack\n");
    }
  }

  return 0;
}
