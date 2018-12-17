################################################################################
# F E T C H   O E F   C O R E   B E N C H M A R K S
################################################################################
# CMake build : library benchmarks

#configure variables
set (TEST_APP_NAME "${LIB_NAME}Benchmark")

#configure directories
set (TEST_MODULE_PATH "${LIBRARY_MODULE_PATH}/benchmark")

#configure test directories
set (TEST_SRC_PATH  "${TEST_MODULE_PATH}/src" )

#set includes
include_directories (${LIBRARY_INCLUDE_PATH} ${TEST_THIRD_PARTY_INCLUDE_PATH})

#set test sources
file (GLOB TEST_SOURCE_FILES "${TEST_SRC_PATH}/*.cpp")

#set target executable
add_executable (${TEST_APP_NAME} ${TEST_SOURCE_FILES})

#add the library
target_link_libraries (${TEST_APP_NAME} ${LIB_NAME} ${PROTOBUF_LIBRARIES} Threads::Threads)

# Turn on CMake testing capabilities
#enable_testing()

#parse catch tests
#ParseAndAddCatchTests (${TEST_APP_NAME})
