cmake_minimum_required(VERSION 3.7...3.27)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

project(LuaBackendHook LANGUAGES CXX)

set(PROGRAM_VERSION "" CACHE STRING "Program version override")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Statically link CRT
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

find_package(ztd.text CONFIG REQUIRED)
find_package(ztd.cuneicode CONFIG REQUIRED)

find_package(wil CONFIG REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(tomlplusplus REQUIRED IMPORTED_TARGET tomlplusplus)

find_package(sol2 CONFIG REQUIRED)
find_package(Lua REQUIRED)

find_library(DISCORD_RPC_LIBRARY_DEBUG discord-rpc
    PATHS ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/lib
    NO_DEFAULT_PATH
    REQUIRED)

find_library(DISCORD_RPC_LIBRARY_RELEASE discord-rpc
    PATHS ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib
    NO_DEFAULT_PATH
    REQUIRED)

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
    # FUTURE(Sirius902) ZTD_STD_LIBRARY_RANGES is broken currently on MSVC
    target_compile_options(DBGHELP PRIVATE -W4 -WX -DZTD_STD_LIBRARY_RANGES=0)
else()
    target_compile_options(DBGHELP PRIVATE -Wall -Wextra -Wpedantic)
endif()

# FUTURE(Sirius902) Use tomlplusplus as header only because of weird linking issues
target_compile_definitions(DBGHELP PRIVATE -DUNICODE -D_UNICODE -DTOML_HEADER_ONLY=1)
target_include_directories(DBGHELP PRIVATE
    external/include
    ${LUA_INCLUDE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(DBGHELP PRIVATE
    ztd.text
    WIL::WIL
    sol2
    ${LUA_LIBRARIES}
    $<$<CONFIG:Debug>:${DISCORD_RPC_LIBRARY_DEBUG}>
    $<$<CONFIG:Release>:${DISCORD_RPC_LIBRARY_RELEASE}>
)

unset(PROGRAM_VERSION CACHE)
