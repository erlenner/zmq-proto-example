#include <string>
#include <stdio.h>

#include <zmq.h>

#include "protocol.pb.h"
#include "zmq_proto.h"

int main()
{
  zmq_proto_context context{1};

  zmq_proto_socket<ZMQ_PROTO_REQ> socket(context, "tcp://localhost:5555");

  while (1)
  {
    protocol::msg_t msg;
    msg.set_a(435);
    msg.set_b(219);

    google::protobuf::Any any;
    any.PackFrom(msg);

    int sent = zmq_proto_send(socket, any);

    if (sent)
    {
      printf("send %d %d\n", msg.a(), msg.b());

      //zmq::message_t reply;
      //zmq::recv_result_t received = socket.recv(reply, zmq::recv_flags::none);

      //if (received)
      //  printf("recv %s\n", reply.data());
    }
  }

  return 0;
}
