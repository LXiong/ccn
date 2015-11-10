// Copyright (c) 2001-2013, SCALABLE Network Technologies, Inc.  All Rights Reserved.
// All Rights Reserved.
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
//

#ifndef __ANE_ANESAT_MAC_H__
#define __ANE_ANESAT_MAC_H__

//
// Uncomment ANESAT_DEBUG to debug this files algorithms
//
// #define ANESAT_DEBUG
// #define ANESAT_TRACE

#include "qualnet_mutex.h"

#include "util_constants.h"
#include "util_atomic.h"
#include "util_nameservice.h"

#include "mac.h"
#include "ane_mac.h"

// #include "anesat_default_mac.h"
#include "anesat_tc.h"
#include "anesat_upstream.h"
#include "anesat_subnet.h"
#include "anesat_node.h"

class AneSatChannelModel;

class AneSatChannelMap : public UTIL::Lockable
{
    std::map<std::string, AneSatChannelModel*> m_map;
    
public:
    
    AneSatChannelMap()
    {
        
    }
    
    ~AneSatChannelMap()
    {
        
    }

    void putElement(std::string mdlname, 
                    AneSatChannelModel* mdl)
    {
        std::map<std::string,AneSatChannelModel*>::iterator pos =
        m_map.find(mdlname);
        
        if (pos == m_map.end())
        {
            m_map[mdlname] = mdl;
        }
        else
        {
            pos->second = mdl;
        }
    }
    
    AneSatChannelModel* getElement(std::string mdlname)
    {
        std::map<std::string,AneSatChannelModel*>::iterator pos =
        m_map.find(mdlname);
        
        ERROR_Assert(pos != m_map.end(),
                     "The simulation has identified a condition where"
                     " the requested channel name is not in the"
                     " simulation data structures.  This is a bug"
                     " and the simulation cannot continue.");
        
        return pos->second;
    }
    
} ;

