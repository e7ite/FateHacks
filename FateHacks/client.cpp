#include "client.hpp"

#include "detour.hpp"
#include "menu.hpp"

namespace fate {
namespace {

// Bridges the game-free CharacterActions interface the menu is built against
// to the real player character. The character pointer is refreshed each frame
// before the menu acts, since it only exists while a game is loaded.
class GameCharacterActions : public CharacterActions {
 public:
  void set_character(CCharacter* character) { character_ = character; }

  void GiveGold(int amount) override {
    if (character_ != nullptr) {
      (character_->*CCharacter::GiveGold)(amount);
    }
  }

  void SetDamageMultiplier(int multiplier) override {
    damage_multiplier_ = multiplier;
  }
  int damage_multiplier() const { return damage_multiplier_; }

 private:
  CCharacter* character_ = nullptr;
  int damage_multiplier_ = 1;
};

// Bridges the game-free TextMeasurer interface the menu is built against to
// the game's loaded font. The font pointer is refreshed each frame before
// checking clicks, since it only exists while a game is loaded.
class GameTextMeasurer : public TextMeasurer {
 public:
  void set_font(CFontMetric* font) { font_ = font; }

  int MeasureWidth(const char* text, float scale) const override {
    return font_ != nullptr ? static_cast<int>(font_->TextWidth(text, scale))
                            : 0;
  }

 private:
  CFontMetric* font_ = nullptr;
};

GameCharacterActions& CheatMenuActions() {
  static GameCharacterActions actions;
  return actions;
}

GameTextMeasurer& CheatMenuMeasurer() {
  static GameTextMeasurer measurer;
  return measurer;
}

Menu& CheatMenu() {
  static Menu menu = BuildCheatMenu(&CheatMenuActions(), &CheatMenuMeasurer());
  return menu;
}

// Virtual-key (VK_INSERT) that opens and closes the cheat menu.
constexpr unsigned int kToggleKey = 0x2D;

// Virtual-key (VK_BACK) that steps out of a submenu, or closes the menu if
// already at the root.
constexpr unsigned int kBackKey = 0x08;

}  // namespace

float ApplyDamageMultiplier(float damage, bool attacker_is_player,
                            int multiplier) {
  return attacker_is_player ? damage * multiplier : damage;
}

void (CCharacter::* CCharacter::GiveGold)(int amount) =
    AddrToFuncPtr<decltype(GiveGold)>(0x59DEB3);

void (CCharacter::* CCharacter::TakeDamage)(CLevel* level, CCharacter* attacker,
                                            float damage, char showEffects) =
    AddrToFuncPtr<decltype(TakeDamage)>(0x5AEB67);

void CCharacter::TakeDamageDetour(CLevel* level, CCharacter* attacker,
                                  float damage, char showEffects) {
  // playerIndex is -1 for anything the player doesn't control.
  bool attacker_is_player = attacker != nullptr && attacker->playerIndex != -1;
  damage = ApplyDamageMultiplier(damage, attacker_is_player,
                                 CheatMenuActions().damage_multiplier());
  (this->*TakeDamage)(level, attacker, damage, showEffects);
}

bool (CGameUI::* CGameUI::Paused)() = AddrToFuncPtr<decltype(Paused)>(0x43759B);
bool (CGameUI::* CGameUI::Render)(IDirect3DDevice8* id3dDevice,
                                  CGameClient* client, void* unk, void* unk2) =
    AddrToFuncPtr<decltype(Render)>(0x4A2E7D);

bool CGameUI::RenderDetour(IDirect3DDevice8* id3dDevice, CGameClient* client,
                           void* unk, void* unk2) {
  // Let the game draw its UI first, then draw the cheat menu on top.
  bool status = (this->*Render)(id3dDevice, client, unk, unk2);

  if (fontMaterial != nullptr && font != nullptr) {
    CheatMenu().Render(id3dDevice, fontMaterial, font, mouseX, mouseY);
  }

  return status;
}

void (CGameClient::* CGameClient::Update)(IDirect3DDevice8* id3dDevice,
                                          HWND handle, float unk) =
    AddrToFuncPtr<decltype(Update)>(0x482BD5);

void CGameClient::UpdateDetour(IDirect3DDevice8* id3dDevice, HWND handle,
                               float unk) {
  CGameUI* ui = this->ui;
  CheatMenuActions().set_character(ui->character);
  CheatMenuMeasurer().set_font(ui->font);

  // Drive the menu from the Update phase, where the "just pressed" input edges
  // are still fresh. (The menu is drawn in CGameUI::RenderDetour.) Insert opens
  // and closes it, Backspace steps back, and a left-click activates whatever
  // row the mouse is over.
  if ((ui->keyboard.*CKeyHandler::KeyPressed)(kToggleKey)) {
    CheatMenu().Toggle();
  }
  if ((ui->keyboard.*CKeyHandler::KeyPressed)(kBackKey)) {
    CheatMenu().Back();
  }
  if (CheatMenu().is_open() && (ui->mouse.*CMouseHandler::ButtonPressed)(
                                   CMouseHandler::EButton::LEFT_CLICK)) {
    CheatMenu().Activate(ui->mouseX, ui->mouseY);
  }

  (this->*Update)(id3dDevice, handle, unk);
}

bool InstallClientDetours() {
  if (!AttachDetour(reinterpret_cast<PVOID*>(&CGameClient::Update),
                    FuncPtrToPVoid(&CGameClient::UpdateDetour))) {
    return false;
  }
  if (!AttachDetour(reinterpret_cast<PVOID*>(&CGameUI::Render),
                    FuncPtrToPVoid(&CGameUI::RenderDetour))) {
    return false;
  }
  if (!AttachDetour(reinterpret_cast<PVOID*>(&CCharacter::TakeDamage),
                    FuncPtrToPVoid(&CCharacter::TakeDamageDetour))) {
    return false;
  }
  return true;
}

}  // namespace fate
