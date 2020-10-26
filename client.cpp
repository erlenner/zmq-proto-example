#include <string>
#include <stdio.h>

#include <zmq.h>

#include "protocol.pb.h"

int main()
{
  void *context = zmq_ctx_new();
  assert(context != NULL);

  {
    int set_io_threads = zmq_ctx_set(context, ZMQ_IO_THREADS, 1);
    assert(set_io_threads == 0);
  }

  void *socket = zmq_socket(context, ZMQ_REQ);
  assert(socket != NULL);

  {
    int rc = zmq_connect(socket, "tcp://localhost:5555");
    assert(rc == 0);
  }

  while (1)
  {
    protocol::msg_t msg;
    msg.set_a(435);
    msg.set_b(219);

    google::protobuf::Any any;
    any.PackFrom(msg);
    const int ser_size = any.ByteSizeLong();
    char ser[ser_size];
    any.SerializeToArray(ser, any.ByteSizeLong());
    int sent = zmq_send(socket, ser, ser_size, 0);

    if (sent)
    {
      printf("send %d %d\n", msg.a(), msg.b());

      zmq::message_t reply;
      zmq::recv_result_t received = socket.recv(reply, zmq::recv_flags::none);

      if (received)
        printf("recv %s\n", reply.data());
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
