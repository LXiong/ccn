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
// use in compliance with the license agreement as part of the QualNet
// software.  This source code and certain of the algorithms contained
// within it are confidential trade secrets of Scalable Network
// Technologies, Inc. and may not be used as the basis for any other
// software, hardware, product or service.

#include "api.h"
#include "partition.h"
#include "external_util.h"
#include "ocula_interface.h"

void OculaData::setProperty(const std::string key, const std::string& val, clocktype currentTime)
{
    // Set our state
    state->setProperty(key, val);

    OculaProperty* p;
    bool gotProperty = state->getProperty(key, &p);

    // Transmit to GUI
    if (gotProperty
        && ((currentTime > p->m_lastUpdate + 100 * MILLI_SECOND) || p->m_lastUpdate == -1)
        && EXTERNAL_SocketValid(&s))
    {
        p->m_lastUpdate = currentTime;

        char str[MAX_STRING_LENGTH * 10];
        int c;
        c = snprintf(str, MAX_STRING_LENGTH * 10, "%s = %s", key.c_str(), val.c_str());
        ERROR_Assert(c < MAX_STRING_LENGTH * 10, "string overflow");
        ERROR_Assert(str[0] == '/', "First characters of path must be an '/' char.");

        EXTERNAL_SocketErrorType err = EXTERNAL_SocketSend(&s, str, strlen(str) + 1);
        if (err != EXTERNAL_NoSocketError)
        {
            ERROR_ReportErrorArgs("Error %d when sending to Ocula\n", err);
        }
        //printf("sent %s\n", str);
    }
}

//---------------------------------------------------------------------------
// External Interface API Functions
//---------------------------------------------------------------------------

