#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fstream>

#include "api.h"
#include "partition.h"
#include "app_util.h"

#include "ccn.h"
#include "nodeData.h"
#include "network_ip.h"

#include <sys/stat.h>
#include <time.h>

#include "MT.h"

void NodeData::NodeInfo_init(Node* node, const NodeInput* nodeInput) {

  char str[MAX_STRING_LENGTH];
  BOOL wasFound = FALSE;
  char log_name[MAX_STRING_LENGTH];

  IO_ReadString(
      ANY_NODEID,
      ANY_ADDRESS,
      nodeInput,
      "CACHE_SIZE",
      &wasFound,
      str);
  if(!wasFound) {
    ERROR_ReportError("wasNotFound at nodeInput\n");
  }
  this->cs_cacheSize = atoi(str);
  mkdir("log", 0755);

  strcpy(log_name, str);
  strcat(log_name, "_");

  IO_ReadString(
      ANY_NODEID,
      ANY_ADDRESS,
      nodeInput,
      "ERROR_RATE",
      &wasFound,
      str);

  if(!wasFound) {
    ERROR_ReportError("wasNotFound at nodeInput\n");
  }

  this->error_rate = strtod(str, NULL);
  strcat(log_name, str);
  strcat(log_name, "_");

  IO_ReadString(
      ANY_NODEID,
      ANY_ADDRESS,
      nodeInput,
      "VIDEO_RATE",
      &wasFound,
      str);

  if(!wasFound) {
    ERROR_ReportError("wasNotFound at nodeInput\n");
  }

  this->video_rate = atoi(str);
  strcat(log_name, str);
  strcat(log_name, "_");

  IO_ReadString(
      ANY_NODEID,
      ANY_ADDRESS,
      nodeInput,
      "COMMON_RATE",
      &wasFound,
      str);

  if(!wasFound) {
    ERROR_ReportError("wasNotFound at nodeInput\n");
  }

  this->common_rate = atoi(str);
  strcat(log_name, str);
  strcat(log_name, "_");

  IO_ReadString(
      ANY_NODEID,
      ANY_ADDRESS,
      nodeInput,
      "CCN_METHOD",
      &wasFound,
      str);

  if(!wasFound) {
    ERROR_ReportError("wasNotFound at nodeInput\n");
  }

  if(!strcmp(str, "DEFAULT")) {
     this->ccn_method = DEFAULT;
  } else if(!strcmp(str, "PRIOR")) {
     this->ccn_method = PRIOR;
  } else if(!strcmp(str, "PROPOSAL")) {
     this->ccn_method = PROPOSAL;
  } else if(!strcmp(str, "DEFAULT_FAST")) {
     this->ccn_method = DEFAULT_FAST;
  } else if(!strcmp(str, "NO_VIDEO")) {
     this->ccn_method = NO_VIDEO;
  } else {
    ERROR_ReportError("Undefined ccn method\n");
  }
  strcat(log_name, str);

  mkdir("specific", 0755);
  this->specific_logDirectory = "specific/" + string(log_name);

  strcat(log_name, "_");
  IO_ReadString(
      ANY_NODEID,
      ANY_ADDRESS,
      nodeInput,
      "SEED",
      &wasFound,
      str);

  if(!wasFound) {
    ERROR_ReportError("wasNotFound at nodeInput\n");
  }
  mkdir("log", 0755);

  //strcat(log_name, str);

  string log_file2;
  log_file2 = log_name;
  log_file2 = "specific/" + log_file2;

  this->isDistReq = false;
  this->videoTimer = 0;
  this->sim_time = DEF_SIM_TIME;
  this->tcp_mss = DEF_TCP_MSS;
  this->max_chunk_num = UINT16_MAX;
  this->currentBufferNumber = 0;
  this->packetGenerateTime = SECOND * 8 * this->tcp_mss * 0.001 / this->video_rate;

  // 30 is average packets by name
  this->commonPacketGenerateTime = SECOND * 8 * this->tcp_mss * 0.001 / this->common_rate * 30;
  this->max_windowSize = 40;

  string log_file;
  log_file = log_name;
  log_file = "log/" + log_file;
  mkdir(log_file.c_str(), 0755);


  strcat(log_name, "/");

  stringstream ss1, ss2;
  ss1 << "log/";
  ss2 << "node" << node->nodeId << ".csv";

  this->name_logDirectory = ss1.str() + string(log_name);
  this->name_logFile      = ss1.str() + string(log_name) + ss2.str();

  this->StatisticalInfo_init();
}

