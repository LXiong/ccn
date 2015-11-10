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

#include <stdio.h>
#include <stdlib.h>

#include "api.h"
#include "partition.h"
#include "trace.h"
#include "product_info.h"

// /**
// FUNCTION      :: TRACE_WriteXMLTraceHeader
// LAYER         ::
// PURPOSE       :: Write trace header information to the trace file
// PARAMETERS    ::
// + nodeInput  : NodeInput*    : Pointer to NodeInput
// + fp         : FILE*         : Pointer to FILE : Trace file pointer
// RETURN        ::  void : NULL
// **/

void TRACE_WriteXMLTraceHeader(NodeInput* nodeInput, FILE* fp)
{
    char buf[MAX_STRING_LENGTH];
    BOOL retVal;

    char traceBuf[TRACE_STRING_LENGTH];
    memset(traceBuf, 0, sizeof(traceBuf));
    sprintf(traceBuf, "<trace_file>\n\n");
    strcat(traceBuf, "<head>\n");

    strcat(traceBuf, "<version>");
    strcat(traceBuf, Product::GetKernelVersionString());
    strcat(traceBuf, "</version>\n");

    // print experiment name
    IO_ReadString(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "EXPERIMENT-NAME",
        &retVal,
        buf);
    if (!retVal)
    {
        ERROR_ReportWarning("EXPERIMENT-NAME unavailable,"
        " assuming default\n");
        strcpy(buf, "default");
    }
    strcat(traceBuf, "<scenario>");
    strcat(traceBuf, buf);
    strcat(traceBuf, "</scenario>\n");

    // read and print experiment comments
    strcat(traceBuf, "<comments>");

    IO_ReadString(
    ANY_NODEID,
    ANY_ADDRESS,
    nodeInput,
    "EXPERIMENT-COMMENT",
    &retVal,
    buf);

    if (!retVal)
    {
        ERROR_ReportWarning("EXPERIMENT-COMMENT not available\n");
        strcpy(buf, "Any user or system free-form comments");
    }

    if ((strlen(buf)) >= MAX_STRING_LENGTH) {
        ERROR_ReportError("EXPERIMENT-COMMENT is too Big\n");
    }

    strcat(traceBuf, buf);
    strcat(traceBuf, "</comments>\n");

    strcat(traceBuf, "</head>\n\n<body>\n\n");
    fprintf(fp, "%s", traceBuf);
    fflush(fp);
}

// /**
// FUNCTION      :: TRACE_WriteXMLTraceTail
// LAYER         ::
// PURPOSE       :: Write trace Tail information to the trace file
// PARAMETERS    ::
// + fp         : FILE*  : Pointer to FILE : Trace file pointer
// RETURN        ::  void : NULL
// **/

void TRACE_WriteXMLTraceTail(FILE* fp)
{
    fprintf(fp, "\n</body>\n\n</trace_file>");
}


// /**
// FUNCTION      :: TRACE_Initialize
// LAYER         ::
// PURPOSE       :: Initialize necessary trace information before
//                  simulation starts.
// PARAMETERS    ::
// + node       : Node* : Pointer to node, doing the packet trace
// + nodeInput  : const NodeInput* : Pointer to NodeInput
// RETURN        ::  void : NULL
// **/

