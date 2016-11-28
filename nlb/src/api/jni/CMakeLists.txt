cmake_minimum_required(VERSION 3.5)
project(libjni_lb)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
include_directories("$ENV{JAVA_HOME}/include")
include_directories("$ENV{JAVA_HOME}/include/linux")
include_directories("../../comm")
include_directories("../")

link_directories("../")
link_directories("../../comm")

set(SOURCE_FILES jni_lb.c jni_lb.h)

add_library(jni_lb SHARED ${SOURCE_FILES})
target_link_libraries(jni_lb  nlbapi comm)
