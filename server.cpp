#include <string>
#include <chrono>
#include <thread>
#include <stdio.h>

#include <zmq.hpp>

#include "protocol.pb.h"

int main() 
{
  void *context = zmq_ctx_new();
  assert(context != NULL);

  {
    int rc = zmq_ctx_set(context, ZMQ_IO_THREADS, 1);
    assert(rc == 0);
  }

  void *socket = zmq_socket(context, ZMQ_REP);
  assert(socket != NULL);

  {
    int rc = zmq_bind(socket, "tcp://localhost:5555");
    assert(rc == 0);
  }

  while (1)
  {
    zmq_msg_t req;
    {
      int rc = zmq_msg_init(&msg);
      assert (rc == 0);
    }
    const int received = zmq_msg_recv(&msg, socket, 0);

    if (received >= 0)
    {
      assert(zmq_msg_size(&msg) == received);

      google::protobuf::Any any;
      any.ParseFromArray(zmq_msg_data(msg), zmq_msg_size(&msg));

      if (any.Is<protocol::msg_t>())
      {
        protocol::msg_t msg;
        any.UnpackTo(&msg);
        printf("recv %d %d\n", msg.a(), msg.b());
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(10));

      {
        google::protobuf::Any any;
        any.PackFrom(google::protobuf::Empty);
        const size_t ser_size = any.ByteSizeLong();
        char ser[ser_size];
        any.SerializeToArray(ser, any.ByteSizeLong());
      }

      const int sent = zmq_msg_send(zmq::message_t("ack", 10), zmq::send_flags::none);

      if (sent)
        printf("send ack\n");
    }
    else
    {
      assert(zmq_errno() == EAGAIN);
    }

    {
      int rc = zmq_msg_close(&msg);
      assert(rc == 0);
    }
  }

  // close socket
  {
    int rc = zmq_close(socket);
    assert(rc == 0);
  }

  // close context
  {
    int rc;
    do {
      rc = zmq_ctx_destroy(ptr);
    } while (rc == -1 && zmq_errno == EINTR);
  }

  return 0;
}