void TRACE_Initialize(Node* node, const NodeInput* nodeInput)
{
    int i;
    BOOL retVal;
    BOOL traceAll = 0;
    char buf[MAX_STRING_LENGTH];

    node->traceData = (TraceData *) MEM_malloc(sizeof(TraceData));
    memset(node->traceData->xmlBuf, 0, MAX_TRACE_LENGTH);

    // Set layer tracing to TRUE by default.
    for (i = 0; i < TRACE_ALL_LAYERS; i++)
    {
        node->traceData->layer[i] = TRUE;
    }

    // set packet trace to true
    node->packetTrace = TRUE;

    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "TRACE-INCLUDED-HEADERS",
        &retVal,
        buf);

    if (retVal)
    {
        if (strcmp(buf, "NONE") == 0)
        {
            node->traceData->traceIncludedHeaders = TRACE_INCLUDED_NONE;
        }
        else if (strcmp(buf, "ALL") == 0)
        {
            node->traceData->traceIncludedHeaders = TRACE_INCLUDED_ALL;
        }
        else if (strcmp(buf, "SELECTED") == 0)
        {
            node->traceData->traceIncludedHeaders = TRACE_INCLUDED_SELECTED;
        }
        else
        {
            ERROR_ReportError("TRACE-INCLUDED-HEADERS is to be either"
                "NONE or ALL or SELECTED\n");
        }
    }
    else
    {
        // set to the default value
        node->traceData->traceIncludedHeaders = TRACE_INCLUDED_NONE;
    }
    // Get layer tracing info from configuration file.

    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "TRACE-APPLICATION-LAYER",
        &retVal,
        buf);

    if (retVal)
    {
        if (strcmp(buf, "NO") == 0)
        {
            node->traceData->layer[TRACE_APPLICATION_LAYER] = FALSE;
        }
        else if (strcmp(buf, "YES") == 0)
        {
            node->traceData->layer[TRACE_APPLICATION_LAYER] = TRUE;
        }
        else
        {
            ERROR_ReportError("TRACE-APPLICATION-LAYER is to be either"
                "NO or YES\n");
        }
    }

    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "TRACE-TRANSPORT-LAYER",
        &retVal,
        buf);

    if (retVal)
    {
        if (strcmp(buf, "NO") == 0)
        {
            node->traceData->layer[TRACE_TRANSPORT_LAYER] = FALSE;
        }
        else if (strcmp(buf, "YES") == 0)
        {
            node->traceData->layer[TRACE_TRANSPORT_LAYER] = TRUE;
        }
        else
        {
            ERROR_ReportError("TRACE-TRANSPORT-LAYER is to be either"
                "NO or YES\n");
        }
    }

    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "TRACE-NETWORK-LAYER",
        &retVal,
        buf);

    if (retVal)
    {
        if (strcmp(buf, "NO") == 0)
        {
            node->traceData->layer[TRACE_NETWORK_LAYER] = FALSE;
        }
        else if (strcmp(buf, "YES") == 0)
        {
            node->traceData->layer[TRACE_NETWORK_LAYER] = TRUE;
        }
        else
        {
            ERROR_ReportError("TRACE-NETWORK-LAYER is to be either"
                "NO or YES\n");
        }
    }

    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "TRACE-MAC-LAYER",
        &retVal,
        buf);

    if (retVal)
    {
        if (strcmp(buf, "NO") == 0)
        {
            node->traceData->layer[TRACE_MAC_LAYER] = FALSE;
        }
        else if (strcmp(buf, "YES") == 0)
        {
            node->traceData->layer[TRACE_MAC_LAYER] = TRUE;
        }
        else
        {
            ERROR_ReportError("TRACE-MAC-LAYER is to be either"
                "NO or YES\n");
        }
    }

    // Now determine with direction we want to trace...

    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "TRACE-DIRECTION",
        &retVal,
        buf);

    if (retVal)
    {
        if (strcmp(buf, "INPUT") == 0)
        {
            node->traceData->direction = TRACE_DIRECTION_INPUT;
        }
        else if (strcmp(buf, "OUTPUT") == 0)
        {
            node->traceData->direction = TRACE_DIRECTION_OUTPUT;
        }
        else if (strcmp(buf, "BOTH") == 0)
        {
            node->traceData->direction = TRACE_DIRECTION_BOTH;
        }
        else
        {
            ERROR_ReportError("TRACE-DIRECTION must be either"
                "INPUT or OUTPUT or BOTH\n");
        }
    }
    else
    {
         // Default is both direction
        node->traceData->direction = TRACE_DIRECTION_BOTH;
    }

    // See if we need to trace all protocols

    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "TRACE-ALL",
        &retVal,
        buf);

    if (retVal)
    {
        if (strcmp(buf, "YES") == 0)
        {
            traceAll = TRUE;
        }
        else if (strcmp(buf, "NO") == 0)
        {
            traceAll = FALSE;
        }
        else
        {
            ERROR_ReportError("TRACE-ALL should be either YES or NO\n");
        }
    }
    else
    {
        // default value is false
        traceAll = FALSE;
    }

    node->traceData->traceAll = traceAll;

    // Since individual protocol trace setting is read in
    // respective trace init, set here to default values

    for (i = 0; i < TRACE_ANY_PROTOCOL; i++)
    {
        node->traceData->traceList[i] = FALSE;
        node->traceData->xmlPrintFn[i] = NULL;
        node->traceData->xmlPrintFun[i] = NULL;
    }
}

