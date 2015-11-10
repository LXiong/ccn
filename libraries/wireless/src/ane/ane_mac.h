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
#ifndef __ANE_MAC_H__
# define __ANE_MAC_H__

#include "main.h"

#include "util_nameservice.h"

/// MAC_ANE_DEFAULT_HEADER_SIZE = 32 (bytes)
#define MAC_ANE_DEFAULT_HEADER_SIZE ((int)(32))

///
/// @file ane_mac.h
///
/// @brief Class definition file for ANE MAC model
///

///
/// @namespace MacAne
///
/// @brief The namespace for the ANE MAC model

namespace MacAne 
{
    /// 
    /// @enum TransmitterStatus
    ///
    /// @brief Enumerated values for a transmitter
    ///
    
    enum TransmitterStatus 
    { 
        /// Represents an active transmitter
        Active = 1, 
        
        /// Represents an inactive transmitter
        Idle = 0
    };
}

/// Enumerated value for the Station Depart state
static const int MacAneEvent_StationDepart = 1;

/// Enumerated value for the Channel Arrive state
static const int MacAneEvent_ChannelArrive = 2;

/// Enumerated value for the Channel Depart state
static const int MacAneEvent_ChannelDepart = 3;

/// Enumerated value for the Station Arrive state
static const int MacAneEvent_StationArrive = 4;

/// Enumerated value for the Station Process state
static const int MacAneEvent_StationProcess = 5;

/// Enumerated value for the Transmitter Idle state
static const int MacAneEvent_TransmitterIdle = 6;

/// Enumerated value for the Station Request state
static const int MacAneEvent_StationRequest = 8;

/// Enumerated value for the Request Indication state
static const int MacAneEvent_RequestIndication = 9;

/// Enumerated value for the Grant Indication state
static const int MacAneEvent_GrantIndication = 7;

/// Enumerated value for the Notify Interest state
static const int MacAneEvent_NotifyInterest = 10;

/// Enumerated value for the Publish Notifications stats
static const int MacAneEvent_PublishNotifications = 11;

/// Default Propagation Time for model
static double const MAC_ANE_DEFAULT_PROPAGATION_TIME = 
    100.0e-6;
    
///
/// Default amount of time to elapse when sending
/// among different QualNet partitions
///

static clocktype const MAC_ANE_DEFAULT_INTER_PARTITION_DELAY =
    100 * MICRO_SECOND;
    
///
/// Default model name
///
static const char* MAC_ANE_DEFAULT_MODEL_NAME = 
    "ane_default_mac";

/// Flag to indicate use centralized access paradigm
static const unsigned int MAC_ANE_CENTRALIZED_ACCESS = 0x1;

/// Flag to indicate use of distributed access paradigm
static const unsigned int MAC_ANE_DISTRIBUTED_ACCESS = 0x2;

/// Default access paradigm
static const unsigned int MAC_ANE_DEFAULT_GESTALT =
    MAC_ANE_DISTRIBUTED_ACCESS;

struct MacAneState;


///
/// @class MacAneEventHandler
/// 
/// @brief An event handler class
///

class MacAneEventHandler 
{
protected:
    
    /// Back pointer to interface state information
    MacAneState* m_mac;
    
public:
    ///
    /// @brief Constructor based on back-pointer information
    ///
    /// @param mac a pointer to the interface MAC structure
    ///
    
    MacAneEventHandler(MacAneState* mac)
    : m_mac(mac) 
    {
    
    }
    
    ///
    /// @brief Default(empty) constructor
    ///
    
    ~MacAneEventHandler() 
    {
        m_mac = NULL;
    }
    
    ///
    /// @brief Event dispatch method
    ///
    /// @param evCode an enumerated event code
    /// @param msg a pointer to a message structure
    ///
    
    virtual void onEvent(int evCode, 
                         Message* msg) = 0;
} ;

///
/// @class MacAneEventStationRequest
///
/// @brief The Station Request state
///

class MacAneEventStationRequest : public MacAneEventHandler
{
public:
    /// 
    /// @brief Constructor for state initializatin
    ///
    /// @param mac a pointer to the interface MAC information
    ///
    
    MacAneEventStationRequest(MacAneState* mac)
    : MacAneEventHandler(mac) 
    { 
    
    }
    
    ///
    /// @brief Event dispatch method
    ///
    /// @param evCode an enumerated event code
    /// @param msg a pointer to a message structure
    ///
    
    void onEvent(int evcode, Message *msg);
} ;