void NodeData::make_reTransMsg(Node* node, CcnMsg* ccnMsg) {
  Message* timeoutMsg;
  AppTimer* info;
  timeoutMsg = MESSAGE_Alloc(
                          node,
                          APP_LAYER,
                          APP_CCN_HOST,
                          MSG_APP_TimerExpired);
  MESSAGE_InfoAlloc(node, timeoutMsg, sizeof(AppTimer));
  info = (AppTimer *) MESSAGE_ReturnInfo(timeoutMsg);
  info->connectionId = (uint64_t)ccnMsg->msg_full_name;
  info->sourcePort = APP_CCN_LISTEN_PORT;
  info->type = APP_TIMER_RE_SEND_PKT;
  
  map<uint64_t, Message*>::iterator it_msg;
  CcnMsgBufferMap_t::iterator it_ccnMsg;

  it_msg = this->cancel_reTransMsg_map.find(ccnMsg->msg_full_name);
  it_ccnMsg = this->reTrans_ccnMsgBuffer.find(ccnMsg->msg_full_name);

  if(it_msg != this->cancel_reTransMsg_map.end()) {
    printf("[node:%d]msg_full_name: %lu\n", node->nodeId, ccnMsg->msg_full_name);
    printf("[node%d][msg%lu] %d %d\n",node->nodeId, ccnMsg->msg_full_name, ccnMsg->msg_name, ccnMsg->msg_chunk_num);
    ERROR_ReportError("Unexpected data in cacnel_reTransMsg_map\n");
  }
  if(it_ccnMsg != this->reTrans_ccnMsgBuffer.end()) {
    printf("[node:%d]msg_full_name: %lu\n", node->nodeId, ccnMsg->msg_full_name);
    ERROR_ReportError("Unexpected data in reTrans_ccnMsgBuffer\n");
  }
  
  this->cancel_reTransMsg_map.insert(pair<uint64_t, Message*>(ccnMsg->msg_full_name, timeoutMsg));
  this->reTrans_ccnMsgBuffer.insert(pair<uint64_t, CcnMsg*>(ccnMsg->msg_full_name, this->NewCcnMsg(ccnMsg)));
  MESSAGE_Send(node, timeoutMsg, 2 * SECOND);
}

void NodeData::send_reTransMsg(Node* node, uint64_t msg_full_name) {
  map<uint64_t, Message*>::iterator it_msg;
  CcnMsgBufferMap_t::iterator it_ccnMsg;

  it_msg    = this->cancel_reTransMsg_map.find(msg_full_name);
  it_ccnMsg = this->reTrans_ccnMsgBuffer.find(msg_full_name);

  if(it_msg == this->cancel_reTransMsg_map.end()) {
    printf("[node:%d]msg_full_name: %lu\n", node->nodeId, msg_full_name);
    ERROR_ReportError("Not found resend msg ");
  }
  if(it_ccnMsg == this->reTrans_ccnMsgBuffer.end()) {
    printf("[node:%d]msg_full_name: %lu\n", node->nodeId, msg_full_name);
    ERROR_ReportError("Not found cancel ccnMsg");
  }


  CcnMsg* ccnMsg;
  ccnMsg = it_ccnMsg->second;
  ccnMsg->resent_times++;

  CcnMsg* copy_ccnMsg;
  copy_ccnMsg = this->NewCcnMsg(ccnMsg);
  

  if(ccnMsg->resent_times > 3) {
    if(ccnMsg->content_type == VideoData) {
      printf("[node%d][chunk_num%d] re-send limit over\n", node->nodeId, ccnMsg->msg_chunk_num);
      this->statData->packet_lostTimes++;  // this is not lost but unacquired
    }
    MESSAGE_CancelSelfMsg(node, it_msg->second);
    return;
  } 

  this->cancel_reTransMsg_map.erase(it_msg);
  this->reTrans_ccnMsgBuffer.erase(it_ccnMsg);

  this->make_reTransMsg(node, copy_ccnMsg);
  
  this->fibSend(node, ccnMsg);
}

void NodeData::Cancel_reTransMsg(Node* node, uint64_t msg_full_name) {
  map<uint64_t, Message*>::iterator it_msg;
  CcnMsgBufferMap_t::iterator it_ccnMsg;
  Message* timeoutMsg;

  it_msg = this->cancel_reTransMsg_map.find(msg_full_name);
  it_ccnMsg = this->reTrans_ccnMsgBuffer.find(msg_full_name);

  if(it_msg == this->cancel_reTransMsg_map.end()) {
    printf("[node%d][chunk_num%lu] \n", node->nodeId, msg_full_name);
    ERROR_ReportError("Not found cancel msg");
  }
  if(it_ccnMsg == this->reTrans_ccnMsgBuffer.end()) {
    printf("[node%d][chunk_num%lu] \n", node->nodeId, msg_full_name);
    ERROR_ReportError("Not found cancel ccnMsg");
  }

  timeoutMsg = it_msg->second;

  MESSAGE_CancelSelfMsg(node, timeoutMsg);

  this->cancel_reTransMsg_map.erase(it_msg);
  this->reTrans_ccnMsgBuffer.erase(it_ccnMsg);
}

uint64_t NodeData::BufferPush(Node* node, CcnMsg* ccnMsg) {
  uint64_t freeNum;
  CcnMsgBufferMap_t::iterator it;

  for(freeNum = this->currentBufferNumber + 1; freeNum != this->currentBufferNumber; freeNum++) {
    it = this->ccnMsgBuffer.find(freeNum);
    if(it == this->ccnMsgBuffer.end()) {
      this->ccnMsgBuffer.insert(pair<uint64_t, CcnMsg*>(freeNum, ccnMsg));
      this->currentBufferNumber = freeNum;
      return freeNum;
    }
  }

  ERROR_ReportError("No free space for ccnMsgBuffer\n");
}

CcnMsg* NodeData::BufferRetrieve_Client(Node* node, Message* msg) {
  TransportToAppOpenResult* openResult;
  openResult = (TransportToAppOpenResult*) MESSAGE_ReturnInfo(msg);

  CcnMsgBufferMap_t::iterator it;
  it = this->ccnMsgBuffer.find(openResult->uniqueId);

  if(it == this->ccnMsgBuffer.end()) {
    ERROR_ReportError("node open unexist msg from ccnMsgBuffer\n");
  }

  CcnMsg* ccnMsg;
  ccnMsg = it->second;
  this->ccnMsgBuffer.erase(it);

  return ccnMsg;
}

