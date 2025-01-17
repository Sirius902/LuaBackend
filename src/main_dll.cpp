#include <array>
#include <chrono>
#include <concepts>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <ztd/text.hpp>

#define WIN32_LEAN_AND_MEAN
#include <shlobj.h>
#include <wil/stl.h>
#include <wil/win32_helpers.h>
#include <windows.h>

#define MiniDumpWriteDump MiniDumpWriteDump_
#include <minidumpapiset.h>
#undef MiniDumpWriteDump

#include "config.h"
#include "game_info.h"
#include "lua_exec.h"
#include "wil_extra.h"

// TODO: Remove after init fix.
#include <thread>

namespace fs = std::filesystem;
namespace ranges = std::ranges;

using MiniDumpWriteDumpProc = decltype(&MiniDumpWriteDump_);

using DirectInput8CreateProc = HRESULT(WINAPI*)(HINSTANCE hinst,
                                                DWORD dwVersion,
                                                LPCVOID riidltf, LPVOID* ppvOut,
                                                LPVOID punkOuter);

using GameFrameProc = std::uint64_t(__cdecl*)(void* rcx);

MiniDumpWriteDumpProc writeDumpProc = nullptr;
DirectInput8CreateProc createProc = nullptr;

GameFrameProc* frameProcPtr = nullptr;
GameFrameProc frameProc = nullptr;

std::optional<Config> config;
std::optional<GameInfo> gameInfo;

std::uint64_t moduleAddress = 0;

template <ranges::bidirectional_range R>
  requires std::same_as<ranges::range_value_t<R>, std::uintptr_t>
std::optional<std::uintptr_t> followPointerChain(std::uintptr_t start,
                                                 const R& offsets) {
  std::uintptr_t current = start;

  for (auto it = ranges::begin(offsets); it != ranges::end(offsets); ++it) {
    if (current == 0)
      return {};

    if (it != ranges::end(offsets) - 1) {
      current = *reinterpret_cast<std::uintptr_t*>(current + *it);
    } else {
      current += *it;
    }
  }

  return current;
}

std::uint64_t __cdecl frameHook(void* rcx) {
  ExecuteLUA();
  return frameProc(rcx);
}

LONG WINAPI crashDumpHandler(PEXCEPTION_POINTERS exceptionPointers) {
  HANDLE file = CreateFileW(L"CrashDump.dmp", GENERIC_READ | GENERIC_WRITE, 0,
                            NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

  if (file != INVALID_HANDLE_VALUE) {
    MINIDUMP_EXCEPTION_INFORMATION mdei;

    mdei.ThreadId = GetCurrentThreadId();
    mdei.ExceptionPointers = exceptionPointers;
    mdei.ClientPointers = TRUE;

    (*writeDumpProc)(GetCurrentProcess(), GetCurrentProcessId(), file,
                     MiniDumpNormal, &mdei, 0, 0);

    CloseHandle(file);
  }

  return EXCEPTION_EXECUTE_HANDLER;
}

struct SectionInfo {
  std::uintptr_t offset;
  std::size_t size;
};

SectionInfo findTextSectionInfo() {
  const auto basePtr = std::bit_cast<std::uint8_t*>(moduleAddress);
  const auto dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(basePtr);
  if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
    throw std::runtime_error{"DOS header is corrupt"};
  }

  const auto ntHeaderPtr = basePtr + dosHeader->e_lfanew;
  const auto ntHeader = reinterpret_cast<PIMAGE_NT_HEADERS64>(ntHeaderPtr);
  if (ntHeader->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
    throw new std::runtime_error{"NT64 header is invalid"};
  }

  std::optional<SectionInfo> textSection;

  for (WORD i = 0; i < ntHeader->FileHeader.NumberOfSections; i++) {
    const auto sectionHeader = reinterpret_cast<PIMAGE_SECTION_HEADER>(
        ntHeaderPtr + sizeof(IMAGE_NT_HEADERS64) +
        (i * sizeof(IMAGE_SECTION_HEADER)));

    constexpr char textSectionName[] = ".text";
    if (std::memcmp(reinterpret_cast<char*>(sectionHeader->Name),
                    textSectionName, sizeof(textSectionName) - 1) == 0) {
      textSection = SectionInfo{
          .offset = sectionHeader->VirtualAddress,
          .size = sectionHeader->SizeOfRawData,
      };
      break;
    }
  }

  return textSection.value();
}

