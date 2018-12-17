################################################################################
# F E T C H  C P P   A G E N T S   H E R D
################################################################################
# CMake build : main application

#configure variables
set (APP_NAME "ClientsHerd")

#configure directories
set (APP_MODULE_PATH "${PROJECT_SOURCE_DIR}/apps/clientsherd")
set (APP_SRC_PATH  "${APP_MODULE_PATH}/src" )

#set includes
include_directories (${LIBRARY_INCLUDE_PATH} ${THIRD_PARTY_INCLUDE_PATH} ${APP_SRC_PATH})

#set sources
#file (GLOB APP_SOURCE_FILES "${APP_SRC_PATH}/ants.cpp")

#set target executable
add_executable (${APP_NAME} "${APP_SRC_PATH}/main.cpp")

#add the library
target_link_libraries (${APP_NAME} ${LIB_NAME} ${OEF_CORE_LIBRARY_NAME} ${PROTOBUF_LIBRARIES} Threads::Threads)

#test
#test
#configure variables
#set (TEST_APP_NAME "${APP_NAME}Test")

#configure directories
#set (TEST_MODULE_PATH "${PROJECT_SOURCE_DIR}/apps/ants")

#configure test directories
#set (TEST_SRC_PATH  "${TEST_MODULE_PATH}/test" )

#set includes
#include_directories (${LIBRARY_INCLUDE_PATH} ${TEST_THIRD_PARTY_INCLUDE_PATH} ${APP_SRC_PATH} ${TEST_SRC_PATH})

#set test sources
#file (GLOB TEST_SOURCE_FILES "${TEST_SRC_PATH}/*.cpp")

#set target executable
#add_executable (${TEST_APP_NAME} ${TEST_SOURCE_FILES} ${APP_SOURCE_FILES})

#add the library
#target_link_libraries (${TEST_APP_NAME} ${LIB_NAME} Threads::Threads)

#enable_testing ()

#parse catch tests
#ParseAndAddCatchTests (${TEST_APP_NAME})

