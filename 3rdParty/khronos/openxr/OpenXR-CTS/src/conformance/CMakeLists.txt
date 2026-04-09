# Copyright (c) 2019-2024, The Khronos Group Inc.
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set(CONFORMANCE_TESTS_FOLDER "Conformance Test Suite")

set(3rdparty_folder ${PROJECT_SOURCE_DIR}/3rdParty)

add_library(
        conformance_framework_mikktspace STATIC
        ${3rdparty_folder}/mikktspace/mikktspace.c
)

target_include_directories(
        conformance_framework_mikktspace
        PUBLIC ${3rdparty_folder}/mikktspace
)
set_target_properties(
        conformance_framework_mikktspace PROPERTIES FOLDER
        ${CONFORMANCE_TESTS_FOLDER}
)



# add function
# inspired by: https://stackoverflow.com/a/47801116
function(make_includable input_file output_file)
    cmake_parse_arguments(
            PARSE_ARGV 2 MAKE_INCLUDABLE "" "" "REPLACE")

    file(READ "${input_file}" content)

    # regex replace
    list(LENGTH MAKE_INCLUDABLE_REPLACE length)
    while(length GREATER_EQUAL 2)
        list(GET MAKE_INCLUDABLE_REPLACE 0 find)
        list(GET MAKE_INCLUDABLE_REPLACE 1 replace)
        list(REMOVE_AT MAKE_INCLUDABLE_REPLACE 0 1)
        string(REGEX REPLACE "${find}" "${replace}" content "${content}")
        list(LENGTH MAKE_INCLUDABLE_REPLACE length)
    endwhile()

    file(WRITE "${output_file}" "R\"raw_text(\n${content})raw_text\"")
    # using https://stackoverflow.com/a/65945763
    # see https://stackoverflow.com/a/56828572 for a different approach
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${input_file}")
endfunction(make_includable)

function(glsl_spv_shader)
    set(options)
    set(oneValueArgs INPUT OUTPUT STAGE VARIABLE TARGET_ENV)
    set(multiValueArgs EXTRA_DEPENDS)
    cmake_parse_arguments(_glsl_spv "${options}" "${oneValueArgs}"
            "${multiValueArgs}" ${ARGN})

    if(GLSL_COMPILER)
        add_custom_command(
                OUTPUT "${_glsl_spv_OUTPUT}"
                COMMAND
                "${GLSL_COMPILER}" #
                "-mfmt=c" #
                "-fshader-stage=${_glsl_spv_STAGE}" #
                "${_glsl_spv_INPUT}" #
                -o "${_glsl_spv_OUTPUT}" #
                "--target-env=${_glsl_spv_TARGET_ENV}" #
                $<IF:$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>,-g,> #
                $<IF:$<CONFIG:Debug>,-O0,-O> #
                MAIN_DEPENDENCY "${_glsl_spv_INPUT}"
                DEPENDS "${_glsl_spv_INPUT}" ${_glsl_spv_EXTRA_DEPENDS}
                USES_TERMINAL)

    elseif(GLSLANG_VALIDATOR)
        # Run glslangValidator if we can find it
        add_custom_command(
                OUTPUT "${_glsl_spv_OUTPUT}"
                COMMAND
                "${GLSLANG_VALIDATOR}" #
                -S "${_glsl_spv_STAGE}" #
                #--nan-clamp #
                -x # output as hex
                -o "${_glsl_spv_OUTPUT}" #
                $<$<CONFIG:Debug,RelWithDebInfo>:-gVS> #
                $<$<CONFIG:Debug>:-Od> #
                $<$<CONFIG:Release>:-g0> #
                "--target-env" "${_glsl_spv_TARGET_ENV}" #
                "${_glsl_spv_INPUT}" #
                MAIN_DEPENDENCY "${_glsl_spv_INPUT}"
                DEPENDS "${_glsl_spv_INPUT}" ${_glsl_spv_EXTRA_DEPENDS}
                USES_TERMINAL)

    else()
        # Use the precompiled .spv files
        get_filename_component(glsl_src_dir "${_glsl_spv_INPUT}" DIRECTORY)

        get_filename_component(glsl_name_we "${_glsl_spv_INPUT}" NAME_WE)
        set(precompiled_file ${glsl_src_dir}/${glsl_name_we}.spv)
        configure_file("${precompiled_file}" "${_glsl_spv_OUTPUT}" COPYONLY)
    endif()
endfunction()

add_subdirectory(utilities)
add_subdirectory(framework)