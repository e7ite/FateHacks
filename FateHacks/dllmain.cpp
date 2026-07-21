#define WIN32_LEAN_AND_MEAN  // Exclude rarely-used stuff from Windows headers

// clang-format off
// WinUser.h does not seem to be self-contained, and need this to be before it.
#include <Windows.h>
// clang-format on

#include <WinUser.h>

#include <iostream>
#include <string>

#include "abi.hpp"
#include "client.hpp"
#include "detour.hpp"
#include "input.hpp"
#include "render.hpp"
#include "stl.hpp"

// client.hpp/input.hpp/render.hpp's contents live in `namespace fate`; the
// structs below still defined here (not yet split out) name them unqualified,
// so bring them in by name. This goes away once those structs move to their
// own modules too.
using ::fate::CCharacter;
using ::fate::CGameUI;
using ::fate::CMouseHandler;
using ::fate::CRefManager;
using ::fate::IDirect3DDevice8;

// All desired custom desired should be placed here, as it is passed all the
// essential game structures from the CGameClient::Update detour, which runs on
// every frame.
void CheatMain(struct CGameClient* client, struct IDirect3DDevice8* id3dDevice);

namespace {

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

#pragma pack(push, 1)
struct CGameClient {
  char _pad00[0x08];                                  // 0x000
  CRefManager* refManager;                            // 0x008
  char _pad01[0x544 - (sizeof(CRefManager*) + 0x8)];  // 0x2A0
  IDirect3DDevice8* id3dDevice;                       // 0x544
  char _pad02[0xD0];                                  // 0x548
  struct CLevel* level;                               // 0x618
  char _pad03[0x40];                                  // 0x61C
  CGameUI* ui;                                        // 0x65C

  // This is the main detour for this cheat, which redirects code execution to
  // the cheat entry point located below.
  static void (CGameClient::*Update)(IDirect3DDevice8* id3dDevice, HWND handle,
                                     float unk);
  void UpdateDetour(IDirect3DDevice8* id3dDevice, HWND handle, float unk) {
    void CheatMain(CGameClient * client, IDirect3DDevice8 * id3dDevice);
    CheatMain(this, id3dDevice);

    (this->*Update)(id3dDevice, handle, unk);
  }
};
#pragma pack(pop)

static_assert(offsetof(CGameClient, id3dDevice) == 0x544);
static_assert(offsetof(CGameClient, refManager) == 0x8);
static_assert(offsetof(CGameClient, level) == 0x618);
static_assert(offsetof(CGameClient, ui) == 0x65C);

void (CGameClient::* CGameClient::Update)(IDirect3DDevice8* id3dDevice,
                                          HWND handle, float unk) =
    AddrToFuncPtr<decltype(Update)>(0x482BD5);

void CheatMain(CGameClient* client, IDirect3DDevice8* id3dDevice) {
  CGameUI* ui = client->ui;

  // Give gold on left-click. This runs in the Update phase (via UpdateDetour),
  // where the "just pressed" mouse state is still fresh. (The overlay text is
  // drawn in CGameUI::RenderDetour, which runs in the render phase.)
  if (!(ui->*CGameUI::Paused)() && (ui->mouse.*CMouseHandler::ButtonPressed)(
                                       CMouseHandler::EButton::LEFT_CLICK)) {
    (ui->character->*CCharacter::GiveGold)(100);
  }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call,
                      LPVOID lpReserved) {
  switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH: {
      if (!CreateDebuggingConsole()) {
        return FALSE;
      }

      if (!AttachDetour(reinterpret_cast<PVOID*>(&CGameClient::Update),
                        FuncPtrToPVoid(&CGameClient::UpdateDetour))) {
        return FALSE;
      }
      if (!AttachDetour(reinterpret_cast<PVOID*>(&CGameUI::Render),
                        FuncPtrToPVoid(&CGameUI::RenderDetour))) {
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
