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

// iostream.h is needed to support DMSO RTI 1.3v6 (non-NG).

#ifdef NOT_RTI_NG
#include <iostream.h>
#else /* NOT_RTI_NG */
#include <iostream>
using namespace std;
#endif /* NOT_RTI_NG */

#include <stdio.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#endif //WIN32

#include "api.h"
#include "partition.h"
#include "app_messenger.h"
#include "external_util.h"
#include "external_socket.h"
#include "vrlink_sim_iface_impl.h"
#include "mac.h"
#include "mapping.h"
#include "phy.h"

#ifdef MILITARY_RADIOS_LIB
#include "mac_link16.h"
#endif

VRLinkSimulatorInterface_Impl* VRLinkSimulatorInterface_Impl::simIface = NULL;

// /**
// FUNCTION :: VRLinkSimulatorInterface_Impl
// PURPOSE :: Returns the pointer of the singleton class.
// PARAMETERS :: None.
// RETURN :: VRLinkSimulatorInterface_Impl* : Singleton calss pointer.
// **/
VRLinkSimulatorInterface_Impl* VRLinkSimulatorInterface_Impl::GetSimulatorInterface()
{
    if (VRLinkSimulatorInterface_Impl::simIface == NULL)
    {
        VRLinkSimulatorInterface_Impl::simIface = new VRLinkSimulatorInterface_Impl;
    }

    return VRLinkSimulatorInterface_Impl::simIface;
}

// /**
// FUNCTION :: ReturnInfoFromMsg
// PURPOSE :: Returns a pointer to the "info" field with given info type in
//            the info array of the message.
// PARAMETERS ::
// + msg : Message* : Message for which "info" field has to be returned.
// RETURN :: char* : Pointer to the "info" field with given type;NULL if not
//                   found.
// **/
char* VRLinkSimulatorInterface_Impl::ReturnInfoFromMsg(Message* msg)
{
    return ::MESSAGE_ReturnInfo(msg);
}

// /**
// FUNCTION :: ReturnEventTypeFromMsg
// PURPOSE :: Returns the event type of a message
// PARAMETERS ::
// + msg : Message* : Message of which the event type is to be returned.
// RETURN :: short : Event type.
// **/
short VRLinkSimulatorInterface_Impl::ReturnEventTypeFromMsg(Message* msg)
{
    return MESSAGE_GetEvent(msg);
}

// /**
// FUNCTION :: ReturnPacketFromMsg
// PURPOSE :: Returns a pointer to the packet of the message.
// PARAMETERS ::
// + msg : Message* : Message whose packet pointer is to be returned.
// RETURN :: char* : Pointer to message packet.
// **/
char* VRLinkSimulatorInterface_Impl::ReturnPacketFromMsg(Message* msg)
{
    return MESSAGE_ReturnPacket(msg);
}

// /**
// FUNCTION :: FreeMessage
// PURPOSE :: When the message is no longer needed it can be freed. Firstly,
//            the data portion of the message is freed. Then the message
//            itself is freed. It is important to remember to free the
//            message, otherwise there will be memory leaks in the program.
// PARAMETERS ::
// + node : Node* : Node which is freeing the message.
// + msg : Message* : Message which has to be freed.
// RETURN :: void : NULL.
// **/
void VRLinkSimulatorInterface_Impl::FreeMessage(Node* node, Message* msg)
{
    ::MESSAGE_Free(node, msg);
}

// /**
// FUNCTION :: AllocateMessage
// PURPOSE :: Allocate a new message structure. This is called when a new
//            message has to be sent through the system. The last three
//            parameters indicate the nodeId, layerType, and eventType
//            that will be set for this message.
// PARAMETERS ::
// + node : Node* : Node which is allocating message.
// + layerType : int : Layer type to be set for this message.
// + protocol : int : Protocol to be set for this message.
// + eventType : int : Event type to be set for this message.
// RETURN :: Message* : Newly allocated message.
// **/
Message* VRLinkSimulatorInterface_Impl::AllocateMessage(
    Node *node,
    int layerType,
    int protocol,
    int eventType)
{
    Message* newMsg = ::MESSAGE_Alloc(node, layerType, protocol, eventType);
    return newMsg;
}

// /**
// FUNCTION :: AllocateInfoInMsg
// PURPOSE :: Allocate the default "info" field for the message. It assumes
//            the type of the info field to be allocated is
//            INFO_TYPE_DEFAULT.
// PARAMETERS ::
// + node : Node* : Node which is allocating the info field.
// + msg : Message* : Message for which the "info" field has to be allocated.
// + infoSize: int : Size of the "info" field to be allocated.
// RETURN :: char* : Pointer to info field.
// **/
char * VRLinkSimulatorInterface_Impl::AllocateInfoInMsg(
    Node *node,
    Message *msg,
    int infoSize)
{
    return ::MESSAGE_InfoAlloc(node, msg, infoSize);
}

// /**
// FUNCTION :: SendMessageWithDelay
// PURPOSE :: Function call used to send a message within QualNet. When a
//            message is sent using this mechanism, only a pointer to the
//            message is actually sent through the system. So the user has
//            to be careful not to do anything with the pointer to the
//            message once SendMessageWithDelay has been called.
// PARAMETERS ::
// + node : Node* : Node which is sending message.
// + msg : Message* : Message to be delivered.
// + delay : clocktype : Delay suffered by this message.
// RETURN :: void : NULL.
// **/
void VRLinkSimulatorInterface_Impl::SendMessageWithDelay(
    Node *node,
    Message *msg,
    clocktype delay)
{
    ::MESSAGE_Send(node, msg, delay);
}