void OculaInitialize(
    EXTERNAL_Interface *iface,
    NodeInput *nodeInput)
{
    char str[MAX_STRING_LENGTH];
    OculaData *data;
    EXTERNAL_SocketErrorType err;

    // Allocate memory for interface-specific data.  The allocated memory
    // is verified by MEM_malloc.  Set the iface->data variable to the
    // newly allocated data for future use.
    data = new OculaData;
    data->state = new OculaState;
    iface->data = (void*) data;

    // Initialize a listening socket and a data socket
    EXTERNAL_SocketInit(&data->s, FALSE);

    SetOculaProperty(
        iface->partition,
        "/locked",
        "1");

    sprintf(str, "%d", (int) iface->partition->numNodes);
    SetOculaProperty(
        iface->partition,
        "/partition/0/numNodes",
        str);

    SetOculaProperty(
        iface->partition,
        "/partition/0/theCurrentTime",
        "0");

    ctoa(iface->partition->maxSimClock, str);
    SetOculaProperty(
        iface->partition,
        "/partition/0/maxSimClock",
        str);

    // Look for global scale
    BOOL wasFound = FALSE;
    IO_ReadString(ANY_NODEID, ANY_ADDRESS, nodeInput, "GUI-NODE-SCALE", &wasFound, str);
    if (wasFound)
    {
        SetOculaProperty(
            iface->partition,
            "/partition/0/scale",
            str);
    }

    // Look for terrain scale
    wasFound = FALSE;
    IO_ReadString(ANY_NODEID, ANY_ADDRESS, nodeInput, "GUI-TERRAIN-SCALE", &wasFound, str);
    if (wasFound)
    {
        SetOculaProperty(
            iface->partition,
            "/partition/0/terrainScale",
            str);
    }

    // Configure terrain

    TerrainData& terrainData = *iface->partition->terrainData;
    if (terrainData.getCoordinateSystem() == LATLONALT)
    {
        SetOculaProperty(
            iface->partition,
            "/partition/0/coordinateSystemType",
            "lat/lon/alt");
    }
    else if (terrainData.getCoordinateSystem() == CARTESIAN)
    {
        SetOculaProperty(
            iface->partition,
            "/partition/0/coordinateSystemType",
            "cartesian");
    }
    else
    {
        ERROR_ReportError("Unknown terrain type");
    }

    sprintf(str, "%f, %f", terrainData.getSW().common.c1, terrainData.getSW().common.c2);
    SetOculaProperty(
        iface->partition,
        "/partition/0/southWestCorner",
        str);

    sprintf(str, "%f, %f", terrainData.getNE().common.c1, terrainData.getNE().common.c2);
    SetOculaProperty(
        iface->partition,
        "/partition/0/northEastCorner",
        str);

    // terrains is in form: "DEM path DEM path"
    std::string* terrains = terrainData.terrainFileList(nodeInput, false);

    std::vector<std::string> terrainStrings;
    char str2[MAX_STRING_LENGTH];
    Coordinates sw;
    Coordinates ne;
    OculaState::StringSplit(*terrains, " ", terrainStrings);
    
    if (terrainStrings.size() > 0)
    {
        sprintf(str, "%d", terrainStrings.size()  - 1);
    }
    else
    {
        strcpy(str, "0");
    }
    SetOculaProperty(
        iface->partition,
        "/partition/0/numTerrains",
        str);
    for (int i = 1; i < terrainStrings.size(); i++)
    {
        sprintf(str, "/partition/0/terrain/%d/name", i - 1);
        SetOculaProperty(
            iface->partition,
            str,
            terrainStrings[i]);

        terrainData.m_elevationData->getElevationBoundaries(i - 1, &sw, &ne);

        sprintf(str, "/partition/0/terrain/%d/southWestCorner", i - 1);
        sprintf(str2, "%f, %f", sw.common.c1, sw.common.c2);
        SetOculaProperty(
            iface->partition,
            str,
            str2);

        sprintf(str, "/partition/0/terrain/%d/northEastCorner", i - 1);
        sprintf(str2, "%f, %f", ne.common.c1, ne.common.c2);
        SetOculaProperty(
            iface->partition,
            str,
            str2);

        double highest;
        double lowest;
        terrainData.m_elevationData->getHighestAndLowestElevation(&sw,
                                              &ne,
                                              &highest,
                                              &lowest);
        sprintf(str, "/partition/0/terrain/%d/highestElevation", i - 1);
        sprintf(str2, "%f", highest);
        SetOculaProperty(
            iface->partition,
            str,
            str2);
        sprintf(str, "/partition/0/terrain/%d/lowestElevation", i - 1);
        sprintf(str2, "%f", lowest);
        SetOculaProperty(
            iface->partition,
            str,
            str2);
    }

    IO_ReadString(ANY_NODEID,
                  ANY_ADDRESS,
                  nodeInput,
                  "GUI-BACKGROUND-IMAGE-FILENAME",
                  &wasFound,
                  str);
    if (!wasFound) {
        strcpy(str, "");
    }

    SetOculaProperty(
        iface->partition,
        "/partition/0/backgroundImage",
        str);

    delete terrains;
}

