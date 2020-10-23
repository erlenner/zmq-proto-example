cmake_minimum_required(VERSION 2.8.3)
project(server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

## Find required protobuf package
#find_package(Protobuf REQUIRED)
#include_directories(${Protobuf_INCLUDE_DIR})

# zmq
find_package(PkgConfig)
pkg_check_modules(ZMQ REQUIRED libzmq)

add_executable(zmq-server
  server.cpp
)
target_link_libraries(zmq-server
  ${ZMQ_LIBRARIES}
)
install(TARGETS zmq-server DESTINATION bin)


add_executable(zmq-client
  client.cpp
)
target_link_libraries(zmq-client
  ${ZMQ_LIBRARIES}
)
install(TARGETS zmq-client DESTINATION bin)
