#include "api.h"
#include "partition.h"
#include "node.h"
#include "phy_connectivity.h"
#include "mac_satcom.h"
#include "mac_switched_ethernet.h"
#include "mac_802_3.h"
#include "propagation.h"
#include "phy.h"

#ifdef ADDON_DB
#include "dbapi.h"
#endif

#ifdef ADDON_BOEINGFCS
#include "mac_ces_wintncw.h"
#include "phy_srw_abstract.h"
#endif

#ifdef WIRELESS_LIB
#include "antenna.h"
#include "antenna_switched.h"
#include "antenna_steerable.h"
#include "antenna_patterned.h"
#include "phy_802_11.h"
#include "phy_abstract.h"
#endif // WIRELESS_LIB

#ifdef SENSOR_NETWORKS_LIB
#include "phy_802_15_4.h"
#endif //SENSOR_NETWORKS_LIB

#ifdef CELLULAR_LIB
#include "phy_gsm.h"
#endif // CELLULAR_LIB

#ifdef MILITARY_RADIOS_LIB
#include "phy_fcsc.h"
#endif /* MILITARY_RADIOS_LIB */

#ifdef ADDON_LINK16
#include "link16_phy.h"
#endif // ADDON_LINK16

#ifdef ADVANCED_WIRELESS_LIB
#include "phy_dot16.h"
#endif // ADVANCED_WIRELESS_LIB

#ifdef LTE_LIB
#include "phy_lte.h"
#endif // LTE_LIB

#ifdef USE_MPI
#include <mpi.h>
#endif /* USE_MPI */

#include "qualnet_mutex.h"
#define PHY_CONNECTIVITY_EVENT 4000

#define HUGE_PATHLOSS_VALUE  10000.0
#define PHY_CONNECTIVITY_DEBUG 0
static QNPartitionMutex *phyConnLock = new QNPartitionMutex();

#ifndef USE_MPI
vector<PHY_CONN_NodePositionData::PER_PARTITION_SATCOM_NODELIST *> SatcomNodeList;
vector<PHY_CONN_NodePositionData::PER_PARTITION_LINK_NODELIST *> LinkNodeList;
vector<PHY_CONN_NodePositionData::PER_PARTITION_SATCOM_NODELIST *> EthernetNodeList;
#else
void PHY_CONN_ProcessSatComAllNodeMsg(
    PartitionData * partition, UInt8 *info,
    PHY_CONN_NodePositionData::PER_PARTITION_SATCOM_NODELIST &);
void PHY_CONN_ProcessLinkAllNodeMsg(
    PartitionData * partition, UInt8 *info);
#endif


void HandlePhyConnInsertion(Node* node,
    PartitionData* partition);

void PHY_CONN_NodePositionData::GetChannelNodeInfo(
    NodeId nodeId,
    int phyIndex,
    int channelIndex,
    PHY_CONN_NodePositionData::ListenableSet::iterator* info,
    BOOL* found)
{
    // Lookup channel
    PHY_CONN_NodePositionData::ListenableChannelNodeList::iterator
        it = this->listenableChannelNodeList.find(channelIndex);
    if (it == this->listenableChannelNodeList.end())
    {
        *found = FALSE;
        return;
    }

    // Lookup transmitter
    PHY_CONN_NodePositionData::ListenableSet::iterator it2;
    it2 = it->second.find(
        PHY_CONN_NodePositionData::ChannelNodeInfo(nodeId, phyIndex));
    if (it2 == it->second.end())
    {
        *found = FALSE;
    }
    else
    {
        *info = it2;
        *found = TRUE;
    }
}

void PHY_CONN_NodePositionData::GetListeningInfo(
            NodeId nodeId,
            int phyIndex,
            int channelIndex,
            BOOL* found)
{
    // Lookup channel
    PHY_CONN_NodePositionData::ListeningChannelNodeList::iterator
        it = this->listeningChannelNodeList.find(channelIndex);
    if (it == this->listeningChannelNodeList.end())
    {
        *found = FALSE;
        return;
    }

    // Lookup transmitter
    PHY_CONN_NodePositionData::ListeningSet::iterator it2;
    it2 = it->second.find(
        PHY_CONN_NodePositionData::ListeningChannelNodeInfo(nodeId, phyIndex));
    if (it2 == it->second.end())
    {
        *found = FALSE;
    }
    else
    {
        *found = TRUE;
    }
}

void PHY_CONN_NodePositionData::ConnectivityMap::AddDuplicateKey(
    TxConnectivity& summary,
    const TxConnectivity& newSummary)
{
    // Update summary with reachable/reachableWorst from newSummary
    // Start by looping over all nodes
    for (TxConnectivity::const_iterator it = newSummary.begin();
        it != newSummary.end();
        it++)
    {
        // Loop over adjacencies
        for (AdjacencyList::const_iterator it2 = it->second.begin();
            it2 != it->second.end();
            it2++)
        {

            if (it2->second.syncType ==
                global_syncType &&
                it2->second.syncType == RX_POWER_INFO)
            {
                if (it2->second.rxPower_mw || it2->second.rxPower_mwWorst)
                {
                    AdjacencyList::iterator it3;

                    // Search for matching summary, then update
                    for (it3 = summary[it->first].begin();
                        it3 != summary[it->first].end();
                        it3++)
                    {
                        if (it3->first.rcverId == it2->first.rcverId
                            && it3->first.channelIndex == it2->first.channelIndex)
                        {
                            it3->second.rxPower_mw = it2->second.rxPower_mw;
                            it3->second.rxPower_mwWorst = it2->second.rxPower_mwWorst;
                            break;
                        }
                    }
                }
            }
            else if (it2->second.syncType ==
                global_syncType &&
                it2->second.syncType == PATHLOSS)
            {
                summary[it2->first.rcverId].insert(*it2);
            }
            else if (
                global_syncType == CONNECTIVITY) // exchange connectivity info
            {
                // If reachable, update summary
                if (it2->second.reachable || it2->second.reachableWorst)
                {
                    AdjacencyList::iterator it3;

                    // Search for matching summary, then update
                    for (it3 = summary[it->first].begin();
                        it3 != summary[it->first].end();
                        it3++)
                    {
                        if (it3->first.rcverId == it2->first.rcverId
                            && it3->first.channelIndex == it2->first.channelIndex)
                        {
                            it3->second.reachable = it2->second.reachable;
                            it3->second.reachableWorst = it2->second.reachableWorst;

                            break;
                        }
                    }
                }
            }
        }
    }
}

#ifdef USE_MPI
UInt32 PHY_CONN_NodePositionData::ConnectivityMap::SummarySize(
    const TxConnectivity* summary)
{
    // Format is 4 byte size of list + Connectivity list per node
    // Size is 4 + sizeof(AdjancencyNode) * adjacencies
    // Note that each AdjacencyNode has receiverId
    int adjacencies = 0;
    for (TxConnectivity::const_iterator it = summary->begin();
        it != summary->end();
        it++)
    {
        adjacencies += it->second.size();
    }

    return 4 + (sizeof(AdjacencyNodeKey) + sizeof(AdjacencyNodeValue))* adjacencies;
}

void PHY_CONN_NodePositionData::ConnectivityMap::SerializeSummary(
    const TxConnectivity* summary,
    UInt8* out)
{
    UInt8* outOrig = out;

    // Pointer for adjacencies, updated as we go through
    UInt32 *adjacencies = (UInt32*) out;
    *adjacencies = 0;
    out += sizeof(UInt32);

    // Loop over receivers for this sender
    for (TxConnectivity::const_iterator it = summary->begin();
        it != summary->end();
        it++)
    {
        *adjacencies += it->second.size();

        // Loop over adjacencies for this sender/receiver pair
        for (AdjacencyList::const_iterator it2 = it->second.begin();
            it2 != it->second.end();
            it2++)
        {
            memcpy(out, &(it2->first), sizeof(AdjacencyNodeKey));
            out += sizeof(AdjacencyNodeKey);

            memcpy(out, &(it2->second), sizeof(AdjacencyNodeValue));
            out += sizeof(AdjacencyNodeValue);
        }
    }

}

void PHY_CONN_NodePositionData::ConnectivityMap::DeserializeSummary(
    TxConnectivity* summary,
    const UInt8* in)
{
    const UInt8* origIn = in;

    // Number of adjacencies
    UInt32 adjacencies = *((UInt32*) in);
    in += sizeof(UInt32);

    // Loop over adjacencies for this sender/receiver pair
    for (int j = 0; j < adjacencies; j++)
    {
        AdjacencyNodeKey* a = (AdjacencyNodeKey*) in;
        in += sizeof(AdjacencyNodeKey);

        AdjacencyNodeValue* b = (AdjacencyNodeValue*) in;
        in += sizeof(AdjacencyNodeValue);

        // Add to map
        (*summary)[a->rcverId].insert(PairAdjacencyKeyValue(*a, *b));
    }

}
#endif

void HandlePhyConnSenderListInsertion(PartitionData *partition,
    int senderId, int senderIndex,
    int rcverId, int rcverIndex,
    PHY_CONN_NodePositionData::InterfaceType,
    PHY_CONN_NodePositionData::InterfaceType);

template <class T>
void PHY_CONN_ClearPtrVector(Node *node, T & input_vector)
{

    int i = 0;
    while (i != input_vector.size())
    {
        MESSAGE_Free(node, input_vector[i]);
        ++i;
    }
    input_vector.clear ();
}

#ifdef USE_MPI
void HandleMPIMessageExchange(Node *node, PartitionData* partition)
{
    int i, j;
    int numPartRecved = 0;

    if (partition->getNumPartitions() == 1)
    {
        return;
    }

    int size = partition->nodePositionData->snd_msgList.size();
    if (PHY_CONNECTIVITY_DEBUG)
    {
        printf("partition %d snd_msgList.size = %d \n",
            partition->partitionId, size);
    }

    // send the size packet.
    Message* msg =
        MESSAGE_Alloc(partition->firstNode,
                     PHY_LAYER,
                     0,
                     MSG_PHY_CONN_CrossPartitionSize);

    MESSAGE_SetInstanceId(msg, partition->partitionId);
    MESSAGE_InfoAlloc(node, msg, sizeof(int));
    *((int*)MESSAGE_ReturnInfo(msg)) = size;

    partition->nodePositionData->snd_sizeMsgList.push_back(msg);

    for (i = 0; i <
        partition->nodePositionData->snd_sizeMsgList.size(); ++i)
    {
        PARALLEL_SendMessageToAllPartitions(
            partition->nodePositionData->snd_sizeMsgList[i],partition);
    }

    if (PHY_CONNECTIVITY_DEBUG)
    {
        numPartRecved =
                partition->nodePositionData->size_msgList.size();
        printf("partition %d size-msg finished numPartRecved = %d\n", partition->partitionId,
            numPartRecved); fflush(stdout);
    }

    while (1)
    {
        numPartRecved =
            partition->nodePositionData->size_msgList.size();
        if (numPartRecved == partition->getNumPartitions() - 1)
        {
            break;
        }
        //printf("partition %d additional loop since numPartRecved = %d \n",
        //    partition->partitionId, numPartRecved); fflush(stdout);

        PARALLEL_GetRemoteMessages(partition);
    }
    partition->nodePositionData->snd_sizeMsgList.clear();

    int numMsgRecved = partition->nodePositionData->msgList.size();
    int numMsgExpected = 0;
    int numMsgSent = 0;
    for (i = 0; i < partition->nodePositionData->size_msgList.size(); ++i)
    {
        numMsgExpected += *((int*)MESSAGE_ReturnInfo(
            partition->nodePositionData->size_msgList[i]));
    }
    if (PHY_CONNECTIVITY_DEBUG) {
        printf("partition %d numPartRcvd %d numMsgRcvd %d numMsgExp %d \n",
            partition->partitionId,
            numPartRecved, numMsgRecved, numMsgExpected); fflush(stdout);
    }

    for (j = 0; j < partition->nodePositionData->snd_msgList.size();
                ++j)
    {
        // send the msgs
        PARALLEL_SendMessageToAllPartitions(
            partition->nodePositionData->snd_msgList[j],partition);
        ++numMsgSent;
        if (PHY_CONNECTIVITY_DEBUG) {
            printf("time %f partition %d snd_msgList.size %d j %d sending msg to all partitions\n",
                (double)partition->getGlobalTime()/SECOND,
                partition->partitionId,
                partition->nodePositionData->snd_msgList.size(), j); fflush(stdout);
        }
    }
    partition->nodePositionData->snd_msgList.clear();

    if (PHY_CONNECTIVITY_DEBUG)
    {
        printf("partition %d all msgs are fnished sending "
            "numMsgSent = %d, "
            "numPartRecved = %d, numMsgExpected = %d, "
            "numMsgRecved = %d\n", partition->partitionId,
            numMsgSent,
            numPartRecved, numMsgExpected, numMsgRecved); fflush(stdout);
    }
    while (1)
    {
        numMsgRecved = partition->nodePositionData->msgList.size();
        numMsgExpected = 0;
        for (i = 0; i < partition->nodePositionData->size_msgList.size(); ++i)
        {
            numMsgExpected += *((int*)MESSAGE_ReturnInfo(
                partition->nodePositionData->size_msgList[i]));
        }
        if (numPartRecved == partition->getNumPartitions() - 1)
        {
            if (numMsgExpected == numMsgRecved){
                break;
            }
        }

        PARALLEL_GetRemoteMessages(partition);
    }
    if (PHY_CONNECTIVITY_DEBUG)
    {
        printf("partition %d numPartRcvd %d numMsgRcvd %d numMsgExp %d \n",
            partition->partitionId, numPartRecved, numMsgRecved, numMsgExpected); fflush(stdout);
    }

    for (i = 0; i < partition->nodePositionData->msgList.size(); ++i)
    {
        int type = -1;

        UInt8 *info = (UInt8 *)MESSAGE_ReturnInfo(
            partition->nodePositionData->msgList[i] ,
            MSG_PHY_CONN_CrossPartitionInfoField);
        memcpy(&type,
            info, sizeof(int));
        if (PHY_CONNECTIVITY_DEBUG)
        {
            printf("partition %d %d-th msg type == %d msgListSize = %d\n",
                partition->partitionId,
                i, type,
                partition->nodePositionData->msgList.size()); fflush(stdout);
        }
        switch(type)
        {
        case PHY_CONN_NodePositionData::LINK_NODE_INFO:
            PHY_CONN_ProcessLinkAllNodeMsg(partition, info);
            break;
        case PHY_CONN_NodePositionData::SATCOM_NODE_INFO:
            PHY_CONN_ProcessSatComAllNodeMsg(partition, info,
                partition->nodePositionData->PerPartitionSatcomNodeList);
            break;
        case PHY_CONN_NodePositionData::ETHERNET_NODE_INFO:
            printf("p%d handle ethernet node info list size %d\n",
                partition->partitionId,
                partition->nodePositionData->PerPartition802_3NodeList.size());
            PHY_CONN_ProcessSatComAllNodeMsg(partition, info,
                partition->nodePositionData->PerPartition802_3NodeList);
            break;
        default:
            ERROR_Assert(FALSE, "Error in HandleMPIMessageExchange.");
        }
    }

    PHY_CONN_ClearPtrVector(partition->firstNode,
        partition->nodePositionData->msgList);
    PHY_CONN_ClearPtrVector(partition->firstNode,
        partition->nodePositionData->size_msgList);
}
#endif
void PHY_CONN_CalculateConnectivity(PartitionData* partition)
{
    PHY_CONN_CollectChannelPositionInfo(partition);
    PHY_CONN_SendSatComNodeMsg(partition);
    PHY_CONN_SendLinkNodeMsg(partition);
    PHY_CONN_Send802_3NodeMsg(partition);

#ifdef USE_MPI
    // mpi msg exchange handling
    HandleMPIMessageExchange(partition->firstNode, partition);
#endif

    HandlePhyConnInsertion(partition->firstNode, partition);

#ifdef ADDON_STATS_MANAGER
    StatsManager_HandlePhyConnectivity(
        partition->firstNode, partition, NULL);
#endif


}

