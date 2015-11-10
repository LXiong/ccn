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
#include "app_messenger.h"
#include "external_util.h"
#include "product_info.h"
#include "vrlink.h"
#include "vrlink_sim_iface_impl.h"

#if defined (WIN32) || defined (_WIN32) || defined (_WIN64)
#include <windows.h>
#elif defined(__linux__) || defined(__linux)
#include <dlfcn.h>
#else
#error The VRLink Interface is not supported on this platform
#endif

// This flag is only valid on partition 0. It's used to support
// calls from the GUI Interface (UpdateMetric).
bool global_isVrlinkInterfaceActive = false;
VRLinkExternalInterface* g_vrlinkExternalIface = NULL;

typedef VRLinkExternalInterface *dsoLoadFunc(VRLinkSimulatorInterface *simIface);

// /**
//FUNCTION :: LoadVRLink
//PURPOSE :: Loads the requires vrlink interface dso/dll for specific vrlink protocols
//PARAMETERS ::
// + ifaceType : VRLinkInterfaceType : Protocol typee.
// + simIface : VRLinkSimulatorInterface* : pointer to the shared memory simuator 
//                                          interface object that is created by p0
//RETURN :: VRLinkExternalInterface* : vrlink external interface pointer
// **/
static VRLinkExternalInterface *LoadVRLink(VRLinkInterfaceType ifaceType,
                                           VRLinkSimulatorInterface *simIface)
{
    VRLinkExternalInterface *extIface = 0;
    dsoLoadFunc *func = NULL;
    char filename[MAX_STRING_LENGTH];    
    std::string home;
    Product::GetProductHome(home);

#if defined (WIN32) || defined (_WIN32) || defined (_WIN64)
    
    if (ifaceType == VRLINK_TYPE_DIS)
    {
        sprintf(filename, "%s/bin/vrlink_interface-dis.dll", home.c_str());
    }
    else if (ifaceType == VRLINK_TYPE_HLA13)
    {
        sprintf(filename, "%s/bin/vrlink_interface-hla13.dll", home.c_str());
    }
    else if (ifaceType == VRLINK_TYPE_HLA1516)
    {
        sprintf(filename, "%s/bin/vrlink_interface-hla1516.dll", home.c_str());
    }
    else
    {
        ERROR_ReportError("Unsupported protocol");
    }
    HMODULE lib = LoadLibrary(filename);

    if (lib == NULL)
    {
        char msg[MAX_STRING_LENGTH];
        sprintf(msg, "could not load vrlink interface library %s - error code: %d",
                filename, GetLastError());
        ERROR_ReportError(msg);
    }

    func = (dsoLoadFunc *) GetProcAddress(lib, "makeVRLinkExternalInterface");
#elif defined(__linux__) || defined(__linux)

    if (ifaceType == VRLINK_TYPE_DIS)
    {
        sprintf(filename, "%s/bin/vrlink_interface-dis.so", home.c_str());
    }
    else if (ifaceType == VRLINK_TYPE_HLA13)
    {
        sprintf(filename, "%s/bin/vrlink_interface-hla13.so", home.c_str());
    }
    else if (ifaceType == VRLINK_TYPE_HLA1516)
    {
        sprintf(filename, "%s/bin/vrlink_interface-hla1516.so", home.c_str());
    }
    else
    {
        ERROR_ReportError("Unsupported protocol");
    }

    void *lib = dlopen(filename, RTLD_LAZY | RTLD_GLOBAL);
    if (lib == NULL)
    {
        char msg[MAX_STRING_LENGTH];
        char* detail = dlerror();
        if (detail != NULL)
        {
            sprintf(msg, "could not load vrlink interface library %s : %s", filename, detail);
        }
        else
        {
            sprintf(msg, "could not load vrlink interface library %s", filename);
        }
        ERROR_ReportError(msg);
    }

    func = (dsoLoadFunc *) dlsym(lib,"makeVRLinkExternalInterface");
#endif

    if (func == NULL)
    {
        ERROR_ReportError("could not find load function in vrlink interface library");
    }
    extIface = (*func)(simIface);
    return extIface;
}