///
/// @class MacAneEventRequestIndication
///
/// @brief The Request Indication state
///

class MacAneEventRequestIndication : public MacAneEventHandler
{
public:
    
    /// 
    /// @brief Constructor for state initializatin
    ///
    /// @param mac a pointer to the interface MAC information
    ///
    
    MacAneEventRequestIndication(MacAneState* mac)
    : MacAneEventHandler(mac) 
    { 
        
    }
    
    ///
    /// @brief Event dispatch method
    ///
    /// @param evCode an enumerated event code
    /// @param msg a pointer to a message structure
    ///
    
    void onEvent(int evcode, Message *msg);
} ;

///
/// @class MacAneEventGrantIndication
///
/// @brief The Grant Indication state
///

class MacAneEventGrantIndication : public MacAneEventHandler
{
public:
    
    /// 
    /// @brief Constructor for state initializatin
    ///
    /// @param mac a pointer to the interface MAC information
    ///
    
    MacAneEventGrantIndication(MacAneState* mac)
    : MacAneEventHandler(mac) 
    { 
    
    }
    
    ///
    /// @brief Event dispatch method
    ///
    /// @param evCode an enumerated event code
    /// @param msg a pointer to a message structure
    ///
    
    void onEvent(int evcode, 
                 Message *msg);
} ;

///
/// @class MacAneEventStationDepart
///
/// @brief The Station Depart state
///

class MacAneEventStationDepart : public MacAneEventHandler
{
public:
    
    /// 
    /// @brief Constructor for state initializatin
    ///
    /// @param mac a pointer to the interface MAC information
    ///
    
    MacAneEventStationDepart(MacAneState* mac)
    : MacAneEventHandler(mac) 
    { 
    
    }

    ///
    /// @brief Event dispatch method
    ///
    /// @param evCode an enumerated event code
    /// @param msg a pointer to a message structure
    ///
    
    void onEvent(int evCode, 
                 Message* msg);
} ;


///
/// @class MacAneEventChannelArrive
///
/// @brief The Channel Arrive state
///

class MacAneEventChannelArrive : public MacAneEventHandler
{
public:
    
    /// 
    /// @brief Constructor for state initializatin
    ///
    /// @param mac a pointer to the interface MAC information
    ///
    
    MacAneEventChannelArrive(MacAneState* mac)
    : MacAneEventHandler(mac) 
    { 
    
    }

    
    ///
    /// @brief Event dispatch method
    ///
    /// @param evCode an enumerated event code
    /// @param msg a pointer to a message structure
    ///
    
    void onEvent(int evCode, Message* msg);
} ;


///
/// @class MacAneEventTransmitterIdle
///
/// @brief The Transmitter Idle state
///

class MacAneEventTransmitterIdle : public MacAneEventHandler
{
public:
    
    /// 
    /// @brief Constructor for state initializatin
    ///
    /// @param mac a pointer to the interface MAC information
    ///
    
    MacAneEventTransmitterIdle(MacAneState* mac)
    : MacAneEventHandler(mac) 
    { 
    
    }
    
    ///
    /// @brief Event dispatch method
    ///
    /// @param evCode an enumerated event code
    /// @param msg a pointer to a message structure
    ///
    
    void onEvent(int evCode, Message* msg);
} ;

///
/// @class MacAneEventChannelDepart
///
/// @brief The Channel Depart state
///

class MacAneEventChannelDepart : public MacAneEventHandler
{
public:
    
    /// 
    /// @brief Constructor for state initializatin
    ///
    /// @param mac a pointer to the interface MAC information
    ///
    
    MacAneEventChannelDepart(MacAneState* mac)
    : MacAneEventHandler(mac) 
    { 
    
    }

    ///
    /// @brief Event dispatch method
    ///
    /// @param evCode an enumerated event code
    /// @param msg a pointer to a message structure
    ///
    
    void onEvent(int evCode, Message* msg);
} ;

///
/// @class MacAneEventStationArrive
///
/// @brief The Station Arrive state
///

class MacAneEventStationArrive : public MacAneEventHandler
{
public:
    
    /// 
    /// @brief Constructor for state initializatin
    ///
    /// @param mac a pointer to the interface MAC information
    ///
    
    MacAneEventStationArrive(MacAneState* mac)
    : MacAneEventHandler(mac) 
    { 
    
    }

    ///
    /// @brief Event dispatch method
    ///
    /// @param evCode an enumerated event code
    /// @param msg a pointer to a message structure
    ///
    
