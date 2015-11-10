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


// /**
// PROTOCOL :: SATELLITE_BENTPIPE
// SUMMARY :: An implementation of a simple best effort MAC protocol for
//            satellite networks.
// LAYER :: Media Access Control
// STATISTICS ::
//+              UNICAST packets sent to the channel : Unicast packets
//               sent to channel.
//+              BROADCAST packets sent to the channel : Broadcast packets
//               send to the channel.
//+              UNICAST packets received from channel : The number of
//               unicast packets successfully received from the channel.
//+              BROADCAST packets received from channel : The number of
//               broadcast packets successfully received from the channel.
// APP_PARAM ::
// CONFIG_PARAM ::
//+              MAC-SATELLITE-BENTPIPE-ROLE : String : Role of satellite
//               subnet node.  Either GROUND-STATION or SATELLITE
//+              MAC-SATELLITE-BENTPIPE-FORWARD-TO-PAYLOAD-PROCESSOR :
//               String : Whether the satellite node should forward the
//               received packets to the IP layer or not.  Either TRUE
//               or FALSE.
//+              MAC-SATELLITE-BENTPIPE-TRANSMIT-POWER-MW : Double :
//               The amount of transmission power the interface has
//               to send to the antenna.  This is specified linearly and
//               in mW.  For example a 3W transmitter would be specified as
//               MAC-SATELLITE-BENTPIPE-TRANSMIT-POWER-MW 3000
// SPECIAL_CONFIG ::
// VALIDATION ::
// IMPLEMENTED_FEATURES ::
// +             Reed-Solomon Viterbi Coding : Implements a general GF256
//               RSV codign system with code shortening.
// +             Processor payload capability : Implements the ability to
//               either redirect a packet to the downlink channel and
//               save key analog parameters or process the packet directly
//               on the local node.
// OMITTED_FEATURES ::
// +             Advanced DAMA.  The existing method has a low effiency and
//               requires more algorithms to schedule the transmissions on
//               the satellite network effectively.
// ASSUMPTIONS ::
// STANDARDS :: DVB-S
// RELATED ::
// **/

#ifndef MAC_SATELLITE_BENTPIPE_H
#define MAC_SATELLITE_BENTPIPE_H

#include "phy_satellite_rsv.h"

#define INVALID_CHANNEL_ID -1
// /**
// ENUM :: MacSatelliteBentpipePriority
// DESCRIPTION :: A list of priorities presently supported by the
// satellite MAC protocol
// **/

enum MacSatelliteBentpipePriority {
    MacSatelliteBentpipePriorityBestEffort = 1
};

// /**
// CONSTANT :: MacSatelliteBentpipeDefaultPriority
// DESCRIPTION :: The default priority of any traffic that is
// not specifically tagged.
// **/

static MacSatelliteBentpipePriority
    const MacSatelliteBentpipeDefaultPriority
    = MacSatelliteBentpipePriorityBestEffort;

// /**
// STRUCT :: MacSatelliteBentpipeData
// DESCRIPTION :: The data structure definition for media independent MAC
// functions.
// **/

struct MacSatelliteBentpipeData {
    Address srcAddr;
    Address dstAddr;
    MacSatelliteBentpipePriority prio;
};

// /**
// ENUM :: MacSatelliteBentpipePduType
// DESCRIPTION :: A list of enumerated values that define the media
// independent PDU types.  Currently the only type supported is the
// MacSatelliteBentpipeData.
// **/
enum MacSatelliteBentpipePduType {
    MacSatelliteBentpipeDataPdu = 1,
};

// /**
// STRUCT :: MacSatelliteBentpipeHeader
// DESCRIPTION :: The satellite MAC header for frame transmitted via
// this type of subnetowork.
// **/
struct MacSatelliteBentpipeHeader{
    MacSatelliteBentpipePduType type;
    unsigned int size;
    //
    // A few notes about the header format.
    //
    // 1. Doubles do not appear to pack identically on supported platforms.
    //    This causes verification runs to not match.  If this occurs, the
    //    size of this structure should be checked.
    // 2. Because of how QualNet receives packets, the uplink SNR must be
    //    stored if it is desired to calculate the overall effect of the
    //    bentpipe -- i.e. that the uplink and downlink noise is additive.
    //
    float uplinkSnr;
    BOOL uplinkSnrValid;
    BOOL isDownlinkPacket;
    union {
        MacSatelliteBentpipeData data;
    } pdu;
};

