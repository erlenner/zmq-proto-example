cmake_minimum_required(VERSION 2.8.3)
project(server)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -Wall")

find_package(Protobuf REQUIRED)
include_directories(${Protobuf_INCLUDE_DIR})

file(GLOB ProtoFiles "${CMAKE_CURRENT_SOURCE_DIR}/*.proto")
PROTOBUF_GENERATE_CPP(ProtoSources ProtoHeaders ${ProtoFiles})
add_library(protos STATIC ${ProtoSources} ${ProtoHeaders})
include_directories("${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(protos ${PROTOBUF_LIBRARY})

# zmq
find_package(PkgConfig)
pkg_check_modules(ZMQ REQUIRED libzmq)

add_executable(zmq-server
  server.cpp
)
target_link_libraries(zmq-server
  ${ZMQ_LIBRARIES}
  protos
)
install(TARGETS zmq-server DESTINATION bin)

add_executable(zmq-client
  client.cpp
)
target_link_libraries(zmq-client
  ${ZMQ_LIBRARIES}
  protos
)
install(TARGETS zmq-client DESTINATION bin)


add_executable(zmq-publisher
  publisher.cpp
)
target_link_libraries(zmq-publisher
  ${ZMQ_LIBRARIES}
  protos
)
install(TARGETS zmq-publisher DESTINATION bin)

add_executable(zmq-subscriber
  subscriber.cpp
)
target_link_libraries(zmq-subscriber
  ${ZMQ_LIBRARIES}
  protos
)
install(TARGETS zmq-subscriber DESTINATION bin)
