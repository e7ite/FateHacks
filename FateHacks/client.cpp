#include "client.hpp"

namespace fate {

void (CCharacter::* CCharacter::GiveGold)(int amount) =
    AddrToFuncPtr<decltype(GiveGold)>(0x59DEB3);

bool (CGameUI::* CGameUI::Paused)() = AddrToFuncPtr<decltype(Paused)>(0x43759B);
bool (CGameUI::* CGameUI::Render)(IDirect3DDevice8* id3dDevice,
                                  CGameClient* client, void* unk, void* unk2) =
    AddrToFuncPtr<decltype(Render)>(0x4A2E7D);

bool CGameUI::RenderDetour(IDirect3DDevice8* id3dDevice, CGameClient* client,
                           void* unk, void* unk2) {
  // Let the game draw its UI first, then draw ours on top.
  bool status = (this->*Render)(id3dDevice, client, unk, unk2);

  static std::unique_ptr<CText, void (*)(CText*)> overlayText(nullptr, nullptr);
  if (overlayText == nullptr && fontMaterial != nullptr && font != nullptr) {
    STLString str("Hellfateo world!");
    overlayText = CText::Create(id3dDevice, fontMaterial, font, &str, 100, 10,
                                0.8f, 0, 2, 1024, 768);
  }
  if (overlayText) {
    (overlayText.get()->*CText::Render)(id3dDevice);
  }

  return status;
}

}  // namespace fate
