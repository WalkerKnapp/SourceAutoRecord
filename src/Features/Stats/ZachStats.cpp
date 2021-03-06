#include "ZachStats.hpp"

#include "Modules/Client.hpp"
#include "Modules/Engine.hpp"
#include "Modules/Server.hpp"

#include "Features/Speedrun/SpeedrunTimer.hpp"

#include <cmath>
#include <cstdlib>
#include <iomanip>

#ifdef _WIN32
#define PLAT_CALL(fn, ...) fn(__VA_ARGS__)
#else
#define PLAT_CALL(fn, ...) fn(nullptr, __VA_ARGS__)
#endif

Variable sar_mtrigger_name("sar_mtrigger_name", "FrenchSaves10ticks", "Name of the current player. Re-enables all triggers when changed.\n", 0);
Variable sar_mtrigger_draw("sar_mtrigger_draw", "0", 0, 2, "How to draw the triggers in-game. 0: do not show. 1: show outline. 2: show full box (appears through walls).\n");
Variable sar_mtrigger_show_chat("sar_mtrigger_show_chat", "1", "Show trigger times in chat.\n");
Variable sar_mtrigger_header("sar_mtrigger_header", "Name, Times", "Header of the csv file.\n", 0);

//Stats

ZachStats* zachStats;

ZachStats::ZachStats()
    : isFirstPlaced(false)
    , lastFrameDrawn(0)
    , lastTriggerSplit(-1)
    , output("", std::ios_base::out)
{
    this->hasLoaded = true;
}

void ZachStats::UpdateTriggers()
{
    auto player = client->GetPlayer(GET_SLOT() + 1);
    if (!player)
        return;

    auto pos = client->GetAbsOrigin(player);

    for (auto trigger : this->GetTriggers()) {
        if (trigger->type == TriggerType::ZYPEH) {
            ZypehTrigger* zypehTrigger = static_cast<ZypehTrigger*>(trigger);

            if (sar_mtrigger_draw.GetBool() && sv_cheats.GetBool() && this->lastFrameDrawn + 59 <= session->GetTick()) {
                this->DrawTrigger(zypehTrigger);
            }

            if (this->CheckZypehTriggers(zypehTrigger, pos)) {
                ZachStats::Output(this->GetStream(), client->GetCMTimer());
                zypehTrigger->triggered = true;
            }
        }
    }

    if (sar_mtrigger_draw.GetBool() && sv_cheats.GetBool() && this->lastFrameDrawn + 59 <= session->GetTick()) {
        this->lastFrameDrawn = session->GetTick();
    }

    if (this->isFirstPlaced) {
        this->PreviewSecond();
    }
}

void ZachStats::AddZypehTrigger(Vector& A, Vector& G, float angle, unsigned int ID)
{
    // 'A' must have minimum coordinates, 'G' must have max
#define SWAP(a,b) do { float tmp = a; a = b; b = tmp; } while (0)
    if (A.x > G.x) SWAP(A.x, G.x);
    if (A.y > G.y) SWAP(A.y, G.y);
    if (A.z > G.z) SWAP(A.z, G.z);
#undef SWAP

    this->DeleteTrigger(ID); // Make sure there's not already a trigger with that ID

    ZypehTrigger* trigger = new ZypehTrigger(A, G, ID, angle);

    this->GetTriggers().push_back(trigger);
    console->Print("Trigger added !\n");
}

void ZachStats::AddZyntexTrigger(const std::string entName, const std::string input, unsigned int ID)
{
    this->DeleteTrigger(ID); // Make sure there's not already a trigger with that ID

    ZyntexTrigger* trigger = new ZyntexTrigger(entName, input, ID);

    this->GetTriggers().push_back(trigger);
    console->Print("Trigger added !\n");
}

void ZachStats::DeleteTrigger(unsigned int ID)
{
    auto& v = this->GetTriggers();
    unsigned int idx = 0;
    for (auto it : v) {
        if (it->ID == ID) {
            delete it;
            v.erase(v.begin() + idx);
            console->Print("Trigger deleted\n");
            break;
        }
        ++idx;
    }
}

void ZachStats::DeleteAll()
{
    auto& v = this->GetTriggers();
    for (auto it : v)
        delete it;
    v.clear();
    console->Print("Triggers deleted\n");
}

void ZachStats::DrawTrigger(ZypehTrigger* trigger)
{
    PLAT_CALL(
        engine->AddBoxOverlay,
        trigger->origin,
        trigger->origVerts[0] - trigger->origin,
        trigger->origVerts[1] - trigger->origin,
        { 0, trigger->angle, 0 },
        255, 0, 0, sar_mtrigger_draw.GetInt() == 2 ? 100 : 0,
        1);
}

