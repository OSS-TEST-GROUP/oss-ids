# Copyright (c) 2026 AUTOCRYPT Co., Ltd. (아우토크립트 주식회사)
# All rights reserved.
#
# [Terms of Use]
# 1. Modifications: Any modifications to this source code must be reported
# and shared with AUTOCRYPT Co., Ltd.
# (소스 코드 수정 시 아우토크립트 주식회사에 해당 내용을 반드시 고지해야 함)
# 2. Commercial Use: Prior written consultation and agreement with the
# Company are required for any commercial purposes.
# (상업적 이용 시 반드시 회사와 사전 협의 및 서면 합의 필요)
#
# For full details, please refer to the LICENSE.md file.

project(SecurityClient LANGUAGES CXX)

file(GLOB_RECURSE SECURITY_CLIENT_CPP_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/app/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/config/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/collection/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/alert/*.cpp"
)

set(SECURITY_CLIENT_EXECUTABLE_SOURCES
    ${SECURITY_CLIENT_CPP_SOURCES}
    "${CMAKE_CURRENT_SOURCE_DIR}/Main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/main_function.cpp"
)

add_executable(SecurityClient
    ${SECURITY_CLIENT_EXECUTABLE_SOURCES}
)

target_include_directories(SecurityClient
    PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_BINARY_DIR}"
)

target_link_libraries(SecurityClient
    PUBLIC
    fmt::fmt
    spdlog::spdlog
    cxxopts::cxxopts
    PRIVATE
    dds_ids_vehicle_types
    dds_ids_detection_types
    libdds-ids::common
    libdds-ids::analysis
)

set_target_properties(SecurityClient PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS YES
)

install(TARGETS SecurityClient RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if(BUILD_TESTING)
    file(GLOB_RECURSE SECURITY_CLIENT_TEST_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/gtest/*.cpp"
    )

    add_executable(SecurityClient_test
        ${SECURITY_CLIENT_CPP_SOURCES}
        ${SECURITY_CLIENT_TEST_SOURCES}
    )

    target_include_directories(SecurityClient_test
        PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/gtest"
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
        "${CMAKE_BINARY_DIR}"
    )

    target_link_libraries(SecurityClient_test
        PRIVATE
        GTest::gtest_main
        dds_ids_vehicle_types
        dds_ids_detection_types
        libdds-ids::common
        libdds-ids::analysis
    )

    set_target_properties(SecurityClient_test PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS YES
    )

    add_test(NAME SecurityClientTest COMMAND SecurityClient_test)
    set_tests_properties(SecurityClientTest PROPERTIES
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endif()
