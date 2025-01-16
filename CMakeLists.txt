cmake_minimum_required(VERSION 3.7...3.27)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

project(LuaBackendHook LANGUAGES CXX)

set(PROGRAM_VERSION "" CACHE STRING "Program version override")

option(WIL_BUILD_TESTS OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(ztd.text
    GIT_REPOSITORY https://github.com/soasis/text.git
    GIT_TAG        c06e3bf990911e09652b784ed01bc68f8f1c1105)

FetchContent_Declare(wil
    GIT_REPOSITORY https://github.com/microsoft/wil.git
    GIT_TAG        8501377980a52d98d1b51d04f60f571a7f0dc3db)

FetchContent_Declare(tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG        d8fa9a1fddc90254cac2366dde23f0b613bc1280)

# Disable DiscordRPC from running clang-format to work around issue with
# its .clang-format file.
set(CLANG_FORMAT_CMD "")

FetchContent_Declare(DiscordRPC
    GIT_REPOSITORY https://github.com/discord/discord-rpc.git
    GIT_TAG        963aa9f3e5ce81a4682c6ca3d136cddda614db33)

FetchContent_MakeAvailable(ztd.text wil tomlplusplus DiscordRPC)

add_subdirectory(external/subproject/lua544)

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
    target_compile_options(DBGHELP PRIVATE -W4 -WX)
else()
    target_compile_options(DBGHELP PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_compile_definitions(DBGHELP PRIVATE -DUNICODE -D_UNICODE)
target_include_directories(DBGHELP PRIVATE
    external/include
    external/subproject/lua544
    ${DiscordRPC_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(DBGHELP PRIVATE
    ztd.text
    WIL
    tomlplusplus::tomlplusplus
    lua54
    discord-rpc
)

unset(PROGRAM_VERSION CACHE)
