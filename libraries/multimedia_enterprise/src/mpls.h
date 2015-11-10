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

#ifndef MPLS_H
#define MPLS_H
#include "mpls_shim.h"


#define DEFAULT_MPLS_TABLE_SIZE 10


// define enumaration for MPLS-LDP Label retation mode
typedef enum {
    LIBERAL,
    CONSERVATIVE
} MplsLabelRetentionMode;


// define enumaration for operations to be performed on label stack
typedef enum {
    FIRST_LABEL,
    REPLACE_LABEL,
    POP_LABEL_STACK,
    REPLACE_LABEL_AND_PUSH
} Mpls_NHLFE_Operation;


// define enumaration for MPLS label encoding scheme.
typedef enum {
    MPLS_SHIM  // rfc 3032

    // Add other encoding techniques here
} MplsLabelEncodingTechnique;


// define enumaration for MPLS label distribution protocols.
typedef enum {
    MPLS_LDP
} MplsLabelDistributionProtocol;


typedef union {
    Mpls_Shim_LabelStackEntry shim;
} MplsLabelEntry;


// define MPLS NHLFE structure
typedef struct {
    int                  nextHop_OutgoingInterface;  // MPLS' "next hop"

    NodeAddress          nextHop;  // QualNet's nextHop - wireless nodes
                                   // use this at the MAC layer

    Mpls_NHLFE_Operation Operation;
    unsigned int*        labelStack;
    int                  numLabels;
} Mpls_NHLFE;


// define structure for ILM entry.
typedef struct {
    unsigned int label;
    Mpls_NHLFE*  nhlfe;      // map label to an nhlfe
} Mpls_ILM_entry;            // Incoming Label Map Entry


// define structure for FEC
typedef struct {
    NodeAddress ipAddress;
    int numSignificantBits;
    // Changed for adding priority support for FEC classification.
    unsigned int priority ;
} Mpls_FEC;


// define structure for FTN entry
typedef struct {
    Mpls_FEC fec;
    Mpls_NHLFE* nhlfe;
} Mpls_FTN_entry; // FEC to NHLFE Map Entry


struct mpls_var_struct;

typedef
void (*MplsNewFECCallbackFunctionType)(
    Node* node,
    struct mpls_var_struct* mpls,
    NodeAddress destAddr,
    int interfaceIndex,
    NodeAddress nextHop);


// define structure for MPLS statistics
typedef struct
{
    int numPacketsSentToMac;
    int numPacketsReceivedFromMac;
    int numPacketHandoveredToIp;
    int numPacketDroppedAtSource;
    int numPendingPacketsCleared;
    int numPacketsDroppedDueToExpiredTTL;
    int numPacketsDroppedDueToBadLabel;
    // The following stats have been added for
    // integrating IP with MPLS.
    int numPacketsSentToMPLSatIngress;
    int numPacketsHandoveredToIPatIngressDueToNoFTN;
    int numPacketsHandoveredToIPatLSRdueToBadLabel;
    int numPacketsHandoveredToIPatLSRdueToMTU;
    int numPacketsDroppedDueToLessMTU;
    int numPacketsReceivedFromIP;
}
MplsStatsType;


// Define MPLS structure.
typedef struct mpls_var_struct
{
    BOOL            collectStats;
    MplsStatsType*  stats;

    Mpls_FTN_entry* FTN;
    int             numFTNEntries;
    int             maxFTNEntries;

    Mpls_ILM_entry* ILM;
    int             numILMEntries;
    int             maxILMEntries;

    int             labelDistributionControlMode;

    MplsLabelEncodingTechnique labelEncoding;

    MplsNewFECCallbackFunctionType ReportNewFECCallback;
    // The following has been added for integrating MPLS with IP.
    BOOL            isEdgeRouter;
    BOOL            routeToIPOnError;
}
MplsData;

typedef enum
{
    MPLS_LABEL_REQUEST_PENDING,
    MPLS_READY_TO_SEND
} MplsStatus;

typedef struct
{
    MplsStatus status;
    NodeAddress nextHopAddress;
    unsigned char macAddress [MAX_MACADDRESS_LENGTH];
    unsigned short hwLength;
    unsigned short hwType;
}
TackedOnInfoWhileInMplsQueueType;

