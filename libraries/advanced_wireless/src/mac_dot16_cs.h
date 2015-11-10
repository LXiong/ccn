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

#ifndef MAC_DOT16_CS_H
#define MAC_DOT16_CS_H

//
// This is the header file of the implementation of Convergence SubLayer
// of IEEE 802.16 MAC
//

// /**
// CONSTANT    :: DOT16_CS_NUM_RECORD : 20
// DESCRIPTION :: Initial size of the array for keeping the list
//                of established classifier records
// **/
#define DOT16_CS_NUM_RECORD    20

// /**
// CONSTANT    :: DOT16_CS_NUM_RECORD_INC : 10
// DESCRIPTION :: The size for incrementally expanding the classifier
//                record array.
// **/
#define DOT16_CS_NUM_RECORD_INC    10

// /**
// CONSTANT    :: DOT16_CS_CLASSIFIER_REMOVAL_DELAY : 5S
// DESCRIPTION :: The delay that the CS sublayer will remove an invalidated
//                classifier record.
// **/
#define DOT16_CS_CLASSIFIER_REMOVAL_DELAY   (5 * SECOND)

// /**
// CONSTANT    :: DOT16e_CS_INIT_IDLE_DELAY : 30S
// DESCRIPTION :: The delay that the CS sublayer will start Idle mode
// **/
#define DOT16e_CS_INIT_IDLE_DELAY   (30 * SECOND)

//Define  TOS types of Service Classes
#define   DOT16_TOS_CLASS_TOS0                   0
#define   DOT16_TOS_CLASS_TOS1                   1
#define   DOT16_TOS_CLASS_TOS2                   2
#define   DOT16_TOS_CLASS_TOS3                   3
#define   DOT16_TOS_CLASS_TOS4                   4
#define   DOT16_TOS_CLASS_TOS5                   5
#define   DOT16_TOS_CLASS_TOS6                   6
#define   DOT16_TOS_CLASS_TOS7                   7

// /**
// ENUM        :: MacDot16DscpClassType
// DESCRIPTION :: DSCP class types
// **/
typedef enum
{
    DOT16_DS_CLASS_BE = 0,      // Best Effort

    // Class 1
    DOT16_DS_CLASS_AF10  = 8,
    DOT16_DS_CLASS_AF11  = 10,  // gold
    DOT16_DS_CLASS_AF12  = 12,  // silver
    DOT16_DS_CLASS_AF13  = 14,  // bronze

    // Class 2
    DOT16_DS_CLASS_AF20  = 16,
    DOT16_DS_CLASS_AF21  = 18,  // gold
    DOT16_DS_CLASS_AF22  = 20,  // silver
    DOT16_DS_CLASS_AF23  = 22,  // bronze

    // Class 3
    DOT16_DS_CLASS_AF30  = 24,
    DOT16_DS_CLASS_AF31  = 26,  // gold
    DOT16_DS_CLASS_AF32  = 28,  // silver
    DOT16_DS_CLASS_AF33  = 30,  // bronze

    // Class 4
    DOT16_DS_CLASS_AF40  = 32,
    DOT16_DS_CLASS_AF41  = 34,  // gold
    DOT16_DS_CLASS_AF42  = 36,  // silver
    DOT16_DS_CLASS_AF43  = 38,  // bronze

    // Express forwarding
    DOT16_DS_CLASS_EXF   = 40,

    // Expedited forwarding
    DOT16_DS_CLASS_EF    = 40,

    // Network control
    Dot16_DS_CLASS_CTRL1 = 48,
    Dot16_DS_CLASS_CTRL2 = 56
}MacDot16DscpClassType;

// /**
// ENUM        :: MacDot16CsClassifierType
// DESCRIPTION :: Type of a CS packet classifier
// **/
typedef enum
{
    DOT16_CS_CLASSIFIER_IPv4,    // IPv4 classifier
    DOT16_CS_CLASSIFIER_IPv6,    // IPv6 classifier
    DOT16_CS_CLASSIFIER_ATM      // ATM  classifier
} MacDot16CsClassifierType;


// /**
// STRUCT      :: MacDot16CsClassifierRecord
// DESCRIPTION :: Data structure contains the classifier and related info
// **/
typedef struct mac_dot16_cs_classifier_record_str
{
    unsigned char type;  // must be the 1st byte, indicate classifier type

    int csfId;  // to uniquely and easily identify a classifier record
    void* classifier; // pointer to the actual classifier structure

    // data service type of this flow
    MacDot16ServiceType serviceType;

    // Basic CID of the intended next hop, only useful for BS to identify SS
    Dot16CIDType basicCid;

    clocktype invalidTime;  // The time stamp when the record is marked
                            // as invalidated
    int refCount;            // keep track of app services using this
                             //classifier

} MacDot16CsClassifierRecord;

