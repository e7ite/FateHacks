#ifndef FATEHACKS_CLIENT_HPP_
#define FATEHACKS_CLIENT_HPP_

// Top-level game objects: the player character, the in-game UI, and the game
// client that owns them (the object our detours attach to).

#include "input.hpp"
#include "render.hpp"

namespace fate {

// Multiplies `damage` by `multiplier` when `attacker_is_player` is true, so
// the damage multiplier cheat only affects damage the player (or a
// player-controlled pet) deals; damage taken passes through unchanged. Kept
// as a pure function, separate from the detour that calls it, so it can be
// unit tested without the game running.
float ApplyDamageMultiplier(float damage, bool attacker_is_player,
                            int multiplier);

// A stat modifier the game understands: a weapon's Damage Dealt Bonus, an
// armor's resistance, etc. The game defines ~73 modifier types internally
// (STRENGTH=0, DEXTERITY=1, ..., DAMAGEBONUS=12, ...; DAMAGEBONUS shows in
// tooltips as "Damage Dealt Bonus", confirmed in-game); only the one this
// codebase reads or writes is named here.
enum class EEffectType : int {
  DamageDealtBonus = 12,
};

// Number of modifier types the game defines (EEffectType's value space, 0..72);
// the size of CInventory::effectValues.
constexpr int kEffectTypeCount = 73;

// One stat modifier on an item. `type`, `activation`, and `value` are the only
// fields we read or write; the rest is opaque internal state (name strings, a
// sound-effect list, timers) we never touch, only let the game's own
// constructor fill in. Sized to the whole object so it can be heap-allocated
// for the game to construct into.
struct CEffect {
  char _pad00[0x8];  // 0x000
  EEffectType type;  // 0x008
  // Which of the item's effect buckets this belongs in. Bucket 0 is the one
  // an equipped item's stat bonuses are summed from into combat.
  int activation;             // 0x00C
  char _pad01[0x4];           // 0x010
  float value;                // 0x014
  char _pad02[0xB4 - 0x018];  // 0x018

  // Deep-copies `source` into this (raw, freshly allocated memory),
  // initializing every one of a CEffect's internal std::string/std::vector
  // members -- the game's own copy constructor. A valid CEffect can't be built
  // any other way from here, since those members can't be constructed by hand.
  static void (CEffect::*CopyConstruct)(const CEffect* source);
};

static_assert(sizeof(CEffect) == 0xB4);
static_assert(offsetof(CEffect, type) == 0x8);
static_assert(offsetof(CEffect, activation) == 0xC);
static_assert(offsetof(CEffect, value) == 0x14);

// One activation bucket's effects on an item: `begin`/`end` bound a raw
// `CEffect*` range. An item has two of these back to back; bucket 0 is the one
// the game sums an equipped item's stat bonuses from.
struct CEffectList {
  char _pad00[0x4];  // 0x000
  CEffect** begin;   // 0x004
  CEffect** end;     // 0x008
  char _pad01[0x4];  // 0x00C
};

static_assert(offsetof(CEffectList, begin) == 0x4);
static_assert(offsetof(CEffectList, end) == 0x8);
static_assert(sizeof(CEffectList) == 0x10);

// An equippable item (weapon, armor, etc). The game stores far more here than
// these fields; they're the only bytes we know or need.
struct CItem {
  char _pad00[0x2F8];      // 0x000
  CEffectList effects[2];  // 0x2F8

  // Adds `effect` to this item, taking ownership: if an effect of the same
  // type already sits in `effect`'s activation bucket, its value is merged and
  // `effect` is freed; otherwise `effect` is inserted directly into that
  // bucket. The game's own CItem::AddNewEffect.
  static void (CItem::*AddNewEffect)(CEffect* effect);
};

static_assert(offsetof(CItem, effects) == 0x2F8);

// A character's inventory, including the cached per-modifier-type totals combat
// reads.
struct CInventory {
  char _pad00[0x74];  // 0x000

  // Per-`EEffectType` sum of every equipped item's contribution. The game
  // re-fills this from each equipped item's own CEffects (via its recompute
  // below) and combat reads it, so a modifier's change needs to be a CEffect
  // on an item for the game's own recompute to keep including it.
  float effectValues[kEffectTypeCount];  // 0x074

  // Re-sums effectValues from scratch from every equipped item's own CEffects.
  // The game's own CInventory recompute; call it after adding an effect so the
  // change is felt immediately rather than at the game's next recompute.
  static void (CInventory::*CalculateEffectValues)();
};

static_assert(offsetof(CInventory, effectValues) == 0x74);

#pragma pack(push, 1)

struct CCharacter {
  char _pad00[0x2A8];  // 0x000
  CItem* weapon;       // 0x2A8: the equipped melee weapon; null if unarmed.
  char _pad01[0x320 - (0x2A8 + sizeof(CItem*))];  // 0x2AC
  int playerIndex;                                // 0x320: -1 for anything
                                                  // not player-controlled.
  char _pad02[0x458 - (0x320 + sizeof(int))];     // 0x324
  CInventory* inventory;                          // 0x458

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
  CKeyHandler keyboard;                                               // 0x000
  char _pad00[0x504 - sizeof(CKeyHandler)];                           // 0x004
  CMouseHandler mouse;                                                // 0x504
  char _pad01a[0x518 - (0x504 + sizeof(CMouseHandler))];              // 0x508
  int mouseX;                                                         // 0x518
  int mouseY;                                                         // 0x51C
  char _pad01b[0x524 - (0x51C + sizeof(int))];                        // 0x520
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

// Finds the first effect in [begin, end) whose `type` matches, or nullptr if
// none match. Skips null entries rather than dereferencing them: this walks a
// raw pointer range straight from game memory, where a null slot is a real
// possibility. Kept pure so it can be unit tested without the game running.
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
