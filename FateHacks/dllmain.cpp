#define WIN32_LEAN_AND_MEAN  // Exclude rarely-used stuff from Windows headers

// clang-format off
// WinUser.h does not seem to be self-contained, and need this to be before it.
#include <Windows.h>
// clang-format on

#include <WinUser.h>
#include <detours/detours.h>

#include <iostream>
#include <memory>
#include <string>
#include <vector>

// All desired custom desired should be placed here, as it is passed all the
// essential game structures from the CGameClient::Update detour, which runs on
// every frame.
void CheatMain(struct CGameClient* client, struct IDirect3DDevice8* id3dDevice);

namespace {

struct DetourData {
  PVOID* targetFunction;
  PVOID detourFunction;

  DetourData(PVOID* targetFunction, PVOID detourFunction)
      : targetFunction(targetFunction), detourFunction(detourFunction) {}
};

LONG DetourCreate(PVOID* targetFunction, PVOID detourFunction) {
  // Initiate Detour Transcation API
  DetourTransactionBegin();

  // Enlists Current Thread in Transaction to Appropriately Update
  // Instruction Pointers for That Thread
  DetourUpdateThread(GetCurrentThread());

  // Allocates the Detour for the Target Function
  DetourAttach(targetFunction, detourFunction);

  // Overwrites the first instruction in the target function to jmp
  // to Detour before returning to target function to restore program flow
  LONG resultStatus = DetourTransactionCommit();
  return resultStatus;
}

LONG DetourRemove(PVOID* targetFunction, PVOID detourFunction) {
  // Initiate Detour Transcation API
  DetourTransactionBegin();

  // Enlists Current Thread in Transaction to Appropriately Update
  // Instruction Pointers for That Thread
  DetourUpdateThread(GetCurrentThread());

  // Deallocates the Detour for the Target Function
  DetourDetach(targetFunction, detourFunction);

  // Restores overwritten instructions of Target Function
  // and restores Target Function Pointer to point to original
  // function
  LONG resultStatus = DetourTransactionCommit();
  return resultStatus;
}

std::wstring FormatError(DWORD lastError) {
  LPWSTR message;
  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                    FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL, lastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPWSTR)&message, 0, NULL);
  return message;
}

// Needed because we need to convert game subroutines into member function
// pointers. Definitely UB, but works for this code, which isn't going to be
// ported to another platform any time soon.
template <typename FuncT>
FuncT AddrToFuncPtr(uintptr_t addr) {
  static_assert(
      std::is_function_v<FuncT> || std::is_member_function_pointer_v<FuncT>,
      "This only works for function pointers!");

  union {
    void* p;
    FuncT type;
  };
  p = reinterpret_cast<void*>(addr);
  return type;
}

// Needed because member function pointers cannot be casted into any other
// pointer type. Definitely UB, but works for this code, which isn't going to be
// ported to another platform any time soon.
template <typename FuncT>
PVOID FuncPtrToPVoid(FuncT f) {
  static_assert(
      std::is_function_v<FuncT> || std::is_member_function_pointer_v<FuncT>,
      "This only works for function pointers!");

  union {
    FuncT pf;
    void* p;
  };
  pf = f;
  return p;
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

std::vector<DetourData> gDetours;

}  // namespace

// Needed because it is not safe for the DLL to allocate heap memory, so we
// should use the game's allocation methods.
void*(__cdecl* GameOperatorNew)(size_t size) =
    reinterpret_cast<void* (*)(size_t)>(0x6364DCUL);
void(__cdecl* GameOperatorDelete)(void*) =
    reinterpret_cast<void (*)(void*)>(0x637E4BUL);

#pragma pack(push, 1)
struct CKeyHandler {
  char _pad00[0x4];  // 0x00

  static bool (CKeyHandler::*KeyPressed)(unsigned int key);
  static bool (CKeyHandler::*KeyHeld)(unsigned int key);
};

bool (CKeyHandler::* CKeyHandler::KeyPressed)(unsigned int key) =
    AddrToFuncPtr<decltype(KeyPressed)>(0x592BD1);
