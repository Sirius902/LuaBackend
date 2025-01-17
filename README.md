# LuaBackend Hook

![Windows Build](https://github.com/Sirius902/LuaBackend/workflows/CI/badge.svg)

## What is this?

This is a fork of TopazTK's [LuaBackend](https://github.com/TopazTK/LuaBackend) which focuses on
consistency and reliability for timing-critical scripts.

## Why use this over LuaBackend or LuaFrontend?

For most scripts there's no issue using LuaBackend or LuaFrontend but for scripts such as the
Garden of Assemblage (GoA) Randomizer, which relies on running once before every frame in-game, it's
important this requirement is met. LuaBackend and LuaFrontend currently aren't syncronized with
the game's main loop, leading to unintended behavior in scripts like GoA which can range from
crashes to warps to incorrect locations. This fork however syncronizes the Lua scripts with the game
loop effectively eliminating these issues. However, scripts that rely on new features from LuaFrontend
are not supported and it is recommended to use LuaFrontend concurrently with LuaBackend Hook to achieve this.

## Support

Supports the PC Global and Japanese versions of:

- Kingdom Hearts Final Mix
- Kingdom Hearts Re: Chain of Memories
- Kingdom Hearts II Final Mix
- Kingdom Hearts Birth by Sleep Final Mix
- Kingdom Hearts Dream Drop Distance HD

Installation instructions are in [INSTALL.md](INSTALL.md).

## Why is the dll file named DBGHELP?

This is so that LuaBackend Hook can hook into the game without requiring an EXE patch, but still
be able to launch automatically with the game.

To support platforms where hooking as DBGHELP does not work such as Linux using Wine,
hooking as DINPUT8 is also supported. To hook as DINPUT8, rename the `DBGHELP.dll` to
`DINPUT8.dll`.

## Building

- Install Visual Studio Community 2022 (or just the build tools) and ensure the `Desktop development with C++` workflow is installed.
- Install CMake: `winget install Kitware.CMake`
- Install vcpkg by following steps 1 and 2.1 from
[here](https://learn.microsoft.com/en-us/vcpkg/get_started/get-started?pivots=shell-powershell).
- Install the required vcpkg dependencies by running the following in PowerShell.

```powershell
$env:VCPKG_DEFAULT_TRIPLET="x64-windows-static"
vcpkg install pkgconf fmt ztd-text wil tomlplusplus sol2 lua rapidjson
```

- Inside the repository run `cmake --preset=default` to configure the project.
- Run `cmake --build build` to build the project. To build for release mode add `--config Release`.
- The artifacts can now be found somewhere in the `build` directory. If building for MSVC in debug mode this will be `build/Debug`.