// /**
// API       :: SendRemoteMessageWithDelay
// LAYER     :: ANY_LAYER
// PURPOSE   :: Function used to send a message to a node that might be
//              on a remote partition.  The system will make a shallow copy
//              of the message, meaning it can't contain any pointers in
//              the info field or the packet itself.  This function is very
//              unsafe.  If you use it, your program will probably crash.
//              Only I can use it.
//
// PARAMETERS ::
// + node       :  Node*     : node which is sending message
// + destNodeId :  NodeId    : nodeId of receiving node
// + msg        :  Message*  : message to be delivered
// + delay      :  clocktype : delay suffered by this message.
// RETURN    :: void : NULL
// **/
void VRLinkSimulatorInterface_Impl::SendRemoteMessageWithDelay(
    Node*     node,
    NodeId    destNodeId,
    Message*  msg,
    clocktype delay)
{
    ::MESSAGE_RemoteSend(node, destNodeId, msg, delay);
}

void VRLinkSimulatorInterface_Impl::SetInstanceIdInMsg(Message* msg, short instance)
{
   MESSAGE_SetInstanceId(msg, instance);
}

void VRLinkSimulatorInterface_Impl::InitMessengerApp(EXTERNAL_Interface *iface)
{
    Node* node = iface->partition->firstNode;

    while (node)
    {
        ::MessengerInit(node);
        node = node->nextNodeData;
    }
}

void VRLinkSimulatorInterface_Impl::SendMessageThroughMessenger(
    Node *node,
    MessengerPktHeader pktHdr,
    char *additionalData,
    int additionalDataSize,
    MessageResultFunctionType functionPtr)
{
    ::MessengerSendMessage(
        node,
        pktHdr,
        additionalData,
        additionalDataSize,
        functionPtr);
}

void VRLinkSimulatorInterface_Impl::MoveHierarchyInGUI(
    int hierarchyID,
    Coordinates coordinates,
    Orientation orientation,
    clocktype time)
{
    ::GUI_MoveHierarchy(hierarchyID, coordinates, orientation, time);
}

BOOL VRLinkSimulatorInterface_Impl::ReturnNodePointerFromPartition(
    PartitionData* partitionData,
    Node** node,
    NodeId nodeId,
    BOOL remoteOK)
{
    return ::PARTITION_ReturnNodePointer(
               partitionData,
               node,
               nodeId,
               remoteOK);
}

NodeAddress VRLinkSimulatorInterface_Impl::GetDefaultInterfaceAddressFromNodeId(
    Node *node,
    NodeAddress nodeId)
{
    return ::MAPPING_GetDefaultInterfaceAddressFromNodeId(node, nodeId);
}

void VRLinkSimulatorInterface_Impl::ReadString(
    const NodeAddress nodeId,
    const NodeAddress interfaceAddress,
    const NodeInput *nodeInput,
    const char *parameterName,
    BOOL *wasFound,
    char *parameterValue)
{
    ::IO_ReadString(
        nodeId,
        interfaceAddress,
        nodeInput,
        parameterName,
        wasFound,
        parameterValue);
}

void VRLinkSimulatorInterface_Impl::ReadInt(
    const NodeAddress nodeId,
    const NodeAddress interfaceAddress,
    const NodeInput *nodeInput,
    const char *parameterName,
    BOOL *wasFound,
    int *parameterValue)
{
    ::IO_ReadInt(
        nodeId,
        interfaceAddress,
        nodeInput,
        parameterName,
        wasFound,
        parameterValue);
}

void VRLinkSimulatorInterface_Impl::ReadDouble(
    const NodeAddress nodeId,
    const NodeAddress interfaceAddress,
    const NodeInput *nodeInput,
    const char *parameterName,
    BOOL *wasFound,
    double *parameterValue)
{
    ::IO_ReadDouble(
        nodeId,
        interfaceAddress,
        nodeInput,
        parameterName,
        wasFound,
        parameterValue);
}

void VRLinkSimulatorInterface_Impl::ReadTime(
    const NodeAddress nodeId,
    const NodeAddress interfaceAddress,
    const NodeInput *nodeInput,
    const char *parameterName,
    BOOL *wasFound,
    clocktype *parameterValue)
{
    ::IO_ReadTime(
        nodeId,
        interfaceAddress,
        nodeInput,
        parameterName,
        wasFound,
        parameterValue);
}

void VRLinkSimulatorInterface_Impl::ReadCachedFile(
    const NodeAddress nodeId,
    const NodeAddress interfaceAddress,
    const NodeInput *nodeInput,
    const char *parameterName,
    BOOL *wasFound,
    NodeInput *parameterValue)
{
    ::IO_ReadCachedFile(
        nodeId,
        interfaceAddress,
        nodeInput,
        parameterName,
        wasFound,
        parameterValue);
}

char* VRLinkSimulatorInterface_Impl::GetDelimitedToken(
    char* dst,
    const char* src,
    const char* delim,
    char** next)
{
    return ::IO_GetDelimitedToken(dst, src, delim, next);
}

void VRLinkSimulatorInterface_Impl::TrimLeft(char* str)
{
    ERROR_Assert(
        (str != NULL),
        "String to trim cannot be NULL");

    char *p = str;
    unsigned stringLength = strlen(str);
    unsigned numBytesWhitespace = 0;

    while (numBytesWhitespace < stringLength
          && isspace(*p))
    {
        p++;
        numBytesWhitespace++;
    }

    if (numBytesWhitespace > 0)
    {
        memmove(
            str,
            str + numBytesWhitespace,
            stringLength - numBytesWhitespace + 1);
    }
}