CcnMsg* NodeData::BufferRetrieve_Host(Node* node, Message* msg) {
  uint64_t ccnMsgMapKey;
  char* payload;
  payload = (char*)MESSAGE_ReturnPacket(msg);
  memcpy(&ccnMsgMapKey, &payload[0], sizeof(uint64_t));

  CcnMsgBufferMap_t::iterator it;
  it = this->ccnMsgBuffer.find(ccnMsgMapKey);

  if(it == this->ccnMsgBuffer.end()) {
    ERROR_ReportError("node requests unexist msg from ccnMsgBuffer\n");
  }
  CcnMsg* ccnMsg;
  ccnMsg = it->second;

  this->ccnMsgBuffer.erase(it);

  return ccnMsg;
}

void NodeData::tcpOpenConnection(Node* node, CcnMsg* ccnMsg, messageType msg_type, int remote_nodeId) {
  this->tcpOpenConnection(node, ccnMsg, msg_type, remote_nodeId, 0);
}

void NodeData::tcpOpenConnection(Node* node, CcnMsg* ccnMsg, messageType msg_type, int remote_nodeId, clocktype packetGenerateTime) {
  short free_port;
  Address node_addr;
  Node*   remote_node;
  Address remote_addr;

  free_port = APP_GetFreePort(node);
  node_addr = MAPPING_GetDefaultInterfaceAddressInfoFromNodeId(node, node->nodeId, NETWORK_IPV4);
  remote_node = MAPPING_GetNodePtrFromHash(GlobalNodeData::nodeHash, remote_nodeId);
  remote_addr = MAPPING_GetDefaultInterfaceAddressInfoFromNodeId(remote_node, remote_nodeId, NETWORK_IPV4);

  ccnMsg->previous_node_id = node->nodeId;

  NodeData* remote_nodeData;
  remote_nodeData = (NodeData*)remote_node->appData.nodeData;
  ccnMsg->bufferMapKey = remote_nodeData->BufferPush(remote_node, ccnMsg);
  
  uint64_t uniqueId;
  uniqueId = this->BufferPush(node, ccnMsg);

  APP_TcpOpenConnection(node, APP_CCN_CLIENT, node_addr, free_port, remote_addr, APP_CCN_LISTEN_PORT, uniqueId, packetGenerateTime);
}

void NodeData::fibSend_DEFAULT(Node* node, CcnMsg* ccnMsg) {
  FibMap_t::iterator it;
  it = this->fibMap.find(ccnMsg->source_node_id);

  if(it == this->fibMap.end()) {
    ERROR_ReportError("error: next node was not found in FIB table\n");
  } 
  this->tcpOpenConnection(node, ccnMsg, Interest, it->second);
}

void NodeData::fibSend_PRIOR(Node* node, CcnMsg* ccnMsg) {
  FibMap_t::iterator it;
  it = this->fibMap.find(ccnMsg->source_node_id);

  if(it == this->fibMap.end()) {
    ERROR_ReportError("error: next node was not found in FIB table\n");
  } 
  this->tcpOpenConnection(node, ccnMsg, Interest, it->second);
}

void NodeData::fibSend_PROPOSAL(Node* node, CcnMsg* ccnMsg) {
  FibMap_t::iterator it;
  it = this->fibMap.find(ccnMsg->source_node_id);

  if(it == this->fibMap.end()) {
    ERROR_ReportError("error: next node was not found in FIB table\n");
  } 
  this->tcpOpenConnection(node, ccnMsg, Interest, it->second);
}

void NodeData::fibSend(Node* node, CcnMsg* ccnMsg) {
  FibMap_t::iterator it;
  it = this->fibMap.find(ccnMsg->source_node_id);

  if(it == this->fibMap.end()) {
    printf("Destination node is %d\n", ccnMsg->source_node_id);
    ERROR_ReportError("error: next node was not found in FIB table\n");
  } 
  this->tcpOpenConnection(node, ccnMsg, Interest, it->second);
}

int NodeData::fibFind(Node* node, CcnMsg* ccnMsg) {
  FibMap_t::iterator it;
  it = this->fibMap.find(ccnMsg->source_node_id);

  if(it == this->fibMap.end()) {
    ERROR_ReportError("error: next node was not found in FIB table\n");
  } 
  return it->second;
}

void NodeData::fibModInsert(Node* node, CcnMsg* ccnMsg, int nodeId) {
  pair<FibModMap_t::iterator, FibModMap_t::iterator> its = fibModMap.equal_range(ccnMsg->msg_name);

  for(FibModMap_t::iterator it = its.first; it != its.second; it++) {
    if(nodeId == it->second) return;
  }
  this->fibModMap.insert(pair<uint32_t, int>(ccnMsg->msg_name, nodeId));
  return;
}

bool NodeData::fibModSend(Node* node, CcnMsg* ccnMsg) {
  pair<FibModMap_t::iterator, FibModMap_t::iterator> its = fibModMap.equal_range(ccnMsg->msg_name);

  for(FibModMap_t::iterator it = its.first; it != its.second; it++) {
    this->tcpOpenConnection(node, NewCcnMsg(ccnMsg), fakeInterest, it->second);

    // 送信数リストに追加
    this->recvModMapInsert(node, ccnMsg);
  }
  return true;
}