bool (CKeyHandler::* CKeyHandler::KeyHeld)(unsigned int key) =
    AddrToFuncPtr<decltype(KeyPressed)>(0x592BEC);

struct IDirect3DDevice8 {
  CKeyHandler keyboard;  // 0x00
};

struct CMouseHandler {
  enum class EButton : int {
    LEFT_CLICK,
    RIGHT_CLICK,
  };

  char buttonData[0x4];  // 0x00

  static bool (CMouseHandler::*ButtonPressed)(EButton key);
  static bool (CMouseHandler::*ButtonHeld)(EButton key);
  static bool (CMouseHandler::*ButtonDoubleClicked)(EButton key);
};

bool (CMouseHandler::* CMouseHandler::ButtonPressed)(EButton key) =
    AddrToFuncPtr<decltype(ButtonPressed)>(0x5933B9);
bool (CMouseHandler::* CMouseHandler::ButtonHeld)(EButton key) =
    AddrToFuncPtr<decltype(ButtonHeld)>(0x5933D3);
bool (CMouseHandler::* CMouseHandler::ButtonDoubleClicked)(EButton key) =
    AddrToFuncPtr<decltype(ButtonDoubleClicked)>(0x5933EE);

struct CCharacter {
  static void (CCharacter::*GiveGold)(int amount);
};

void (CCharacter::* CCharacter::GiveGold)(int amount) =
    AddrToFuncPtr<decltype(GiveGold)>(0x59DEB3);

struct CConfirmMenu {
  char _pad00[0x594];  // 0x00

  static void (CConfirmMenu::*Render)(IDirect3DDevice8* id3dDevice);

  // Using this factory method because we are invoking a constructor of the
  // game code, and we want to make sure we correctly allocate/free the
  // memory required for this structure using the game's operator new and
  // delete.
  static std::unique_ptr<CConfirmMenu, void (*)(CConfirmMenu*)> Create(
      IDirect3DDevice8* id3dDevice, struct CRefManager* refManager,
      struct CSettings* settings, struct CGameStateManager* gameStateManager) {
    auto menu = std::unique_ptr<CConfirmMenu, void (*)(CConfirmMenu*)>(
        reinterpret_cast<CConfirmMenu*>(GameOperatorNew(sizeof(CConfirmMenu))),
        [](CConfirmMenu* menu) { (menu->*Destructor)(); });
    (menu.get()->*Constructor)(id3dDevice, refManager, settings,
                               gameStateManager);
    return menu;
  }

 private:
  CConfirmMenu() = default;

  static CConfirmMenu* (CConfirmMenu::*Constructor)(
      IDirect3DDevice8* id3dDevice, struct CRefManager* refManager,
      struct CSettings* settings, struct CGameStateManager* gameStateManager);
  static void (CConfirmMenu::*Destructor)();
};

CConfirmMenu* (CConfirmMenu::* CConfirmMenu::Constructor)(
    IDirect3DDevice8* id3dDevice, struct CRefManager* refManager,
    struct CSettings* settings, struct CGameStateManager* gameStateManager) =
    AddrToFuncPtr<decltype(Constructor)>(0x477AEC);
void (CConfirmMenu::* CConfirmMenu::Destructor)() =
    AddrToFuncPtr<decltype(Destructor)>(0x477D68);

void (CConfirmMenu::* CConfirmMenu::Render)(IDirect3DDevice8* id3dDevice) =
    AddrToFuncPtr<decltype(Render)>(0x478BB2);

// Needed because the standard library implementation used by the game most
// likely is not the same as the one included by MVSC here.
template <typename T>
struct STLVector {
  static int (STLVector<T>::*size)();

  T* operator[](int idx) { return (this->*opIndex)(idx); }

  static T* (STLVector<T>::*opIndex)(int idx);
};

template <typename T>
int (STLVector<T>::* STLVector<T>::size)() =
    AddrToFuncPtr<decltype(size)>(0x417560);
