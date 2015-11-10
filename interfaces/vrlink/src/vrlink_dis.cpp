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

// iostream.h is needed to support DMSO RTI 1.3v6 (non-NG).

#ifndef DtDIS
#error This file should only be used in the DIS specific build
#endif

#include "vrlink_shared.h"
#include "vrlink_dis.h"

#include <vlutil/vlInetDevice.h>
#include <vlutil/vlDisSocket.h>

// /**
// FUNCTION :: VRLinkDIS
// PURPOSE :: Initializing function for DIS.
// PARAMETERS ::
// + ifacePtr : EXTERNAL_Interface* : External Interface pointer.
// + nodeInput : NodeInput* : Pointer to cached file.
// **/
VRLinkDIS::VRLinkDIS(
    EXTERNAL_Interface* ifacePtr,
    NodeInput* nodeInput,
    VRLinkSimulatorInterface *simIface) : VRLink(ifacePtr, nodeInput, simIface)
{
    InitVariables(ifacePtr, nodeInput);
}

// /**
//FUNCTION :: InitVariables
//PURPOSE :: To initialize the DIS specific variables with default values.
//PARAMETERS ::
// + ifacePtr : EXTERNAL_Interface* : External Interface pointer.
// + nodeInput : NodeInput* : Pointer to cached file.
//RETURN :: void : NULL
// **/
void VRLinkDIS::InitVariables(
    EXTERNAL_Interface* ifacePtr,
    NodeInput* nodeInput)
{
    iface = ifacePtr;
    if (simIface->GetPartitionIdForIface(ifacePtr) == 0)
    {
        exerciseIdentifier = 1;
        receiveDelay = 0;
        maxReceiveDuration = 0;
        ipAddressIsMulticast = false;
        ttl = 2;
        deviceAddress = DtStringToInetAddr("127.0.0.1");
        isSubnetMaskSet = false;
    }
}

#ifdef MULTIPLE_MULTICAST_INTERFACE_SUPPORT
//Multigroup feature currently DISABLED
/*void VRLinkDIS::InitMulticastParameters(NodeInput* nodeInput)
{
        
    // group format: 
    // VRLINK-JOIN-MULTICAST-GROUP[0] <interface-IP-address, multicast-IP-address>
    int numGroups = 0;
    BOOL isFound = FALSE;
    simIface->IO_ReadInt(ANY_NODEID,
               ANY_ADDRESS,
               nodeInput,
               "VRLINK-NUM-MULTICAST-GROUPS",
               &isFound,
               &numGroups);
    
    char buf[MAX_STRING_LENGTH];
    for (int groupIndex = 0; groupIndex < numGroups; groupIndex ++)
    {        

        simIface->IO_ReadStringInstance(
            ANY_NODEID,
            ANY_ADDRESS,
            nodeInput,
            "VRLINK-JOIN-MULTICAST-GROUP",
            groupIndex,
            FALSE,
            &isFound,
            buf);
                
        if (isFound == TRUE) 
        {
            // split the buf            
            char localBuffer[MAX_STRING_LENGTH];
            char *stringPtr;

            simIface->IO_GetDelimitedToken(localBuffer, buf, "(,) ", &stringPtr);
            //printf("%s\n", localBuffer);
            
            DtInetDevice *intf = NULL;
            if (strcmp(localBuffer, "default"))
            {                                
                if (isalpha((int)*localBuffer))
                {
                    intf = new DtInetDevice(DtString(localBuffer));
                }else 
                {
                    DtInetAddr ip = DtStringToInetAddr(localBuffer);
                    intf = new DtInetDevice(ip);            
                }

                // check if the ip is a valid interface address

                if (!intf->isOk())
                {
                    char errString[MAX_STRING_LENGTH];
                    sprintf(errString, "%s: invalid interface address\n",
                        buf);
                    simIface->ERROR_ReportError(errString);
                }
            
            }
                
            simIface->IO_GetDelimitedToken(localBuffer, stringPtr, "(,) ", &stringPtr);            
            //printf("%s\n", localBuffer);

            if (localBuffer)
            {        
                // find group address in token2
                DtInetAddr group = DtStringToInetAddr(localBuffer);

                //ERROR_Assert(IsMulticastIpAddress(group.toULong()),
                //    "Please configure a multicast group address.");
                myMcastGroups.push_back(DisMcastGroupIntfcBinding(group, intf));

                if (debug)
                {
                    if (intf)
                    {
                    printf("join on mcast %s interface %s \n",
                        group.toDtString().c_str(), 
                        intf->macAddrString().c_str());
                    }else {
                        printf("join on mcast %s \n",
                        group.toDtString().c_str());            
                    }
                }
           
            }
            else {
                //ERROR_Assert(FALSE, "Please provide multicast group address. ");
            }
        }    
    }
}*/
#endif

