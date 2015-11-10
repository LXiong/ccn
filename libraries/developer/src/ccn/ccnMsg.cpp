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

void CcnMsg::EncodeFullNameMsg() {
  this->msg_full_name = (uint64_t)this->msg_name << 32 | this->msg_chunk_num;
}

void CcnMsg::DecodeFullNameMsg() {
  this->msg_name  = this->msg_full_name >> 32;
  this->msg_chunk_num = this->msg_full_name & UINT32_MAX;
}

void CcnMsg::App_TcpSendCcnMsg(Node* node, Message* msg) {
  char* payload;
  payload = this->EncodePayload();

  TransportToAppOpenResult *openResult;
  openResult = (TransportToAppOpenResult*) MESSAGE_ReturnInfo(msg);

  Message* sendMsg;
  sendMsg = MESSAGE_Alloc(
            node,
            TRANSPORT_LAYER,
            TransportProtocol_TCP,
            MSG_TRANSPORT_FromAppSend);


  MESSAGE_PacketAlloc(node, sendMsg, sizeof(uint64_t), TRACE_CCN);
  memcpy(MESSAGE_ReturnPacket(sendMsg), payload, sizeof(uint64_t));

  MESSAGE_InfoAlloc(node, sendMsg, sizeof(AppToTcpSend));
  AppToTcpSend *sendRequest;
  sendRequest = (AppToTcpSend *) MESSAGE_ReturnInfo(sendMsg);
  sendRequest->connectionId = openResult->connectionId;
  sendRequest->ttl = IPDEFTTL;

  //Trace Information
  ActionData acnData;
  acnData.actionType = SEND;
  acnData.actionComment = NO_COMMENT;
  TRACE_PrintTrace(node, sendMsg, TRACE_APPLICATION_LAYER,
      PACKET_OUT, &acnData);

  //MESSAGE_AddVirtualPayload(node, sendMsg, this->payload_length - sizeof(uint32_t));
  MESSAGE_AddVirtualPayload(node, sendMsg, this->payload_length - sizeof(uint64_t));

  APP_TcpSend(node, sendMsg);
}

char* CcnMsg::EncodePayload() {
  char *payload;
  payload = (char*)MEM_malloc(this->payload_length);
  memcpy(&payload[0], &this->bufferMapKey, sizeof(uint64_t));

  return payload;
}


void CcnMsg::DecodePayload(char* payload) {
  memcpy(&this->bufferMapKey, &payload[0], sizeof(uint64_t));
}

void CcnMsg::DecodePayload(Message* msg) {
  this->DecodePayload((char*)MESSAGE_ReturnPacket(msg));
}

bool CcnMsg::checkHopCount() {
  this->hops_limit--;
  if(this->hops_limit < 1) {
    printf("hop limit over: this msg has deleted\n");
    return false;
  }
  else 
    return true;
}

void CcnMsg::printSize(Message* msg) {
  printf("\n--------------------------\n");
  printf("msg size is %d.\n", MESSAGE_ReturnPacketSize(msg));
  printf("msg virtual size is %d.\n", MESSAGE_ReturnVirtualPacketSize(msg));
  printf("msg actual size is %d.\n", MESSAGE_ReturnActualPacketSize(msg));
  printf("receivedMsg->msg_type is %d\n", this->msg_type);
  printf("--------------------------\n\n");
}

void CcnMsg::printState() {
  printf("\n--------------------------\n");
  printf("print ccn state\n");
  printf("msg type = %d\n", this->msg_type);
  printf("msg name = %d\n", this->msg_name);
  printf("msg chunk = %d\n", this->msg_chunk_num);
  printf("sender node id = %d\n", this->sender_node_id);
  printf("source node id = %d\n", this->source_node_id);
  printf("previous node id = %d\n", this->previous_node_id);
  printf("payload length = %d\n", this->payload_length);
  printf("--------------------------\n\n");
}
