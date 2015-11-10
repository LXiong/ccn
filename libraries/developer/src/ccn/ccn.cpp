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

int source_node_id;
GlobalNodeData* global_node_data;
uint32_t msg_videoData_name = 20000;

void AppCCNNodeInit(Node* node, NodeRole role, const NodeInput *nodeInput)
{

  char str[MAX_STRING_LENGTH];
  BOOL wasFound = FALSE;


  if(global_node_data == NULL) {


    global_node_data = new GlobalNodeData();
  }

  global_node_data->setMaxNodeId(node->nodeId);



  // nodeDataポインタをnodeに保存
  // nodeDataはノードでのccnMsgの取り扱いを担う
  NodeData* nodeData;
  nodeData = (NodeData*) node->appData.nodeData;
  if(nodeData == NULL) {
    nodeData = new NodeData(node, nodeInput);
    node->appData.nodeData = nodeData;
  }
  nodeData->node_role = role;

  //fujiwara
  		  printf("before FIB_INPUT\n");

  //FIB input
  nodeData->fibMapInput(node, "fib.conf");

  // ハンドラの登録
  APP_RegisterNewApp(node, APP_CCN_HOST, nodeData);

 // TCP受信準備
  Address node_addr;
  node_addr = MAPPING_GetDefaultInterfaceAddressInfoFromNodeId(node, node->nodeId, NETWORK_IPV4);
  APP_TcpServerListen(node, APP_CCN_HOST, node_addr, (short) APP_CCN_LISTEN_PORT);
  
  std::ofstream ofs(nodeData->name_logFile.c_str());
  ofs << "node->getNodeTime()" << ",      " << "generateToRecv_Delay" << ",      " << "sendToRecv_Delay" << ",      " << "ccnMsg->msg_chunk_num" << std::endl;


  // ノードの役割ごとに初期設定
  switch(role) 
  {
    // クライアント
    case CLIENT: 
    {
      // timer message送信
      // ライブストリーミング
      if(nodeData->ccn_method != NO_VIDEO) {
        APP_SetTimer(node, APP_CCN_CLIENT, 0, APP_CCN_LISTEN_PORT, APP_TIMER_SEND_PKT, 800 * MILLI_SECOND);
      }

      // 通常コンテンツ
      APP_SetTimer(node, APP_CCN_CLIENT, 0, APP_CCN_LISTEN_PORT, APP_TIMER_REGULAR_SEND_PKT, 1 * SECOND + 10 * MILLI_SECOND * node->nodeId);
      break;
    }

    // 中継ノード
    case RELAY_NODE:
    {
      break;
    }

    // ソースノード
    case SERVER:
    {
      source_node_id = node->nodeId;
      
      uint32_t chunk_num;
      clocktype generateTime;

      generateTime = 0;

      double randNum;
      for(chunk_num = 0; chunk_num < 100000; chunk_num++) {
        generateTime += nodeData->return_packetRandomGenerateTime();
        nodeData->dataGenerateTime_map.insert(pair<uint32_t, clocktype>(chunk_num, generateTime));

        randNum = (double) (1.0 / (RAND_MAX + 1.0)) * rand();
        nodeData->randomNum_map.insert(pair<uint32_t, double>(chunk_num, randNum));
      }
      break;
    }

    default:
      ERROR_ReportError("");
  }
}

