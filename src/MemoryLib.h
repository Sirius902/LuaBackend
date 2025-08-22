#pragma once

#include <algorithm>
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

#define WIN32_LEAN_AND_MEAN
#include <psapi.h>
#include <shlobj.h>
#include <tlhelp32.h>
#include <wil/stl.h>
#include <wil/win32_helpers.h>
#include <windows.h>

#include "wil_extra.h"

class MemoryLib {
private:
  static inline STARTUPINFOW _sInfo;
  static inline PROCESS_INFORMATION _pInfo;
  static inline bool _bigEndian = false;

  class protect_lock {
  private:
    std::uintptr_t address;
    std::size_t size;
    DWORD protection;
    bool is_acquired;

  public:
    protect_lock(std::uintptr_t address, std::size_t size)
        : address(address), size(size), protection(0), is_acquired(false) {
      if (VirtualProtect((void*)address, size, PAGE_EXECUTE_READWRITE, &protection) !=
          0)
        is_acquired = true;
    }

    ~protect_lock() {
      if (good())
        VirtualProtect((void*)address, size, protection, &protection);
    }

    bool good() const noexcept { return is_acquired; }
  };

public:
  static inline std::uint64_t ExecAddress;
  static inline std::uint64_t BaseAddress;
  static inline DWORD PIdentifier = 0;
  static inline HANDLE PHandle = NULL;
  static inline std::wstring PName;

  static HMODULE FindBaseAddr(HANDLE InputHandle, std::wstring InputName) {
    HMODULE hMods[1024];
    DWORD cbNeeded;
    unsigned int i;

    if (EnumProcessModules(InputHandle, hMods, sizeof(hMods), &cbNeeded)) {
      for (i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
        std::wstring szModName;
        if (wil::GetModuleFileNameExW(InputHandle, hMods[i], szModName)) {
          if (szModName.find(InputName) != std::wstring::npos)
            return hMods[i];
        }
      }
    }

    return nullptr;
  }

  static DWORD FindProcessId(const std::wstring& processName) {
    PROCESSENTRY32 processInfo;
    processInfo.dwSize = sizeof(processInfo);

    HANDLE processesSnapshot =
        CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    if (processesSnapshot == INVALID_HANDLE_VALUE)
      return 0;

    Process32First(processesSnapshot, &processInfo);
    if (!processName.compare(processInfo.szExeFile)) {
      CloseHandle(processesSnapshot);
      return processInfo.th32ProcessID;
    }

    while (Process32Next(processesSnapshot, &processInfo)) {
      if (!processName.compare(processInfo.szExeFile)) {
        CloseHandle(processesSnapshot);
        return processInfo.th32ProcessID;
      }
    }

    CloseHandle(processesSnapshot);
    return 0;
  }

  static void SetBaseAddr(std::uint64_t InputAddress) {
    BaseAddress = InputAddress;
  }

  static int ExecuteProcess(std::wstring InputName, std::uint64_t InputAddress,
                            bool InputEndian) {
    ZeroMemory(&_sInfo, sizeof(_sInfo));
    _sInfo.cb = sizeof(_sInfo);
    ZeroMemory(&_pInfo, sizeof(_pInfo));

    if (CreateProcessW(InputName.c_str(), NULL, NULL, NULL, TRUE, 0, NULL, NULL,
                       &_sInfo, &_pInfo) == 0)
      return -1;

    BaseAddress = InputAddress;
    _bigEndian = InputEndian;

    return 0;
  };

  static bool LatchProcess(std::wstring InputName, std::uint64_t InputAddress,
                           bool InputEndian) {
    ZeroMemory(&_sInfo, sizeof(_sInfo));
    _sInfo.cb = sizeof(_sInfo);
    ZeroMemory(&_pInfo, sizeof(_pInfo));

    PIdentifier =
        FindProcessId(std::wstring(InputName.begin(), InputName.end()));
    PHandle = OpenProcess(PROCESS_ALL_ACCESS, false, PIdentifier);

    wil_extra::GetProcessImageFileNameW(MemoryLib::PHandle, PName);
    BaseAddress = InputAddress;

    ExecAddress = (std::uint64_t)FindBaseAddr(PHandle, PName);
    _bigEndian = InputEndian;

    if (PHandle == NULL)
      return false;

    return true;
  };

