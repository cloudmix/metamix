cmake_minimum_required(VERSION 3.10)
project(metamix VERSION 1.2.3)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules)


##############################################################################
## Compiler settings

set(
  WARN_FALGS

  -pedantic
  -Wall
  -Wextra
  -Weffc++
  -Wno-maybe-uninitialized
  -Wduplicated-cond
  -Wduplicated-branches
  -Wlogical-op
  -Wnull-dereference
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARN_FLAGS}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARN_FLAGS}")


##############################################################################
## Boost

set(Boost_USE_STATIC_LIBS ON)
find_package(
  Boost 1.66.0 REQUIRED COMPONENTS
  log
  program_options
  system
  unit_test_framework
)


##############################################################################
## FFmpeg

find_package(FFmpeg REQUIRED)


##############################################################################
## Third-party headers

include_directories(SYSTEM vendor)


##############################################################################
## Metamix core source

add_library(
  metamix-core-obj OBJECT

  src/abstract_input.cpp src/abstract_input.h
  src/application_context.cpp src/application_context.h
  src/binary_emitter_util.h
  src/binary_parser_util.h
  src/binary_parser.h
  src/byte_vector_io.h
  src/clear_input.cpp src/clear_input.h
  src/clock_types.h
  src/clock.cpp src/clock.h
  src/ffmpeg.cpp src/ffmpeg.h
  src/h264/av_packet_nalu_parser.h
  src/h264/av_packet_nalu.h
  src/h264/emitter.h
  src/h264/nalu_parser.cpp src/h264/nalu_parser.h
  src/h264/nalu.cpp src/h264/nalu.h
  src/h264/rbsp.h
  src/h264/sei_parser.cpp src/h264/sei_parser.h
  src/h264/sei_payload.cpp src/h264/sei_payload.h
  src/h264/stdseis.cpp src/h264/stdseis.h
  src/input_manager.h
  src/io/io_handle.cpp src/io/io_handle.h
  src/io/remux_loop.cpp src/io/remux_loop.h
  src/io/sink_handle.cpp src/io/sink_handle.h
  src/io/source_handle.cpp src/io/source_handle.h
  src/io/stream_classification.h
  src/iospec.h
  src/log.cpp src/log.h
  src/metadata_kind.h
  src/metadata_queue.h
  src/metadata.h
  src/optional_io.h
  src/proc/controller.cpp src/proc/controller.h
  src/proc/extractor.cpp src/proc/extractor.h
  src/proc/injector.cpp src/proc/injector.h
  src/program_options.cpp src/program_options.h
  src/scte35/crc32.cpp src/scte35/crc32.h
  src/scte35/emitter.h
  src/scte35/parser.cpp src/scte35/parser.h
  src/scte35/scte35.cpp src/scte35/scte35.h
  src/slice.h
  src/supervisor.h
  src/user_defined_input.cpp src/user_defined_input.h
  src/util.cpp src/util.h
  src/variant_io.h
  src/version.cpp src/version.h
)

configure_file(
  ${PROJECT_SOURCE_DIR}/src/config.h.in
  ${PROJECT_BINARY_DIR}/include.dir/src/config.h
)

target_include_directories(
  metamix-core-obj PUBLIC

  ${Boost_INCLUDE_DIRS}
  ${FFMPEG_INCLUDE_DIR}
  ${PROJECT_SOURCE_DIR}
  ${PROJECT_BINARY_DIR}/include.dir
)

add_library(metamix-core INTERFACE)
target_sources(metamix-core INTERFACE $<TARGET_OBJECTS:metamix-core-obj>)

target_link_libraries(
  metamix-core INTERFACE

  pthread
  Boost::log
  Boost::program_options
  Boost::system
  ${FFMPEG_LIBRARIES}
)


##############################################################################
## Metamix main executable

add_executable(metamix src/main.cpp)
target_link_libraries(metamix metamix-core)


##############################################################################
## Test suite

add_executable(
  metamix-tests

  test/main.cpp

  test/clock_test.cpp
  test/h264/nalu_test.cpp
  test/h264/rbsp_test.cpp
  test/metadata_queue_test.cpp
  test/scte35/parser_emitter_test.cpp
  test/ts_ticker_test.cpp
)

target_include_directories(
  metamix-tests PUBLIC

  ${Boost_INCLUDE_DIRS}
  ${FFMPEG_INCLUDE_DIR}
  ${PROJECT_SOURCE_DIR}
  ${PROJECT_BINARY_DIR}/include.dir
)

target_link_libraries(
  metamix-tests

  pthread
  Boost::unit_test_framework
  metamix-core
)


##############################################################################
## Installer

install(TARGETS metamix DESTINATION bin)
install(PROGRAMS util/fbremix DESTINATION bin)

SET(CPACK_GENERATOR TGZ)

set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})

include(CPack)