// /**
//FUNCTION :: InitConfigurableParameters
//PURPOSE :: To initialize the user configurable DIS parameters or initialize
//           the corresponding variables with the default values.
//PARAMETERS ::
// + nodeInput : NodeInput* : Pointer to cached file.
//RETURN :: void : NULL
// **/
void VRLinkDIS::InitConfigurableParameters(NodeInput* nodeInput)
{
    BOOL parameterFound;
    char buf[MAX_STRING_LENGTH];

    simIface->IO_ReadString(ANY_NODEID,
                            ANY_ADDRESS,
                            nodeInput,
                            "DIS",
                            &parameterFound,
                            buf);

    if (parameterFound && strcmp(buf, "YES") == 0)
    {
        ReadLegacyParameters(nodeInput);
        return;
    }

    VRLink::InitConfigurableParameters(nodeInput);

    simIface->ReadString(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "VRLINK-DEBUG-PRINT-COMMS",
                &parameterFound,
                buf);

    cout << "VRLink debugging output (Comms)           = ";

    if (parameterFound && strcmp(buf, "NO") == 0)
    {
        debugPrintComms = false;
        cout << "Off" << endl;
    }
    else
    {
        debugPrintComms = true;
        cout << "On" << endl;
    }

    simIface->ReadString(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "VRLINK-DEBUG-PRINT-COMMS-2",
                &parameterFound,
                buf);

    cout << "VRLink debugging output (Comms-2)         = ";

    if (parameterFound && strcmp(buf, "NO") == 0)
    {
        debugPrintComms2 = false;
        cout << "Off" << endl;
    }
    else
    {
        debugPrintComms2 = true;
        cout << "On" << endl;
    }

    simIface->ReadString(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "VRLINK-DEBUG-PRINT-MAPPING",
                &parameterFound,
                buf);

    cout << "VRLink debugging output (mapping)         = ";

    if (parameterFound && strcmp(buf, "NO") == 0)
    {
        debugPrintMapping = false;
        cout << "Off" << endl;
    }
    else
    {
        debugPrintMapping = true;
        cout << "On" << endl;
    }

    simIface->ReadString(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "VRLINK-DEBUG-PRINT-DAMAGE",
                &parameterFound,
                buf);

    cout << "VRLink debugging output (damage)          = ";

    if (parameterFound && strcmp(buf, "NO") == 0)
    {
        debugPrintDamage = false;
        cout << "Off" << endl;
    }
    else
    {
        debugPrintDamage = true;
        cout << "On" << endl;
    }

    simIface->ReadString(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "VRLINK-DEBUG-PRINT-TX-STATE",
                &parameterFound,
                buf);

    cout << "VRLink debugging output (TX state)        = ";

    if (parameterFound && strcmp(buf, "NO") == 0)
    {
        debugPrintTxState = false;
        cout << "Off" << endl;
    }
    else
    {
        debugPrintTxState = true;
        cout << "On" << endl;
    }

    simIface->ReadString(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "VRLINK-DEBUG-PRINT-TX-POWER",
                &parameterFound,
                buf);

    cout << "VRLink debugging output (TX power)        = ";

    if (parameterFound && strcmp(buf, "NO") == 0)
    {
        debugPrintTxPower = false;
        cout << "Off" << endl;
    }
    else
    {
        debugPrintTxPower = true;
        cout << "On" << endl;
    }

    simIface->ReadString(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "VRLINK-DEBUG-PRINT-MOBILITY",
                &parameterFound,
                buf);

    cout << "VRLink debugging output (mobility)        = ";

    if (parameterFound && strcmp(buf, "YES") == 0)
    {
        debugPrintMobility = true;
        cout << "On" << endl;
    }
    else
    {
        debugPrintMobility = false;
        cout << "Off" << endl;
    }

    simIface->ReadString(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "VRLINK-DEBUG-PRINT-TRANSMITTER-PDU",
                &parameterFound,
                buf);

    cout << "VRLink debugging output (Transmitter PDU) = ";

    if (parameterFound && strcmp(buf, "YES") == 0)
    {
        debugPrintTransmitterPdu = true;
        cout << "On" << endl;
    }
    else
    {
        debugPrintTransmitterPdu = false;
        cout << "Off" << endl;
    }

    simIface->ReadString(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "VRLINK-DEBUG-PRINT-PDUS",
                &parameterFound,
                buf);

    cout << "VRLink debugging output (PDU hex dump)    = ";

    if (parameterFound && strcmp(buf, "YES") == 0)
    {
        debugPrintPdus = true;
        cout << "On" << endl;
    }
    else
    {
        debugPrintPdus = false;
        cout << "Off" << endl;
    }

    cout << endl;

    simIface->ReadString(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "VRLINK-DIS-NETWORK-INTERFACE",
                &parameterFound,
                buf);

    if (parameterFound)
    {
        deviceAddress = DtStringToInetAddr(buf);

        cout << "Using the following network interface for DIS: "
                << buf << "." << endl;
    }
    else
    {
        cout << "Using the following network interface for DIS: 127.0.0.1\n";
    }

    simIface->ReadString(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "VRLINK-DIS-IP-ADDRESS",
                &parameterFound,
                buf);

    if (!parameterFound)
    {
        // Parameter was not specified.

        // Default to listening to local broadcast IP address.
        strcpy(buf, "127.255.255.255");
    }

    ipAddress = DtStringToInetAddr(buf);
    ipAddressIsMulticast = IsMulticastIpAddress(ipAddress);

    if (ipAddressIsMulticast)
    {
        cout << "Listening for DIS PDUs at multicast IP address "
                << buf << "." << endl;
    }
    else
    {
        cout << "Listening for DIS PDUs at local broadcast IP address "
                << buf << "." << endl;
    }    

    simIface->ReadString(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "VRLINK-DIS-SUBNET-MASK",
                &parameterFound,
                buf);

    if (parameterFound)
    {
        subnetMask = DtStringToInetAddr(buf);
        isSubnetMaskSet = true;
    }    

    int int_disPort;

    simIface->ReadInt(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "VRLINK-DIS-PORT",
                &parameterFound,
                &int_disPort);

    if (parameterFound)
    {
        VRLinkVerify(int_disPort > 0, "DIS port must be > 0");
        VRLinkVerify(int_disPort <= USHRT_MAX, "DIS port too large");

        port = (unsigned short) int_disPort;
    }
    else
    {
        port = 3000;
    }

    cout << "Listening for DIS PDUs on UDP port " << port << "."<< endl;

    int int_exerciseIdentifier;

    simIface->IO_ReadInt(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "VRLINK-DIS-EXERCISE-ID",
        &parameterFound,
        &int_exerciseIdentifier);

    if (parameterFound)
    {
        VRLinkVerify(int_exerciseIdentifier >= 0,
                     "DIS Exercise Identifier must be >= 0");
        VRLinkVerify(int_exerciseIdentifier <= UCHAR_MAX,
                     "DIS Exercise Identifier too large");

        exerciseIdentifier = (unsigned char) int_exerciseIdentifier;
    }

    cout << "Listening for DIS PDUs with Exercise ID: "
             << (unsigned) exerciseIdentifier << "."
             << endl;

    cout << endl;

    simIface->ReadTime(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "VRLINK-RECEIVE-DELAY",
                &parameterFound,
                &receiveDelay);

    if (parameterFound)
    {
        VRLinkVerify(receiveDelay >= 0, "Can't use negative time");
    }
    else
    {
        receiveDelay = 200 * MILLI_SECOND;
    }

    char receiveDelayString[MAX_STRING_LENGTH];
    simIface->PrintClockInSecond(receiveDelay, receiveDelayString);

    cout << "VRLink interface receive delay            = " << receiveDelayString
            << " second(s)" << endl;

    simIface->SetExternalReceiveDelay(iface, receiveDelay);

    simIface->ReadTime(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "VRLINK-MAX-RECEIVE-DURATION",
                &parameterFound,
                &maxReceiveDuration);

    if (parameterFound)
    {
        VRLinkVerify(maxReceiveDuration >= 0, "Can't use negative time");
    }
    else
    {
        maxReceiveDuration = 5 * MILLI_SECOND;
    }

    char maxReceiveDurationString[MAX_STRING_LENGTH];
    simIface->PrintClockInSecond(maxReceiveDuration, maxReceiveDurationString);

    cout << "VRLink interface max receive duration     = "
            << maxReceiveDurationString << " second(s)" << endl;

    // find ttl
    int val;
    simIface->IO_ReadInt(ANY_NODEID,
               ANY_ADDRESS,
               nodeInput,
               "VRLINK-MULTICAST-TTL",
               &parameterFound,
               &val);

    if (parameterFound && ttl < 0 || ttl > 255)
    {
        char errString[MAX_STRING_LENGTH];
        sprintf(errString, "%s: invalid ttl \n",
            buf);
        simIface->ERROR_ReportError(errString);
    }
    else if (parameterFound)
    {
        ttl = val;
    }

    simIface->IO_ReadString(ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "VRLINK-DISABLE-REFLECTED-RADIO-TRANSMITTER-TIMEOUTS",
        &parameterFound,
        buf);

    if (parameterFound && strcmp(buf, "YES") == 0)
    {
        disableReflectRadioTimeouts = true;
    }

#ifdef MULTIPLE_MULTICAST_INTERFACE_SUPPORT
    //Multigroup feature currently DISABLED
    //InitMulticastParameters(nodeInput);
#endif
}


