#ifndef FATEHACKS_CLIENT_HPP_
#define FATEHACKS_CLIENT_HPP_

// Top-level game objects: the player character, the in-game UI, and the game
// client that owns them (the object our detours attach to).

#include "input.hpp"
#include "render.hpp"

namespace fate {

#pragma pack(push, 1)

struct CCharacter {
  static void (CCharacter::*GiveGold)(int amount);
};

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

struct CGameClient {
  char _pad00[0x08];                                  // 0x000
  CRefManager* refManager;                            // 0x008
  char _pad01[0x544 - (sizeof(CRefManager*) + 0x8)];  // 0x2A0
  IDirect3DDevice8* id3dDevice;                       // 0x544
  char _pad02[0xD0];                                  // 0x548
  struct CLevel* level;                               // 0x618
  char _pad03[0x40];                                  // 0x61C
  CGameUI* ui;                                        // 0x65C

  // The main detour for this cheat: it redirects execution to the cheat entry
  // point (CheatMain) every frame. Body is in client.cpp.
  static void (CGameClient::*Update)(IDirect3DDevice8* id3dDevice, HWND handle,
                                     float unk);
  void UpdateDetour(IDirect3DDevice8* id3dDevice, HWND handle, float unk);
};
#pragma pack(pop)

static_assert(offsetof(CGameClient, id3dDevice) == 0x544);
static_assert(offsetof(CGameClient, refManager) == 0x8);
static_assert(offsetof(CGameClient, level) == 0x618);
static_assert(offsetof(CGameClient, ui) == 0x65C);

// Hooks CGameClient::Update and CGameUI::Render so UpdateDetour/RenderDetour
// run every frame. Called once from DllMain; returns false if either hook
// fails.
bool InstallClientDetours();

}  // namespace fate

#endif  // FATEHACKS_CLIENT_HPP_
