#ifndef FATEHACKS_ABI_HPP_
#define FATEHACKS_ABI_HPP_

// ABI-level glue for calling into the game's own code: helpers that reinterpret
// raw code addresses as callable (member) function pointers, and vice versa.
// These are generic (not game-specific), so they stay in the global namespace
// rather than `fate`.

#define WIN32_LEAN_AND_MEAN
// clang-format off
// WinUser.h does not seem to be self-contained, and needs Windows.h before it.
#include <Windows.h>
// clang-format on
#include <WinUser.h>

#include <cstdint>
#include <type_traits>

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

#endif  // FATEHACKS_ABI_HPP_