void AppLayerCCNClient(Node* node, Message* msg) {
  NodeData* nodeData;
  nodeData = (NodeData*)node->appData.nodeData;

  Node* source_node;
  source_node = MAPPING_GetNodePtrFromHash(GlobalNodeData::nodeHash, source_node_id);
  nodeData->source_nodeData = (NodeData*)source_node->appData.nodeData;
  NodeData* source_nodeData;
  source_nodeData = nodeData->source_nodeData;

  switch(msg->eventType) 
  {
    // タイマーメッセージ
    case MSG_APP_TimerExpired:        // タイマーメッセージ
    {
      AppTimer* timer;
      timer = (AppTimer *)MESSAGE_ReturnInfo(msg);

      // timer messageの種類によって処理を分ける
      switch(timer->type) 
      {
        case APP_TIMER_SEND_PKT:
        {
          clocktype send_delayTime;
          uint32_t chunk_num;
          map<uint32_t, clocktype>::iterator it;

          chunk_num = 0;
          do {
            chunk_num++; 
            it = source_nodeData->dataGenerateTime_map.find(chunk_num);
            if(it == source_nodeData->dataGenerateTime_map.end()) {
              ERROR_ReportError("dataGenerateTime_map error\n");
            }
            send_delayTime = it->second - node->getNodeTime();
          } while(send_delayTime < 0);
          nodeData->set_lastChunkNum(msg_videoData_name + node->nodeId % 3, chunk_num - 1);
          nodeData->set_lastRecvMsg(msg_videoData_name + node->nodeId % 3, chunk_num - 1);

          CcnMsg* ccnMsg;
          do {
            ccnMsg = new CcnMsg();
            ccnMsg->ccn_method = nodeData->ccn_method;
            ccnMsg->resent_times = 0;
            ccnMsg->msg_type = Interest;
            ccnMsg->msg_name = msg_videoData_name + node->nodeId % 3;
            ccnMsg->msg_chunk_num = nodeData->return_lastChunkNum(ccnMsg) + 1;
            ccnMsg->EncodeFullNameMsg();
            ccnMsg->sender_node_id = node->nodeId;
            ccnMsg->source_node_id = source_node_id;
            ccnMsg->payload_length = 30;
            ccnMsg->hops_limit = 20;
            ccnMsg->interest_genTime = node->getNodeTime();
            ccnMsg->content_type = VideoData;
            ccnMsg->end_chunk_num = UINT32_MAX;

            it = source_nodeData->dataGenerateTime_map.find(ccnMsg->msg_chunk_num);
            ccnMsg->data_genTime = it->second;

            nodeData->set_lastChunkNum(ccnMsg);
            nodeData->fibSend(node, ccnMsg);
            if(ccnMsg->ccn_method == DEFAULT) {
              nodeData->make_reTransMsg(node, ccnMsg);
              nodeData->set_lastRecvMsg(msg_videoData_name + node->nodeId % 3, UINT32_MAX - 3);
              nodeData->reqMapInput(node, ccnMsg);
            }
            else if(ccnMsg->ccn_method == DEFAULT_FAST) {
              nodeData->make_reTransMsg(node, ccnMsg);
              nodeData->set_lastRecvMsg(msg_videoData_name + node->nodeId % 3, UINT32_MAX - 3);
              nodeData->reqMapInput(node, ccnMsg);
            }
            else if(ccnMsg->ccn_method == PRIOR) {
              nodeData->reqMapInput_PRIOR(node, ccnMsg);
              break;
            }
            else if(ccnMsg->ccn_method == PROPOSAL) {
              nodeData->set_lastRecvMsg(msg_videoData_name + node->nodeId % 3, UINT32_MAX - 1);
              nodeData->reqMapInput(node, ccnMsg);
              //printf("[node%d][msg%d] sendInterest\n", node->nodeId, ccnMsg->msg_name);
              break;
            }
          } while(nodeData->set_windowSize(node, ccnMsg));
          break;
        }  // APP_TIMER_SEND_PKT
        
        case APP_TIMER_DATA_SEND_PKT:
        {
          CcnMsg* ccnMsg;
          for(int i = 0; i < 3; i++) {
            ccnMsg = new CcnMsg();
            ccnMsg->resent_times = 0;
            ccnMsg->ccn_method = PRIOR;
            ccnMsg->msg_type = Data;
            ccnMsg->msg_name = msg_videoData_name + i;
            ccnMsg->msg_chunk_num = nodeData->return_lastChunkNum(ccnMsg) + 1;
            ccnMsg->EncodeFullNameMsg();
            ccnMsg->sender_node_id = node->nodeId;
            ccnMsg->source_node_id = source_node_id;
            ccnMsg->payload_length = nodeData->tcp_mss;
            ccnMsg->hops_limit = 20;
            ccnMsg->interest_genTime = node->getNodeTime();
            ccnMsg->content_type = VideoData;
            ccnMsg->end_chunk_num = UINT32_MAX;
            
            map<uint32_t, clocktype>::iterator it;
            it = nodeData->dataGenerateTime_map.find(ccnMsg->msg_chunk_num);
            ccnMsg->data_genTime = it->second;
    
            clocktype send_delayTime;
            send_delayTime = ccnMsg->data_genTime - node->getNodeTime();
            if(send_delayTime < 0) send_delayTime = 0;

            nodeData->set_lastChunkNum(ccnMsg);
            nodeData->pitSend_PRIOR(node, nodeData->NewCcnMsg(ccnMsg), send_delayTime);
            APP_SetTimer(node, APP_CCN_CLIENT, 0, APP_CCN_LISTEN_PORT, APP_TIMER_DATA_SEND_PKT, send_delayTime);
          }
          delete ccnMsg;
          break;
        }  // APP_TIMER_DATA_SEND_PKT

        case APP_TIMER_fakeINTEREST_SEND_PKT:
        {
          // 動画の配信分をすべて出す
          CcnMsg* ccnMsg;
          for(int i = 0; i < 3; i++) {
            ccnMsg = new CcnMsg();
            ccnMsg->resent_times = 0;
            ccnMsg->ccn_method = PROPOSAL;
            ccnMsg->msg_type = fakeInterest;
            ccnMsg->msg_name = msg_videoData_name + i;
            ccnMsg->msg_chunk_num = nodeData->return_lastChunkNum(ccnMsg) + 1;
            ccnMsg->EncodeFullNameMsg();
            ccnMsg->sender_node_id = node->nodeId;
            ccnMsg->source_node_id = source_node_id;
            ccnMsg->payload_length = nodeData->tcp_mss;
            ccnMsg->hops_limit = 20;
            ccnMsg->interest_genTime = node->getNodeTime();
            ccnMsg->content_type = VideoData;
            ccnMsg->end_chunk_num = UINT32_MAX;
            
            map<uint32_t, clocktype>::iterator it;
            it = nodeData->dataGenerateTime_map.find(ccnMsg->msg_chunk_num);
            ccnMsg->data_genTime = it->second;

            clocktype send_delayTime;
            send_delayTime = ccnMsg->data_genTime - node->getNodeTime();
            if(send_delayTime < 0) send_delayTime = 0;

            nodeData->set_lastChunkNum(ccnMsg);
            nodeData->fibModSend(node, nodeData->NewCcnMsg(ccnMsg), send_delayTime);
            APP_SetTimer(node, APP_CCN_CLIENT, 0, APP_CCN_LISTEN_PORT, APP_TIMER_fakeINTEREST_SEND_PKT, send_delayTime);
          }
          delete ccnMsg;
          break;
        }  // APP_TIMER_fakeINTEREST_SEND_PKT

        case APP_TIMER_REGULAR_SEND_PKT:
        {
          clocktype send_delayTime;
          uint32_t chunk_num;
          map<uint32_t, clocktype>::iterator it;
          uint32_t next_msgName = global_node_data->return_MsgName();
          uint32_t end_chunk_num = next_msgName % 40 + 10;

          CcnMsg* ccnMsg;
          do {
            ccnMsg = new CcnMsg();
            ccnMsg->resent_times = 0;
            ccnMsg->ccn_method = DEFAULT;
            ccnMsg->msg_type = Interest;
            ccnMsg->msg_name = next_msgName;
            ccnMsg->msg_chunk_num = nodeData->return_lastChunkNum(ccnMsg) + 1;
            ccnMsg->EncodeFullNameMsg();
            ccnMsg->sender_node_id = node->nodeId;
            ccnMsg->source_node_id = source_node_id;
            ccnMsg->payload_length = 30;
            ccnMsg->hops_limit = 20;
            ccnMsg->content_type = CommonData;
            ccnMsg->end_chunk_num = end_chunk_num;
            ccnMsg->interest_genTime = node->getNodeTime();
            ccnMsg->data_genTime     = node->getNodeTime();

            nodeData->set_lastChunkNum(ccnMsg);
            nodeData->reqMapInput(node, ccnMsg);
            nodeData->fibSend_DEFAULT(node, ccnMsg);
            nodeData->make_reTransMsg(node, ccnMsg);

            if(ccnMsg->msg_chunk_num > ccnMsg->end_chunk_num) break;
          } while(nodeData->set_windowSize(node, ccnMsg));

          APP_SetTimer(node, APP_CCN_CLIENT, 0, APP_CCN_LISTEN_PORT, APP_TIMER_REGULAR_SEND_PKT, nodeData->commonPacketGenerateTime);
          break;
        } // APP_TIMER_REGULAR_SEND_PKT

        default:
          ERROR_ReportError("Undefined timer type\n");
      }
      break;
    } // MSG_APP_TimerExpired

    case MSG_APP_FromTransOpenResult:  // TCPでactive open処理が完了
    {
      // データ送信処理 
      // 送信データの操作は出来る限りHostの処理にしておく
      // ここでは送信処理だけ
      TransportToAppOpenResult *openResult;
      openResult = (TransportToAppOpenResult*) MESSAGE_ReturnInfo(msg);

      //TCPコネクションが不成立時(失敗)
      if (openResult->connectionId < 0)
      {
        char buf[MAX_STRING_LENGTH];
        ctoa(node->getNodeTime(),buf);
        node->appData.numAppTcpFailure++;
        break;
      }

      // バッファーからccnメッセージを取り出し
      CcnMsg* ccnMsg;
      ccnMsg = nodeData->BufferRetrieve_Client(node, msg);

      nodeData->StatisticalInfo_sendCcnMsg(node, ccnMsg);

      ccnMsg->App_TcpSendCcnMsg(node, msg);
      break;
    } // MSG_APP_FromTransOpenResult

    case MSG_APP_FromTransDataSent:    // TCPでデータ転送終了
      TransportToAppDataSent *openResult;
      openResult = (TransportToAppDataSent*) MESSAGE_ReturnInfo(msg);

      APP_TcpCloseConnection(node, openResult->connectionId);

      break;
    case MSG_APP_FromTransCloseResult:
      break;
    default:
      ERROR_ReportError("msg->eventType error: undefined eventType\n");
  }
  MESSAGE_Free(node, msg);
}


