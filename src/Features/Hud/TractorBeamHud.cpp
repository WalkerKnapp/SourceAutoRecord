#include "TractorBeamHud.hpp"

#include "Features/Speedrun/SpeedrunTimer.hpp"

#include "Modules/Scheme.hpp"
#include "Modules/Server.hpp"
#include "Modules/Surface.hpp"
#include "Features/EntityList.hpp"

#include "Variable.hpp"

TractorBeamHud speedrunHud;

Variable sar_tbeam_hud("sar_tbeam_hud", "0", 0, "Agony.\n");

TractorBeamHud::TractorBeamHud()
    : Hud(HudType_InGame, false, SourceGame_SupportsS3)
{
}
bool TractorBeamHud::ShouldDraw()
{
    return sar_tbeam_hud.GetBool() && Hud::ShouldDraw();
}
void TractorBeamHud::Paint(int slot)
{
    auto font = scheme->GetDefaultFont() + 1;

    void* player = server->GetPlayer(1);
    void** pplocaldata = reinterpret_cast<void**>((uintptr_t)player + 5060); //apparently it's a struct and not a pointer lmfao

    int m_nTractorBeamCount = *reinterpret_cast<int*>((uintptr_t)pplocaldata + 396);

    void* m_hTractorBeam = *reinterpret_cast<void**>((uintptr_t)pplocaldata + 392);

    surface->DrawTxt(font, 5, 10, this->GetColor("255 255 255 255"), "m_hTractorBeam: %#08X", m_hTractorBeam);
    surface->DrawTxt(font, 5, 30, this->GetColor("255 255 255 255"), "m_nTractorBeamCount: %X", m_nTractorBeamCount);


    void* m_pShadowStand = *reinterpret_cast<void**>((uintptr_t)player + 3160);
    void* m_pShadowCrouch = *reinterpret_cast<void**>((uintptr_t)player + 3164);

    using _IsAsleep = bool(__thiscall*)(void* thisptr);
    using _IsCollisionEnabled = bool(__thiscall*)(void* thisptr);
    using _GetPosition = void(__thiscall*)(void* thisptr, Vector* worldPosition, QAngle* angles);
    
    _IsAsleep IsAsleep = Memory::VMT<_IsAsleep>(m_pShadowStand, 0x2);
    _IsCollisionEnabled IsCollisionEnabled = Memory::VMT<_IsCollisionEnabled>(m_pShadowStand, 6);
    _GetPosition GetPosition = Memory::VMT<_GetPosition>(m_pShadowStand, 48);

    auto drawPhysicsInfo = [=](int y, void* shadow, const char* name) {
        
        Vector v;
        QAngle q;
        GetPosition(shadow, &v, &q);
        bool collisionEnabled = IsCollisionEnabled(shadow);
        bool asleep = IsAsleep(shadow);

        Color posColor = this->GetColor("255 255 255 255");
        Color enableColor = this->GetColor("128 255 128 255");
        Color disableColor = this->GetColor("255 128 128 255");

        if (!collisionEnabled) {
            posColor._color[3] = 128;
            enableColor._color[3] = 128;
            disableColor._color[3] = 128;
        }

        surface->DrawTxt(font, 5, y, posColor, "%s (%#08X): ", name, shadow);
        surface->DrawTxt(font, 15, y + 20, posColor, "pos: (%.03f, %.03f, %.03f)", v.x, v.y, v.z);
        surface->DrawTxt(font, 15, y + 40, posColor, "ang: (%.03f, %.03f, %.03f)", q.x, q.y, q.z);
        
        surface->DrawTxt(font, 15, y + 60, collisionEnabled ? enableColor : disableColor, "IsCollisionEnabled(): %s", collisionEnabled ? "true" : "false");
        
        surface->DrawTxt(font, 15, y + 80, asleep ? enableColor : disableColor, "IsAsleep(): %s", asleep ? "true" : "false");
    };

    drawPhysicsInfo(70, m_pShadowStand, "m_pShadowStand");
    drawPhysicsInfo(180, m_pShadowCrouch, "m_pShadowCrouch");
}
bool TractorBeamHud::GetCurrentSize(int& xSize, int& ySize)
{
    return false;
}