void OculaInitializeNodes(
    EXTERNAL_Interface *iface,
    NodeInput *nodeInput)
{
    char str[MAX_STRING_LENGTH];
    char str2[MAX_STRING_LENGTH];
    OculaData *data;
    EXTERNAL_SocketErrorType err;

    // Configure interfaces
    for (Node* node = iface->partition->firstNode;
        node != NULL;
        node = node->nextNodeData)
    {
        sprintf(str, "/node/%d/numberPhys", node->nodeId);
        sprintf(str2, "%d", node->numberPhys);
        SetOculaProperty(
            iface->partition,
            str,
            str2);

        for (int i = 0; i < node->numberPhys; i++)
        {
            sprintf(str, "/node/%d/phy/%d/protocol", node->nodeId, i);
            SetOculaProperty(
                iface->partition,
                str,
                PHY_GetPhyName(node, i));

            // Loop over all channels for this phy
            double maxRange = -1;
            double range;
            for (int channelIndex = 0;
                channelIndex < PROP_NumberChannels(node);
                channelIndex++)
            {
                range = PHY_PropagationRange(node, node, i, i, channelIndex, FALSE);
                if (range > maxRange)
                {
                    maxRange = range;
                }
            }

            sprintf(str, "/node/%d/phy/%d/propagationRange", node->nodeId, i);
            sprintf(str2, "%f", maxRange);
            SetOculaProperty(
                iface->partition,
                str,
                str2);
        }

        // Set icon
        BOOL wasFound = FALSE;
        // First reading GUI-NODE-3D-ICON
        IO_ReadString(node->nodeId, ANY_ADDRESS, nodeInput, "GUI-NODE-3D-ICON", &wasFound, str2);
        if (wasFound)
        {
            sprintf(str, "/node/%d/gui/model", node->nodeId);
            SetOculaProperty(
                iface->partition,
                str,
                str2);
        }
        if (!wasFound)
        {
            // Try reading GUI-NODE-2D-ICON
            IO_ReadString(node->nodeId, ANY_ADDRESS, nodeInput, "GUI-NODE-2D-ICON", &wasFound, str2);
            if (wasFound)
            {
                sprintf(str, "/node/%d/gui/model", node->nodeId);
                SetOculaProperty(
                    iface->partition,
                    str,
                    str2);
            }
        }
        if (!wasFound)
        {
            // Try reading NODE-ICON
            IO_ReadString(node->nodeId, ANY_ADDRESS, nodeInput, "NODE-ICON", &wasFound, str2);
            if (wasFound)
            {
                sprintf(str, "/node/%d/gui/model", node->nodeId);
                SetOculaProperty(
                    iface->partition,
                    str,
                    str2);
            }
        }

        // Set scale
        wasFound = FALSE;
        IO_ReadString(node->nodeId, ANY_ADDRESS, nodeInput, "GUI-NODE-SCALE", &wasFound, str2);
        if (wasFound)
        {
            sprintf(str, "/node/%d/gui/scale", node->nodeId);
            SetOculaProperty(
                iface->partition,
                str,
                str2);
        }

        // Set ground mobility
        sprintf(str, "/node/%d/isGroundNode", node->nodeId);
        if (node->mobilityData->groundNode)
        {
            strcpy(str2, "1");
        }
        else
        {
            strcpy(str2, "0");
        }
        SetOculaProperty(
            iface->partition,
            str,
            str2);
    }

    data = (OculaData*) iface->data;

    // Listen for a socket connection on port 5132.  The newly opened socket
    // connection will be returned in the data->s socket structure.
    printf("Connecting to Ocula on port 4000...\n");
    fflush(stdout);
    err = EXTERNAL_SocketConnect(
        &data->s,
        "127.0.0.1",
        4000,
        20);
    if (err != EXTERNAL_NoSocketError)
    {
        ERROR_ReportError("Error connecting");
    }

    // Dump out our current state
    // Do this in two passes: one for partition data, and one for nodes
    std::map<std::string, OculaProperty>::iterator it;
    for (it = data->state->begin();
        it != data->state->end();
        ++it)
    {
        // Verify it's for partition data
        if (it->first.find("/partition") != 0)
        {
            continue;
        }

        char str[MAX_STRING_LENGTH];
        int c;
        c = snprintf(str, 200, "%s = %s", it->first.c_str(), it->second.m_value.c_str());
        ERROR_Assert(c < MAX_STRING_LENGTH, "string overflow");

        EXTERNAL_SocketErrorType err = EXTERNAL_SocketSend(&data->s, str, strlen(str) + 1);
        if (err != EXTERNAL_NoSocketError)
        {
            ERROR_ReportErrorArgs("Error %d when sending to Ocula\n", err);
        }
    }

    for (it = data->state->begin();
        it != data->state->end();
        ++it)
    {
        // Verify it's NOT for partition data
        if (it->first.find("/partition") == 0)
        {
            continue;
        }

        char str[MAX_STRING_LENGTH];
        int c;
        c = snprintf(str, 200, "%s = %s", it->first.c_str(), it->second.m_value.c_str());
        ERROR_Assert(c < MAX_STRING_LENGTH, "string overflow");

        EXTERNAL_SocketErrorType err = EXTERNAL_SocketSend(&data->s, str, strlen(str) + 1);
        if (err != EXTERNAL_NoSocketError)
        {
            ERROR_ReportErrorArgs("Error %d when sending to Ocula\n", err);
        }
        //printf("sent %s\n", str);
    }
    for (it = data->state->begin();
        it != data->state->end();
        ++it)
    {
        // Verify it's NOT for partition data
        if (it->first.find("/partition") == 0)
        {
            continue;
        }

        char str[MAX_STRING_LENGTH];
        int c;
        c = snprintf(str, 200, "%s = %s", it->first.c_str(), it->second.m_value.c_str());
        ERROR_Assert(c < MAX_STRING_LENGTH, "string overflow");

        EXTERNAL_SocketErrorType err = EXTERNAL_SocketSend(&data->s, str, strlen(str) + 1);
        if (err != EXTERNAL_NoSocketError)
        {
            ERROR_ReportErrorArgs("Error %d when sending to Ocula\n", err);
        }
        //printf("sent %s\n", str);
    }

    // And unlock
    SetOculaProperty(
        iface->partition,
        "/locked",
        "0");
}