void VRLinkSimulatorInterface_Impl::TrimRight(char* str)
{
    ERROR_Assert(
        (str != NULL),
        "String to trim cannot be NULL");

    unsigned stringLength = strlen(str);
    if (stringLength == 0) { return; }

    // p points one byte to left of terminating NULL.
    char *p;

    for (p = &str[stringLength - 1]; isspace(*p); p--)
    {
        if (p == str)
        {
            // Entire string was whitespace.
            *p = 0;
            return;
        }
    }//while//

    *(p + 1) = 0;
}

void VRLinkSimulatorInterface_Impl::ParseNodeIdHostOrNetworkAddress(
    const char s[],
    NodeAddress *outputNodeAddress,
    int *numHostBits,
    BOOL *isNodeId)
{
    ::IO_ParseNodeIdHostOrNetworkAddress(
        s,
        outputNodeAddress,
        numHostBits,
        isNodeId);
}

char* VRLinkSimulatorInterface_Impl::GetTokenOrEmptyField(
    char* token,
    unsigned tokenBufSize,
    unsigned& numBytesCopied,
    bool& overflowed,
    const char*& src,
    const char* delimiters,
    bool& foundEmptyField)
{
    return GetTokenHelper(
               false,
               foundEmptyField,
               overflowed,
               token,
               tokenBufSize,
               numBytesCopied,
               src,
               delimiters);
}

char* VRLinkSimulatorInterface_Impl::GetToken(
    char* token,
    unsigned tokenBufSize,
    unsigned& numBytesCopied,
    bool& overflowed,
    const char*& src,
    const char* delimiters)
{
    bool unusedFoundEmptyField = false;

    return GetTokenHelper(
               true,
               unusedFoundEmptyField,
               overflowed,
               token,
               tokenBufSize,
               numBytesCopied,
               src,
               delimiters);
}

// This function returns a "token" from a string.  Tokens are separated by
// delimiters.  E.g., if the delimiter is "," and the string is
// "foo,bar,,baz", then the tokens are "foo", "bar", empty field, and "baz".
//
// A description of the parameters follows:
//
// If skipEmptyFields is true, then empty fields will not be returned, but
// the next available token will be returned instead.  If skipEmptyFields is
// false, the function will return empty fields.
//
// The foundEmptyField argument will initially be set to false by the
// function.  If skipEmptyFields is false, and an empty field is found, then
// the argument will be set to true.
//
// The overflowed argument will initially be set to false by the function.
// If the token parameter is non-NULL, and the length of the token (excluding
// the null terminator) is greater than tokenBufSize - 1, then overflowed will
// be set to true.
//
// If the token parameter is non-NULL:
//
//   The token will be copied to the buffer pointed to by token and
//   null-terminated.  The parameter tokenBufSize is specified by the user,
//   and is the size of the token buffer in bytes.  tokenBufSize should be
//   large enough to store both the actual characters in the token and a
//   null terminator.  Specifically:
//
//   If tokenBufSize is 0, then nothing will be done to the token parameter.
//   If tokenBufSize is > 0, as much of the token is copied to the token
//   parameter as possible, with the requirement that a null terminator will
//   always be set.
//
//   If skipEmptyFields is false, an empty field is detected, and
//   tokenBufSize is > 0, then token[0] will be set to the null terminator.
//
// If the token parameter is NULL:
//
//   The token parameter is left alone, the tokenBufSize parameter is
//   ignored, and other behavior will occur (described later).
//
// The function attempts to set the numBytesToCopyOrCopied parameter to the
// number of bytes in the token (excluding the null terminator).  If
// skipEmptyFields is false and an empty field is detected,
// numBytesToCopyOrCopied is set to 0.
//
// If the token parameter is non-NULL:
//
//   numBytesToCopyOrCopied may not exceed tokenBufSize - 1 ("- 1" allows for
//   the null terminator).
//
// If the token parameter is NULL:
//
//   numBytesToCopyOrCopied will always equal the actual size of the token
//   (excluding the null terminator).
//
// The src parameter points to the string which contains the tokens.  src is
// also set by the function to point to a new location in the string when:
//
//   (1) A token was found, or
//   (2) skipEmptyFields is false and src points to an empty field.
//
// If either case is true:
//
//   For case (1), this also means the byte after the token is either a
//   delimiter or a null terminator.  If that byte was a delimiter, then src
//   is set to point to one byte after the delimiter byte.  If that byte was
//   a null terminator, then src is set to point to the null terminator.
//
//   For case (2), src is set to point to src[1].
//
//   For both cases, the function will return a non-NULL value.  If the token
//   parameter was passed in as non-NULL, then the token parameter is
//   returned.  If the token parameter was passed in as NULL, then:
//
//   For case (1), a pointer to the first byte of the token is returned.
//
//   For case (2), a pointer to the empty field (which is the same as the
//   original value of src) is returned.
//
// If neither of the above cases is true, then one of the following must be
// true:
//
//   (1) src points to a null terminator, or
//   (2) skipEmptyFields is true and src points to only empty fields through
//       the end of the string.
//
//   The function returns NULL in either situation above.  When NULL is
//   returned, numBytesToCopyOrCopied is set to 0, and no other argument is
//   modified by the function.
//
// The delimiters parameter is not optional (the function will assert false
// if the delimiters parameter is passed in as NULL or as a string which
// consists of only a null terminator).  The delimiters argument should point
// to a string, where each character in the string indicates a delimiter.
//
// Comments on the return value:
//
// If skipEmptyFields is true, and the function returns a non-NULL value,
// this means a token has definitely been found.
//
// If skipEmptyFields is false, and the function returns a non-NULL value,
// what was returned could either be a token or an empty field.  Check the
// value of the foundEmptyField argument to determine which.
//
// If a NULL value is ever returned, that means no token was found and there
// are no more tokens in the string.

