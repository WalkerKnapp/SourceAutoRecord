#include "MaterialSystem.hpp"

#include "Cheats.hpp"
#include "Command.hpp"
#include "Interface.hpp"
#include "Module.hpp"
#include "Offsets.hpp"
#include "SAR.hpp"
#include "Utils.hpp"

REDECL(MaterialSystem::UncacheUnusedMaterials);

DETOUR(MaterialSystem::UncacheUnusedMaterials, bool bRecomputeStateSnapshots) {
    auto start = std::chrono::high_resolution_clock::now();
    bool bRecomputeStateSnapshotFixed = sar_janky_loading_hack.GetBool() ? false : bRecomputeStateSnapshots;
    auto result = MaterialSystem::UncacheUnusedMaterials(thisptr, bRecomputeStateSnapshotFixed);
    auto stop = std::chrono::high_resolution_clock::now();
    console->DevMsg("UncacheUnusedMaterials - %dms\n", std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count());
    return result;
}

bool MaterialSystem::Init()
{
    this->materials = Interface::Create(this->Name(), "VMaterialSystem0");
    if (this->materials) {
        if (sar.game->Is(SourceGame_Portal2Engine)) {
            this->materials->Hook(MaterialSystem::UncacheUnusedMaterials_Hook, MaterialSystem::UncacheUnusedMaterials, 77);
        }
    }

    return this->hasLoaded = this->materials;
}
void MaterialSystem::Shutdown()
{
    Interface::Delete(this->materials);
}

MaterialSystem* materialSystem;
