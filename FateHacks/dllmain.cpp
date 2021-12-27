#include "pch.h"

#include <vector>
#include <iostream>

#include <detours/detours.h>

namespace
{

struct DetourData
{
    PVOID* targetFunction;
    PVOID detourFunction;

    DetourData(PVOID* targetFunction, PVOID detourFunction)
        : targetFunction(targetFunction), detourFunction(detourFunction) {}
};

LONG DetourCreate(PVOID* targetFunction, PVOID detourFunction)
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

LONG DetourRemove(PVOID* targetFunction, PVOID detourFunction)
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

template <typename R, typename T, typename ...Ts>
R(T::* AddrToMemberFuncPtr(uintptr_t addr))(Ts...)
{
    union
    {
        R(T::*pf)(Ts...);
        void* p;
    };
    p = reinterpret_cast<void*>(addr);
    return pf;
}

template<typename R, typename T, typename ...Ts>
PVOID MemberFuncToPVoid(R(T::* f)(Ts...))
{
    union
    {
        R(T::*pf)(Ts...);
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

struct CGameClient
{
    static void (CGameClient::*Update)(void* id3dDevice, HWND handle, float unk);
    void UpdateDetour(void *id3dDevice, HWND handle, float unk)
    {
        std::wcerr << "Hello world";

        (this->*Update)(id3dDevice, handle, unk);
    }
};

void (CGameClient::* CGameClient::Update)(void* id3dDevice, HWND handle, float unk) = AddrToMemberFuncPtr<void, CGameClient, void*, HWND, float>(0x482BD5);

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

            DetourData updateDetour(reinterpret_cast<PVOID *>(&CGameClient::Update),
                                    MemberFuncToPVoid<void, CGameClient, void *, HWND, float>(&CGameClient::UpdateDetour));
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