void PHY_CONN_CollectConnectSample(PartitionData* partition)
{
    PHY_CONN_CalculateConnectivity(partition);
}



Coordinates PHY_CONN_NodePositionData::ReturnNodePosition(NodeId nodeId)
{
    NodePositionDatabase::iterator iter =
        positions.find(nodeId);

    ERROR_Assert(iter != positions.end(),
        "Error in StatsDB::ReturnNodePosition");

    return iter->second;
}

void PHY_CONN_ObtainNodesOnOtherPartitionPositions(
        PartitionData * partitionData,
        NodeInput* nodeInput)
{
    // ALlocate new class
    partitionData->nodePositionData = new PHY_CONN_NodePositionData;


    // The EXTERNAL_ package can forward between partitions. To
    // accomplish this EXTERNAL uses partition communication.

#ifndef USE_MPI
    QNPartitionLock lock(phyConnLock);
    SatcomNodeList.push_back(& partitionData->nodePositionData->PerPartitionSatcomNodeList);
    EthernetNodeList.push_back(& partitionData->nodePositionData->PerPartition802_3NodeList);
    LinkNodeList.push_back(& partitionData->nodePositionData->PerPartitionLinkNodeList);
#endif
}


void PHY_CONN_GetNodePosition(PartitionData* partition, Node* node, Coordinates* position)
{
    // If parallel get position using node map
    if (node->partitionData->isRunningInParallel())
    {
        PHY_CONN_NodePositionData* nodePositionData =
            partition->nodePositionData;
        *position = nodePositionData->ReturnNodePosition(node->nodeId);
    }
    else
    {
        MOBILITY_ReturnCoordinates(node, position);
    }
}

double PHY_CONN_ReturnNodePairDistance(
    Node * node, int coordinateSystemType,
    Node* txNode, Node * rxNode)
{
    double distance;
    Coordinates txPosition;
    Coordinates rxPosition;

    PHY_CONN_GetNodePosition(node->partitionData, txNode, &txPosition);
    PHY_CONN_GetNodePosition(node->partitionData, rxNode, &rxPosition);

    COORD_CalcDistance(
        coordinateSystemType,
        &txPosition, &rxPosition, &distance);

    return distance;
}


void SendNodePositionInformation(PartitionData* partitionData)
{
    if (partitionData->isRunningInParallel())
    {
        partitionData->nodePositionData->positions.Summarize(partitionData);
    }
}

void SendListenableChannelInformation(
    PartitionData* partitionData)
{
    if (!partitionData->isRunningInParallel())
    {
        return;
    }

    partitionData->nodePositionData->listenableChannelNodeList.Summarize(partitionData);
}

void SendListeningChannelInformation(PartitionData *partitionData)
{
    if (partitionData->isRunningInParallel())
    {
        partitionData->nodePositionData->listeningChannelNodeList.Summarize(partitionData);
    }
}

void PHY_CONN_CollectChannelPositionInfo(PartitionData * partitionData)
{
    // construct the structure
    Node * firstNode = partitionData->firstNode;

    if (PHY_CONNECTIVITY_DEBUG)
    {
        printf("partition %d currentTime %f\n", partitionData->partitionId,
            (double) partitionData->getGlobalTime() / SECOND);
    }

    for (; firstNode != NULL; firstNode = firstNode->nextNodeData)
    {
        int phyIndex, c;
        int totalChannels = PROP_NumberChannels(firstNode);
        for (phyIndex = 0; phyIndex < firstNode->numberPhys; ++phyIndex)
        {
            int l;

            //PHY_GetTransmissionChannel(firstNode, phyIndex, &l);
            for (l = 0; l < totalChannels; ++l)
            {

                if (PHY_IsListeningToChannel(firstNode, phyIndex, l) == TRUE)
                {
                    // find the listening channel
                    break;
                }
            }


            PHY_CONN_NodePositionData::ListeningChannelNodeList & listening
                = partitionData->nodePositionData->listeningChannelNodeList;
            PHY_CONN_NodePositionData::ListeningChannelNodeInfo listeningInfo(
                    firstNode->nodeId,
                    phyIndex);
            listening[l].insert(listeningInfo);


            for (c = 0; c < totalChannels; ++c)
            {

                if (PHY_CanListenToChannel(firstNode, phyIndex, c) == FALSE)
                {
                    continue;
                }

                double txPower_mW;
                double gain_dBi;

                PHY_GetTransmitPower(
                    firstNode,
                    phyIndex,
                    &txPower_mW);

                // Get antenna gain, used for receiver gain
                PhyData* phy = firstNode->phyData[phyIndex];
                switch (phy->antennaData->antennaModelType)
                {
                    case ANTENNA_OMNIDIRECTIONAL:
                    {
                        AntennaOmnidirectional* omniDirectional =
                            (AntennaOmnidirectional*) phy->antennaData->antennaVar;
                        gain_dBi = omniDirectional->antennaGain_dB;
                        break;
                    }

                    case ANTENNA_SWITCHED_BEAM:
                    {
                        AntennaSwitchedBeam* switchedBeam =
                            (AntennaSwitchedBeam*) phy->antennaData->antennaVar;
                        gain_dBi = switchedBeam->antennaGain_dB;
                        break;
                    }

                    case ANTENNA_STEERABLE:
                    {
                        AntennaSteerable* steerable =
                            (AntennaSteerable*) phy->antennaData->antennaVar;
                        gain_dBi = steerable->antennaGain_dB;
                        break;
                    }

                    case ANTENNA_PATTERNED:
                    {
                        // this is place holder.
                        gain_dBi = 0;
                        break;
                    }

                    default:
                    {
                        char err[MAX_STRING_LENGTH];
                        sprintf(err, "Unknown ANTENNA-MODEL-TYPE %d.\n",
                            phy->antennaData->antennaModelType);
                        ERROR_ReportError(err);
                        break;
                    }
                }

                // insert into the listenable channel list
                PHY_CONN_NodePositionData::ListenableChannelNodeList& listenable
                    = partitionData->nodePositionData->listenableChannelNodeList;
                PHY_CONN_NodePositionData::ChannelNodeInfo channelNodeInfo(
                    firstNode->nodeId,
                    phyIndex,
                    phy->phyModel,
                    phy->antennaData->antennaModelType,
                    ANTENNA_ReturnHeight(firstNode, phyIndex),
                    txPower_mW,
                    ANTENNA_ReturnSystemLossIndB(firstNode, phyIndex),
                    gain_dBi);
                listenable[c].insert(channelNodeInfo);
            }


        }

        if (partitionData->isRunningInParallel())
        {

        // we do not need to do it for sequential
        Coordinates coordinates;
        MOBILITY_ReturnCoordinates(firstNode, &coordinates) ;
        if (PHY_CONNECTIVITY_DEBUG)
            {
        printf("time %f partition %d node %d position %f %f %f \n",
            (double) partitionData->getGlobalTime() / SECOND,
            partitionData->partitionId,
            firstNode->nodeId,
                    coordinates.common.c1,
                    coordinates.common.c2,
                    coordinates.common.c3);
                fflush(stdout);
        }

            // Set node position
            partitionData->nodePositionData->positions[firstNode->nodeId]
                = coordinates;
        }
    }


    SendListenableChannelInformation(partitionData);
    SendListeningChannelInformation(partitionData);
    SendNodePositionInformation(partitionData);
}

#ifdef USE_MPI
void ComposeLinkNodeMsg(PartitionData * partitionData,
                        int numLinks, int length)
{
    // compose the message
    int DEBUG_LENGTH = 0;

    // Assign the address for which the timer is meant for
    // partitionId, totalLength, (<senderId, senderIndex, receiverId, receiverIndex> ...)
    int totalLength = sizeof(int) * 3 + numLinks * sizeof(int) * 4;
    if (PHY_CONNECTIVITY_DEBUG)
    {
        printf("time %f partition %d totalLength for link node msg  = %d \n",
                (double)partitionData->getGlobalTime()/SECOND,
            partitionData->partitionId, totalLength);
    }

    Message *msg = MESSAGE_Alloc(partitionData->firstNode, PHY_LAYER, 0,
                       MSG_PHY_CONN_CrossPartitionMessage);

    MESSAGE_SetInstanceId(msg, (short) partitionData->partitionId);

    MESSAGE_AddInfo(partitionData->firstNode, msg,
        totalLength, MSG_PHY_CONN_CrossPartitionInfoField);
    UInt8 *info =
        (UInt8*)MESSAGE_ReturnInfo(msg,
        MSG_PHY_CONN_CrossPartitionInfoField);
    UInt8* dup_info = info;

    // type
    int type = PHY_CONN_NodePositionData::LINK_NODE_INFO;
    memcpy(info, &type, sizeof(int));
    DEBUG_LENGTH += sizeof(int);
    info += sizeof(int);

    // partitionId
    memcpy(info, &partitionData->partitionId, sizeof(int));
    DEBUG_LENGTH += sizeof(int);
    info += sizeof(int);

    // totalLength
    memcpy(info, &totalLength, sizeof(int));
    DEBUG_LENGTH += sizeof(int);
    info += sizeof(int);

    PHY_CONN_NodePositionData::PER_PARTITION_LINK_NODELIST::const_iterator iter
        = partitionData->nodePositionData->PerPartitionLinkNodeList.begin();

    int index = 0;
    for (; iter != partitionData->nodePositionData->
        PerPartitionLinkNodeList.end(); ++iter)
    {
        if (PHY_CONNECTIVITY_DEBUG)
        {
            printf("partition %d sends out link msg sender %d [%d] -> receiver %d [%d] \n",
                partitionData->partitionId,
                iter->first.first, iter->first.second,
                iter->second.first, iter->second.second);
        }

        // sender id

        memcpy(info + index , &(iter->first.first), sizeof(int));
        DEBUG_LENGTH += sizeof(int);
        index += sizeof(int);

        // sender index
        memcpy(info + index , &(iter->first.second), sizeof(int));
        DEBUG_LENGTH += sizeof(int);
        index += sizeof(int);

        // receiver id

        memcpy(info + index , &(iter->second.first), sizeof(int));
        DEBUG_LENGTH += sizeof(int);
        index += sizeof(int);

        // receiver index

        memcpy(info + index , &(iter->second.second), sizeof(int));
        DEBUG_LENGTH += sizeof(int);
        index += sizeof(int);
    }

    if (DEBUG_LENGTH != totalLength)
    {
        printf("Error at time %f partition %d debug_length %d totalLength %d \n",
            (double)partitionData->getGlobalTime()/SECOND,
            partitionData->partitionId,
            DEBUG_LENGTH, totalLength); fflush(stdout);
    }

    if (PHY_CONNECTIVITY_DEBUG)
    {
        printf("time %f partition %d insert link snd_msgList.size = %d msg size = %d\n",
            (double)partitionData->getGlobalTime()/SECOND,
            partitionData->partitionId,
            partitionData->nodePositionData->snd_msgList.size(),
            totalLength);
    }
    partitionData->nodePositionData->snd_msgList.push_back(
        msg);

}
#endif

