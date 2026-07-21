#ifndef FATEHACKS_DETOUR_HPP_
#define FATEHACKS_DETOUR_HPP_

// Thin wrapper around Microsoft Detours. Not game-specific, so it stays outside
// the `fate` namespace -- any module can hook a function through it.

#define WIN32_LEAN_AND_MEAN
// clang-format off
#include <Windows.h>
// clang-format on

// Attaches a detour redirecting `*targetFunction` to `detourFunction` and
// remembers the pair so it can later be undone by DetachAllDetours(). Returns
// false if the underlying Detours transaction fails.
bool AttachDetour(PVOID* targetFunction, PVOID detourFunction);

// Detaches every detour installed via AttachDetour(), in preparation for the
// DLL unloading.
void DetachAllDetours();

#endif  // FATEHACKS_DETOUR_HPP_
