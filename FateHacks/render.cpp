#include "render.hpp"

namespace fate {

void (CText::* CText::Render)(IDirect3DDevice8* id3dDevice) =
    AddrToFuncPtr<decltype(Render)>(0x449CF3);
void (CText::* CText::Update)(IDirect3DDevice8* id3dDevice, STLString* str,
                              float x, float y, float alpha, char unk, int unk2,
                              int unk3, int unk4) =
    AddrToFuncPtr<decltype(Update)>(0x449060);
void (CText::* CText::Constructor)(
    IDirect3DDevice8* id3dDevice, struct CMaterial* cMaterial,
    struct CFontMetric* font, STLString* str, float x, float y, float alpha,
    char unk, int unk2, int unk3,
    int unk4) = AddrToFuncPtr<decltype(Constructor)>(0x448464);
void (CText::* CText::DeleteCText)(int flags) =
    AddrToFuncPtr<decltype(DeleteCText)>(0x438E5C);

CConfirmMenu* (CConfirmMenu::* CConfirmMenu::Constructor)(
    IDirect3DDevice8* id3dDevice, CRefManager* refManager,
    struct CSettings* settings, struct CGameStateManager* gameStateManager) =
    AddrToFuncPtr<decltype(Constructor)>(0x477AEC);
void (CConfirmMenu::* CConfirmMenu::Destructor)() =
    AddrToFuncPtr<decltype(Destructor)>(0x477D68);
void (CConfirmMenu::* CConfirmMenu::Render)(IDirect3DDevice8* id3dDevice) =
    AddrToFuncPtr<decltype(Render)>(0x478BB2);

}  // namespace fate
