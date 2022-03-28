#include <chrono>
#include <iostream>
#include <filesystem>
#include <string_view>
#include <vector>

#include <unordered_map>
#include <optional>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>

#define MiniDumpWriteDump MiniDumpWriteDump_Original
#include <minidumpapiset.h>
#undef MiniDumpWriteDump

#include <MemoryLib.h>
#include <LuaBackend.h>
#include <mIni/ini.h>

#include "x86_64.h"
#include "game_info.h"

// TODO: Remove after init fix.
#include <thread>

const std::unordered_map<std::string_view, GameInfo> gameInfos{
    { "KINGDOM HEARTS FINAL MIX.exe", { 0x22B7280, 0x3A0606, "kh1" } },
    { "KINGDOM HEARTS Re_Chain of Memories.exe", { 0xBF7A80, 0x4E4660, "recom" } },
    { "KINGDOM HEARTS II FINAL MIX.exe", { 0x89E9A0, 0x56454E, "kh2" } },
    { "KINGDOM HEARTS Birth by Sleep FINAL MIX.exe", { 0x110B5970, 0x60E334, "bbs" } },
};

namespace fs = std::filesystem;

using MiniDumpWriteDumpProc = decltype(&MiniDumpWriteDump_Original);

using GameFrameProc = std::uint64_t(__cdecl*)(void* rcx);

MiniDumpWriteDumpProc writeDumpProc = nullptr;

GameFrameProc* frameProcPtr = nullptr;
GameFrameProc frameProc = nullptr;

std::optional<GameInfo> gameInfo{};

extern "C"
{
    using namespace std;
    using namespace mINI;

    bool _funcOneState = true;
    bool _funcTwoState = true;
    bool _funcThreeState = true;

    vector<string> _scriptPaths;

    bool _showConsole = false;
    bool _requestedReset = false;

    LuaBackend* _backend = nullptr;

    chrono::high_resolution_clock::time_point _sClock;
    chrono::high_resolution_clock::time_point _msClock;

    void EnterWait()
    {
        string _output;
        getline(std::cin, _output);
        return;
    }

    void ResetLUA()
    {
        printf("\n");
        ConsoleLib::MessageOutput("Reloading...\n\n", 0);
        _backend = new LuaBackend(_scriptPaths, MemoryLib::ExecAddress + MemoryLib::BaseAddress);

        if (_backend->loadedScripts.size() == 0)
            ConsoleLib::MessageOutput("No scripts found! Reload halted!\n\n", 3);

        ConsoleLib::MessageOutput("Executing initialization event handlers...\n\n", 0);

        for (auto _script : _backend->loadedScripts)
            if (_script->initFunction)
            {
                auto _result = _script->initFunction();

                if (!_result.valid())
                {
                    sol::error _err = _result;
                    ConsoleLib::MessageOutput(_err.what(), 3);
                    printf("\n\n");
                }
            }

        ConsoleLib::MessageOutput("Reload complete!\n\n", 1);

        _msClock = chrono::high_resolution_clock::now();
        _sClock = chrono::high_resolution_clock::now();

        _requestedReset = false;
    }

    int __declspec(dllexport) EntryLUA(int ProcessID, HANDLE ProcessH, uint64_t TargetAddress, vector<string> ScriptPaths)
    {
        ShowWindow(GetConsoleWindow(), SW_HIDE);

        cout << "======================================" << "\n";
        cout << "======= LuaBackendHook | v1.2.1 ======" << "\n";
        cout << "====== Copyright 2021 - TopazTK ======" << "\n";
        cout << "======================================" << "\n";
        cout << "=== Compatible with LuaEngine v5.0 ===" << "\n";
        cout << "========== Embedded Version ==========" << "\n";
        cout << "======================================" << "\n\n";

        ConsoleLib::MessageOutput("Initializing LuaEngine v5.0...\n\n", 0);
        _scriptPaths = ScriptPaths;

        MemoryLib::ExternProcess(ProcessID, ProcessH, TargetAddress);

        _backend = new LuaBackend(ScriptPaths, MemoryLib::ExecAddress + TargetAddress);
        _backend->frameLimit = 16;

        if (_backend->loadedScripts.size() == 0)
        {
            ConsoleLib::MessageOutput("No scripts were found! Initialization halted!\n\n", 3);
            return -1;
        }

        ConsoleLib::MessageOutput("Executing initialization event handlers...\n\n", 0);

        for (auto _script : _backend->loadedScripts)
            if (_script->initFunction)
            {
                auto _result = _script->initFunction();

                if (!_result.valid())
                {
                    sol::error _err = _result;
                    ConsoleLib::MessageOutput(_err.what(), 3);
                    printf("\n\n");
                }
            }

        ConsoleLib::MessageOutput("Initialization complete!\n", 1);
        ConsoleLib::MessageOutput("Press 'F1' to reload all scripts, press 'F2' to toggle the console, press 'F3' to set execution frequency.\n\n", 0);

        _msClock = chrono::high_resolution_clock::now();
        _sClock = chrono::high_resolution_clock::now();

        return 0;
    }

    void __declspec(dllexport) ExecuteLUA()
    {
        if (_requestedReset == false)
        {
            auto _currTime = chrono::high_resolution_clock::now();
            auto _msTime = std::chrono::duration_cast<std::chrono::milliseconds>(_currTime - _msClock).count();
            auto _sTime = std::chrono::duration_cast<std::chrono::milliseconds>(_currTime - _sClock).count();

            if (GetKeyState(VK_F3) & 0x8000 && _funcThreeState)
            {
                switch (_backend->frameLimit)
                {
                case 16:
                    _backend->frameLimit = 8;
                    ConsoleLib::MessageOutput("Frequency set to 120Hz.\n", 0);
                    break;
                case 8:
                    _backend->frameLimit = 4;
                    ConsoleLib::MessageOutput("Frequency set to 240Hz.\n", 0);
                    break;
                case 4:
                    _backend->frameLimit = 16;
                    ConsoleLib::MessageOutput("Frequency set to 60Hz.\n", 0);
                    break;
                }

                _sTime = 0;
                _funcThreeState = false;
                _sClock = chrono::high_resolution_clock::now();
            }
            if (GetKeyState(VK_F2) & 0x8000 && _funcTwoState)
            {
                if (_showConsole)
                {
                    ShowWindow(GetConsoleWindow(), SW_HIDE);
                    _showConsole = false;
                }

                else
                {
                    ShowWindow(GetConsoleWindow(), SW_RESTORE);
                    _showConsole = true;
                }

                _sTime = 0;
                _funcTwoState = false;
                _sClock = chrono::high_resolution_clock::now();
            }
            if (GetKeyState(VK_F1) & 0x8000 && _funcOneState)
            {
                _requestedReset = true;

                _sTime = 0;
                _funcOneState = false;
                _sClock = chrono::high_resolution_clock::now();
            }

            for (int i = 0; i < _backend->loadedScripts.size(); i++)
            {
                auto _script = _backend->loadedScripts[i];

                if (_script->frameFunction)
                {
                    auto _result = _script->frameFunction();

                    if (!_result.valid())
                    {
                        sol::error _err = _result;
                        ConsoleLib::MessageOutput(_err.what(), 3);
                        printf("\n\n");

                        _backend->loadedScripts.erase(_backend->loadedScripts.begin() + i);
                    }
                }
            }

            _msClock = chrono::high_resolution_clock::now();

            if (_sTime > 250)
            {
                _funcOneState = true;
                _funcTwoState = true;
                _funcThreeState = true;
                _sClock = chrono::high_resolution_clock::now();
            }
        }

        else
            ResetLUA();
    }

    bool __declspec(dllexport) CheckLUA()
    {
        auto _int = MemoryLib::ReadInt(0);

        if (_int == 0)
            return false;

        return true;
    }

    int __declspec(dllexport) VersionLUA()
    {
        return 128;
    }
}