void PHY_CONN_SendLinkNodeMsg(PartitionData * partitionData)
{
    // construct the structure
    int numLinks = 0;

    if (PHY_CONNECTIVITY_DEBUG)
    {
        printf("partition %d currentTime %f p2p link\n", partitionData->partitionId,
            (double) partitionData->getGlobalTime() / SECOND);
    }

    if (partitionData->nodePositionData->linkInformationSent == FALSE)
    {
        partitionData->nodePositionData->linkInformationSent = TRUE;
    }else {
        return;
    }

    Node *firstNode = partitionData->firstNode;
    for (; firstNode != NULL; firstNode = firstNode->nextNodeData)
    {
        for (int k = 0; k < firstNode->numberInterfaces; ++k)
        {
            if (firstNode->macData[k]->macProtocol == MAC_PROTOCOL_LINK)
            {
                LinkData *link = (LinkData *)firstNode->macData[k]->macVar;

                pair<int, int> key =
                    pair<int, int>(firstNode->nodeId, k);

                PHY_CONN_NodePositionData::PER_PARTITION_LINK_NODELIST::iterator
                iter =
                    partitionData->nodePositionData->PerPartitionLinkNodeList.find(key);

                if (iter == partitionData->nodePositionData->
                    PerPartitionLinkNodeList.end())
                {
                    partitionData->nodePositionData->PerPartitionLinkNodeList[key] =
                        pair<int, int>(link->destNodeId, link->destInterfaceIndex);
                    if (PHY_CONNECTIVITY_DEBUG)
                    {
                        printf(">>found link node %d [%d] -> node %d [%d]\n",
                            firstNode->nodeId, k,
                            link->destNodeId, link->destInterfaceIndex);
                    }
                    ++numLinks;
                }

            }
        }
    }

    if (!partitionData->isRunningInParallel())
    {
        return;
    }
#ifndef USE_MPI

    PARALLEL_SynchronizePartitions(partitionData, BARRIER_TYPE_PhyLinkConnectivity);
    {
        QNPartitionLock lock(phyConnLock);

        for (unsigned int i = 0; i < partitionData->getNumPartitions(); i++)
        {
            if (i == partitionData->partitionId)
            {
                continue;
            }

            // copy over
            ERROR_Assert(i < LinkNodeList.size(), "Error in PHY_CONN_SendLinkNodeMsg." );
            PHY_CONN_NodePositionData::PER_PARTITION_LINK_NODELIST *
                r_linkNodeList = LinkNodeList[i];
            PHY_CONN_NodePositionData::PER_PARTITION_LINK_NODELIST::const_iterator iter =
                r_linkNodeList->begin();
            while (iter != r_linkNodeList->end())
            {

                partitionData->nodePositionData->PerPartitionLinkNodeList[iter->first] = iter->second;

                ++iter;
            }
        }
    }
    PARALLEL_SynchronizePartitions(partitionData, 
        BARRIER_TYPE_PhyLinkConnectivityEnd);


#else
    int length = 0;

    if (numLinks == 0)
    {
        return;
    }
    ComposeLinkNodeMsg(partitionData, numLinks, length);
#endif

}

#ifdef USE_MPI
void PHY_CONN_ProcessLinkAllNodeMsg(PartitionData * partition, UInt8 *info)
{
    // wait for all other partitions information
    int lengthSofar = 0;
    int partitionId;
    int totalLength = 0;

    // decode the message

    // type
    int type = 0;
    memcpy(&type, info, sizeof(int));
    lengthSofar += sizeof(int);
    info += sizeof(int);

    // partitionId

    memcpy(&partitionId, info, sizeof(int));
    lengthSofar += sizeof(int);
    info += sizeof(int);
    if (PHY_CONNECTIVITY_DEBUG)
    {
        printf("time %f partition %d receives a cross partition link message.partition Id %d\n",
            (double)partition->getGlobalTime()/SECOND,
            partition->partitionId, partitionId); fflush(stdout);
    }
    if (partitionId == partition->partitionId){
        return;
    }

    memcpy(&totalLength, info, sizeof(int));
    lengthSofar += sizeof(int);
    info += sizeof(int);

    while (lengthSofar < totalLength)
    {
        for (int i = 0; i < sizeof(int) * 4; i+= sizeof(int))
        {

            int senderId = 0;
            int senderIndex = -1;
            int rcverId = 0;
            int rcverIndex = -1;

            // nodeId
            memcpy(&senderId, info + i, sizeof(int));
            i+= sizeof(int);

            // phy index
            memcpy(&senderIndex, info + i, sizeof(int));
            i+= sizeof(int);

            // nodeId
            memcpy(&rcverId, info + i, sizeof(int));
            i+= sizeof(int);

            // phy index
            memcpy(&rcverIndex, info + i, sizeof(int));

            pair<int, int> key = pair<int, int>(senderId, senderIndex);

            partition->nodePositionData->PerPartitionLinkNodeList[key]
                = pair<int, int>(rcverId, rcverIndex);
            if (PHY_CONNECTIVITY_DEBUG)
            {
                printf("partitionId %d senderId %d senderIndex %d rcverId %d rcverIndex %d\n",
                    partition->partitionId,
                    senderId, senderIndex, rcverId, rcverIndex);
            }
        }
        lengthSofar += sizeof(int) * 4;
        info += sizeof(int) * 4; //

    }
}

void ComposeSatComNodeMsg(PartitionData *partitionData,
                          int numSubnets, int length,
                          int msg_type,
                          PHY_CONN_NodePositionData::PER_PARTITION_SATCOM_NODELIST node_list)
{

    int DEBUG_LENGTH = 0;
    // compose the message

    // Assign the address for which the timer is meant for
    // satcom partitionId, totalLength, subnetIndex, type, length, (<nodeId, phyIndex> ...)
    // ethernet partitionId, totalLength, senderId, senderPhyIndex, length, (<nodeId, phyIndex> ...)
    int totalLength = sizeof(int) * 3 + numSubnets * sizeof(int) * 3 + length;
    if (PHY_CONNECTIVITY_DEBUG)
    {
        printf("time %f partition %d totalLength for ethernet msg  = %d \n",
            (double)partitionData->getGlobalTime()/SECOND,
            partitionData->partitionId,
            totalLength);
    }

    Message *msg = MESSAGE_Alloc(partitionData->firstNode, PHY_LAYER, 0,
        MSG_PHY_CONN_CrossPartitionMessage);

    MESSAGE_SetInstanceId(msg, (short) partitionData->partitionId);

    MESSAGE_AddInfo(partitionData->firstNode, msg,
        totalLength, MSG_PHY_CONN_CrossPartitionInfoField);
    UInt8 *info =
                (UInt8*)MESSAGE_ReturnInfo(msg,
                MSG_PHY_CONN_CrossPartitionInfoField);
    UInt8 *dup_info = info;

    // type
    int type = msg_type;
    memcpy(info, &type, sizeof(int));
    DEBUG_LENGTH += sizeof(int);
    info += sizeof(int);

    // partitionId
    memcpy(info, &partitionData->partitionId, sizeof(int));
    DEBUG_LENGTH += sizeof(int);
    info += sizeof(int);

    // totalLength
    memcpy(info, &totalLength, sizeof(int));
    DEBUG_LENGTH += sizeof(int);
    info += sizeof(int);

    PHY_CONN_NodePositionData::PER_PARTITION_SATCOM_NODELIST::const_iterator iter
        = node_list.begin();

    int index = 0;
    for (; iter != node_list.end(); ++iter)
    {
        memcpy(info + index, &(iter->first.first), sizeof(int)); // subnetIndex
        DEBUG_LENGTH += sizeof(int);
        index += sizeof(int);

        memcpy(info + index, &(iter->first.second), sizeof(int)); // type
        DEBUG_LENGTH += sizeof(int);
        index += sizeof(int);

        int perSatComLength =
            iter->second->size() * (sizeof(int) * 2);
        memcpy(info + index, &perSatComLength, sizeof(int)); // per subnet length
        DEBUG_LENGTH += sizeof(int);
        index += sizeof(int);

        set<PHY_CONN_NodePositionData::SatComNodeInfo >::const_iterator set_iter;
        for (set_iter = iter->second->begin(); set_iter != iter->second->end();
            ++set_iter)
        {
            // node id

            memcpy(info + index , &(set_iter->nodeId), sizeof(int));
            DEBUG_LENGTH += sizeof(int);
            index += sizeof(int);

            // phy index
            memcpy(info + index , &(set_iter->interfaceIndex), sizeof(int));
            DEBUG_LENGTH += sizeof(int);
            index += sizeof(int);

        }
        if (PHY_CONNECTIVITY_DEBUG)
        {
            printf("time %f partition %d subnetIndex %d perSatComLength %d \n",
                (double)partitionData->getGlobalTime()/SECOND,
                partitionData->partitionId, iter->first.first, perSatComLength);
        }
    }

    if (DEBUG_LENGTH != totalLength)
    {
        printf("Error at time %f partition %d debug_length %d totalLength %d \n",
            (double)partitionData->getGlobalTime()/SECOND,
            partitionData->partitionId,
            DEBUG_LENGTH, totalLength); fflush(stdout);
    }

    if (PHY_CONNECTIVITY_DEBUG)
    {
        printf("time %f partition %d insert satcom snd_msgList.size = %d \n",
            (double)partitionData->getGlobalTime()/SECOND,
            partitionData->partitionId,
            partitionData->nodePositionData->snd_msgList.size());
    }
    partitionData->nodePositionData->snd_msgList.push_back(msg);

}
#endif

void PHY_CONN_Send802_3NodeMsg(PartitionData * partitionData)
{
    // construct the structure
    int length = 0;
    int numEthernets = 0;

    if (PHY_CONNECTIVITY_DEBUG)
        printf("partition %d currentTime %f 802_3\n", partitionData->partitionId,
            (double) partitionData->getGlobalTime() / SECOND);

    if (partitionData->nodePositionData->ethernetInformationSent == FALSE)
    {
        partitionData->nodePositionData->ethernetInformationSent = TRUE;
    }else {
        return;
    }

    Node *firstNode = partitionData->firstNode;
    for (; firstNode != NULL; firstNode = firstNode->nextNodeData)
    {
        for (int k = 0; k < firstNode->numberInterfaces; ++k)
        {
            if (firstNode->macData[k]->macProtocol == MAC_PROTOCOL_802_3) // Mar. 3
            {
                MacData802_3* mac802_3 = (MacData802_3 *)
                              firstNode->macData[k]->macVar;

                ListItem* tempListItem = mac802_3->neighborList->first;

                while (tempListItem != NULL)
                {

                    SubnetMemberData* getNeighborInfo =
                        (SubnetMemberData *) tempListItem->data;
                    pair<int, int> key =
                        pair<int, int>(firstNode->nodeId, k);

                    PHY_CONN_NodePositionData::PER_PARTITION_SATCOM_NODELIST::iterator
                    iter =
                        partitionData->nodePositionData->PerPartition802_3NodeList.find(key);

                    if (iter == partitionData->nodePositionData->
                        PerPartition802_3NodeList.end())
                    {

                        partitionData->nodePositionData->PerPartition802_3NodeList[key]
                            = new set<PHY_CONN_NodePositionData::SatComNodeInfo >;
                        iter = partitionData->nodePositionData->
                            PerPartition802_3NodeList.find(key);

                        ++numEthernets;
                    }
                    iter->second->insert(PHY_CONN_NodePositionData::SatComNodeInfo(
                        getNeighborInfo->nodeId,
                        getNeighborInfo->interfaceIndex));

                    if (PHY_CONNECTIVITY_DEBUG)
                    {
                        printf(">>found 802.3 node %d [%d] -> node %d [%d]\n",
                            firstNode->nodeId, k,
                            getNeighborInfo->nodeId, getNeighborInfo->interfaceIndex);
                    }
                    tempListItem = tempListItem->next;
                    length += sizeof(int) * 2;
                }
            }
        }
    }

    if (!partitionData->isRunningInParallel())
    {
        return;
    }

#ifndef USE_MPI

    // 802_3NodeList

    PARALLEL_SynchronizePartitions(partitionData, 
        BARRIER_TYPE_PhyConSend802_3);

    {
        QNPartitionLock lock(phyConnLock);
        for (unsigned int i = 0; i < partitionData->getNumPartitions(); i++)
        {
            if (i == partitionData->partitionId)
            {
                continue;
            }

            // copy over
            ERROR_Assert(i < EthernetNodeList.size(), "Error in PHY_CONN_Send802_3NodeMsg." );
            PHY_CONN_NodePositionData::PER_PARTITION_SATCOM_NODELIST *
                r_802_3NodeList = EthernetNodeList[i];
            PHY_CONN_NodePositionData::PER_PARTITION_SATCOM_NODELIST::const_iterator r_iter =
                r_802_3NodeList->begin();

            while (r_iter != r_802_3NodeList->end())
            {
                PHY_CONN_NodePositionData::PER_PARTITION_SATCOM_NODELIST::const_iterator
                   iter = partitionData->nodePositionData->
                        PerPartition802_3NodeList.find(r_iter->first);
                if (iter == partitionData->nodePositionData->
                    PerPartition802_3NodeList.end())
                {
                    partitionData->nodePositionData->PerPartition802_3NodeList[r_iter->first]
                    = new set<PHY_CONN_NodePositionData::SatComNodeInfo >;
                    iter = partitionData->nodePositionData->
                        PerPartition802_3NodeList.find(r_iter->first);
                }

                set<PHY_CONN_NodePositionData::SatComNodeInfo >::const_iterator n_iter
                    = r_iter->second->begin();
                while (n_iter != r_iter->second->end())
                {
                    iter->second->insert(*n_iter);
                    ++n_iter;
                }
                ++r_iter;
            }
        }
    }

    PARALLEL_SynchronizePartitions(partitionData, 
        BARRIER_TYPE_PhyConSend802_3End);

#else
    if (numEthernets == 0)
    {
        return;
    }
    ComposeSatComNodeMsg(partitionData, numEthernets, length,
        PHY_CONN_NodePositionData::ETHERNET_NODE_INFO,
        partitionData->nodePositionData->PerPartition802_3NodeList);

#endif

}

