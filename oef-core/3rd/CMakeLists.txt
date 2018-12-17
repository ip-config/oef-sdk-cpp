# Distributed under the MIT License (See accompanying file /LICENSE )

# CMake build : third party

#configure directories
set (THIRD_PARTY_MODULE_PATH "${PROJECT_SOURCE_DIR}/3rd")

# -------------------------------------------------------------------------
# catch

#configure directories
set (CATCH_MODULE_PATH "${THIRD_PARTY_MODULE_PATH}/Catch")
set (CATCH_INCLUDE_PATH "${CATCH_MODULE_PATH}/include")

#include custom cmake function
include ( "${CATCH_MODULE_PATH}/contrib/ParseAndAddCatchTests.cmake")

# -------------------------------------------------------------------------
# asio

#configure directories
set (ASIO_MODULE_PATH "${THIRD_PARTY_MODULE_PATH}/asio")
set (ASIO_INCLUDE_PATH "${THIRD_PARTY_MODULE_PATH}/asio")

# -------------------------------------------------------------------------
# hayai

#configure directories
set (HAYAI_MODULE_PATH "${THIRD_PARTY_MODULE_PATH}/hayai")
set (HAYAI_INCLUDE_PATH "${THIRD_PARTY_MODULE_PATH}/hayai")

# -------------------------------------------------------------------------
# mapbox variant

#configure directories
set (VARIANT_MODULE_PATH "${THIRD_PARTY_MODULE_PATH}/mapbox")
set (VARIANT_INCLUDE_PATH "${THIRD_PARTY_MODULE_PATH}/")

# -------------------------------------------------------------------------
# spdlog

#configure directories
set (SPDLOG_MODULE_PATH "${THIRD_PARTY_MODULE_PATH}/spdlog")
set (SPDLOG_INCLUDE_PATH "${THIRD_PARTY_MODULE_PATH}/")

# -------------------------------------------------------------------------
# clara

#configure directories
set (CLARA_MODULE_PATH "${THIRD_PARTY_MODULE_PATH}/Clara")
set (CLARA_INCLUDE_PATH "${CLARA_MODULE_PATH}/include")

# -------------------------------------------------------------------------

#set variables
set (THIRD_PARTY_INCLUDE_PATH ${VARIANT_INCLUDE_PATH} ${ASIO_INCLUDE_PATH} ${CLARA_INCLUDE_PATH} ${SPDLOG_INCLUDE_PATH})

#set variables for tests
set (TEST_THIRD_PARTY_INCLUDE_PATH  ${CATCH_INCLUDE_PATH} ${HAYAI_INCLUDE_PATH})

#export vars
set (THIRD_PARTY_INCLUDE_PATH  ${THIRD_PARTY_INCLUDE_PATH} PARENT_SCOPE)
set (TEST_THIRD_PARTY_INCLUDE_PATH  ${TEST_THIRD_PARTY_INCLUDE_PATH} PARENT_SCOPE)
