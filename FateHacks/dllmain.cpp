#include "pch.h"

#include <vector>
#include <iostream>

#include <detours/detours.h>

namespace
{

struct DetourData
{
    PVOID *targetFunction;
    PVOID detourFunction;

    DetourData(PVOID *targetFunction, PVOID detourFunction)
        : targetFunction(targetFunction), detourFunction(detourFunction) {}
};

LONG DetourCreate(PVOID *targetFunction, PVOID detourFunction)
{
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

LONG DetourRemove(PVOID *targetFunction, PVOID detourFunction)
{
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

std::wstring FormatError(DWORD lastError)
{
    LPWSTR message;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, lastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&message, 0, NULL);
    return message;
}

template <typename FuncT>
FuncT AddrToFuncPtr(uintptr_t addr)
{
    static_assert(std::is_function_v<FuncT> || std::is_member_function_pointer_v<FuncT>, "This only works for function pointers!");

    union
    {
        void *p;
        FuncT type;
    };
    p = reinterpret_cast<void *>(addr);
    return type;
}

template<typename FuncT>
PVOID FuncPtrToPVoid(FuncT f)
{
    static_assert(std::is_function_v<FuncT> || std::is_member_function_pointer_v<FuncT>, "This only works for function pointers!");

    union
    {
        FuncT pf;
        void* p;
    };
    pf = f;
    return p;
}

BOOL CreateDebuggingConsole()
{
    if (!AllocConsole())
    {
        std::wcerr << "AllocConsole() " << FormatError(GetLastError()) << std::endl;
        return FALSE;
    }
    if (!SetConsoleTitle(L"FATE Hack"))
    {
        std::wcerr << "SetConsoleTitle() " << FormatError(GetLastError()) << std::endl;
        return FALSE;
    }

    FILE* f;
    char errDesc[0x40] = { 0 };
    if (errno_t err = freopen_s(&f, "CONOUT$", "w", stdout))
    {
        std::wcerr << "freopen_s(stdout) " << strerror_s<sizeof(errDesc)>(errDesc, err);
        return FALSE;
    }
    if (errno_t err = freopen_s(&f, "CONOUT$", "w", stderr))
    {
        std::wcerr << "freopen_s(stderr) " << strerror_s<sizeof(errDesc)>(errDesc, err);
        return FALSE;
    }

    return TRUE;
}

BOOL DestroyDebuggingConsole()
{
    if (!FreeConsole())
    {
        std::wcerr << "FreeConsole() " << FormatError(GetLastError()) << std::endl;
        return FALSE;
    }
    fclose(stdout);
    fclose(stderr);

    return TRUE;
}

std::vector<DetourData> detours;

} // namespace

struct CMouseHandler
{
    enum class EButton : int
    {
        LEFT_CLICK,
        RIGHT_CLICK,
    };

    char buttonData[0x4];           // 0x00

    static bool (CMouseHandler:: *ButtonPressed)(EButton key);
    static bool (CMouseHandler:: *ButtonHeld)(EButton key);
    static bool (CMouseHandler:: *ButtonDoubleClicked)(EButton key);
};

bool (CMouseHandler:: *CMouseHandler::ButtonPressed)(EButton key) = AddrToFuncPtr<decltype(ButtonPressed)>(0x5933B9);
bool (CMouseHandler:: *CMouseHandler::ButtonHeld)(EButton key) = AddrToFuncPtr<decltype(ButtonHeld)>(0x5933D3);
bool (CMouseHandler:: *CMouseHandler::ButtonDoubleClicked)(EButton key) = AddrToFuncPtr<decltype(ButtonDoubleClicked)>(0x5933EE);

#pragma pack(push, 1)
struct CCharacter
{
    static void (CCharacter:: *GiveGold)(int amount);
};

void (CCharacter:: *CCharacter::GiveGold)(int amount) = AddrToFuncPtr<decltype(GiveGold)>(0x59DEB3);

struct CGameUI
{
    char _pad00[0x504];                 // 0x000
    CMouseHandler mouse;                // 0x504
    char _pad01[0x70];                  // 0x508
    CCharacter *character;              // 0x578
};

static_assert(offsetof(CGameUI, mouse) == 0x504);
static_assert(offsetof(CGameUI, character) == 0x578);

struct CGameClient
{
    char _pad00[0x618];                 // 0x000
    struct CLevel *level;               // 0x618
    char _pad03[0x40];                  // 0x61C
    CGameUI *ui;                        // 0x65C

    static void (CGameClient:: *Update)(void *id3dDevice, HWND handle, float unk);
    void UpdateDetour(void *id3dDevice, HWND handle, float unk)
    {
        if ((this->ui->mouse.*CMouseHandler::ButtonPressed)(CMouseHandler::EButton::LEFT_CLICK))
        {
            (this->ui->character->*CCharacter::GiveGold)(100);
        }

        (this->*Update)(id3dDevice, handle, unk);
    }
};
#pragma pack(pop)

static_assert(offsetof(CGameClient, level) == 0x618);
static_assert(offsetof(CGameClient, ui) == 0x65C);

void (CGameClient:: *CGameClient::Update)(void *id3dDevice, HWND handle, float unk) = AddrToFuncPtr<decltype(Update)>(0x482BD5);

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )    
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
        {
            if (!CreateDebuggingConsole())
                return FALSE;

            DetourData updateDetour(reinterpret_cast<PVOID *>(&CGameClient::Update), FuncPtrToPVoid(&CGameClient::UpdateDetour));
            LONG result = DetourCreate(updateDetour.targetFunction, updateDetour.detourFunction);
            if (result == NO_ERROR)
                detours.push_back(updateDetour);
        }
        break;
        case DLL_PROCESS_DETACH:
        {
            if (!DestroyDebuggingConsole())
                return FALSE;

            for (DetourData i : detours)
                DetourRemove(i.targetFunction, i.detourFunction);
        }
        break;
    }
    return TRUE;
}

