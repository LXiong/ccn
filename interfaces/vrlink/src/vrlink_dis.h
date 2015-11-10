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

#ifndef VRLINK_DIS_H
#define VRLINK_DIS_H

class DtInetDevice;

const char newLine = '\n';
const char singleQuotation = '\'';
const char doubleQuotation = '"';
const char stringSeperator = ';';
const char nullTerminator = 0;

/*NOTES - 9/20/12 - I had to move the MULTIPLE_MULTICAST_INTERFACE_SUPPORT  
    define into the header file, other wise the VRLinkDIS size is different
    in header and cpp. This lead to members in the base class having their 
    memory overwritten in VRLinkDIS::InitVariables. Ex: reflectedEntities' 
    end pointer(hash_map in VRLink) was being corrupted. When inserting data
    into reflectedEntities, an illegal write/crash occured.*/
#include "vlutil/vlVersion.h"

#if VR_LINK_MAJOR_VERSION > 3
#define MULTIPLE_MULTICAST_INTERFACE_SUPPORT
#elif VR_LINK_MAJOR_VERSION == 3 && VR_LINK_MINOR_VERSION > 12
#define MULTIPLE_MULTICAST_INTERFACE_SUPPORT
#endif

// /**
// CLASS        ::  VRLinkDIS
// DESCRIPTION  ::  Class to store DIS information.
// **/
class VRLinkDIS : public VRLink
{
    bool ipAddressIsMulticast;
    DtInetAddr ipAddress;
    DtInetAddr deviceAddress;
    DtInetAddr subnetMask;
    bool isSubnetMaskSet;

    unsigned char exerciseIdentifier;
    int port;

    short ttl;
    
#ifdef MULTIPLE_MULTICAST_INTERFACE_SUPPORT
/*  //Mulitgroup feature currently DISABLED
    typedef std::pair<DtInetAddr, DtInetDevice *> DisMcastGroupIntfcBinding;
    typedef std::vector<DisMcastGroupIntfcBinding> DisMcastGroupBindings;
    DisMcastGroupBindings myMcastGroups; */
#endif

    clocktype receiveDelay;
    clocktype maxReceiveDuration;

    public:
    VRLinkDIS(EXTERNAL_Interface* iface, NodeInput* nodeInput, VRLinkSimulatorInterface *simIface);
    void InitVariables(EXTERNAL_Interface* ifacePtr, NodeInput* nodeInput);
    void RegisterProtocolSpecificCallbacks();

    void InitConfigurableParameters(NodeInput* nodeInput); 

    void ReadLegacyParameters(NodeInput* nodeInput);

    RadioIdKey GetRadioIdKey(DtSignalInteraction* inter);
    RadioIdKey GetRadioIdKey(DtRadioTransmitterRepository* radioTsr);

    void CreateFederation();
    void ProcessEvent(Node* node, Message* msg);
    void ProcessSignalIxn(DtSignalInteraction* inter);

    void UpdateMetric(unsigned nodeId,
        const MetricData& metric,
        void* metricValue,
        clocktype updateTime);

    void AppProcessSendRtssEvent(Node* node);

#ifdef MULTIPLE_MULTICAST_INTERFACE_SUPPORT
    //Mulitgroup feature currently DISABLED
    //void InitMulticastParameters(NodeInput* nodeInput);
#endif
};

// /**
// FUNCTION :: ConvertStringToIpAddress
// PURPOSE :: Converts the string passed to ipAddress.
// PARAMETERS ::
// + s : const char* : String to be converted.
// + ipAddress : unsigned& : Resulting ipAddress (i.e. after conversion).
// RETURN :: bool : Returns TRUE if passed string has been succesfully
//                  converted to ipAddress; FALSE otherwise.
// **/
bool ConvertStringToIpAddress(const char* s, unsigned& ipAddress);

// /**
// FUNCTION :: IsMulticastIpAddress
// PURPOSE :: Checks whether the passed ipAddress is multicast or not.
// PARAMETERS ::
// + ipAddress : unsigned : IpAddress to be checked.
// RETURN :: bool : Returns TRUE if the passed ipAddress is multicast; FALSE
//                  otherwise.
// **/
bool IsMulticastIpAddress(DtInetAddr addr);

// /**
// FUNCTION :: IsLocalBroadcastIpAddress
// PURPOSE :: Checks whether the passed ipAddress is broadcast or not.
// PARAMETERS ::
// + ipAddress : unsigned : IpAddress to be checked.
// RETURN :: bool : Returns TRUE if the passed ipAddress is broadcast; FALSE
//                  otherwise.
// **/
bool IsLocalBroadcastIpAddress(unsigned ipAddress);

#endif /* VRLINK_DIS_H */
