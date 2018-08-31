#include "Client.hpp"

#include "Console.hpp"
#include "Engine.hpp"
#include "VGui.hpp"

#include "Features/Tas/CommandQueuer.hpp"
#include "Features/Tas/ReplaySystem.hpp"

#include "Cheats.hpp"
#include "Game.hpp"
#include "Interface.hpp"
#include "Offsets.hpp"
#include "Utils.hpp"

REDECL(Client::HudUpdate)
REDECL(Client::CreateMove)
REDECL(Client::GetName)

void* Client::GetPlayer()
{
    return this->GetClientEntity(this->s_EntityList->ThisPtr(), engine->GetLocalPlayerIndex());
}
Vector Client::GetAbsOrigin()
{
    auto player = this->GetPlayer();
    return (player) ? *(Vector*)((uintptr_t)player + Offsets::m_vecAbsOrigin) : Vector();
}
QAngle Client::GetAbsAngles()
{
    auto player = this->GetPlayer();
    return (player) ? *(QAngle*)((uintptr_t)player + Offsets::m_angAbsRotation) : QAngle();
}
Vector Client::GetLocalVelocity()
{
    auto player = this->GetPlayer();
    return (player) ? *(Vector*)((uintptr_t)player + Offsets::m_vecVelocity) : Vector();
}
int Client::GetFlags()
{
    auto player = this->GetPlayer();
    return (player) ? *(int*)((uintptr_t)player + Offsets::GetFlags) : 0;
}

// CHLClient::HudUpdate
DETOUR(Client::HudUpdate, unsigned int a2)
{
    if (tasQueuer->isRunning) {
        for (auto&& tas = tasQueuer->frames.begin(); tas != tasQueuer->frames.end();) {
            tas->FramesLeft--;
            if (tas->FramesLeft <= 0) {
                console->DevMsg("[%i] %s\n", engine->currentFrame, tas->Command.c_str());
                engine->ExecuteCommand(tas->Command.c_str());
                tas = tasQueuer->frames.erase(tas);
            } else {
                ++tas;
            }
        }
    }

    ++engine->currentFrame;
    return Client::HudUpdate(thisptr, a2);
}

// ClientModeShared::CreateMove
DETOUR(Client::CreateMove, float flInputSampleTime, CUserCmd* cmd)
{
    vgui->inputHud->SetButtonBits(cmd->buttons);

    if (cmd->command_number) {
        if (tasReplaySystem->isPlaying) {
            tasReplaySystem->Play(cmd);
        } else if (tasReplaySystem->isRecording) {
            tasReplaySystem->Record(cmd);
        }
    }

    return Client::CreateMove(thisptr, flInputSampleTime, cmd);
}

// CHud::GetName
DETOUR_T(const char*, Client::GetName)
{
    // Never allow CHud::FindElement to find this HUD
    if (sar_disable_challenge_stats_hud.GetBool())
        return "";

    return Client::GetName(thisptr);
}

bool Client::Init()
{
    bool readJmp = false;
#ifdef _WIN32
    readJmp = sar.game->version == SourceGame::TheStanleyParable
        || sar.game->version == SourceGame::TheBeginnersGuide;
#endif

    this->g_ClientDLL = Interface::Create(MODULE("client"), "VClient0");
    this->s_EntityList = Interface::Create(MODULE("client"), "VClientEntityList0", false);

    if (this->g_ClientDLL) {
        this->g_ClientDLL->Hook(this->HudUpdate_Hook, this->HudUpdate, Offsets::HudUpdate);

        if (sar.game->version == SourceGame::Portal2) {
            auto leaderboard = Command("+leaderboard");
            if (!!leaderboard) {
                using _GetHud = void*(__cdecl*)(int unk);
                using _FindElement = void*(__func*)(void* thisptr, const char* pName);

                auto cc_leaderboard_enable = (uintptr_t)leaderboard.ThisPtr()->m_pCommandCallback;
                auto GetHud = Memory::Read<_GetHud>(cc_leaderboard_enable + Offsets::GetHud);
                auto FindElement = Memory::Read<_FindElement>(cc_leaderboard_enable + Offsets::FindElement);
                auto CHUDChallengeStats = FindElement(GetHud(-1), "CHUDChallengeStats");

                if (this->g_HUDChallengeStats = Interface::Create(CHUDChallengeStats)) {
                    this->g_HUDChallengeStats->Hook(this->GetName_Hook, this->GetName, Offsets::GetName);
                }
            }
        } else if (sar.game->version == SourceGame::TheStanleyParable) {
            auto IN_ActivateMouse = this->g_ClientDLL->Original(Offsets::IN_ActivateMouse, readJmp);
            auto g_InputAddr = Memory::DerefDeref<void*>(IN_ActivateMouse + Offsets::g_Input);

            if (auto g_Input = Interface::Create(g_InputAddr, false)) {
                auto GetButtonBits = g_Input->Original(Offsets::GetButtonBits, readJmp);
                Memory::Deref(GetButtonBits + Offsets::in_jump, &this->in_jump);

                auto JoyStickApplyMovement = g_Input->Original(Offsets::JoyStickApplyMovement, readJmp);
                Memory::Read(JoyStickApplyMovement + Offsets::KeyDown, &this->KeyDown);
                Memory::Read(JoyStickApplyMovement + Offsets::KeyUp, &this->KeyUp);
            }
        }

        auto HudProcessInput = this->g_ClientDLL->Original(Offsets::HudProcessInput, readJmp);
        void* clientMode = nullptr;
        if (sar.game->IsPortal2Engine()) {
            typedef void* (*_GetClientMode)();
            auto GetClientMode = Memory::Read<_GetClientMode>(HudProcessInput + Offsets::GetClientMode);
            clientMode = GetClientMode();
        } else {
            clientMode = Memory::DerefDeref<void*>(HudProcessInput + Offsets::GetClientMode);
        }

        if (this->g_pClientMode = Interface::Create(clientMode)) {
            this->g_pClientMode->Hook(this->CreateMove_Hook, this->CreateMove, Offsets::CreateMove);
        }
    }

    if (this->s_EntityList) {
        this->GetClientEntity = this->s_EntityList->Original<_GetClientEntity>(Offsets::GetClientEntity, readJmp);
    }

    return this->hasLoaded = this->g_ClientDLL && this->s_EntityList;
}
void Client::Shutdown()
{
    Interface::Delete(this->g_ClientDLL);
    Interface::Delete(this->g_pClientMode);
    Interface::Delete(this->g_HUDChallengeStats);
    Interface::Delete(this->s_EntityList);
}

Client* client;