bool NodeData::fibModSend(Node* node, CcnMsg* ccnMsg, clocktype send_delayTime) {
  pair<FibModMap_t::iterator, FibModMap_t::iterator> its = fibModMap.equal_range(ccnMsg->msg_name);

  for(FibModMap_t::iterator it = its.first; it != its.second; it++) {
    this->tcpOpenConnection(node, NewCcnMsg(ccnMsg), fakeInterest, it->second, send_delayTime);

    // 送信数リストに追加
    this->recvModMapInsert(node, ccnMsg);
  }
  return true;
}

bool NodeData::pitModInsert(Node* node, CcnMsg* ccnMsg) {
  this->pitModMap.insert(pair<uint64_t, int>(ccnMsg->msg_full_name, ccnMsg->previous_node_id));
  return true;
}

bool NodeData::pitModSend(Node* node, CcnMsg* ccnMsg) {
  PitModMap_t::iterator it;
  it = this->pitModMap.find(ccnMsg->msg_full_name);
  this->recvModMapRemove(node, ccnMsg);

  // 最初のfakeDataを受け取ったら転送
  if(isNotSent_fakeData(node, ccnMsg)) {
    if(it == this->pitModMap.end()) {
      ERROR_ReportError("error: next node info is not found in in PIT\n");
    }
//if(this->node_role == RELAY_NODE)
    //printf("[node%d]->[node%d][msg%lu] forward fakeData %d %d\n",node->nodeId, it->second, ccnMsg->msg_full_name, ccnMsg->msg_name, ccnMsg->msg_chunk_num);
    this->tcpOpenConnection(node, NewCcnMsg(ccnMsg), fakeData, it->second);
  }

  // すべてのfakeDataを受け取ったらpitModMapを削除
  if(isRecvAll_fakeData(node, ccnMsg)) {
    it = this->pitModMap.find(ccnMsg->msg_full_name);
    this->pitModMap.erase(it);

//if(this->node_role == RELAY_NODE)
    //printf("[node%d][msg%lu] deletemsg %d %d\n",node->nodeId, ccnMsg->msg_full_name, ccnMsg->msg_name, ccnMsg->msg_chunk_num);
    this->csDelete(ccnMsg->msg_full_name);
    this->lruDelete(ccnMsg->msg_full_name);
  }
  return true;
}

void NodeData::csModInsert(Node* node, CcnMsg* ccnMsg) {
  this->csInsert(ccnMsg->msg_full_name);
}
//
//bool NodeData::csModSend(Node* node, CcnMsg* ccnMsg) {
//  CsMap_t::iterator it;
//  it = this->csMap.find(ccnMsg->msg_full_name);
//
//  if(it == this->csMap.end()) {
//    //no msg data in this CS
//    return false;
//  } else {
//    //this->tcpOpenConnection(node, ccnMsg, trueData, ccnMsg->previous_node_id);
//    it->second++;
//    return true;
//  }
//}

void NodeData::reqModMapInput(Node* node, CcnMsg* ccnMsg) {
  this->reqModMap.insert(pair<uint32_t, uint32_t>(ccnMsg->msg_name, 0));
}

bool NodeData::reqModMapSearch(Node* node, CcnMsg* ccnMsg) {
  RequestModMap_t::iterator it;
  it = this->reqModMap.find(ccnMsg->msg_name);

  if(it != this->reqModMap.end()) {
    return true;
  } else {
    //リクエストなし
    return false;
  }
}

void NodeData::recvModMapInsert(Node* node, CcnMsg* ccnMsg) {
  RecvMap_t::iterator it;
  it = this->recvModMap.find(ccnMsg->msg_full_name);

  RecvInfo* recvInfo;
  if(it == this->recvModMap.end()) {
    recvInfo = new RecvInfo();

    this->recvModMap.insert(pair<uint64_t, RecvInfo*>(ccnMsg->msg_full_name, recvInfo));
  } else {
    recvInfo = it->second;
    recvInfo->clientNum++;
    recvInfo->isUsed = false;;
  }
}

void NodeData::recvModMapRemove(Node* node, CcnMsg* ccnMsg) {
  RecvMap_t::iterator it;
  it = this->recvModMap.find(ccnMsg->msg_full_name);

  RecvInfo* recvInfo;
  if(it != this->recvModMap.end()) {
    recvInfo = it->second;
    if(recvInfo->clientNum == 0) 
      return;
    recvInfo->clientNum--;
  } else {
    assert(FALSE);
  }
}

bool NodeData::isNotSent_fakeData(Node* node, CcnMsg* ccnMsg) {
  RecvMap_t::iterator it;
  it = this->recvModMap.find(ccnMsg->msg_full_name);

  RecvInfo* recvInfo;
  if(it != this->recvModMap.end()) {
    recvInfo = it->second;

    if(recvInfo->isUsed == false) {
      recvInfo->isUsed = true;
      return true;
    } else {
      return false;
    }
  } else {
    assert(FALSE);
  }
}

