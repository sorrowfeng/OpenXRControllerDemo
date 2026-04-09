cmake_minimum_required(VERSION 3.10.2)

project(OpenXRControllerDemo C CXX)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(3rdParty)
add_subdirectory(framework)

add_library(OpenXRControllerDemo MODULE
    app/src/main/cpp/main.cpp
)

target_include_directories(OpenXRControllerDemo
    PUBLIC
    ${ANDROID_NDK}/sources/android/native_app_glue
    app/src/main/cpp
)

target_compile_options(OpenXRControllerDemo
    PRIVATE
    -Werror
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-missing-field-initializers
    -Wno-unused-function
    -Wno-unused-private-field
    -fno-stack-protector
)

target_link_libraries(OpenXRControllerDemo
    PRIVATE
    android
    app_glue
    EGL
    GLESv3
    log
    openxr-gfxwrapper
    sampleframework
    stb
)

set_target_properties(OpenXRControllerDemo PROPERTIES LINK_FLAGS "-u ANativeActivity_onCreate")