// Static MAC header size in bytes
static const int MacSatelliteBentpipeHeaderSize = 32;

// /**
// STRUCT :: MacSatelliteBentpipeStatistics
// DESCRIPTION :: A data structure containing the list of all
// statistics generated by each satellite MAC protocol state machine.
// **/
struct MacSatelliteBentpipeStatistics {
    UInt32 unicastPktsSent;
    UInt32 broadcastPktsSent;
    UInt32 unicastPktsRecd;
    UInt32 broadcastPktsRecd;
};

// /**
// CONSTANT :: MacSatelliteBentpipeC
// DESCRIPTION :: A local satellite constant equal to the speed of
// propagation for light waves in a vacuum.
// **/
static double const MacSatelliteBentpipeC = 299.8e6; // metres/sec

// /**
// ENUM :: MacSatelliteBentpipeRole
// DESCRIPTION :: The role fulfilled by a specific MAC process.  Either
// it is a ground station or a satellite.
// **/
enum MacSatelliteBentpipeRole {
    MacSatelliteBentpipeRoleGroundStation = 1,
    MacSatelliteBentpipeRoleSatellite
};

// /**
// STRUCT : MacSatelliteBentpipeState
// DESCRIPTION :: A data structure that holds the inter-process state
// of the satellite MAC state machine.  This is held by the node data
// structure between invocations of the satellite MAC state machine.
// **/

static const int MacSatelliteBentpipeChannelQueueSize = 50000;

struct MacSatelliteBentpipeChannel {
    int channelIndex;
    PhyStatusType phyStatus;
    MacSatelliteBentpipeStatistics stats;

    Queue* msgQueue;
};

struct MacSatelliteBentpipeUpDownChannelInfo
{
    MacSatelliteBentpipeChannel* upChannelInfo;
    MacSatelliteBentpipeChannel* downChannelInfo;
};

struct MacSatelliteBentpipeState {
    MacData *macData;
    MacSatelliteBentpipeRole role;
    Address localAddr;
    double dataRate;
    double transmitPowermW;
    BOOL forwardToPayloadProcessor;

    LinkedList* upDownChannelInfoList;
};

// /**
// CONSTANT :: MacSatelliteBentpipeDefaultTransmitPowermW
// DESCRIPTION :: The default transmit power of the satellite ground
// station if unspecified in the simulation configuration file.
// **/

static double const MacSatelliteBentpipeDefaultTransmitPowermW = 3000.0;

// /**
// CONSTANT :: MacSatelliteBentpipeDefaultForwardToPayloadProcessor
// DESCRIPTION :: The default behavior for a satellite to forward to the
// processor payload or not if unspecified in the simulation configuration
// file.
// **/

static BOOL const MacSatelliteBentpipeDefaultForwardToPayloadProcessor
    = FALSE;

// /**
// CONSTANT :: MacSatelliteBentpipeRoleGroundStation
// DESCRIPTION :: The default role of the satellite MAC protocol
// instantiation if not specified in the simulation configuration file.
// **/

static MacSatelliteBentpipeRole const MacSatelliteBentpipeDefaultRole
    = MacSatelliteBentpipeRoleGroundStation;

// /**
// FUNCTION :: MacSatelliteBentpipeInit
// LAYER :: Media Access Control
// PURPOSE :: This routine is called by the MAC layer dispatch code to
//            initialize a <node,interface> pair for use with the satellite
//            MAC protocol.  This routine is used by other routines outside
//            of this file.
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        interfaceIndex : int : The integer index of the local
//          interface.
// +        nodeInput : NodeInput* : A pointer to the data structure that
//          contains the QualNet configuration file (and auxillary files)
//          information deserialized into memory.
// +        nodesInSubnet : int : The count of <node,interface> pairs that
//          are in the local subnet.
// RETURN :: void : This routine does not return any value.
// **/

