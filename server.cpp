#include <string>
#include <chrono>
#include <thread>
#include <iostream>

#include <zmq.hpp>

int main() 
{
  // initialize the zmq context with a single IO thread
  zmq::context_t context{1};

  // construct a REP (reply) socket and bind to interface
  zmq::socket_t socket{context, zmq::socket_type::rep};
  socket.bind("tcp://*:5555");

  // prepare some static data for responses
  const std::string data{"World"};

  for (;;)
  {
    zmq::message_t request;

    // receive a request from client
    zmq::recv_result_t received = socket.recv(request, zmq::recv_flags::none);
    if (received)
    {
      std::cout << "Received " << request.to_string() << std::endl;

      // simulate work
      std::this_thread::sleep_for(std::chrono::milliseconds(10));

      // send the reply to the client
      zmq::send_result_t sent = socket.send(zmq::buffer(data), zmq::send_flags::none);
    }
  }

  return 0;
}