// /**
// FUNCTION      :: TRACE_IsTraceAll
// LAYER         ::
// PURPOSE       :: Determine if TRACE-ALL is enabled from
//                  configuration file.
// PARAMETERS    ::
// + node       : Node* : Pointer to node, doing the packet trace
// RETURN        :: BOOL : TRUE if TRACE-ALL is enabled, FALSE otherwise.
// **/

BOOL TRACE_IsTraceAll(Node* node)
{
    return node->traceData->traceAll;
}

// /**
// FUNCTION      :: TRACE_EnableTraceXML
// LAYER         ::
// PURPOSE       :: Enable XML trace for a particular protocol.
// PARAMETERS    ::
// + node         : Node*     : Pointer to node, doing the packet trace
// + protocol     : TraceProtocolType : protocol to enable trace for
// + protocolName : char* : name of protocol
// + xmlPrintFn   : TracePrintXMLFn : protocol callback function
// + writeMap     : BOOL :  flag to print protocol ID map
// RETURN        ::  void : NULL
// **/

void TRACE_EnableTraceXML(
    Node* node,
    TraceProtocolType protocol,
    const char* protocolName,
    TracePrintXMLFn xmlPrintFn,
    BOOL writeMap)
{
    FILE* fp = node->partitionData->traceFd;
    node->traceData->traceList[protocol] = TRUE;

    node->traceData->xmlPrintFn[protocol] = xmlPrintFn;
    if (node->partitionData->traceEnabled && writeMap)
    {
        fprintf(fp, "<protocol_map>%d %s</protocol_map>\n",
            protocol, protocolName);
        fflush(fp);
    }
}

// /**
// FUNCTION      :: TRACE_EnableTraceXMLFun
// LAYER         ::
// PURPOSE       :: Enable XML trace for a particular protocol.
// PARAMETERS    ::
// + node         : Node*     : Pointer to node, doing the packet trace
// + protocol     : TraceProtocolType : protocol to enable trace for
// + protocolName : char* : name of protocol
// + xmlPrintFn   : TracePrintXMLFn : protocol callback function
// + writeMap     : BOOL :  flag to print protocol ID map
// RETURN        ::  void : NULL
// **/

void TRACE_EnableTraceXMLFun(
    Node* node,
    TraceProtocolType protocol,
    const char* protocolName,
    TracePrintXMLFun xmlPrintFun,
    BOOL writeMap)
{
    FILE* fp = node->partitionData->traceFd;
    node->traceData->traceList[protocol] = TRUE;
    node->traceData->xmlPrintFun[protocol] = xmlPrintFun;
    if (node->partitionData->traceEnabled && writeMap)
    {
        fprintf(fp, "<protocol_map>%d %s</protocol_map>\n",
            protocol, protocolName);
        fflush(fp);
    }
}
//end

// /**
// FUNCTION      :: TRACE_DisableTraceXML
// LAYER         ::
// PURPOSE       :: Disable XML trace for a particular protocol.
// PARAMETERS    ::
// + node       : Node* : Pointer to node, doing the packet trace
// + protocol       : TraceProtocolType : protocol to enable trace for
// + protocolName   : char* : name of protocol
// + writeMap       : BOOL :  flag to print protocol ID map
// RETURN        ::  void : NULL
// **/

void TRACE_DisableTraceXML(
    Node* node,
    TraceProtocolType protocol,
    const char* protocolName,
    BOOL writeMap)
{
    FILE* fp = node->partitionData->traceFd;
    node->traceData->traceList[protocol] = FALSE;
    if (node->partitionData->traceEnabled && writeMap)
    {
        fprintf(fp, "<protocol_map>%d %s</protocol_map>\n",
            protocol, protocolName);
        fflush(fp);
    }
}

// /**
// FUNCTION   :: TRACE_WriteToBufferXML
// LAYER      ::
// PURPOSE    :: Write trace information to a buffer, which will then
//                  be printed to a file.
// PARAMETERS ::
// + node     : Node* : Pointer to node, doing the packet trace
// + buf      : char* : content to print to trace file
// RETURN     ::  void : NULL
// **/

