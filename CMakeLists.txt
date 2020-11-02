cmake_minimum_required(VERSION 2.8.3)
project(server)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -Wall")

find_package(Protobuf REQUIRED)
include_directories(${Protobuf_INCLUDE_DIR})

function(PROTOBUF_GENERATE_CPP_CUSTOM SRCS HDRS)
  # lifted from https://github.com/Kitware/CMake/blob/v3.6.3/Modules/FindProtobuf.cmake#L112
  if(NOT ARGN)
    message(SEND_ERROR "Error: PROTOBUF_GENERATE_CPP_CUSTOM called without any proto files")
    return()
  endif()

  set(_protobuf_include_path -I ${CMAKE_CURRENT_SOURCE_DIR})

  if(DEFINED PROTOBUF_IMPORT_DIRS AND NOT DEFINED Protobuf_IMPORT_DIRS)
    set(Protobuf_IMPORT_DIRS "${PROTOBUF_IMPORT_DIRS}")
  endif()

  if(DEFINED Protobuf_IMPORT_DIRS)
    foreach(DIR ${Protobuf_IMPORT_DIRS})
      get_filename_component(ABS_PATH ${DIR} ABSOLUTE)
      list(FIND _protobuf_include_path ${ABS_PATH} _contains_already)
      if(${_contains_already} EQUAL -1)
          list(APPEND _protobuf_include_path -I ${ABS_PATH})
      endif()
    endforeach()
  endif()

  set(${SRCS})
  set(${HDRS})
  foreach(FIL ${ARGN})
    get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
    get_filename_component(FIL_WE ${FIL} NAME_WE)

    list(APPEND ${SRCS} "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb.cc")
    list(APPEND ${HDRS} "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb.h")

    add_custom_command(
      OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb.cc"
             "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb.h"
      COMMAND  ${Protobuf_PROTOC_EXECUTABLE}
      ARGS --cpp_out  ${CMAKE_CURRENT_BINARY_DIR} ${_protobuf_include_path} ${ABS_FIL}
      DEPENDS ${ABS_FIL} ${Protobuf_PROTOC_EXECUTABLE}
      COMMENT "Running C++ protocol buffer compiler on ${FIL}"
      VERBATIM )
  endforeach()

  set_source_files_properties(${${SRCS}} ${${HDRS}} PROPERTIES GENERATED TRUE)
  set(${SRCS} ${${SRCS}} PARENT_SCOPE)
  set(${HDRS} ${${HDRS}} PARENT_SCOPE)
endfunction()

file(GLOB ProtoFiles "${CMAKE_CURRENT_SOURCE_DIR}/*.proto")
PROTOBUF_GENERATE_CPP_CUSTOM(ProtoSources ProtoHeaders ${ProtoFiles})
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