    void onEvent(int evcode, Message* msg);
} ;


///
/// @class MacAneEventStationProcess
///
/// @brief The Station Process state
///

class MacAneEventStationProcess : public MacAneEventHandler
{
public:
    
    /// 
    /// @brief Constructor for state initializatin
    ///
    /// @param mac a pointer to the interface MAC information
    ///
    
    MacAneEventStationProcess(MacAneState* mac)
    : MacAneEventHandler(mac) 
    { 
    
    }

    ///
    /// @brief Event dispatch method
    ///
    /// @param evCode an enumerated event code
    /// @param msg a pointer to a message structure
    ///
    
    void onEvent(int evcode, Message* msg);
} ;


///
/// @struct MacAneBandwidthRequest
///
/// @brief Bandwidth request data structure
///

struct MacAneBandwidthRequest
{
    /// ID of node requesting bandwidth
    NodeAddress nodeId;
    
    /// Logical interface of requesting process
    int ifidx;
    
    ///
    /// @enum MacAnePriority
    ///
    /// @brief Request priority definition
    ///
    
    enum MacAnePriority 
    {
        /// Normal delivery
        Nominal = 1,
        
        /// Expedited delivery
        Expedited = 2,
        
        /// Bulk, low priority delivery
        Bulk = 3
    } prio; ///< Priority field of request
    
    /// size of packet in bits
    int pktSizeInBits;
    
    /// The local (TX) time transmission started
    double transmissionStartTime;
    
    /// The length of transmission
    double transmissionDuration;
    
    /// A flag to indicate the request has been processed
    bool processed;
    
    /// A flag to indicate that the request has been granted
    bool granted;
    
    ///
    /// @brief Constructor based on directly provided data
    ///
    /// @param p_nodeId The calling nodeId
    /// @param p_ifidx The calling interface index
    /// @param p_prio The priority of the request reported by user
    /// @param p_bits The size of the request
    ///
    
    MacAneBandwidthRequest(NodeId p_nodeId = 0, 
                           int p_ifidx = -1,
                           MacAnePriority p_prio = Nominal,
                           int p_bits = 0)
    : nodeId(p_nodeId), ifidx(p_ifidx), prio(p_prio),
      pktSizeInBits(p_bits), transmissionStartTime(0),
      transmissionDuration(0), processed(false), granted(false)
    { 
    
    }
    
    ///
    /// @brief Indicate that this request should be accepted
    ///
    /// @param p_transmissionStartTime local time that transmission
    /// can start
    /// @param p_transmissionDuration time duration over which the
    /// channel is allocated to the transmitting node
    ///
    
    void acceptRequest(double p_transmissionStartTime,
                       double p_transmissionDuration) 
    { 
        processed = true;
        granted = true; 
        
        transmissionStartTime = p_transmissionStartTime;
        transmissionDuration = p_transmissionDuration;
    }
    
    ///
    /// @brief Indicate that this request should be rejected
    ///
    
    void rejectRequest() 
    { 
        processed = true;
        granted = false; 
        
        transmissionStartTime = 0;
        transmissionDuration = 0;
    }
    
    ///
    /// @brief Indicates whether the local request has been accepted
    ///
    /// @returns True if granted, false otherwise
    ///
    
    bool isGranted() 
    { 
        return processed && granted; 
    }
} ;

/// 
/// @struct MacAneChannelTerminus
///
/// @brief A descriptor of an endpoint for packet
/// transmission
///

struct MacAneChannelTerminus
{
    /// The NodeAddress of the terminating station
    NodeAddress nodeId;
    
    /// The interface index of the terminating process
    int nodeIfIdx;
    
    ///
    /// A flag to indicate whether this is the first copy of the
    /// original message
    /// TRUE : Is first copy
    /// FALSE : Is not first copy
    ///
    
    BOOL firstCopy;
} ;

///
/// @struct MacAneHeader
///
/// @brief The MAC header for the ANE protocol
///

struct MacAneHeader 
{
    /// The source address of the MAC frame
    Address srcAddr;
    
    /// The destination address of the MAC frame
    Address dstAddr;
    
    /// The actual size of the transmission (SDU) in bytes
    int actualSizeBytes;
    
    /// The header size in bytes (PCI)
    int headerSizeBytes;
    
    /// The source node ID of the sender
    NodeId sourceNodeId;
    
    /// The interface index of the sender
    int sourceNodeIfIdx;
} ;

class MacAneChannelModel;

///
/// @struct MacAneBase
///
/// @brief Basic MAC functionality definition
///