// /**
//FUNCTION :: ReadLegacyParameters
//PURPOSE :: To initialize the user configurable DIS parameters or initialize
//           the corresponding variables with the default values.
//PARAMETERS ::
// + nodeInput : NodeInput* : Pointer to cached file.
//RETURN :: void : NULL
// **/
void VRLinkDIS::ReadLegacyParameters(NodeInput* nodeInput)
{
    BOOL parameterFound;
    char buf[MAX_STRING_LENGTH];

    simIface->ReadString(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "DIS-DEBUG-PRINT-COMMS",
                &parameterFound,
                buf);

    cout << "DIS debugging output (Comms)           = ";

    if (parameterFound && strcmp(buf, "NO") == 0)
    {
        debugPrintComms = false;
        cout << "Off" << endl;
    }
    else
    {
        debugPrintComms = true;
        cout << "On" << endl;
    }

    simIface->ReadString(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "DIS-DEBUG-PRINT-COMMS-2",
                &parameterFound,
                buf);

    cout << "DIS debugging output (Comms-2)         = ";

    if (parameterFound && strcmp(buf, "NO") == 0)
    {
        debugPrintComms2 = false;
        cout << "Off" << endl;
    }
    else
    {
        debugPrintComms2 = true;
        cout << "On" << endl;
    }

    simIface->ReadString(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "DIS-DEBUG-PRINT-MAPPING",
                &parameterFound,
                buf);

    cout << "DIS debugging output (mapping)         = ";

    if (parameterFound && strcmp(buf, "NO") == 0)
    {
        debugPrintMapping = false;
        cout << "Off" << endl;
    }
    else
    {
        debugPrintMapping = true;
        cout << "On" << endl;
    }

    simIface->ReadString(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "DIS-DEBUG-PRINT-DAMAGE",
                &parameterFound,
                buf);

    cout << "DIS debugging output (damage)          = ";

    if (parameterFound && strcmp(buf, "NO") == 0)
    {
        debugPrintDamage = false;
        cout << "Off" << endl;
    }
    else
    {
        debugPrintDamage = true;
        cout << "On" << endl;
    }

    simIface->ReadString(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "DIS-DEBUG-PRINT-TX-STATE",
                &parameterFound,
                buf);

    cout << "DIS debugging output (TX state)        = ";

    if (parameterFound && strcmp(buf, "NO") == 0)
    {
        debugPrintTxState = false;
        cout << "Off" << endl;
    }
    else
    {
        debugPrintTxState = true;
        cout << "On" << endl;
    }

    simIface->ReadString(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "DIS-DEBUG-PRINT-TX-POWER",
                &parameterFound,
                buf);

    cout << "DIS debugging output (TX power)        = ";

    if (parameterFound && strcmp(buf, "NO") == 0)
    {
        debugPrintTxPower = false;
        cout << "Off" << endl;
    }
    else
    {
        debugPrintTxPower = true;
        cout << "On" << endl;
    }

    simIface->ReadString(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "DIS-DEBUG-PRINT-MOBILITY",
                &parameterFound,
                buf);

    cout << "DIS debugging output (mobility)        = ";

    if (parameterFound && strcmp(buf, "YES") == 0)
    {
        debugPrintMobility = true;
        cout << "On" << endl;
    }
    else
    {
        debugPrintMobility = false;
        cout << "Off" << endl;
    }

    simIface->ReadString(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "DIS-DEBUG-PRINT-TRANSMITTER-PDU",
                &parameterFound,
                buf);

    cout << "DIS debugging output (Transmitter PDU) = ";

    if (parameterFound && strcmp(buf, "YES") == 0)
    {
        debugPrintTransmitterPdu = true;
        cout << "On" << endl;
    }
    else
    {
        debugPrintTransmitterPdu = false;
        cout << "Off" << endl;
    }

    simIface->ReadString(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "DIS-DEBUG-PRINT-PDUS",
                &parameterFound,
                buf);

    cout << "DIS debugging output (PDU hex dump)    = ";

    if (parameterFound && strcmp(buf, "YES") == 0)
    {
        debugPrintPdus = true;
        cout << "On" << endl;
    }
    else
    {
        debugPrintPdus = false;
        cout << "Off" << endl;
    }

    cout << endl;

    cout << "Using the following network interface for DIS: 127.0.0.1\n";

    simIface->ReadString(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "DIS-IP-ADDRESS",
                &parameterFound,
                buf);

    if (!parameterFound)
    {
        // Parameter was not specified.

        // Default to listening to local broadcast IP address.

        strcpy(buf, "127.255.255.255");
        ipAddress = DtStringToInetAddr(buf);
    }
    else
    {
        // Parameter was specified, but was the old default, upgrade it to the new default
        if (strcmp(buf, "255.255.255.255") == 0)
        {
            strcpy(buf, "127.255.255.255");
        }
        else
        {
            cout << "WARNING: " << buf << " may not be reachable by the loopback interface.\n"
                << "VR-Link parameter upgrade is required to select another network interface.\n" << endl;
        }

        ipAddress = DtStringToInetAddr(buf);
        ipAddressIsMulticast = IsMulticastIpAddress(ipAddress);
    }

    if (ipAddressIsMulticast)
    {
        cout << "Listening for DIS PDUs at multicast IP address "
                << buf << "." << endl;
    }
    else
    {
        cout << "Listening for DIS PDUs at local broadcast IP address "
                << buf << "." << endl;
    }    

    int int_disPort;

    simIface->ReadInt(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "DIS-PORT",
                &parameterFound,
                &int_disPort);

    if (parameterFound)
    {
        VRLinkVerify(int_disPort > 0, "DIS port must be > 0");
        VRLinkVerify(int_disPort <= USHRT_MAX, "DIS port too large");

        port = (unsigned short) int_disPort;
    }
    else
    {
        port = 3000;
    }

    cout << "Listening for DIS PDUs on UDP port " << port << "."<< endl;

    int int_exerciseIdentifier;

    simIface->IO_ReadInt(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "DIS-EXERCISE-ID",
        &parameterFound,
        &int_exerciseIdentifier);

    if (parameterFound)
    {
        VRLinkVerify(int_exerciseIdentifier >= 0,
                     "DIS Exercise Identifier must be >= 0");
        VRLinkVerify(int_exerciseIdentifier <= UCHAR_MAX,
                     "DIS Exercise Identifier too large");

        exerciseIdentifier = (unsigned char) int_exerciseIdentifier;        
    }

    cout << "Listening for DIS PDUs with Exercise ID: "
                 << (unsigned) exerciseIdentifier << "."
                 << endl;

    cout << endl;

    simIface->ReadTime(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "DIS-RECEIVE-DELAY",
                &parameterFound,
                &receiveDelay);

    if (parameterFound)
    {
        VRLinkVerify(receiveDelay >= 0, "Can't use negative time");
    }
    else
    {
        receiveDelay = 200 * MILLI_SECOND;
    }

    char receiveDelayString[MAX_STRING_LENGTH];
    simIface->PrintClockInSecond(receiveDelay, receiveDelayString);

    cout << "DIS interface receive delay            = " << receiveDelayString
            << " second(s)" << endl;

    simIface->SetExternalReceiveDelay(iface, receiveDelay);

    simIface->ReadTime(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "DIS-MAX-RECEIVE-DURATION",
                &parameterFound,
                &maxReceiveDuration);

    if (parameterFound)
    {
        VRLinkVerify(maxReceiveDuration >= 0, "Can't use negative time");
    }
    else
    {
        maxReceiveDuration = 5 * MILLI_SECOND;
    }

    char maxReceiveDurationString[MAX_STRING_LENGTH];
    simIface->PrintClockInSecond(maxReceiveDuration, maxReceiveDurationString);

    cout << "DIS interface max receive duration     = "
            << maxReceiveDurationString << " second(s)" << endl;

    simIface->ReadTime(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "DIS-MOBILITY-INTERVAL",
                &parameterFound,
                &mobilityInterval);

    if (parameterFound)
    {
        VRLinkVerify(mobilityInterval >= 0.0, "Can't use negative time");
    }
    else
    {
        mobilityInterval = 500 * MILLI_SECOND;
    }

    char mobilityIntervalString[MAX_STRING_LENGTH];
    simIface->PrintClockInSecond(mobilityInterval, mobilityIntervalString);

    cout << "DIS mobility interval                  = " << mobilityIntervalString
            << " second(s)" << endl;

    cout << endl;

    BOOL retVal = false;

    simIface->ReadDouble(
                ANY_NODEID,
                ANY_ADDRESS,
                nodeInput,
                "DIS-XYZ-EPSILON",
                &retVal,
                &xyzEpsilon);

    if (retVal == FALSE)
    {
        xyzEpsilon = 0.57735026918962576450914878050196;
    }

    VRLinkVerify(xyzEpsilon >= 0.0, "Can't use negative epsilon value");

    cout << "GCC (x,y,z) epsilons are ("
            << xyzEpsilon << ","
            << xyzEpsilon << ","
            << xyzEpsilon << " meter(s))" << endl
         << "  For movement to be reflected in QualNet, change in position"
            " must be" << endl
         << "  >= any one of these values." << endl;

    char path[g_PathBufSize];

    simIface->IO_ReadString(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "DIS-ENTITIES-FILE-PATH",
        &retVal,
        path);

    VRLinkVerify(
        retVal == TRUE,
        "DIS-ENTITIES-FILE-PATH must be specified",
        __FILE__, __LINE__);

    entitiesPath = path;

    simIface->IO_ReadString(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "DIS-RADIOS-FILE-PATH",
        &retVal,
        path);

    VRLinkVerify(
        retVal == TRUE,
        "DIS-RADIOS-FILE-PATH must be specified",
        __FILE__, __LINE__);

    radiosPath = path;

    simIface->IO_ReadString(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "DIS-NETWORKS-FILE-PATH",
        &retVal,
        path);

    VRLinkVerify(
        retVal == TRUE,
        "DIS-NETWORKS-FILE-PATH must be specified",
        __FILE__, __LINE__);

    networksPath = path;
}