template <typename T>
T* (STLVector<T>::* STLVector<T>::opIndex)(int idx) =
    AddrToFuncPtr<decltype(opIndex)>(0x41758E);

struct STLString {
  STLString() { (this->*STLString::constructor)(""); }
  STLString(const char* str) { (this->*STLString::constructor)(str); }
  ~STLString() { (this->*STLString::destructor)(0, 0); }

  static char* (STLString::*get)();
  static void (STLString::*constructor)(const char* str);
  static void (STLString::*destructor)(int unk, int unk2);

 private:
  // Size reference from 0x40FB6E mentioning the size of the string in stack is
  // 0x1C, and 0x42D8C3 mentioning the next member set is 0x1C offset after
  // string.
  char __pad00[0x1C];
};

char* (STLString::* STLString::get)() = AddrToFuncPtr<decltype(get)>(0x403808);
void (STLString::* STLString::constructor)(const char* str) =
    AddrToFuncPtr<decltype(constructor)>(0x405BAC);
void (STLString::* STLString::destructor)(int unk, int unk2) =
    AddrToFuncPtr<decltype(destructor)>(0x4035E6);

struct CText {
  char _pad00[0x78];  // 0x00

  static void (CText::*Render)(IDirect3DDevice8* id3dDevice);
  // Rebuilds the mesh from a new string. Use this to edit the text after
  // Create.
  static void (CText::*Update)(IDirect3DDevice8* id3dDevice, STLString* str,
                               float x, float y, float alpha, char unk,
                               int unk2, int unk3, int unk4);

  static std::unique_ptr<CText, void (*)(CText*)> Create(
      IDirect3DDevice8* id3dDevice, struct CMaterial* cMaterial,
      struct CFontMetric* font, STLString* str, float x, float y, float alpha,
      char unk, int unk2, int unk3, int unk4) {
    auto text = std::unique_ptr<CText, void (*)(CText*)>(
        reinterpret_cast<CText*>(GameOperatorNew(sizeof(CText))),
        [](CText* text) { (text->*DeleteCText)(/*flags=*/1); });
    (text.get()->*Constructor)(id3dDevice, cMaterial, font, str, x, y, alpha,
                               unk, unk2, unk3, unk4);
    return text;
  }

 private:
  CText() = default;

  static void (CText::*Constructor)(IDirect3DDevice8* id3dDevice,
                                    struct CMaterial* cMaterial,
                                    struct CFontMetric* font, STLString* str,
                                    float x, float y, float alpha, char unk,
                                    int unk2, int unk3, int unk4);
  static void (CText::*DeleteCText)(int flags);
};

void (CText::* CText::Render)(IDirect3DDevice8* id3dDevice) =
    AddrToFuncPtr<decltype(Render)>(0x449CF3);
void (CText::* CText::Update)(IDirect3DDevice8* id3dDevice, STLString* str,
                              float x, float y, float alpha, char unk, int unk2,
                              int unk3, int unk4) =
    AddrToFuncPtr<decltype(Update)>(0x449060);
void (CText::* CText::Constructor)(
    IDirect3DDevice8* id3dDevice, struct CMaterial* cMaterial,
    struct CFontMetric* font, STLString* str, float x, float y, float alpha,
    char unk, int unk2, int unk3,
    int unk4) = AddrToFuncPtr<decltype(Constructor)>(0x448464);
void (CText::* CText::DeleteCText)(int flags) =
    AddrToFuncPtr<decltype(DeleteCText)>(0x438E5C);

struct MaterialRef {
  char __pad00[0x24];
  STLString part1;
  STLString part2;
  STLString part3;
  STLString part4;
  STLString part5;
};

static_assert(offsetof(MaterialRef, part1) == 0x24);
static_assert(offsetof(MaterialRef, part2) == 0x40);
static_assert(offsetof(MaterialRef, part3) == 0x5C);
static_assert(offsetof(MaterialRef, part4) == 0x78);
static_assert(offsetof(MaterialRef, part5) == 0x94);