void PHY_CONN_SendSatComNodeMsg(PartitionData * partitionData)
{

    // construct the structure
    int length = 0;

    if (PHY_CONNECTIVITY_DEBUG)
    {
        printf("partition %d currentTime %f SatCom\n", partitionData->partitionId,
            (double) partitionData->getGlobalTime() / SECOND);
    }

    if (partitionData->nodePositionData->subnetInformationSent == FALSE)
    {
        partitionData->nodePositionData->subnetInformationSent = TRUE;
    }else {
        return;
    }

    int i, j, k;
    int numSubnets = 0;
    for (j = 0; j < partitionData->subnetData.numSubnets; ++j)
    {
        SubnetMemberData memberData;

        for (i = 0; i <
            partitionData->subnetData.subnetList[j].numMembers; ++i)
        {

            memberData =
                partitionData->subnetData.subnetList[j].memberList[i];
            // send this subnet's information to remote partition

            Node* node = MAPPING_GetNodePtrFromHash(partitionData->nodeIdHash,
                                            memberData.nodeId);

            if (node == NULL) {continue;}

            k = memberData.interfaceIndex;
            pair<int, int> key;

            if (node->macData[k]->macProtocol == MAC_PROTOCOL_SATCOM)
            {
                MacSatComData *satCom =
                    (MacSatComData *)node->macData[memberData.interfaceIndex]->macVar;

                key = pair<int, int>(j, (int)satCom->type);
            }
#ifdef ADDON_BOEINGFCS
            else if (node->macData[k]->macProtocol == MAC_PROTOCOL_CES_WINTNCW)
            {
                MacCesWintNcwData *satCom =
                    (MacCesWintNcwData *)node->macData[memberData.interfaceIndex]->macVar;

                key = pair<int, int>(j, (int)satCom->type);
            }
#endif
            //else if (node->macData[k]->macProtocol == MAC_PROTOCOL_802_3)
            //{
            //    key = pair<int, int>(j, MAC_PROTOCOL_802_3);
            //}
            else if (node->macData[k]->macProtocol == MAC_PROTOCOL_SWITCHED_ETHERNET)
            {
                key = pair<int, int>(j, MAC_PROTOCOL_SWITCHED_ETHERNET);
            }
            else
            {
                continue;
            }

            PHY_CONN_NodePositionData::PER_PARTITION_SATCOM_NODELIST::iterator
                iter =
                    partitionData->nodePositionData->PerPartitionSatcomNodeList.find(key);

            if (iter == partitionData->nodePositionData->
                PerPartitionSatcomNodeList.end())
            {
                partitionData->nodePositionData->PerPartitionSatcomNodeList[key]
                = new set<PHY_CONN_NodePositionData::SatComNodeInfo >;
                iter = partitionData->nodePositionData->
                    PerPartitionSatcomNodeList.find(key);
                ++numSubnets;
            }


            iter->second->insert(PHY_CONN_NodePositionData::SatComNodeInfo(
                memberData.nodeId,
                memberData.interfaceIndex));
            if (PHY_CONNECTIVITY_DEBUG)
            {
                printf("time %f partition %d insert node %d [%d] on subnetIndex %d \n",
                    (double) partitionData->getGlobalTime() / SECOND,
                    partitionData->partitionId,
                    memberData.nodeId, memberData.interfaceIndex, j);
            }
            length += sizeof(int) * 2;
        }
    }

    if (partitionData->getNumPartitions() == 1)
    {
        return;
    }

#ifndef USE_MPI

    PARALLEL_SynchronizePartitions(partitionData, 
        BARRIER_TYPE_PhyConSendSatCom);

    {
        QNPartitionLock lock(phyConnLock);
        for (unsigned int i = 0; i < partitionData->getNumPartitions(); i++)
        {
            if (i == partitionData->partitionId)
            {
                continue;
            }

            // copy over
            ERROR_Assert(i < SatcomNodeList.size(), "Error in PHY_CONN_SendSatComNodeMsg." );
            PHY_CONN_NodePositionData::PER_PARTITION_SATCOM_NODELIST *
                r_satcomNodeList = SatcomNodeList[i];
            PHY_CONN_NodePositionData::PER_PARTITION_SATCOM_NODELIST::const_iterator r_iter =
                r_satcomNodeList->begin();

            while (r_iter != r_satcomNodeList->end())
            {
                PHY_CONN_NodePositionData::PER_PARTITION_SATCOM_NODELIST::const_iterator
                   iter = partitionData->nodePositionData->
                        PerPartitionSatcomNodeList.find(r_iter->first);
                if (iter == partitionData->nodePositionData->
                    PerPartitionSatcomNodeList.end())
                {
                    partitionData->nodePositionData->PerPartitionSatcomNodeList[r_iter->first]
                    = new set<PHY_CONN_NodePositionData::SatComNodeInfo >;
                    iter = partitionData->nodePositionData->
                        PerPartitionSatcomNodeList.find(r_iter->first);
                }

                set<PHY_CONN_NodePositionData::SatComNodeInfo >::const_iterator n_iter
                    = r_iter->second->begin();
                while (n_iter != r_iter->second->end())
                {
                    iter->second->insert(*n_iter);
                    ++n_iter;
                }
                ++r_iter;
            }
        }
    }

    PARALLEL_SynchronizePartitions(partitionData,
        BARRIER_TYPE_PhyConSendSatComEnd);

#else
    if (numSubnets == 0)
    {
        return;
    }
    ComposeSatComNodeMsg(partitionData, numSubnets, length,
        PHY_CONN_NodePositionData::SATCOM_NODE_INFO,
        partitionData->nodePositionData->PerPartitionSatcomNodeList);
#endif

}

#ifdef USE_MPI
void PHY_CONN_ProcessSatComAllNodeMsg(PartitionData * partition, UInt8 *info,
    PHY_CONN_NodePositionData::PER_PARTITION_SATCOM_NODELIST & node_list)
{
    // wait for all other partitions information
    int lengthSofar = 0;
    int partitionId;
    int totalLength = 0;

    // decode the message

    // type
    int type = 0;
    memcpy(&type, info, sizeof(int));
    lengthSofar += sizeof(int);
    info += sizeof(int);

    // partitionId

    memcpy(&partitionId, info, sizeof(int));
    lengthSofar += sizeof(int);
    info += sizeof(int);
    if (PHY_CONNECTIVITY_DEBUG)
    {
        printf("time %f partition %d receives a cross "
            "partition satcom/ethernet message.partition Id %d\n",
            (double)partition->getGlobalTime()/SECOND,
            partition->partitionId, partitionId); fflush(stdout);
    }
    if (partitionId == partition->partitionId)
    {
        return;
    }

    memcpy(&totalLength, info, sizeof(int));
    lengthSofar += sizeof(int);
    info += sizeof(int);

    while (lengthSofar < totalLength)
    {
        int subnetIndex = 0;
        int perSatComLength = 0;
        int type = 0;

        memcpy(&subnetIndex, info, sizeof(int));
        lengthSofar += sizeof(int);
        info += sizeof(int);

        memcpy(&type, info, sizeof(int));
        lengthSofar += sizeof(int);
        info += sizeof(int);

        memcpy(&perSatComLength, info, sizeof(int));
        lengthSofar += sizeof(int);
        info += sizeof(int);

        for (int i = 0; i < perSatComLength; i+= sizeof(int))
        {

            int nodeId = 0;
            int phyIndex = -1;

            // nodeId
            memcpy(&nodeId, info + i, sizeof(int));
            i+= sizeof(int);

            // phy index
            memcpy(&phyIndex, info + i, sizeof(int));

            pair<int, int> key = pair<int, int>(subnetIndex, type);
            PHY_CONN_NodePositionData::PER_PARTITION_SATCOM_NODELIST::iterator
            iter =
                node_list.find(key);
            if (iter == node_list.end())
            {

                node_list[key]
                    = new set<PHY_CONN_NodePositionData::SatComNodeInfo >;

                iter = node_list.find(key);
            }

            iter->second->insert(PHY_CONN_NodePositionData::SatComNodeInfo(
                nodeId, phyIndex));

            if (PHY_CONNECTIVITY_DEBUG)
            {
                printf("partitionId %d subnetIndex %d type %d per satcomlength %d nodeId %d phyIndex %d "
                    "nodeList size %d \n",
                    partition->partitionId,
                    subnetIndex, type, perSatComLength, nodeId, phyIndex,
                    node_list.size());
            }

        }
        lengthSofar += perSatComLength;
        info += perSatComLength; //

    }
}
#endif



void HandleLinkPhyConnInsertion(Node* node,
                                   PartitionData* partition)
{

    PHY_CONN_NodePositionData *nodePositionData =
        partition->nodePositionData;

    PHY_CONN_NodePositionData::PER_PARTITION_LINK_NODELIST::const_iterator
        txrx_iter =
            nodePositionData->PerPartitionLinkNodeList.begin();

    for (; txrx_iter != nodePositionData->PerPartitionLinkNodeList.end();
        ++txrx_iter)
    {

#ifdef ADDON_DB

        StatsDBPhyConnParam phyParam;
        phyParam.m_SenderId = txrx_iter->first.first;
        phyParam.m_ReceiverId = txrx_iter->second.first;
        phyParam.m_PhyIndex = txrx_iter->first.second;
        phyParam.m_ReceiverPhyIndex = txrx_iter->second.second;
        phyParam.m_ChannelIndex = "Link";
        STATSDB_HandlePhyConnTableUpdate(node, phyParam);
#endif

        HandlePhyConnSenderListInsertion(partition,
            txrx_iter->first.first, txrx_iter->first.second,
            txrx_iter->second.first, txrx_iter->second.second,
            /*-1, -1, -1,*/
            PHY_CONN_NodePositionData::WIRED_LINK,
            PHY_CONN_NodePositionData::WIRED_LINK);

    }

    // not to clear here since link information does not change
    // during simulation
    //nodePositionData->PerPartitionLinkNodeList.clear();
}

void FindSenderReceiverChannelListening(
    PHY_CONN_NodePositionData *nodePositionData,
    int txNodeId, int k,
    int rxNodeId, int l,
    int channelIndex, BOOL & s_listening,
    BOOL & r_listening)
{
    // Get listening info for tx and rx node

    nodePositionData->GetListeningInfo(
        txNodeId,
        k,
        channelIndex,
        &s_listening);

    nodePositionData->GetListeningInfo(
        rxNodeId,
        l,
        channelIndex,
        &r_listening);
}

void HandlePhyConnPathlossInsertion(PartitionData *partition,
    int senderId, int senderIndex,
    int rcverId, int rcverIndex, int channelIndex,
    double pathloss,
    PHY_CONN_NodePositionData::InterfaceType st,
    PHY_CONN_NodePositionData::InterfaceType rt)
{
    PHY_CONN_NodePositionData *nodePositionData =
        partition->nodePositionData;

    BOOL s_listening = FALSE;
    BOOL r_listening = FALSE;
    BOOL connected;

    FindSenderReceiverChannelListening(
        nodePositionData, senderId, senderIndex,
        rcverId, rcverIndex, channelIndex, s_listening,
        r_listening);

    //if (channelIndex != -1 && (
    //    s_listening == FALSE || r_listening == FALSE))
    // channelIndex is -1 for models that do not have channels
    pair<int, int> key = pair<int, int>(senderId, rcverId);
    if (channelIndex == -1 || ( s_listening == TRUE && r_listening == TRUE))
    {
        connected = TRUE;
    }
    else
    {
        connected = FALSE;
    }


    PHY_CONN_NodePositionData::AdjacencyNodeKey adjacencyKey(
        rcverId,
        channelIndex,
        senderIndex,
        rcverIndex);

    PHY_CONN_NodePositionData::AdjacencyNodeValue adjacencyValue(
        st,
        rt,
        connected,
        pathloss);

    nodePositionData->connectivity[senderId][rcverId].
    insert(PairAdjacencyKeyValue(adjacencyKey,
        adjacencyValue));

    if (PHY_CONNECTIVITY_DEBUG)
    {
        printf("node %d [%d] - node %d [%d] on %d \n",
         senderId, senderIndex, rcverId, rcverIndex,
         channelIndex);
    }
}


void HandlePhyConnSenderListInsertion(PartitionData *partition,
    int senderId, int senderIndex,
    int rcverId, int rcverIndex,
    //int channelIndex,
    //double rxPower_mw, double rxPower_mwWorst,
    PHY_CONN_NodePositionData::InterfaceType st,
    PHY_CONN_NodePositionData::InterfaceType rt)
{
    PHY_CONN_NodePositionData *nodePositionData =
        partition->nodePositionData;

    PHY_CONN_NodePositionData::AdjacencyNodeKey adjacencyKey(
        rcverId,
        -1,
        senderIndex,
        rcverIndex);
    PHY_CONN_NodePositionData::AdjacencyNodeValue adjacencyValue(
        st,
        rt);

    nodePositionData->connectivity[senderId][rcverId].
        insert(PairAdjacencyKeyValue(adjacencyKey, adjacencyValue));

    if (PHY_CONNECTIVITY_DEBUG)
    {
        printf("node %d [%d] - node %d [%d] on %d \n",
         senderId, senderIndex, rcverId, rcverIndex,
         -1);
    }
}

void Handle802_3PhyConnInsertion(Node *node,
                                 PartitionData *partition)
{
    PHY_CONN_NodePositionData *nodePositionData =
        partition->nodePositionData;

    PHY_CONN_NodePositionData::PER_PARTITION_SATCOM_NODELIST::const_iterator
        tx_iter =
            nodePositionData->PerPartition802_3NodeList.begin();

    for (; tx_iter != nodePositionData->PerPartition802_3NodeList.end();
        ++tx_iter)
    {
        int senderId = tx_iter->first.first;

        int phyIndex = tx_iter->first.second;

        // find the member nodes
            pair<int, int> key = pair<int, int>(senderId, phyIndex);

            PHY_CONN_NodePositionData::PER_PARTITION_SATCOM_NODELIST::const_iterator
                rx_iter = nodePositionData->PerPartition802_3NodeList.find(key);

            if (rx_iter == nodePositionData->PerPartition802_3NodeList.end())
            {
                continue;
            }


            // sender node to each member node
            set<PHY_CONN_NodePositionData::SatComNodeInfo >::const_iterator
                rx_node_iter = rx_iter->second->begin();

            while (rx_node_iter != rx_iter->second->end())
            {
                if (PHY_CONNECTIVITY_DEBUG)
                {
                    printf("p:%d find ethernet node %d [%d] -> node %d [%d]\n",
                        node->partitionData->partitionId,
                        tx_iter->first.first, tx_iter->first.second,
                        rx_node_iter->nodeId, rx_node_iter->interfaceIndex);
                }
#ifdef ADDON_DB
                StatsDBPhyConnParam phyParam;
                phyParam.m_SenderId = tx_iter->first.first;
                phyParam.m_ReceiverId = rx_node_iter->nodeId;
                phyParam.m_PhyIndex = tx_iter->first.second;
                phyParam.m_ReceiverPhyIndex = rx_node_iter->interfaceIndex;
                phyParam.m_ChannelIndex = "Ethernet";

                STATSDB_HandlePhyConnTableUpdate(node, phyParam);
#endif
                HandlePhyConnSenderListInsertion(partition,
                    tx_iter->first.first, tx_iter->first.second,
                    rx_node_iter->nodeId, rx_node_iter->interfaceIndex,
                    /*-1, -1, -1,*/ PHY_CONN_NodePositionData::ETHERNET,
                    PHY_CONN_NodePositionData::ETHERNET);

                ++rx_node_iter;
            }
    }

    // do not clear since subnet information does not change with simulation
}

