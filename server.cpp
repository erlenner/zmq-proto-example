#include <string>
#include <chrono>
#include <thread>
#include <stdio.h>

#include <zmq.hpp>

#include "protocol.pb.h"

int main() 
{
  // initialize the zmq context with a single IO thread
  zmq::context_t context{1};

  // construct a REP (reply) socket and bind to interface
  zmq::socket_t socket{context, zmq::socket_type::rep};
  socket.bind("tcp://*:5555");

  while (1)
  {
    zmq::message_t request;

    zmq::recv_result_t received = socket.recv(request, zmq::recv_flags::none);
    if (received)
    {
      google::protobuf::Any any;
      any.ParseFromArray(request.data(), request.size());

      if (any.Is<protocol::msg_t>())
      {
        protocol::msg_t msg;
        any.UnpackTo(&msg);
        printf("recv %d %d\n", msg.a(), msg.b());
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(10));

      zmq::send_result_t sent = socket.send(zmq::message_t("ack", 10), zmq::send_flags::none);

      if (sent)
        printf("send ack\n");
    }
  }

  return 0;
}