std::optional<std::uintptr_t> followPointerChain(std::uintptr_t start, const std::vector<std::uintptr_t>& offsets) {
    std::uintptr_t current = start;

    for (auto it = offsets.cbegin(); it != offsets.cend(); ++it) {
        if (current == 0) return {};

        if (it != offsets.cend() - 1) {
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

LONG WINAPI crashHandler(PEXCEPTION_POINTERS exceptionPointers) {
    HANDLE file = CreateFileA("CrashDump.dmp", GENERIC_READ | GENERIC_WRITE, 0,
        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (file != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION mdei;

        mdei.ThreadId = GetCurrentThreadId();
        mdei.ExceptionPointers = exceptionPointers;
        mdei.ClientPointers = TRUE;

        (*writeDumpProc)(GetCurrentProcess(), GetCurrentProcessId(),
            file, MiniDumpNormal, (exceptionPointers != 0) ? &mdei : 0, 0, 0);

        CloseHandle(file);
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

bool hookGame(std::uint64_t moduleAddress) {
    static_assert(sizeof(std::uint64_t) == sizeof(std::uintptr_t));

    SetUnhandledExceptionFilter(crashHandler);

    const std::vector<std::uintptr_t> frameProcOffsets{ 0x3E8, 0x0, 0x20 };
    const std::vector<std::uintptr_t> graphicsProcOffsets{ 0x2D8 };

    std::uintptr_t pointerStruct = moduleAddress + gameInfo->pointerStructOffset;

    if (auto ptr = followPointerChain(pointerStruct, frameProcOffsets)) {
        frameProcPtr = reinterpret_cast<GameFrameProc*>(*ptr);
    } else {
        return false;
    }

    if (*frameProcPtr == nullptr) return false;

    DWORD originalProt = 0;
    VirtualProtect(frameProcPtr, sizeof(frameProcPtr), PAGE_READWRITE, &originalProt);
    frameProc = *frameProcPtr;
    *frameProcPtr = frameHook;
    VirtualProtect(frameProcPtr, sizeof(frameProcPtr), originalProt, &originalProt);

    return true;
}

DWORD WINAPI entry(LPVOID lpParameter) {
    char modulePath[MAX_PATH];
    GetModuleFileNameA(nullptr, modulePath, MAX_PATH);

    std::string_view moduleName = modulePath;
    std::string_view gamesPath = modulePath;
    std::size_t pos = moduleName.rfind("\\");
    if (pos != std::string_view::npos) {
        moduleName.remove_prefix(pos + 1);
        gamesPath.remove_suffix(gamesPath.size() - pos);
    }

    auto entry = gameInfos.find(moduleName);
    if (entry != gameInfos.end()) {
        gameInfo = entry->second;
    } else {
        return 0;
    }

    uint64_t moduleAddress = (uint64_t)GetModuleHandleA(nullptr);
    uint64_t baseAddress = moduleAddress + gameInfo->baseAddress;

    vector<string> scriptPaths = {};
    fs::path scriptLocationsFile = fs::path{ gamesPath } / "LuaScriptLocations.txt";
    if (fs::exists(scriptLocationsFile)) {
        string gameScriptsKey = "[" + string(gameInfo->scriptsPath) + "]";

        ifstream input(scriptLocationsFile);
        string line;
        BOOL inCorrectGame = false;
        while (getline(input, line)) {
            size_t length = line.length();
            if (length == 0) {
                continue;
            }

            // Some crude parsing going on here, but seems preferable to bundling an entire parsing library
            if (line.at(0) == '[') {
                if (line == gameScriptsKey) {
                    inCorrectGame = true;
                }
                else {
                    inCorrectGame = false;
                }
            }
            else {
                if (inCorrectGame) {
                    fs::path scriptPath = fs::path{ line };
                    if (fs::exists(scriptPath)) {
                        scriptPaths.push_back(scriptPath.u8string());
                    }
                }
            }
        }

        input.close();
    }

    if (scriptPaths.empty()) {
        // Attempt to fall back to the scripts folder in Documents
        char scriptsPath[MAX_PATH];
        SHGetFolderPathA(0, CSIDL_MYDOCUMENTS, nullptr, 0, scriptsPath);
        std::strcat(scriptsPath, "\\KINGDOM HEARTS HD 1.5+2.5 ReMIX\\scripts");

        fs::path gameScriptsPath = fs::path{ scriptsPath } / gameInfo->scriptsPath;
        if (fs::exists(gameScriptsPath)) {
            scriptPaths.push_back(gameScriptsPath.u8string());
        }
    }

    if (!scriptPaths.empty()) {
        AllocConsole();
        std::freopen("CONOUT$", "w", stdout);

        if (EntryLUA(GetCurrentProcessId(), GetCurrentProcess(), baseAddress, scriptPaths) == 0) {
            // TODO: Hook after game initialization is done.
            while (!hookGame(moduleAddress)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
            }
        } else {
            std::cout << "Failed to initialize internal LuaBackend!" << std::endl;
        }
    }

    return 0;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
    static HMODULE dbgHelp = nullptr;

    switch (fdwReason) {
        case DLL_PROCESS_ATTACH: {
            char dllPath[MAX_PATH];
            GetSystemDirectoryA(dllPath, MAX_PATH);
            std::strcat(dllPath, "\\DBGHELP.dll");
            dbgHelp = LoadLibraryA(dllPath);
            writeDumpProc = (MiniDumpWriteDumpProc)GetProcAddress(dbgHelp, "MiniDumpWriteDump");

            if (CreateThread(nullptr, 0, entry, nullptr, 0, nullptr) == nullptr) {
                return FALSE;
            }

            break;
        }
        case DLL_PROCESS_DETACH: {
            FreeLibrary(dbgHelp);
            break;
        }
        default:
            break;
    }

    return TRUE;
}

extern "C" __declspec(dllexport) BOOL WINAPI MiniDumpWriteDump(
    HANDLE hProcess,
    DWORD ProcessId,
    HANDLE hFile,
    MINIDUMP_TYPE DumpType,
    PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
    PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
    PMINIDUMP_CALLBACK_INFORMATION CallbackParam
) {
    return writeDumpProc(hProcess, ProcessId, hFile, DumpType, ExceptionParam, UserStreamParam, CallbackParam);
}
