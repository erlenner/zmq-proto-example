#include <string>
#include <chrono>
#include <thread>
#include <stdio.h>

#include <zmq.hpp>
#include "zmq_proto.h"
#include "protocol.pb.h"
#include <google/protobuf/empty.pb.h>

int main() 
{
  zmq_proto_context context{1};
  zmq_proto_socket<ZMQ_PROTO_REP> socket(context, "tcp://*:5555");

  while (1)
  {
    google::protobuf::Any req;
    const int received = zmq_proto_recv(socket, req);

    if (received >= 0)
    {
      if (req.Is<protocol::msg_t>())
      {
        protocol::msg_t msg;
        req.UnpackTo(&msg);
        printf("recv %d %d\n", msg.a(), msg.b());
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(10));

      google::protobuf::Any rep;
      rep.PackFrom(google::protobuf::Empty{});

      int sent = zmq_proto_send(socket, rep);
      if (sent >= 0)
        printf("send ack\n");
    }
  }

  return 0;
}