//----------------------------------------------------------------------
// MACRO      : MplsReturnStatus()
//
// PURPOSE    : returning pointer to the "status" field of the structure
//              "TackedOnInfoWhileInMplsQueueType". The structure is
//              assumed to copied on msg->info field of message.
//
// PARAMETERS : pointer to message structure.
//----------------------------------------------------------------------
#define MplsReturnStatus(msg) (((TackedOnInfoWhileInMplsQueueType*)  \
                                  (MESSAGE_ReturnInfo(msg)))->status)


//----------------------------------------------------------------------
// MACRO      : MplsReturnOutgoingInterfaceFromNHLFE()
//
// PURPOSE    : returning outgoing interface index to reach the next
//              hop from NHLFE structure.
//
// PARAMETERS : pointer to NHLFE structure.
//----------------------------------------------------------------------
#define MplsReturnOutgoingInterfaceFromNHLFE(x) \
            (x->nextHop_OutgoingInterface)

//-----------------------------------------------------------------------
// FUNCTION     : MplsInit()
//
// PURPOSE      : Initialize variables, being called once for each node
//                at the beginning.
//
// PARAMETERS   : node - The node to be initialized.
//                nodeInput - the input configuration file
//                interfaceIndex - interface index
//                mplsVar - pointer to pointer to "MplsData" structure.
//
// RETURN VALUE : void
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
void MplsInit(
    Node* node,
    const NodeInput* nodeInput,
    int interfaceIndex,
    MplsData** mplsVar);


//-------------------------------------------------------------------------
// FUNCTION     : MplsLayer()
//
// PURPOSE      : Process MPLS Messages
//
// PARAMETERS   : node - the node which is receiving the message
//                interfaceIndex - interface index via which the message
//                                 is received
//                msg - the event/message
//
// RETURN VALUE : void
//
// ASSUMPTION   : None
//-------------------------------------------------------------------------
void MplsLayer(Node* node, int interfaceIndex, Message* msg);


//-------------------------------------------------------------------------
// FUNCTION     : MplsFinalize()
//
// PURPOSE      : Print out statistics, being called at the end. Calls
//                MplsPrintStats()
//
// PARAMETERS   : node - node who's statistics is to be printed.
//
// RETURN VALUE : void
//
// ASSUMPTION   : none
//-------------------------------------------------------------------------
void MplsFinalize(Node* node);

//-------------------------------------------------------------------------
// FUNCTION     : MplsIpOutputQueueInsert()
//
// PURPOSE      : Inserting a packet into the mpls queue.If the packet is
//                from ip and destination address is ANY_DEST then add a
//                broadcast label to the packet and then insert into the
//                queue.
//
// PARAMETERS   : node - node which is inserting the packet in the queue
//                mpls - pointer to mpls structure.
//                interfaceIndex - interface index of the queue.
//                msg - message/packet to be inserted in the queue.
//                nextHopAddress - next hop address of the packet/message.
//                QueueIsFull - MplsIpOutputQueueInsert() will set this
//                              parameter to TRUE if queue is full.
//                packetFromIP - set this parameter to TRUE if the packet
//                               is received from IP.
//
// RETURN VALUE : void
//
// ASSUMPTION   : none
//-------------------------------------------------------------------------
void MplsIpOutputQueueInsert(
    Node* node,
    MplsData* mpls,
    int interfaceIndex,
    Message* msg,
    NodeAddress nextHopAddress,
    BOOL* QueueIsFull,
    BOOL packetFromIP);


//-------------------------------------------------------------------------
// FUNCTION     : MplsOutputQueueIsEmpty()
//
// PURPOSE      : checking whether a queue associated with a perticular
//                interface index is empty or not.
//
// PARAMETERS   : node - node which is checking the queue
//                interfaceIndex - interface index of the queue.
//
// RETURN VALUE : BOOL, Returns TRUE is queue is empty or FALSE otherwise
//
// ASSUMPTIONS  : none
//-------------------------------------------------------------------------
BOOL MplsOutputQueueIsEmpty(Node* node, int interfaceIndex);

void MplsNotificationOfPacketDrop(
    Node*  node,
    Message*  msg,
    NodeAddress nextHopAddress,
    int interfaceIndex);


