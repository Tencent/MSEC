cmake_minimum_required(VERSION 3.5)
project(jni)

set(PROTOBUF_PATH  "../../../../third_party/protobuf/protobuf-bin/")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
include_directories("../src")
include_directories("$ENV{JAVA_HOME}/include/")
include_directories("$ENV{JAVA_HOME}/include/linux")
include_directories("${PROTOBUF_PATH}/include")
link_directories("../src")
link_directories("../lib")
link_directories("${PROTOBUF_PATH}/lib")




set(SOURCE_FILES   jni_log.h jni_log.cpp)
add_library(jni_log SHARED ${SOURCE_FILES})
target_link_libraries(jni_log  logsys_api protobuf)