// /**
//FUNCTION :: VRLinkGetIfaceDataFromNode
//PURPOSE :: Returns the parition's vrlink external interface object.
//PARAMETERS ::
// + node : Node* : Pointer to the node.
//RETURN :: VRLinkExternalInterface* : vrlink external interface pointer
// **/
static VRLinkExternalInterface* VRLinkGetIfaceDataFromNode(Node* node)
{
    EXTERNAL_Interface *iface = node->partitionData->interfaceTable[EXTERNAL_VRLINK];
    if (iface != NULL)
    {
        return (VRLinkExternalInterface*)iface->data;
    }
    else
    {
        return NULL;
    }
}

// /**
//FUNCTION :: VRLinkGetProtocol
//PURPOSE :: Returns the protocol type (HLA / DIS) enabled.
//PARAMETERS ::
// + nodeInput : NodeInput* : Pointer to cached file.
//RETURN :: VRLinkInterfaceType : Protocol type.
// **/
static VRLinkInterfaceType VRLinkGetProtocol(NodeInput* nodeInput)
{
    BOOL retVal;
    char buf[MAX_STRING_LENGTH];

    IO_ReadString(ANY_NODEID,
                  ANY_ADDRESS,
                  nodeInput,
                  "VRLINK-PROTOCOL",
                  &retVal,
                  buf);

    VRLinkInterfaceType type = VRLINK_TYPE_NONE;

    if (retVal && strcmp(buf, "DIS") == 0)
    {
        cout << "VRLINK Interface active for DIS support." << endl;        
        type = VRLINK_TYPE_DIS;
    }
    else if (retVal && strcmp(buf, "HLA13") == 0)
    {
        cout << "VRLINK Interface active for HLA 1.3 support." << endl;
        type = VRLINK_TYPE_HLA13;
    }
    else if (retVal && strcmp(buf, "HLA1516") == 0)
    {
        cout << "VRLINK Interface active for HLA 1516 support." << endl;
        type = VRLINK_TYPE_HLA1516;
    }
    else if (retVal)
    {
        ERROR_ReportError("Unsupported VRLink Protocol");
    }
    else
    {
        IO_ReadString(ANY_NODEID,
                      ANY_ADDRESS,
                      nodeInput,
                      "DIS",
                      &retVal,
                      buf);

        if (retVal && strcmp(buf, "YES") == 0)
        {
            cout << "VRLINK Interface active for legacy DIS support." << endl;
            cout << "**Please upgrade your scenario to use VR-Link scenario parameters.**\n";
            type = VRLINK_TYPE_DIS;
        }
        else
        {
            IO_ReadString(ANY_NODEID,
                          ANY_ADDRESS,
                          nodeInput,
                          "HLA",
                          &retVal,
                          buf);

            if (retVal && strcmp(buf, "YES") == 0)
            {
                cout << "VRLINK Interface active for legacy HLA support." << endl;
                cout << "**Please upgrade your scenario to use VR-Link scenario parameters.**\n";
                type = VRLINK_TYPE_HLA13;
            }
        }
    }

    return type;
}
// /**
//FUNCTION : VRLinkIsActive
//PURPOSE : To ascertain whether this scenario uses the VRLink Interface
//PARAMETERS :
//     nodeInput: pointer to contents of the config file
//RETURN :: bool
// **/
bool VRLinkIsActive(NodeInput* nodeInput)
{
    BOOL retVal;
    char buf[MAX_STRING_LENGTH];

    bool vrlinkActive = false;
    bool legacyDIS = false;
    bool legacyHLA = false;

    IO_ReadString(ANY_NODEID,
                  ANY_ADDRESS,
                  nodeInput,
                  "VRLINK",
                  &retVal,
                  buf);

    if (retVal && strcmp(buf, "YES") == 0)
    {
        vrlinkActive = true;
    }

    IO_ReadString(ANY_NODEID,
                  ANY_ADDRESS,
                  nodeInput,
                  "DIS",
                  &retVal,
                  buf);

    if (retVal && strcmp(buf, "YES") == 0)
    {
        legacyDIS = true;
    }

    IO_ReadString(ANY_NODEID,
                  ANY_ADDRESS,
                  nodeInput,
                  "HLA",
                  &retVal,
                  buf);

    if (retVal && strcmp(buf, "YES") == 0)
    {
        legacyHLA = true;
    }

    if (vrlinkActive && legacyDIS)
    {
        ERROR_ReportError("VRLINK and DIS parameters are both set. "
                          "Please use one or the other");
    }

    if (vrlinkActive && legacyHLA)
    {
        ERROR_ReportError("VRLINK and HLA parameters are both set. "
                          "Please use one or the other");
    }

    if (legacyDIS && legacyHLA)
    {
        ERROR_ReportError("The VRLink Interface does not support using DIS and "
                          "HLA at the same time.");
    }

    return vrlinkActive || legacyDIS || legacyHLA;
}