//-------------------------------------------------------------------------
// FUNCTION     : MplsOutputQueueTopPacketForAPriority()
//
// PURPOSE      : Look at the packet at the front of the specified priority
//                output queue
//
// PARAMETERS   : node - the node which is performing the above operation.
//                interfaceIndex - interface index of the queue
//                priority - Network queueing priority
//                msg - the message/packet to be looked for.
//                nextHopAddress - next hop address of the packet.
//                posInQueue - the packet's position in the queue.
//
// RETURN     :: BOOL : TRUE if there is a packet, FALSE otherwise.
//
// ASSUMPTIONS  : none
//-------------------------------------------------------------------------
BOOL MplsOutputQueueTopPacketForAPriority(
    Node*  node,
    int interfaceIndex,
    TosType priority,
    Message**  msg,
    NodeAddress*  nextHopAddress,
    MacHWAddress* nextHopMacAddr,
    int posInQueue);


//-------------------------------------------------------------------------
// FUNCTION     : MplsOutputQueueDequeuePacketForAPriority()
//
// PURPOSE      : Remove the packet at the front of the specified priority
//                output queue.
//
// PARAMETERS   : node - the node which is performing the dequeue operation.
//                interfaceIndex - interface index of the queue
//                priority - Network queueing priority
//                msg - the message/packet to be dequeued.
//                nextHopAddress - next hop address of the packet.
//                posInQueue - the packet's position in the queue.
//
// RETURN     :: BOOL :  TRUE if dequeued successfully, FALSE otherwise.
//
// ASSUMPTIONS  : none
//-------------------------------------------------------------------------
BOOL MplsOutputQueueDequeuePacketForAPriority(
    Node* node,
    int interfaceIndex,
    TosType priority,
    Message** msg,
    NodeAddress* nextHopAddress,
    MacHWAddress* nextHopMacAddr,
    int posInQueue);


//-------------------------------------------------------------------------
// FUNCTION     : MplsOutputQueueDequeuePacketForAPriority()
//
// PURPOSE      : Remove the packet at the front of the specified priority
//                output queue.
//
// PARAMETERS   : node - the node which is performing the dequeue operation.
//                interfaceIndex - interface index of the queue
//                priority - Network queueing priority
//                msg - the message/packet to be dequeued.
//                nextHopAddress - next hop address of the packet.
//
// RETURN VALUE : void
//
// ASSUMPTIONS  : none
//-------------------------------------------------------------------------
void MplsReceivePacketFromMacLayer(
    Node* node,
    int interfaceIndex,
    Message* msg,
    NodeAddress lastHopAddress);


//-------------------------------------------------------------------------
// FUNCTION     : MplsOutputQueueTopPacket()
//
// PURPOSE      : Look at the packet at the front of the output queue
//
// PARAMETERS   : node - the node which is performing the above operation.
//                interfaceIndex - interface index of the queue
//                msg - the message/packet to be looked for.
//                nextHopAddress - next hop address of the packet.
//                priority - Network queueing priority
//                posInQueue - the packet's position in the queue.
//
// RETURN     :: BOOL : TRUE if there is a packet, FALSE otherwise.
//
// ASSUMPTIONS  : none
//-------------------------------------------------------------------------
BOOL MplsOutputQueueTopPacket(
    Node* node,
    int interfaceIndex,
    Message** msg,
    NodeAddress* nextHopAddress,
    MacHWAddress* nextHopMacAddr,
    TosType* priority,
    int posInQueue);


//-------------------------------------------------------------------------
// FUNCTION     : MplsOutputQueueDequeuePacket()
//
// PURPOSE      : Remove the packet at the front of output queue.
//
// PARAMETERS   : node - the node which is performing the dequeue operation.
//                interfaceIndex - interface index of the queue
//                priority - Network queueing priority
//                msg - the message/packet to be dequeued.
//                nextHopAddress - next hop address of the packet.
//                priority - priority of the packet.
//                posInQueue - position of packet in queue which can be
//                             dequeued
//
// RETURN     :: BOOL :  TRUE if dequeued successfully, FALSE otherwise.
//
// ASSUMPTIONS  : none
//-------------------------------------------------------------------------
BOOL MplsOutputQueueDequeuePacket(
    Node* node,
    int interfaceIndex,
    Message** msg,
    NodeAddress* nextHopAddress,
    MacHWAddress* nextHopMacAddr,
    TosType* priority,
    int posInQueue);


