#include "SeamshotFind.hpp"

#include "Features/Session.hpp"

#include "Modules/Console.hpp"
#include "Modules/Engine.hpp"
#include "Modules/Server.hpp"

#include "Command.hpp"
#include "Variable.hpp"


Variable sar_seamshot_finder("sar_seamshot_finder", "0", 0, 1, "Enables or disables seamshot finder overlay.\n");

SeamshotFind* seamshotFind;

SeamshotFind::SeamshotFind()
{
    this->hasLoaded = true;
}

// Commands
CON_COMMAND(sar_seamshot_test, "Tests something...\n")
{
    Vector v1(-100, 100, 0);
    Vector v2(100.1, 100.1, 0.1);
    engine->AddLineOverlay(v1 ,v2, 255, 255, 255, false, 5.0);
    console->Print("AAAAAAAAA\n");
}