struct CRefManager {
  char __pad00[0x50];
  STLVector<MaterialRef*> materialRefs;
};

static_assert(offsetof(CRefManager, materialRefs) == 0x50);

struct CGameUI {
  char _pad00[0x504];                                                 // 0x000
  CMouseHandler mouse;                                                // 0x504
  char _pad01[0x1C];                                                  // 0x508
  struct CSettings* settings;                                         // 0x524
  CRefManager* refManager;                                            // 0x528
  struct CGameStateManager* gameStateManager;                         // 0x52C
  char _pad02a[0x564 - (sizeof(struct CGameStateManager*) + 0x52C)];  // 0x530
  struct CMaterial* fontMaterial;                                     // 0x564
  char _pad02b[0x570 - (sizeof(struct CMaterial*) + 0x564)];          // 0x568
  struct CFontMetric* font;                                           // 0x570
  char _pad03[0x578 - (sizeof(struct CFontMetric*) + 0x570)];         // 0x574
  CCharacter* character;                                              // 0x578

  static bool (CGameUI::*Paused)();

  static bool (CGameUI::*Render)(IDirect3DDevice8* id3dDevice,
                                 struct CGameClient* client, void* unk,
                                 void* unk2);
  bool RenderDetour(IDirect3DDevice8* id3dDevice, struct CGameClient* client,
                    void* unk, void* unk2);
};

static_assert(offsetof(CGameUI, mouse) == 0x504);
static_assert(offsetof(CGameUI, settings) == 0x524);
static_assert(offsetof(CGameUI, refManager) == 0x528);
static_assert(offsetof(CGameUI, gameStateManager) == 0x52C);
static_assert(offsetof(CGameUI, fontMaterial) == 0x564);
static_assert(offsetof(CGameUI, font) == 0x570);
static_assert(offsetof(CGameUI, character) == 0x578);

bool (CGameUI::* CGameUI::Paused)() = AddrToFuncPtr<decltype(Paused)>(0x43759B);

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

std::unique_ptr<CText, void (*)(CText*)> gOverlayText(nullptr, nullptr);

bool (CGameUI::* CGameUI::Render)(IDirect3DDevice8* id3dDevice,
                                  CGameClient* client, void* unk, void* unk2) =
    AddrToFuncPtr<decltype(Render)>(0x4A2E7D);
bool CGameUI::RenderDetour(IDirect3DDevice8* id3dDevice, CGameClient* client,
                           void* unk, void* unk2) {
  // Let the game draw its UI first, then draw ours on top.
  bool status = (this->*Render)(id3dDevice, client, unk, unk2);

  if (gOverlayText == nullptr && fontMaterial != nullptr && font != nullptr) {
    STLString str("Hellfateo world!");
    gOverlayText = CText::Create(id3dDevice, fontMaterial, font, &str, 100, 10,
                                 0.8f, 0, 2, 1024, 768);
  }

  if (gOverlayText) {
    (gOverlayText.get()->*CText::Render)(id3dDevice);
  }

  return status;
}

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

      DetourData updateDetour(reinterpret_cast<PVOID*>(&CGameClient::Update),
                              FuncPtrToPVoid(&CGameClient::UpdateDetour));
      if (DetourCreate(updateDetour.targetFunction,
                       updateDetour.detourFunction) != NO_ERROR) {
        return FALSE;
      }
      gDetours.push_back(updateDetour);

      DetourData renderUIDetour(reinterpret_cast<PVOID*>(&CGameUI::Render),
                                FuncPtrToPVoid(&CGameUI::RenderDetour));
      if (DetourCreate(renderUIDetour.targetFunction,
                       renderUIDetour.detourFunction) != NO_ERROR) {
        return FALSE;
      }
      gDetours.push_back(renderUIDetour);
    } break;
    case DLL_PROCESS_DETACH: {
      if (!DestroyDebuggingConsole()) {
        return FALSE;
      }

      for (DetourData i : gDetours) {
        DetourRemove(i.targetFunction, i.detourFunction);
      }
    } break;
  }
  return TRUE;
}