//-------------------------------------------------------------------------
// FUNCTION     : MplsAssignFECtoOutgoingLabelAndInterface()
//
// PURPOSE      : creating an FTN entry, and also to change status of all
//                the pending packets in the queue from
//                MPLS_LABEL_REQUEST_PENDING, to MPLS_READY_TO_SEND.This
//                function will be used by an egress router.
//
// PARAMETERS   : node - the node which is performing the above operation
//                ipAddress - ipaddress (Ftn->ipAddress) to be added.
//                numSignificantBits - number of significant bits in IP
//                                     address.
//                nextHopAddr - next hop address to reach the destination
//                              (ipAddress is actually the destination
//                               address)
//                operation - Operation to be performed on the nhlfe entry.
//                labelStack - pointer to the label-stack to be
//                             added in FTN entry
//                numLabels - number of Labels to pushed in the label stack
//
// RETURN VALUE : none.
//
// ASSUMPTION   : label stack depth is 1.
//-------------------------------------------------------------------------
void MplsAssignFECtoOutgoingLabelAndInterface(
    Node* node,
    NodeAddress ipAddress,
    int numSignificantBits,
    NodeAddress nextHopAddr,
    Mpls_NHLFE_Operation Operation,
    unsigned int* labelStack,
    int numLabels);


//-------------------------------------------------------------------------
// FUNCTION     : MplsReturnStateSpace()
//
// PURPOSE      : Returning the pointer to the mpls structure (if exists).
//
// PARAMETERS   : node - Pointer to the node structure; the node which is
//                       performing the above operation.
//
// RETURN VALUE : pointer to the mpls structure if initialized and exists;
//                the return value is NULL otherwise
//
// ASSUMPTION   : The function will be used as an interface function to
//                other modules, outside the scope of this file.
//                E.g MPLS-LDP, RSVP-TE etc.
//-------------------------------------------------------------------------
MplsData* MplsReturnStateSpace(Node* node);


//-------------------------------------------------------------------------
// FUNCTION     : MplsRoutePacket()
//
// PURPOSE      : Routing a packet to the next hop is corresponding FTN
//                entry for that dest is found in the FTN table. Otherwise
//                it invokes a callback function to start label
//                distribution operation.
//
// PARAMETERS   : node - the node which is tring to route the packet.
//                mpls - pointer to mpls structure.
//                msg - msg/packet to be routed.
//                destAddr - destination address of the msg/packet.
//                packetWasRouted - MplsRoutePacket() will set this
//                                  parameterto TRUE if the packet is
//                                  successfully routed.
//                priority - priority of the packet.
//                incomingInterface - Index of interface on which packet
//                                    arrived.
//                outgoingInterface - Used only when the application
//                                    specifies a specific interface to use
//                                    to transmit packet.
//
// RETURN VALUE : void
//
// ASSUMPTION   : none
//-------------------------------------------------------------------------
// Changed for adding priority support for FEC classification.
// While routing packet via MPLS take care of packet's priority.
void MplsRoutePacket(
    Node* node,
    MplsData* mpls,
    Message* msg,
    NodeAddress destAddr,
    BOOL* packetWasRouted,
    unsigned int priority,
    int incomingInterface,
    int outgoingInterface);


//-------------------------------------------------------------------------
// FUNCTION     : MplsSetNewFECCallbackFunction()
//
// PURPOSE      : seting a callback function to invoke label distibution
//                operation
//
// PARAMETERS   : node - node which is setting call back function.
//                NewFECCallbackFunctionPtr - pointer to callback function
//
// RETURN VALUE : none.
//
// ASSUMPTION   : none
//-------------------------------------------------------------------------
void MplsSetNewFECCallbackFunction(
    Node* node,
    MplsNewFECCallbackFunctionType NewFECCallbackFunctionPtr);