void MacSatelliteBentpipeInit(Node *node,
                              int interfaceIndex,
                              NodeAddress interfaceAddress,
                              NodeInput const *nodeInput,
                              int nodesInSubnet);

// /**
// FUNCTION :: MacSatelliteBentpipeProcessEvent
// LAYER :: Media Access Control
// PURPOSE :: This routine is called by the MAC layer dispatch code to
//            process an event in the control path of the protocol.
//            Presently this protocol does not create or accept any such
//            events. This routine is invoked by other modules outside of
//            this file.
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        interfaceIndex : int : The integer index of the local
//          interface starting at 0.
// +        msg : Message* : A pointer to the message data structure
//          associated with the event.
// RETURN :: void : This routine does not return any value.
// **/

void MacSatelliteBentpipeProcessEvent(Node *node,
                                      int interfaceIndex,
                                      Message *msg);

// /**
// FUNCTION :: MacSatelliteBentpipeNetworkLayerHasPacketToSend
// LAYER :: Media Access Control
// PURPOSE :: This routine is called by the MAC layer dispatch code to
//            indicate that the local network layer output queue has
//            a packet ready to be transmitted on the local subnet.  This
//            routine is called by other routines outside of this file.
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        state : MacSatelliteBentpipeState* : A pointer to the memory
//          region containing satellite MAC protocol-specific data.
// RETURN :: void : This routine does not return any value.
// **/

void
MacSatelliteBentpipeNetworkLayerHasPacketToSend(Node *node,
                                            MacSatelliteBentpipeState *state);

// /**
// FUNCTION :: MacSatelliteBentpipeReceivePacketFromPhy
// LAYER :: Media Access Control
// PURPOSE :: This routine is called by the MAC layer dispatch code to
//            process the reception of a packet in its entirety by
//            the physical layer.  This routine is invoked by other modules
//            outside of this file.
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        state : MacSatelliteBentpipeState* : A pointer to the
//          local protocol-specific data structure for the satellite
//          MAC.
// +        msg : Message* : A pointer to the message data structure
//          representing the packet received by the PHY.
// RETURN :: void : This routine does not return any value.
// **/

void MacSatelliteBentpipeReceivePacketFromPhy(Node *node,
                                          MacSatelliteBentpipeState *state,
                                          Message *msg);

// /**
// FUNCTION :: MacSatelliteBentpipePhyStatusChangeNotification
// LAYER :: Media Access Control
// PURPOSE :: This routine is called by the MAC layer dispatch code to
//            inform the local MAC process that the PHY layer has reported
//            a change in its state.  This routine is invoked by other modules
//            outside of this file.  The MAC process can take action as a
//            result of the status change but cannot veto the action
//            directly.
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        state : MacSatelliteBentpipeState* : A pointer to the
//          local protocol-specific data structure for the satellite
//          MAC.
// +        oldPhyStatus : PhyStatusType : An enumerated value holding the
//          status of the PHY process _before_ the change in state occured.
// +        newPhyStatus : PhyStatusType : An enumerated value holding the
//          status of the PHY process _after_ the change in state occured.
// RETURN :: void : This routine does not return any value.
// **/

void
MacSatelliteBentpipeReceivePhyStatusChangeNotification(Node *node,
                                           MacSatelliteBentpipeState *state,
                                           PhyStatusType oldPhyStatus,
                                           PhyStatusType newPhyStatus);

// /**
// FUNCTION :: MacSatelliteBentpipeRunTimeStat
// LAYER :: Media Access Control
// PURPOSE :: This routine is called by the MAC layer dispatch code to
//            report the current statistics of the layer via the
//            IO_ utility library.  It is usually only called at the
//            end of the simulation but may be called at any point during
//            the simulation itself.  This routine is called by other
//            functions outside of this file.
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        interfaceIndex : int : The integer index of the interface
//          associated with the local MAC process.
// RETURN :: void : This routine does not return any value.
// **/

void MacSatelliteBentpipeRunTimeStat(Node *node,
                                     int interfaceIndex);