void HandleSatComPhyConnInsertion(Node* node,
                                   PartitionData* partition)
{

    PHY_CONN_NodePositionData *nodePositionData =
        partition->nodePositionData;

    PHY_CONN_NodePositionData::PER_PARTITION_SATCOM_NODELIST::const_iterator
        tx_iter =
            nodePositionData->PerPartitionSatcomNodeList.begin();

    for (; tx_iter != nodePositionData->PerPartitionSatcomNodeList.end();
        ++tx_iter)
    {
        int subnetIndex = tx_iter->first.first;

        int type = tx_iter->first.second;

        if (type == (int)MAC_SATCOM_GROUND)
        {
            // find the satellie
            pair<int, int> key = pair<int, int>(subnetIndex,
                (int)MAC_SATCOM_SATELLITE);
            PHY_CONN_NodePositionData::PER_PARTITION_SATCOM_NODELIST::const_iterator
                rx_iter = nodePositionData->PerPartitionSatcomNodeList.find(key);

            if (rx_iter == nodePositionData->PerPartitionSatcomNodeList.end())
            {
                continue;
            }

            // each ground node to the satellite node
            set<PHY_CONN_NodePositionData::SatComNodeInfo >::const_iterator
                tx_node_iter = tx_iter->second->begin();

            set<PHY_CONN_NodePositionData::SatComNodeInfo >::const_iterator
                rx_node_iter = rx_iter->second->begin();

            ERROR_Assert(rx_node_iter != rx_iter->second->end(),
                "Error in HandleSatComPhyConnInsertion.");

            if (PHY_CONNECTIVITY_DEBUG)
            {
                printf("tx node size = %"TYPES_SIZEOFMFT"u rx satellite node %d\n",
                    tx_iter->second->size(), rx_node_iter->nodeId);
            }
            while (tx_node_iter != tx_iter->second->end())
            {
                if (PHY_CONNECTIVITY_DEBUG)
                {
                    printf("find ground node %d [%d] -> satellite node %d [%d]\n",
                        tx_node_iter->nodeId, tx_node_iter->interfaceIndex,
                        rx_node_iter->nodeId, rx_node_iter->interfaceIndex);
                }
#ifdef ADDON_DB
                StatsDBPhyConnParam phyParam;
                phyParam.m_SenderId = tx_node_iter->nodeId;
                phyParam.m_ReceiverId = rx_node_iter->nodeId;
                phyParam.m_PhyIndex = tx_node_iter->interfaceIndex;
                phyParam.m_ReceiverPhyIndex = rx_node_iter->interfaceIndex;
                phyParam.m_ChannelIndex = "SatCom";

                STATSDB_HandlePhyConnTableUpdate(node, phyParam);
#endif
                HandlePhyConnSenderListInsertion(partition,
                    tx_node_iter->nodeId, tx_node_iter->interfaceIndex,
                    rx_node_iter->nodeId, rx_node_iter->interfaceIndex,
                    /*-1, -1, -1,*/ PHY_CONN_NodePositionData::SATCOM_GROUND,
                    PHY_CONN_NodePositionData::SATCOM_SATELLITE);
                ++tx_node_iter;
            }

        }
        else if (type == (int)MAC_SATCOM_SATELLITE)
        {
            // find the ground nodes
            pair<int, int> key = pair<int, int>(subnetIndex,
                (int)MAC_SATCOM_GROUND);
            PHY_CONN_NodePositionData::PER_PARTITION_SATCOM_NODELIST::const_iterator
                rx_iter = nodePositionData->PerPartitionSatcomNodeList.find(key);

            if (rx_iter == nodePositionData->PerPartitionSatcomNodeList.end())
            {
                continue;
            }

            set<PHY_CONN_NodePositionData::SatComNodeInfo >::const_iterator
                tx_node_iter = tx_iter->second->begin();

            ERROR_Assert(tx_node_iter != tx_iter->second->end(),
                "Error in HandleSatComPhyConnInsertion.");

            // satellite node to each ground node
            set<PHY_CONN_NodePositionData::SatComNodeInfo >::const_iterator
                rx_node_iter = rx_iter->second->begin();

            while (rx_node_iter != rx_iter->second->end())
            {
                if (PHY_CONNECTIVITY_DEBUG)
                {
                    printf("find satellite node %d [%d] -> ground node %d [%d]\n",
                        tx_node_iter->nodeId, tx_node_iter->interfaceIndex,
                        rx_node_iter->nodeId, rx_node_iter->interfaceIndex);
                }
#ifdef ADDON_DB
                StatsDBPhyConnParam phyParam;
                phyParam.m_SenderId = tx_node_iter->nodeId;
                phyParam.m_ReceiverId = rx_node_iter->nodeId;
                phyParam.m_PhyIndex = tx_node_iter->interfaceIndex;
                phyParam.m_ReceiverPhyIndex = rx_node_iter->interfaceIndex;
                phyParam.m_ChannelIndex = "SatCom";

                STATSDB_HandlePhyConnTableUpdate(node, phyParam);
#endif
                HandlePhyConnSenderListInsertion(partition,
                    tx_node_iter->nodeId, tx_node_iter->interfaceIndex,
                    rx_node_iter->nodeId, rx_node_iter->interfaceIndex,
                    /*-1, -1, -1,*/ PHY_CONN_NodePositionData::SATCOM_SATELLITE,
                    PHY_CONN_NodePositionData::SATCOM_GROUND);

                ++rx_node_iter;
            }

        }

#ifdef ADDON_BOEINGFCS
        else if (type == (int)MAC_CES_WINT_NCW_GROUND)
        {
            // find the satellie
            pair<int, int> key = pair<int, int>(subnetIndex,
                (int)MAC_CES_WINT_NCW_SATELLITE);
            PHY_CONN_NodePositionData::PER_PARTITION_SATCOM_NODELIST::const_iterator
                rx_iter = nodePositionData->PerPartitionSatcomNodeList.find(key);

            if (rx_iter == nodePositionData->PerPartitionSatcomNodeList.end())
            {
                continue;
            }

            // each ground node to the satellite node
            set<PHY_CONN_NodePositionData::SatComNodeInfo >::const_iterator
                tx_node_iter = tx_iter->second->begin();

            set<PHY_CONN_NodePositionData::SatComNodeInfo >::const_iterator
                rx_node_iter = rx_iter->second->begin();

            ERROR_Assert(rx_node_iter != rx_iter->second->end(),
                "Error in HandleSatComPhyConnInsertion.");

            if (PHY_CONNECTIVITY_DEBUG)
            {
                printf("tx node size = %d rx satellite node %d\n",
                    tx_iter->second->size(), rx_node_iter->nodeId);
            }
            while (tx_node_iter != tx_iter->second->end())
            {
                if (PHY_CONNECTIVITY_DEBUG)
                {
                    printf("find ground node %d [%d] -> satellite node %d [%d]\n",
                        tx_node_iter->nodeId, tx_node_iter->interfaceIndex,
                        rx_node_iter->nodeId, rx_node_iter->interfaceIndex);
                }
#ifdef ADDON_DB
                StatsDBPhyConnParam phyParam;
                phyParam.m_SenderId = tx_node_iter->nodeId;
                phyParam.m_ReceiverId = rx_node_iter->nodeId;
                phyParam.m_PhyIndex = tx_node_iter->interfaceIndex;
                phyParam.m_ReceiverPhyIndex = rx_node_iter->interfaceIndex;
                phyParam.m_ChannelIndex = "SatCom";

                STATSDB_HandlePhyConnTableUpdate(node, phyParam);
#endif
                HandlePhyConnSenderListInsertion(partition,
                    tx_node_iter->nodeId, tx_node_iter->interfaceIndex,
                    rx_node_iter->nodeId, rx_node_iter->interfaceIndex,
                    /*-1, -1, -1,*/ PHY_CONN_NodePositionData::NCW_GROUND,
                    PHY_CONN_NodePositionData::NCW_SATELLITE);
                ++tx_node_iter;
            }

        }
        else if (type == (int)MAC_CES_WINT_NCW_SATELLITE)
        {
            // find the ground nodes
            pair<int, int> key = pair<int, int>(subnetIndex,
                (int)MAC_CES_WINT_NCW_GROUND);
            PHY_CONN_NodePositionData::PER_PARTITION_SATCOM_NODELIST::const_iterator
                rx_iter = nodePositionData->PerPartitionSatcomNodeList.find(key);

            if (rx_iter == nodePositionData->PerPartitionSatcomNodeList.end())
            {
                continue;
            }


           set<PHY_CONN_NodePositionData::SatComNodeInfo >::const_iterator
                tx_node_iter = tx_iter->second->begin();

            ERROR_Assert(tx_node_iter != tx_iter->second->end(),
                "Error in HandleSatComPhyConnInsertion.");

            // satellite node to each ground node
            set<PHY_CONN_NodePositionData::SatComNodeInfo >::const_iterator
                rx_node_iter = rx_iter->second->begin();

            while (rx_node_iter != rx_iter->second->end())
            {
                if (PHY_CONNECTIVITY_DEBUG)
                {
                    printf("find satellite node %d [%d] -> ground node %d [%d]\n",
                        tx_node_iter->nodeId, tx_node_iter->interfaceIndex,
                        rx_node_iter->nodeId, rx_node_iter->interfaceIndex);
                }
#ifdef ADDON_DB
                StatsDBPhyConnParam phyParam;
                phyParam.m_SenderId = tx_node_iter->nodeId;
                phyParam.m_ReceiverId = rx_node_iter->nodeId;
                phyParam.m_PhyIndex = tx_node_iter->interfaceIndex;
                phyParam.m_ReceiverPhyIndex = rx_node_iter->interfaceIndex;
                phyParam.m_ChannelIndex = "SatCom";

                STATSDB_HandlePhyConnTableUpdate(node, phyParam);
#endif
                HandlePhyConnSenderListInsertion(partition,
                    tx_node_iter->nodeId, tx_node_iter->interfaceIndex,
                    rx_node_iter->nodeId, rx_node_iter->interfaceIndex,
                    /*-1, -1, -1,*/ PHY_CONN_NodePositionData::NCW_SATELLITE,
                    PHY_CONN_NodePositionData::NCW_GROUND);

                ++rx_node_iter;
            }

        }
#endif

        else if (type ==  (int)MAC_PROTOCOL_LINK)
        {
            // find the link nodes
            set<PHY_CONN_NodePositionData::SatComNodeInfo >::const_iterator
                tx_node_iter = tx_iter->second->begin();


            while (tx_node_iter != tx_iter->second->end())
            {
                set<PHY_CONN_NodePositionData::SatComNodeInfo >::const_iterator
                    rx_node_iter = tx_iter->second->begin();

                while (rx_node_iter != tx_iter->second->end())
                {
                    if (rx_node_iter->nodeId == tx_node_iter->nodeId)
                    {
                        continue;
                    }
                    if (PHY_CONNECTIVITY_DEBUG)
                    {
                        printf("find link node %d [%d] -> node %d [%d]\n",
                            tx_node_iter->nodeId, tx_node_iter->interfaceIndex,
                            rx_node_iter->nodeId, rx_node_iter->interfaceIndex);
                    }
#ifdef ADDON_DB
                    StatsDBPhyConnParam phyParam;
                    phyParam.m_SenderId = tx_node_iter->nodeId;
                    phyParam.m_ReceiverId = rx_node_iter->nodeId;
                    phyParam.m_PhyIndex = tx_node_iter->interfaceIndex;
                    phyParam.m_ReceiverPhyIndex = rx_node_iter->interfaceIndex;
                    phyParam.m_ChannelIndex = "Link";

                    STATSDB_HandlePhyConnTableUpdate(node, phyParam);
#endif
                    HandlePhyConnSenderListInsertion(partition,
                        tx_node_iter->nodeId, tx_node_iter->interfaceIndex,
                        rx_node_iter->nodeId, rx_node_iter->interfaceIndex,
                        /*-1, -1, -1,*/ PHY_CONN_NodePositionData::WIRED_LINK,
                        PHY_CONN_NodePositionData::WIRED_LINK);

                    ++rx_node_iter;
                }
                ++tx_node_iter;
            }
        }
        else if (type ==  (int)MAC_PROTOCOL_SWITCHED_ETHERNET )
        {

            set<PHY_CONN_NodePositionData::SatComNodeInfo >::const_iterator
                tx_node_iter = tx_iter->second->begin();

            while (tx_node_iter != tx_iter->second->end())
            {
                set<PHY_CONN_NodePositionData::SatComNodeInfo >::const_iterator
                    rx_node_iter = tx_iter->second->begin();

                while (rx_node_iter != tx_iter->second->end())
                {
                    if (rx_node_iter->nodeId == tx_node_iter->nodeId)
                    {
                        ++rx_node_iter;
                        continue;
                    }
                    if (PHY_CONNECTIVITY_DEBUG)
                    {
                        printf("find switched ethernet node %d [%d] -> node %d [%d]\n",
                            tx_node_iter->nodeId, tx_node_iter->interfaceIndex,
                            rx_node_iter->nodeId, rx_node_iter->interfaceIndex);
                    }

#ifdef ADDON_DB
                    StatsDBPhyConnParam phyParam;
                    phyParam.m_SenderId = tx_node_iter->nodeId;
                    phyParam.m_ReceiverId = rx_node_iter->nodeId;
                    phyParam.m_PhyIndex = tx_node_iter->interfaceIndex;
                    phyParam.m_ReceiverPhyIndex = rx_node_iter->interfaceIndex;
                    phyParam.m_ChannelIndex = "Switched Ethernet";
                    STATSDB_HandlePhyConnTableUpdate(node, phyParam);
#endif
                    HandlePhyConnSenderListInsertion(partition,
                        tx_node_iter->nodeId, tx_node_iter->interfaceIndex,
                        rx_node_iter->nodeId, rx_node_iter->interfaceIndex,
                        /*-1, -1, -1,*/
                        PHY_CONN_NodePositionData::SWITCHED_ETHERNET,
                        PHY_CONN_NodePositionData::SWITCHED_ETHERNET);

                    ++rx_node_iter;
                }
                ++tx_node_iter;
            }
        }
    }

    // do not clear since subnet information does not change with simulation
}

