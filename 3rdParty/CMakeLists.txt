

# This is a little helper library for setting up OpenGL
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/khronos/openxr/OpenXR-CTS/src/common/gfxwrapper_opengl.c")
    add_library(openxr-gfxwrapper
            STATIC
            ${CMAKE_CURRENT_SOURCE_DIR}/khronos/openxr/OpenXR-CTS/src/common/gfxwrapper_opengl.c
            ${CMAKE_CURRENT_SOURCE_DIR}/khronos/openxr/OpenXR-CTS/src/common/gfxwrapper_opengl.h
    )

    target_include_directories(
            openxr-gfxwrapper
            PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/khronos/openxr/OpenXR-CTS/src/common
            ${CMAKE_CURRENT_SOURCE_DIR}/khronos/pch
            ${CMAKE_CURRENT_SOURCE_DIR}/khronos/glext
            ${ANDROID_NDK}/sources/android/native_app_glue
            ${CMAKE_CURRENT_SOURCE_DIR}/stb
    )

    target_link_libraries(openxr-gfxwrapper
            PUBLIC
#            android
            app_glue
            EGL
            GLESv3
    )
endif()

# TODO: add build flavor
set(XR_USE_GRAPHICS_API_OPENGL_ES TRUE)
add_definitions(-DXR_USE_GRAPHICS_API_OPENGL_ES)
message(STATUS "Enabling OpenGL|ES support")
set(XR_USE_PLATFORM_ANDROID TRUE)
add_definitions(-DXR_USE_PLATFORM_ANDROID)
set(XR_USE_ANDROID TRUE)
add_definitions(-DXR_USE_ANDROID)

# imgui
# https://github.com/ocornut/imgui
add_library(
    imgui STATIC
        ${CMAKE_CURRENT_LIST_DIR}/imgui/imgui.cpp
        ${CMAKE_CURRENT_LIST_DIR}/imgui/imgui_draw.cpp
        ${CMAKE_CURRENT_LIST_DIR}/imgui/imgui_tables.cpp
        ${CMAKE_CURRENT_LIST_DIR}/imgui/imgui_widgets.cpp
        ${CMAKE_CURRENT_LIST_DIR}/imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(
        imgui
        PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/imgui
)

# glm
add_definitions(-DGLM_FORCE_NEON  -DGLM_FORCE_CTOR_INIT)
add_subdirectory(glm-0.9.9.8/glm)

# stb
add_subdirectory(stb)

# conformance framework lib
add_subdirectory(khronos/openxr/OpenXR-CTS/src/conformance)

# add basisu lib
add_subdirectory(basis_universal)
