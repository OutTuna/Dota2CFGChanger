cmake_minimum_required(VERSION 3.14)

project(DotaManager)

set(CMAKE_POLICY_DEFAULT_CMP0048 NEW)
cmake_policy(SET CMP0077 NEW)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(APP_VERSION "0.0" CACHE STRING "App version, e.g. 1.7 or v1.7 ('v' optional)")
string(REGEX REPLACE "^v" "" APP_VERSION_STRING "${APP_VERSION}")
if(NOT APP_VERSION_STRING MATCHES "^[0-9]+[.][0-9]+([.][0-9]+)?([.][0-9]+)?$")
    message(FATAL_ERROR "APP_VERSION must include major and minor components, e.g. v1.13")
endif()
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/generated")
configure_file("${CMAKE_SOURCE_DIR}/cmake/AppVersion.h.in"
    "${CMAKE_BINARY_DIR}/generated/app_version.h" @ONLY)
string(REPLACE "." ";" APP_VERSION_PARTS "${APP_VERSION_STRING}")
list(GET APP_VERSION_PARTS 0 APP_VERSION_MAJOR)
list(LENGTH APP_VERSION_PARTS APP_VERSION_PARTS_LEN)
if(APP_VERSION_PARTS_LEN GREATER 1)
    list(GET APP_VERSION_PARTS 1 APP_VERSION_MINOR)
else()
    set(APP_VERSION_MINOR "0")
endif()


set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

if(WIN32)
    set(CURL_USE_SCHANNEL ON  CACHE BOOL "" FORCE)
    set(CURL_USE_OPENSSL  OFF CACHE BOOL "" FORCE)
else()
    set(CURL_USE_OPENSSL  ON  CACHE BOOL "" FORCE)
    set(CURL_USE_SCHANNEL OFF CACHE BOOL "" FORCE)
endif()

set(BUILD_CURL_EXE           OFF CACHE BOOL "" FORCE)
set(ENABLE_CURL_MANUAL       OFF CACHE BOOL "" FORCE)
set(BUILD_LIBCURL_DOCS       OFF CACHE BOOL "" FORCE)
set(BUILD_MISC_DOCS          OFF CACHE BOOL "" FORCE)

set(ZLIB_BUILD_SHARED_LIBS   OFF CACHE BOOL "" FORCE)
set(CPR_USE_SYSTEM_CURL      OFF CACHE BOOL "" FORCE)
set(CPR_BUILD_TESTS          OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS          OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES      OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS         OFF CACHE BOOL "" FORCE)

include(FetchContent)