// /**
// FUNCTION :: AntennaSwitchedBeamGetBestGainAtThisDirection
// PURPOSE :: Get the best gain at given direction for switched beam antenna
//
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyindex  : int   : phy index.
// + DOA       : Orientation : Given direction.
// RETURN :: double: The best gain at given direction
// **/

static
double AntennaSwitchedBeamGetBestGainAtThisDirection(
    Node* node,
    int phyIndex,
    Orientation DOA)
{
    PhyData* phyData = node->phyData[phyIndex];
    AntennaSwitchedBeam* switched =
        (AntennaSwitchedBeam* )phyData->antennaData->antennaVar;

    int i;
    int maxGainPattern = ANTENNA_PATTERN_NOT_SET;
    double maxGain_dBi = ANTENNA_LOWEST_GAIN_dBi;
    const int originalPatternIndex = switched->patternIndex;
    const int numPatterns = switched->numPatterns;

    ERROR_Assert(phyData->antennaData->antennaModelType ==
        ANTENNA_SWITCHED_BEAM , "antennaModelType is switched beam.\n");

    // get the maximum gain at given direction
    for (i = 0; i < numPatterns; i++)
    {
        double gain_dBi;

        gain_dBi =
                AntennaSwitchedBeamGainForThisDirectionWithPatternIndex(
                        node,
                        phyIndex,
                        i,
                        DOA);

        if (gain_dBi > maxGain_dBi)
        {
            maxGainPattern = i;
            maxGain_dBi = gain_dBi;
        }
     }

    if (maxGainPattern == ANTENNA_PATTERN_NOT_SET ||
        maxGain_dBi < switched->antennaGain_dB)
    {
        maxGain_dBi = switched->antennaGain_dB;
    }

    switched->patternIndex = originalPatternIndex;

    return maxGain_dBi;
}

// /**
// FUNCTION :: AntennaSwitchedBeamGetWorstGainAtThisDirection
// PURPOSE :: Get the worst gain at given direction for switched beam antenna
//
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyindex  : int   : phy index.
// + DOA       : Orientation : Given direction.
// RETURN :: double : The worst gain at given direction
// **/

static
double AntennaSwitchedBeamGetWorstGainAtThisDirection(
    Node* node,
    int phyIndex,
    Orientation DOA)
{
    PhyData* phyData = node->phyData[phyIndex];
    AntennaSwitchedBeam* switched =
        (AntennaSwitchedBeam* )phyData->antennaData->antennaVar;

    int i;
    int worstGainPattern = ANTENNA_PATTERN_NOT_SET;
    double worstGain_dBi = switched->antennaGain_dB;
    const int originalPatternIndex = switched->patternIndex;
    const int numPatterns = switched->numPatterns;

    ERROR_Assert(phyData->antennaData->antennaModelType ==
        ANTENNA_SWITCHED_BEAM , "antennaModelType is switched beam.\n");

    // get the worst gain at given direction
    for (i = 0; i < numPatterns; i++)
    {
        double gain_dBi;

        gain_dBi =
                AntennaSwitchedBeamGainForThisDirectionWithPatternIndex(
                        node,
                        phyIndex,
                        i,
                        DOA);

        if (gain_dBi < worstGain_dBi)
        {
            worstGainPattern = i;
            worstGain_dBi = gain_dBi;
        }
     }

    if (worstGainPattern == ANTENNA_PATTERN_NOT_SET ||
        worstGain_dBi > switched->antennaGain_dB)
    {
        worstGain_dBi = switched->antennaGain_dB;
    }

    switched->patternIndex = originalPatternIndex;

    return  worstGain_dBi;
}

// /**
// FUNCTION :: steerableObtainSteeringAngleForThisDirection
// PURPOSE :: Get the steering angle at given direction for steerable antenna
//
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyindex  : int   : phy index.
// + DOA       : Orientation : Given direction.
// + steeringAngle  : Orientation* : To store the steering angle.
// RETURN ::  void : NULL
// **/

static
void SteerableObtainSteeringAngleForThisDirection(
     Node* node,
     int phyIndex,
     Orientation DOA,
     Orientation* steeringAngle)
{
    PhyData* phyData = node->phyData[phyIndex];
    AntennaSteerable* steerable =
        (AntennaSteerable* )phyData->antennaData->antennaVar;

    Orientation nodeOrientation;

    // get the boresight angle
    Orientation boreSightAngle = steerable->antennaPatterns->
                        boreSightAngle[steerable->patternIndex];

    ERROR_Assert(phyData->antennaData->antennaModelType ==
        ANTENNA_STEERABLE ,"antennaModelType is not steerable.\n");

    // get node's orientation
    MOBILITY_ReturnOrientation(node, &nodeOrientation);

    // get the steering angle
    steeringAngle->azimuth =
                (short) COORD_NormalizeAzimuthAngle(
                            DOA.azimuth -
                            nodeOrientation.azimuth +
                            boreSightAngle.azimuth);

    steeringAngle->elevation =
                (short) COORD_NormalizeElevationAngle(
                            DOA.elevation -
                            nodeOrientation.elevation -
                            boreSightAngle.elevation);

    if (abs(steeringAngle->elevation) > 90)
    {
        steeringAngle->azimuth += ANGLE_RESOLUTION/2;
        steeringAngle->elevation =
            ANGLE_RESOLUTION/2 - steeringAngle->elevation;
        steeringAngle->azimuth = (OrientationType)
            COORD_NormalizeAzimuthAngle (steeringAngle->azimuth);
        steeringAngle->elevation = (OrientationType)
            COORD_NormalizeElevationAngle (steeringAngle->elevation);
    }

    return;
}

// /**
// FUNCTION :: steerableSetSteeringAngle
// PURPOSE :: set the steering angle for steerable antenna
//
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyindex  : int   : phy index.
// + DOA       : Orientation : Given direction.
// RETURN ::  void : NULL
// **/

static
void SteerableSetSteeringAngle(
     Node* node,
     int phyIndex,
     Orientation DOA)
{
    PhyData* phyData = node->phyData[phyIndex];
    AntennaSteerable* steerable =
        (AntennaSteerable* )phyData->antennaData->antennaVar;
    Orientation steeringAngle;
    int patternIndex = 1;

    ERROR_Assert(phyData->antennaData->antennaModelType ==
        ANTENNA_STEERABLE ,
            "antennaModelType is not Steerable.\n");

    // set the pattern index for steerable antenna
    if (steerable->numPatterns == 1)
    {
        AntennaSteerableSetPattern(node, phyIndex, 0);
    } else
    {
        const int patternSectorAngle =
           steerable->patternSetRepeatAngle / steerable->numPatterns;

        patternIndex =
           ((DOA.azimuth + (patternSectorAngle/2)) %
             steerable->patternSetRepeatAngle) / patternSectorAngle;

        AntennaSteerableSetPattern(node, phyIndex, patternIndex);
    }//if//

    // get the steering angle
    SteerableObtainSteeringAngleForThisDirection(node,
                                                 phyIndex,
                                                 DOA,
                                                 &steeringAngle);

    steerable->steeringAngle = steeringAngle;

    return;
}

// /**
// FUNCTION :: AntennaSteerableGetBestGainAtThisDirection
// PURPOSE :: Get the best gain at given direction for steerable antenna
//
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyindex  : int   : phy index.
// + DOA       : Orientation : Given direction.
// RETURN :: double : The best gain at given direction
// **/

static
double AntennaSteerableGetBestGainAtThisDirection(
    Node* node,
    int phyIndex,
    Orientation DOA)
{
    int i;
    int maxGainPattern = ANTENNA_PATTERN_NOT_SET;
    double maxGain_dBi = ANTENNA_LOWEST_GAIN_dBi;
    Orientation steeringAngle;
    int patternIndex;

    PhyData* phyData = node->phyData[phyIndex];

    AntennaSteerable* steerable =
        (AntennaSteerable* )phyData->antennaData->antennaVar;

    steeringAngle = steerable->steeringAngle;
    patternIndex = steerable->patternIndex;

    // set the steering angle for given direction
    SteerableSetSteeringAngle(
             node,
             phyIndex,
             DOA);

    int numPatterns = steerable->numPatterns;

    // get the maximum gain for steerable antenna
    for (i = 0; i < numPatterns; i++)
    {
        double gain_dBi;

        gain_dBi =
            AntennaSteerableGainForThisDirectionWithPatternIndex(
                         node,
                         phyIndex,
                         i,
                         DOA);

        if (gain_dBi > maxGain_dBi)
        {
            maxGainPattern = i;
            maxGain_dBi = gain_dBi;
        }
    }

    if (maxGainPattern == ANTENNA_PATTERN_NOT_SET ||
        maxGain_dBi < steerable->antennaGain_dB)
    {
        maxGain_dBi = steerable->antennaGain_dB;
    }

    steerable->steeringAngle = steeringAngle;
    steerable->patternIndex = patternIndex;

    return maxGain_dBi;

}

// /**
// FUNCTION :: AntennaSteerableGetWorstGainAtThisDirection
// PURPOSE :: Get the worst gain at given direction for steerable antenna
//
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyindex  : int   : phy index.
// + DOA       : Orientation : Given direction.
// RETURN :: double : The worst gain at given direction for steering antenna
// **/

static
double AntennaSteerableGetWorstGainAtThisDirection(
    Node* node,
    int phyIndex,
    Orientation DOA)
{
    int i;
    int worstGainPattern = ANTENNA_PATTERN_NOT_SET;
    double worstGain_dBi;
    Orientation steeringAngle;
    int patternIndex;

    PhyData* phyData = node->phyData[phyIndex];
    AntennaSteerable* steerable =
        (AntennaSteerable* )phyData->antennaData->antennaVar;

    worstGain_dBi = steerable->antennaGain_dB;
    steeringAngle = steerable->steeringAngle;
    patternIndex = steerable->patternIndex;

    // set the steering angle for given direction
    SteerableSetSteeringAngle(
             node,
             phyIndex,
             DOA);

    int numPatterns = steerable->numPatterns;

    // get the worst gain for it at given direction
    for (i = 0; i < numPatterns; i++)
    {
        double gain_dBi;

        gain_dBi =
            AntennaSteerableGainForThisDirectionWithPatternIndex(
                         node,
                         phyIndex,
                         i,
                         DOA);

        if (gain_dBi < worstGain_dBi)
        {
            worstGainPattern = i;
            worstGain_dBi = gain_dBi;
        }
    }

    if (worstGainPattern == ANTENNA_PATTERN_NOT_SET ||
        worstGain_dBi > steerable->antennaGain_dB)
    {
        worstGain_dBi = steerable->antennaGain_dB;
    }

    steerable->steeringAngle = steeringAngle;
    steerable->patternIndex = patternIndex;

    return worstGain_dBi;

}

// /**
// FUNCTION :: AntennaPatternedGetBestGainAtThisDirection
// PURPOSE :: Get the best gain at given direction for patterned antenna
//
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyindex  : int   : phy index.
// + DOA       : Orientation : Given direction.
// RETURN :: double : The best gain at given direction for patterned antenna
// **/

static
double AntennaPatternedGetBestGainAtThisDirection(
     Node* node,
     int phyIndex,
     Orientation DOA)
{
    PhyData* phyData = node->phyData[phyIndex];
    AntennaPatterned* patterned =
        (AntennaPatterned *)phyData->antennaData->antennaVar;

    int i;
    double maxGain_dBi = ANTENNA_LOWEST_GAIN_dBi;
    int originalPatternIndex = patterned->patternIndex;
    int numPatterns = patterned->numPatterns;

    ERROR_Assert(phyData->antennaData->antennaModelType ==
        ANTENNA_PATTERNED ,
            "antennaModelType not patterned.\n");
    // get the best gain for it
    for (i = 0; i < numPatterns; i++)
    {
        double gain_dBi;

        AntennaPatternedSetPattern(node, phyIndex, i);
        gain_dBi =
            ANTENNA_PatternedGainForThisDirection(
                node,
                phyIndex,
                DOA);

        if (gain_dBi > maxGain_dBi)
        {
             maxGain_dBi = gain_dBi;
        }
    }

    patterned->patternIndex = originalPatternIndex;

    return maxGain_dBi;
}

// /**
// FUNCTION :: AntennaPatternedGetworstGainAtThisDirection
// PURPOSE :: Get the worst gain at given direction for patterned antenna
//
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyindex  : int   : phy index.
// + DOA       : Orientation : Given direction.
// RETURN :: double : The worst gain at given direction
// **/

static
double AntennaPatternedGetworstGainAtThisDirection(
     Node* node,
     int phyIndex,
     Orientation DOA)
{
    PhyData* phyData = node->phyData[phyIndex];
    AntennaPatterned* patterned =
        (AntennaPatterned *)phyData->antennaData->antennaVar;

    int i;
    double worstGain_dBi = patterned->antennaGain_dB;
    int originalPatternIndex = patterned->patternIndex;
    int numPatterns = patterned->numPatterns;

    ERROR_Assert(phyData->antennaData->antennaModelType ==
        ANTENNA_PATTERNED ,
            "antennaModelType not patterned.\n");
    // get the worst gain for it
    for (i = 0; i < numPatterns; i++)
    {
        double gain_dBi;

        AntennaPatternedSetPattern(node, phyIndex, i);
        gain_dBi =
            ANTENNA_PatternedGainForThisDirection(
                node,
                phyIndex,
                DOA);

        if (gain_dBi < worstGain_dBi)
        {
            worstGain_dBi = gain_dBi;
        }
    }

    patterned->patternIndex = originalPatternIndex;

    return worstGain_dBi;
}


// these are copied from phy.cpp
#define PACKET_SIZE      (1024 * 8) // 1kbytes
#define PACKET_ERROR_RATIO  0.1
// /**
// FUNCTION :: PHY_IsNodesReachable
// PURPOSE :: check if the nodes pair are reachable for given signal power
//
// PARAMETERS ::
// + txNode      : Node* : Pointer to tx node.
// + rxNode      : Node* : Pointer to rx node.
// + txModel     : PhyModel : Tx phy model
// + rxInterfaceIndex  : int   : interface index for the rx node.
// + channelIndex      : int   : index of the channel.
// + rxpower_mW      : double* : the signal power which is used to check if the nodes pair are reachable