class AneSatChannelModel 
: public MacAneChannelModel 
{
    void distributedMemoryInitialize()
    {
        static UTIL_AtomicInteger initLock = UTIL_AtomicValue(1);
        
        if (UTIL_AtomicDecrementAndTest(&initLock) == TRUE) 
        {
            
#if defined(ANESAT_DEBUG)
            
            printf("Performing first time ANESAT initialization..."); 
            fflush(stdout);
            
#endif /* ANESAT_DEBUG */
            
            std::string key = "/QualNet/AddOns/Ane/ChannelMap";
            
            AneSatChannelMap* map = new AneSatChannelMap();
            UTIL_NameServicePutImmutable(key, map);
            
#if defined(ANESAT_DEBUG)
            
            printf("[done].\n");
            
#endif /* ANESAT_DEBUG */
        }        
    }
    
    AneSatSubnetData* distributedJoinChannel(char* channelName,
                                             NodeInput const *nodeInput, 
                                             NodeAddress interfaceAddress)
    {
        char tmpstr[1024];

        sprintf(tmpstr, 
                "/QualNet/AddOns/Ane/Channel/%s", 
                channelName);
        
        std::string key(tmpstr);
        
#if defined(ANESAT_DEBUG)
        printf("Attempting to join channel..."); fflush(stdout);
#endif /* ANESAT_DEBUG */
        
        AneSatSubnetData *tsd = new AneSatSubnetData(mac, 
                                                     channelName);
        
        BOOL owner = UTIL_NameServicePutImmutable(key, 
                                                  tsd);
        
        if (owner == FALSE) 
        {
            delete tsd;
            
            tsd = (AneSatSubnetData*)
                UTIL_NameServiceGetImmutable(key, 
                                             STRONG);
#if defined(ANESAT_DEBUG)
            printf("joined as client...[done].\n");
#endif /* ANESAT_DEBUG */
        } 
        else 
        {
#if defined(ANESAT_DEBUG)
            printf("joined as owner...initializing..."); 
            fflush(stdout);
#endif /* ANESAT_DEBUG */
            
            tsd->initialize(nodeInput, 
                            interfaceAddress);
            
#if defined(ANESAT_DEBUG)
            printf("[done].\n");
#endif /* ANESAT_DEBUG */
        }
        
#if defined(ANESAT_DEBUG)
        printf("Creating node data structures..."); 
        fflush(stdout);
#endif /* ANESAT_DEBUG */

        
#if defined(ANESAT_DEBUG)
        printf("[done].\n");
#endif /* ANESAT_DEBUG */
        
        return tsd;
        
    }
public:
    AneSatChannelModel(MacAneState* mac) 
    : MacAneChannelModel(mac) 
    { 
    
    }
    
    ~AneSatChannelModel() 
    { 
    
    }

    void moduleAllocate(void) 
    {
        if (((mac->gestalt() & MAC_ANE_DISTRIBUTED_ACCESS) ==
             MAC_ANE_DISTRIBUTED_ACCESS)
            || (((mac->gestalt() & MAC_ANE_CENTRALIZED_ACCESS) ==
                MAC_ANE_CENTRALIZED_ACCESS) && mac->isHeadend))
        {
            distributedMemoryInitialize();
        }
    }
    
    void moduleFinalize() 
    { 
    
    }

    void moduleInitialize(NodeInput const *nodeInput, 
                          Address* interfaceAddress) 
    {
        char buf[1024];
        BOOL wasFound;

#if defined(ANESAT_DEBUG)
        
        printf("Initializing ANESAT[%d/%d] on channel %s\n",
            mac->myNode->nodeId, mac->myIfIdx, buf);
        
#endif /* ANESAT_DEBUG */
    
        IO_ReadString(mac->myNode,
                      mac->myNode->nodeId, 
                      mac->myIfIdx, //interfaceAddress,
                      nodeInput, 
                      "ANESAT-CHANNEL-ID", 
                      &wasFound, 
                      buf);
        
        ERROR_Assert(wasFound == TRUE,
                     "The ANE model has detected a configuration"
                     " error.  The parameter ANESAT-CHANNEL-ID"
                     " must be defined in the simulation"
                     " configuration file.  The simulation"
                     " cannot continue.  Please verify the"
                     " simulation configuration file and"
                     " ensure that the proper line is included"
                     " for this subnet.");
        
        AneSatNodeData* tnd(NULL);
        
        if ((mac->gestalt() & MAC_ANE_DISTRIBUTED_ACCESS) ==
            MAC_ANE_DISTRIBUTED_ACCESS
            || (((mac->gestalt() & MAC_ANE_CENTRALIZED_ACCESS) ==
                MAC_ANE_CENTRALIZED_ACCESS) && mac->isHeadend))
        {
            AneSatSubnetData* tsd =
                distributedJoinChannel(buf, 
                                       nodeInput,
                                       interfaceAddress->interfaceAddr.ipv4);
            
            tnd = new AneSatNodeData(mac, 
                                     tsd);
        }
        else
        {
            tnd = new AneSatNodeData(mac);
        }
        
        mac->data = (void*)tnd;
        tnd->initialize(nodeInput, 
                        interfaceAddress->interfaceAddr.ipv4);
    }

    void insertTransmission(Message *msg, 
                            MacAneHeader* hdr)
    { 
    
    }
    
    void removeTransmission(Message *msg, 
                            MacAneHeader* hdr) 
    { 
    
    }

    void getDelayByNode(NodeAddress dstNodeId, 
                        bool& dropped, 
                        double& delay) 
    { 
        AneSatNodeData *tnd = (AneSatNodeData*)mac->data;

        dropped = false;
        
        {
            QNThreadLock lock(tnd->getSubnetMutex());
            
            //tnd->acquire();
            delay = tnd->getLatency(mac->myNode->nodeId,
                                    dstNodeId);
            //tnd->release();
        }
        
        if (delay < 0.0) 
        {
            dropped = true;
        }
    
#if defined(ANESAT_DEBUG)
        
        printf("ANESAT[%d/%d]: Latency set to %0.3lf ms [%c]\n", 
            mac->myNode->nodeId, mac->myIfIdx, delay * 1000.0,
            (dropped == true) ? 'D' : ' ');
        
#endif /* ANESAT_DEBUG */
        
    }

    void stationProcess(MacAneHeader* hdr, 
                        BOOL& keepPacket) 
    {
        keepPacket = TRUE;
    }
    
    void resolve(char *) 
    { 
    
    }

    void getTransmissionTime(Message *msg, 
                             MacAneHeader* hdr, 
                             double& transmissionStartTime,
                             double& transmissionDuration) 
    {
        AneSatNodeData *tnd = (AneSatNodeData*)mac->data;
        
        int packetSizeBytes = hdr->actualSizeBytes
            + hdr->headerSizeBytes;

        {
            QNThreadLock lock(tnd->getSubnetMutex());
            
            //  tnd->acquire();
            tnd->getTransmissionTime(hdr->sourceNodeId,
                                     hdr->sourceNodeIfIdx,
                                     8*packetSizeBytes,
                                     transmissionStartTime,
                                     transmissionDuration);
            // tnd->release();
        }
        
#if defined(ANESAT_DEBUG) || defined(ANESAT_TRACE)
        printf("ANESAT[%d/%d] getTransmissionTime %d bytes (start=%0.3le, "
               "end=%0.3le)\n", 
               mac->myNode->nodeId,
               mac->myIfIdx, 
               packetSizeBytes,
               transmissionStartTime,
               transmissionDuration);
#endif /* ANESAT_DEBUG || ANESAT_TRACE */
        
    }
    
    void getNodeReadyTime(Message* msg,
                          MacAneHeader* hdr,
                          double& nodeReadyTime) 
    { 
        AneSatNodeData *tnd = (AneSatNodeData*)mac->data;

        tnd->getNodeReadyTime(msg, 
                              hdr, 
                              nodeReadyTime);
    }

    void managementRequest(ManagementRequest *req, 
                           ManagementResponse *resp) 
    {
        AneSatNodeData *tnd = (AneSatNodeData*)mac->data;

        tnd->managementRequest(req, resp);
    }
} ;

#endif /* __ANE_ANESAT_MAC_H__ */
