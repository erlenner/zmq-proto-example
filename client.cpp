#include <string>
#include <stdio.h>

#include <zmq.hpp>

#include "protocol.pb.h"

int main()
{
  zmq::context_t context{1};

  zmq::socket_t socket{context, zmq::socket_type::req};
  socket.connect("tcp://localhost:5555");

  const std::string data{"Hello"};

  for (auto request_num = 0;; ++request_num)
  {
    protocol::msg_t msg;
    msg.set_a(435);
    msg.set_b(219);

    printf("send %d %d\n", msg.a(), msg.b());

    google::protobuf::Any any;
    any.PackFrom(msg);
    const size_t ser_size = any.ByteSizeLong();
    char ser[ser_size];
    any.SerializeToArray(ser, any.ByteSizeLong());
    zmq::send_result_t sent = socket.send(zmq::message_t(ser, ser_size), zmq::send_flags::none);

    zmq::message_t reply{};
    zmq::recv_result_t received = socket.recv(reply, zmq::recv_flags::none);

    printf("recv %s\n", reply.data());
  }

  return 0;
}