FetchContent_Declare(cpr   GIT_REPOSITORY https://github.com/libcpr/cpr.git GIT_TAG 1.11.1 GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(cpr)

FetchContent_Declare(json  GIT_REPOSITORY https://github.com/nlohmann/json GIT_TAG v3.11.3 GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(json)

FetchContent_Declare(glfw  GIT_REPOSITORY https://github.com/glfw/glfw GIT_TAG 3.3.8 GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(imgui GIT_REPOSITORY https://github.com/ocornut/imgui GIT_TAG v1.91.6 GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(imgui)

FetchContent_Declare(stb   GIT_REPOSITORY https://github.com/nothings/stb GIT_TAG 2c980bb59875b0d32144a71867fbdebb2f77cd20 GIT_SHALLOW FALSE)
FetchContent_MakeAvailable(stb)

set(THEME_RESOURCES
    ${CMAKE_SOURCE_DIR}/resources/themes/theme_dark.json
    ${CMAKE_SOURCE_DIR}/resources/themes/theme_indigo.json
    ${CMAKE_SOURCE_DIR}/resources/themes/theme_vermillion.json
    ${CMAKE_SOURCE_DIR}/resources/themes/theme_classic_steam.json
    ${CMAKE_SOURCE_DIR}/resources/themes/theme_crimson.json
    ${CMAKE_SOURCE_DIR}/resources/themes/bg_crimson.png
)
set(EMBEDDED_THEMES_DIR "${CMAKE_BINARY_DIR}/generated")
add_custom_command(
    OUTPUT "${EMBEDDED_THEMES_DIR}/embedded_themes.cpp" "${EMBEDDED_THEMES_DIR}/embedded_themes.h"
    COMMAND ${CMAKE_COMMAND}
        "-DTHEMES_DIR=${CMAKE_SOURCE_DIR}/resources/themes"
        "-DOUTPUT_DIR=${EMBEDDED_THEMES_DIR}"
        -P "${CMAKE_SOURCE_DIR}/cmake/EmbedThemes.cmake"
    DEPENDS ${THEME_RESOURCES} "${CMAKE_SOURCE_DIR}/cmake/EmbedThemes.cmake"
    VERBATIM
)

add_custom_command(
    OUTPUT "${EMBEDDED_THEMES_DIR}/embedded_locales.h"
    COMMAND ${CMAKE_COMMAND}
        "-DLOCALES_DIR=${CMAKE_SOURCE_DIR}/resources/locales"
        "-DOUTPUT_DIR=${EMBEDDED_THEMES_DIR}"
        -P "${CMAKE_SOURCE_DIR}/cmake/EmbedLocales.cmake"
    DEPENDS "${CMAKE_SOURCE_DIR}/resources/locales/en.json"
        "${CMAKE_SOURCE_DIR}/resources/locales/ru.json"
        "${CMAKE_SOURCE_DIR}/resources/locales/uk.json"
        "${CMAKE_SOURCE_DIR}/cmake/EmbedLocales.cmake"
    VERBATIM
)

find_package(Threads REQUIRED)
add_library(dotamanager_core STATIC
    src/core/app.cpp
    src/core/file_ops.cpp
    src/core/localization.cpp
    src/platform/platform.cpp
    src/network/steam_api.cpp
    src/core/avatar_data.cpp
    src/core/release_info.cpp
    src/core/checksum.cpp
    src/core/updates.cpp
    src/network/update_transport.cpp
    ${EMBEDDED_THEMES_DIR}/embedded_locales.h
)
target_include_directories(dotamanager_core PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/network
    ${CMAKE_SOURCE_DIR}/src/platform
    ${EMBEDDED_THEMES_DIR}
    PRIVATE ${stb_SOURCE_DIR})
target_link_libraries(dotamanager_core PUBLIC cpr::cpr nlohmann_json::nlohmann_json Threads::Threads)

if(WIN32)
    target_link_libraries(dotamanager_core PUBLIC shell32 ole32 advapi32 bcrypt)
    if(MSVC)
        target_compile_options(dotamanager_core PRIVATE /utf-8)
    else()
        target_compile_options(dotamanager_core PRIVATE -finput-charset=UTF-8 -fexec-charset=UTF-8)
    endif()
else()
    find_package(OpenSSL REQUIRED COMPONENTS Crypto)
    target_link_libraries(dotamanager_core PUBLIC OpenSSL::Crypto)
endif()

add_executable(DotaManager
        ${EMBEDDED_THEMES_DIR}/embedded_themes.cpp
        ${EMBEDDED_THEMES_DIR}/embedded_themes.h
        main.cpp
        src/core/app.h
        src/ui/avatar.cpp
        src/ui/avatar.h
        src/ui/ui.h
        src/ui/ui_internal.h
        src/ui/theme.cpp
        src/ui/panels.cpp
        src/ui/main_window.cpp
        src/ui/success_popup.cpp
        src/ui/update_popup.cpp
        resources/icons/app_icon.h
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(DotaManager PRIVATE
        ${EMBEDDED_THEMES_DIR}
        ${CMAKE_SOURCE_DIR}/resources/icons
        ${CMAKE_SOURCE_DIR}/src/ui
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
        ${stb_SOURCE_DIR}
)

target_link_libraries(DotaManager PRIVATE dotamanager_core glfw)
if(NOT WIN32)
    set(OpenGL_GL_PREFERENCE LEGACY)
    find_package(OpenGL REQUIRED)
    target_link_libraries(DotaManager PRIVATE OpenGL::GL)
endif()


if(WIN32)
    if(EXISTS "${CMAKE_SOURCE_DIR}/resources/icons/Icon.ico" AND EXISTS "${CMAKE_SOURCE_DIR}/resources/windows/app.manifest")
        configure_file(
                "${CMAKE_SOURCE_DIR}/resources/windows/app.rc.in"
                "${CMAKE_BINARY_DIR}/generated_app.rc"
                @ONLY
        )
        target_sources(DotaManager PRIVATE "${CMAKE_BINARY_DIR}/generated_app.rc")
    endif()

    target_link_libraries(DotaManager PRIVATE opengl32 shell32 ole32 advapi32)
    set_property(TARGET DotaManager PROPERTY WIN32_EXECUTABLE ON)

    if(MINGW)
        target_link_options(DotaManager PRIVATE
                -static
                -static-libgcc
                -static-libstdc++
        )
    endif()

    if(MSVC)
        target_compile_options(DotaManager PRIVATE /utf-8)
        target_link_options(DotaManager PRIVATE /MANIFEST:NO)
    else()
        target_compile_options(DotaManager PRIVATE -finput-charset=UTF-8 -fexec-charset=UTF-8)
    endif()
endif()

option(DOTAMANAGER_BUILD_TESTS "Build regression tests" OFF)
if(DOTAMANAGER_BUILD_TESTS)
    enable_testing()
    add_executable(version_build_checks tests/version_build_checks.cpp)
    target_link_libraries(version_build_checks PRIVATE dotamanager_core)
    add_test(NAME version_build_checks COMMAND version_build_checks "${APP_VERSION_STRING}")
    add_executable(update_checks tests/update_checks.cpp src/core/release_info.cpp src/core/checksum.cpp)
    add_executable(update_service_checks tests/update_service_checks.cpp src/core/updates.cpp src/core/release_info.cpp src/core/checksum.cpp)
    add_executable(avatar_checks tests/avatar_checks.cpp)
    target_link_libraries(avatar_checks PRIVATE dotamanager_core)
    add_executable(update_ui_checks tests/update_ui_checks.cpp src/ui/update_popup.cpp
        ${imgui_SOURCE_DIR}/imgui.cpp ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp ${imgui_SOURCE_DIR}/imgui_widgets.cpp)
    target_include_directories(update_ui_checks PRIVATE ${imgui_SOURCE_DIR})
    target_link_libraries(update_ui_checks PRIVATE dotamanager_core)
    foreach(test_target update_checks update_service_checks avatar_checks update_ui_checks)
        target_include_directories(${test_target} PRIVATE
            ${EMBEDDED_THEMES_DIR}
            ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/core
            ${CMAKE_SOURCE_DIR}/src/network ${CMAKE_SOURCE_DIR}/src/platform)
        target_link_libraries(${test_target} PRIVATE nlohmann_json::nlohmann_json Threads::Threads)
        if(WIN32)
            target_link_libraries(${test_target} PRIVATE bcrypt)
        else()
            target_link_libraries(${test_target} PRIVATE OpenSSL::Crypto)
        endif()
        if(MSVC)
            target_compile_options(${test_target} PRIVATE /UNDEBUG /utf-8)
        else()
            target_compile_options(${test_target} PRIVATE -UNDEBUG)
        endif()
        if(test_target STREQUAL "avatar_checks")
            add_test(NAME ${test_target} COMMAND ${test_target} "${CMAKE_SOURCE_DIR}/resources/themes/bg_crimson.png")
        else()
            add_test(NAME ${test_target} COMMAND ${test_target})
        endif()
    endforeach()
    target_compile_definitions(update_service_checks PRIVATE DOTA_APP_VERSION="1.9")
endif()