// /**
//FUNCTION :: CreateFederation
//PURPOSE :: Initializes QualNet on the specified port and exercise id for
//           DIS.
//PARAMETERS :: None
//RETURN :: void : NULL
// **/
void VRLinkDIS::CreateFederation()
{
    DtExerciseConnInitializer initializer;
    initializer.setPort(port);
    initializer.setSiteId(siteId);
    initializer.setExerciseId((int)exerciseIdentifier);
    initializer.setApplicationNumber(applicationId);
    initializer.setDestinationAddress(ipAddress.toDtString());
    initializer.setDeviceAddress(DtString(deviceAddress.toDtString()));
    if (isSubnetMaskSet)
    {
        initializer.setSubnetMask(DtString(subnetMask.toDtString()));
    }

    if (ipAddressIsMulticast)
    {
        initializer.setMulticastTtl((int)ttl); // ttl = 2 is the default
    }

    DtExerciseConn::InitializationStatus status;
    
    try
    {
        exConn = new DtExerciseConn(
                        initializer,
                        &status);
       
    }DtCATCH_AND_WARN(cout);

    if (status != 0)
    {
        VRLinkReportError(
            "VRLINK: could not create dis socket",
                __FILE__, __LINE__);
    }

    DtInetSocket *  skt = (DtInetSocket *) exConn->socket();
    DtDisSocket * dskt;
    if (skt && skt->isOk())
    {
        dskt = dynamic_cast<DtDisSocket*>(skt);
        if (!dskt )
        {
            char errorString[MAX_STRING_LENGTH] = {0};
            sprintf(errorString, "Error in federation creation to: %s\n",
                ipAddress.toDtString().c_str());
            //ERROR_Assert(FALSE, errorString);
        }
    }   
    else
    {
        char errorString[MAX_STRING_LENGTH] = {0};
        sprintf(errorString, "Error in federation creation to: %s\n",
            ipAddress.toDtString().c_str());
        //ERROR_Assert(FALSE, errorString);
    }

#ifdef MULTIPLE_MULTICAST_INTERFACE_SUPPORT
    // Feature currently DISABLED
    //for (size_t s = 0; s != myMcastGroups.size(); ++s)
    //{
    //            
    //    if (myMcastGroups[s].second)
    //    {
    //        printf("join on group addr %s, intf addr %s \n", 
    //            myMcastGroups[s].first.toDtString().c_str(),
    //            myMcastGroups[s].second->ipv4Addr()->toDtString().c_str());
    //    }
    //    
    //    dskt->
    //        joinMulticastGroup(
    //        myMcastGroups[s].first, myMcastGroups[s].second);
    //}
#endif
}


