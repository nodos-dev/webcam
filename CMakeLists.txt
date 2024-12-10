# Copyright MediaZ Teknoloji A.S. All Rights Reserved.
cmake_minimum_required(VERSION 3.24.2)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_CXX_STANDARD 20)

if (NOT WITH_NODOS_WORKSPACE)
    message(FATAL_ERROR "This repo currently does not support builds without Nodos workspace. "
    "Place this repo under nodos-workspace/Module folder and run cmake -S ./Toolchain/CMake -B Build from workspace root.")
endif()

nos_find_sdk("1.2.0" NOS_PLUGIN_SDK_TARGET NOS_SUBSYSTEM_SDK_TARGET NOS_SDK_DIR)
set(FLATC_EXECUTABLE ${NOS_SDK_DIR}/bin/flatc${CMAKE_EXECUTABLE_SUFFIX})

# Softcam (To create virtual camera on Windows)
if (WIN32) 
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/External/softcam "${CMAKE_CURRENT_BINARY_DIR}/softcamlibraries")

    nos_get_targets(NOSWEBCAM_EXTERNAL_TARGETS "./External/softcam")
    nos_group_targets("${NOSWEBCAM_EXTERNAL_TARGETS}" "External")
endif()

# Dependencies
nos_get_module("nos.sys.vulkan" "5.8" NOS_SYS_VULKAN_TARGET_5_8)

# Schemas
nos_generate_flatbuffers("${CMAKE_CURRENT_SOURCE_DIR}/Config" "${CMAKE_CURRENT_SOURCE_DIR}/Source" "cpp" "${NOS_SDK_DIR}/types" nosWebcam_generated)

set(wmf_libs ole32.lib mf.lib mfuuid.lib mfreadwrite.lib Shlwapi.lib mfplat.lib)
list(APPEND DEPENDENCIES ${NOS_SYS_VULKAN_TARGET_5_8} ${NOS_PLUGIN_SDK_TARGET} ${wmf_libs} nosWebcam_generated softcamStatic)
list(APPEND INCLUDE_FOLDERS
    ${EXTERNAL_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
    ${CMAKE_CURRENT_LIST_DIR}/External
    ${CMAKE_CURRENT_LIST_DIR}/External/softcam
    ${CMAKE_CURRENT_LIST_DIR}/External/softcam/src/softcam
    ${CMAKE_CURRENT_LIST_DIR}/External/softcam/src/softcamcore
)

nos_add_plugin("nosWebcam" "${DEPENDENCIES}" "${INCLUDE_FOLDERS}")
set_target_properties(nosWebcam PROPERTIES CXX_STANDARD 23)

# Project generation
nos_group_targets("nosWebcam" "NOS Plugins")