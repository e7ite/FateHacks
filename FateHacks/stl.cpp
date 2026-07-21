#include "stl.hpp"

namespace fate {

void*(__cdecl* GameOperatorNew)(size_t size) =
    reinterpret_cast<void* (*)(size_t)>(0x6364DCUL);
void(__cdecl* GameOperatorDelete)(void*) =
    reinterpret_cast<void (*)(void*)>(0x637E4BUL);

char* (STLString::* STLString::get)() = AddrToFuncPtr<decltype(get)>(0x403808);
void (STLString::* STLString::constructor)(const char* str) =
    AddrToFuncPtr<decltype(constructor)>(0x405BAC);
void (STLString::* STLString::destructor)(int unk, int unk2) =
    AddrToFuncPtr<decltype(destructor)>(0x4035E6);

}  // namespace fate
