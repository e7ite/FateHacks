#ifndef FATEHACKS_STL_HPP_
#define FATEHACKS_STL_HPP_

// MSVC's STL container layout isn't ABI-stable across compiler versions, and
// the game was built with an older MSVC than this DLL is, so std::string/
// std::vector here don't match the layout the game's compiled code expects.
// We bind directly to the game's own entry points and layout instead.

#include <cstddef>

#include "abi.hpp"

namespace fate {

// Needed because it is not safe for the DLL to allocate heap memory, so we
// should use the game's allocation methods. (Defined in stl.cpp.)
extern void*(__cdecl* GameOperatorNew)(size_t size);
extern void(__cdecl* GameOperatorDelete)(void*);

// Needed because the standard library implementation used by the game most
// likely is not the same as the one included by MSVC here.
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

}  // namespace fate

#endif  // FATEHACKS_STL_HPP_