  static void ExternProcess(DWORD InputID, HANDLE InputH,
                            std::uint64_t InputAddress) {
    PIdentifier = InputID;
    PHandle = InputH;

    wil_extra::GetProcessImageFileNameW(MemoryLib::PHandle, PName);

    BaseAddress = InputAddress;
    ExecAddress = (std::uint64_t)FindBaseAddr(PHandle, PName);
  };

  template <typename T,
            std::enable_if_t<std::is_trivially_constructible_v<T>, int> = 0>
  static T readScalarAbsolute(std::uint64_t Address) {
    protect_lock lock(Address, sizeof(T));
    if (!lock.good())
      return 0;

    return *reinterpret_cast<T*>(Address);
  }

  static inline std::uint8_t ReadByteAbsolute(std::uint64_t Address) {
    return readScalarAbsolute<std::uint8_t>(Address);
  }

  static inline std::uint16_t ReadShortAbsolute(std::uint64_t Address) {
    return readScalarAbsolute<std::uint16_t>(Address);
  }

  static inline std::uint32_t ReadIntAbsolute(std::uint64_t Address) {
    return readScalarAbsolute<std::uint32_t>(Address);
  }

  static inline std::uint64_t ReadLongAbsolute(std::uint64_t Address) {
    return readScalarAbsolute<std::uint64_t>(Address);
  }

  static inline float ReadFloatAbsolute(std::uint64_t Address) {
    return readScalarAbsolute<float>(Address);
  }

  static inline bool ReadBoolAbsolute(std::uint64_t Address) {
    return ReadByteAbsolute(Address) != 0;
  }

  static std::vector<std::uint8_t> ReadBytesAbsolute(std::uint64_t Address,
                                                     int Length) {
    std::vector<std::uint8_t> _buffer;

    protect_lock lock(Address, static_cast<std::size_t>(Length));
    if (lock.good()) {
      _buffer.resize(Length);
      std::memcpy(_buffer.data(), (void*)Address, Length);
    }

    return _buffer;
  }

  static std::string ReadStringAbsolute(std::uint64_t Address, int Length) {
    std::string _output;

    protect_lock lock(Address, static_cast<std::size_t>(Length));
    if (lock.good()) {
      _output.resize(Length);
      std::memcpy(_output.data(), (void*)Address, Length);
    }

    return _output;
  }

  template <typename T>
  static T readScalar(std::uint64_t Address, bool absolute = false) {
    if (absolute) {
      return readScalarAbsolute<T>(Address);
    } else {
      return readScalarAbsolute<T>(Address + BaseAddress);
    }
  }

  static inline std::uint8_t ReadByte(std::uint64_t Address,
                                      bool absolute = false) {
    return readScalar<std::uint8_t>(Address, absolute);
  }

  static inline std::uint16_t ReadShort(std::uint64_t Address,
                                        bool absolute = false) {
    return readScalar<std::uint16_t>(Address, absolute);
  }

  static inline std::uint32_t ReadInt(std::uint64_t Address,
                                      bool absolute = false) {
    return readScalar<std::uint32_t>(Address, absolute);
  }

  static inline std::uint64_t ReadLong(std::uint64_t Address,
                                       bool absolute = false) {
    return readScalar<std::uint64_t>(Address, absolute);
  }

  static inline float ReadFloat(std::uint64_t Address, bool absolute = false) {
    return readScalar<float>(Address, absolute);
  }

  static inline bool ReadBool(std::uint64_t Address, bool absolute = false) {
    return ReadByte(Address, absolute) != 0;
  }

  static std::vector<std::uint8_t> ReadBytes(std::uint64_t Address, int Length,
                                             bool absolute = false) {
    if (absolute) {
      return ReadBytesAbsolute(Address, Length);
    } else {
      return ReadBytesAbsolute(Address + BaseAddress, Length);
    }
  }

  static std::string ReadString(std::uint64_t Address, int Length,
                                bool absolute = false) {
    if (absolute) {
      return ReadStringAbsolute(Address, Length);
    } else {
      return ReadStringAbsolute(Address + BaseAddress, Length);
    }
  }

  template <typename T,
            std::enable_if_t<std::is_trivially_copy_assignable_v<T>, int> = 0>
  static void writeScalarAbsolute(std::uint64_t Address, T t) {
    protect_lock lock(Address, sizeof(T));
    if (!lock.good())
      return;

    *reinterpret_cast<T*>(Address) = t;
  }