struct MacAneBase 
{
    /// Pointer to the local node data structure
    Node *myNode;
    
    /// Logical index of local node interface index
    int myIfIdx;
    
    /// Textual representation of the module name
    std::string myModuleName;
    
    /// State of the process
    int status;
    
    ///
    /// A flag to indicate whether the MAC should forward frames
    /// in a promiscuous manner
    ///
    
    BOOL isPromiscuous;
    
    ///
    /// The information about local nodes in subnet (on same
    /// processor
    ///
    
    SubnetMemberData *nodeList;
    
    /// The size of the node list
    int nodeListSize;
    
    /// A map of remote state information only used in distributed mode
    std::map<int, MacAneState*> remoteMacData;
    
    /// A map of event code to handlers
    std::map<int, MacAneEventHandler*> eventMap;

    /// A pointer to a channel model for this MAC
    MacAneChannelModel *channelModel;
    
    ///
    /// @brief Constructor for the base MAC
    ///
    /// @param p_node a pointer to a node data structure
    /// @param p_ifidx the interface index of the current MAC process
    ///
    
    MacAneBase(Node* p_node,
               int p_ifidx)
    : myNode(p_node), 
      myIfIdx(p_ifidx),
      myModuleName("AbstractNetworkEquation"),
      status(-1),
      isPromiscuous(FALSE),
      nodeList(NULL),
      nodeListSize(-1),
      channelModel(NULL)
    {
    
    }
} ;

///
/// @struct MacAneSerializableBase
///
/// @brief A class that is a serialized transmission system
///

struct MacAneSerializableBase 
{
    /// The current transmitter status
    MacAne::TransmitterStatus transmitterStatus;
    
    ///
    /// @brief Default (empty) constructor for class
    ///
    
    MacAneSerializableBase() : transmitterStatus(MacAne::Idle)
    {
    
    }
} ;

///
/// @struct MacAneStatistics
/// 
/// @brief Statistics class the ANE MAC
///

struct MacAneStatistics
{
    /// Number of packets sent by MAC
    int pktsSent;
    
    /// Number of packet received by MAC
    int pktsRecd;
    
    /// Number of packet forwarded by MAC
    int pktsFwd;
    
    MacAneStatistics()
    : pktsSent(0),
      pktsRecd(0),
      pktsFwd(0)
    {
    
    }
} ;

///
/// @struct MacAneHostAndInterfaceTidbit
///
/// @brief A sortable projection of nodeID and Interface
///

struct MacAneHostAndInterfaceTidbit
{
    /// NodeID part of identifier
    NodeAddress nodeId;
    
    /// interface index part of indentifier
    int ifidx;
    
    ///
    /// @brief Constructor for tidbit
    ///
    /// @param p_nodeId node identifier part
    /// @param p_ifidx interface identifier part
    ///
    
    MacAneHostAndInterfaceTidbit(NodeAddress p_nodeId = 0, 
                                 int p_ifidx = -1)
    : nodeId(p_nodeId), ifidx(p_ifidx)
    {
            
    }
    
    ///
    /// @brief Sorting operator
    ///
    /// @brief A sorting operator for the Tidbit
    ///
    /// @param x a tidbit to be compared against
    ///
    /// @returns true if current tidbit is "smaller" and false
    /// otherwise
    ///
    
    bool operator <(const MacAneHostAndInterfaceTidbit x) const
    {
        if (nodeId < x.nodeId) 
        {
            return true;
        }
        
        if (nodeId > x.nodeId)
        {
            return false;
        }
        
        if (ifidx < x.ifidx)
        {
            return true;
        }
        
        return false;
    }
} ;
        
///
/// @struct MacAneRouteTidbit
///
/// @brief An enhanced sosrting ID using the address as well
///

struct MacAneRouteTidbit : public MacAneHostAndInterfaceTidbit
{
    /// The address part of the tidbit
    Address addr;
    
    ///
    /// @brief Constructor for tidbit
    ///
    /// @param p_nodeId node identifier part
    /// @param p_ifidx interface identifier part
    /// @param p_addr a pointer to the address part
    ///
    
    MacAneRouteTidbit(int p_nodeId = 0, 
                      int p_ifidx = -1,
                      Address* p_addr = NULL)
    : MacAneHostAndInterfaceTidbit(p_nodeId, p_ifidx)
    {
        if (p_addr == NULL) 
        {
            addr.networkType = NETWORK_IPV4;
            addr.interfaceAddr.ipv4 = ANY_DEST;
        }
        else
        {
            memcpy((void*)&addr,
                   (void*)p_addr,
                   sizeof(Address));
        }
        
    }
    