// /**
// FUNCTION :: MacSatelliteBentpipeFinalize
// LAYER :: Media Access Control
// PURPOSE :: This routine is called by the MAC layer dispatch code to
//            request that the current MAC process cease processing
//            and release any resources currently allocated to it.  This
//            normally occurs at the end of the simulation but in the future
//            may be called whenever the current MAC operation is
//            dynamically terminated.
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        interfaceIndex : int : The integer index of the interface
//          associated with the local MAC process.
// RETURN :: void : This routine does not return any value.
// **/

void MacSatelliteBentpipeFinalize(Node *node,
                                  int interfaceIndex);

// /**
// FUNCTION :: MacSatelliteBentpipeReadUpDownLinkInfo
// LAYER :: MAC
// PURPOSE :: This routine is called by the MAC-SATELLITE-BENTPIPE
//            init to read the uplink and downlink channel configurations
//            at ground station and satellite node
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        interfaceIndex : int : The integer index of the local
//          interface.
// +        nodeInput : NodeInput* : A pointer to the data structure that
//          contains the QualNet configuration file (and auxillary files)
//          information deserialized into memory.
// RETURN :: void : NULL
// **/
void
MacSatelliteBentpipeReadUpDownLinkInfo(
    Node*node,
    int interfaceIndex,
    NodeInput const *nodeInput);

// /**
// FUNCTION :: MacSatelliteBentpipeSetChannelInfo
// LAYER :: MAC
// PURPOSE :: This routine is to set the channel information
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        upDownChannelPairPtr : MacSatelliteBentpipeUpDownChannelInfo*
//          : A pointer to the data structure the up down channel info
//            parameters
// RETURN :: void : NULL
// **/
void
MacSatelliteBentpipeSetChannelInfo(
    Node* node,
    MacSatelliteBentpipeUpDownChannelInfo* upDownChannelPairPtr);

// /**
// FUNCTION :: MacSatelliteBentpipeAddUpDownLinkChannel
// LAYER    :: MAC
// PURPOSE ::  This routine is to return the channel information of the
//             uplink channel
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        phyIndex : int : phy index of this MAC
// +        state : MacSatelliteBentpipeState* : A pointer to the
//          local protocol-specific data structure for the satellite
//          MAC.
// +        interfaceIndex : int : interfaceIndex of the node
// +        upLinkChannelId : int : upLinkChannelId for which the
//          information needs to be added
// +        downLinkChannelId : int : downLinkChannelId for which the
//          information needs to be added
// RETURN :: void : NULL
// **/
void
MacSatelliteBentpipeAddUpDownLinkChannel(
    Node* node,
    int phyIndex,
    MacSatelliteBentpipeState* state,
    int interfaceIndex,
    int upLinkChannelId,
    int downLinkChannelId);

// /**
// FUNCTION :: MacSatelliteBentpipeGetUpLinkChannelInfo
// LAYER    :: MAC
// PURPOSE ::  This routine is to return the channel information of the
//             uplink channel
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        state : MacSatelliteBentpipeState* : A pointer to the
//          local protocol-specific data structure for the satellite
//          MAC.
// +        upLinkChannelId : int : upLinkChannelId for which the
//          information is needed
// RETURN :: MacSatelliteBentpipeChannel* : uplink channel info pointer
// **/
MacSatelliteBentpipeChannel*
MacSatelliteBentpipeGetUpLinkChannelInfo(
    Node* node,
    MacSatelliteBentpipeState* state,
    int upLinkChannelId);

// /**
// FUNCTION :: MacSatelliteBentpipeGetDownLinkChannelInfo
// LAYER    :: MAC
// PURPOSE ::  This routine is to return the channel information of the
//             downlink channel
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        state : MacSatelliteBentpipeState* : A pointer to the
//          local protocol-specific data structure for the satellite
//          MAC.
// +        downLinkChannelId : int : downLinkChannelId for which the
//          information is needed
// RETURN :: MacSatelliteBentpipeChannel* : downlink channel info pointer
// **/
MacSatelliteBentpipeChannel*
MacSatelliteBentpipeGetDownLinkChannelInfo(
    Node* node,
    MacSatelliteBentpipeState* state,
    int downLinkChannelId);

