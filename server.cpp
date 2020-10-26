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

  for (;;)
  {
    zmq::message_t request;

    // receive a request from client
    zmq::recv_result_t received = socket.recv(request, zmq::recv_flags::none);
    if (received)
    {
      protocol::msg_t msg;
      msg.ParseFromString(request.to_string());
      printf("recv: %d %d\n", msg.a(), msg.b());

      // simulate work
      std::this_thread::sleep_for(std::chrono::milliseconds(10));

      // send the reply to the client
      zmq::send_result_t sent = socket.send(zmq::message_t(0), zmq::send_flags::none);
    }
  }

  return 0;
}