void AppLayerCCNHost(Node* node, Message* msg) {
  NodeData* nodeData;
  nodeData = (NodeData*)node->appData.nodeData;

  switch(msg->eventType) 
  {
    case MSG_APP_FromTransDataReceived: // データ受け取り時
    {
      TransportToAppDataReceived *openResult;
      openResult = (TransportToAppDataReceived*) MESSAGE_ReturnInfo(msg);
      APP_TcpCloseConnection(node, openResult->connectionId);

      // msgからccnメッセージを取得
      // ccnメッセージを処理
      // データ送信の場合、TCP接続要求を送る

      // msgのキーからccnメッセージを取り出す
      CcnMsg* ccnMsg;
      ccnMsg = nodeData->BufferRetrieve_Host(node, msg);

      Node* remote_node;
      NodeData* remote_nodeData;

      remote_node = MAPPING_GetNodePtrFromHash(GlobalNodeData::nodeHash, ccnMsg->previous_node_id);
      remote_nodeData = (NodeData*)remote_node->appData.nodeData;

      // パケットロストによる取得エラー
      if(nodeData->node_role + remote_nodeData->node_role == CLIENT + RELAY_NODE) {
        if((rand() * 100.0) < (nodeData->error_rate * RAND_MAX)) {
          //printf("packet lost occured at %d->%d\n", remote_node->nodeId, node->nodeId);
          //nodeData->statData->packet_lostTimes++;
          if(ccnMsg->ccn_method == PROPOSAL && 
              (ccnMsg->msg_type == Interest || ccnMsg->msg_type == Data)){
          } else if(ccnMsg->ccn_method == PRIOR) {
            nodeData->statData->packet_lostTimes++;  // this is not lost but unacquired
            break;
          } else {
            break;
          }
        }
      }

      nodeData->StatisticalInfo_recvCcnMsg(node, ccnMsg);

      // ホップカウント(loop検出用)
      if(!ccnMsg->checkHopCount()) {
        printf("hop count is over\n");
        break;
      }

      switch(ccnMsg->ccn_method) 
      {

        case DEFAULT:
        case DEFAULT_FAST:
        {
          switch(ccnMsg->msg_type) {
            case Interest:
            {
              // ソースノードまで転送
              // PITに名前を記録(msg_full_name)
              // ソースノードならDataパケットを返送
              // 返送する際はPITを参照
              
              if(nodeData->pitInsert_DEFAULT(node, ccnMsg)) {
                // PITで登録済み
                delete ccnMsg;
                break;
              }

              if(nodeData->csSend(node, ccnMsg) == true) {
                // キャッシュヒット
                if(ccnMsg->content_type == CommonData) {
                  nodeData->StatisticalInfo_cacheHit(node, ccnMsg);
                }
                break;
              }

              // ソースノード到着
              if(ccnMsg->source_node_id == node->nodeId) {
                // InterestをDataに変換
                nodeData->convertInterest_intoData(ccnMsg);

                // Dataの送信時間設定
                clocktype send_delayTime;

                if(ccnMsg->content_type == VideoData) {
                  map<uint32_t, clocktype>::iterator it;
                  it = nodeData->dataGenerateTime_map.find(ccnMsg->msg_chunk_num);
                  send_delayTime = it->second - node->getNodeTime();
                } else {
                  send_delayTime = 0;
                }

                // PITで設定時間に則った送信
                if(send_delayTime < 0) {
                  nodeData->pitSend_DEFAULT(node, ccnMsg);
                } else {
                  nodeData->pitSend_DEFAULT(node, ccnMsg, send_delayTime);
                }
              }
              else {
                // FIBによるフォワーディング
                nodeData->fibSend_DEFAULT(node, ccnMsg);
              }
              break;
            } // Interest

            case Data:
            {
              //if(ccnMsg->content_type == CommonData) {
              //  nodeData->statData->commonPacket_recv++;
              //  nodeData->statData->commonPacketSize_recv += ccnMsg->payload_length;
              //} else if(ccnMsg->content_type == VideoData) {
              //  nodeData->statData->videoPacket_recv++;
              //  nodeData->statData->videoPacketSize_recv += ccnMsg->payload_length;
              //}

              if(nodeData->reqMapSearch(node, ccnMsg)) {
                if(ccnMsg->content_type == CommonData) {
                  nodeData->statData->request_commonPacket_recv++;
                  nodeData->statData->request_commonPacketSize_recv += ccnMsg->payload_length;
                } else if(ccnMsg->content_type == VideoData) {
                  nodeData->statData->request_videoPacket_recv++;
                  nodeData->statData->request_videoPacketSize_recv += ccnMsg->payload_length;
                }
                
                // PROPOSAL再送での誤爆回避
                if(nodeData->ccn_method == PROPOSAL) {
                  if(ccnMsg->content_type == VideoData) {
                    nodeData->Cancel_reTransMsg(node, ccnMsg->msg_full_name);
                    nodeData->recvRequestData(node, ccnMsg);  // Statistical data 
                    delete ccnMsg;
                    break;  
                  }
                }

                // 再送パケットは次のパケットを要求しないため、ここで終了
                if(ccnMsg->resent_times > 0) {
                  if(ccnMsg->content_type == VideoData) {
                    if(nodeData->ccn_method == DEFAULT) {
                      nodeData->Cancel_reTransMsg(node, ccnMsg->msg_full_name);
                      nodeData->recvRequestData(node, ccnMsg);  // Statistical data 
                      delete ccnMsg;
                      break;
                    }
                    else if(nodeData->ccn_method == DEFAULT_FAST) {
                      nodeData->Cancel_reTransMsg(node, ccnMsg->msg_full_name);
                      nodeData->recvRequestData(node, ccnMsg);  // Statistical data 
                      delete ccnMsg;
                      break;
                    }
                  }
                }
                // 未取得chunkを取得
                if(nodeData->ccn_method == DEFAULT_FAST) {
                  if(ccnMsg->content_type == VideoData) {
                    if(ccnMsg->msg_chunk_num > nodeData->return_lastRecvMsg(ccnMsg) + 1) {

                      CcnMsg* t_ccnMsg;
                      t_ccnMsg = nodeData->NewCcnMsg(ccnMsg);
                      t_ccnMsg->msg_chunk_num = nodeData->return_lastRecvMsg(ccnMsg) + 1;
                      t_ccnMsg->EncodeFullNameMsg();
                      t_ccnMsg->payload_length = 30;
                      t_ccnMsg->resent_times = 1;
                      t_ccnMsg->msg_type = Interest;
                      t_ccnMsg->hops_limit = 20;

                      nodeData->reqMapSearch(node, t_ccnMsg);
                      nodeData->reqMapInput(node, t_ccnMsg);

                      nodeData->Cancel_reTransMsg(node, t_ccnMsg->msg_full_name);
                      nodeData->make_reTransMsg(node, t_ccnMsg);

                      Node* source_node;
                      map<uint32_t, clocktype>::iterator it;
                      source_node = MAPPING_GetNodePtrFromHash(GlobalNodeData::nodeHash, source_node_id);
                      nodeData->source_nodeData = (NodeData*)source_node->appData.nodeData;
                      NodeData* source_nodeData;
                      source_nodeData = nodeData->source_nodeData;

                      it = source_nodeData->dataGenerateTime_map.find(t_ccnMsg->msg_chunk_num);
                      t_ccnMsg->data_genTime = it->second;

                      nodeData->set_windowSize(node, t_ccnMsg);
                      nodeData->fibSend(node, t_ccnMsg);
                    }
                  }
                }
                nodeData->release_windowSize(node, ccnMsg);

                nodeData->set_lastRecvMsg(ccnMsg);
                nodeData->Cancel_reTransMsg(node, ccnMsg->msg_full_name);
                nodeData->recvRequestData(node, ccnMsg);  // Statistical data 

                if(ccnMsg->end_chunk_num < nodeData->return_lastChunkNum(ccnMsg) + 1) {
                  delete ccnMsg;
                  break;
                }


                // クライアントにData到着
                ccnMsg->msg_type = Interest;
                ccnMsg->msg_chunk_num = nodeData->return_lastChunkNum(ccnMsg) + 1;
                ccnMsg->EncodeFullNameMsg();
                ccnMsg->resent_times = 0;
                ccnMsg->payload_length = 30;
                ccnMsg->hops_limit = 20;
                ccnMsg->interest_genTime = node->getNodeTime();

                map<uint32_t, clocktype>::iterator it;
                it = nodeData->source_nodeData->dataGenerateTime_map.find(ccnMsg->msg_chunk_num);
                ccnMsg->data_genTime = it->second;

                nodeData->set_lastChunkNum(ccnMsg);
                nodeData->set_windowSize(node, ccnMsg);
                nodeData->make_reTransMsg(node, ccnMsg);
                nodeData->reqMapInput(node, ccnMsg);
                nodeData->fibSend(node, ccnMsg);

                break;
              }
              nodeData->csInsert_DEFAULT(node, ccnMsg);
              nodeData->pitSend_DEFAULT(node, ccnMsg);
              break;
            }

            default:
            {
              ERROR_ReportError("Undefined msg type\n");
            }
          }
          break;
        }

        case PRIOR:
        {
          switch(ccnMsg->msg_type) {
            case Interest:
            {
              // ソースノードまで転送
              // PITに名前を記録(msg_name)
              // ソースノードならDataパケットを返送
              // またDataパケットを一定時間ごとに生成・転送
              // 返送する際はPITを参照
              if(nodeData->csSend(node, ccnMsg) == true) {
                // キャッシュヒット
                nodeData->StatisticalInfo_cacheHit(node, ccnMsg);
                break;
              }

              if(nodeData->pitInsert_PRIOR(node, ccnMsg)) {
                // PITで登録済み
                delete ccnMsg;
                break;
              }

              if(ccnMsg->source_node_id == node->nodeId) {
                nodeData->convertInterest_intoData(ccnMsg);
                nodeData->set_lastChunkNum(ccnMsg);
                APP_SetTimer(node, APP_CCN_CLIENT, 0, APP_CCN_LISTEN_PORT, APP_TIMER_DATA_SEND_PKT, 0 * MILLI_SECOND);
              }
              else {
                nodeData->fibSend_PRIOR(node, ccnMsg);
              }

              break;
            }

            case Data:
            {
              if(nodeData->reqMapSearch_PRIOR(node, ccnMsg)) {
                nodeData->recvRequestData(node, ccnMsg);  // Statistical data 
                delete ccnMsg;
                break;
              }
              nodeData->pitSend_PRIOR(node, ccnMsg);
              break;
            }

            default:
              ERROR_ReportError("Undefined msg type\n");
          }
          break;
        }

        case PROPOSAL:
        {
          switch(ccnMsg->msg_type) {
            case Interest:
            {
              nodeData->fibModInsert(node, ccnMsg, ccnMsg->previous_node_id);
              if(nodeData->pitInsert_PROPOSAL(node, ccnMsg)) {
                // PITで登録済み
                delete ccnMsg;
                break;
              }

              if(ccnMsg->source_node_id == node->nodeId) {
                nodeData->set_lastChunkNum(ccnMsg);
                nodeData->convertInterest_intoData(ccnMsg);
                nodeData->pitSend_PROPOSAL(node, ccnMsg);
                // プッシュ配信登録
                APP_SetTimer(node, APP_CCN_CLIENT, 0, APP_CCN_LISTEN_PORT, APP_TIMER_fakeINTEREST_SEND_PKT, 0 * MILLI_SECOND);
              }
              else {
                nodeData->fibSend_PROPOSAL(node, ccnMsg);
              }
              break;
            }

            case Data:
            {
              if(nodeData->reqMapSearch(node, ccnMsg)) {
                nodeData->recvRequestData(node, ccnMsg);  // Statistical data 

                // プッシュ配信登録済みクライアントであることを記録
                nodeData->reqModMapInput(node, ccnMsg);
                delete ccnMsg;
                break;
              }
              nodeData->pitSend_PROPOSAL(node, ccnMsg);

              break;
            }

            case fakeInterest:
            {
              nodeData->pitModInsert(node, ccnMsg);
              nodeData->csModInsert(node, ccnMsg);
if(nodeData->node_role == CLIENT)
if(ccnMsg->msg_name != msg_videoData_name + node->nodeId%3)
    printf("[node%d][msg%lu] fuckfuckfuck %d %d\n",node->nodeId, ccnMsg->msg_full_name, ccnMsg->msg_name, ccnMsg->msg_chunk_num);

              if(nodeData->reqModMapSearch(node, ccnMsg)) { // クライアント到着
                nodeData->recvRequestData(node, ccnMsg);  // Statistical data 
                nodeData->recvModMapInsert(node, ccnMsg);

                nodeData->convertFakeInterest_intoFakeData(ccnMsg);
                nodeData->pitModSend(node, ccnMsg);

                // 未取得chunkを取得
                if(ccnMsg->msg_chunk_num > nodeData->return_lastRecvMsg(ccnMsg) + 1) {
                  CcnMsg* ccnMsg;
                  ccnMsg = new CcnMsg();
                  ccnMsg->resent_times = 1;
                  ccnMsg->ccn_method = DEFAULT;
                  ccnMsg->msg_type = Interest;
                  ccnMsg->msg_name = msg_videoData_name + node->nodeId % 3;
                  ccnMsg->msg_chunk_num = nodeData->return_lastRecvMsg(ccnMsg) + 1;
                  ccnMsg->EncodeFullNameMsg();
                  ccnMsg->sender_node_id = node->nodeId;
                  ccnMsg->source_node_id = source_node_id;
                  ccnMsg->payload_length = 30;
                  ccnMsg->hops_limit = 20;
                  ccnMsg->interest_genTime = node->getNodeTime();
                  ccnMsg->content_type = VideoData;
                  ccnMsg->end_chunk_num = UINT16_MAX;

                  Node* source_node;
                  map<uint32_t, clocktype>::iterator it;
                  source_node = MAPPING_GetNodePtrFromHash(GlobalNodeData::nodeHash, source_node_id);
                  nodeData->source_nodeData = (NodeData*)source_node->appData.nodeData;
                  NodeData* source_nodeData;
                  source_nodeData = nodeData->source_nodeData;

                  it = source_nodeData->dataGenerateTime_map.find(ccnMsg->msg_chunk_num);
                  ccnMsg->data_genTime = it->second;

                  nodeData->reqMapSearch(node, ccnMsg);
                  nodeData->set_windowSize(node, ccnMsg);
                  nodeData->reqMapInput(node, ccnMsg);
                  nodeData->make_reTransMsg(node, ccnMsg);
                  nodeData->fibSend(node, ccnMsg);
                } else if (ccnMsg->msg_chunk_num == nodeData->return_lastRecvMsg(ccnMsg) + 1){
                  //printf("Got actual packet\n");
                } else {
                }
                nodeData->set_lastRecvMsg(ccnMsg);
                break;
              }


              if(nodeData->fibModSend(node, ccnMsg)) {
                delete ccnMsg;
                break;
              }
              break;
            }

            case fakeData:
            {
              // クライアントにデータ到着
              if(ccnMsg->source_node_id == node->nodeId) {
                delete ccnMsg;
                break;
              }

              nodeData->pitModSend(node, ccnMsg);
              break;
            }

            default:
              ERROR_ReportError("Undefined msg type\n");
          }
          break;
        }

        default:
          ERROR_ReportError("Undefined ccn_method\n");
      }
      break;
    }

    case MSG_APP_FromTransOpenResult:   // TCPでpassive open処理が終了
      //if(node->nodeId == 1) printf("openResult->connectionId: %lu\n", openResult->connectionId);
      break;
    case MSG_APP_FromTransListenResult: // コネクション要求待機状態を確立
      //if(node->nodeId == 1) printf("openResult->connectionId: %lu\n", openResult->connectionId);
      break;

    case MSG_APP_TimerExpired: 
    {  
      AppTimer* timer;
      timer = (AppTimer *)MESSAGE_ReturnInfo(msg);
      switch(timer->type) {
        case APP_TIMER_RE_SEND_PKT:
          nodeData->send_reTransMsg(node, timer->connectionId);
        break;
      }
      break;
    }
    case MSG_APP_FromTransCloseResult:
      break;
    default:
      printf("msg->eventType = %d\n", msg->eventType);
      ERROR_ReportError("msg->eventType error: undefined eventType\n");
  }
  MESSAGE_Free(node, msg);
}

//Finalize Function
void AppCCNClientFinalize(Node* node, AppInfo* appInfo) {
  NodeData* nodeData = (NodeData *)appInfo->appDetail;
}

void AppCCNHostFinalize(Node* node, AppInfo* appInfo) {
  NodeData* nodeData = (NodeData *)appInfo->appDetail;
  if(node->nodeId == 1) nodeData->StatisticalInfo_print_totalNodeInfo(node, global_node_data->maximumActiveNodeNum);
}