bool NodeData::isRecvAll_fakeData(Node* node, CcnMsg* ccnMsg) {
  RecvMap_t::iterator it;
  it = this->recvModMap.find(ccnMsg->msg_full_name);

  RecvInfo* recvInfo;
  if(it != this->recvModMap.end()) {
    recvInfo = it->second;

    if(recvInfo->clientNum == 0) {
      return true;
    } else {
//if(node->nodeId == 55) 
//printf("[msg:%d %d] clientNum%d\n", ccnMsg->msg_name, ccnMsg->msg_chunk_num, recvInfo->clientNum);
      return false;
    }
  } else {
    return true;
  }
}


void NodeData::convertInterest_intoData(CcnMsg* ccnMsg) {
  //ccnMsg->ccn_method       = ccnMsg->ccn_method;
  ccnMsg->msg_type         = Data;
  //ccnMsg->msg_name         = ccnMsg->msg_name;
  //ccnMsg->msg_chunk_num    = ccnMsg->msg_chunk_num;
  //ccnMsg->msg_full_name    = ccnMsg->msg_full_name;
  //ccnMsg->sender_node_id   = ccnMsg->sender_node_id;
  //ccnMsg->source_node_id   = ccnMsg->source_node_id;
  //ccnMsg->previous_node_id = ccnMsg->previous_node_id;
  ccnMsg->payload_length   = this->tcp_mss;
  //ccnMsg->hops_limit       = ccnMsg->hops_limit;
  //ccnMsg->interest_genTime = ccnMsg->interest_genTime;
  //ccnMsg->data_genTime     = ccnMsg->data_genTime;
  //ccnMsg->content_type     = ccnMsg->content_type;
  //ccnMsg->end_chunk_num    = ccnMsg->end_chunk_num;
}

void NodeData::convertFakeInterest_intoFakeData(CcnMsg* ccnMsg) {
  //ccnMsg->ccn_method       = ccnMsg->ccn_method;
  ccnMsg->msg_type         = fakeData;
  //ccnMsg->msg_name         = ccnMsg->msg_name;
  //ccnMsg->msg_chunk_num    = ccnMsg->msg_chunk_num;
  //ccnMsg->msg_full_name    = ccnMsg->msg_full_name;
  //ccnMsg->sender_node_id   = ccnMsg->sender_node_id;
  //ccnMsg->source_node_id   = ccnMsg->source_node_id;
  //ccnMsg->previous_node_id = ccnMsg->previous_node_id;
  ccnMsg->payload_length   = 30;
  //ccnMsg->hops_limit       = ccnMsg->hops_limit;
  //ccnMsg->interest_genTime = ccnMsg->interest_genTime;
  //ccnMsg->data_genTime     = ccnMsg->data_genTime;
  //ccnMsg->content_type     = ccnMsg->content_type;
  //ccnMsg->end_chunk_num    = ccnMsg->end_chunk_num;
}

CcnMsg* NodeData::NewCcnMsg(CcnMsg* ccnMsg) {
  CcnMsg* copy_CcnMsg;
  copy_CcnMsg = new CcnMsg();
  memcpy(copy_CcnMsg, ccnMsg, sizeof(*ccnMsg));

  return copy_CcnMsg;
}

bool NodeData::pitSend_DEFAULT(Node* node, CcnMsg* ccnMsg, clocktype send_delayTime) {
  pair<PitMap_t::iterator, PitMap_t::iterator> its;

  its = pitMap.equal_range(ccnMsg->msg_full_name);
  for(PitMap_t::iterator it = its.first; it != its.second; it++) {
    this->tcpOpenConnection(node, NewCcnMsg(ccnMsg), Data, it->second, send_delayTime);
  }

  its = pitMap.equal_range(ccnMsg->msg_full_name);
  pitMap.erase(its.first, its.second);

  return true;
}

bool NodeData::pitSend_DEFAULT(Node* node, CcnMsg* ccnMsg) {
  pair<PitMap_t::iterator, PitMap_t::iterator> its;

  its = pitMap.equal_range(ccnMsg->msg_full_name);
  for(PitMap_t::iterator it = its.first; it != its.second; it++) {
    this->tcpOpenConnection(node, NewCcnMsg(ccnMsg), Data, it->second);
  }

  its = pitMap.equal_range(ccnMsg->msg_full_name);
  pitMap.erase(its.first, its.second);

  return true;
}

bool NodeData::pitSend_PRIOR(Node* node, CcnMsg* ccnMsg) {
  pair<p_PitMap_t::iterator, p_PitMap_t::iterator> its;

  its = p_pitMap.equal_range(ccnMsg->msg_name);
  for(p_PitMap_t::iterator it = its.first; it != its.second; it++) {
    this->tcpOpenConnection(node, NewCcnMsg(ccnMsg), Data, it->second);
  }

  return true;
}

bool NodeData::pitSend_PRIOR(Node* node, CcnMsg* ccnMsg, clocktype send_delayTime) {
  pair<p_PitMap_t::iterator, p_PitMap_t::iterator> its;

  its = p_pitMap.equal_range(ccnMsg->msg_name);
  for(p_PitMap_t::iterator it = its.first; it != its.second; it++) {
    this->tcpOpenConnection(node, NewCcnMsg(ccnMsg), Data, it->second, send_delayTime);
  }

  return true;
}

bool NodeData::pitSend_PROPOSAL(Node* node, CcnMsg* ccnMsg) {
  pair<PitMap_t::iterator, PitMap_t::iterator> its;

  its = pitMap.equal_range(ccnMsg->msg_full_name);
  for(PitMap_t::iterator it = its.first; it != its.second; it++) {
    this->tcpOpenConnection(node, NewCcnMsg(ccnMsg), Data, it->second);
  }

  return true;
}

