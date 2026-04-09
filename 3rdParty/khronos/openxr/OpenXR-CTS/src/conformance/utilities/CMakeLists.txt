# Copyright (c) 2019-2024, The Khronos Group Inc.
#
# SPDX-License-Identifier: Apache-2.0

add_library(
        conformance_utilities STATIC
        image.cpp
        opengl_utils.cpp
        stringification.cpp
        throw_helpers.cpp
        utils.cpp
)

target_include_directories(
        conformance_utilities
        PUBLIC
        ${PROJECT_SOURCE_DIR}/3rdParty/span-lite/include
        # openxr
        "${PROJECT_SOURCE_DIR}/openxr/"
        "${PROJECT_SOURCE_DIR}/openxr/pico"
)

target_link_libraries(
        conformance_utilities PRIVATE
        conformance_framework_basisu
)

if(TARGET openxr-gfxwrapper)
    target_link_libraries(conformance_utilities PRIVATE openxr-gfxwrapper)
endif()

if(GLSLANG_VALIDATOR AND NOT GLSL_COMPILER)
    target_compile_definitions(
            conformance_utilities PUBLIC USE_GLSLANGVALIDATOR
    )
endif()

if(XR_USE_GRAPHICS_API_VULKAN)
    target_include_directories(
            conformance_utilities PUBLIC ${Vulkan_INCLUDE_DIRS}
    )
    target_link_libraries(conformance_utilities PUBLIC ${Vulkan_LIBRARY})
endif()

set_target_properties(
        conformance_utilities PROPERTIES FOLDER ${CONFORMANCE_TESTS_FOLDER}
)
