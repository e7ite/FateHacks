#ifndef FATEHACKS_RENDER_HPP_
#define FATEHACKS_RENDER_HPP_

// Rendering resources bound to the game's Direct3D device.

#include <memory>

#include "abi.hpp"
#include "stl.hpp"

namespace fate {

struct IDirect3DDevice8;

struct CFontMetric {
  int charWidths[256];
  float spacing;
  int widthBias;

  // The on-screen width of `text` at `scale`, matching CText::Update's layout
  // math -- so checking a click against a rendered label can use the same
  // numbers the game used to draw it.
  float TextWidth(const char* text, float scale) const {
    float total = 0.0f;
    for (const unsigned char* p = reinterpret_cast<const unsigned char*>(text);
         *p != '\0'; ++p) {
      unsigned int raw_width = static_cast<unsigned int>(widthBias) +
                               static_cast<unsigned int>(charWidths[*p]);
      total += (static_cast<float>(raw_width) + spacing) * scale;
    }
    return total;
  }
};

static_assert(offsetof(CFontMetric, spacing) == 0x400);
static_assert(offsetof(CFontMetric, widthBias) == 0x404);

struct CText {
  char _pad00[0x78];

  static void (CText::*Render)(IDirect3DDevice8* id3dDevice);
  // Rebuilds the mesh from a new string. Use this to edit the text after
  // Create.
  static void (CText::*Update)(IDirect3DDevice8* id3dDevice, STLString* str,
                               float x, float y, float alpha, char unk,
                               int unk2, int unk3, int unk4);

  static std::unique_ptr<CText, void (*)(CText*)> Create(
      IDirect3DDevice8* id3dDevice, struct CMaterial* cMaterial,
      struct CFontMetric* font, STLString* str, float x, float y, float alpha,
      char unk, int unk2, int unk3, int unk4) {
    auto text = std::unique_ptr<CText, void (*)(CText*)>(
        reinterpret_cast<CText*>(GameOperatorNew(sizeof(CText))),
        [](CText* text) { (text->*DeleteCText)(/*flags=*/1); });
    (text.get()->*Constructor)(id3dDevice, cMaterial, font, str, x, y, alpha,
                               unk, unk2, unk3, unk4);
    return text;
  }

 private:
  CText() = default;

  static void (CText::*Constructor)(IDirect3DDevice8* id3dDevice,
                                    struct CMaterial* cMaterial,
                                    struct CFontMetric* font, STLString* str,
                                    float x, float y, float alpha, char unk,
                                    int unk2, int unk3, int unk4);
  static void (CText::*DeleteCText)(int flags);
};

struct MaterialRef {
  char __pad00[0x24];
  STLString part1;
  STLString part2;
  STLString part3;
  STLString part4;
  STLString part5;
};

static_assert(offsetof(MaterialRef, part1) == 0x24);
static_assert(offsetof(MaterialRef, part2) == 0x40);
static_assert(offsetof(MaterialRef, part3) == 0x5C);
static_assert(offsetof(MaterialRef, part4) == 0x78);
static_assert(offsetof(MaterialRef, part5) == 0x94);

struct CRefManager {
  char __pad00[0x50];
  STLVector<MaterialRef*> materialRefs;
};

static_assert(offsetof(CRefManager, materialRefs) == 0x50);

struct CConfirmMenu {
  char _pad00[0x594];

  static void (CConfirmMenu::*Render)(IDirect3DDevice8* id3dDevice);

  // Using this factory method because we are invoking a constructor of the
  // game code, and we want to make sure we correctly allocate/free the
  // memory required for this structure using the game's operator new and
  // delete.
  static std::unique_ptr<CConfirmMenu, void (*)(CConfirmMenu*)> Create(
      IDirect3DDevice8* id3dDevice, CRefManager* refManager,
      struct CSettings* settings, struct CGameStateManager* gameStateManager) {
    auto menu = std::unique_ptr<CConfirmMenu, void (*)(CConfirmMenu*)>(
        reinterpret_cast<CConfirmMenu*>(GameOperatorNew(sizeof(CConfirmMenu))),
        [](CConfirmMenu* menu) { (menu->*Destructor)(); });
    (menu.get()->*Constructor)(id3dDevice, refManager, settings,
                               gameStateManager);
    return menu;
  }

 private:
  CConfirmMenu() = default;

  static CConfirmMenu* (CConfirmMenu::*Constructor)(
      IDirect3DDevice8* id3dDevice, CRefManager* refManager,
      struct CSettings* settings, struct CGameStateManager* gameStateManager);
  static void (CConfirmMenu::*Destructor)();
};

}  // namespace fate

#endif  // FATEHACKS_RENDER_HPP_