void ZachStats::PreviewSecond()
{
    //Trace ray to place the 2nd point
    CGameTrace tr;
    engine->TraceFromCamera(65535.0, tr);

    auto const& G = tr.endpos;

    //Draw the box

    Vector origin{ (G.x + A.x) / 2, (G.y + A.y) / 2, (G.z + A.z) / 2 };
    PLAT_CALL(
        engine->AddBoxOverlay,
        origin,
        A - origin,
        G - origin,
        { 0, 0, 0 },
        255, 0, 0, 0,
        0);
}

std::vector<Trigger*>& ZachStats::GetTriggers()
{
    return g_triggers;
}

Trigger* ZachStats::GetTriggerByID(unsigned int ID)
{
    for (auto trigger : this->GetTriggers()) {
        if (trigger->ID == ID) {
            return trigger;
        }
    }
    return nullptr;
}

void ZachStats::ResetTriggers()
{
    for (auto trigger : this->GetTriggers()) {
        trigger->triggered = false;
    }
    this->lastTriggerSplit = -1;
    this->lastFrameDrawn = 0;
}

bool ZachStats::ExportTriggers(std::string filePath)
{
    if (filePath.length() < 4 || filePath.substr(filePath.length() - 4, 4) != ".cfg") {
        filePath += ".cfg";
    }

    std::ofstream file(filePath, std::ios::out | std::ios::trunc);
    if (!file.good()) {
        file.close();
        return false;
    }

    file << "// Explanation: sar_mtrigger_add ID A.x A.y A.z B.x B.y B.z angle OR sar_zach_trigger_add entity_name input"
         << "sar_mtrigger_header " << sar_mtrigger_header.GetString()
         << std::endl;

    for (auto trigger : this->GetTriggers()) {
        if (trigger->type == TriggerType::ZYPEH) {
            ZypehTrigger* zypehTrigger = static_cast<ZypehTrigger*>(trigger);
            file << "sar_mtrigger_add " << zypehTrigger->ID
                 << " " << zypehTrigger->origVerts[0].x << " " << zypehTrigger->origVerts[0].y << " " << zypehTrigger->origVerts[0].z
                 << " " << zypehTrigger->origVerts[1].x << " " << zypehTrigger->origVerts[1].y << " " << zypehTrigger->origVerts[1].z
                 << " " << zypehTrigger->angle
                 << ";"
                 << std::endl;
        } else if (trigger->type == TriggerType::ZYNTEX) {
            ZyntexTrigger* zyntexTrigger = static_cast<ZyntexTrigger*>(trigger);
            file << "sar_mtrigger_add " << zyntexTrigger->ID << " " << zyntexTrigger->entName << " " << zyntexTrigger->input << ";"
                 << std::endl;
        }
    }

    file.close();
    return true;
}

void ZachStats::Output(std::stringstream& output, const float time)
{
    char out[128];
    std::snprintf(out, 128, "%s -> %.3f (%.3f)", sar_mtrigger_name.GetString(), time, zachStats->lastTriggerSplit < 0 ? time : (time - zachStats->lastTriggerSplit));

    if (sar_mtrigger_show_chat.GetBool()) {
        client->Chat(TextColor::ORANGE, out);
    } else {
        std::strncat(out, "\n", 2);
        console->Print(out);
    }

    output.clear(); // Clear flags
    //Stop annoy me
    output << CSV_SEPARATOR << std::fixed << std::setprecision(3) << time << " (" << (zachStats->lastTriggerSplit < 0 ? time : (time - zachStats->lastTriggerSplit)) << ")";

    zachStats->lastTriggerSplit = time;

    if (sar_speedrun_IL.GetBool() && sv_bonus_challenge.GetBool()) {
        speedrun->Split();
    }
}

bool ZachStats::ExportCSV(std::string filePath)
{
    if (filePath.length() < 4 || filePath.substr(filePath.length() - 4, 4) != ".csv") {
        filePath += ".csv";
    }

    std::ofstream file(filePath, std::ios::out | std::ios::trunc);
    if (!file.good()) {
        file.close();
        return false;
    }

    this->output.clear(); // Clear flags
#ifdef _WIN32
    // Fine Microsoft we'll do your dumb thing
    file << MICROSOFT_PLEASE_FIX_YOUR_SOFTWARE_SMHMYHEAD << std::endl;
#endif
    file << sar_mtrigger_header.GetString() << std::endl;
    file << this->output.str() << std::endl;

    file.close();
    return true;
}