char* VRLinkSimulatorInterface_Impl::GetTokenHelper(
    bool skipEmptyFields,
    bool& foundEmptyField,
    bool& overflowed,
    char* token,
    unsigned tokenBufSize,
    unsigned& numBytesToCopyOrCopied,
    const char*& src,
    const char* delimiters)
{
    ERROR_Assert(
        (src != NULL),
        "Source string cannot be NULL");

    ERROR_Assert(
        (delimiters != NULL),
        "Delimiters cannot be NULL");

    unsigned numDelimiters = strlen(delimiters);

    ERROR_Assert(
        (numDelimiters > 0),
        "Number of delimiters must be greater than zero");

    foundEmptyField = false;
    overflowed = false;

    if (src[0] == 0)
    {
        numBytesToCopyOrCopied = 0;
        return NULL;
    }

    // Pointer p will point to token after if statement.

    const char* p = NULL;

    if (skipEmptyFields)
    {
        // Skip past empty fields to find the first token.

        p = src;

        while (1)
        {
            unsigned i;
            for (i = 0; i < numDelimiters; i++)
            {
                if (*p == delimiters[i]) { break; }
            }

            if (i == numDelimiters)
            {
                // Token found.
                // (since the current character is not a delimiter)

                break;
            }

            p++;

            if (*p == 0)
            {
                // Couldn't find a token.

                numBytesToCopyOrCopied = 0;
                return NULL;
            }
        }//while//

        // p now points to token.
    }
    else
    {
        // Determine if there is an empty field.

        unsigned i;
        for (i = 0; i < numDelimiters; i++)
        {
            if (src[0] == delimiters[i]) { break; }
        }

        if (i < numDelimiters)
        {
            // Found an empty field.

            foundEmptyField = true;

            char* emptyField = (char*) src;
            src = &src[1];
            numBytesToCopyOrCopied = 0;

            if (token != NULL)
            {
                if (tokenBufSize > 0) { token[0] = 0; }
                return token;
            }
            else
            {
                return emptyField;
            }
        }//if//

        // There is a token.  Set p to point to token.

        p = src;
    }

    // Token has been found, and p points to it.

    const char* tokenStart = p;

    // Look for right-side delimiters.
    // After the while statement, p will point to src's null terminator or
    // a delimiter, whichever occurs first.

    while (1)
    {
        p++;

        if (*p == 0)
        {
            // Found end of string.
            // Set src to point to null terminator.

            src = p;
            break;
        }

        unsigned i;
        for (i = 0; i < numDelimiters; i++)
        {
            if (*p == delimiters[i]) { break; }
        }

        if (i < numDelimiters)
        {
            // Found delimiter on right side of token.
            // Set src to point to remainder of string (starting from the
            // byte after the first delimiter).

            src = p + 1;
            break;
        }
    }//while//

    // p points to src's null terminator or a delimiter, whichever occurred
    // first.  (p - 1 points to the last character of the token.)

    if (token != NULL)
    {
        if (tokenBufSize <= 1)
        {
            if (tokenBufSize == 1) { token[0] = 0; }
            numBytesToCopyOrCopied = 0;
            return token;
        }

        unsigned numBytesToCopy = p - tokenStart;

        if (numBytesToCopy > tokenBufSize - 1)
        {
            overflowed = true;

            numBytesToCopy = tokenBufSize - 1;
        }

        memcpy(token, tokenStart, numBytesToCopy);
        token[numBytesToCopy] = 0;

        numBytesToCopyOrCopied = numBytesToCopy;

        return token;
    }
    else
    {
        numBytesToCopyOrCopied = p - tokenStart;
        return (char*) tokenStart;
    }//if//
}

void VRLinkSimulatorInterface_Impl::RandomSetSeed(
    RandomSeed seed,
    UInt32 globalSeed,
    UInt32 nodeId,
    UInt32 protocolId,
    UInt32 instanceId)
{
    RANDOM_SetSeed(seed, globalSeed, nodeId, protocolId, instanceId);
}

double VRLinkSimulatorInterface_Impl::RandomErand(RandomSeed seed)
{
    return RANDOM_erand(seed);
}

EXTERNAL_Interface* VRLinkSimulatorInterface_Impl::GetExternalInterfaceForVRLink(
    Node* node)
{
    return node->partitionData->interfaceTable[EXTERNAL_VRLINK];
}

void VRLinkSimulatorInterface_Impl::SetExternalReceiveDelay(
                    EXTERNAL_Interface *iface,
                    clocktype delay)
{
    EXTERNAL_SetReceiveDelay(iface, delay);
}

void VRLinkSimulatorInterface_Impl::SetExternalSimulationEndTime(
                    EXTERNAL_Interface* iface,
                    clocktype duration)
{
    EXTERNAL_SetSimulationEndTime(iface, duration);
}

clocktype VRLinkSimulatorInterface_Impl::QueryExternalTime(
    EXTERNAL_Interface *iface)
{
    return EXTERNAL_QueryExternalTime(iface);
}