// /**
//FUNCTION :: VRLinkIsActive
//PURPOSE :: To ascertain whether VRLink is enabled or not.
//PARAMETERS :: None.
//RETURN :: bool
// **/
bool VRLinkIsActive(Node *node)
{
    EXTERNAL_Interface *iface = node->partitionData->interfaceTable[EXTERNAL_VRLINK];
    return iface != NULL;
}

// /**
//FUNCTION :: VRLinkIsActive
//PURPOSE :: To ascertain whether VRLink is enabled or not.
//PARAMETERS :: None.
//RETURN :: bool
// **/
bool VRLinkIsActive()
{
    return global_isVrlinkInterfaceActive;
}

// /**
//FUNCTION :: VRLinkInit
//PURPOSE :: To verify that OS is 32bit; initialize external interface.
//PARAMETERS ::
// + iface : EXTERNAL_Interface* : External Interface pointer.
// + nodeInput : NodeInput* : Pointer to cached file.
//RETURN :: void : NULL
// **/
void VRLinkInit(EXTERNAL_Interface* iface, NodeInput* nodeInput)
{
    if (PARTITION_GetTerrainPtr(iface->partition)->getCoordinateSystem() != 1)
    {
        ERROR_ReportError("Error: VRLink should only be used with LATLONALT Coordinate Systems");
    }

    // This function is called before nodes and protocols are initialized.
    // Protocol state-data is not available (memory not allocated).
    VRLinkInterfaceType ifaceType = VRLinkGetProtocol(nodeInput);

    ERROR_Assert(
        ifaceType,
        "Specified interface not currently supported");

    VRLinkSimulatorInterface* simIface =
                           VRLinkSimulatorInterface_Impl::GetSimulatorInterface();

    VRLinkExternalInterface* extIface = LoadVRLink(ifaceType, simIface);

    if (extIface == NULL)
    {
        ERROR_ReportError("Could not create vrlink interface");
    }

    iface->data = extIface;
    if (iface->partition->partitionId == 0)
    {
        global_isVrlinkInterfaceActive = true;
        g_vrlinkExternalIface = extIface;
    }

    extIface->Init(iface, nodeInput);    
}

// /**
//FUNCTION :: VRLinkInitNodes
//PURPOSE :: To initialize VRLink on an interface.
//PARAMETERS ::
// + iface : EXTERNAL_Interface* : External Interface pointer.
// + nodeInput : NodeInput* : Pointer to cached file.
//RETURN :: void : NULL
// **/
void VRLinkInitNodes(EXTERNAL_Interface* iface, NodeInput* nodeInput)
{
    VRLinkExternalInterface* ifaceData =
                                      (VRLinkExternalInterface*) iface->data;

    ifaceData->InitNodes(iface, nodeInput);
}