  static inline void WriteByteAbsolute(std::uint64_t Address,
                                       std::uint8_t Input) {
    writeScalarAbsolute<std::uint8_t>(Address, Input);
  }

  static inline void WriteShortAbsolute(std::uint64_t Address,
                                        std::uint16_t Input) {
    writeScalarAbsolute<std::uint16_t>(Address, Input);
  }

  static inline void WriteIntAbsolute(std::uint64_t Address,
                                      std::uint32_t Input) {
    writeScalarAbsolute<std::uint32_t>(Address, Input);
  }

  static inline void WriteLongAbsolute(std::uint64_t Address,
                                       std::uint64_t Input) {
    writeScalarAbsolute<std::uint64_t>(Address, Input);
  }

  static inline void WriteFloatAbsolute(std::uint64_t Address, float Input) {
    writeScalarAbsolute<float>(Address, Input);
  }

  static inline void WriteBoolAbsolute(std::uint64_t Address, bool Input) {
    WriteByteAbsolute(Address, Input ? 1 : 0);
  }

  static void WriteBytesAbsolute(std::uint64_t Address,
                                 std::vector<std::uint8_t> Input) {
    protect_lock lock(Address, Input.size());
    if (lock.good())
      std::memcpy((void*)Address, Input.data(), Input.size());
  }

  static void WriteStringAbsolute(std::uint64_t Address, std::string Input) {
    protect_lock lock(Address, Input.size());
    if (lock.good())
      std::memcpy((void*)Address, Input.data(), Input.size());
  }

  template <typename T>
  static void writeScalar(std::uint64_t Address, T const& t,
                          bool absolute = false) {
    if (absolute) {
      writeScalarAbsolute<T>(Address, t);
    } else {
      writeScalarAbsolute<T>(Address + BaseAddress, t);
    }
  }

  static inline void WriteByte(std::uint64_t Address, std::uint8_t Input,
                               bool absolute = false) {
    writeScalar<std::uint8_t>(Address, Input, absolute);
  }

  static inline void WriteShort(std::uint64_t Address, std::uint16_t Input,
                                bool absolute = false) {
    writeScalar<std::uint16_t>(Address, Input, absolute);
  }

  static inline void WriteInt(std::uint64_t Address, std::uint32_t Input,
                              bool absolute = false) {
    writeScalar<std::uint32_t>(Address, Input, absolute);
  }

  static inline void WriteLong(std::uint64_t Address, std::uint64_t Input,
                               bool absolute = false) {
    writeScalar<std::uint64_t>(Address, Input, absolute);
  }

  static inline void WriteFloat(std::uint64_t Address, float Input,
                                bool absolute = false) {
    writeScalar<float>(Address, Input, absolute);
  }

  static inline void WriteBool(std::uint64_t Address, bool Input,
                               bool absolute = false) {
    WriteByte(Address, Input ? 1 : 0, absolute);
  }

  static void WriteBytes(std::uint64_t Address, std::vector<std::uint8_t> Input,
                         bool absolute = false) {
    if (absolute) {
      WriteBytesAbsolute(Address, std::move(Input));
    } else {
      WriteBytesAbsolute(Address + BaseAddress, std::move(Input));
    }
  }

  static void WriteString(std::uint64_t Address, std::string Input,
                          bool absolute = false) {
    if (absolute) {
      WriteStringAbsolute(Address, std::move(Input));
    } else {
      WriteStringAbsolute(Address + BaseAddress, std::move(Input));
    }
  }

  static void WriteExec(std::uint64_t Address,
                        std::vector<std::uint8_t> Input) {
    protect_lock lock(Address, Input.size());
    if (lock.good())
      std::memcpy((void*)(Address + ExecAddress), Input.data(), Input.size());
  }

  static inline std::uint64_t GetPointer(std::uint64_t Address,
                                         std::uint64_t Offset,
                                         bool absolute = false) {
    std::uint64_t _temp = ReadLong(Address, absolute);
    return _temp + Offset;
  }

  static inline std::uint64_t GetPointerAbsolute(std::uint64_t Address,
                                                 std::uint64_t Offset) {
    std::uint64_t _temp = ReadLongAbsolute(Address);
    return _temp + Offset;
  }
};