// /**
// FUNCTION :: RegisterProtocolSpecificCallbacks
// PURPOSE :: Registers DIS specific callbacks with the federation.
// PARAMETERS :: None.
// RETURN :: void : NULL
// **/
void VRLinkDIS::RegisterProtocolSpecificCallbacks()
{
    //there are currently no DIS specific callbacks; timeout processing is
    //currently not enabled for DIS.
    
    //ReflectedRadioList should never disable the timeout feature.  Otherwise a
    //final pdu could be lost and the entity will never be removed from the list then

    //refEntityList->setTimeoutProcessing(false);
    
    if (disableReflectRadioTimeouts)
    {
        refRadioTxList->setTimeoutProcessing(false);
    }
}

// /**
// FUNCTION :: GetRadioIdKey
// PURPOSE :: Get the RadioIdKey for signal interaction.
// PARAMETERS ::
// + dataIxnInfo : DtSignalInteraction* : Pointer to signal interaction
// RETURN :: RadioIdKey : EntityIdentifierKey and int pair
// **/
RadioIdKey VRLinkDIS::GetRadioIdKey(DtSignalInteraction* inter)
{
    EntityIdentifierKey* entityKey = GetEntityIdKeyFromDtEntityId(
                                                         &inter->entityId());

    return pair<EntityIdentifierKey, unsigned int>(
                                               *entityKey, inter->radioId());
}