// RETURN :: BOOL : if the nodes pair reachable
// **/

static
BOOL PHY_IsNodesReachable(Node* txNode,
                Node* rxNode,
                PhyModel txModel,
                int   rxInterfaceIndex,
                int   channelIndex,
                double *rxpower_mW)
{

    BOOL isReachable = FALSE;

    PhyData*        thisRadio;
    PhyModel         phyModel;

    thisRadio = rxNode->phyData[rxInterfaceIndex];
    phyModel = rxNode->phyData[rxInterfaceIndex]->phyModel;

    // Return false if nodes are in different PHY models
    // Note that we only get here if both rx and tx are listening
    // to this channel
    if (txModel != phyModel)
    {
        return isReachable;
    }

    // Check if it is reachable for given signal power for different PHY models
    switch (phyModel) {
        case PHY802_11a:
        case PHY802_11b:
        {
            PhyData802_11*   phy802_11 = NULL;
            phy802_11 = (PhyData802_11*)thisRadio->phyVar;

            if (*rxpower_mW >=
               phy802_11->rxSensitivity_mW[phy802_11->rxDataRateType])
            {
                isReachable = TRUE;
            }

            return isReachable;
            break;
        }

#ifdef CELLULAR_LIB
        case PHY_GSM:
        {
            PhyDataGsm*      phyGsm;
            phyGsm = (PhyDataGsm*) thisRadio->phyVar;

            if (*rxpower_mW >= phyGsm->rxSensitivity_mW)
            {
                isReachable = TRUE;
            }

            return isReachable;
            break;
        }
#endif // CELLULAR_LIB

        case PHY_ABSTRACT:
        {
            PhyDataAbstract* phyAbstract;
            phyAbstract = (PhyDataAbstract*)thisRadio->phyVar;

            if (*rxpower_mW >= phyAbstract->rxThreshold_mW)
            {
                isReachable = TRUE;
            }

            return isReachable;
            break;
        }
#ifdef SENSOR_NETWORKS_LIB
        case PHY802_15_4:
        {
            PhyData802_15_4* phy802_15_4;
            phy802_15_4 = (PhyData802_15_4*)thisRadio->phyVar;

            if (*rxpower_mW >= phy802_15_4->rxSensitivity_mW)
            {
                isReachable = TRUE;
            }

            return isReachable;
            break;
        }
#endif

#ifdef ADVANCED_WIRELESS_LIB
        case PHY802_16:
        {
            PhyDataDot16*    phy802_16 = NULL;
            phy802_16 = (PhyDataDot16*)thisRadio->phyVar;
            double rxPower_mW =
                *rxpower_mW * phy802_16->PowerLossDueToCPTime;

            if (rxPower_mW >= phy802_16->rxSensitivity_mW[0])
            {
                isReachable = TRUE;
            }

            return isReachable;
            break;
        }
#endif // ADVANCED_WIRELESS_LIB
#ifdef ADDON_BOEINGFCS
        case PHY_SRW_ABSTRACT:
        {
            PhyDataSrwAbstract* phySrwAbstract;
            phySrwAbstract = (PhyDataSrwAbstract*)thisRadio->phyVar;

            if (*rxpower_mW >= phySrwAbstract->rxSensitivity_mW)
            {
                isReachable = TRUE;
            }

            return isReachable;
            break;
        }
#endif // ADDON_BOEINGFCS

#ifdef LTE_LIB
        case PHY_LTE:
        {
            PhyDataLte* phyLte = (PhyDataLte*)thisRadio->phyVar;

            if (!PhyLteLessRxSensitivity(rxNode,
                                    rxInterfaceIndex,
                                    phyLte,
                                    *rxpower_mW))
            {
                isReachable = TRUE;
            }

            return isReachable;
            break;
        }
#endif // LTE_LIB

        default:
        {
            ERROR_ReportError("Unsupported Radio type.");
            return isReachable;
            break;
        }
    }

}

// /**
// FUNCTION :: PHY_ComputeRxPower
// PURPOSE :: Compute receiver power from tx side
//            Updates rxInfo
// PARAMETERS ::
// + partition   : PartitionData* : The current partition
// + txNode      : Node* : Pointer to the Tx node.
// + rxNode      : Node* : Pointer to the Rx node.
// + txInfo      : ListeningChannelNodeInfo* : Info for tx node
// + rxInfo      : ListeningChannelNodeInfo* : Info for rx node
// + channelIndex   : int     : Channel index.
// + rxPower_mw     : double* : Receiving power (output)
// + rxPower_mwWorst: double* : Receiving power in worst case (output)
// RETURN :: None
// **/
static
void PHY_ComputeRxPower(
    PartitionData* partition,
    Node* txNode,
    Node* rxNode,
    double pathloss_dB,
    const PHY_CONN_NodePositionData::ChannelNodeInfo* txInfo,
    const PHY_CONN_NodePositionData::ChannelNodeInfo* rxInfo,
    int channelIndex,
    double* rxPower_mw,
    double* rxPower_mwWorst)
{

    PropChannel* propChannel
        = &(txNode->partitionData->propChannel[channelIndex]);

    PropPathProfile profile;
    memset(&profile, 0, sizeof(PropPathProfile));

    COORD_CalcDistanceAndAngle(
                  txNode->partitionData->terrainData->getCoordinateSystem(),
                  &(profile.fromPosition),
                  &(profile.toPosition),
                  &(profile.distance),
                  //&(profile.propDelay),
                  &(profile.txDOA),
                  &(profile.rxDOA));

    // pathloss matrix file support
    if (pathloss_dB == NEGATIVE_PATHLOSS_dB) {
        if (propChannel->profile->pathlossModel == PL_MATRIX) {

            pathloss_dB =
                    PathlossMatrix(txNode,
                                   txNode->nodeId,
                                   rxNode->nodeId,
                                   channelIndex,
                                   txNode->getNodeTime());
        }
    }

    if (pathloss_dB == NEGATIVE_PATHLOSS_dB)
    {
        pathloss_dB = HUGE_PATHLOSS_VALUE;
    }

    if (pathloss_dB < 0.0)
    {
        pathloss_dB = 0.0;
    }

    double txAntennaGain_dBiBest;
    double txAntennaGain_dBiWorst;
    double txPower_mW = txInfo->txPower_mW;


    PhyData*        tx;
    AntennaOmnidirectional* txOmniDirectional;
    PhyModel         txPhyModel = PHY_NONE;
    PhyData802_11*   phy802_11 = NULL;
    PhyDataAbstract* phyAbstract = NULL;

    tx = txNode->phyData[txInfo->phyIndex];
    txPhyModel = tx->phyModel;


    // estimate the antenna gain for both best and worst case
    switch (tx->antennaData->antennaModelType)
    {
        case ANTENNA_OMNIDIRECTIONAL:
        {
            txOmniDirectional =
                 (AntennaOmnidirectional*)tx->antennaData->antennaVar;
            txAntennaGain_dBiBest = txOmniDirectional->antennaGain_dB;
            txAntennaGain_dBiWorst = txOmniDirectional->antennaGain_dB;

            break;
        }
        case ANTENNA_SWITCHED_BEAM:
        {
            txAntennaGain_dBiBest =
                    AntennaSwitchedBeamGetBestGainAtThisDirection(
                                    txNode,
                                    txInfo->phyIndex,
                                    profile.txDOA);

            txAntennaGain_dBiWorst =
                    AntennaSwitchedBeamGetWorstGainAtThisDirection(
                                    txNode,
                                    txInfo->phyIndex,
                                    profile.txDOA);
            // shift Tx power
            if (txPhyModel ==  PHY802_11a || txPhyModel == PHY802_11b)
            {
                phy802_11 = (PhyData802_11*)tx->phyVar;

                if (txAntennaGain_dBiBest >
                   phy802_11->directionalAntennaGain_dB)
                {
                txPower_mW =
                        NON_DB(IN_DB(txPower_mW) -
                        phy802_11->directionalAntennaGain_dB);
                }
            }
            else if (txPhyModel == PHY_ABSTRACT)
            {
                phyAbstract = (PhyDataAbstract*)tx->phyVar;

                if (txAntennaGain_dBiBest >
                   phyAbstract->directionalAntennaGain_dB)
                {
                txPower_mW =
                        NON_DB(IN_DB(txPower_mW) -
                        phyAbstract->directionalAntennaGain_dB);
                }
            }

            break;
        }
        case ANTENNA_STEERABLE:
        {
            txAntennaGain_dBiBest =
                    AntennaSteerableGetBestGainAtThisDirection(
                                    txNode,
                                    txInfo->phyIndex,
                                    profile.txDOA);

            txAntennaGain_dBiWorst =
                    AntennaSteerableGetWorstGainAtThisDirection(
                                    txNode,
                                    txInfo->phyIndex,
                                    profile.txDOA);
            // shift Tx power
            if (txPhyModel ==  PHY802_11a || txPhyModel == PHY802_11b)
            {
                phy802_11 = (PhyData802_11*)tx->phyVar;

                if (txAntennaGain_dBiBest >
                   phy802_11->directionalAntennaGain_dB)
                {
                txPower_mW =
                        NON_DB(IN_DB(txPower_mW) -
                        phy802_11->directionalAntennaGain_dB);
                }
            }
            else if (txPhyModel == PHY_ABSTRACT)
            {
                phyAbstract = (PhyDataAbstract*)tx->phyVar;
                if (txAntennaGain_dBiBest >
                   phyAbstract->directionalAntennaGain_dB)
                {
                txPower_mW =
                        NON_DB(IN_DB(txPower_mW) -
                        phyAbstract->directionalAntennaGain_dB);
                }
            }

            break;
        }
        case ANTENNA_PATTERNED:
        {
            txAntennaGain_dBiBest =
                    AntennaPatternedGetBestGainAtThisDirection(
                                    txNode,
                                    txInfo->phyIndex,
                                    profile.txDOA);

            txAntennaGain_dBiWorst =
                    AntennaPatternedGetworstGainAtThisDirection(
                                    txNode,
                                    txInfo->phyIndex,
                                    profile.txDOA);
            break;
        }

        default:
        {
            char err[MAX_STRING_LENGTH];
            sprintf(err, "Unknown ANTENNA-MODEL-TYPE %d.\n",
                 tx->antennaData->antennaModelType);
            ERROR_ReportError(err);
            break;
        }

    }

    // received power for best case

    double rxPower_dBm =
                IN_DB(txPower_mW) + txAntennaGain_dBiBest -
                txInfo->systemLossIndB - pathloss_dB -
                rxInfo->systemLossIndB + rxInfo->gain_dBi;

    *rxPower_mw = NON_DB(rxPower_dBm);

    // received power for worst case
    double rxPower_dBmWorst =
                IN_DB(txPower_mW) + txAntennaGain_dBiWorst -
                txInfo->systemLossIndB - pathloss_dB -
                rxInfo->systemLossIndB + rxInfo->gain_dBi;

    *rxPower_mwWorst = NON_DB(rxPower_dBmWorst);
}

// /**
// FUNCTION :: PHY_EstimateConnectivity
// PURPOSE :: Estimate the connectivity for the nodes pair
//
// PARAMETERS ::
// + partition   : PartitionData* : The current partition
// + txNode      : Node* : Pointer to the Tx node.
// + rxNode      : Node* : Pointer to the Rx node.
// + txModel     : PhyModel: Tx model
// + rxPhyIndex  : int   : Info for rx node
// + rxPower_mw  : double : Receiver power
// + rxPower_mwWorst : double : Receiver power in worst case
// + channelIndex      : int   : Channel index.
// + reachable         : BOOL  : Reachable
// + ReachableWorst    : BOOL  : Reachable for the worst case
// RETURN :: BOOL : The reachable result for the best case
// **/
static
void PHY_EstimateConnectivity(
    PartitionData* partition,
    Node* txNode,
    Node* rxNode,
    PhyModel txModel,
    int rxPhyIndex,
    int   channelIndex,
    double rxPower_mw,
    double rxPower_mwWorst,
    BOOL *reachable,
    BOOL *reachableWorst)
{
    // Check if reachable in best case
    *reachable = PHY_IsNodesReachable(
        txNode,
        rxNode,
        txModel,
        rxPhyIndex,
        channelIndex,
        &rxPower_mw);
    if (*reachable)
    {
        // Check if reachable for worst case
        *reachableWorst = PHY_IsNodesReachable(
            txNode,
            rxNode,
            txModel,
            rxPhyIndex,
            channelIndex,
            &rxPower_mwWorst);
    }
    else
    {
        // Not reachable => not reachable in worst case
        *reachableWorst = FALSE;
    }
}


