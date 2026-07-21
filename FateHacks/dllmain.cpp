#define WIN32_LEAN_AND_MEAN  // Exclude rarely-used stuff from Windows headers

// clang-format off
// WinUser.h does not seem to be self-contained, and need this to be before it.
#include <Windows.h>
// clang-format on

#include <WinUser.h>

#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>

#include "client.hpp"
#include "detour.hpp"

namespace {

// In the anonymous namespace so it's visible through the rest of this file
// (DllMain below) without `fate::` qualification.
using ::fate::InstallClientDetours;

std::wstring FormatError(DWORD lastError) {
  LPWSTR message;
  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                    FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL, lastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPWSTR)&message, 0, NULL);
  return message;
}

// Spawns a terminal when the DLL is injected, solely for debugging purposes
BOOL CreateDebuggingConsole() {
  if (!AllocConsole()) {
    std::wcerr << "AllocConsole() " << FormatError(GetLastError()) << std::endl;
    return FALSE;
  }
  if (!SetConsoleTitle(L"FATE Hack")) {
    std::wcerr << "SetConsoleTitle() " << FormatError(GetLastError())
               << std::endl;
    return FALSE;
  }

  FILE* f;
  char errDesc[0x40] = {0};
  if (errno_t err = freopen_s(&f, "CONOUT$", "w", stdout)) {
    std::wcerr << "freopen_s(stdout) "
               << strerror_s<sizeof(errDesc)>(errDesc, err);
    return FALSE;
  }
  if (errno_t err = freopen_s(&f, "CONOUT$", "w", stderr)) {
    std::wcerr << "freopen_s(stderr) "
               << strerror_s<sizeof(errDesc)>(errDesc, err);
    return FALSE;
  }

  return TRUE;
}

// Destroys the terminal created if CreateDebuggingConsole(), was invoked,
// solely for debugging purposes. Perhaps create a type for this and create with
// constructors to ensure this isn't called before a terminal is created?
BOOL DestroyDebuggingConsole() {
  if (!FreeConsole()) {
    std::wcerr << "FreeConsole() " << FormatError(GetLastError()) << std::endl;
    return FALSE;
  }
  fclose(stdout);
  fclose(stderr);

  return TRUE;
}

}  // namespace

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call,
                      LPVOID lpReserved) {
  switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH: {
      if (!CreateDebuggingConsole()) {
        return FALSE;
      }
      if (!InstallClientDetours()) {
        return FALSE;
      }
    } break;
    case DLL_PROCESS_DETACH: {
      if (!DestroyDebuggingConsole()) {
        return FALSE;
      }
      DetachAllDetours();
    } break;
  }
  return TRUE;
}
