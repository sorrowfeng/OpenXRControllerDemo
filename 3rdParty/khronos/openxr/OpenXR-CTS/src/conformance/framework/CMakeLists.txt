# Copyright (c) 2019-2024, The Khronos Group Inc.
#
# SPDX-License-Identifier: Apache-2.0

# tinygltf
add_library(conformance_framework_tinygltf STATIC cts_tinygltf.cpp)

target_link_libraries(
        conformance_framework_tinygltf
        PRIVATE
        stb
)

target_include_directories(
        conformance_framework_tinygltf
        PRIVATE
        # 3rdParty
        ${PROJECT_SOURCE_DIR}/3rdParty
        # stb
        ${PROJECT_SOURCE_DIR}/3rdParty/stb
        # tinygltf impl needs bundled json.hpp
        ${PROJECT_SOURCE_DIR}/3rdParty/tinygltf
        # openxr
        "${PROJECT_SOURCE_DIR}/openxr/"
        "${PROJECT_SOURCE_DIR}/openxr/pico"
)

target_compile_definitions(
        conformance_framework_tinygltf
        PUBLIC TINYGLTF_USE_CPP14 TINYGLTF_NO_STB_IMAGE_WRITE
        # TODO once we have rapidjson to avoid stack overflow
        # RAPIDJSON_PARSE_DEFAULT_FLAGS=kParseIterativeFlag TINYGLTF_USE_RAPIDJSON
        # TINYGLTF_USE_RAPIDJSON_CRTALLOCATOR
)

if(SUPPORTS_Werrorunusedparameter)
    target_compile_options(
            conformance_framework_tinygltf PRIVATE -Wno-unused-parameter
    )
endif()
set_target_properties(
        conformance_framework_tinygltf PROPERTIES FOLDER
        ${CONFORMANCE_TESTS_FOLDER}
)

# PBR subsystem and glTF handling
add_subdirectory(pbr)
add_subdirectory(gltf)

add_library(
        conformance_framework
        STATIC
        RGBAImage.cpp
)

target_include_directories(
        conformance_framework
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        # 3rdParty
        ${PROJECT_SOURCE_DIR}/3rdParty
        # framework
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        # src
        ${CMAKE_CURRENT_SOURCE_DIR}/../../
        # openxr
        "${PROJECT_SOURCE_DIR}/openxr/"
        "${PROJECT_SOURCE_DIR}/openxr/pico"
)

target_link_libraries(
        conformance_framework
        PUBLIC
        conformance_framework_gltf
        conformance_framework_tinygltf
#        conformance_utilities
)

if(GLSLANG_VALIDATOR AND NOT GLSL_COMPILER)
    target_compile_definitions(
            conformance_framework PUBLIC USE_GLSLANGVALIDATOR
    )
endif()

set_target_properties(
        conformance_framework PROPERTIES FOLDER ${CONFORMANCE_TESTS_FOLDER}
)
