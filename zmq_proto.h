#include <zmq.h>
#include <google/protobuf/message.h>

//#define zmq_proto_check(...) do { int rc = __VA_ARGS__; assert(rc == 0); } while(0)

class zmq_proto_context
{
  void *context;

public:

  zmq_proto_context()
  : context(NULL)
  {}

  zmq_proto_context(int n_threads)
  {
    int rc = init(n_threads); // ignore return value
    assert(rc == 0);
  }

  int init(int n_threads)
  {
    context = zmq_ctx_new();
    assert(context != NULL);

    {
      int rc = zmq_ctx_set(context, ZMQ_IO_THREADS, n_threads);
      assert(rc == 0);
    }

    return 0;
  }

  void* get_handle() const
  {
    return context;
  }

  ~zmq_proto_context()
  {
    int rc;
    do {
      rc = zmq_ctx_destroy(context);
    } while (rc == -1 && zmq_errno() == EINTR);

    assert(rc == 0);
    context = NULL;
  }
};

enum zmq_proto_socket_type{ ZMQ_PROTO_REQ = ZMQ_REQ, ZMQ_PROTO_REP = ZMQ_REP };

template<zmq_proto_socket_type socket_type>
class zmq_proto_socket
{
  void *socket;

public:

  zmq_proto_socket()
  : socket(NULL)
  {}

  zmq_proto_socket(const zmq_proto_context& context, const char *addr)
  {
    int rc = init(context, addr);
    assert(rc == 0);
  }

  int init(const zmq_proto_context &context, const char *addr)
  {
    socket = zmq_socket(context.get_handle(), socket_type);
    assert(socket != NULL);

    {
      int rc;

      switch(socket_type)
      {
        case ZMQ_PROTO_REQ:
          rc = zmq_connect(socket, addr);
          break;
        case ZMQ_PROTO_REP:
          rc = zmq_bind(socket, addr);
          break;
      }

      assert(rc == 0);
    }

    return 0;
  }

  void* get_handle() const
  {
    return socket;
  }

  ~zmq_proto_socket()
  {
    int rc = zmq_close(socket);
    assert(rc == 0);

    socket = NULL;
  }
};


class zmq_proto_msg
{
  zmq_msg_t msg;

public:

  zmq_proto_msg()
  {}

  zmq_proto_msg(int size)
  {
    init(size);
  }

  int init(int size)
  {
    if (size <= -1)
    {
      int rc = zmq_msg_init(&msg);
      assert (rc == 0);
    }
    else
    {
      assert(false); // not implemented
    }
    return 0;
  }

  zmq_msg_t* get_handle()
  {
    return &msg;
  }

  ~zmq_proto_msg()
  {
    int rc = zmq_msg_close(&msg);
    assert(rc == 0);
  }

};

template<zmq_proto_socket_type socket_type, typename Message>
std::enable_if_t<std::is_base_of<::google::protobuf::Message, Message>::value, int>
zmq_proto_send(const zmq_proto_socket<socket_type>& socket, const Message& msg)
{
  const int ser_size = msg.ByteSizeLong();
  char ser[ser_size];
  msg.SerializeToArray(ser, msg.ByteSizeLong());

  int sent = zmq_send(socket.get_handle(), ser, ser_size, 0);

  if (sent < 0)
  {
    assert(zmq_errno() == EAGAIN);
    sent = -1;
  }

  return sent;
}


template<zmq_proto_socket_type socket_type, typename Message>
std::enable_if_t<std::is_base_of<::google::protobuf::Message, Message>::value, int>
zmq_proto_recv(const zmq_proto_socket<socket_type>& socket, Message& proto_msg)
{
  zmq_proto_msg msg{-1};

  int received = zmq_msg_recv(msg.get_handle(), socket.get_handle(), 0);

  if (received < 0)
  {
    assert(zmq_errno() == EAGAIN);
    received = -1;
  }
  else
  {
    assert(zmq_msg_size(msg.get_handle()) == static_cast<size_t>(received));

    proto_msg.ParseFromArray(zmq_msg_data(msg.get_handle()), zmq_msg_size(msg.get_handle()));
  }

  return received;
}

