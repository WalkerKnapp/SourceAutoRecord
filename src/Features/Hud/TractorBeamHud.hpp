#pragma once
#include "Hud.hpp"

#include "Variable.hpp"
#include "Command.hpp"

class TractorBeamHud : public Hud {
public:
    TractorBeamHud();
    bool ShouldDraw() override;
    void Paint(int slot) override;
    bool GetCurrentSize(int& xSize, int& ySize) override;
};

extern TractorBeamHud tractorBeamHud;

extern Variable sar_vphys_hud;
extern Variable sar_vphys_hud_x;
extern Variable sar_vphys_hud_y;

extern Command sar_vphys_setgravity;
extern Command sar_vphys_setangle;
extern Command sar_vphys_setspin;