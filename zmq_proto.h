#include <zmq.h>
#include <google/protobuf/message.h>
#include <google/protobuf/any.pb.h>
#include <google/protobuf/empty.pb.h>

#define zmq_proto_strerror zmq_strerror
#define zmq_proto_errno zmq_errno()

#ifdef ZMQ_PROTO_DEBUG
#define zmq_proto_assert(expr) do { (void)sizeof(expr); } while(0)
#else
#define zmq_proto_assert(expr) do { if (!(expr)) { fprintf(stderr, "[ %s:%d %s ] " "assertion failed: (" #expr ") errno: %s\n", __FILE__, __LINE__, __func__, zmq_proto_strerror(zmq_proto_errno)); exit(1); } } while(0)
#endif
//#define zmq_proto_check(...) do { int rc = __VA_ARGS__; zmq_proto_assert(rc == 0); } while(0)

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
    zmq_proto_assert(rc == 0);
  }

  int init(int n_threads)
  {
    context = zmq_ctx_new();
    zmq_proto_assert(context != NULL);

    {
      int rc = zmq_ctx_set(context, ZMQ_IO_THREADS, n_threads);
      zmq_proto_assert(rc == 0);
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

    zmq_proto_assert(rc == 0);
    context = NULL;
  }
};

enum zmq_proto_socket_type{ ZMQ_PROTO_REQ = ZMQ_REQ, ZMQ_PROTO_REP = ZMQ_REP, ZMQ_PROTO_PUB = ZMQ_PUB, ZMQ_PROTO_SUB = ZMQ_SUB };

template<zmq_proto_socket_type socket_type>
class zmq_proto_socket
{
  void *socket;
  char *addr;

public:

  zmq_proto_socket()
  : socket(NULL)
  , addr(NULL)
  {}

  zmq_proto_socket(const zmq_proto_context& context, const char *addr)
  {
    int rc = init(context, addr);
    zmq_proto_assert(rc == 0);
  }

  int init(const zmq_proto_context &context, const char *addr)
  {
    const int addr_size = strlen(addr) + 1;
    this->addr = (char*)malloc(addr_size);
    memcpy(this->addr, addr, addr_size);

    socket = zmq_socket(context.get_handle(), static_cast<int>(socket_type));
    zmq_proto_assert(socket != NULL);

    {
      int rc;

      switch(socket_type)
      {
        case ZMQ_PROTO_REP:
        case ZMQ_PROTO_PUB:
          rc = zmq_bind(socket, addr);
          break;
        case ZMQ_PROTO_REQ:
          rc = zmq_connect(socket, addr);
          break;
        case ZMQ_PROTO_SUB:
        {
          {
            int opt = 1;
            rc = zmq_setsockopt(socket, ZMQ_CONFLATE, &opt, sizeof(opt));
          }
          zmq_proto_assert(rc == 0);
          rc = zmq_connect(socket, addr);
          zmq_proto_assert(rc == 0);
          rc = zmq_setsockopt(socket, ZMQ_SUBSCRIBE, "", 0);
          break;
        }
      }

      zmq_proto_assert(rc == 0);
    }

    return 0;
  }

  void* get_handle() const
  {
    return socket;
  }

  ~zmq_proto_socket()
  {
    switch(socket_type)
    {
      case ZMQ_PROTO_REP:
      case ZMQ_PROTO_PUB:
        break;
      case ZMQ_PROTO_REQ:
      case ZMQ_PROTO_SUB:
      {
        int rc = zmq_disconnect(socket, addr);
        zmq_proto_assert(rc == 0);
        break;
      }
    }
    free(addr);

    rc = zmq_close(socket);
    zmq_proto_assert(rc == 0);

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
      zmq_proto_assert (rc == 0);
    }
    else
    {
      zmq_proto_assert(false); // not implemented
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
    zmq_proto_assert(rc == 0);
  }

};

enum zmq_proto_send_flags { ZMQ_PROTO_NONE = 0, ZMQ_PROTO_DONTWAIT = ZMQ_DONTWAIT, ZMQ_PROTO_SNDMORE = ZMQ_SNDMORE };

template<zmq_proto_socket_type socket_type, typename Message>
std::enable_if_t<std::is_base_of<::google::protobuf::Message, Message>::value, int>
zmq_proto_send_raw(const zmq_proto_socket<socket_type>& socket, const Message& msg, zmq_proto_send_flags flags = ZMQ_PROTO_NONE)
{
  const int ser_size = msg.ByteSizeLong();
  char ser[ser_size];
  msg.SerializeToArray(ser, msg.ByteSizeLong());

  int sent = zmq_send(socket.get_handle(), ser, ser_size, static_cast<int>(flags));

  if (sent < 0)
  {
    zmq_proto_assert(zmq_errno() == EAGAIN);
    sent = -1;
  }

  return sent;
}

template<zmq_proto_socket_type socket_type, typename Message>
std::enable_if_t<std::is_base_of<::google::protobuf::Message, Message>::value, int>
zmq_proto_send(const zmq_proto_socket<socket_type>& socket, const Message& msg, zmq_proto_send_flags flags = ZMQ_PROTO_NONE)
{
  google::protobuf::Any req;
  req.PackFrom(msg);

  return zmq_proto_send_raw(socket, req, flags);
}

template<zmq_proto_socket_type socket_type, typename Message>
std::enable_if_t<std::is_base_of<::google::protobuf::Message, Message>::value, int>
zmq_proto_recv_raw(const zmq_proto_socket<socket_type>& socket, Message& p_msg)
{
  zmq_proto_msg msg{-1};

  int received = zmq_msg_recv(msg.get_handle(), socket.get_handle(), 0);

  if (received < 0)
  {
    zmq_proto_assert(zmq_errno() == EAGAIN);
    received = -1;
  }
  else
  {
    zmq_proto_assert(zmq_msg_size(msg.get_handle()) == static_cast<size_t>(received));

    p_msg.ParseFromArray(zmq_msg_data(msg.get_handle()), zmq_msg_size(msg.get_handle()));
  }

  return received;
}

template<zmq_proto_socket_type socket_type, typename Message>
std::enable_if_t<std::is_base_of<::google::protobuf::Message, Message>::value, int>
zmq_proto_recv(const zmq_proto_socket<socket_type>& socket, Message& p_msg)
{
  google::protobuf::Any any;
  int received = zmq_proto_recv_raw(socket, any);
  if (received >= 0)
  {
    if (any.Is<Message>())
      any.UnpackTo(&p_msg);
    else
      received = 0;
  }
  else
    received = -1;

  return received;
}