void ZachStats::NewSession()
{
    auto& stream = zachStats->GetStream();
    stream.clear(); // Clear flags
    if (stream.tellp() != std::streampos(0))
        stream << "\n";
    stream << sar_mtrigger_name.GetString();
}

bool ZachStats::CheckZypehTriggers(ZypehTrigger* trigger, Vector& pos)
{
    if (trigger->triggered) {
        return false;
    }

    // Step 1: collide on z (vertical). As the box is effectively axis-aligned on
    // this axis, we can just compare the point's z coordinate to the top
    // and bottom of the cuboid

    float zMin = trigger->verts[0].z, zMax = trigger->verts[4].z; // A point on the bottom and top respectively
    if (pos.z < zMin || pos.z > zMax) {
        /*if (trigger->ID == 1) {
            console->Print("triggered %d\n", trigger->ID);
        }*/
        return false;
    }

    // Step 2: collide on x and y. We can now ignore the y axis entirely,
    // and instead look at colliding the point {p.x, p.y} with the
    // rectangle defined by the top or bottom face of the cuboid

    // Algorithm stolen from https://stackoverflow.com/a/2752754/13932065

    float bax = trigger->verts[1].x - trigger->verts[0].x;
    float bay = trigger->verts[1].y - trigger->verts[0].y;
    float dax = trigger->verts[3].x - trigger->verts[0].x;
    float day = trigger->verts[3].y - trigger->verts[0].y;

    if ((pos.x - trigger->verts[0].x) * bax + (pos.y - trigger->verts[0].y) * bay < 0) {
        if (trigger->ID == 1) {
        }
        return false;
    }
    if ((pos.x - trigger->verts[1].x) * bax + (pos.y - trigger->verts[1].y) * bay > 0) {
        if (trigger->ID == 1) {
        }
        return false;
    }
    if ((pos.x - trigger->verts[0].x) * dax + (pos.y - trigger->verts[0].y) * day < 0) {
        if (trigger->ID == 1) {
        }
        return false;
    }
    if ((pos.x - trigger->verts[3].x) * dax + (pos.y - trigger->verts[3].y) * day > 0) {
        if (trigger->ID == 1) {
        }
        return false;
    }

    return true;
}

void ZachStats::CheckZyntexTriggers(void* entity, const char* input)
{
    for (auto trigger : g_triggers) {
        if (!trigger->triggered) {
            if (trigger->type == TriggerType::ZYNTEX) {
                ZyntexTrigger* zyntexTrigger = static_cast<ZyntexTrigger*>(trigger);
                char* entName = server->GetEntityName(entity);
                if (!entName) entName = ""; // Apparently comparing a string to a null pointer is UB for some reason
                if (zyntexTrigger->input == input && zyntexTrigger->entName == entName) {
                    ZachStats::Output(this->GetStream(), client->GetCMTimer());
                    zyntexTrigger->triggered = true;
                }
            }
        }
    }
}

CON_COMMAND(sar_mtrigger_add, "Usage 1 -> sar_mtrigger_add <id> <A.x> <A.y> <A.z> <B.x> <B.y> <B.z> [angle] : add a trigger with the specified ID, position, and optional angle.\n"
                              "Usage 2 -> sar_mtrigger_add <id> <entity name> <input> : add a trigger with the specified ID that will trigger at a specific entity input.\n")
{
    if (args.ArgC() != 4 && args.ArgC() != 8 && args.ArgC() != 9) {
        return console->Print(sar_mtrigger_add.ThisPtr()->m_pszHelpString);
    }

    char* end;
    int id = std::strtol(args[1], &end, 10);
    if (*end != 0 || end == args[1]) {
        // ID argument is not a number
        return console->Print(sar_mtrigger_add.ThisPtr()->m_pszHelpString);
    }

    if (args.ArgC() >= 8) {
        Vector A = Vector(std::atof(args[2]), std::atof(args[3]), std::atof(args[4]));
        Vector G = Vector(std::atof(args[5]), std::atof(args[6]), std::atof(args[7]));

        float angle = 0;
        if (args.ArgC() == 9) {
            angle = std::atof(args[8]);
        }

        zachStats->AddZypehTrigger(A, G, angle, id);
    } else {
        std::string entName = args[2];
        std::string input = args[3];

        zachStats->AddZyntexTrigger(entName, input, id);
    }
}

