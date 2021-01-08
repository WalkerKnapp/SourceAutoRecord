#include "PortalEnvPreserve.hpp"
#include "Offsets.hpp"
#include "Modules/Console.hpp"
#include "Modules/Server.hpp"

PortalEnvPreserve *portalEnvPreserve;

Variable sar_portal_env_preserve("sar_portal_env_preserve", "0", 0, 1, "Prevents the portal environment from being lost when the portal bubble is exited");

PortalEnvPreserve::PortalEnvPreserve(void) {
	this->hasLoaded = true;
}

void PortalEnvPreserve::Update(void) {
	if (!sv_cheats.GetBool() || !sar_portal_env_preserve.GetBool()) {
		this->envLastTick = NULL;
		return;
	}

	void *player = server->GetPlayer(1);

	if (player == NULL) {
		this->envLastTick = NULL;
		return;
	}

	void **portalEnv = (void **)((uintptr_t)player + Offsets::m_hPortalEnvironment);

	if ((uintptr_t)*portalEnv == 0xFFFFFFFF) {
		if (this->envLastTick != NULL) {
			console->Print("Portal env changed from %p to NULL; reverting\n", this->envLastTick);
			*portalEnv = this->envLastTick;
		}
	} else {
		this->envLastTick = *portalEnv;
	}
}
