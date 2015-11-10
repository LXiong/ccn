// Copyright (c) 2001-2013, SCALABLE Network Technologies, Inc.  All Rights Reserved.
//                          600 Corporate Pointe
//                          Suite 1200
//                          Culver City, CA 90230
//                          info@scalable-networks.com
//
// This source code is licensed, not sold, and is subject to a written
// license agreement.  Among other things, no portion of this source
// code may be copied, transmitted, disclosed, displayed, distributed,
// translated, used as the basis for a derivative work, or used, in
// whole or in part, for any program or purpose other than its intended
// use in compliance with the license agreement as part of the EXata
// software.  This source code and certain of the algorithms contained
// within it are confidential trade secrets of Scalable Network
// Technologies, Inc. and may not be used as the basis for any other
// software, hardware, product or service.


#include <iostream>
#include "sim_types.h"
#include "Config.h"
#include "configFiles.h"
#include "vrlink.h"

using namespace SNT_SYNCH;

int main(int argc, const char* argv[])
{
    if (!Config::instance().parseCommandLine(argc, argv))
    {
        Config::instance().Usage(argv[0]);
        return 1;
    }

    Config::instance().readParameterFiles();
    Config::instance().readModelFiles();

    VRLinkInterfaceType type = GetVRLinkInterfaceType(
                                   Config::instance().externalInterfaceType);
    VRLinkExternalInterface* vrlink = NULL;

    if (type == VRLINK_TYPE_DIS)
    {
        vrlink = VRLinkInit(Config::instance().debug,
                        type,
                        (char*)Config::instance().scenarioName.c_str(),
                        (char*)Config::instance().disPort.c_str(),
                        (char*)Config::instance().disDestIpAddress.c_str(),
                        (char*)Config::instance().disDeviceAddress.c_str(),
                        (char*)Config::instance().disSubnetMask.c_str());
    }
    else if (type == VRLINK_TYPE_HLA13
             || type == VRLINK_TYPE_HLA1516)
    {
        vrlink = VRLinkInit(Config::instance().debug,
                        type,
                        (char*)Config::instance().scenarioName.c_str(),
                        (char*)Config::instance().rprVersion.c_str(),
                        (char*)Config::instance().federationName.c_str(),
                        (char*)Config::instance().federateName.c_str(),
                        (char*)Config::instance().fedPath.c_str());
    }


    clocktype simTime = 0;
    clocktype actualTimeOut = Config::instance().timeout * SECOND;
    NodeSet ns;


    while (simTime < actualTimeOut)
    {
        vrlink->Tick(simTime);
        vrlink->Receive();
        ns.extractNodes(vrlink);
        // sleep for 100MS
        Sleep(100);
        simTime = simTime + (100 * MILLI_SECOND);
    }
    ns.processExtractedNodes();

    ConfigFileWriter cfWriter;
    cfWriter.createScenarioDir(Config::instance().scenarioDir);
    std::string path =
                  Config::instance().scenarioDir + Config::instance().dirSep;

    cfWriter.writeRadiosFile(
        path + Config::instance().getParameter("VRLINK-RADIOS-FILE-PATH").value,
        ns);
    cfWriter.writeEntitiesFile(
        path + Config::instance().getParameter("VRLINK-ENTITIES-FILE-PATH").value,
        ns);
    cfWriter.writeNetworksFile(
        path + Config::instance().getParameter("VRLINK-NETWORKS-FILE-PATH").value,
        ns);

    cfWriter.writeNodesFile(
        path + Config::instance().getParameter("NODE-POSITION-FILE").value,
        ns);
    cfWriter.writeConfigFile(
        path + Config::instance().getParameter("CONFIG-FILE-PATH").value,
        ns);
    cfWriter.writeRouterModelFile(
        path + Config::instance().getParameter("ROUTER-MODEL-CONFIG-FILE").value,
        ns);

    if (type != VRLINK_TYPE_DIS)
    {
        cfWriter.copyFederationFile();
    }
    cfWriter.copyIconFiles();

    return 0;
}