clocktype VRLinkSimulatorInterface_Impl::QueryExternalSimulationTime(
    EXTERNAL_Interface *iface)
{
    return EXTERNAL_QuerySimulationTime(iface);
}

// /**
// FUNCTION :: RemoteExternalSendMessage
// PURPOSE :: Send a message to the external interface on a different
//            partition. This function makes it possible for your external
//            interface to send a message to your external interface that is
//            on a different/remote partition. You will then need to add
//            your message handler into the function EXTERNAL_ProcessEvent().
//            Lastly, you can request a best-effort delivery of your message
//            to the remote external interface by passing in a delay value of
//            0 and a scheduling type of EXTERNAL_SCHEDULE_LOOSELY. Be aware
//            that best-effort messages may be scheduled at slightly
//            different simulation times each time your run your simulation.
//            Further notes about scheduling. If your external event won't
//            result in additional qualnet events, except those that will be
//            scheduled after safe time, then you can use LOOSELY. If, your
//            event is going to schedule additional qualnet event though,
//            then you must use EXTERNAL_SCHEDULE_SAFE (so that the event is
//            delayed to the next safe time). If you violate safe time you
//            will get assertion failures for safe time of signal receive
//            time.
// PARAMETERS::
// + iface : EXTERNAL_Interface* : Your external interface
// + destinationPartitionId : int : The partitionId that you want to send to
// + msg : Message * : The external message to send
// + delay : clocktype : When the message should be scheduled on the remote
//                       partion. Make certain that if you don't use Loose
//                       scheduling that you don't violate safe time.
// + scheduling : ExternalScheduleType : Whether this event can be executed
//                loosely.
// RETURN :: void : None.
// **/
void VRLinkSimulatorInterface_Impl::RemoteExternalSendMessage(
    EXTERNAL_Interface* iface,
    int destinationPartitionId,
    Message* msg,
    clocktype delay,
    ExternalScheduleType scheduling)
{
    EXTERNAL_MESSAGE_RemoteSend(
        iface,
        destinationPartitionId,
        msg,
        delay,
        scheduling);
}

void VRLinkSimulatorInterface_Impl::
ChangeExternalNodePositionOrientationAndVelocityAtTime(
        EXTERNAL_Interface *iface,
        Node *node,
        clocktype mobilityEventTime,
        double c1,
        double c2,
        double c3,
        short azimuth,
        short elevation,
        double c1Speed,
        double c2Speed,
        double c3Speed)
{
    EXTERNAL_ChangeNodePositionOrientationAndVelocityAtTime(
        iface,
        node,
        mobilityEventTime,
        c1,
        c2,
        c3,
        azimuth,
        elevation,
        c1Speed,
        c2Speed,
        c3Speed);
}
// /**
// FUNCTION :: GetExternalPhyTxPower
// PURPOSE :: Returns the physical layer transmission power on the node
//            where node might be on a remote partition.
// PARAMETERS::
// + node : Node * : Pointer to node.
// + phyIndex : int : Phy index.
// + newTxPower : double : New tx power - see PHY_SetTransmitPower ()
// RETURN :: void : NULL.
// **/
void VRLinkSimulatorInterface_Impl::GetExternalPhyTxPower(
    Node* node,
    int phyIndex,
    double* txPowerPtr)
{
    EXTERNAL_PHY_GetTxPower(node, phyIndex, txPowerPtr);
}

// /**
// FUNCTION :: SetExternalPhyTxPower
// PURPOSE :: Assign a num physical layer transmission power on the node
//            where node might be on a remote partition.
// PARAMETERS::
// + node : Node * : Pointer to node (local or proxy) that should be changed
// + phyIndex : int : Phy index.
// + newTxPower : double : New tx power - see PHY_SetTransmitPower ()
// RETURN :: void : NULL.
// **/
void VRLinkSimulatorInterface_Impl::SetExternalPhyTxPower(
    Node* node,
    int phyIndex,
    double newTxPower)
{
    EXTERNAL_PHY_SetTxPower(node, phyIndex, newTxPower);
}

// /**
// FUNCTION :: PrintClockInSecond
// PURPOSE :: Print a clocktype value in second.The result is copied to a
//            string in seconds.
// PARAMETERS  ::
// + clock          : clocktype : Time in clocktype
// + stringInSecond : char * : string containing time in seconds
// RETURN :: void : NULL.
// **/
void VRLinkSimulatorInterface_Impl::PrintClockInSecond(
    clocktype clock,
    char stringInSecond[])
{
    TIME_PrintClockInSecond(clock, stringInSecond);
}

// /**
// FUNCTION :: GetCurrentSimTime
// PURPOSE :: Gets current simulation time of a node.
// PARAMETERS::
// + node : Node * : Pointer to node.
// RETURN :: clocktype : Current simulation time.
// **/
clocktype VRLinkSimulatorInterface_Impl::GetCurrentSimTime(Node* node)
{
    return node->getNodeTime();
}

// /**

// API       :: ReturnDuplicateMsg
// LAYER     :: ANY LAYER
// PURPOSE   :: Create a new message which is an exact duplicate
//              of the message supplied as the parameter to the function and
//              return the new message.
// PARAMETERS ::
// + node    :  Node*    : node is calling message copy
// + msg     :  Message* : message for which duplicate has to be made
// RETURN    :: Message* : Pointer to the new message
// **/
Message* VRLinkSimulatorInterface_Impl::ReturnDuplicateMsg(
    Node* node,
    const Message* msg)
{
    return MESSAGE_Duplicate(node, msg);
}