bool NodeData::pitSend_PROPOSAL(Node* node, CcnMsg* ccnMsg, clocktype send_delayTime) {
  pair<PitMap_t::iterator, PitMap_t::iterator> its;

  its = pitMap.equal_range(ccnMsg->msg_full_name);
  for(PitMap_t::iterator it = its.first; it != its.second; it++) {
    this->tcpOpenConnection(node, NewCcnMsg(ccnMsg), Data, it->second, send_delayTime);
  }

  return true;
}

bool NodeData::pitSend(Node* node, CcnMsg* ccnMsg) {
  switch(ccnMsg->ccn_method)
  {
    case DEFAULT:
      return pitSend_DEFAULT(node, ccnMsg);
      break;
    case PRIOR:
      return pitSend_PRIOR(node, ccnMsg);
      break;
    default:
      break;
  }
}

bool NodeData::pitInsert_PROPOSAL(Node* node, CcnMsg* ccnMsg) {
  PitMap_t::iterator it;

  it = this->pitMap.find(ccnMsg->msg_full_name);
  this->pitMap.insert(pair<uint64_t, int>(ccnMsg->msg_full_name, ccnMsg->previous_node_id));

  if(it != this->pitMap.end()) {
    return true;
  } else {
    return false;
  }
}

bool NodeData::pitInsert_PRIOR(Node* node, CcnMsg* ccnMsg) {
  p_PitMap_t::iterator it;

  it = this->p_pitMap.find(ccnMsg->msg_name);
  this->p_pitMap.insert(pair<uint32_t, int>(ccnMsg->msg_name, ccnMsg->previous_node_id));

  if(it != this->p_pitMap.end()) {
    return true;
  } else {
    return false;
  }
}

bool NodeData::pitInsert_DEFAULT(Node* node, CcnMsg* ccnMsg) {
  PitMap_t::iterator it;

  it = this->pitMap.find(ccnMsg->msg_full_name);
  this->pitMap.insert(pair<uint64_t, int>(ccnMsg->msg_full_name, ccnMsg->previous_node_id));

  if(it != this->pitMap.end()) {
    return true;
  } else {
    return false;
  }
}

bool NodeData::pitInsert(Node* node, CcnMsg* ccnMsg) {
  PitMap_t::iterator it;

  it = this->pitMap.find(ccnMsg->msg_full_name);
  this->pitMap.insert(pair<uint64_t, int>(ccnMsg->msg_full_name, ccnMsg->previous_node_id));

  if(it != this->pitMap.end()) {
    return true;
  } else {
    return false;
  }
}

void NodeData::csDelete(uint64_t msg_full_name) {
  CsMap_t::iterator it = this->csMap.find(msg_full_name);
  if(it != this->csMap.end()) {
    this->csMap.erase(it);
  } else {
    printf("msg_full_name = %lu\n", msg_full_name);
    ERROR_ReportError("not found\n");
  }
}

void NodeData::csInsert_DEFAULT(Node* node, CcnMsg* ccnMsg) {
  this->csMap.insert(pair<uint64_t, int>(ccnMsg->msg_full_name, 1));

  this->csLru.insert(this->csLru.begin(), ccnMsg->msg_full_name);
  if(this->csLru.size() > this->cs_cacheSize) {
    uint64_t delete_msg;
    delete_msg = this->csLru.at(this->csLru.size() - 1);
    this->csDelete(delete_msg);

    this->csLru.pop_back();
  }
}

void NodeData::csInsert(Node* node, CcnMsg* ccnMsg) {
  this->csMap.insert(pair<uint64_t, int>(ccnMsg->msg_full_name, 1));

  this->csLru.insert(this->csLru.begin(), ccnMsg->msg_full_name);
  if(this->csLru.size() > this->cs_cacheSize) {
    uint64_t delete_msg;
    delete_msg = this->csLru.at(this->csLru.size() - 1);
    this->csDelete(delete_msg);

    this->csLru.pop_back();
  }
}

void NodeData::csInsert(uint64_t msg_full_name) {
  CsMap_t::iterator it = this->csMap.find(msg_full_name);
  if(it != this->csMap.end()) {
    it->second++;
    this->lruRefresh(msg_full_name);
  } else {
    this->csMap.insert(pair<uint64_t, int>(msg_full_name, 1));
    this->csLru.insert(this->csLru.begin(), msg_full_name);

    if(this->csLru.size() > this->cs_cacheSize) {
      uint64_t delete_msg;
      delete_msg = this->csLru.at(this->csLru.size() - 1);
      this->csDelete(delete_msg);

      this->csLru.pop_back();
    }
  }

}

void NodeData::lruDelete(uint64_t msg_full_name) {
  CsLru_t::iterator it;
  for(it = this->csLru.begin(); it != this->csLru.end(); it++) {
    if(*it == msg_full_name) {
      this->csLru.erase(it);
      return;
    }
  }
  //ERROR_ReportError("LRU delete error\n");
}


void NodeData::lruRefresh(uint64_t msg_full_name) {
  CsLru_t::iterator it;
  for(it = this->csLru.begin(); it != this->csLru.end(); it++) {
    if(*it == msg_full_name) {
      this->csLru.erase(it);
      this->csLru.insert(this->csLru.begin(), msg_full_name);
      return;
    }
  }
  ERROR_ReportError("LRU refresh error\n");
}

