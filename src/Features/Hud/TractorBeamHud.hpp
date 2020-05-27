#pragma once
#include "Hud.hpp"

#include "Variable.hpp"

class TractorBeamHud : public Hud {
public:
    TractorBeamHud();
    bool ShouldDraw() override;
    void Paint(int slot) override;
    bool GetCurrentSize(int& xSize, int& ySize) override;
};

extern TractorBeamHud tractorBeamHud;

extern Variable sar_tbeam_hud;
extern Variable sar_tbeam_hud_x;
extern Variable sar_tbeam_hud_y;