std::optional<GameFrameProc*> findFrameProc(const SectionInfo& textInfo) {
  constexpr char sig[] = "\x48\x89\x35\xA7\xB6\x69\x00\x48\x8B\xC6";
  constexpr char mask[] = "\xFF\xFF\xFF\x00\x00\x00\x00\xFF\xFF\xFF";
  static_assert(sizeof(sig) == sizeof(mask));

  const char* textStart =
      std::bit_cast<const char*>(moduleAddress + textInfo.offset);
  const char* textEnd = textStart + textInfo.size;

  for (const char* p = textStart; p != textEnd - (sizeof(sig) - 1); p++) {
    bool isMatch = true;
    for (std::size_t i = 0; i < sizeof(sig) - 1; i++) {
      if ((p[i] & mask[i]) != (sig[i] & mask[i])) {
        isMatch = false;
        break;
      }
    }

    if (!isMatch)
      continue;

    const auto scanRes = std::bit_cast<std::uintptr_t>(p);
    const auto appPtrAddress =
        scanRes + 0x7 + *reinterpret_cast<const std::int32_t*>(scanRes + 0x3);
    const auto appPtr = *std::bit_cast<void**>(appPtrAddress);

    if (appPtr == nullptr)
      return std::nullopt;

    const auto vtableAddress = *static_cast<std::uintptr_t*>(appPtr);
    const auto onFrameEntry = vtableAddress + 4 * sizeof(std::uintptr_t);
    return std::bit_cast<GameFrameProc*>(onFrameEntry);
  }

  return std::nullopt;
}

bool hookGame() {
  static_assert(sizeof(std::uint64_t) == sizeof(std::uintptr_t));

  const auto textInfo = findTextSectionInfo();
  const auto frameProcPtrOpt = findFrameProc(textInfo);

  if (!frameProcPtrOpt.has_value()) {
    return false;
  }

  frameProcPtr = frameProcPtrOpt.value();
  if (*frameProcPtr == nullptr)
    return false;

  DWORD originalProt = 0;
  VirtualProtect(frameProcPtr, sizeof(frameProcPtr), PAGE_READWRITE,
                 &originalProt);
  frameProc = *frameProcPtr;
  *frameProcPtr = frameHook;
  VirtualProtect(frameProcPtr, sizeof(frameProcPtr), originalProt,
                 &originalProt);

  SetUnhandledExceptionFilter(crashDumpHandler);

  return true;
}