bool NodeData::csSend(Node* node, CcnMsg* ccnMsg) {
  CsMap_t::iterator it;
  it = this->csMap.find(ccnMsg->msg_full_name);

  if(it != this->csMap.end()) {
    this->lruRefresh(ccnMsg->msg_full_name);

    this->convertInterest_intoData(ccnMsg);
    this->pitSend_DEFAULT(node, ccnMsg);
    //printf("[node%d]csSend occured\n", node->nodeId);

    return true;
  } else {
    return false;
  }
}

void NodeData::csPrint(Node* node) {
  CsMap_t::iterator it;
  for(it = this->csMap.begin(); it != this->csMap.end(); it++) {
    printf("[node%d] %lu\n", node->nodeId, it->first);
  }
}

void NodeData::reqMapInput(Node* node, CcnMsg* ccnMsg) {
  this->reqMap.insert(pair<uint64_t, clocktype>(ccnMsg->msg_full_name, node->getNodeTime()));
}

void NodeData::reqMapInput_PRIOR(Node* node, CcnMsg* ccnMsg) {
  this->p_reqMap.insert(pair<uint32_t, clocktype>(ccnMsg->msg_name, node->getNodeTime()));
}

bool NodeData::reqMapSearch_PRIOR(Node* node, CcnMsg* ccnMsg) {
  p_RequestMap_t::iterator it;
  it = this->p_reqMap.find(ccnMsg->msg_name);

  if(it != this->p_reqMap.end()) {
    //this->StatisticalInfo_recvCcnMsg(node, ccnMsg, node->getNodeTime() - it->second);
    return true;
  } else {
    return false;
  }
}

bool NodeData::reqMapSearch(Node* node, CcnMsg* ccnMsg) {
  RequestMap_t::iterator it;
  it = this->reqMap.find(ccnMsg->msg_full_name);

  if(it != this->reqMap.end()) {
    //this->StatisticalInfo_recvCcnMsg(node, ccnMsg, node->getNodeTime() - it->second);
    // reqmap.erase does wrong action when packet loss occured.
    this->reqMap.erase(it);
    return true;
  } else {
    return false;
  }
}

void NodeData::fibMapInput(Node* node, const char* filename) {
  ifstream ifs;
  ifs.open(filename);

  if(!ifs) {
    ERROR_ReportError("error: cannot open file\n");
  }
  
  int sourceNodeId, destNodeId, nextNodeId;
  string str;
  while(getline(ifs, str)) {
    sscanf(str.data(), "%d:%d %d\n", &sourceNodeId, &destNodeId, &nextNodeId);
    if(sourceNodeId != node->nodeId) {
      continue;
    }
    this->fibMap.insert(pair<int, int>(destNodeId, nextNodeId));
  }
  ifs.close();

printf("sorceNodeID  ");
  cout<< sourceNodeId <<endl;
  FibMap_t::iterator itr;
  for(itr=fibMap.begin();itr!=fibMap.end();itr++)
    {
printf("キー  ");
      cout<< itr->first << endl;
printf("値  ");
      cout<< itr->second << endl;
    }
}

void NodeData::release_windowSize(Node* node, CcnMsg* ccnMsg) {
  Cur_WindowSize_t::iterator it;

  it = this->cur_windowSize.find(ccnMsg->msg_name);
  if(it != this->cur_windowSize.end()) {
    if(it->second < 0) {
      ERROR_ReportError("error: window size is Zero\n");
    }
    it->second--;
  } else {
    printf("[node%d][msg%lu] release window size error\n", node->nodeId, ccnMsg->msg_full_name);
    ERROR_ReportError("error: no window size\n");
  }
}

uint8_t NodeData::return_windowSize(CcnMsg* ccnMsg) {
  Cur_WindowSize_t::iterator it;

  it = this->cur_windowSize.find(ccnMsg->msg_name);
  if(it != this->cur_windowSize.end()) {
    return it->second;
  } else {
    ERROR_ReportError("no chunk information\n");
  }
}

bool NodeData::set_windowSize(Node* node, CcnMsg* ccnMsg) {
  Cur_WindowSize_t::iterator it;
  uint8_t windowSize;

  it = this->cur_windowSize.find(ccnMsg->msg_name);
  if(it == this->cur_windowSize.end()) {
    windowSize = 1;
  } else {
    if(it->second < this->max_windowSize) {
      windowSize = it->second + 1;
      this->cur_windowSize.erase(it);
    } else {
      return false;
    }
  }

  this->cur_windowSize.insert(pair<uint32_t, uint8_t>(ccnMsg->msg_name, windowSize));

  if(windowSize < this->max_windowSize) {
    return true;
  } else {
    return false;
  }
}

void NodeData::set_lastChunkNum(CcnMsg* ccnMsg) {
  this->set_lastChunkNum(ccnMsg->msg_name, ccnMsg->msg_chunk_num);
}

void NodeData::set_lastChunkNum(uint32_t msg_name, uint32_t chunk_num) {
  map<uint32_t, uint32_t>::iterator it;
  it = this->last_chunk_num.find(msg_name);

  if(it == this->last_chunk_num.end()) {
  } else {
    this->last_chunk_num.erase(it);
  }
  this->last_chunk_num.insert(pair<uint32_t, uint32_t>(msg_name, chunk_num));
}

