#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fstream>

#include "api.h"
#include "partition.h"
#include "app_util.h"
#include "network_ip.h"

#include "ccn.h"
#include "nodeData.h"
#include "statistic.h"

// キャッシュヒット時
void NodeData::StatisticalInfo_cacheHit(Node* node, CcnMsg* ccnMsg) {
  StatisticalData* stat_data = this->statData;
  stat_data->cache_hitTimes++;
}

// CcnMsg送信時
void NodeData::StatisticalInfo_sendCcnMsg(Node* node, CcnMsg* ccnMsg)  {
  StatisticalData* stat_data = this->statData;

  stat_data->Packet_send++;
  stat_data->PacketSize_send += ccnMsg->payload_length;

  switch(ccnMsg->msg_type)
  {
    case Interest:
      stat_data->interestPacket_send++;
      break;
    case Data:
      stat_data->dataPacket_send++;
      break;
    case fakeInterest:
    case fakeData:
      break;
    default:
      printf("case is %d\n", ccnMsg->msg_type);
      ERROR_ReportError("ccnMsg->msg_type error: undefined eventype\n");
  }

  switch(ccnMsg->content_type)
  {
    case CommonData:
      stat_data->commonPacket_send++;
      stat_data->commonPacketSize_send += ccnMsg->payload_length;
      break;

    case VideoData:
      stat_data->videoPacket_send++;
      stat_data->videoPacketSize_send += ccnMsg->payload_length;
      break;

    default:
      printf("case is %d\n", ccnMsg->content_type);
      ERROR_ReportError("ccnMsg->content_type error: undefined eventype\n");
  }

}

// CcnMsg受信時
void NodeData::StatisticalInfo_recvCcnMsg(Node* node, CcnMsg* ccnMsg, clocktype sendToRecv_Delay)  {
  StatisticalData* stat_data = this->statData;

  stat_data->Packet_recv++;
  stat_data->PacketSize_recv  += ccnMsg->payload_length;
  stat_data->SendToRecv_Delay += sendToRecv_Delay;

  switch(ccnMsg->msg_type)
  {
    case Interest:
      stat_data->interestPacket_recv++;
      break;
    case Data:
      stat_data->dataPacket_recv++;
      break;
    case fakeInterest:
    case fakeData:
      break;
    default:
      printf("case is %d\n", ccnMsg->msg_type);
      ERROR_ReportError("ccnMsg->msg_type error: undefined eventype\n");
  }

  switch(ccnMsg->content_type)
  {
    case CommonData:
      stat_data->commonPacket_recv++;
      stat_data->commonPacketSize_recv += ccnMsg->payload_length;
      break;

    case VideoData:
      stat_data->videoPacket_recv++;
      stat_data->videoPacketSize_recv += ccnMsg->payload_length;
      break;

    default:
      printf("case is %d\n", ccnMsg->content_type);
      ERROR_ReportError("ccnMsg->content_type error: undefined eventype\n");
  }

  if(ccnMsg->content_type == CommonData) return;

  clocktype generateToRecv_Delay;
  generateToRecv_Delay = node->getNodeTime() - ccnMsg->data_genTime;
  if(generateToRecv_Delay > 0) {
    if(node->nodeId < 51) {
      stat_data->GenerateToRecv_Delay += generateToRecv_Delay;
    }
  }

  std::ofstream ofs(this->name_logFile.c_str(), ios::app);

  ofs << node->getNodeTime() << ",      " << generateToRecv_Delay << ",      " << sendToRecv_Delay << ",      " << ccnMsg->msg_chunk_num << ",             " << ccnMsg->resent_times << std::endl;

  ofs.close();
}