// /**
// STRUCT      :: MacDot16CsStats
// DESCRIPTION :: Data structure for storing dot16 CS statistics
// **/
typedef struct
{
    int numPktsFromUpper;

    int numPktsClassified;
    int numPktsIpv6Classified;

    int numPktsDropped;
    int numPktsHeld;

} MacDot16CsStats;

// /**
// CONSTANT    :: DOT16_FRAGMENT_HOLD_TIME_INTERVAL : 5 * SECOND
// DESCRIPTION :: The delay added to ip fragment hold time to calculate
//                the maximum duration for which a fragment classifier will
//                remain in ipFragmentClassifierMap
// **/
#define DOT16_FRAGMENT_HOLD_TIME_INTERVAL (5 * SECOND)

// /**
// CONSTANT    :: DOT16_FRAGMENT_CLASSIFIER_MAP_MAX_SIZE : 256
// DESCRIPTION :: The maximum number of fragment classifier map 
//                (ipFragmentClassifierMap)can have. 
// **/
#define DOT16_FRAGMENT_CLASSIFIER_MAP_MAX_SIZE 256

// /**
// STRUCT      :: MacDot16CsIPFragmentClassifierKey
// DESCRIPTION :: Data structure containing the key for fragment classifier 
//                element in Map(ipFragmentClassifierMap)
// **/

struct MacDot16CsIPFragmentClassifierKey
{
    UInt32 ip_id;   //IP identification
    unsigned char  ip_p;    // protocol
    Address srcAddr;    // Address of the source node (IPv4, IPv6 or ATM)
    Address dstAddr;    // Address of the dest node (IPv4, IPv6 or ATM)

};

// /**
// STRUCT      :: MacDot16CsIPFragmentClassifierValue
// DESCRIPTION :: Data structure containing the value for fragment classifier
//                element in Map(ipFragmentClassifierMap)
// **/
struct MacDot16CsIPFragmentClassifierValue
{
    unsigned short srcPort;  // Port number at source node
    unsigned short dstPort;  // Port number at dest node
    clocktype fragmentCalssifierHoldTime; // Lifetime of a fragment
                                          // classifier
};

// /**
// STRUCT      :: MacDot16CsIPFragmentClassifierCompare
// DESCRIPTION :: Structure to implement compare function for fragment
//                classifier map
// **/

struct MacDot16CsIPFragmentClassifierCompare
{
    bool operator()(const MacDot16CsIPFragmentClassifierKey& key1,
                    const MacDot16CsIPFragmentClassifierKey& key2 ) const
    {
        if (key1.ip_id < key2.ip_id)
        {
            return true;
        }
        else if (key1.ip_id == key2.ip_id)
        {
            if (key1.ip_p < key2.ip_p)
            {
                return true;
            }
            else if (key1.ip_p == key2.ip_p)
            {
                if (key1.srcAddr.networkType == NETWORK_IPV4)
                {
                    if (GetIPv4Address(key1.srcAddr) <
                        GetIPv4Address(key2.srcAddr))
                    {
                        return true;
                    }
                    else if (GetIPv4Address(key1.srcAddr) ==
                             GetIPv4Address(key2.srcAddr))
                    {
                        if (GetIPv4Address(key1.dstAddr) <
                            GetIPv4Address(key2.dstAddr))
                        {
                            return true;
                        }

                    }
                 }
                else if (key1.srcAddr.networkType == NETWORK_IPV6)
                {
                    if (CMP_ADDR6(key1.srcAddr.interfaceAddr.ipv6,
                                  key2.srcAddr.interfaceAddr.ipv6) == -1)
                    {
                        return true;
                    }
                    else if (CMP_ADDR6(key1.srcAddr.interfaceAddr.ipv6,
                                       key2.srcAddr.interfaceAddr.ipv6) == 0)
                    {
                        if (CMP_ADDR6(key1.dstAddr.interfaceAddr.ipv6,
                                      key2.dstAddr.interfaceAddr.ipv6) == -1)
                        {
                            return true;
                        }
                    }
                }
            }
        }
        return false;
    }
};

// /**
// Map Typedef       :: MacDot16CsIpFragmentClassifierMap
// DESCRIPTION :: Data-type to define Map for storing fragment classifiers
// **/
typedef std::map<MacDot16CsIPFragmentClassifierKey,
                 MacDot16CsIPFragmentClassifierValue,
                 MacDot16CsIPFragmentClassifierCompare>
             MacDot16CsIpFragmentClassifierMap;


