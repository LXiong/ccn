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

#if defined (WIN32) || defined (_WIN32) || defined (_WIN64)
#include <windows.h>
#elif defined(__linux__) || defined(__linux)
#include <dlfcn.h>
#else
#error The VRLink Interface is not supported on this platform
#endif

#include "vrlink.h"
#include "testfed_data.h"
#include "testfed_shared.h"
#include <string>

typedef VRLinkExternalInterface *dsoLoadFunc();

// /**
//FUNCTION :: LoadVRLink
//PURPOSE :: Loads the requires vrlink interface dso/dll for specific vrlink protocols
//PARAMETERS ::
// + ifaceType : VRLinkInterfaceType : Protocol typee.
// + simIface : VRLinkSimulatorInterface* : pointer to the shared memory simuator 
//                                          interface object that is created by p0
//RETURN :: VRLinkExternalInterface* : vrlink external interface pointer
// **/
static VRLinkExternalInterface* LoadVRLink(VRLinkInterfaceType ifaceType)
{
    VRLinkExternalInterface *extIface = 0;
    dsoLoadFunc *func = NULL;
    char filename[MAX_STRING_LENGTH];    
    std::string home;

#if defined (WIN32) || defined (_WIN32) || defined (_WIN64)
    
    if (ifaceType == VRLINK_TYPE_DIS)
    {
        sprintf(filename, "testfed_vrlink_interface-dis.dll");
    }
    else if (ifaceType == VRLINK_TYPE_HLA13)
    {
        sprintf(filename, "testfed_vrlink_interface-hla13.dll");
    }
    else if (ifaceType == VRLINK_TYPE_HLA1516)
    {
        sprintf(filename, "testfed_vrlink_interface-hla1516.dll");
    }
    else
    {
        ReportError("Unsupported protocol");
    }
    HMODULE lib = LoadLibrary(filename);

    if (lib == NULL)
    {
        char msg[MAX_STRING_LENGTH];
        sprintf(msg, "could not load vrlink interface library %s - error code: %d",
                filename, GetLastError());
        ReportError(msg);
    }

    func = (dsoLoadFunc *) GetProcAddress(lib, "makeVRLinkExternalInterface");
#elif defined(__linux__) || defined(__linux)

    if (ifaceType == VRLINK_TYPE_DIS)
    {
        sprintf(filename, "testfed_vrlink_interface-dis.so");
    }
    else if (ifaceType == VRLINK_TYPE_HLA13)
    {
        sprintf(filename, "testfed_vrlink_interface-hla13.so");
    }
    else if (ifaceType == VRLINK_TYPE_HLA1516)
    {
        sprintf(filename, "testfed_vrlink_interface-hla1516.so");
    }
    else
    {
        ReportError("Unsupported protocol");
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
        ReportError(msg);
    }

    func = (dsoLoadFunc *) dlsym(lib,"makeVRLinkExternalInterface");
#endif

    if (func == NULL)
    {
        ReportError("could not find load function in vrlink interface library");
    }
    extIface = (*func)();
    return extIface;
}

// /**
//FUNCTION :: VRLinkInit
//PURPOSE :: To verify that OS is 32bit; initialize external interface.
//PARAMETERS ::
// + iface : EXTERNAL_Interface* : External Interface pointer.
// + nodeInput : NodeInput* : Pointer to cached file.
//RETURN :: void : NULL
// **/
//THIS FUNCTION must be defined below LoadVRLink
VRLinkExternalInterface* VRLinkInit(bool debug,
    VRLinkInterfaceType type,
    char scenarioName[],
    char connectionVar1[],
    char connectionVar2[],
    char connectionVar3[],
    char connectionVar4[])
{
    VRLinkExternalInterface* vrlink = LoadVRLink(type);
    vrlink->Init(debug, 
        type,
        scenarioName,
        connectionVar1,
        connectionVar2,
        connectionVar3,
        connectionVar4);

    return vrlink;
}