    ///
    /// @brief Logic to sort an address
    ///
    /// @param x an address to compare the current context to
    ///
    /// @returns true if current address is less than x, false
    /// otherwise
    ///
    
    bool addrLessThan(Address& x)
    {
        switch(addr.networkType)
        {
            case NETWORK_IPV4:
                return addr.interfaceAddr.ipv4 < x.interfaceAddr.ipv4;
                
            break;
                
            case NETWORK_IPV6:
            {
                int k;
                
                for (k = 0; k < 4; k++)
                {
                    if (addr.interfaceAddr.ipv6.s6_addr32[k] <
                        x.interfaceAddr.ipv6.s6_addr32[k])
                    {
                        return true;
                    }
                }
                return false;
            }
                
            break;
                
            case NETWORK_ATM:
            case NETWORK_INVALID:
            default:
                ERROR_ReportError("The ANE model has detected"
                                  " an error in the programming"
                                  " logic on this model.  The model"
                                  " expected to only utilized IPv4"
                                  " and IPv6 addresses.  In this case,"
                                  " another type of address was passed"
                                  " to it.  If the intention of this"
                                  " simulation run was to utilize a"
                                  " non-IP network, this option is"
                                  " not available in the current"
                                  " generation model.  If this was"
                                  " not the intention, please review"
                                  " the configuration file for"
                                  " possible errors.  The simulation"
                                  " cannot continue.");
                
        }
        
        /* NOTREACHED */
        return false;
    }
    
    ///
    /// @brief Sorting operator
    ///
    /// @brief A sorting operator for the Tidbit
    ///
    /// @param x a tidbit to be compared against
    ///
    /// @returns true if current tidbit is "smaller" and false
    /// otherwise
    ///
    
    bool operator <(const MacAneRouteTidbit x) const
    {

        if (nodeId < x.nodeId) 
        {
            return true;
        }
        
        if (nodeId > x.nodeId)
        {
            return false;
        }
        
        if (ifidx < x.ifidx)
        {
            return true;
        }
        
        if (ifidx > x.ifidx)
        {
            return false;
        }
        
        ERROR_Assert(addr.networkType == x.addr.networkType,
                     "A comparison between network types is not"
                     " possible because they do not match, the"
                     " simulation has a programmatic error and"
                     " cannot continue.");
        
        switch(addr.networkType)
        {
            case NETWORK_IPV4:
                return GetIPv4Address(addr) < GetIPv4Address(x.addr);
                break;
                
            case NETWORK_IPV6:
            {
                const in6_addr* a6 = &addr.interfaceAddr.ipv6;
                const in6_addr* xa6 = &x.addr.interfaceAddr.ipv6;
                
                unsigned int i;
                for (i = 0; i < sizeof(*a6); i++)
                {
                    if (a6->s6_addr[i] < xa6->s6_addr[i])
                    {
                        return true;
                    }
                }
                return false;
            }
                
            default:
                ERROR_ReportError("A comparison between these two"
                                  " addresses is not possible because"
                                  " the network addressing type is"
                                  " not supported by the model.  The"
                                  " simulation cannot continue.");
                
                return false;
        }
    }
} ;

///
/// @struct MacAnePacketRouter
///
/// @brief A structure to identify whether a destination is interested
/// in a certain address
///

struct MacAnePacketRouter 
{
    /// A lookup table that exists on a per-interface basis
    std::map<MacAneRouteTidbit,bool> lookupTable;
    
    ///
    /// @brief Default(empty) constructor
    ///
    MacAnePacketRouter()
    {
        
    }
    
    /// 
    /// @brief Decide whether or not this object is interested in 
    /// a given address
    ///
    /// @param nodeId a node ID to be considered
    /// @param ifidx an interface ID to be considered
    /// @param addr a pointer to an address to be considered
    ///
    /// @returns True if the system has registered interest, false
    /// otherwise
    ///
    
