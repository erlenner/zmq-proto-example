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

namespace zmq_proto
{

class context_t
{
  void *context;

public:

  context_t()
  : context(NULL)
  {}

  context_t(int n_threads)
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

  ~context_t()
  {
    int rc;
    do {
      rc = zmq_ctx_destroy(context);
    } while (rc == -1 && zmq_errno() == EINTR);

    zmq_proto_assert(rc == 0);
    context = NULL;
  }
};

enum socket_type{ REQ = ZMQ_REQ, REP = ZMQ_REP, PUB = ZMQ_PUB, SUB = ZMQ_SUB };

template<socket_type socket_type>
class socket_t
{
  void *socket;

public:

  socket_t()
  : socket(NULL)
  {}

  socket_t(const context_t& context, const char *addr)
  {
    int rc = init(context, addr);
    zmq_proto_assert(rc == 0);
  }

  int init(const context_t &context, const char *addr)
  {
    socket = zmq_socket(context.get_handle(), static_cast<int>(socket_type));
    zmq_proto_assert(socket != NULL);

    {
      int rc;

      switch(socket_type)
      {
        case REP:
        case PUB:
          rc = zmq_bind(socket, addr);
          break;
        case REQ:
          rc = zmq_connect(socket, addr);
          break;
        case SUB:
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

  ~socket_t()
  {
    int rc = zmq_close(socket);
    zmq_proto_assert(rc == 0);

    socket = NULL;
  }
};


class msg_t
{
  zmq_msg_t msg;

public:

  msg_t()
  {}

  msg_t(int size)
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

  ~msg_t()
  {
    int rc = zmq_msg_close(&msg);
    zmq_proto_assert(rc == 0);
  }

};

enum send_flags { NONE = 0, DONTWAIT = ZMQ_DONTWAIT, SNDMORE = ZMQ_SNDMORE };

template<socket_type socket_type, typename Message, int id_size>
std::enable_if_t<std::is_base_of<::google::protobuf::Message, Message>::value, int>
send_raw(const Message& msg, const socket_t<socket_type>& socket, const char(&id)[id_size], send_flags flags = NONE)
{
  // first id as null terminated raw string, then serialized protobuf message
  const int buf_size = id_size + msg.ByteSizeLong();
  char buf[buf_size];
  memcpy(buf, id, id_size);
  msg.SerializeToArray(buf + id_size, msg.ByteSizeLong());

  int sent = zmq_send(socket.get_handle(), buf, buf_size, static_cast<int>(flags));

  if (sent < 0)
  {
    zmq_proto_assert(zmq_errno() == EAGAIN);
    sent = -1;
  }

  return sent;
}

template<socket_type socket_type, typename Message, int id_size>
std::enable_if_t<std::is_base_of<::google::protobuf::Message, Message>::value, int>
send(const Message& msg, const socket_t<socket_type>& socket, const char(&id)[id_size], send_flags flags = NONE)
{
  google::protobuf::Any req;
  req.PackFrom(msg);

  return send_raw(req, socket, id, flags);
}

template<socket_type socket_type, typename Message, int id_size>
std::enable_if_t<std::is_base_of<::google::protobuf::Message, Message>::value, int>
recv_raw(Message& p_msg, const socket_t<socket_type>& socket, const char(&id)[id_size])
{
  msg_t msg{-1};

  int received = zmq_msg_recv(msg.get_handle(), socket.get_handle(), 0);

  if (received < 0)
  {
    zmq_proto_assert(zmq_errno() == EAGAIN);
    received = -1;
  }
  else
  {
    zmq_proto_assert(zmq_msg_size(msg.get_handle()) == static_cast<size_t>(received));

    const char* buf = (char*)zmq_msg_data(msg.get_handle());

    if ((strncmp(buf, id, id_size) != 0)
      || !p_msg.ParseFromArray(buf + id_size, zmq_msg_size(msg.get_handle()) - id_size))
    {
      received = -1;
    }
  }

  return received;
}

template<socket_type socket_type, typename Message, int id_size>
std::enable_if_t<std::is_base_of<::google::protobuf::Message, Message>::value, int>
recv(Message& p_msg, const socket_t<socket_type>& socket, const char(&id)[id_size])
{
  google::protobuf::Any any;
  int received = recv_raw(any, socket, id);
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

}
