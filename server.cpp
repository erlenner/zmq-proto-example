#include <string>
#include <chrono>
#include <thread>
#include <stdio.h>

#include "zmq_proto.h"
#include "protocol.pb.h"

int main() 
{
  zmq_proto::context_t context{1};
  zmq_proto::socket_t<zmq_proto::REP> socket(context, "tcp://*:5555");

  while (1)
  {
    printf("i\n");
    protocol::msg_t req;
    const int received = zmq_proto::recv(socket, req);

    if (received > 0)
    {
      printf("recv %d %d\n", req.a(), req.b());

      std::this_thread::sleep_for(std::chrono::milliseconds(10));

      int sent = zmq_proto::send(socket, google::protobuf::Empty{});
      if (sent >= 0)
        printf("send ack\n");
    }
    else if(received == 0)
    {
      printf("got unexpected reply message\n");
    }
  }

  return 0;
}
