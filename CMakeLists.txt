# Copyright MediaZ Teknoloji A.S. All Rights Reserved.

# Softcam (To create virtual camera on Windows)
if (WIN32)    
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/External/softcam ${CMAKE_CURRENT_BINARY_DIR}/External/softcam)

    target_compile_definitions(${NOS_PLUGIN_TARGET} PRIVATE -DNOMINMAX)
    nos_get_targets(NOSWEBCAM_EXTERNAL_TARGETS "${CMAKE_CURRENT_BINARY_DIR}/External/softcam")
    nos_group_targets("${NOSWEBCAM_EXTERNAL_TARGETS}" "External")
endif()

set(wmf_libs ole32.lib mf.lib mfuuid.lib mfreadwrite.lib Shlwapi.lib mfplat.lib)
list(APPEND DEPENDENCIES ${wmf_libs} softcamStatic)
list(APPEND INCLUDE_FOLDERS
    ${CMAKE_CURRENT_SOURCE_DIR}/External
    ${CMAKE_CURRENT_SOURCE_DIR}/External/softcam
    ${CMAKE_CURRENT_SOURCE_DIR}/External/softcam/src/softcam
    ${CMAKE_CURRENT_SOURCE_DIR}/External/softcam/src/softcamcore
)

target_include_directories(${NOS_PLUGIN_TARGET} PRIVATE "${INCLUDE_FOLDERS}")
target_link_libraries(${NOS_PLUGIN_TARGET} PRIVATE "${DEPENDENCIES}")
set_property(TARGET ${NOS_PLUGIN_TARGET}
    PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set_target_properties(${NOS_PLUGIN_TARGET} PROPERTIES CXX_STANDARD 23)