cmake_minimum_required(VERSION 3.14)

project(DotaManager)

set(CMAKE_POLICY_DEFAULT_CMP0048 NEW)
cmake_policy(SET CMP0077 NEW)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(APP_VERSION "0.0" CACHE STRING "App version, e.g. 1.7 or v1.7 ('v' optional)")
string(REGEX REPLACE "^v" "" APP_VERSION_STRING "${APP_VERSION}")
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
    ${CMAKE_SOURCE_DIR}/themes/theme_dark.json
    ${CMAKE_SOURCE_DIR}/themes/theme_indigo.json
    ${CMAKE_SOURCE_DIR}/themes/theme_vermillion.json
    ${CMAKE_SOURCE_DIR}/themes/theme_classic_steam.json
    ${CMAKE_SOURCE_DIR}/themes/theme_crimson.json
    ${CMAKE_SOURCE_DIR}/themes/bg_crimson.png
)
set(EMBEDDED_THEMES_DIR "${CMAKE_BINARY_DIR}/generated")
add_custom_command(
    OUTPUT "${EMBEDDED_THEMES_DIR}/embedded_themes.cpp" "${EMBEDDED_THEMES_DIR}/embedded_themes.h"
    COMMAND ${CMAKE_COMMAND}
        "-DTHEMES_DIR=${CMAKE_SOURCE_DIR}/themes"
        "-DOUTPUT_DIR=${EMBEDDED_THEMES_DIR}"
        -P "${CMAKE_SOURCE_DIR}/cmake/EmbedThemes.cmake"
    DEPENDS ${THEME_RESOURCES} "${CMAKE_SOURCE_DIR}/cmake/EmbedThemes.cmake"
    VERBATIM
)

add_executable(DotaManager
        ${EMBEDDED_THEMES_DIR}/embedded_themes.cpp
        ${EMBEDDED_THEMES_DIR}/embedded_themes.h
        main.cpp
        app.cpp
        app.h
        avatar.cpp
        avatar.h
        ui/ui.h
        ui/ui_internal.h
        ui/theme.cpp
        ui/panels.cpp
        ui/main_window.cpp
        ui/success_popup.cpp
        app_icon.h
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(DotaManager PRIVATE
        ${EMBEDDED_THEMES_DIR}
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/ui
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
        ${stb_SOURCE_DIR}
)

target_link_libraries(DotaManager PRIVATE cpr::cpr nlohmann_json::nlohmann_json glfw)
if(NOT WIN32)
    find_package(OpenGL REQUIRED)
    target_link_libraries(DotaManager PRIVATE OpenGL::GL)
endif()


if(WIN32)
    if(EXISTS "${CMAKE_SOURCE_DIR}/Icon.ico" AND EXISTS "${CMAKE_SOURCE_DIR}/app.manifest")
        configure_file(
                "${CMAKE_SOURCE_DIR}/app.rc.in"
                "${CMAKE_BINARY_DIR}/generated_app.rc"
                @ONLY
        )
        target_sources(DotaManager PRIVATE "${CMAKE_BINARY_DIR}/generated_app.rc")
    endif()

    target_link_libraries(DotaManager PRIVATE opengl32 shell32 ole32)
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