// /**
// STRUCT      :: MacDot16CsData
// DESCRIPTION :: Data structure of Dot16 CS
// **/
typedef struct struct_mac_dot16_cs_str
{
    int csfId; // counter used to assign ID to classifiers

    // list of established classifier records
    // Separated list for different classifier types
    MacDot16CsClassifierRecord** classifierRecord;
    int numClassifierRecord;
    int allocClassifierRecord;
    // general statistics
    MacDot16CsStats stats;
    // Map to store the classifier information for fragmented packets
    MacDot16CsIpFragmentClassifierMap* ipFragmentClassifierMap;
} MacDot16Cs;


//--------------------------------------------------------------------------
//  API functions
//--------------------------------------------------------------------------

// /**
// FUNCTION   :: MacDot16CsInvalidClassifier
// LAYER      :: MAC
// PURPOSE    :: Invalid a classifier by the classifier ID
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 structure
// + csfId     : int           : ID of the classifier
// RETURN     :: void : NULL
// **/
void MacDot16CsInvalidClassifier(
        Node* node,
        MacDataDot16* dot16,
        int csfId);

// /**
// FUNCTION   :: MacDot16CsGetClassifierInfo
// LAYER      :: MAC
// PURPOSE    :: Fill in classifier info structure.
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + msg       : Message*      : Packet from upper layer
// + serviceType : MacDot16ServiceType : Data service type of this flow
// + pktInfo   : MacDot16CsClassifier* : classifer info of this flow
// RETURN     :: MacDot16CsClassifierRecord
// **/
MacDot16CsClassifierRecord* MacDot16CsGetClassifierInfo(
        Node* node,
        MacDataDot16* dot16,
        MacDot16CsClassifier* classifierInfo);

// /**
// FUNCTION   :: MacDot16CsNewClassifier
// LAYER      :: MAC
// PURPOSE    :: create new classifier info structure.
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + msg       : Message*      : Packet from upper layer
// + serviceType : MacDot16ServiceType : Data service type of this flow
// + pktInfo   : MacDot16CsClassifier* : classifer info of this flow
// RETURN     :: MacDot16CsClassifierRecord
// **/
MacDot16CsClassifierRecord* MacDot16CsNewClassifier(
        Node* node,
        MacDataDot16* dot16,
        MacDot16ServiceType serviceType,
        Message* msg,
        unsigned char* macAddress,
        MacDot16QoSParameter* qosInfo,
        MacDot16CsClassifier* classifierInfo);


// /**
// FUNCTION   :: MacDot16CsPacketFromUpper
// LAYER      :: MAC
// PURPOSE    :: Upper layer pass a data PDU to SAP of Dot16 CS
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 structure
// + msg       : Message*      : Packet from upper layer
// + nextHop   : NodeAddress   : Address of the next hop
// + networkType : int         : Type of network
// + priority  : TosType       : Priority of the packet
// + msgDropped  : BOOL*    : Parameter to determine whether msg was dropped
// RETURN     :: BOOL : TRUE, buffer of DOT16 is full, cannot hold more pkt
//                      FALSE, otherwise
// **/
BOOL MacDot16CsPacketFromUpper(
        Node* node,
        MacDataDot16* dot16,
        Message* msg,
        Mac802Address nextHop,
        int networkType,
        TosType priority,
        BOOL* msgDropped);

// /**
// FUNCTION   :: MacDot16CsPacketFromLower
// LAYER      :: MAC
// PURPOSE    :: Lower Sublayer(CPS) pass a data PDU to CS
// PARAMETERS ::
// + node      : Node*         : Pointer to node.
// + dot16     : MacDataDot16* : Pointer to DOT16 structure
// + msg       : Message*      : Packet from lower layer
// + lastHopMacAddr : unsigned char* : MAC address of previous hop
// RETURN     :: void : NULL
// **/
void MacDot16CsPacketFromLower(
        Node* node,
        MacDataDot16* dot16,
        Message* msg,
        unsigned char* lastHopMacAddr,
        MacDot16BsSsInfo* ssInfo = NULL);

// /**
// FUNCTION   :: MacDot16CsInit
// LAYER      :: MAC
// PURPOSE    :: Initialize dot16 CS at a given interface.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + nodeInput        : const NodeInput*  : Pointer to node input.
// RETURN     :: void : NULL
// **/
void MacDot16CsInit(
        Node* node,
        int interfaceIndex,
        const NodeInput* nodeInput);

// /**
// FUNCTION   :: MacDot16CsLayer
// LAYER      :: MAC
// PURPOSE    :: Handle timers and layer messages.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + msg              : Message*          : Message for node to interpret.
// RETURN     :: void : NULL
// **/
void MacDot16CsLayer(
        Node *node,
        int interfaceIndex,
        Message *msg);

// /**
// FUNCTION   :: MacDot16CsFinalize
// LAYER      :: MAC
// PURPOSE    :: Print stats and clear protocol variables.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// RETURN     :: void : NULL
// **/
void MacDot16CsFinalize(
        Node *node,
        int interfaceIndex);

#endif // MAC_DOT16_CS_H