//-------------------------------------------------------------------------
// FUNCTION     : MplsCheckAndAssignLabelToILM()
//
// PURPOSE      : To install a label for forwarding and switching use.
//                This will be used for control mode ORDERED only.
//
// PARAMETERS   : node - node which is installing label.
//                ipAddress - Fec->ipAddress
//                numSignificantBits - number of significant bits in
//                                     Fec->ipAddress
//                nextHopAddr - next hop address to reach the destination
//                              (ipAddress is actually the destination
//                               address)
//                operation - Operation to be performed on the nhlfe entry.
//                incomingLabel - pointer to incoming label
//                outgoingLabel - pointer to outgoing label stack.
//                putInLabel - set this parameter TRUE if incoming label
//                             to put.
//                outgoingLabel - set this parameter TRUE if outgoing
//                                label to put.
//                numLabels - number of labels
//
// RETURN VALUE : none.
//
// ASSUMPTIONS  : This will be used for control mode ORDERED only.
//-------------------------------------------------------------------------
void MplsCheckAndAssignLabelToILM(
    Node* node,
    NodeAddress ipAddress,
    int numSignificantBits,
    NodeAddress nextHopAddr,
    Mpls_NHLFE_Operation Operation,
    unsigned int* incomingLabel,
    unsigned int* outgoingLabel,
    BOOL putInLabel,
    BOOL putOutLabel,
    int numLabels);


//-------------------------------------------------------------------------
// FUNCTION     : MplsRemoveILMEntry()
//
// PURPOSE      : Removing an ILM entry from ILM table. and returning the
//                corresponding outgoing label.
//
// PARAMETERS   : mpls - pointer to mpls structure.
//                label - incoming label to be deleted
//                outLabel - corresponding outgoing label
// RETURN VALUE : none.
//
// ASSUMPTIONS  : none
//-------------------------------------------------------------------------
void MplsRemoveILMEntry(
    MplsData* mpls,
    unsigned int label,
    unsigned int* outLabel);


//-------------------------------------------------------------------------
// FUNCTION     : MplsRemoveFTNEntry()
//
// PURPOSE      : Removing and FTN entry from FTN table. Given
//                Fec->ipAddress and corresponding outgoing Label
//
// PARAMETERS   : mpls - pointer to mpls structure.
//                ipAddress - ipAddress to be matched
//                outLabel - outgoing label to be matched
//
// RETURN VALUE : void
//
// ASSUMPTIONS  : none
//-------------------------------------------------------------------------
void MplsRemoveFTNEntry(
    MplsData* mpls,
    NodeAddress ipAddress,
    unsigned int outLabel);

//-------------------------------------------------------------------------
// FUNCTION     : AssignLabelForControlModeIndependent()
//
// PURPOSE      : To install a label for forwarding and switching use.
//                This will be used for control mode INDEPEDENT only.
//
// PARAMETERS   : node - node which is installing label.
//                ipAddress - Fec->ipAddress
//                numSignificantBits - number of significant bits in
//                                     Fec->ipAddress
//                nextHopAddr - next hop address to reach the destination
//                              (ipAddress is actually the destination
//                               address)
//                operation - Operation to be performed on the nhlfe entry.
//                incomingLabel - pointer to incoming label
//                outgoingLabel - pointer to outgoing label stack.
//                putInLabel - set this parameter TRUE if incoming label
//                             to put.
//                outgoingLabel - set this parameter TRUE if outgoing
//                                label to put.
//                numLabels - number of labels
//
// RETURN VALUE : none.
//
// ASSUMPTIONS  : This will be used for control mode INDEPEDENT only.
//-------------------------------------------------------------------------
void AssignLabelForControlModeIndependent(
    Node* node,
    NodeAddress ipAddress,
    int numSignificantBits,
    NodeAddress nextHopAddr,
    Mpls_NHLFE_Operation Operation,
    unsigned int* inLabel,
    unsigned int* outLabel,
    BOOL putInLabel,
    BOOL putOutLabel,
    int numLabels);

//-------------------------------------------------------------------------
// FUNCTION     : MplsAddNHLFEEntry()
//
// PURPOSE      : Adding an entry to mpls NHLFE structure.
//
// PARAMETERS   : mpls - pointer to mpls structure
//                nextHop_OutgoingInterface - outgoing interface to reach
//                                           the next hop.
//                nextHop - Where next hop is the node such that label on the
//                          top of the label stack can be used to switch and
//                          forward traffic to that node.
//                Operation - label operation to be performed on label stack
//                labelStack - pointer to the label to be put on the top
//                             of the label stack
//
// RETURN VALUE : pointer to the newly added NHLFE entry in the NHLFE table
//
// ASSUMPTION   : currently supported label stack depth is only one.
//-------------------------------------------------------------------------
Mpls_NHLFE* MplsAddNHLFEEntry(
    MplsData* mpls,
    int nextHop_OutgoingInterface,
    NodeAddress nextHop,
    Mpls_NHLFE_Operation Operation,
    unsigned int* labelStack,
    int numLabels);