DWORD WINAPI entry([[maybe_unused]] LPVOID lpParameter) {
  std::wstring modulePathStr;
  wil::GetModuleFileNameW(nullptr, modulePathStr);

  fs::path modulePath = fs::path{modulePathStr};
  fs::path moduleDir = modulePath.parent_path();
  std::wstring moduleNameW = modulePath.filename();

  std::u8string moduleName =
      ztd::text::transcode(moduleNameW, ztd::text::wide_utf16, ztd::text::utf8,
                           ztd::text::replacement_handler);

  try {
    config = Config::load("LuaBackend.toml");
    auto entry = config->gameInfo(moduleName);
    if (entry) {
      gameInfo = *entry;
    } else {
      return 0;
    }

    moduleAddress = (std::uint64_t)GetModuleHandleW(nullptr);
    std::uint64_t baseAddress = moduleAddress + gameInfo->baseAddress;

    fs::path gameDocsRoot = [&]() {
      PWSTR docsRootStr;
      SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &docsRootStr);

      return fs::path{docsRootStr} / gameInfo->gameDocsPathStr;
    }();

    std::vector<fs::path> scriptPaths;
    for (const auto& path : gameInfo->scriptPaths) {
      if (path.relative) {
        fs::path gameScriptsPath = gameDocsRoot / path.str;
        if (fs::exists(gameScriptsPath)) {
          scriptPaths.push_back(gameScriptsPath);
        }
      } else {
        fs::path gameScriptsPath = fs::path{path.str};
        if (fs::exists(gameScriptsPath)) {
          scriptPaths.push_back(gameScriptsPath);
        }
      }
    }

    if (!scriptPaths.empty()) {
      AllocConsole();
      SetConsoleOutputCP(CP_UTF8);
      FILE* f;
      freopen_s(&f, "CONOUT$", "w", stdout);

      if (EntryLUA(GetCurrentProcessId(), GetCurrentProcess(), baseAddress,
                   std::move(scriptPaths)) == 0) {
        // TODO: Hook after game initialization is done.
        while (!hookGame()) {
          std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
      } else {
        std::cout << "Failed to initialize internal LuaBackend!" << std::endl;
      }
    }
  } catch (std::exception& e) {
    std::string msg = "entry exception: " + std::string(e.what()) +
                      "\n\nScripts failed to load.";
    std::wstring wmsg =
        ztd::text::transcode(msg, ztd::text::compat_utf8, ztd::text::wide_utf16,
                             ztd::text::replacement_handler);
    MessageBoxW(NULL, wmsg.c_str(), L"LuaBackendHook", MB_ICONERROR | MB_OK);
  }

  return 0;
}

BOOL WINAPI DllMain([[maybe_unused]] HINSTANCE hinstDLL, DWORD fdwReason,
                    [[maybe_unused]] LPVOID lpReserved) {
  static HMODULE dbgHelp = nullptr;
  static HMODULE dinput8 = nullptr;

  switch (fdwReason) {
  case DLL_PROCESS_ATTACH: {
    std::wstring systemDirectoryStr;
    wil::GetSystemDirectoryW(systemDirectoryStr);
    fs::path dllPath = fs::path{systemDirectoryStr} / L"DBGHELP.dll";

    dbgHelp = LoadLibraryW(dllPath.wstring().c_str());
    writeDumpProc =
        (MiniDumpWriteDumpProc)GetProcAddress(dbgHelp, "MiniDumpWriteDump");

    fs::path dinput8Path = fs::path{systemDirectoryStr} / L"DINPUT8.dll";

    dinput8 = LoadLibraryW(dinput8Path.wstring().c_str());
    createProc =
        (DirectInput8CreateProc)GetProcAddress(dinput8, "DirectInput8Create");

    if (CreateThread(nullptr, 0, entry, nullptr, 0, nullptr) == nullptr) {
      return FALSE;
    }

    break;
  }
  case DLL_PROCESS_DETACH: {
    FreeLibrary(dbgHelp);
    FreeLibrary(dinput8);
    break;
  }
  default:
    break;
  }

  return TRUE;
}

extern "C" __declspec(dllexport) HRESULT WINAPI
DirectInput8Create(HINSTANCE hinst, DWORD dwVersion, LPCVOID riidltf,
                   LPVOID* ppvOut, LPVOID punkOuter) {
  return createProc(hinst, dwVersion, riidltf, ppvOut, punkOuter);
}

extern "C" __declspec(dllexport) BOOL WINAPI MiniDumpWriteDump(
    HANDLE hProcess, DWORD ProcessId, HANDLE hFile, MINIDUMP_TYPE DumpType,
    PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
    PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
    PMINIDUMP_CALLBACK_INFORMATION CallbackParam) {
  return writeDumpProc(hProcess, ProcessId, hFile, DumpType, ExceptionParam,
                       UserStreamParam, CallbackParam);
}