// /**
// FUNCTION :: MacSatelliteBentpipeGetChannelInfo
// LAYER    :: MAC
// PURPOSE :: This routine is to get the channel info of a particular
//            channel
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        state : MacSatelliteBentpipeState* : A pointer to the
//          local protocol-specific data structure for the satellite
//          MAC.
// +        channelIndex : the messages is added to this channel queue
// RETURN :: MacSatelliteBentpipeChannel* : pointer to the channel info
// **/
MacSatelliteBentpipeChannel*
MacSatelliteBentpipeGetChannelInfo(
    Node* node,
    MacSatelliteBentpipeState *state,
    int channelIndex);

// /**
// FUNCTION :: MacSatelliteBentpipeGetUpLinkChannelInfoGS
// LAYER :: MAC
// PURPOSE :: This routine is to return the uplink channel information
//            of ground station
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        state : MacSatelliteBentpipeState* : A pointer to the
//          local protocol-specific data structure for the satellite
//          MAC.
// RETURN :: MacSatelliteBentpipeChannel* : channel info pointer
// **/
MacSatelliteBentpipeChannel*
MacSatelliteBentpipeGetUpLinkChannelInfoGS(
    Node* node,
    MacSatelliteBentpipeState* state);

// /**
// FUNCTION :: MacSatelliteBentpipePhyStatusChangeNotification
// LAYER :: MAC
// PURPOSE :: This routine is called by the PHY layer to
//            inform the local MAC process that the PHY layer has reported
//            a change in the channel state.
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        channelIndex : channel whose status has changed
// +        interfaceIndex : interfaceIndex of the node
// +        oldPhyStatus : PhyStatusType : An enumerated value holding the
//          status of the PHY process _before_ the change in state occured.
// +        newPhyStatus : PhyStatusType : An enumerated value holding the
//          status of the PHY process _after_ the change in state occured.
// RETURN :: void : NULL
// **/
void
MacSatelliteBentpipeReceivePhyStatusChangeNotification(
    Node *node,
    int channelIndex,
    int interfaceIndex,
    PhyStatusType oldPhyStatus,
    PhyStatusType newPhyStatus);

// /**
// FUNCTION :: MacSatelliteBentpipeInsertInChannelQueue
// LAYER    :: MAC
// PURPOSE :: This routine is to insert the message in channel queue
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        state : MacSatelliteBentpipeState* : A pointer to the
//          local protocol-specific data structure for the satellite
//          MAC.
// +        channelIndex : the message is inserted to this channel queue
// +        msg : message pointer to insert in the queue
// RETURN :: void : NULL
// **/
void
MacSatelliteBentpipeInsertInChannelQueue(
    Node* node,
    MacSatelliteBentpipeState *state,
    int channelIndex,
    Message* msg);

// /**
// FUNCTION :: MacSatelliteBentpipeChannelHasPacketToSend
// LAYER    :: MAC
// PURPOSE :: This routine is to send next packet from the queue
//            whenever a new message has been added to the queue
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        state : MacSatelliteBentpipeState* : A pointer to the
//          local protocol-specific data structure for the satellite
//          MAC.
// +        channelIndex : the messages is added to this channel queue
// RETURN :: void : NULL
// **/
void
MacSatelliteBentpipeChannelHasPacketToSend(
    Node* node,
    MacSatelliteBentpipeState *state,
    int channelIndex);

// /**
// FUNCTION :: MacSatelliteBentpipeSendNxtPktFromChannelQueue
// LAYER    :: MAC
// PURPOSE :: This routine dequeues msg from channel queue and
//            transmit the signal.
// PARAMETERS ::
// +        node : Node* : A pointer to the local node data structure
// +        state : MacSatelliteBentpipeState* : A pointer to the
//          local protocol-specific data structure for the satellite
//          MAC.
// +        channelIndex : the messages will be dequeued from this
//          channelIndex queue
// RETURN :: void : NULL
// **/
void
MacSatelliteBentpipeSendNxtPktFromChannelQueue(
    Node* node,
    MacSatelliteBentpipeState* state,
    int channelIndex);

#endif /* MAC_SATELLITE_BENTPIPE_H */