void TRACE_WriteToBufferXML(Node* node, char* buf)
{
    int length = (int)(strlen(node->traceData->xmlBuf) + strlen(buf));
    if (length >= MAX_TRACE_LENGTH)
    {
        fprintf(node->partitionData->traceFd, "%s", node->traceData->xmlBuf);
        memset(node->traceData->xmlBuf, 0, MAX_TRACE_LENGTH);
    }

    strcat(node->traceData->xmlBuf, buf);
}

// /**
// FUNCTION   :: TRACE_PrintTraceXML
// LAYER      ::
// PURPOSE    :: Print XML trace information to file.
// PARAMETERS ::
// + node       : Node* : Pointer to node, doing the packet trace
// + message    : Message* : packet to print trace info from
// + layerType  : TraceLayerType : what layer is calling this function
// + actionData : ActionData* : more details about the packet action
// + netType    : NetworkType  more details about the packet action
// RETURN      ::  void : NULL
// **/

static
void TRACE_PrintTraceXML(Node* node,
                         Message* message,
                         ActionData* actionData,
                         NetworkType netType)
{
    int i;
    char clockStr[MAX_STRING_LENGTH];
    FILE* fp = node->partitionData->traceFd;
    int numHeaders = message->numberOfHeaders;
    int* headerProtocols = message->headerProtocols;
    BOOL* traceList = node->traceData->traceList;
    char* xmlBuf = node->traceData->xmlBuf;
    TracePrintXMLFun* xmlPrintFun = node->traceData->xmlPrintFun;
    TracePrintXMLFn* xmlPrintFn = node->traceData->xmlPrintFn;
    TraceIncludedHeadersType traceIncludedHeaders =
        node->traceData->traceIncludedHeaders;
    char hdrBuf[MAX_STRING_LENGTH];

    TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

    // Start record element.
    fprintf(fp, "\n<rec>\n");

    char tmpBuf[MAX_STRING_LENGTH];
    memset(tmpBuf, 0, sizeof(tmpBuf));

    sprintf(tmpBuf, "%hu %d %s %d %hu %d",
        message->originatingNodeId,
        message->sequenceNumber,
        clockStr,
        message->originatingProtocol,
        node->nodeId,
        headerProtocols[numHeaders - 1]);

    //  print packet action
    strcat(tmpBuf, " <action>");
    memset(hdrBuf, 0, sizeof(hdrBuf));
    sprintf(hdrBuf, " %hu", actionData->actionType);
    strcat(tmpBuf, hdrBuf);
    memset(hdrBuf, 0, sizeof(hdrBuf));

    switch (actionData->actionType)
    {
        case SEND:
        case RECV:
        case DROP:
            sprintf(hdrBuf, " %hu", actionData->actionComment);
            break;
        case ENQUEUE:
        case DEQUEUE:
            sprintf(hdrBuf, " <queue> %hu %hu</queue>",
                    actionData->pktQueue.interfaceID,
                    actionData->pktQueue.queuePriority);
            break;
        default:
            ERROR_ReportError("TRACE_PrintTraceXML: Invalid actionType\n");
    }
    strcat(tmpBuf, hdrBuf);
    strcat(tmpBuf, "</action>");
    fprintf(fp, "<rechdr> %s</rechdr>\n", tmpBuf);


    // start rec body
    fprintf(fp, "<recbody>\n");

    if (traceList[headerProtocols[numHeaders - 1]] == TRUE
        && traceIncludedHeaders == TRACE_INCLUDED_NONE)
    {
        // When not tracing any included headers,
        // simply call the protocol trace function.

        if (xmlPrintFun[headerProtocols[numHeaders - 1]] != NULL)
        {
            xmlPrintFun[headerProtocols[numHeaders - 1]](node, message,netType);

            fprintf(fp, "%s", xmlBuf);
            fprintf(fp, "\n");

            memset(xmlBuf, 0, MAX_TRACE_LENGTH);
        }
        else if (xmlPrintFn[headerProtocols[numHeaders - 1]] != NULL)
        {
            xmlPrintFn[headerProtocols[numHeaders - 1]](node, message);
            fprintf(fp, "%s", xmlBuf);
            fprintf(fp, "\n");

            memset(xmlBuf, 0, MAX_TRACE_LENGTH);
        }

        else
        {
            ERROR_ReportError(
                "TRACE_PrintTraceXML: Invalid xmlPrintFn - 1.\n");
        }
    }
    else
    {
        if (numHeaders <= 0)
        {
            ERROR_ReportError(
                "TRACE_PrintTraceXML: Number of headers <= 0.\n");
        }

        int totalHeaderSize = 0;
        for (i = 0; i < numHeaders; i++)
        {
            totalHeaderSize += message->headerSizes[i];
        }

        // Cannot call MESSAGE_RemoveHeader() as msg->numberOfHeaders
        // is decremented there.  So, we must do it this way...

        message->packet += totalHeaderSize;
        message->packetSize -= totalHeaderSize;

        for (i = 0; i < numHeaders; i++)
        {
            // Get the right message header to print.
            message->packet -= message->headerSizes[i];
            message->packetSize += message->headerSizes[i];
            if (i == 0)
            {
                if (headerProtocols[i] == headerProtocols[i+1])
                {
                    continue;
                }
            }

            // However, if we are not interested in this header,
            // then skip it.

            if (traceList[headerProtocols[i]] == FALSE
                && traceIncludedHeaders != TRACE_INCLUDED_ALL)
            {
                continue;
            }

            // Call protocol's header printing function
            if (xmlPrintFun[headerProtocols[i]] != NULL)

            {
              xmlPrintFun[headerProtocols[i]](node, message , netType);

                fprintf(fp, "%s", xmlBuf);
                fprintf(fp, "\n");

                memset(xmlBuf, 0, MAX_TRACE_LENGTH);
            }
            else if (xmlPrintFn[headerProtocols[i]] != NULL)

            {
                xmlPrintFn[headerProtocols[i]](node, message);

                fprintf(fp, "%s", xmlBuf);
                fprintf(fp, "\n");

                memset(xmlBuf, 0, MAX_TRACE_LENGTH);
            }
            else
            {
                ; // Skip protocols where trace not implemented.
            }
        } // for
    } // if

    // end of record
    fprintf(fp, "</recbody>\n</rec>\n");
    fflush(fp);
}

