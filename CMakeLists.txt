################################################################################
# F E T C H   O E F   S D K   C P P   P R O J E C T
################################################################################
cmake_minimum_required(VERSION 3.2)

set(PROJECT_NAME oef-sdk)
project(${PROJECT_NAME} VERSION 0.5 LANGUAGES CXX)
add_compile_options(-std=c++1y)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
#set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "/usr/bin/iwyu")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -DASIO_STANDALONE")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -DASIO_STANDALONE -fsanitize=address")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -DASIO_STANDALONE -fsanitize=leak")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -DASIO_STANDALONE -fsanitize=thread")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -DASIO_STANDALONE -fsanitize=undefined")

find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)
include(GNUInstallDirs)

add_subdirectory(oef-core)
message("oef-core/3rd = ${THIRD_PARTY_INCLUDE_PATH}")
include_directories(${THIRD_PARTY_INCLUDE_PATH} ${Protobuf_INCLUDE_DIRS} ${OEF_CORE_LIBRARY_INCLUDE_PATH} ${CMAKE_CURRENT_BINARY_DIR}/oef-core/lib)
set(OEFCORE_LIB_PATH "oef-core/lib")

#add_subdirectory(3rd EXCLUDE_FROM_ALL)
#set(THIRD_PARTY_INCLUDE_PATH ${THIRD_PARTY_INCLUDE_PATH} PARENT_SCOPE)
#include_directories(${THIRD_PARTY_INCLUDE_PATH} ${Protobuf_INCLUDE_DIRS})

add_subdirectory(lib)
include_directories(lib/include)

message("PROTOBUF = ${PROTOBUF_LIBRARIES}")

add_subdirectory(apps)

enable_testing()