CON_COMMAND(sar_mtrigger_place, "sar_mtrigger_place <id> : place a trigger with the given ID at the position being aimed at.\n")
{
    if (args.ArgC() != 2) {
        return console->Print(sar_mtrigger_place.ThisPtr()->m_pszHelpString);
    }

    if (!sv_cheats.GetBool()) {
        // Trigger placement adds an overlay (even if temporarily),
        // hence is a cheat
        console->Print("sar_mtrigger_place requires sv_cheats.\n");
        return;
    }

    char* end;
    int id = std::strtol(args[1], &end, 10);
    if (*end != 0 || end == args[1]) {
        // ID argument is not a number
        return console->Print(sar_mtrigger_place.ThisPtr()->m_pszHelpString);
    }

    CGameTrace tr;
    bool hit = engine->TraceFromCamera(65535.0f, tr);
    if (!hit) {
        return console->Print("You aimed at the void.\n");
    }

    if (!sar_mtrigger_draw.GetBool()) {
        console->Print("sar_mtrigger_draw set to 1 !\n");
        sar_mtrigger_draw.SetValue(1);
    }

    if (!zachStats->isFirstPlaced) {
        zachStats->A = tr.endpos;
        zachStats->isFirstPlaced = true;
    } else {
        zachStats->AddZypehTrigger(zachStats->A, tr.endpos, 0.0f, id);
        zachStats->isFirstPlaced = false;
    }
}

CON_COMMAND(sar_mtrigger_rotate, "sar_mtrigger_rotate <id> <angle> : changes the rotation of a trigger to the given angle, in degrees.\n")
{
    if (args.ArgC() != 3) {
        return console->Print(sar_mtrigger_rotate.ThisPtr()->m_pszHelpString);
    }

    char* end;

    int id = std::strtol(args[1], &end, 10);
    if (*end != 0 || end == args[1]) {
        // ID argument is not a number
        return console->Print(sar_mtrigger_rotate.ThisPtr()->m_pszHelpString);
    }

    int angle = std::strtol(args[2], &end, 10);
    if (*end != 0 || end == args[2]) {
        // ID argument is not a number
        return console->Print(sar_mtrigger_rotate.ThisPtr()->m_pszHelpString);
    }

    auto trigger = zachStats->GetTriggerByID(id);
    if (trigger == nullptr) {
        return console->Print("No such trigger !\n");
    } else if (trigger->type != TriggerType::ZYPEH) {
        return console->Print("Not a zone trigger !\n");
    }

    ZypehTrigger* zypehTrigger = static_cast<ZypehTrigger*>(trigger);
    zypehTrigger->SetRotation(angle);
}

CON_COMMAND(sar_mtrigger_delete, "sar_mtrigger_delete <id> : deletes the trigger with the given ID.\n")
{
    if (args.ArgC() != 2) {
        return console->Print(sar_mtrigger_delete.ThisPtr()->m_pszHelpString);
    }

    char* end;
    int id = std::strtol(args[1], &end, 10);
    if (*end != 0 || end == args[1]) {
        // ID argument is not a number
        return console->Print(sar_mtrigger_delete.ThisPtr()->m_pszHelpString);
    }

    zachStats->DeleteTrigger(id);
}

CON_COMMAND(sar_mtrigger_delete_all, "sar_mtrigger_delete_all : deletes every triggers.\n")
{
    if (args.ArgC() != 1) {
        return console->Print(sar_mtrigger_delete_all.ThisPtr()->m_pszHelpString);
    }

    zachStats->DeleteAll();
}

CON_COMMAND(sar_mtrigger_export_stats, "sar_mtrigger_export_stats <filename> : Export the current stats to the specified .csv file.\n")
{
    if (args.ArgC() != 2) {
        return console->Print(sar_mtrigger_export_stats.ThisPtr()->m_pszHelpString);
    }

    if (zachStats->ExportCSV(args[1])) {
        console->Print("Successfully exported to %s.\n", args[1]);
    } else {
        console->Print("Could not export stats.\n");
    }
}

CON_COMMAND(sar_mtrigger_export_triggers, "sar_mtrigger_export_triggers <filename> : Export the current triggers to the specified .csv file.\n")
{
    if (args.ArgC() != 2) {
        return console->Print(sar_mtrigger_export_triggers.ThisPtr()->m_pszHelpString);
    }

    if (zachStats->ExportTriggers(args[1])) {
        console->Print("Successfully exported to %s.\n", args[1]);
    } else {
        console->Print("Could not export triggers.\n");
    }
}

CON_COMMAND(sar_mtrigger_reset, "Resets the state of the output and all triggers, ready for gathering stats.\n")
{
    zachStats->ResetTriggers();
    zachStats->ResetStream();
}
