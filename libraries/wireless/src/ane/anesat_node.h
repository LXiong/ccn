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
//
#ifndef __ANE_ANESAT_NODE_H__
# define __ANE_ANESAT_NODE_H__

#include "mac.h"
#include "qualnet_mutex.h"

#include "ane_mac.h"
#include "anesat_subnet.h"
#include "anesat_tc.h"

#if defined(DEBUG_PR4785)
# undef DEBUG_PR4785
#endif

//
// Uncomment ANESAT_DEBUG_TIMING to get debugging information 
// on media access timing.
//

// #define ANESAT_DEBUG_TIMING

///
/// Default bandwidths for the traffic conditioners
///
#define ANESAT_DEFAULT_BANDWIDTH_LIMIT ((double)512.0e3)
#define ANESAT_DEFAULT_BANDWIDTH_MINIMUM ((double)64.0e3)

/// @Brief AneSat termination class
///
/// @Class AneSatNodeData
/// The AneSatNodeData structure contains basic linkage information to
/// the ANE data structures and pointers to the shared subnet
/// information.  It also contains a pointer to the local 
/// upstream traffic conditioner.
///
/// @see class AneSatSubnetData
///
class AneSatNodeData {
protected:
    /// Pointer to ANE MAC state in local interface
    MacAneState* mac; 
    
    /// Pointer to local subnet state structure
    /// Only used if gestalt permits
    AneSatSubnetData *subnet; 

    /// Upstream traffic conditioner
    AneSatTrafficConditioner *uptc; 

public:
    ///
    /// The basic operation of the object is to simply link the 
    /// instance into the 
    /// structure hierarchy upon allocation.  Upon deallocation the 
    /// class must 
    /// destroy its upstream traffic conditioner and release whatever 
    /// shared subnet
    /// resources it has acquired.
    ///
    /// @param pMac The local ANE (parent) data structure
    /// @param psub The local subnet data structure (shared)
    ///

    AneSatNodeData(MacAneState* pMac, 
                   AneSatSubnetData *psub)
    : mac(pMac), subnet(psub) 
    { 
    
    }
    
    AneSatNodeData(MacAneState* pMac)
        : mac(pMac), subnet(NULL)
    {
            
    }
    
    ~AneSatNodeData() 
        { 
            delete uptc; 
        
            if ((mac->gestalt() & MAC_ANE_DISTRIBUTED_ACCESS) == 
                MAC_ANE_DISTRIBUTED_ACCESS)
            {
                delete subnet; 
            }
        }