void PHY_CONN_NodePositionData::HandleChannelPhyConnInsertion(
    Node* node,
    PartitionData* partition)
{
    // Already performed at this time:
    // Exchange position, listening channel information
    //
    // 4 steps:
    // Calculate rxPower at each tx node to each rx node
    // Exchange rxPower to all partitions
    // Calculate connectivity at each rx node from each tx node
    // Exchange connectivity to all partitions

    // Calculate pathloss from each tx node to each LOCAL rx node
    connectivity.clear();
    ListenableChannelNodeList::iterator channel_iter;
    for (channel_iter = listenableChannelNodeList.begin();
        channel_iter != listenableChannelNodeList.end();
        ++channel_iter)
    {
        int channelIndex = channel_iter->first;

        // local node information
        ListenableSet::iterator tx_node_iter;
        for (tx_node_iter = channel_iter->second.begin();
            tx_node_iter != channel_iter->second.end();
            tx_node_iter++)
        {
            // Get transmitting node only if on local partition
            // Only perform connectivity for tx node on local partition
            Node* txNode = NULL;
            PARTITION_ReturnNodePointer(
                    partition,
                    &txNode,
                    tx_node_iter->nodeId,
                    TRUE);
            ERROR_Assert(txNode, "Invalid transmitter");

            ListenableSet::iterator rx_node_iter;
            for (rx_node_iter = channel_iter->second.begin();
                rx_node_iter != channel_iter->second.end();
                rx_node_iter++)
            {
                if (tx_node_iter->nodeId == rx_node_iter->nodeId)
                {
                    //printf("tx nodeId %d == rx nodeId %d \n",
                    //    tx_node_iter->first, rx_node_iter->first);
                    continue;
                }

                Node* rxNode = NULL;
                PARTITION_ReturnNodePointer(
                    partition,
                    &rxNode,
                    rx_node_iter->nodeId,
                    FALSE);
                if (!rxNode)
                {
                    continue;
                }


                // Compute pathloss for rx nodes

                PropChannel* propChannel =
                    &(partition->propChannel[channelIndex]);
                double wavelength = propChannel->profile->wavelength;

                PropPathProfile profile;
                PHY_CONN_GetNodePosition(
                    partition, txNode, &(profile.fromPosition));
                PHY_CONN_GetNodePosition(
                    partition, rxNode, &(profile.toPosition));

                COORD_CalcDistanceAndAngle(
                  partition->terrainData->getCoordinateSystem(),
                  &(profile.fromPosition),
                  &(profile.toPosition),
                  &(profile.distance),
                  &(profile.txDOA),
                  &(profile.rxDOA));

                // estimate pathloss
                double pathloss_dB = 0;

                PROP_CalculatePathloss(
                    rxNode,
                    txNode->nodeId,
                    rxNode->nodeId,
                    channelIndex,
                    wavelength,
                    tx_node_iter->antennaHeight,
                    rx_node_iter->antennaHeight,
                    &profile,
                    &pathloss_dB);


                 // Add to receiver map
                HandlePhyConnPathlossInsertion(
                    partition,
                    txNode->nodeId,
                    tx_node_iter->phyIndex,
                    rxNode->nodeId,
                    rx_node_iter->phyIndex,
                    channelIndex,
                    pathloss_dB,
                    PHY_CONN_NodePositionData::WIRELESS,
                    PHY_CONN_NodePositionData::WIRELESS);


            }
        }
    }

    // Exchange pathloss to all partitions
    // Assigns new entries in the map

    connectivity.global_syncType = PATHLOSS;
    connectivity.Summarize(partition);

    // //
    // then compute rx power at each LOCAL tx node to each rx node

    ConnectivityMap::iterator txConn;
    for (txConn = connectivity.begin();
            txConn != connectivity.end();
            ++txConn)
    {
        //int channelIndex = tx_iter->first;
        Node *txNode = NULL;
        PARTITION_ReturnNodePointer(
                partition,
                &txNode,
                txConn->first,
                FALSE);
        if (!txNode)
        {
            continue;
        }

        TxConnectivity::iterator rxConn;
        for (rxConn = txConn->second.begin();
            rxConn != txConn->second.end();
            rxConn++)
        {
            // Get receiving node only if on local partition
            // Only perform connectivity for rx node on local partition
            Node *rxNode;
            PARTITION_ReturnNodePointer(
                partition,
                &rxNode,
                rxConn->first,
                TRUE);

            ERROR_Assert(rxNode, "Invalid receiver");

            // Loop over possible adjacencies
            AdjacencyList::iterator adj;
            for (adj = rxConn->second.begin();
                adj != rxConn->second.end();
                adj++)
            {


                // get sender's listenable info
                ListenableSet::iterator txInfo;
                BOOL found = FALSE;
                GetChannelNodeInfo(
                    txConn->first,
                    adj->first.senderIndex, adj->first.channelIndex,
                    &txInfo, &found);

                ERROR_Assert(found, "Error in rx power calculation.");

                ListenableSet::iterator rxInfo;
                GetChannelNodeInfo(
                    adj->first.rcverId,
                    adj->first.rcverIndex,
                    adj->first.channelIndex,
                    &rxInfo, &found);

                ERROR_Assert(found, "Error in rx power calculation.");

                // Compute power for rx nodes
                double rxPower_mw = 0;
                double rxPower_mwWorst = 0;
                PHY_ComputeRxPower(partition,
                    txNode,
                    rxNode,
                    adj->second.pathloss_dB,
                    &(*txInfo),
                    &(*rxInfo),
                    adj->first.channelIndex,
                    &rxPower_mw,
                    &rxPower_mwWorst);

                if (PHY_CONNECTIVITY_DEBUG)
                {
                    printf("rx power cal - p %d from node %d to node %d "
                        "on channel %d rxPower_mw %e\n",
                        partition->partitionId,
                        txConn->first,adj->first.rcverId,
                        adj->first.channelIndex, rxPower_mw);
                }

                if (rxPower_mw > 1.0e300)
                {
                    printf("infinity\n");
                }

                //if (rxPower_mw > 0)
                {
                    // calculate rx power from txnode to rx node
                    adj->second.rxPower_mw = rxPower_mw;
                    adj->second.rxPower_mwWorst = rxPower_mwWorst;
                    adj->second.syncType = RX_POWER_INFO;

                }

            }

        }
    }

    connectivity.global_syncType = RX_POWER_INFO;
    connectivity.Summarize(partition);

    // Calculate connectivity at each rx node from each tx node

    for (txConn = connectivity.begin();
        txConn != connectivity.end();
        ++txConn)
    {
        Node *txNode;
        PARTITION_ReturnNodePointer(
                partition,
                &txNode,
                txConn->first,
                TRUE);
        ERROR_Assert(txNode, "Invalid transmitting node");

        TxConnectivity::iterator rxConn;
        for (rxConn = txConn->second.begin();
            rxConn != txConn->second.end();
            rxConn++)
        {
            // Get receiving node only if on local partition
            // Only perform connectivity for rx node on local partition
            Node *rxNode;
            PARTITION_ReturnNodePointer(
                partition,
                &rxNode,
                rxConn->first,
                FALSE);
            if (!rxNode)
            {
                continue;
            }

            // Loop over possible adjacencies
            AdjacencyList::iterator adj;
            for (adj = rxConn->second.begin();
                adj != rxConn->second.end();
                adj++)
            {
                // Get sender antenna info
                ListenableSet::iterator info;
                BOOL found;
                GetChannelNodeInfo(
                    txNode->nodeId,
                    adj->first.senderIndex,
                    adj->first.channelIndex,
                    &info,
                    &found);
                ERROR_Assert(
                    found,
                    "No transmitter info");

                //  here we need to make up for the real rx gain dBi if
                //  patterned antenna is used
                //  TO Change: move the whole rx antenna gain here
                PhyData* rxPhy = rxNode->phyData[adj->first.rcverIndex];

                if (rxPhy->antennaData->antennaModelType ==
                    ANTENNA_PATTERNED)
                {
                    PropPathProfile profile;

                    PHY_CONN_GetNodePosition(partition, txNode, &(profile.fromPosition));
                    PHY_CONN_GetNodePosition(partition, rxNode, &(profile.toPosition));

                    COORD_CalcDistanceAndAngle(
                          rxNode->partitionData->terrainData->getCoordinateSystem(),
                          &(profile.fromPosition),
                          &(profile.toPosition),
                          &(profile.distance),
                          &(profile.txDOA),
                          &(profile.rxDOA));

                    double rx_PatternedGain_dBi =
                        AntennaPatternedGainForThisDirection(
                            rxNode, adj->first.rcverIndex, profile.rxDOA);

                    double rxPower_dBm = IN_DB(adj->second.rxPower_mw) + rx_PatternedGain_dBi;

                    adj->second.rxPower_mw = NON_DB(rxPower_dBm);

                }
                BOOL reachable = FALSE;
                BOOL reachableWorst = FALSE;

                PHY_EstimateConnectivity(partition,
                    txNode,
                    rxNode,
                    info->model,
                    adj->first.rcverIndex,
                    adj->first.channelIndex,
                    adj->second.rxPower_mw,
                    adj->second.rxPower_mwWorst,
                    &reachable,
                    &reachableWorst);
                if (reachable)
                {

                    adj->second.reachable = reachable;
                    adj->second.reachableWorst = reachableWorst;
                }

            }
        }
    }

    // Exchange connectivity to all partitions
    // Updates entries in the map with reachable/reachableWorst
    connectivity.global_syncType = CONNECTIVITY;
    connectivity.Summarize(partition);

    if (!partition->partitionId)
    {
        for (txConn = connectivity.begin();
            txConn != connectivity.end();
            ++txConn)
        {
            TxConnectivity::iterator rxConn;
            for (rxConn = txConn->second.begin();
                rxConn != txConn->second.end();
                rxConn++)
            {

                // Loop over possible adjacencies
                AdjacencyList::iterator adj;
                for (adj = rxConn->second.begin();
                    adj != rxConn->second.end();
                    adj++)
                {

                    if (adj->second.reachable)
                    {
#ifdef ADDON_DB
                        set<PHY_CONN_NodePositionData::ChannelNodeInfo >::iterator info;
                        BOOL found = FALSE;
                        GetChannelNodeInfo(
                            txConn->first,
                            adj->first.senderIndex,
                            adj->first.channelIndex,
                            &info,
                            &found);
                        ERROR_Assert(
                            found,
                            "No transmitter info");

                        StatsDBPhyConnParam phyParam;
                        phyParam.m_SenderId = txConn->first;
                        phyParam.m_ReceiverId = adj->first.rcverId;
                        phyParam.m_PhyIndex = adj->first.senderIndex;
                        phyParam.m_ReceiverPhyIndex = adj->first.rcverIndex;
                        phyParam.m_ChannelIndex = STATSDB_IntToString(adj->first.channelIndex);
                        BOOL s_listening = FALSE;
                        BOOL r_listening = FALSE;


                        FindSenderReceiverChannelListening(
                            this,
                            txConn->first,
                            adj->first.senderIndex,
                            adj->first.rcverId,
                            adj->first.rcverIndex,
                            adj->first.channelIndex,
                            s_listening,
                            r_listening);
                        phyParam.senderListening = s_listening;
                        phyParam.receiverListening = r_listening;
                        phyParam.antennaType = info->type;
                        phyParam.reachableWorst = adj->second.reachableWorst;

                        STATSDB_HandlePhyConnTableUpdate(node, phyParam);
#endif
                    }
                }
            }
        }
    }


    listenableChannelNodeList.clear();
    listeningChannelNodeList.clear();
    positions.clear();
}

void HandlePhyConnInsertion(Node* node,
                                   PartitionData* partition)
{
    if (PHY_CONNECTIVITY_DEBUG)
    {
        printf("HandlePhyConnInsertion node %d time %f \n",
            node->nodeId, (double)node->getNodeTime()/SECOND);
    }

    partition->nodePositionData->HandleChannelPhyConnInsertion(node, partition);

    HandleLinkPhyConnInsertion(node, partition);
    HandleSatComPhyConnInsertion(node, partition);
    Handle802_3PhyConnInsertion(node, partition);

}




 // example of usage
/*    vector<PHY_CONN_NodePositionData::AdjacencyNode >  adjNodeStru;
    PHY_CONN_ReturnPhyConnectivity(partition, 3, adjNodeStru);
    for (int i = 0; i < adjNodeStru.size(); ++i)
       printf("s %d r %d st%d rt%d \n", 3, adjNodeStru[i].rcverId,
          adjNodeStru[i].sType, adjNodeStru[i].rType);

    printf("Potential: 3->4 = %d \n", PHY_CONN_ReturnPotPhyConnectivity(
         partition, 3, 4));

    printf("3->4 = %d \n", PHY_CONN_ReturnPhyConnectivity(partition, 3, 4));
    */

//======================APIs

BOOL PHY_CONN_ReturnPotPhyConnectivity(
    PartitionData *partition, int txNodeId, int rxNodeId)
{
    PHY_CONN_NodePositionData *nodePositionData =
            partition->nodePositionData;

    // Check for txNodeId
    PHY_CONN_NodePositionData::ConnectivityMap::iterator it =
        nodePositionData->connectivity.find(txNodeId);
    if (it == nodePositionData->connectivity.end())
    {
        return FALSE;
    }

    // Check if txNodeId connected to rxNodeId
    PHY_CONN_NodePositionData::TxConnectivity::iterator it2 =
        it->second.find(rxNodeId);

    return it2 != it->second.end();
}

BOOL PHY_CONN_ReturnPhyConnectivity(
    PartitionData *partition, int txNodeId, int rxNodeId)
{
    PHY_CONN_NodePositionData *nodePositionData =
            partition->nodePositionData;

    // Check for txNodeId
    PHY_CONN_NodePositionData::ConnectivityMap::iterator it =
        nodePositionData->connectivity.find(txNodeId);
    if (it == nodePositionData->connectivity.end())
    {
        return FALSE;
    }

    // Check if txNodeId connected to rxNodeId
    PHY_CONN_NodePositionData::TxConnectivity::iterator it2 =
        it->second.find(rxNodeId);
    if (it2 == it->second.end())
    {
        return FALSE;
    }

    // Check if they are connected on some channel
    PHY_CONN_NodePositionData::AdjacencyList::iterator it3;
    for (it3 = it2->second.begin();
        it3 != it2->second.end();
        it3++)
    {
        if (it3->second.connected)
        {
            return TRUE;
        }
    }

    // Potential connectivity, but not listening to same channel
    return FALSE;
}

BOOL PHY_CONN_ReturnPhyConnectivity(PartitionData *partition,
    int txNodeId,
    vector<PairAdjacencyKeyValue>& adjNodeStru)
{
    PHY_CONN_NodePositionData *nodePositionData =
            partition->nodePositionData;

    adjNodeStru.clear();

    // First lookup sender
    PHY_CONN_NodePositionData::ConnectivityMap::iterator it =
        nodePositionData->connectivity.find(txNodeId);
    if (it == nodePositionData->connectivity.end())
    {
        return FALSE;
    }

    // Now iterate through every connected node
    PHY_CONN_NodePositionData::TxConnectivity::iterator it2;
    for (it2 = it->second.begin();
        it2 != it->second.end();
        it2++)
    {
        PHY_CONN_NodePositionData::AdjacencyList::iterator it3;
        for (it3 = it2->second.begin();
            it3 != it2->second.end();
            it3++)
        {
            adjNodeStru.push_back(*it3);
        }
    }

    return TRUE;
}