    bool isInterested(int nodeId, 
                      int ifidx,
                      Address* addr)
    {
        bool interested(false);
        
        std::map<MacAneRouteTidbit,bool>::iterator pos =
            lookupTable.find(MacAneRouteTidbit(nodeId, ifidx, addr));
        
#ifdef DEBUG
        printf("Interest in (%d,%d,0x%x)=",
               nodeId,
               ifidx,
               addr->interfaceAddr.ipv4);
#endif /* DEBUG */
        
        if (pos != lookupTable.end()) 
        {
            interested = pos->second;
        }
        /*
         else
         {
             ERROR_ReportError("ANE has found a potential logic"
                               " error with the packet router logic."
                               "  For the simulation, it is assumed"
                               " that the node in question is not"
                               " interested in the current frame.  "
                               " Be sure to check the simulation"
                               " results to verify accuracy.  Also"
                               " contact QualNet support to ensure"
                               " that there is not a logic bug"
                               " in the ANE model.");
         }
         */
#ifdef DEBUG        
        if (interested)
        {
            printf("true\n");
        }
        else
        {
            printf("false\n");
        }
#endif /* DEBUG */
        
        return interested;
    }
    
    /// 
    /// @brief Notify interest in a given <nodeId, ifidx, addr> 
    /// tidbit
    ///
    /// @param nodeId A node ID of interest
    /// @param ifidx A interface index of interest
    /// @param addr a pointer to an address of interest
    /// @param interested a flag whether or not the current process
    /// is interested in this tidbit
    ///
    
    void notifyInterest(int nodeId,
                        int ifidx,
                        Address* addr,
                        bool interested)
    {
        std::map<MacAneRouteTidbit,bool>::iterator pos =
            lookupTable.find(MacAneRouteTidbit(nodeId, ifidx, addr));
  
#ifdef DEBUG
        
        printf("(%d,%d) reports interest=",
               nodeId,
               ifidx);
        
        if (interested) 
        {
            printf("true");
        }
        else
        {   
            printf("false");
        }
        
        printf(" in 0x%x\n",
               addr->interfaceAddr.ipv4);
        
#endif /* DEBUG */
        
        if (pos == lookupTable.end()) 
        {
            lookupTable[MacAneRouteTidbit(nodeId, ifidx, addr)] =
                interested;
        }
        else 
        {
            pos->second = interested;
        }
    }
} ;

#include "gestalt.h"

///
/// @struct MacAneState
///
/// @brief The state machine for an ANE abstract MAC
///

struct MacAneState 
    : public MacAneBase, 
      public MacAneSerializableBase,
      public MacAneStatistics,
      public MacAnePacketRouter
{    
    /// Subnet type of state machine
    enum { ClientServer=1, PeerToPeer } subnetType;
    
    /// Centralized switching node ID
    NodeId csNodeId;
    
    /// Centralized switching interface index
    int csIfidx;
    
    NodeId npNodeId;
    int npNodeIfIdx;
    
    bool isNetworkProcessor()
    {
        bool sameNode = myNode->nodeId == npNodeId;
        bool sameIfIdx = myIfIdx == npNodeIfIdx;
        
        return sameNode && sameIfIdx;
    }
    
    bool isHeadend;

    void *data;  // used for channel model storage
        
    MacAneState(Node* p_node,
                int p_ifidx,
                std::string p_protocolName)
    : MacAneBase(p_node, p_ifidx), 
      MacAneSerializableBase(),
      MacAneStatistics(),
      MacAnePacketRouter(),
      subnetType(PeerToPeer),
      csNodeId(0),
      csIfidx(-1),
      npNodeId(0),
      npNodeIfIdx(-1),
      isHeadend(false),
      data(NULL)
    {
    
    }
    
    unsigned int gestalt()
    {
        unsigned int flags = MAC_ANE_DEFAULT_GESTALT;

        std::string preferSharedMemory = UTIL::Gestalt::get_s("GESTALT-PREFER-SHARED-MEMORY");

        if (preferSharedMemory == "auto" 
            || preferSharedMemory == "Auto"
            || preferSharedMemory == "AUTO") 
        {
            PartitionData* partitionData = myNode->partitionData;
            int numPartitions = partitionData->getNumPartitions();

            if (numPartitions > 1)
            {
                flags &= ~MAC_ANE_DISTRIBUTED_ACCESS;
                flags |=  MAC_ANE_CENTRALIZED_ACCESS;
            }
       } 
       else if (UTIL::Gestalt::get_b("GESTALT-PREFER-SHARED-MEMORY"))
       {
           flags &= ~MAC_ANE_CENTRALIZED_ACCESS;
           flags |=  MAC_ANE_DISTRIBUTED_ACCESS;
       }
       else 
       {
           flags &= ~MAC_ANE_DISTRIBUTED_ACCESS;
           flags |=  MAC_ANE_CENTRALIZED_ACCESS;
       }
        
        return flags;
    }
    
    bool nodeIsInterestedInPacket(int nodeId, 
                                  int ifidx, 
                                  Address* addr)
    {
        bool isInterested;
        
        /*
        printf("Assessing interest for 0x%x on (%d,%d)\n", 
               addr->interfaceAddr.ipv4,
               nodeId,
               ifidx);
         */
        
        if ((gestalt() & MAC_ANE_CENTRALIZED_ACCESS) 
            == MAC_ANE_CENTRALIZED_ACCESS)
        {
            isInterested =
                MacAnePacketRouter::isInterested(nodeId,
                                                 ifidx,
                                                 addr);
        }
        else
        {
            std::map<int, MacAneState*>::iterator pos =
                remoteMacData.find(nodeId);
            
            MacAneState* remoteMac;
            
            if (pos == remoteMacData.end())
            {
                char keybuf[BUFSIZ];
                
                sprintf(keybuf, 
                        "/QualNet/Addons/Satellite/Ane/StateData/AneMac"
                        "/Node[%d]/Interface[%d]",
                        nodeId, 
                        ifidx);
                
                remoteMac = (MacAneState*)
                    UTIL_NameServiceGetImmutable(keybuf, 
                                                 STRONG);
                
                ERROR_Assert(remoteMac != NULL,
                             "The ANE MAC relies on the ability"
                             " to access remote data via a name service"
                             " mechanism.  The remote MAC has not"
                             " registered itself with this mapping"
                             " service.  Please review the code for"
                             " programmatic errors.");
                
                remoteMacData[nodeId] = remoteMac; 
            }
            else
            {
                remoteMac = pos->second;
            }
            
            extern bool MacAneIsInterestedInFrameQ(MacAneState*, 
                                                   NodeAddress);
            
            isInterested = 
                MacAneIsInterestedInFrameQ(remoteMac,
                                           addr->interfaceAddr.ipv4);
        }
        
        return isInterested;
    }
                
} ;

