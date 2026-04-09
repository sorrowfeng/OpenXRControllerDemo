# Copyright (c) 2019-2024, The Khronos Group Inc.
#
# SPDX-License-Identifier: Apache-2.0

add_library(
        conformance_framework_pbr STATIC
        PbrCommon.cpp
        GltfLoader.cpp
        PbrMaterial.cpp
        PbrModel.cpp
        PbrSharedState.cpp
        PbrTexture.cpp
)

set_target_properties(
        conformance_framework_pbr PROPERTIES FOLDER ${CONFORMANCE_TESTS_FOLDER}
)

target_link_libraries(
        conformance_framework_pbr
        PUBLIC conformance_framework_tinygltf conformance_framework_gltf
        conformance_utilities
        PRIVATE
        stb
)

if(MSVC)
    # Turns off ABI compatibility warning
    target_compile_definitions(
            conformance_framework_pbr PUBLIC _ENABLE_EXTENDED_ALIGNED_STORAGE
    )
endif()

if(XR_USE_GRAPHICS_API_OPENGL OR XR_USE_GRAPHICS_API_OPENGL_ES)
#    include(make_includable)

    make_includable(
            "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/PbrVertexShader_glsl.vert"
            "${CMAKE_CURRENT_BINARY_DIR}/PbrVertexShader_glsl_src.h"
    )
    make_includable(
            "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/PbrPixelShader_glsl.frag"
            "${CMAKE_CURRENT_BINARY_DIR}/PbrPixelShader_glsl_src.h"
    )

    set(from_gl "^#version [0-9]+")
    set(to_gles "#version 320 es")
    make_includable(
            "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/PbrVertexShader_glsl.vert"
            "${CMAKE_CURRENT_BINARY_DIR}/PbrVertexShader_glsl_src_es.h"
            REPLACE
            "${from_gl}"
            "${to_gles}"
    )
    make_includable(
            "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/PbrPixelShader_glsl.frag"
            "${CMAKE_CURRENT_BINARY_DIR}/PbrPixelShader_glsl_src_es.h"
            REPLACE
            "${from_gl}"
            "${to_gles}"
    )

    target_sources(
            conformance_framework_pbr
            PRIVATE
            "${CMAKE_CURRENT_BINARY_DIR}/PbrPixelShader_glsl_src.h"
            "${CMAKE_CURRENT_BINARY_DIR}/PbrVertexShader_glsl_src.h"
            OpenGL/GLResources.cpp
            OpenGL/GLFormats.cpp
            OpenGL/GLTexture.cpp
            OpenGL/GLTextureCache.cpp
            OpenGL/GLMaterial.cpp
            OpenGL/GLModel.cpp
            OpenGL/GLPrimitive.cpp
    )
endif()

if(TARGET openxr-gfxwrapper)
    target_link_libraries(conformance_framework_pbr PRIVATE openxr-gfxwrapper)
endif()

if(XR_USE_GRAPHICS_API_VULKAN)
    target_include_directories(
            conformance_framework_pbr PRIVATE ${Vulkan_INCLUDE_DIRS}
    )
    target_link_libraries(conformance_framework_pbr PRIVATE ${Vulkan_LIBRARY})
endif()

if(XR_USE_GRAPHICS_API_VULKAN)
#    include(glsl_shader)

    glsl_spv_shader(
            INPUT
            "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/PbrVertexShader_glsl.vert"
            OUTPUT
            "${CMAKE_CURRENT_BINARY_DIR}/PbrVertexShader_glsl_spv.h"
            STAGE
            vert
            VARIABLE
            g_PbrVertexShader_vulkan
            TARGET_ENV
            vulkan1.0
    )
    glsl_spv_shader(
            INPUT
            "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/PbrPixelShader_glsl.frag"
            OUTPUT
            "${CMAKE_CURRENT_BINARY_DIR}/PbrPixelShader_glsl_spv.h"
            STAGE
            frag
            VARIABLE
            g_PbrPixelShader_vulkan
            TARGET_ENV
            vulkan1.0
    )

    if(GLSLANG_VALIDATOR AND NOT GLSL_COMPILER)
        target_compile_definitions(
                conformance_framework_pbr PRIVATE USE_GLSLANGVALIDATOR
        )
    endif()

    target_sources(
            conformance_framework_pbr
            PRIVATE
            "${CMAKE_CURRENT_BINARY_DIR}/PbrPixelShader_glsl_spv.h"
            "${CMAKE_CURRENT_BINARY_DIR}/PbrVertexShader_glsl_spv.h"
            Vulkan/VkPipelineStates.cpp
            Vulkan/VkResources.cpp
            Vulkan/VkFormats.cpp
            Vulkan/VkTexture.cpp
            Vulkan/VkTextureCache.cpp
            Vulkan/VkMaterial.cpp
            Vulkan/VkModel.cpp
            Vulkan/VkPrimitive.cpp
    )
    if(GLSLANG_VALIDATOR)
        # If we can, run this check of shader structure offsets in the unit test suite.
        add_test(
                NAME pbr_glsl_offsets
                COMMAND
                "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/check-offsets.py"
                "${GLSLANG_VALIDATOR}"
        )
    endif()
endif()

target_include_directories(
        conformance_framework_pbr
        PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}
        ${PROJECT_SOURCE_DIR}/3rdParty/khronos/openxr/OpenXR-CTS/src/common
#        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../src/common
#        "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../src/conformance"
        # Strong types for integers, etc.
        ${PROJECT_SOURCE_DIR}/3rdParty/type-lite/include
        # Backport span
        ${PROJECT_SOURCE_DIR}/3rdParty//span-lite/include
        # openxr
        ${PROJECT_SOURCE_DIR}/openxr
        ${PROJECT_SOURCE_DIR}/openxr/pico
        PRIVATE
        ${PROJECT_SOURCE_DIR}/3rdParty/stb
)
