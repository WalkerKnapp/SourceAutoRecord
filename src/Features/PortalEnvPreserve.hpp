#pragma once

#include "Variable.hpp"
#include "Features/Feature.hpp"

class PortalEnvPreserve : public Feature {
public:
  PortalEnvPreserve(void);
  void Update(void);
private:
  void *envLastTick;
};

extern PortalEnvPreserve *portalEnvPreserve;

extern Variable sar_portal_env_preserve;
