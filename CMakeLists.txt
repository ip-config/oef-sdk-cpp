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

#code coverage
if (ENABLE_COVERAGE)
    message("-- Code coverage enabled")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()

if (ENABLE_COVERAGE_LLVM)
    message("-- Code coverage enabled for LLVM")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-instr-generate")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcoverage-mapping")
endif()

find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)
include(GNUInstallDirs)

add_subdirectory(oef-core)
set(THIRD_PARTY_INCLUDE_PATH ${THIRD_PARTY_INCLUDE_PATH} PARENT_SCOPE)
set(TEST_THIRD_PARTY_INCLUDE_PATH ${TEST_THIRD_PARTY_INCLUDE_PATH} PARENT_SCOPE)
message("oef-core/3rd = ${THIRD_PARTY_INCLUDE_PATH}")
include_directories(${THIRD_PARTY_INCLUDE_PATH} ${Protobuf_INCLUDE_DIRS} ${OEF_CORE_LIBRARY_INCLUDE_PATH} ${CMAKE_CURRENT_BINARY_DIR}/oef-core/lib)
set(OEF_CORE_LIBRARY_INCLUDE_PATH "${LIBRARY_INCLUDE_PATH}" PARENT_SCOPE)
set(OEF_CORE_LIBRARY_NAME "${LIB_NAME}" PARENT_SCOPE)
set(OEFCORE_LIB_PATH "oef-core/lib")

#add_subdirectory(3rd EXCLUDE_FROM_ALL)
#set(THIRD_PARTY_INCLUDE_PATH ${THIRD_PARTY_INCLUDE_PATH} PARENT_SCOPE)
#include_directories(${THIRD_PARTY_INCLUDE_PATH} ${Protobuf_INCLUDE_DIRS})

add_subdirectory(lib)
add_dependencies("${PROJECT_NAME}Lib" oef-core)
include_directories(lib/include)

message("PROTOBUF = ${PROTOBUF_LIBRARIES}")

add_subdirectory(apps)

enable_testing()