//-------------------------------------------------------------------------
// FUNCTION     : MplsAddILMEntry()
//
// PURPOSE      : adding an entry to mpls ILM table.
//
// PARAMETERS   : mpls - pointer to mpls structure.
//                incomingLabel - label to be inserted in the ILM table
//                nhlfe - corresponding nhlfe entry to be added
//
// RETURN VALUE : void
//
// ASSUMPTIONS  : none
//-------------------------------------------------------------------------
void MplsAddILMEntry(
    MplsData* mpls,
    unsigned int incomingLabel,
    Mpls_NHLFE* nhlfe);

//-------------------------------------------------------------------------
// FUNCTION     : MplsAddFTNEntry()
//
// PURPOSE      : Adding an entry to FTN table.
//
// PARAMETERS   : mpls - pointer to mpls structure
//                fec - FEC to be added in the FTN table.
//                nhlfe - corresponding nhlfe entry to be added.
//
// RETURN VALUE : void
//
// ASSUMPTION   : none
//-------------------------------------------------------------------------
void MplsAddFTNEntry(MplsData* mpls, Mpls_FEC fec, Mpls_NHLFE* nhlfe);

//-------------------------------------------------------------------------
// FUNCTION     : Print_FTN_And_ILM()
//
// PURPOSE      : printing ILM and FTN tables.
//
// PARAMETERS   : node - node which is printing the ILM and FTN table.
//
// RETURN VALUE : void
//
// ASSUMPTIONS  : none
//-------------------------------------------------------------------------
void Print_FTN_And_ILM(Node* node);

//-------------------------------------------------------------------------
// FUNCTION     : MplsSetLabelAdvertisementMode
//
// PURPOSE      : Informing MPLS-LDP label advertisement to mac layer mpls
//
// PARAMETERS   : node - the node which is performing the above operation.
//                controlMode - label advertisement control mode st by
//                              MPLS-LDP
//
// RETURN VALUE : none
//
// ASSUMPTIONS  : none
//-------------------------------------------------------------------------
void MplsSetLabelAdvertisementMode(Node* node, int controlMode);

int SetAllPendingReadyToSend(
    Node* node,
    int interfaceIndex,
    NodeAddress destIp,
    MplsData* mpls,
    Mpls_NHLFE* nhlfe);


//-------------------------------------------------------------------------
// FUNCTION     : MplsIpQueueInsert()
//
// PURPOSE      : Inserting a packet into the mpls queue.If the packet is
//                from ip and destination address is ANY_DEST then add a
//                broadcast label to the packet and then insert into the
//                queue.
//
// PARAMETERS   : node - node which is inserting the packet in the queue
//                mpls - pointer to Mpls structure
//                msg - pointer to Message structure.
//                interfaceIndex - interface index of the queue.
//                nextHopAddress - next hop address of the packet/message.
//                status - Mpls Status
//                tos - Type of Service
//                packetFromIP - set this parameter to TRUE if the packet
//                               is received from IP.
//                QueueIsFull - MplsIpOutputQueueInsert() will set this
//                              parameter to TRUE if queue is full.
//
// RETURN VALUE : void
//
// ASSUMPTION   : none
//-------------------------------------------------------------------------

void MplsIpQueueInsert(Node *node,
                       MplsData *mpls,
                       Message *msg,
                       int interfaceIndex,
                       NodeAddress nextHopAddress,
                       MplsStatus status,
                       TosType* tos,
                       BOOL packetFromIP,
                       BOOL* QueueIsFull);

//-----------------------------------------------------------------------------
// FUNCTION     MplsExtractInfoField
// PURPOSE      Extract the information from mpls packet info field.
// PARAMETERS   Node *node
//                  Pointer to node.
//              int interfaceIndex
//                  Index of interface.
//              Message **msg
//                  Mpls Packet.
// RETURN       Next hop of mpls packet.
//
// ASSUMPTION   : none
//-------------------------------------------------------------------------
NodeAddress
MplsExtractInfoField(Node* node,
                     int interfaceIndex,
                     Message **msg,
                     MacHWAddress* nexthopmacAddr);

#endif // end MPLS_H

