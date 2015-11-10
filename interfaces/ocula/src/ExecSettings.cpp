#include "ExecSettings.h"
#include <sstream>

ExecSettings::ExecSettings()
{
    numberOfProcessors = 1;
    isRemote = false;
    isDistributed = false;
    isEmulationMode = true;
    forceTerrainRebuild = false;
    skipOgreSettings = false;
    interactivePort = DEFAULT_OCULA_PORT;
}


ExecSettings::~ExecSettings()
{
}


std::string ExecSettings::interactivePortString() const {
    std::stringstream buf;
    buf << interactivePort;
    return buf.str();
}


