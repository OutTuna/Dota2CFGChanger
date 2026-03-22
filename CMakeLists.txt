cmake_minimum_required(VERSION 3.14)
project(DotaManager)

set(CMAKE_POLICY_DEFAULT_CMP0048 NEW)
cmake_policy(SET CMP0077 NEW)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(ZLIB_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(CPR_USE_SYSTEM_CURL OFF CACHE BOOL "" FORCE)
set(CPR_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)

include(FetchContent)

FetchContent_Declare(cpr GIT_REPOSITORY https://github.com/libcpr/cpr.git GIT_TAG 1.11.1)
FetchContent_MakeAvailable(cpr)

FetchContent_Declare(json GIT_REPOSITORY https://github.com/nlohmann/json GIT_TAG v3.11.3)
FetchContent_MakeAvailable(json)

FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw GIT_TAG 3.3.8)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(imgui GIT_REPOSITORY https://github.com/ocornut/imgui GIT_TAG v1.91.6)
FetchContent_MakeAvailable(imgui)

add_executable(DotaManager
        main.cpp
        app_logic.cpp
        app_logic.h
        app_state.h
        ui.cpp
        ui.h
        app_icon.h
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

if(WIN32 AND EXISTS "${CMAKE_SOURCE_DIR}/app.rc")
    target_sources(DotaManager PRIVATE app.rc)
endif()

target_include_directories(DotaManager PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(DotaManager PRIVATE cpr::cpr nlohmann_json::nlohmann_json glfw)

if(WIN32)
    file(WRITE "${CMAKE_BINARY_DIR}/generated.rc"
            "IDI_ICON1 ICON \"${CMAKE_SOURCE_DIR}/icon.ico\"\n")
    target_sources(DotaManager PRIVATE "${CMAKE_BINARY_DIR}/generated.rc")
    target_link_libraries(DotaManager PRIVATE opengl32 shell32 ole32)
    set_property(TARGET DotaManager PROPERTY WIN32_EXECUTABLE ON)
    if(MSVC)
        target_compile_options(DotaManager PRIVATE /utf-8)
    else()
        target_compile_options(DotaManager PRIVATE -finput-charset=UTF-8 -fexec-charset=UTF-8)
    endif()
endif()