// /**
// API       :: MESSAGE_AddInfo
// PURPOSE   :: Allocate one "info" field with given info type for the
//              message. This function is used for the delivery of data
//              for messages which are NOT packets as well as the delivery
//              of extra information for messages which are packets. If a
//              "info" field with the same info type has previously been
//              allocated for the message, it will be replaced by a new
//              "info" field with the specified size. Once this function
//              has been called, MESSAGE_ReturnInfo function can be used
//              to get a pointer to the allocated space for the info field
//              in the message structure.
// PARAMETERS ::
// + node    :  Node* : node which is allocating the info field.
// + msg     :  Message* : message for which "info" field
//                         has to be allocated
// + infoSize:  int : size of the "info" field to be allocated
// + infoType:  unsigned short : type of the "info" field to be allocated.
// RETURN    :: char* : Pointer to the added info field
// **/
char* VRLinkSimulatorInterface_Impl::AddInfoInMsg(
    Node *node,
    Message *msg,
    int infoSize,
    unsigned short infoType)
{
    return MESSAGE_AddInfo(node, msg, infoSize, infoType);
}

// /**
// FUNCTION :: ConvertDoubleToClocktype
// PURPOSE :: Converts double to clocktype.
// PARAMETERS  ::
// + timeInSeconds : double : double to be converted to clocktype.
// RETURN :: clocktype : Converted double to clocktype.
// **/
clocktype VRLinkSimulatorInterface_Impl::ConvertDoubleToClocktype(
    double timeInSeconds)
{
    // 1.0 units of double is assumed to be 1 second.
    // 1 unit of clocktype is assumed to be 1 nanosecond.
    // Any fractional nanosecond in doubleValue is rounded to the nearest
    // 1 unit of clocktype, not truncated.

    return (clocktype) ((timeInSeconds * 1e9) + 0.5);
}

// /**
// FUNCTION :: ConvertClocktypeToDouble
// PURPOSE :: Converts clocktype to double.
// PARAMETERS  ::
// + timeInNS : clocktype : clocktype to be converted to double.
// RETURN :: double : Converted clocktype to double 
// **/
double VRLinkSimulatorInterface_Impl::ConvertClocktypeToDouble(clocktype timeInNS)
{
    // 1 unit of clocktype is assumed to be 1 nanosecond.
    // 1.0 units of double is assumed to be 1 second.

    return ((double) timeInNS) / 1e9;
}

void VRLinkSimulatorInterface_Impl::EXTERNAL_ActivateNode(EXTERNAL_Interface *iface,
    Node *node,
    ExternalScheduleType scheduling,
    clocktype activationEventTime)
{
    ::EXTERNAL_ActivateNode(iface, node, scheduling, activationEventTime);
}

void VRLinkSimulatorInterface_Impl::EXTERNAL_DeactivateNode(EXTERNAL_Interface *iface,
    Node *node,
    ExternalScheduleType scheduling,
    clocktype deactivationEventTime)
{
    ::EXTERNAL_DeactivateNode(iface, node, scheduling, deactivationEventTime);
}

char* VRLinkSimulatorInterface_Impl::IO_GetDelimitedToken(
    char* dst,
    const char* src,
    const char* delim,
    char** next)
{
    return ::IO_GetDelimitedToken(dst, src, delim, next);
}

void VRLinkSimulatorInterface_Impl::IO_ReadInt(
    const NodeAddress nodeId,
    const NodeAddress interfaceAddress,
    const NodeInput *nodeInput,
    const char *index,
    BOOL *wasFound,
    int *readVal)
{
   ::IO_ReadInt(
    nodeId,
    interfaceAddress,
    nodeInput,
    index,
    wasFound,
    readVal);
}

void VRLinkSimulatorInterface_Impl::IO_ReadString(
    const NodeAddress nodeId,
    const NodeAddress interfaceAddress,
    const NodeInput *nodeInput,
    const char *index,
    BOOL *wasFound,
    char *readVal)
{
    ::IO_ReadString(
    nodeId,
    interfaceAddress,
    nodeInput,
    index,
    wasFound,
    readVal);
}

void VRLinkSimulatorInterface_Impl::IO_ReadStringInstance(
    const NodeAddress nodeId,
    const NodeAddress interfaceAddress,
    const NodeInput *nodeInput,
    const char *parameterName,
    const int parameterInstanceNumber,
    const BOOL fallbackIfNoInstanceMatch,
    BOOL *wasFound,
    char *parameterValue)
{
    ::IO_ReadStringInstance(
    nodeId,
    interfaceAddress,
    nodeInput,
    parameterName,
    parameterInstanceNumber,
    fallbackIfNoInstanceMatch,
    wasFound,
    parameterValue);
}

NodeInput * VRLinkSimulatorInterface_Impl::IO_CreateNodeInput(bool allocateRouterModelInput)
{
    return ::IO_CreateNodeInput(allocateRouterModelInput);
}

void VRLinkSimulatorInterface_Impl::IO_CacheFile(NodeInput *nodeInput, const char *filename)
{
    ::IO_CacheFile(nodeInput, filename);
}

