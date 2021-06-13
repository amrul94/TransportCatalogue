cmake_minimum_required(VERSION 3.17)
project(Transport_catalogue)

set(CMAKE_CXX_STANDARD 17)

find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS transport_catalogue.proto map_renderer.proto svg.proto graph.proto
        transport_router.proto)

set(TRANSPORT_CATALOGUE_FILES main.cpp domain.h domain.cpp geo.h geo.cpp json.h json.cpp
        json_reader.cpp json_reader.h request_handler.h request_handler.cpp transport_catalogue.h transport_catalogue.cpp
        request_handler.cpp map_renderer.h map_renderer.cpp svg.h svg.cpp tests.h json_builder.h json_builder.cpp graph.h
        ranges.h router.h transport_router.h transport_router.cpp log_duration.h serialization.h serialization.cpp)

add_executable(transport_catalogue ${PROTO_SRCS} ${PROTO_HDRS} ${TRANSPORT_CATALOGUE_FILES})

target_include_directories(transport_catalogue PUBLIC ${Protobuf_INCLUDE_DIRS})
target_include_directories(transport_catalogue PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

string(REPLACE "protobuf.lib" "protobufd.lib" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")
string(REPLACE "protobuf.a" "protobufd.a" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")

target_link_libraries(transport_catalogue "$<IF:$<CONFIG:Debug>,${Protobuf_LIBRARY_DEBUG},${Protobuf_LIBRARY}>" Threads::Threads)