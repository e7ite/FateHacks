#ifndef FATEHACKS_CLIENT_HPP_
#define FATEHACKS_CLIENT_HPP_

// Top-level game objects: the player character, the in-game UI, and the game
// client that owns them (the object our detours attach to).

#include "input.hpp"
#include "render.hpp"

namespace fate {

enum class EEffectType : int {
  DamageDealtBonus = 12,
};

struct CEffect {
  char _pad00[0x8];
  EEffectType type;
  int activation;
  char _pad01[0x14 - (0xC + sizeof(activation))];
  float value;
  char _pad02[0xB4 - (0x14 + sizeof(value))];

  static void (CEffect::*CopyConstruct)(const CEffect* source);
};

static_assert(sizeof(CEffect) == 0xB4);
static_assert(offsetof(CEffect, type) == 0x8);
static_assert(offsetof(CEffect, activation) == 0xC);
static_assert(offsetof(CEffect, value) == 0x14);

struct CEffectList {
  char _pad00[0x4];
  CEffect** begin;
  CEffect** end;
  char _pad01[0x10 - (0x8 + sizeof(end))];
};

static_assert(sizeof(CEffectList) == 0x10);
static_assert(offsetof(CEffectList, begin) == 0x4);
static_assert(offsetof(CEffectList, end) == 0x8);

struct CItem {
  char _pad00[0x2F8];
  CEffectList effects[2];

  // Adds `effect` to this item, taking ownership: if an effect of the same
  // type already sits in `effect`'s activation bucket, its value is merged and
  // `effect` is freed; otherwise `effect` is inserted directly into that
  // bucket.
  static void (CItem::*AddNewEffect)(CEffect* effect);
};

static_assert(offsetof(CItem, effects) == 0x2F8);

struct CInventory {
  char _pad00[0x74];
  float effectValues[73];

  // Re-sums effectValues from scratch from every equipped item's own CEffects.
  // The game's own CInventory recompute; call it after adding an effect so the
  // change is felt immediately rather than at the game's next recompute.
  static void (CInventory::*CalculateEffectValues)();
};

static_assert(offsetof(CInventory, effectValues) == 0x74);

#pragma pack(push, 1)

struct CCharacter {
  char _pad00[0x2A8];
  CItem* weapon;
  char _pad01[0x320 - (0x2A8 + sizeof(weapon))];
  int playerIndex;
  char _pad02[0x458 - (0x320 + sizeof(playerIndex))];
  CInventory* inventory;

  static void (CCharacter::*GiveGold)(int amount);

  static void (CCharacter::*TakeDamage)(struct CLevel* level,
                                        CCharacter* attacker, float damage,
                                        char showEffects);
  void TakeDamageDetour(struct CLevel* level, CCharacter* attacker,
                        float damage, char showEffects);

  // Recomputes this character's final combat stats from its inventory's
  // effectValues and active buffs. Call after the inventory recompute for an
  // item-effect change to reach combat.
  static void (CCharacter::*CalculateEffectValues)();
};

static_assert(offsetof(CCharacter, weapon) == 0x2A8);
static_assert(offsetof(CCharacter, playerIndex) == 0x320);
static_assert(offsetof(CCharacter, inventory) == 0x458);

struct CGameUI {
  CKeyHandler keyboard;
  char _pad00[0x504 - (0x00 + sizeof(keyboard))];
  CMouseHandler mouse;
  char _pad01a[0x518 - (0x504 + sizeof(mouse))];
  int mouseX;
  int mouseY;
  char _pad01b[0x524 - (0x51C + sizeof(mouseY))];
  struct CSettings* settings;
  CRefManager* refManager;
  struct CGameStateManager* gameStateManager;
  char _pad02a[0x564 - (0x52C + sizeof(gameStateManager))];
  struct CMaterial* fontMaterial;
  char _pad02b[0x570 - (0x564 + sizeof(fontMaterial))];
  struct CFontMetric* font;
  char _pad03[0x578 - (0x570 + sizeof(font))];
  CCharacter* character;