// /**
// FUNCTION :: GetRadioIdKey
// PURPOSE :: Get the RadioIdKey for radio transmitter repository.
// PARAMETERS ::
// + dataIxnInfo : DtRadioTransmitterRepository* : Pointer to radio 
//                  transmitter repository
// RETURN :: RadioIdKey : EntityIdentifierKey and int pair
// **/
RadioIdKey VRLinkDIS::GetRadioIdKey(DtRadioTransmitterRepository* radioTsr)
{
    EntityIdentifierKey* entityKey = GetEntityIdKeyFromDtEntityId(
                                                      &radioTsr->entityId());

    return pair<EntityIdentifierKey, unsigned int>(
                                            *entityKey, radioTsr->radioId());
}

// /**
// FUNCTION :: ProcessEvent
// PURPOSE :: Handles all the protocol events for DIS.
// PARAMETERS ::
// + node : Node* : Pointer to node.
// + msg : Message* : Pointer to message.
// RETURN :: void : NULL
// **/
void VRLinkDIS::ProcessEvent(Node* node, Message* msg)
{
    //DIS process event
    switch (simIface->GetMessageEventTypesUsedInVRLink(
        simIface->ReturnEventTypeFromMsg(msg)))
    {
        case EXTERNAL_VRLINK_HierarchyMobility:
            {
                AppProcessMoveHierarchy(node, msg);
                break;
            }
        case EXTERNAL_VRLINK_ChangeMaxTxPower:
            {
                AppProcessChangeMaxTxPowerEvent(msg);
                break;
            }
        case EXTERNAL_VRLINK_AckTimeout:
            {
                AppProcessTimeoutEvent(msg);
                break;
            }
        case EXTERNAL_VRLINK_SendRtss:
            {
                if (simIface->IsMilitaryLibraryEnabled())
                {
                    Node* rtssNode;
                    RtssForwardedInfo* rtssForwardedInfo = (RtssForwardedInfo*)
                                              simIface->ReturnInfoFromMsg(msg);

                    //You are on the partition with the node so get node* from local node hash
                    rtssNode = simIface->GetNodePtrFromOtherNodesHash(
                            node, rtssForwardedInfo->nodeId, false);

                    AppProcessSendRtssEvent(rtssNode);
                    break;
                }
            }
        case EXTERNAL_VRLINK_SendRtssForwarded:
            {
                if (simIface->IsMilitaryLibraryEnabled())
                {
                    // See ProcessSendRtssEvent() for sender...
                    // This messages was MSG_EXTERNAL_VRLINK_SendRtss, but
                    // we are receiving this notification from a remote
                    // partition and thus have to locate the node by nodeId.
                    Node* rtssNode;
                    RtssForwardedInfo* rtssForwardedInfo = (RtssForwardedInfo*)
                                              simIface->ReturnInfoFromMsg(msg);
                    rtssNode = simIface->GetNodePtrFromOtherNodesHash(
                            node, rtssForwardedInfo->nodeId, true);
                    SendRtssNotification(rtssNode);
                    break;
                }
            }
        case EXTERNAL_VRLINK_StartMessengerForwarded:
            {
                // See method SendSimulatedMsgUsingMessenger()
                // for sender...
                EXTERNAL_VRLinkStartMessenegerForwardedInfo* startInfo =
                    (EXTERNAL_VRLinkStartMessenegerForwardedInfo*)
                    simIface->ReturnInfoFromMsg(msg);
                Node* srcNode;
                srcNode = simIface->GetNodePtrFromOtherNodesHash(
                        node, startInfo->srcNodeId, false);

                StartSendSimulatedMsgUsingMessenger(
                    srcNode,
                    startInfo->srcNetworkAddress,
                    startInfo->destNodeId,
                    startInfo->smInfo,
                    startInfo->requestedDataRate,
                    startInfo->dataMsgSize,
                    startInfo->voiceMsgDuration,
                    startInfo->isVoice,
                    startInfo->timeoutDelay,
                    startInfo->unicast,
                    startInfo->sendDelay,
                    startInfo->isLink11,
                    startInfo->isLink16);
                    break;
            }
        case EXTERNAL_VRLINK_CompletedMessengerForwarded:
            {
                EXTERNAL_VRLinkCompletedMessenegerForwardedInfo*
                    completedInfo =
                           (EXTERNAL_VRLinkCompletedMessenegerForwardedInfo*)
                            simIface->ReturnInfoFromMsg(msg);
                
                    Node* destNode = NULL;
                    destNode = simIface->GetNodePtrFromOtherNodesHash(
                            node, completedInfo->destNodeId, true);
                    if (!destNode)
                    {
                        destNode = simIface->GetNodePtrFromOtherNodesHash(
                                node, completedInfo->destNodeId, false);
                    }
                    AppMessengerResultCompleted(destNode,
                                                completedInfo->smInfo,
                                                completedInfo->success);
                break;
            }
        case EXTERNAL_VRLINK_InitialPhyMaxTxPowerRequest:
            {
                AppProcessGetInitialPhyTxPowerRequest(node, msg);
                break;
            }
        case EXTERNAL_VRLINK_InitialPhyMaxTxPowerResponse:
            {
                AppProcessGetInitialPhyTxPowerResponse(msg);
                break;
            }
        default:
            VRLinkReportError(
                "Unknown message type encountered",
                __FILE__, __LINE__);
    }//end of switch

    simIface->FreeMessage(node, msg);
}