    ///
    /// The bulk of the initialization work is done here.  This is 
    /// where all parameters are read
    /// and validated for accuracy.  
    ///
    /// @param nodeInput The QualNet config file input data structure
    /// @param interfaceAddress The interface's own address
    ///
    /// The default traffic conditioner type is NONE.
    ///
    void initialize(NodeInput const *nodeInput, 
                    NodeAddress interfaceAddress) 
    { 
        BOOL wasFound = FALSE;
        double tmp;

        if (mac->isHeadend == false) 
        {
            double const DefaultBandwidthLimit = 
                ANESAT_DEFAULT_BANDWIDTH_LIMIT;
            
            double const DefaultBandwidthMinimum = 
                ANESAT_DEFAULT_BANDWIDTH_MINIMUM;

            ///
            /// The type of traffic conditioning is controlled by 
            /// the parameter
            /// <ul>
            /// <li> ANESAT-UPSTREAM-TRAFFIC-CONDITIONING-TYPE NONE | 
            ///      STRICT | RESIDUAL DEFAULT NONE \n
            ///      This controls the type of conditioner instantiated 
            ///      on each upstream.
            /// </ul>
            ///
            
            char conditionerType[1024];
            IO_ReadString(mac->myNode,
                          mac->myNode->nodeId, 
                          mac->myIfIdx,//interfaceAddress, 
                          nodeInput, 
                          "ANESAT-UPSTREAM-TRAFFIC-CONDITIONING-TYPE", 
                          &wasFound, 
                          conditionerType);
            
            if (wasFound == FALSE) 
            {
                strcpy(conditionerType, "NONE"); 
            }
            
            ///
            /// Presently three types of traffic conditioners are 
            /// supported:
            ///
            /// <ul>
            /// <li> NONE: No conditioning is performed at the ingress 
            ///            interface
            /// <li> RESIDUAL: A residual bandwidth limiting operation 
            ///                is performed 
            ///                @see class AneSatResidualTrafficConditioner
            /// <li> STRICT: Bandwidth is strictly limited to the 
            ///              value of the traffic limit 
            ///              @see class AneSatStrictTrafficConditioner
            /// <ul>
            ///
            if (strcmp(conditionerType, "NONE") == 0) 
            {
                // A NULL conditioner indicates no conditioning 
                // should be done.
                
                uptc = NULL;
                
            } else if (strcmp(conditionerType, "RESIDUAL") == 0) 
            {
                double bwLimit, minBw;
                
                ///
                /// Two config variables are read in the case of a 
                /// residual conditioner
                /// <ul>
                /// <li>ANESAT-UPSTREAM-BANDWIDTH-LIMIT (bits/sec) 
                ///     Nominal bandwidth limit of conditioner
                /// <li>ANESAT-UPSTREAM-BANDWIDTH-MINIMUM (bits/sec) 
                ///     Strict minimum limit of conditioner
                /// </ul>
                ///
                IO_ReadDouble(mac->myNode,
                              mac->myNode->nodeId, 
                              mac->myIfIdx,//interfaceAddress, 
                              nodeInput, 
                              "ANESAT-UPSTREAM-BANDWIDTH-LIMIT", 
                              &wasFound, 
                              &tmp);
                
                if (wasFound == TRUE)
                {
                    bwLimit = tmp;
                }
                else 
                { 
                    bwLimit = DefaultBandwidthLimit;
                }

                IO_ReadDouble(mac->myNode,
                              mac->myNode->nodeId, 
                              mac->myIfIdx,//interfaceAddress, 
                              nodeInput, 
                              "ANESAT-UPSTREAM-BANDWIDTH-MINIMUM", 
                              &wasFound, 
                              &tmp);
                
                if (wasFound == TRUE)
                {
                    minBw = tmp;
                }
                else
                {
                    minBw = DefaultBandwidthMinimum;
                }
        
                uptc = new AneSatResidualTrafficConditioner(bwLimit, 
                                                            minBw);

#if defined(ANESAT_DEBUG_TC)
                
                printf("[%d.%d] upstream tc= RESIDUAL"
                       " { lim=%0.3e, min=%0.3e }\n",
                       mac->myNode->nodeId, 
                       mac->myIfIdx, 
                       bwLimit, 
                       minBw);
                
#endif // ANESAT_DEBUG_TC
                
            } 
            else if (strcmp(conditionerType, "STRICT") == 0) 
            {
                double bwLimit;
                
                /// 
                /// Only a single config variable is used for the 
                /// strict conditioner
                /// <ul>
                /// <li> ANESAT-UPSTREAM-BANDWIDTH-LIMIT (bits/sec) 
                ///      Strict bandwidth limit of conditioner
                /// </ul>
                ///
                
                IO_ReadDouble(mac->myNode,
                              mac->myNode->nodeId, 
                              mac->myIfIdx,//interfaceAddress, 
                              nodeInput, 
                              "ANESAT-UPSTREAM-BANDWIDTH-LIMIT", 
                              &wasFound, 
                              &tmp);
                
                if (wasFound == TRUE)
                {
                    bwLimit = tmp;
                }
                else
                {
                    bwLimit = DefaultBandwidthLimit;
                }

                uptc = new AneSatStrictTrafficConditioner(bwLimit);
                
#if defined(ANESAT_DEBUG_TC)
                
                printf("[%d.%d] upstream tc= STRICT { lim=%0.3e }\n",
                       mac->myNode->nodeId, 
                       mac->myIfIdx, 
                       bwLimit);
                
#endif // ANESAT_DEBUG_TC
                
            } 
            else
            {
                ERROR_ReportError("Unknown traffic conditioner,"
                                  " must be NONE, RESIDUAL or STRICT");
            }
        } 
        else
        {
            
            /// The downstream (headend) does not need a traffic 
            /// conditioner (yet).  All traffic conditioning
            /// is performed by the IP layer.
            
            uptc = NULL;
        }
        
    }
        
    QNThreadMutex* getSubnetMutex()
    {
        return subnet->getMutex();
    }
        
    /// @brief get local latency for a given destination node
    ///
    /// @param dstNodeId the destination node index
    /// @returns The latency of the complete transmission in seconds
    ///
    
    double getLatency(NodeAddress srcNodeId,
                      NodeAddress dstNodeId) 
    { 
        // Ask the subnet how much time to allocate to this 
        // transmission to reach that particular destination node.
        
        bool isCentralized = 
            (mac->gestalt() & MAC_ANE_CENTRALIZED_ACCESS) ==
            MAC_ANE_CENTRALIZED_ACCESS;
        
        if (isCentralized)
        {
        
            ERROR_Assert(mac->isNetworkProcessor(),
                 "The ANE model has detected a logic error.  In"
                 " centralized access mode, the calculation"
                 " for node-to-node latency should only"
                 " occur on the network processing node.  The"
                 " simulation-runtime has detected this is"
                 " not the case.  In this case,"
                 " the calculation occurred on another node, or,"
                 " the"
                 " routine to detect the network processor has"
                 " a logic error.  Please report this to"
                 " SNT technical support. The simulation can"
                 " not continue.");
        }
        
        
        return subnet->getLatency(mac, 
                                  srcNodeId,
                                  dstNodeId); 
    }
    