// /**
// FUNCTION   :: TRACE_PrintTraceXML
// LAYER      ::
// PURPOSE    :: Print XML trace information to file.
// PARAMETERS ::
// + node       : Node* : Pointer to node, doing the packet trace
// + message    : Message* : packet to print trace info from
// + layerType  : TraceLayerType : what layer is calling this function
// + actionData : ActionData* : more details about the packet action
// RETURN      ::  void : NULL
// **/

static
void TRACE_PrintTraceXML(Node* node,
                         Message* message,
                         ActionData* actionData)
{
    int i;
    char clockStr[MAX_STRING_LENGTH];
    FILE* fp = node->partitionData->traceFd;
    int numHeaders = message->numberOfHeaders;
    int* headerProtocols = message->headerProtocols;
    BOOL* traceList = node->traceData->traceList;
    char* xmlBuf = node->traceData->xmlBuf;
    TracePrintXMLFn* xmlPrintFn = node->traceData->xmlPrintFn;
    TraceIncludedHeadersType traceIncludedHeaders =
        node->traceData->traceIncludedHeaders;
    char hdrBuf[MAX_STRING_LENGTH];

    TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

    // Start record element.
    fprintf(fp, "\n<rec>\n");

    // print record header
    // <rechdr>
    // Source Node, Message Sequence No., Simulation Time,
    // Originating Protocol, Processing Node ID, Tracing Protocol ID
    // <action> actionData </action>
    // </rechdr>

    char tmpBuf[MAX_STRING_LENGTH];
    memset(tmpBuf, 0, sizeof(tmpBuf));

    sprintf(tmpBuf, "%hu %d %s %d %hu %d",
        message->originatingNodeId,
        message->sequenceNumber,
        clockStr,
        message->originatingProtocol,
        node->nodeId,
        headerProtocols[numHeaders - 1]);

    //  print packet action
    strcat(tmpBuf, " <action>");
    memset(hdrBuf, 0, sizeof(hdrBuf));
    sprintf(hdrBuf, " %hu", actionData->actionType);
    strcat(tmpBuf, hdrBuf);
    memset(hdrBuf, 0, sizeof(hdrBuf));

    switch (actionData->actionType)
    {
        case SEND:
        case RECV:
        case DROP:
            sprintf(hdrBuf, " %hu", actionData->actionComment);
            break;
        case ENQUEUE:
        case DEQUEUE:
            sprintf(hdrBuf, " <queue> %hu %hu</queue>",
                    actionData->pktQueue.interfaceID,
                    actionData->pktQueue.queuePriority);
            break;
        default:
            ERROR_ReportError("TRACE_PrintTraceXML: Invalid actionType\n");
    }
    strcat(tmpBuf, hdrBuf);
    strcat(tmpBuf, "</action>");
    fprintf(fp, "<rechdr> %s</rechdr>\n", tmpBuf);


    // start rec body
    fprintf(fp, "<recbody>\n");

    if (traceList[headerProtocols[numHeaders - 1]] == TRUE
        && traceIncludedHeaders == TRACE_INCLUDED_NONE)
    {
        // When not tracing any included headers,
        // simply call the protocol trace function.

        if (xmlPrintFn[headerProtocols[numHeaders - 1]] != NULL)
        {
            xmlPrintFn[headerProtocols[numHeaders - 1]](node, message);

            fprintf(fp, "%s", xmlBuf);
            fprintf(fp, "\n");

            memset(xmlBuf, 0, MAX_TRACE_LENGTH);
        }
        else
        {
            ERROR_ReportError(
                "TRACE_PrintTraceXML: Invalid xmlPrintFn - 1.\n");
        }
    }
    else if (numHeaders == 0 &&
        (message->protocolType == APP_GEN_FTP_SERVER))
    {
        if (traceList[TRACE_GEN_FTP] == TRUE) {
            xmlPrintFn[TRACE_GEN_FTP](node, message);

        }
        fprintf(fp, "%s", xmlBuf);
        fprintf(fp, "\n");

        memset(xmlBuf, 0, MAX_TRACE_LENGTH);
        fprintf(fp, "</recbody>\n</rec>\n");
        fflush(fp);
        return;

    }

    else
    {
        if (numHeaders <= 0)
        {
            ERROR_ReportError(
                "TRACE_PrintTraceXML: Number of headers <= 0.\n");
        }

        int totalHeaderSize = 0;
        for (i = 0; i < numHeaders; i++)
        {
            totalHeaderSize += message->headerSizes[i];
        }

        // Cannot call MESSAGE_RemoveHeader() as msg->numberOfHeaders
        // is decremented there.  So, we must do it this way...

        message->packet += totalHeaderSize;
        message->packetSize -= totalHeaderSize;

        for (i = 0; i < numHeaders; i++)
        {
            // Get the right message header to print.
            message->packet -= message->headerSizes[i];
            message->packetSize += message->headerSizes[i];
            if (i == 0)
            {
                if (headerProtocols[i] == headerProtocols[i+1])
                {
                    continue;
                }
            }
            // However, if we are not interested in this header,
            // then skip it.

            if (traceList[headerProtocols[i]] == FALSE
                && traceIncludedHeaders != TRACE_INCLUDED_ALL)
            {
                continue;
            }

            // Call protocol's header printing function
            if (xmlPrintFn[headerProtocols[i]] != NULL)
            {
                xmlPrintFn[headerProtocols[i]](node, message);

                fprintf(fp, "%s", xmlBuf);
                fprintf(fp, "\n");

                memset(xmlBuf, 0, MAX_TRACE_LENGTH);
            }
            else
            {
                ; // Skip protocols where trace not implemented.
            }
        } // for
    } // if

    // end of record
    fprintf(fp, "</recbody>\n</rec>\n");
    fflush(fp);
}