// CcnMsg受信時
void NodeData::StatisticalInfo_recvCcnMsg(Node* node, CcnMsg* ccnMsg)  {
  StatisticalData* stat_data = this->statData;

  stat_data->Packet_recv++;
  stat_data->PacketSize_recv += ccnMsg->payload_length;

  switch(ccnMsg->msg_type)
  {
    case Interest:
      stat_data->interestPacket_recv++;
      break;
    case Data:
      stat_data->dataPacket_recv++;
      break;
    case fakeInterest:
      stat_data->fakeInterestPacket_recv++;
      break;
    case fakeData:
      stat_data->fakeDataPacket_recv++;
      break;
  }

  switch(ccnMsg->content_type)
  {
    case CommonData:
      stat_data->commonPacket_recv++;
      stat_data->commonPacketSize_recv += ccnMsg->payload_length;
      break;

    case VideoData:
      stat_data->videoPacket_recv++;
      stat_data->videoPacketSize_recv += ccnMsg->payload_length;
      break;

    default:
      printf("case is %d\n", ccnMsg->content_type);
      ERROR_ReportError("ccnMsg->content_type error: undefined eventype\n");
  }

  if(ccnMsg->content_type == CommonData) return;

  clocktype generateToRecv_Delay;
  generateToRecv_Delay = node->getNodeTime() - ccnMsg->interest_genTime;
  if(generateToRecv_Delay > 0) {
    if(node->nodeId < 51)  {
      stat_data->GenerateToRecv_Delay += generateToRecv_Delay;
    }
  }

  //char fname[40];
  //sprintf(fname, "node/node%d.csv", node->nodeId);
  //std::ofstream ofs(fname, ios::app);
  //ofs << node->getNodeTime() << ",      " << generateToRecv_Delay << ",      " << ccnMsg->msg_chunk_num << std::endl;

  //ofs.close();
}

void NodeData::recvRequestData(Node* node, CcnMsg* ccnMsg) {
  StatisticalData* stat_data = this->statData;
  if(ccnMsg->content_type == CommonData) return;

  clocktype sendToRecv_Delay;
  clocktype generateToRecv_Delay;

  RequestMap_t::iterator it;
  it = reqMap.find(ccnMsg->msg_full_name);

  sendToRecv_Delay = node->getNodeTime() - it->second;
  generateToRecv_Delay = node->getNodeTime() - ccnMsg->data_genTime;

  std::ofstream ofs(this->name_logFile.c_str(), ios::app);
  ofs << node->getNodeTime() << ",      " << generateToRecv_Delay << ",      " << ccnMsg->msg_name << ",      " << ccnMsg->msg_chunk_num << ",  " << (int)ccnMsg->resent_times << std::endl;

  string file_name = "delay.csv";
  std::ofstream ofs1((this->name_logDirectory + file_name).c_str(), ios::app);
  if(ccnMsg->resent_times == 1) {
    ofs1 <<  node->nodeId << "," << node->getNodeTime() << ",      " << generateToRecv_Delay << ",      " << ccnMsg->msg_name << ",      " << ccnMsg->msg_chunk_num << ",  " << (int)ccnMsg->resent_times << std::endl;
  }

  ofs.close();
  ofs1.close();
}

void NodeData::StatisticalInfo_init() {
  this->statData = new StatisticalData();
}

