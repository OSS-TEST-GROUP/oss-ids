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

set(DDS_IDS_VEHICLE_IDL_FILES
    AdvancedVehicleState.idl
    DoorSwitch.idl
    DoorUnlockLockIndicator.idl
    autoware_vehicle_msgs/msg/ControlModeReport.idl
    autoware_vehicle_msgs/msg/GearReport.idl
    autoware_vehicle_msgs/msg/HazardLightsReport.idl
    autoware_vehicle_msgs/msg/SteeringReport.idl
    autoware_vehicle_msgs/msg/TurnIndicatorsReport.idl
    autoware_vehicle_msgs/msg/VelocityReport.idl
    builtin_interfaces/msg/Time.idl
    std_msgs/msg/Header.idl
    tier4_api_msgs/msg/DoorStatus.idl
    tier4_vehicle_msgs/msg/ActuationStatus.idl
    tier4_vehicle_msgs/msg/ActuationStatusStamped.idl
)

set(DDS_IDS_DETECTION_IDL_FILES
    detectionReport.idl
)

set(DDS_IDS_IDL_FILES
    ${DDS_IDS_VEHICLE_IDL_FILES}
    ${DDS_IDS_DETECTION_IDL_FILES}
)

set(DDS_IDS_BROKEN_TYPEOBJECT_IDL_FILES
    builtin_interfaces/msg/Time.idl
    std_msgs/msg/Header.idl
)

set(DDS_IDS_TYPEOBJECT_SHIM_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/type_object_support_shims.cxx"
)

set(DDS_IDS_FASTDDSGEN_STAMP_DIR "${CMAKE_BINARY_DIR}/idl_codegen")
file(MAKE_DIRECTORY "${DDS_IDS_FASTDDSGEN_STAMP_DIR}")

if(DEFINED FASTDDSGEN_EXECUTABLE AND FASTDDSGEN_EXECUTABLE)
    set(FASTDDSGEN_EXE "${FASTDDSGEN_EXECUTABLE}" CACHE FILEPATH "Path to fastddsgen executable" FORCE)
endif()

if(DEFINED FASTDDSGEN_EXE AND FASTDDSGEN_EXE MATCHES "-NOTFOUND$")
    unset(FASTDDSGEN_EXE CACHE)
endif()

if(NOT DEFINED FASTDDSGEN_EXE)
    find_program(FASTDDSGEN_EXE NAMES fastddsgen HINTS ${CMAKE_PROGRAM_PATH} PATHS ENV PATH)

    if(FASTDDSGEN_EXE)
        set(FASTDDSGEN_EXE "${FASTDDSGEN_EXE}" CACHE FILEPATH "Path to fastddsgen executable" FORCE)
    endif()
endif()

message(STATUS "FASTDDSGEN_EXE: ${FASTDDSGEN_EXE}")

if(NOT FASTDDSGEN_EXE)
    message(STATUS "fastddsgen not found; target 'generate_idl' is unavailable")
else()
    set(DDS_IDS_FASTDDSGEN_STAMPS)

    foreach(_idl ${DDS_IDS_IDL_FILES})
        get_filename_component(_base "${_idl}" NAME_WE)
        string(REPLACE "/" "_" _idl_key "${_idl}")
        set(_stamp "${DDS_IDS_FASTDDSGEN_STAMP_DIR}/${_idl_key}.stamp")
        add_custom_command(
            OUTPUT "${_stamp}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${DDS_IDS_TYPE_DIR}"
            COMMAND ${FASTDDSGEN_EXE} -I "${CMAKE_CURRENT_SOURCE_DIR}" -d "${DDS_IDS_TYPE_DIR}" "${_idl}"
            COMMAND ${CMAKE_COMMAND} -E touch "${_stamp}"
            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${_idl}" "${FASTDDSGEN_EXE}"
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            COMMENT "fastddsgen: Generating types from ${_idl}"
            VERBATIM
        )
        message(STATUS "add custom command: ${FASTDDSGEN_EXE} -I ${CMAKE_CURRENT_SOURCE_DIR} -d ${DDS_IDS_TYPE_DIR} ${_idl}")
        list(APPEND DDS_IDS_FASTDDSGEN_STAMPS "${_stamp}")
    endforeach()

    # Make IDL generation part of the normal build so types are produced when building the project
    add_custom_target(generate_idl ALL DEPENDS ${DDS_IDS_FASTDDSGEN_STAMPS})
endif()

function(dds_ids_collect_generated_sources out_var)
    set(_sources)
    foreach(_idl ${ARGN})
        get_filename_component(_base "${_idl}" NAME_WE)
        get_filename_component(_dir "${_idl}" DIRECTORY)
        if(_dir STREQUAL "")
            set(_generated_prefix "${DDS_IDS_TYPE_DIR}/${_base}")
        else()
            set(_generated_prefix "${DDS_IDS_TYPE_DIR}/${_dir}/${_base}")
        endif()
        list(APPEND _sources
            "${_generated_prefix}PubSubTypes.cxx"
        )
        list(FIND DDS_IDS_BROKEN_TYPEOBJECT_IDL_FILES "${_idl}" _skip_typeobject_index)
        if(_skip_typeobject_index EQUAL -1)
            list(APPEND _sources
                "${_generated_prefix}TypeObjectSupport.cxx"
            )
        endif()
    endforeach()
    set(${out_var} "${_sources}" PARENT_SCOPE)
endfunction()

dds_ids_collect_generated_sources(DDS_IDS_VEHICLE_TYPE_SOURCES ${DDS_IDS_VEHICLE_IDL_FILES})
list(APPEND DDS_IDS_VEHICLE_TYPE_SOURCES ${DDS_IDS_TYPEOBJECT_SHIM_SOURCE})
dds_ids_collect_generated_sources(DDS_IDS_DETECTION_TYPE_SOURCES ${DDS_IDS_DETECTION_IDL_FILES})

# Mark generated sources so CMake won't expect them to exist at configure time
if(NOT "${DDS_IDS_VEHICLE_TYPE_SOURCES}" STREQUAL "")
    set_source_files_properties(${DDS_IDS_VEHICLE_TYPE_SOURCES} PROPERTIES GENERATED TRUE)
endif()

if(NOT "${DDS_IDS_DETECTION_TYPE_SOURCES}" STREQUAL "")
    set_source_files_properties(${DDS_IDS_DETECTION_TYPE_SOURCES} PROPERTIES GENERATED TRUE)
endif()

add_library(dds_ids_vehicle_types STATIC ${DDS_IDS_VEHICLE_TYPE_SOURCES})
target_include_directories(dds_ids_vehicle_types PUBLIC "${DDS_IDS_TYPE_DIR}")
target_link_libraries(dds_ids_vehicle_types PUBLIC ${DDS_IDS_RUNTIME_LIBS})
set_target_properties(dds_ids_vehicle_types PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS YES
)

add_library(dds_ids_detection_types STATIC ${DDS_IDS_DETECTION_TYPE_SOURCES})
target_include_directories(dds_ids_detection_types PUBLIC "${DDS_IDS_TYPE_DIR}")
target_link_libraries(dds_ids_detection_types PUBLIC ${DDS_IDS_RUNTIME_LIBS})
set_target_properties(dds_ids_detection_types PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS YES
)

if(TARGET generate_idl)
    add_dependencies(dds_ids_vehicle_types generate_idl)
    add_dependencies(dds_ids_detection_types generate_idl)
endif()
