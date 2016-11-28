cmake_minimum_required(VERSION 3.5)
project(jni_monitor)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
include_directories("/usr/lib/jvm/java-1.7.0-openjdk-1.7.0.101-2.6.6.1.el7_2.x86_64/include/")
include_directories("/usr/lib/jvm/java-1.7.0-openjdk-1.7.0.101-2.6.6.1.el7_2.x86_64/include/linux")
include_directories("../")

link_directories("../")

set(SOURCE_FILES  jni_monitor.h jni_monitor.c)
add_library(jni_monitor SHARED ${SOURCE_FILES})
target_link_libraries(jni_monitor monitor)