void NodeData::StatisticalInfo_print_totalNodeInfo(Node* node, int maxActiveNode) {

  string fname;
  stringstream ss;
  ss << "node.csv";
  fname = this->name_logDirectory + ss.str();
  std::ofstream ofs(fname.c_str(), ios::app);

  ofs << "node->nodeId,"
    << "packet_send,packet_recv,"
    << "packetSize_send," 
    << "packetSize_recv,"
    << "sendToRecv_delay," 
    << "generateToRecv_delay," 
    << "cache_hitTimes," 
    << "packet_lostTimes," 
    << "commonPacket_recv,"
    << "videoPacket_recv," 
    << "commonPacketSize_recv,"
    << "videoPacketSize_recv,"
    << "request_commonPacket_recv,"
    << "request_videoPacket_recv," 
    << "request_commonPacketSize_recv,"
    << "request_videoPacketSize_recv,"
    << "_commonPacket_send,"
    << "_videoPacket_send," 
    << "_commonPacketSize_send,"
    << "_videoPacketSize_send,"
    << "Packet_send," 
    << "et_send," 
    << "Packet_recv,"
    << "et_recv,"
    << "fakeInterest_send," 
    << "fakeData_send," 
    << "fakeInterest_recv," 
    << "fakeData_recv,"
    << "cs_size" 
    << endl;

  Node* cur_node;
  Address cur_addr;
  NodeData* cur_nodeData;
  StatisticalData* stat_data;

  uint64_t total_Packet_send              = 0;
  uint64_t total_Packet_recv              = 0;
  uint64_t total_PacketSize_send          = 0;
  uint64_t total_PacketSize_recv          = 0;
  uint64_t total_SendToRecv_Delay         = 0;
  uint64_t total_GenerateToRecv_Delay     = 0;
  uint64_t total_cache_hitTimes           = 0;
  uint64_t total_packet_lostTimes         = 0;
  uint64_t total_commonPacket_recv        = 0;
  uint64_t total_videoPacket_recv         = 0;
  uint64_t total_commonPacketSize_recv    = 0;
  uint64_t total_videoPacketSize_recv     = 0;
  uint64_t total_request_commonPacket_recv        = 0;
  uint64_t total_request_videoPacket_recv         = 0;
  uint64_t total_request_commonPacketSize_recv    = 0;
  uint64_t total_request_videoPacketSize_recv     = 0;
  uint64_t total_commonPacket_send        = 0;
  uint64_t total_videoPacket_send         = 0;
  uint64_t total_commonPacketSize_send    = 0;
  uint64_t total_videoPacketSize_send     = 0;

  uint64_t total_interestPacket_send      = 0;
  uint64_t total_dataPacket_send          = 0;
  uint64_t total_interestPacket_recv      = 0;
  uint64_t total_dataPacket_recv          = 0;

  uint64_t total_fakeInterestPacket_send  = 0;
  uint64_t total_fakeDataPacket_send      = 0;
  uint64_t total_fakeInterestPacket_recv  = 0;
  uint64_t total_fakeDataPacket_recv      = 0;

  
  uint64_t cs_size;

  for(int nodeId = 1; nodeId <= maxActiveNode; nodeId++) {
    cur_node = MAPPING_GetNodePtrFromHash(GlobalNodeData::nodeHash, nodeId);
    cur_addr = MAPPING_GetDefaultInterfaceAddressInfoFromNodeId(cur_node, nodeId, NETWORK_IPV4);

    cur_nodeData = (NodeData*) cur_node->appData.nodeData;
    stat_data = cur_nodeData->statData;

    total_Packet_send              += stat_data->Packet_send; 
    total_Packet_recv              += stat_data->Packet_recv; 
    total_PacketSize_send          += stat_data->PacketSize_send; 
    total_PacketSize_recv          += stat_data->PacketSize_recv; 
    total_SendToRecv_Delay         += stat_data->SendToRecv_Delay; 
    total_GenerateToRecv_Delay     += stat_data->GenerateToRecv_Delay; 
    total_cache_hitTimes           += stat_data->cache_hitTimes; 
    total_packet_lostTimes         += stat_data->packet_lostTimes; 
    total_commonPacket_recv        += stat_data->commonPacket_recv; 
    total_videoPacket_recv         += stat_data->videoPacket_recv;
    total_commonPacketSize_recv    += stat_data->commonPacketSize_recv;
    total_videoPacketSize_recv     += stat_data->videoPacketSize_recv;
    total_request_commonPacket_recv        += stat_data->request_commonPacket_recv; 
    total_request_videoPacket_recv         += stat_data->request_videoPacket_recv;
    total_request_commonPacketSize_recv    += stat_data->request_commonPacketSize_recv;
    total_request_videoPacketSize_recv     += stat_data->request_videoPacketSize_recv;
    total_commonPacket_send        += stat_data->commonPacket_recv; 
    total_videoPacket_send         += stat_data->videoPacket_recv;
    total_commonPacketSize_send    += stat_data->commonPacketSize_recv;
    total_videoPacketSize_send     += stat_data->videoPacketSize_recv;
    
    total_interestPacket_send      += stat_data->interestPacket_send; 
    total_dataPacket_send          += stat_data->dataPacket_send; 
    total_interestPacket_recv      += stat_data->interestPacket_recv; 
    total_dataPacket_recv          += stat_data->dataPacket_recv; 

    total_fakeInterestPacket_send  += stat_data->fakeInterestPacket_send; 
    total_fakeDataPacket_send      += stat_data->fakeDataPacket_send; 
    total_fakeInterestPacket_recv  += stat_data->fakeInterestPacket_recv; 
    total_fakeDataPacket_recv      += stat_data->fakeDataPacket_recv;

    cs_size = 0;
    CsMap_t::iterator it;
    for(it = cur_nodeData->csMap.begin(); it != cur_nodeData->csMap.end(); it++) {
      cs_size += DEF_TCP_MSS;
    }

    ofs << nodeId << ","
      << stat_data->Packet_send << "," 
      << stat_data->Packet_recv << "," 
      << stat_data->PacketSize_send << "," 
      << stat_data->PacketSize_recv << "," 
      << stat_data->SendToRecv_Delay << "," 
      << stat_data->GenerateToRecv_Delay << "," 
      << stat_data->cache_hitTimes << "," 
      << stat_data->packet_lostTimes << "," 
      << stat_data->commonPacket_recv<< ","
      << stat_data->videoPacket_recv<< ","
      << stat_data->commonPacketSize_recv<< ","
      << stat_data->videoPacketSize_recv<< ","
      << stat_data->request_commonPacket_recv<< ","
      << stat_data->request_videoPacket_recv<< ","
      << stat_data->request_commonPacketSize_recv<< ","
      << stat_data->request_videoPacketSize_recv<< ","
      << stat_data->commonPacket_send<< ","
      << stat_data->videoPacket_send<< ","
      << stat_data->commonPacketSize_send<< ","
      << stat_data->videoPacketSize_send<< ","

      << stat_data->interestPacket_send << "," 
      << stat_data->dataPacket_send << "," 
      << stat_data->interestPacket_recv << "," 
      << stat_data->dataPacket_recv << "," 

      << stat_data->fakeInterestPacket_send << "," 
      << stat_data->fakeDataPacket_send << "," 
      << stat_data->fakeInterestPacket_recv << "," 
      << stat_data->fakeDataPacket_recv << "," 
      << cs_size 
      << endl; 
  }

  ofs << "total" << " ," 
    << total_Packet_send << "," 
    << total_Packet_recv << "," 
    << total_PacketSize_send << "," 
    << total_PacketSize_recv << "," 
    << total_SendToRecv_Delay << "," 
    << total_GenerateToRecv_Delay << "," 
    << total_cache_hitTimes << "," 
    << total_packet_lostTimes << "," 
    << total_commonPacket_recv << ","
    << total_videoPacket_recv     << ","
    << total_commonPacketSize_recv << ","
    << total_videoPacketSize_recv  << ","
    << total_request_commonPacket_recv << ","
    << total_request_videoPacket_recv     << ","
    << total_request_commonPacketSize_recv << ","
    << total_request_videoPacketSize_recv  << ","
    << total_commonPacket_send << ","
    << total_videoPacket_send     << ","
    << total_commonPacketSize_send << ","
    << total_videoPacketSize_send  << ","

    << total_interestPacket_send << "," 
    << total_dataPacket_send << "," 
    << total_interestPacket_recv << "," 
    << total_dataPacket_recv << "," 

    << total_fakeInterestPacket_send << "," 
    << total_fakeDataPacket_send << "," 
    << total_fakeInterestPacket_recv << "," 
    << total_fakeDataPacket_recv 
    << endl; 

  string fname1;
  stringstream ss1;
  ss1 << "node.csv";

  fname1 = this->specific_logDirectory + ss1.str();

  std::ofstream ofs1(fname1.c_str(), ios::app);

  ofs1  
    << total_cache_hitTimes << "," 
    << total_request_commonPacket_recv << "," 
    << total_packet_lostTimes 
    << endl; 
}
