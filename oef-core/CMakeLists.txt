################################################################################
# F E T C H   O E F   C O R E   P R O J E C T
################################################################################
cmake_minimum_required(VERSION 3.2)

set(PROJECT_NAME oef-core)
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
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g ")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftest-coverage")
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

add_subdirectory(3rd EXCLUDE_FROM_ALL)
set(THIRD_PARTY_INCLUDE_PATH ${THIRD_PARTY_INCLUDE_PATH} PARENT_SCOPE)
set(TEST_THIRD_PARTY_INCLUDE_PATH ${TEST_THIRD_PARTY_INCLUDE_PATH} PARENT_SCOPE)
include_directories(${THIRD_PARTY_INCLUDE_PATH} ${Protobuf_INCLUDE_DIRS})

#add_subdirectory(proto)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/lib/)
add_subdirectory(lib)
include_directories(${LIBRARY_INCLUDE_PATH})
message("oef core: ${LIBRARY_INCLUDE_PATH}")
set(OEF_CORE_LIBRARY_INCLUDE_PATH "${LIBRARY_INCLUDE_PATH}" PARENT_SCOPE)
set(OEF_CORE_LIBRARY_NAME "${LIB_NAME}" PARENT_SCOPE)

message("PROTOBUF = ${PROTOBUF_LIBRARIES}")

add_subdirectory(apps)

enable_testing()
