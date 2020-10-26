#include <string>
#include <stdio.h>

#include <zmq.hpp>

#include "protocol.pb.h"

int main()
{
  // initialize the zmq context with a single IO thread
  zmq::context_t context{1};

  // construct a REQ (request) socket and connect to interface
  zmq::socket_t socket{context, zmq::socket_type::req};
  socket.connect("tcp://localhost:5555");

  // set up some static data to send
  const std::string data{"Hello"};

  for (auto request_num = 0;; ++request_num)
  {

    protocol::msg_t msg;
    msg.set_a(435);
    msg.set_b(219);

    std::string msg_serialized = msg.SerializeAsString();
    printf("send %d %d\n", msg.a(), msg.b());
    zmq::send_result_t sent = socket.send(zmq::buffer(msg_serialized), zmq::send_flags::none);

    // wait for reply from server
    zmq::message_t reply{};
    zmq::recv_result_t received = socket.recv(reply, zmq::recv_flags::none);

    std::cout << "Received " << reply.to_string();
    std::cout << " (" << request_num << ")";
    std::cout << std::endl;
  }

  return 0;
}