    /// @brief get actual serialization time of packet on channel
    ///
    /// @param packetSizeInBits Packet size in bits
    /// @returns Serialization time in seconds
    ///
    /// The formulas used are listed below
    /// <ul>
    /// <li> \f$t_{ch} = \frac{l_{pkt}}{r_{ch}} \f$
    /// <li> \f$t_{wait} = \frac{l_{pkt}}{r_{eff}} \f$
    /// <li> \ft_{total} = t_{wait} + t_{ma} \f$ where 
    ///      \f$ t_{ma} \f$ is the media access delay
    /// </ul>
    ///    
    
    void getTransmissionTime(NodeAddress nodeId,
                             int ifidx,
                             int packetSizeInBits,
                             double& transmissionStartTime,
                             double& transmissionDuration) 
    {
        bool isCentralized = 
            (mac->gestalt() & MAC_ANE_CENTRALIZED_ACCESS) 
            == MAC_ANE_CENTRALIZED_ACCESS;
        
        bool isNetworkProcessor = mac->isHeadend;
        
        ERROR_Assert(!isCentralized 
                     || (isCentralized && isNetworkProcessor),
                     "The ANE model has detected a logic error.  In"
                     " centralized access mode, the calculation"
                     " for node-to-node media should only"
                     " occur on the network processing node.  The"
                     " simulation-runtime has detected this is"
                     " not the case.  In this case,"
                     " the calculation occurred on another node, or,"
                     " the"
                     " routine to detect the network processor has"
                     " a logic error.  Please report this to"
                     " SNT technical support. The simulation can"
                     " not continue.");
        
        double channelBandwidth = 
            subnet->getBandwidth(mac, 
                                 nodeId,
                                 ifidx);
        
        double mediaAccessLatency = 
            subnet->getMediaAccessLatency(mac,
                                          nodeId,
                                          ifidx);
        
        transmissionStartTime = mediaAccessLatency;
        transmissionDuration = 
            (double)packetSizeInBits / channelBandwidth;
        
        //
        // Report serialization delay back to subnet for 
        // its own processing
        //
        
        subnet->reportSerializationDelay(mac, 
                                         nodeId,
                                         ifidx,
                                         transmissionDuration); 
    }
        
    
    void getNodeReadyTime(Message* msg,
                          MacAneHeader* hdr,
                          double& nodeReadyTime)
    {
        int packetSizeBytes = hdr->actualSizeBytes
            + hdr->headerSizeBytes;

        int pktSizeInBits = 8 * packetSizeBytes;
        
        nodeReadyTime = 0;
                
        if (mac->isHeadend == true || uptc == NULL) 
        {
            nodeReadyTime = 0;        
        }
        else
        {
            double nodeBandwidth = 
                uptc->getBandwidthAvailable(mac->myNode);
            
            ERROR_Assert(nodeBandwidth > 0,
                         "The ANE model has determined that the"
                         " node bandwidth has fallen to zero.  This"
                         " implies an infinite transmission duration,"
                         " which is not allowed in QualNet.  Please"
                         " confirm your configuration file for this"
                         " simulation.  If it appears correct, contact"
                         " QualNet technical support for more"
                         " assistance.  The simulation cannot"
                         " continue.");
            
            nodeReadyTime = (double)pktSizeInBits / nodeBandwidth;
            
            uptc->reportBitsSent(pktSizeInBits);
        } 
    }
    
    /// @brief Handle MAC management request 
    ///
    /// @param req a pointer to a management request message 
    ///        @see struct ManagementRequest
    /// @param resp a pointer to a management response message 
    ///        @see struct ManagementResponse
    ///
    /// The following messages are supported
    /// <ul>
    /// <li> ManagementRequestEcho automatically echos success 
    ///      back to caller
    /// <li> ManagementRequestSetBandwidthLimit sets bandwidth 
    ///      if possible and support (only STRICT conditioning 
    ///      supports this presently)
    /// </ul>
    ///
    /// All other commands are not supported.
    ///
    
    void managementRequest(ManagementRequest *req, 
                           ManagementResponse *resp) 
    {
        switch(req->type) {
        case ManagementRequestUnspecified:
        case ManagementRequestSetGroupMembership:
            resp->result = ManagementResponseUnsupported;
            resp->data = 0;
        break;
        case ManagementRequestEcho:
            resp->result = ManagementResponseOK;
            resp->data = 0;
        break;
        case ManagementRequestSetBandwidthLimit:
            
#if defined(DEBUG_PR4785)
            printf("Processing bandiwidth limit request...\n");
#endif

            if (uptc != NULL) 
            {
                uptc->setBandwidthLimit(*(double*)req->data);
                resp->result = ManagementResponseOK;
                resp->data = 0;
            } else {
                resp->result = ManagementResponseUnsupported;
                resp->data = 0;
            }
        break;
        default:
            ERROR_ReportError("Unsupported Management Request Type.");
        }
    }
} ;

#endif                          /* __ANE_ANESAT_NODE_H__ */