class MacAneDynamicModule {
public:
    virtual void moduleAllocate() = 0;
    
    virtual void moduleInitialize(const NodeInput* nodeInput,
                                  Address* interfaceAddress) = 0;
                                  
    virtual void moduleFinalize(void) = 0;
} ;

class MacAneDataPath {
public:
    virtual void insertTransmission(Message* msg, 
                                    MacAneHeader* hdr) = 0;
                                    
    virtual void removeTransmission(Message* msg, 
                                    MacAneHeader* hdr) = 0;
                                    
    virtual void getDelayByNode(NodeAddress nodeId, 
                                bool& dropped, 
                                double& delay) = 0;
                                
    virtual void stationProcess(MacAneHeader* hdr, 
                                BOOL& received) = 0;
                                
    virtual void getTransmissionTime(Message *msg, 
                                     MacAneHeader* hdr,
                                     double& transmissionStartTime,
                                     double& transmissionDuration) = 0; 
    
    virtual void getNodeReadyTime(Message* msg,
                                  MacAneHeader* hdr,
                                  double& nodeReadyTime) = 0;
                                        
    virtual void managementRequest(ManagementRequest* req, 
                                   ManagementResponse* resp) = 0;
} ;


class MacAneChannelModel 
    : public MacAneDynamicModule, public MacAneDataPath 
{
protected:
    MacAneState* mac;
    
    double bandwidth;
    double dropRatio;
    
public: 
    double getBandwidth() 
    {
        return bandwidth;
    }
    void setBandwidth(double p_bandwidth)
    {
        bandwidth = p_bandwidth;
    }
    
    double getDropRatio()
    {
        return dropRatio;
    }
    void setDropRatio(double p_dropRatio)
    {
        dropRatio = p_dropRatio;
    }

    
    static double getBandwidth(MacAneState* mac) {
        return mac->channelModel->getBandwidth();
    }
    
    static double getDropRatio(MacAneState* mac) {
        return mac->channelModel->getDropRatio();
    }
    
    MacAneChannelModel(MacAneState* pMac) : mac(pMac) 
    { 
    
    }
    ~MacAneChannelModel() 
    {
    
    }
    
    int getHeaderSize()
    {
        return MAC_ANE_DEFAULT_HEADER_SIZE;
    }
} ;