  static bool (CGameUI::*Paused)();

  static bool (CGameUI::*Render)(IDirect3DDevice8* id3dDevice,
                                 struct CGameClient* client, void* unk,
                                 void* unk2);
  bool RenderDetour(IDirect3DDevice8* id3dDevice, struct CGameClient* client,
                    void* unk, void* unk2);
};

static_assert(offsetof(CGameUI, keyboard) == 0x000);
static_assert(offsetof(CGameUI, mouse) == 0x504);
static_assert(offsetof(CGameUI, mouseX) == 0x518);
static_assert(offsetof(CGameUI, mouseY) == 0x51C);
static_assert(offsetof(CGameUI, settings) == 0x524);
static_assert(offsetof(CGameUI, refManager) == 0x528);
static_assert(offsetof(CGameUI, gameStateManager) == 0x52C);
static_assert(offsetof(CGameUI, fontMaterial) == 0x564);
static_assert(offsetof(CGameUI, font) == 0x570);
static_assert(offsetof(CGameUI, character) == 0x578);

struct CGameClient {
  char _pad00[0x08];
  CRefManager* refManager;
  char _pad01[0x544 - (0x8 + sizeof(refManager))];
  IDirect3DDevice8* id3dDevice;
  char _pad02[0x618 - (0x544 + sizeof(id3dDevice))];
  struct CLevel* level;
  char _pad03[0x65C - (0x618 + sizeof(level))];
  CGameUI* ui;

  // The main detour for this cheat: it redirects execution to the cheat entry
  // point (CheatMain) every frame. Body is in client.cpp.
  static void (CGameClient::*Update)(IDirect3DDevice8* id3dDevice, HWND handle,
                                     float unk);
  void UpdateDetour(IDirect3DDevice8* id3dDevice, HWND handle, float unk);
};
#pragma pack(pop)

static_assert(offsetof(CGameClient, refManager) == 0x8);
static_assert(offsetof(CGameClient, id3dDevice) == 0x544);
static_assert(offsetof(CGameClient, level) == 0x618);
static_assert(offsetof(CGameClient, ui) == 0x65C);

// Multiplies `damage` by `multiplier` when `attacker_is_player` is true, so
// the damage multiplier cheat only affects damage the player (or a
// player-controlled pet) deals; damage taken passes through unchanged. Kept
// as a pure function, separate from the detour that calls it, so it can be
// unit tested without the game running.
float ApplyDamageMultiplier(float damage, bool attacker_is_player,
                            int multiplier);

// Finds the first effect in [begin, end) whose `type` matches, or nullptr if
// none match. Skips null entries rather than dereferencing them: this walks a
// raw pointer range straight from game memory, where a null slot can occur.
// Kept pure so it can be unit tested without the game running.
CEffect* FindEffect(CEffect* const* begin, CEffect* const* end,
                    EEffectType type);

// Adds `delta` to `item`'s Damage Dealt Bonus by inserting or growing a
// CEffect of that type on the item -- the same representation the game uses
// for a "+X Damage Dealt Bonus" magic affix -- so the game's own recompute of
// an equipped item's stat totals includes it going forward.
//
// If a Damage Dealt Bonus effect already exists, just adds to its value (a
// plain field write, no allocation). Otherwise it copy-constructs a new
// CEffect from any existing effect on the item (the only safe way to build
// one, since a CEffect's internal members can't be constructed by hand),
// repoints the copy to be a bucket-0 Damage Dealt Bonus effect, and hands it
// to the game's own AddNewEffect. Returns false only if the item has no
// effect at all to copy from.
//
// Calls game functions directly (the copy constructor, AddNewEffect), so only
// its no-allocation path is exercised by the unit tests.
bool AddDamageDealtBonusEffect(CItem* item, float delta);

// Hooks CGameClient::Update and CGameUI::Render so UpdateDetour/RenderDetour
// run every frame. Called once from DllMain; returns false if either hook
// fails.
bool InstallClientDetours();

}  // namespace fate

#endif  // FATEHACKS_CLIENT_HPP_
