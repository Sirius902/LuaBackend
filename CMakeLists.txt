cmake_minimum_required(VERSION 3.7...3.27)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

project(LuaBackendHook LANGUAGES CXX)

set(PROGRAM_VERSION "" CACHE STRING "Program version override")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(fmt CONFIG REQUIRED)

find_package(ztd.text CONFIG REQUIRED)
find_package(ztd.cuneicode CONFIG REQUIRED)

find_package(wil CONFIG REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(tomlplusplus REQUIRED IMPORTED_TARGET tomlplusplus)

find_package(sol2 CONFIG REQUIRED)
find_package(Lua REQUIRED)

include(FetchContent)

# Disable DiscordRPC from running clang-format to work around issue with
# its .clang-format file.
set(CLANG_FORMAT_CMD "")

FetchContent_Declare(DiscordRPC
    GIT_REPOSITORY https://github.com/discord/discord-rpc.git
    GIT_TAG        963aa9f3e5ce81a4682c6ca3d136cddda614db33)

FetchContent_MakeAvailable(DiscordRPC)

# Required by DiscordRPC.
find_package(RapidJSON CONFIG REQUIRED)

# Add RapidJSON include path for DiscordRPC.
target_link_libraries(discord-rpc PRIVATE rapidjson)

set(SOURCE
    src/main_dll.cpp
    src/config.cpp
    src/lua_exec.cpp
    src/header_text.cpp
    src/ConsoleLib.cpp
    src/DCInstance.cpp
    src/LuaBackend.cpp
    src/config.h
    src/game_info.h
    src/lua_exec.h
    src/wil_extra.h
    src/header_text.h
    src/ConsoleLib.h
    src/DCInstance.h
    src/LuaBackend.h
    src/MemoryLib.h
)

add_custom_target(clang-format
    COMMAND clang-format -i -style=file ${SOURCE}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_custom_target(build-info
    COMMAND ${CMAKE_COMMAND}
        -DPROGRAM_VERSION:STRING=${PROGRAM_VERSION}
        -DINSTALL_DIR:STRING=${CMAKE_CURRENT_BINARY_DIR}
        -P CMake/BuildInfo.cmake
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    VERBATIM
)

add_library(DBGHELP SHARED ${SOURCE})
add_dependencies(DBGHELP build-info)

if(MSVC)
    # ZTD_STD_LIBRARY_RANGES is broken currently on MSVC
    target_compile_options(DBGHELP PRIVATE -W4 -WX  -DZTD_STD_LIBRARY_RANGES=0)
else()
    target_compile_options(DBGHELP PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_compile_definitions(DBGHELP PRIVATE -DUNICODE -D_UNICODE -DTOML_HEADER_ONLY=1)
target_include_directories(DBGHELP PRIVATE
    external/include
    ${DiscordRPC_SOURCE_DIR}/include
    ${LUA_INCLUDE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(DBGHELP PRIVATE
    fmt::fmt
    ztd.text
    WIL::WIL
    sol2
    ${LUA_LIBRARIES}
    discord-rpc
)

unset(PROGRAM_VERSION CACHE)