void VRLinkSimulatorInterface_Impl::IO_ReadCachedFile(
    const NodeAddress nodeId,
    const NodeAddress interfaceAddress,
    const NodeInput *nodeInput,
    const char *parameterName,
    BOOL *wasFound,
    NodeInput *parameterValue)
{
    ::IO_ReadCachedFile(nodeId,
                      interfaceAddress,
                      nodeInput,
                      parameterName,
                      wasFound,
                      parameterValue);
}
void VRLinkSimulatorInterface_Impl::EXTERNAL_hton(void* ptr, unsigned int size)
{
    ::EXTERNAL_hton(ptr, size);
}
void VRLinkSimulatorInterface_Impl::EXTERNAL_ntoh(void* ptr, unsigned int size)
{
    ::EXTERNAL_ntoh(ptr, size);
}

BOOL VRLinkSimulatorInterface_Impl::ERROR_WriteError(int   type,
                             const char* condition,
                             const char* msg,
                             const char* file,
                             int   lineno)
{
    return ::ERROR_WriteError(type, condition, msg, file, lineno);
}

const MetricLayerData &VRLinkSimulatorInterface_Impl::getMetricLayerData(int idx)
{
    return g_metricData[idx];
}

unsigned short VRLinkSimulatorInterface_Impl::GetDestNPGId(Node* node, int interfaceIndex)
{
#ifdef MILITARY_RADIOS_LIB
    return Link16GetDestNPGId(node, interfaceIndex);
#endif
    return 0;
}

int VRLinkSimulatorInterface_Impl::GetRxDataRate(Node *node, int phyIndex)
{
    return PHY_GetRxDataRate(node, phyIndex);
}

bool VRLinkSimulatorInterface_Impl::IsMilitaryLibraryEnabled()
{
    return PARTITION_IsMilitaryLibraryEnabled();
}

int VRLinkSimulatorInterface_Impl::GetCoordinateSystemType(PartitionData* p)
{
    return PARTITION_GetTerrainPtr(p)->getCoordinateSystem();
}

int VRLinkSimulatorInterface_Impl::GetNumInterfacesForNode(Node* node)
{
    return node->numberInterfaces;
}

NodeAddress VRLinkSimulatorInterface_Impl::GetInterfaceAddressForInterface(Node* node, int index)
{
    return MAPPING_GetInterfaceAddressForInterface(node, node->nodeId, index);
}

int VRLinkSimulatorInterface_Impl::GetPartitionIdForIface(EXTERNAL_Interface *iface)
{
    return iface->partition->partitionId;
}

PartitionData* VRLinkSimulatorInterface_Impl::GetPartitionForIface(EXTERNAL_Interface *iface)
{
    return iface->partition;
}

void VRLinkSimulatorInterface_Impl::GetMinAndMaxLatLon(EXTERNAL_Interface* iface,
        double& minLat, double& maxLat, double& minLon, double& maxLon)
{
    minLat = iface->partition->terrainData->getOrigin().latlonalt.latitude;
    maxLat = minLat + 
        iface->partition->terrainData->getDimensions().latlonalt.latitude;
    minLon = iface->partition->terrainData->getOrigin().latlonalt.longitude;
    maxLon = minLon + 
        iface->partition->terrainData->getDimensions().latlonalt.longitude;
}

NodeId VRLinkSimulatorInterface_Impl::GetNodeId(Node* node)
{
    return node->nodeId;
}

NodeInput* VRLinkSimulatorInterface_Impl::GetNodeInputFromIface(EXTERNAL_Interface* iface)
{
    return iface->partition->nodeInput;
}

void VRLinkSimulatorInterface_Impl::GetNodeInputDetailForIndex(NodeInput* input, int i, char** variableName, char** value)
{
    *variableName = input->variableNames[i];
    *value = input->values[i];
}

int VRLinkSimulatorInterface_Impl::GetNumLinesOfNodeInput(NodeInput* input)
{
    return input->numLines;
}

int VRLinkSimulatorInterface_Impl::GetLocalPartitionIdForNode(Node* node)
{
    return node->partitionData->partitionId;
}

int VRLinkSimulatorInterface_Impl::GetRealPartitionIdForNode(Node* node)
{
    return node->partitionId;
}

Node* VRLinkSimulatorInterface_Impl::GetFirstNodeOnPartition(EXTERNAL_Interface* iface)
{
    return iface->partition->firstNode;
}

NodePointerCollection* VRLinkSimulatorInterface_Impl::GetAllNodes(EXTERNAL_Interface* iface)
{
    return iface->partition->allNodes;
}

ExternalScheduleType VRLinkSimulatorInterface_Impl::GetExternalScheduleSafeType()
{
    return EXTERNAL_SCHEDULE_SAFE;
}

MessageEventTypesUsedInVRLink VRLinkSimulatorInterface_Impl::GetMessageEventTypesUsedInVRLink(int val)
{
    switch (val)
    {
        case MSG_EXTERNAL_VRLINK_HierarchyMobility:
            return EXTERNAL_VRLINK_HierarchyMobility;
        case MSG_EXTERNAL_VRLINK_ChangeMaxTxPower:
            return EXTERNAL_VRLINK_ChangeMaxTxPower;
        case MSG_EXTERNAL_VRLINK_AckTimeout:
            return EXTERNAL_VRLINK_AckTimeout;
        case MSG_EXTERNAL_VRLINK_SendRtss:
            return EXTERNAL_VRLINK_SendRtss;
        case MSG_EXTERNAL_VRLINK_SendRtssForwarded:
            return EXTERNAL_VRLINK_SendRtssForwarded;
        case MSG_EXTERNAL_VRLINK_StartMessengerForwarded:
            return EXTERNAL_VRLINK_StartMessengerForwarded;
        case MSG_EXTERNAL_VRLINK_CompletedMessengerForwarded:
            return EXTERNAL_VRLINK_CompletedMessengerForwarded;
        case MSG_EXTERNAL_VRLINK_InitialPhyMaxTxPowerRequest:
            return EXTERNAL_VRLINK_InitialPhyMaxTxPowerRequest;
        case MSG_EXTERNAL_VRLINK_InitialPhyMaxTxPowerResponse:
            return EXTERNAL_VRLINK_InitialPhyMaxTxPowerResponse;
        case MSG_EXTERNAL_VRLINK_CheckMetricUpdate:
            return EXTERNAL_VRLINK_CheckMetricUpdate;
        default:
            return Invalid_MessageEventType;
    }
}

