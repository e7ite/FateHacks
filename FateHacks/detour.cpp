#include "detour.hpp"

#include <detours/detours.h>

#include <vector>

namespace {

struct DetourEntry {
  PVOID* targetFunction;
  PVOID detourFunction;
};

std::vector<DetourEntry> gDetours;

}  // namespace

bool AttachDetour(PVOID* targetFunction, PVOID detourFunction) {
  // Initiate Detour Transcation API
  DetourTransactionBegin();

  // Enlists Current Thread in Transaction to Appropriately Update
  // Instruction Pointers for That Thread
  DetourUpdateThread(GetCurrentThread());

  // Allocates the Detour for the Target Function
  DetourAttach(targetFunction, detourFunction);

  // Overwrites the first instruction in the target function to jmp
  // to Detour before returning to target function to restore program flow
  if (DetourTransactionCommit() != NO_ERROR) {
    return false;
  }

  gDetours.push_back({targetFunction, detourFunction});
  return true;
}

void DetachAllDetours() {
  for (const DetourEntry& entry : gDetours) {
    // Initiate Detour Transcation API
    DetourTransactionBegin();

    // Enlists Current Thread in Transaction to Appropriately Update
    // Instruction Pointers for That Thread
    DetourUpdateThread(GetCurrentThread());

    // Deallocates the Detour for the Target Function
    DetourDetach(entry.targetFunction, entry.detourFunction);

    // Restores overwritten instructions of Target Function
    // and restores Target Function Pointer to point to original
    // function
    DetourTransactionCommit();
  }
  gDetours.clear();
}