class MacAneDefaultChannelModel 
    : public MacAneChannelModel 
{
public:
    MacAneDefaultChannelModel(MacAneState* p_mac) 
    : MacAneChannelModel(p_mac) 
    { 
    
    }
    ~MacAneDefaultChannelModel() 
    {
    
    }
    
    void moduleAllocate();
    
    void moduleInitialize(const NodeInput* nodeInput,
                          Address* interfaceAddress);
                          
    void moduleFinalize();
    
    void resolve(char *filename) 
    { 
    
    }
    
    void insertTransmission(Message* msg, 
                            MacAneHeader* hdr);
                            
    void removeTransmission(Message* msg, 
                            MacAneHeader* hdr);
                            
    void getDelayByNode(NodeAddress nodeId, 
                        bool& dropped, 
                        double& delay);
                        
    void stationProcess(MacAneHeader* hdr, 
                        BOOL& keepPacket);
                        
    void getTransmissionTime(Message* msg, 
                             MacAneHeader* hdr,
                             double& transmissionStartTime,
                             double& transmissionDuration);
    
    void getNodeReadyTime(Message* msg,
                          MacAneHeader* hdr,
                          double& nodeReadyTime)
    {
        nodeReadyTime = 0;  // default behavior is immediately
    }

                             
    void managementRequest(ManagementRequest* req, 
                           ManagementResponse* resp)
    {
        resp->result = ManagementResponseUnsupported;
        resp->data = 0;
    }
} ;

struct MacAneInterestNotification
{
    NodeId reportingNodeId;
    int reportingNodeIfIdx;
    Address requestedAddress;
    bool interestLevel;
} ;

class MacAneEventNotifyInterest : public MacAneEventHandler
{
public:
    MacAneEventNotifyInterest(MacAneState* mac)
    : MacAneEventHandler(mac)
    {
        
    }
    
    void onEvent(int evcode, Message* msg) 
    {
        MacAneInterestNotification notify;
        
        memcpy((void*)&notify,
               MESSAGE_ReturnPacket(msg),
               sizeof(MacAneInterestNotification));
        
/*
        printf("Interest Notification Received: %d/%d\n",
               m_mac->myNode->nodeId, 
               m_mac->myIfIdx);
*/
        
        m_mac->notifyInterest(notify.reportingNodeId,
                              notify.reportingNodeIfIdx,
                              &notify.requestedAddress,
                              notify.interestLevel);
        
        MESSAGE_Free(m_mac->myNode,
                     msg);
    }
} ;

class MacAneEventPublishNotifications : public MacAneEventHandler
{
    void sendNotification(Address* addr);
public:
    MacAneEventPublishNotifications(MacAneState* mac)
    : MacAneEventHandler(mac)
    {
        
    }
    
    void onEvent(int evcode, Message* msg)
    {
        Address addr;
        
        /*
        printf("Entity %d/%d sending notifications\n",
               m_mac->myNode->nodeId,
               m_mac->myIfIdx);
        */
        
        addr.networkType = NETWORK_IPV4;
        addr.interfaceAddr.ipv4
            = MAC_VariableHWAddressToFourByteMacAddress(
                         m_mac->myNode,
                         &m_mac->myNode->macData[m_mac->myIfIdx]->macHWAddr);
        
        //printf("-->0x%x\n", addr.interfaceAddr.ipv4);
        sendNotification(&addr);
        
        addr.interfaceAddr.ipv4 = ANY_DEST;
        //printf("-->0x%x\n", addr.interfaceAddr.ipv4);

        sendNotification(&addr);
        
        int k;
        for (k = 0; k < m_mac->myNode->numberInterfaces; k++)
        {
            
            addr.interfaceAddr.ipv4 
                = NetworkIpGetInterfaceBroadcastAddress(m_mac->myNode,
                                                        k);
            
            //printf("-->0x%x\n", addr.interfaceAddr.ipv4);

            sendNotification(&addr);
        }
        
        MESSAGE_Free(m_mac->myNode,
                     msg);
    }
} ;

//
// Header declarations
//
//

void MacAneLayer(Node*, int, Message*);

void MacAneSetTcBandwidth(Node*, int, double);

void MacAneManagementRequest(Node*, 
                             int, 
                             ManagementRequest*,
                             ManagementResponse*);

void MacAneInitialize(Node*,
                      int,
                      const NodeInput*,
                      NodeAddress,
                      SubnetMemberData*,
                      int,
                      int);
                      
void MacAneFinalize(Node*, int);

void MacAneNetworkLayerHasPacketToSend(Node*,
                                       MacAneState*);
                                       
void MacAneRunTimeStat(Node*, MacAneState*);
                      

#endif                          /* __ANE_MAC_H__ */