// /**
// FUNCTION :: ProcessSignalIxn
// PURPOSE :: Processes the signal interaction received from an outside
//            federate.
// PARAMETERS ::
// + inter : DtSignalInteraction* : Received interaction.
// RETURN :: void : NULL
// **/
void VRLinkDIS::ProcessSignalIxn(DtSignalInteraction* inter)
{
    unsigned protocolId = *((unsigned*)inter->data());
    DtEncodingClass encodingClass = inter->encodingClass();

    if (debugPrintComms2)
    {
        cout << "VRLINK: Received Signal PDU (";
    }

    simIface->EXTERNAL_ntoh(&protocolId, sizeof(protocolId));
    if ((protocolId == cerUserProtocolId) &&
        (encodingClass == DtApplicationSpecific))
    {
        if (debugPrintComms2)
        {
            cout << "QualNet Comm Effects Request\n";
        }
        if (debugPrintPdus)
        {
            cout<<"\nSignal PDU Details:\n";
            inter->printDataToStream(cout);
            cout << "\n--" << endl;
        }

        ProcessCommEffectsRequest(inter);
    }
    else
    {
        if (debugPrintComms2)
        {
            cout << encodingClass;

            if (encodingClass == DtApplicationSpecific)
            {
                cout << ", " << protocolId;
            }
        }
    }

    if (debugPrintComms2)
    {
        cout << ")" << endl;
    }
}