// /**
// FUNCTION   :: TRACE_PrintTrace
// LAYER      ::
// PURPOSE    :: Print XML trace information to file.
// PARAMETERS ::
// + node         : Node* : Pointer to node, doing the packet trace
// + message      : Message* : packet to print trace info from
// + layerType    : TraceLayerType : what layer is calling this function
// + pktDirection : PacketDirection : in which direction trace will be done
// + actionData   : ActionData* : more details about the packet action
// RETURN         ::  void : NULL
// **/

void TRACE_PrintTrace(Node* node,
                      Message* message,
                      TraceLayerType layerType,
                      PacketDirection pktDirection,
                      ActionData* actionData)
{
    BOOL found = FALSE;
    int i;
    // If tracing is disabled, don't print anything to trace
    if (!node->partitionData->traceEnabled)
    {
        return;
    }

    // Trace only direction specified...
    // OUTPUT or INPUT tracing implies packet output from a node or
    // packet input to a node as a whole(not layer specific)

    TraceDirectionType traceDir = node->traceData->direction;
    if (traceDir != TRACE_DIRECTION_BOTH)
    {
        if (traceDir == TRACE_DIRECTION_OUTPUT)
        {
            if (pktDirection != PACKET_OUT)
            {
                return;
            }
        }
        else if (traceDir == TRACE_DIRECTION_INPUT)
        {
            if (pktDirection != PACKET_IN)
            {
                return;
            }
        }
        else
        {
            ERROR_ReportError("Invalid Trace Direction\n");
        }
    }

    // start printTrace XML

    // If tracing protocol is not selected and
    // included protocols are not output, return.

   if (message->numberOfHeaders == 0 &&
         message->protocolType == APP_GEN_FTP_SERVER &&
         node->traceData->traceList[TRACE_GEN_FTP] == TRUE) {
        TRACE_PrintTraceXML(node, message, actionData);
        return;
    }


    int traceProto = message->headerProtocols[message->numberOfHeaders - 1];
    if (node->traceData->traceList[traceProto] == FALSE && node->traceData
        ->traceIncludedHeaders != TRACE_INCLUDED_SELECTED)
    {
        return;
    }

    // Check to see if there's anything to print...
    for (i = 0; i < message->numberOfHeaders; i++)
    {
        if (node->traceData->traceList[message->headerProtocols[i]] == TRUE
            && node->traceData->xmlPrintFn[message->headerProtocols[i]]
                    != NULL)
        {
            found = TRUE;
            break;
        }
    }

    // If nothing to print, then don't do anything.
    if (!found)
    {
        return;
    }
    TRACE_PrintTraceXML(node, message, actionData);
}