int VRLinkSimulatorInterface_Impl::GetMessageEventType(MessageEventTypesUsedInVRLink val)
{
    switch (val)
    {
        case EXTERNAL_VRLINK_HierarchyMobility:
            return MSG_EXTERNAL_VRLINK_HierarchyMobility;
        case EXTERNAL_VRLINK_ChangeMaxTxPower:
            return MSG_EXTERNAL_VRLINK_ChangeMaxTxPower;
        case EXTERNAL_VRLINK_AckTimeout:
            return MSG_EXTERNAL_VRLINK_AckTimeout;
        case EXTERNAL_VRLINK_SendRtss:
            return MSG_EXTERNAL_VRLINK_SendRtss;
        case EXTERNAL_VRLINK_SendRtssForwarded:
            return MSG_EXTERNAL_VRLINK_SendRtssForwarded;
        case EXTERNAL_VRLINK_StartMessengerForwarded:
            return MSG_EXTERNAL_VRLINK_StartMessengerForwarded;
        case EXTERNAL_VRLINK_CompletedMessengerForwarded:
            return MSG_EXTERNAL_VRLINK_CompletedMessengerForwarded;
        case EXTERNAL_VRLINK_InitialPhyMaxTxPowerRequest:
            return MSG_EXTERNAL_VRLINK_InitialPhyMaxTxPowerRequest;
        case EXTERNAL_VRLINK_InitialPhyMaxTxPowerResponse:
            return MSG_EXTERNAL_VRLINK_InitialPhyMaxTxPowerResponse;
        case EXTERNAL_VRLINK_CheckMetricUpdate:
            return MSG_EXTERNAL_VRLINK_CheckMetricUpdate;
        default:
            return -1;
    }
}

int VRLinkSimulatorInterface_Impl::GetExternalLayerId()
{
    return EXTERNAL_LAYER;
}

int VRLinkSimulatorInterface_Impl::GetVRLinkExternalInterfaceType()
{
    return EXTERNAL_VRLINK;
}

clocktype VRLinkSimulatorInterface_Impl::GetCurrentTimeForPartition(EXTERNAL_Interface* iface)
{
    return iface->partition->getGlobalTime();
}

bool VRLinkSimulatorInterface_Impl::GetGuiOption(EXTERNAL_Interface* iface)
{
    return iface->partition->guiOption;
}

void* VRLinkSimulatorInterface_Impl::GetDataForExternalIface(EXTERNAL_Interface* iface)
{
    return iface->data;
}

Node* VRLinkSimulatorInterface_Impl::GetNodePtrFromOtherNodesHash(Node* node, NodeAddress nodeId, bool remote)
{
    if (remote)
    {
        return ::MAPPING_GetNodePtrFromHash(node->partitionData->remoteNodeIdHash, nodeId); 
        
    }
    else
    {
        return ::MAPPING_GetNodePtrFromHash(node->partitionData->nodeIdHash, nodeId); 
    }
}

MetricDataTypesUsedInVRLink VRLinkSimulatorInterface_Impl::GetMetricDataTypesUsedInVRLink(int i)
{
    switch (i)
    {
        case GUI_INTEGER_TYPE:
               return IntegerType;
        case GUI_DOUBLE_TYPE:
            return DoubleType;
        case GUI_UNSIGNED_TYPE:
            return UnsignedType;
        default:
            return InvalidType;
    }
}

Int32 VRLinkSimulatorInterface_Impl::GetGlobalSeed(Node* node)
{
    return node->globalSeed;
}

void VRLinkSimulatorInterface_Impl::SetMessengerPktHeader(MessengerPktHeader& header,
        unsigned short pktNum,
        clocktype initialPrDelay,
        NodeAddress srcAddr,
        NodeAddress destAddr,
        int msgId,
        clocktype lifetime,
        TransportTypesUsedInVRLink transportType,
        unsigned short destNPGId,
        MessengerTrafficTypesUsed appType,
        clocktype freq,
        unsigned int fragSize,
        unsigned short numFrags)
{
    memset(&header,0,sizeof(MessengerPktHeader));

    switch (appType)
    {
        case GeneralTraffic:
            header.appType = GENERAL;
            break;
        case VoiceTraffic:
            header.appType = VOICE_PACKET;
            break;
        default:
            break;
    }
    switch (transportType)
    {
        case Unreliable:
            header.transportType = TRANSPORT_TYPE_UNRELIABLE;
            break;
        case MAC:
            header.transportType = TRANSPORT_TYPE_MAC;
            break;
        default:
            break;
    }
    header.pktNum = pktNum;
    header.initialPrDelay = initialPrDelay;
    header.srcAddr = srcAddr;
    header.destAddr = destAddr;
    header.msgId = msgId;
    header.lifetime = lifetime;
    header.destNPGId = destNPGId;
    header.freq = freq;
    header.fragSize = fragSize;
    header.numFrags = numFrags;
}