// /**
// FUNCTION :: AppProcessSendRtssEvent
// PURPOSE :: Handles the event type MSG_EXTERNAL_VRLINK_SendRtss.
// PARAMETERS ::
// + node : Node* : Pointer to node.
// RETURN :: void : NULL
// **/
void VRLinkDIS::AppProcessSendRtssEvent(Node* node)
{
    if (simIface->IsMilitaryLibraryEnabled())
    {        
        if (simIface->GetLocalPartitionIdForNode(node) != 0)
        {
            // We currently are not on p0, this is a remote node processing this;
            // forward the processing of the ready to send event to p0
            // We'd like to use MESSAGE_RemoteSend (node, destNodeId, msg, -1);
            // but we can't becuase that looks up which partition the node
            // lives on, and we actually need to send to p0 proxy node.
            // We'd like to use EXTERNAL_MESSAGE_SendAnyNode () but again we
            // can't because that schedules on the partition where the node
            // really lives.
            // 1.) Allocate an RtssEventNotification message

            Message * rtssForwardedMsg = simIface->AllocateMessage(
                node,
                simIface->GetExternalLayerId(),       // special layer
                simIface->GetVRLinkExternalInterfaceType(),      // protocol
                simIface->GetMessageEventType(EXTERNAL_VRLINK_SendRtssForwarded));
            // 2.) Set info - info is the node's nodeId

            simIface->AllocateInfoInMsg (
                node,
                rtssForwardedMsg,
                sizeof(RtssForwardedInfo));

            RtssForwardedInfo * rtssForwardedInfo = (RtssForwardedInfo *)
                simIface->ReturnInfoFromMsg (rtssForwardedMsg);

            rtssForwardedInfo->nodeId = simIface->GetNodeId(node);

            // 3.) Forward the message to p0 - vrlink interface on p0 will then
            // be able complete handling via SendRtssNotification()

            EXTERNAL_Interface* iface =
                                   simIface->GetExternalInterfaceForVRLink(node);

            simIface->RemoteExternalSendMessage (
                          iface,
                          0,
                          rtssForwardedMsg,
                          0,
                          simIface->GetExternalScheduleSafeType());
        }
        else
        {
            SendRtssNotification(node);
        }
    }
}

// /**
// FUNCTION :: ConvertStringToIpAddress
// PURPOSE :: Converts the string passed to ipAddress.
// PARAMETERS ::
// + s : const char* : String to be converted.
// + ipAddress : unsigned& : Resulting ipAddress (i.e. after conversion).
// RETURN :: bool : Returns TRUE if passed string has been succesfully
//                  converted to ipAddress; FALSE otherwise.
// **/
bool ConvertStringToIpAddress(const char* s, unsigned& ipAddress)
{
    unsigned short a, b, c, d;

    int retVal = sscanf(s, "%hu.%hu.%hu.%hu", &a, &b, &c, &d);
    if (retVal != 4
        || a > 255 || b > 255 || c > 255 || d > 255)
    { return false; }

    ipAddress = (a << 24) + (b << 16) + (c << 8) + d;
    return true;
}

// /**
// FUNCTION :: IsMulticastIpAddress
// PURPOSE :: Checks whether the passed ipAddress is multicast or not.
// PARAMETERS ::
// + ipAddress : unsigned : IpAddress to be checked.
// RETURN :: bool : Returns TRUE if the passed ipAddress is multicast; FALSE
//                  otherwise.
// **/
bool IsMulticastIpAddress(DtInetAddr addr)
{
#ifdef MULTIPLE_MULTICAST_INTERFACE_SUPPORT
    unsigned ipAddress = addr.toULong();
#else
    unsigned ipAddress = addr;
#endif

    // Corresponds to >= 224.0.0.0 and < 240.0.0.0.

    return (ipAddress >= 0xe0000000 && ipAddress < 0xf0000000);
}

// /**
//FUNCTION :: UpdateMetric
//PURPOSE :: Updates values of stored metrics sent by the GUI.
//PARAMETERS ::
// + unsigned : NodeId : node id of metric
// + metric : const MetricData : definition of metric being updated
// + metricValue : void* : pointer to new value of the metric
// + updateTime : clocktype : when this value update occurred
//RETURN :: void : NULL
// **/
void VRLinkDIS::UpdateMetric(unsigned nodeId,
        const MetricData& metric,
        void* metricValue,
        clocktype updateTime)
{
}