// /**
// FUNCTION   :: TRACE_PrintTrace
// LAYER      ::
// PURPOSE    :: Print XML trace information to file.
// PARAMETERS ::
// + node         : Node* : Pointer to node, doing the packet trace
// + message      : Message* : packet to print trace info from
// + layerType    : TraceLayerType : what layer is calling this function
// + pktDirection : PacketDirection : in which direction trace will be done
// + actionData   : ActionData* : more details about the packet action
// +
// RETURN         ::  void : NULL
// **/

void TRACE_PrintTrace(Node* node,
                      Message* message,
                      TraceLayerType layerType,
                      PacketDirection pktDirection,
                      ActionData* actionData,
                      NetworkType netType)
{
    BOOL found = FALSE;
    int i;

    // If tracing is disabled, don't print anything to trace
    if (!node->partitionData->traceEnabled)
    {
        return;
    }

    // Trace only direction specified...
    // OUTPUT or INPUT tracing implies packet output from a node or
    // packet input to a node as a whole(not layer specific)

    TraceDirectionType traceDir = node->traceData->direction;
    if (traceDir != TRACE_DIRECTION_BOTH)
    {
        if (traceDir == TRACE_DIRECTION_OUTPUT)
        {
            if (pktDirection != PACKET_OUT)
            {
                return;
            }
        }
        else if (traceDir == TRACE_DIRECTION_INPUT)
        {
            if (pktDirection != PACKET_IN)
            {
                return;
            }
        }
        else
        {
            ERROR_ReportError("Invalid Trace Direction\n");
        }
    }

    // start printTrace XML

    // If tracing protocol is not selected and
    // included protocols are not output, return.

    int traceProto = message->headerProtocols[message->numberOfHeaders - 1];
    if (node->traceData->traceList[traceProto] == FALSE && node->traceData
        ->traceIncludedHeaders != TRACE_INCLUDED_SELECTED)
    {
        return;
    }

    // Check to see if there's anything to print...
    for (i = 0; i < message->numberOfHeaders; i++)
    {
        if (node->traceData->traceList[message->headerProtocols[i]] == TRUE
            && ((node->traceData->xmlPrintFun[message->headerProtocols[i]]
            != NULL) ||
            (node->traceData->xmlPrintFn[message->headerProtocols[i]]
            != NULL)))
        {
            found = TRUE;
            break;
        }
    }

    // If nothing to print, then don't do anything.
    if (!found)
    {
        return;
    }
    TRACE_PrintTraceXML(node, message, actionData, netType);
}
