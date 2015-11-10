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

#ifndef VRLINK_MAIN_H
#define VRLINK_MAIN_H

#include <stdio.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <sys/types.h>
#include <sys/timeb.h>
#else /* _WIN32 */
#include <unistd.h>
#include <sys/time.h>
#endif /* _WIN32 */

#include <vl/exerciseConn.h>
#include <vl/fomMapper.h>
#include <vl/exConnInit.h>
#include <vl/reflectedEnt.h>
#include <vl/reflEntList.h>
#include <vl/topoView.h>
#include <vl/entityPub.h>
#include <vl/entitySR.h>
#include <vl/refRadXmList.h>
#include <vl/commentInter.h>
#include <vl/signalInter.h>
#include <vl/dataInter.h>
#include <vlpi/EntityTypes.h>
#include <vlpi/entityType.h>
#include <vlpi/articulatedPart.h>
#include <vlpi/attachedPart.h>
#include <vlutil/vlString.h>
#include <vlutil/vlProcessControl.h>
#include <vlutil/vlMachineTypes.h>
#include <vlutil/vlPrint.h>
#include <vlutil/vlStringUtil.h>
#include <matrix/vlMath.h>

#define __NETINET_IN_H__

#include "api.h"
#include "partition.h"
#include "app_messenger.h"
#include "external_util.h"
#include "vrlink_sim_iface.h"

#include "vrlink.h"

// iostream.h is needed to support DMSO RTI 1.3v6 (non-NG).
#ifdef NOT_RTI_NG
#include <iostream.h>
#else /* NOT_RTI_NG */
#include <iostream>
using namespace std;
#endif /* NOT_RTI_NG */

#if defined (WIN32) || defined (_WIN32) || defined (_WIN64)
#include <hash_map>
using namespace stdext;
#else
#include <ext/hash_map>

namespace __gnu_cxx
{
    template<> struct hash< std::string >
    {
        size_t operator()( const std::string& x ) const
        {
            return hash< const char* >()( x.c_str() );
        }
    };
}

using namespace __gnu_cxx;
#endif


//forward declaration
class VRLink;

// /**
// CLASS        ::  VRLinkExternalInterface
// DESCRIPTION  ::  Main class used to store VRLink interface information.
// **/
class VRLinkExternalInterface_Impl : public VRLinkExternalInterface
{
    private:
    VRLinkInterfaceType ifaceType;
    VRLink* vrlink;
    EXTERNAL_Interface* iface;

    public:

    ~VRLinkExternalInterface_Impl();
    
    VRLinkExternalInterface_Impl(VRLinkSimulatorInterface* simIface);

    VRLinkInterfaceType GetType();
    VRLink* GetVRLinkInterfaceData();
    VRLinkSimulatorInterface* GetVRLinkSimulatorInterface();
    void Init(EXTERNAL_Interface* iface, NodeInput* nodeInput);
    void InitNodes(EXTERNAL_Interface* iface, NodeInput* nodeInput);
    void Receive(EXTERNAL_Interface* iface);
    void ProcessEvent(Node* node, Message* msg);
    void Finalize(EXTERNAL_Interface* iface);

    void UpdateMetric(unsigned nodeId,
        const MetricData& metric,
        void* metricValue,
        clocktype updateTime);

    void SendRtssNotificationIfNodeIsVRLinkEnabled(Node* node);
};

#endif /* VRLINK_MAIN_H */
