
cmake_minimum_required(VERSION 3.10.2)


# openxr
if (CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a" OR ANDROID_ABI STREQUAL "arm64-v8a" OR
    CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
    set(LOADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../openxr/pico/loader/lib/arm64-v8a)
elseif (CMAKE_ANDROID_ARCH_ABI STREQUAL "armeabi-v7a" OR ANDROID_ABI STREQUAL "armeabi-v7a" OR
        CMAKE_SYSTEM_PROCESSOR STREQUAL "armv7-a" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "armv7l")
    set(LOADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../openxr/pico/loader/lib/armeabi-v7a)
else ()
    message(FATAL_ERROR "Unsupported architecture: ${CMAKE_ANDROID_ARCH_ABI} / ${CMAKE_SYSTEM_PROCESSOR}")
endif ()
find_library(OPENXR_LOADER NAMES openxr_loader
        PATHS ${LOADER_DIR}
        NO_DEFAULT_PATH
        NO_CMAKE_FIND_ROOT_PATH
        NO_SYSTEM_ENVIRONMENT_PATH
        NO_CMAKE_PATH
        NO_CMAKE_ENVIRONMENT_PATH
        NO_PACKAGE_ROOT_PATH
        NO_CMAKE_SYSTEM_PATH)

if (NOT OPENXR_LOADER)
    message(FATAL_ERROR "openxr_loader not found")
endif ()


add_library(app_glue STATIC ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

set(OPENXR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../openxr)
set(OPENXR_PICO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../openxr/pico)
set(3RDPARTY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../3rdParty)
set(GUI_PATH src/gui)
set(UTIL_PATH src/util)
set(MODEL_PATH src/model)
set(COLLISION_PATH src/model/collision)
set(EVENT_PATH src/model/event)
set(OBJECTS_PATH src/model/objects)
set(MODEL_GEOMETRY_PATH src/model/geometry)
set(GRAPHICS_PLUGIN_PATH src/graphicsPlugin)
set(PLATFORM_PLUGIN_PATH src/platformPlugin)
set(OPENXR_WRAPPER_PATH src/openxrWrapper)
set(OPENXR_EXTENSION_PATH src/openxrWrapper/extensions)


file(GLOB
        CPP_SOURCES
        src/AndroidOpenXrProgram.cpp
        src/ExceptionHandlerProgram.cpp
        ${GUI_PATH}/*.cpp
        ${MODEL_GEOMETRY_PATH}/*.cpp
        ${COLLISION_PATH}/*.cpp
#        ${EVENT_PATH}/*.cpp
        ${OBJECTS_PATH}/*.cpp
        ${GRAPHICS_PLUGIN_PATH}/*.cpp
        ${OPENXR_WRAPPER_PATH}/*.cpp
        ${OPENXR_EXTENSION_PATH}/*.cpp
        ${UTIL_PATH}/*.cpp
)

add_library(sampleframework
        STATIC
        ${CPP_SOURCES}
)

target_include_directories(
        sampleframework
        PUBLIC
        src
        ${OPENXR_DIR}
        ${OPENXR_PICO_DIR}
        ${GUI_PATH}
        ${UTIL_PATH}
        ${MODEL_PATH}
        ${COLLISION_PATH}
        ${EVENT_PATH}
        ${OBJECTS_PATH}
        ${MODEL_GEOMETRY_PATH}
        ${GRAPHICS_PLUGIN_PATH}
        ${PLATFORM_PLUGIN_PATH}
        ${OPENXR_WRAPPER_PATH}
        ${OPENXR_EXTENSION_PATH}
        ${3RDPARTY_PATH}/khronos/openxr/OpenXR-CTS/src/common
        ${3RDPARTY_PATH}/khronos/openxr/OpenXR-CTS/src/conformance/framework
        ${3RDPARTY_PATH}/khronos/openxr/OpenXR-CTS/src/conformance/utilities
        ${3RDPARTY_PATH}/khronos/openxr/OpenXR-CTS/src/conformance/framework/gltf
        ${3RDPARTY_PATH}/khronos/openxr/OpenXR-CTS/src/conformance/framework/pbr
        ${3RDPARTY_PATH}/khronos/pch
        ${3RDPARTY_PATH}/khronos/opengl
        ${3RDPARTY_PATH}/imgui
        ${3RDPARTY_PATH}/imgui/backends
        ${3RDPARTY_PATH}/glm-0.9.9.8
        ${3RDPARTY_PATH}/stb
        ${3RDPARTY_PATH}/tinygltf
        ${3RDPARTY_PATH}/type-lite/include
        ${3RDPARTY_PATH}/span-lite/include
        ${ANDROID_NDK}/sources/android/native_app_glue
)

target_compile_definitions(
        sampleframework
        PUBLIC
        XR_USE_ANDROID=1
        XR_USE_PLATFORM_ANDROID=1
        XR_USE_GRAPHICS_API_OPENGL_ES=1
        ANDROID_NDK
)

target_compile_options(
        sampleframework
        PUBLIC
        -Werror
        -Wall
        -Wextra
        -Wno-unused-parameter
        -Wno-missing-field-initializers
        -Wno-unused-function
        -Wno-unused-private-field
        -Wno-unused-variable
#        -fstack-protector-all
        -fno-stack-protector
)

target_link_libraries(
        sampleframework
        PUBLIC
        ${OPENXR_LOADER}
        android
        app_glue
        EGL
        GLESv3
        log
        openxr-gfxwrapper
        imgui
        glm
        stb

        PRIVATE
        # add conformance framework libs
#        conformance_framework_gltf
#        conformance_framework_tinygltf
        conformance_framework
        conformance_framework_pbr
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(
            sampleframework
            PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-invalid-offsetof>
    )
endif()
