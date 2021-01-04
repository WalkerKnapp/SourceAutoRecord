#pragma once
#include "Module.hpp"

#include "Command.hpp"
#include "Interface.hpp"
#include "Offsets.hpp"
#include "Utils.hpp"

class MaterialSystem : public Module {
public:
    Interface* materials = nullptr;


public:
    DECL_DETOUR(UncacheUnusedMaterials, bool bRecomputeStateSnapshots);

    bool Init() override;
    void Shutdown() override;
    const char* Name() override { return MODULE("materialsystem"); }
};

extern MaterialSystem* materialSystem;