void OculaReceive(EXTERNAL_Interface *iface)
{
    OculaData *data = (OculaData*) iface->data;

    bool available;
    EXTERNAL_SocketErrorType err = EXTERNAL_SocketDataAvailable(
        &data->s,
        &available);
    if (err != EXTERNAL_NoSocketError)
    {
        ERROR_ReportErrorArgs("Error %d when receiving data from Ocula\n", err);
    }
    while(available)
    {
        char command;
        unsigned int recd;
        err = EXTERNAL_SocketRecv(
            &data->s,
            &command,
            1,
            &recd);
        if (err != EXTERNAL_NoSocketError)
        {
            ERROR_ReportErrorArgs("Error %d when receiving data from Ocula\n", err);
        }

        if (recd == 1)
        {
            if (command == PAUSE_COMMAND)
            {
                EXTERNAL_Pause(iface);
            }
            else if (command == PLAY_COMMAND)
            {
                EXTERNAL_Resume(iface);
            }
            else if (command == HITL_COMMAND)
            {
                // Read hitl command
                // Read space
                err = EXTERNAL_SocketRecv(
                    &data->s,
                    &command,
                    1,
                    &recd);
                if (err != EXTERNAL_NoSocketError)
                {
                    ERROR_ReportErrorArgs("Error %d when receiving data from Ocula\n", err);
                }
                ERROR_Assert(command == ' ', "Expecting space");

                // Read command until 0
                std::string commandStr;
                do
                {
                    err = EXTERNAL_SocketRecv(
                        &data->s,
                        &command,
                        1,
                        &recd);
                    if (err != EXTERNAL_NoSocketError)
                    {
                        ERROR_ReportErrorArgs("Error %d when receiving data from Ocula\n", err);
                    }

                    if (command != 0)
                    {
                        commandStr += command;
                    }
                } while (command != 0);

                if (commandStr != "")
                {
                    GUI_HandleHITLInput(commandStr.c_str(), iface->partition);
                }
            }
            else
            {
                ERROR_ReportError("Unknown command");
            }
        }

        EXTERNAL_SocketErrorType err = EXTERNAL_SocketDataAvailable(
            &data->s,
            &available);
        if (err != EXTERNAL_NoSocketError)
        {
            ERROR_ReportErrorArgs("Error %d when receiving data from Ocula\n", err);
        }
    }
}

void OculaFinalize(EXTERNAL_Interface *iface)
{
    OculaData *data;
    EXTERNAL_SocketErrorType err;

    // Extract interface-specific data
    data = (OculaData*) iface->data;

    // Close the data socket
    err = EXTERNAL_SocketClose(&data->s);
    if (err != EXTERNAL_NoSocketError)
    {
        ERROR_ReportError("Error closing socket");
    }
}
