#include <string>
#include <stdio.h>

#include <zmq.h>
#include "zmq_proto.h"
#include "protocol.pb.h"

int main()
{
  zmq_proto_context context{1};
  zmq_proto_socket<ZMQ_PROTO_SUB> socket(context, "tcp://localhost:5555");

  while (1)
  {
    protocol::msg_t msg;
    const int received = zmq_proto_recv(socket, msg);

    if (received > 0)
      printf("recv %d %d\n", msg.a(), msg.b());
    else if(received == 0)
      printf("got unexpected message\n");
  }

  return 0;
}