uint32_t NodeData::return_lastChunkNum(CcnMsg* ccnMsg) {
  return this->return_lastChunkNum(ccnMsg->msg_name, ccnMsg->msg_chunk_num);
}

uint32_t NodeData::return_lastChunkNum(uint32_t msg_name, uint32_t chunk_num) {
  map<uint32_t, uint32_t>::iterator it;
  it = this->last_chunk_num.find(msg_name);

  if(it != this->last_chunk_num.end()) {
    return it->second;
  } else {
    set_lastChunkNum(msg_name, chunk_num);
    return 0;
  }
}

void NodeData::set_lastRecvMsg(CcnMsg* ccnMsg) {
  this->set_lastRecvMsg(ccnMsg->msg_name, ccnMsg->msg_chunk_num);
}

void NodeData::set_lastRecvMsg(uint32_t msg_name, uint32_t chunk_num) {
  map<uint32_t, uint32_t>::iterator it;
  it = this->last_recv_msg.find(msg_name);

  if(it == this->last_recv_msg.end()) {
  } else {
    this->last_recv_msg.erase(it);
  }
  this->last_recv_msg.insert(pair<uint32_t, uint32_t>(msg_name, chunk_num));
}

uint32_t NodeData::return_lastRecvMsg(CcnMsg* ccnMsg) {
  return this->return_lastRecvMsg(ccnMsg->msg_name, ccnMsg->msg_chunk_num);
}

uint32_t NodeData::return_lastRecvMsg(uint32_t msg_name, uint32_t chunk_num) {
  map<uint32_t, uint32_t>::iterator it;
  it = this->last_recv_msg.find(msg_name);

  if(it == this->last_recv_msg.end()) {
    return UINT16_MAX - 8;
  } else {
    return it->second;
    //this->last_recv_msg.erase(it);
  }
  //this->last_recv_msg.insert(pair<uint16_t, uint16_t>(msg_name, chunk_num));
}

bool NodeData::check_missingChunkNum(CcnMsg* ccnMsg) {
  return this->check_missingChunkNum(ccnMsg->msg_name, ccnMsg->msg_chunk_num);
}

bool NodeData::check_missingChunkNum(uint32_t msg_name, uint32_t chunk_num) {
  map<uint32_t, uint32_t>::iterator it;
  it = this->last_recv_msg.find(msg_name);

  if(it != this->last_recv_msg.end()) {
    if(chunk_num == it->second + 1) {
      return true;
    } else {
      return false;
    }
  } else {
    return true;
  }
}

void NodeData::NodeInfo_init() {
  this->isDistReq = false;
  this->video_rate = DEF_VIDEO_BITRATE;
  this->video_rate = 500;
  this->videoTimer = 0;
  this->sim_time = DEF_SIM_TIME;
  this->tcp_mss = DEF_TCP_MSS;
  this->max_chunk_num = UINT16_MAX;
  this->currentBufferNumber = 0;
  this->packetGenerateTime = SECOND * 8 * this->tcp_mss * 0.001 / this->video_rate;
  this->max_windowSize = 40;
}

clocktype NodeData::return_packetGenerateTime(CcnMsg* ccnMsg) {
  return this->return_packetGenerateTime(ccnMsg->msg_chunk_num);
}

clocktype NodeData::return_packetGenerateTime(uint32_t chunk_num) {
  return (clocktype) packetGenerateTime * chunk_num;
}

clocktype NodeData::return_packetRandomGenerateTime() {
  return (clocktype) (this->packetGenerateTime * (0.5 + (rand() * (1.5 - 0.5 + 1.0) / (1.0 + RAND_MAX))));
}

#define file_size 10000

// CommonDataの名前を決定するためのZipf分布初期化
void GlobalNodeData::ZipfInit() {

	  //fujiwara
		  printf("ZipInit\n");
  int i;
  double sum = 0.0;
  for(i = 1; i < file_size; i++) {
    sum += 1.0 / (double) i;
  }

  for(i = 0; i < file_size; i++) {
    if(i == 0) 
      GlobalNodeData::Zipf[i] = (1 / ((double) i + 1.0)) / sum;
    else
      GlobalNodeData::Zipf[i] = (1 / ((double) i + 1.0)) / sum + GlobalNodeData::Zipf[i - 1];
  }
}

// Zipf分布によるランダムなCommonData名前決定
uint16_t GlobalNodeData::return_MsgName() {
  double target;
  uint32_t top, bottom, mid;

  static BOOL INIT_RAND = false;
  if(!INIT_RAND) {
    init_genrand((unsigned)time(NULL));
    INIT_RAND = true;
  }
  target = genrand_real1();
  top = file_size;

  while(bottom < top) {
    mid = (bottom + top) / 2;
    if(this->Zipf[(uint16_t)mid] == target) {
      break;
    } else if(this->Zipf[(uint16_t)mid] < target) {
      bottom  = mid + 1;
    } else { 
      top = mid - 1;
    }
    if(mid == 0) break;
    if(mid == file_size) break;
  }

  // 動画コンテンツ名前と重複する場合、ずらす
  if(mid == 10000) mid++;

  return (uint32_t)mid;
}

// ノード番号最大値を記録

void GlobalNodeData::setMaxNodeId(int nodeId) {
  if(this->maximumActiveNodeNum < nodeId) {
    this->maximumActiveNodeNum = nodeId;
  }
}