// /**
//FUNCTION :: VRLinkReceive
//PURPOSE :: To receive interactions from an outside federate.
//PARAMETERS ::
// + iface : EXTERNAL_Interface* : External Interface pointer.
//RETURN :: void : NULL
// **/
void VRLinkReceive(EXTERNAL_Interface* iface)
{
    //only p0 registers this function
    VRLinkExternalInterface* ifaceData =
                                      (VRLinkExternalInterface*) iface->data;
    ifaceData->Receive(iface);
}

// /**
// FUNCTION :: VRLinkProcessEvent
// PURPOSE :: Handles all the protocol events for VRLink.
// PARAMETERS ::
// + node : Node* : Pointer to node.
// + msg : Message* : Pointer to message.
// RETURN :: void : NULL
// **/
void VRLinkProcessEvent(Node* node, Message* msg)
{
    VRLinkExternalInterface* vrlinkExtIface =
                                            VRLinkGetIfaceDataFromNode(node);

    if (vrlinkExtIface == NULL)
    {
        cout << endl
             << "VRLink interface is not currently active." << endl;
        return;
    }

    vrlinkExtIface->ProcessEvent(node, msg);
}

// /**
// FUNCTION :: VRLinkFinalize
// PURPOSE :: VRLink's finalize function.
// PARAMETERS ::
// + iface : EXTERNAL_Interface* : External Interface pointer.
// RETURN :: void : NULL
// **/
void VRLinkFinalize(EXTERNAL_Interface* iface)
{
    if (iface->partition->partitionId == 0)
    {
        VRLinkExternalInterface* vrLinkExtIface =
                                      (VRLinkExternalInterface*) iface->data;

        if (vrLinkExtIface == NULL)
        {
            cout << endl
                 << "VRLink interface is not currently active." << endl;
            return;
        }

        vrLinkExtIface->Finalize(iface);
    }
}

// /**
// FUNCTION :: VRLinkUpdateMetric
// PURPOSE :: To send state updates to GUI.
// PARAMETERS ::
// + nodeId : unsigned : Id of the node.
// + metric : const MetricData& : Metric data.
// + metricValue : void* : Metric value.
// + updateTime : clocktype : Update time.
// RETURN :: void : NULL.
// **/
void VRLinkUpdateMetric(
    unsigned nodeId,
    const MetricData& metric,
    void* metricValue,
    clocktype updateTime)
{
    //We are using this global parameter instead because we can set it specfically for 
    //p0, which is the only partitions that handles GUI metric updates.
    ERROR_Assert(
        (g_vrlinkExternalIface != NULL),
        "global vrlink external interface has not been set, init must of failed");

    g_vrlinkExternalIface->UpdateMetric(nodeId, metric, metricValue, updateTime);
}

// /**
// FUNCTION :: VRLinkSendRtssNotificationIfNodeIsVRLinkEnabled
// PURPOSE :: To send Rtss notification if node is VRLink enabled.
// PARAMETERS ::
// + node : Node* : Node pointer.
// RETURN :: void : NULL
// **/
void VRLinkSendRtssNotificationIfNodeIsVRLinkEnabled(Node* node)
{    
    VRLinkSimulatorInterface* simIface =
                           VRLinkSimulatorInterface_Impl::GetSimulatorInterface();

    if (simIface->IsMilitaryLibraryEnabled())
    {
        VRLinkExternalInterface* vrlinkExtIface =
                                                VRLinkGetIfaceDataFromNode(node);

        if (vrlinkExtIface != NULL)
        {
            // Various radio macs will invoke this to indicate ready to send.
            // If this node is on a partition other than 0 (and thus
            // has no actually VRLink federation) a message will be created
            // to forward the rtss to p0.
            vrlinkExtIface->SendRtssNotificationIfNodeIsVRLinkEnabled(node);
        }
    }
}
