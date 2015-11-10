#ifndef NODE_DATA_H
#define NODE_DATA_H
#include <stdio.h>

#include "ccn.h"
#include "statistic.h"

typedef map<int, bool> VideoMap_t;      // map<msg_chunk_num, isStored>

typedef map<uint64_t, CcnMsg*> CcnMsgBufferMap_t; 
typedef map<uint32_t, CcnMsg*> CcnMsgDelayBufferMap_t; 
                                        // map<packetGenerateTime, ccnMsg>
typedef map<int, int> FibMap_t;         // map<destNodeId, nextNodeId>
typedef multimap<uint64_t, int> PitMap_t;    // multimap<msg_full_name, nextNodeId>
typedef multimap<uint32_t, int> p_PitMap_t;    // p_ means persistent
                                                        // multimap<msg_name, nextNodeId>
typedef map<uint64_t, int> PitModMap_t;    // multimap<msg_full_name, nextNodeId>
typedef map<uint64_t, int> CsMap_t;          // map<msg_full_name, clientNum>
typedef vector<uint64_t> CsLru_t;

typedef map<uint64_t, clocktype> RequestMap_t;    // map<msg_full_name, createTime>
typedef map<uint32_t, clocktype> p_RequestMap_t;    // p_ means persistent
                                                    // map<msg_name, createTime>

typedef map<uint32_t, uint32_t> RequestModMap_t;    // map<msg_name, last_chunk_num>

typedef multimap<int, int> FibModMap_t; // multimap<destNodeId, nextNodeId>

typedef map<uint32_t, uint8_t> Cur_WindowSize_t; // map<msg_name, windowSize>
typedef set<CcnMsg*> GlobalCcnBuffer_t;

#define DEF_VIDEO_BITRATE 4000 // 4000 kbps -> 500 KB/s
#define DEF_SIM_TIME 30 // sec
#define DEF_TCP_MSS 1460 // 1460 Byte  defined in senarious/*.config
#define DEF_CCN_WINDOW_SIZE 40

//#define UINT32_MAX (0xffffffff)
//#define UINT16_MAX (0xffff)
#define UINT32_MAX ((uint32_t)-1)
#define UINT16_MAX ((uint16_t)-1)

class GlobalNodeData
{
public:
  static IdToNodePtrMap* nodeHash;

  uint32_t Zipf[UINT32_MAX];

  int maximumActiveNodeNum;

  // use for statistical 
  void setMaxNodeId(int nodeId);
  void ZipfInit();

//  GlobalNodeData() {
//    ZipfInit();
//  }
  uint16_t return_MsgName();
  uint16_t return_MsgName(Node* node, uint16_t requestNum);
};




class RecvInfo
{
public:
  bool isUsed;
  uint8_t clientNum;

  RecvInfo() {
    this->isUsed = false;
    this->clientNum = 1;
  }
};

typedef map<uint64_t, RecvInfo*> RecvMap_t; // map<msg_full_name, clientInfo>


class NodeData
{
public:
  NodeRole node_role;
  NodeData* source_nodeData;
  CCN_method ccn_method;

  string name_logDirectory;      // 統計ログ出力用 statistic_total
  string name_logFile;           // 各ノードごとのログ出力
  string specific_logDirectory;  

  // リアルタイムストリーミング用のランダム値
  map<uint32_t, double> randomNum_map;    // ソースノードでのみ生成
  map<uint32_t, clocktype> dataGenerateTime_map;    // ソースノードでのみ生成

  // 再送
  //map<uint32_t, Message*> cancel_reTransMsg_map;
  map<uint64_t, Message*> cancel_reTransMsg_map;
  CcnMsgBufferMap_t reTrans_ccnMsgBuffer; // <msg_full_name, CcnMsg*>
  void Cancel_reTransMsg(Node* node, uint64_t msg_full_name);
  void send_reTransMsg(Node* node, uint64_t msg_full_name);
  void make_reTransMsg(Node* node, CcnMsg* ccnMsg);


  // 送信したInterestの最も最後のchunk番号をmsg名前ごとに記録
  map<uint32_t, uint32_t> last_chunk_num;   // map<msg_name, last_chunk_num>
  void set_lastChunkNum(CcnMsg* ccnMsg);
  void set_lastChunkNum(uint32_t msg_name, uint32_t chunk_num);
  uint32_t return_lastChunkNum(uint32_t msg_name, uint32_t chunk_num);
  uint32_t return_lastChunkNum(CcnMsg* ccnMsg);

  // 最後に到着したchunk番号をmsg名前ごとに記録
  map<uint32_t, uint32_t> last_recv_msg;   // map<msg_name, last_chunk_num>
  void set_lastRecvMsg(CcnMsg* ccnMsg);
  void set_lastRecvMsg(uint32_t msg_name, uint32_t chunk_num);
  uint32_t return_lastRecvMsg(uint32_t msg_name, uint32_t chunk_num);
  uint32_t return_lastRecvMsg(CcnMsg* ccnMsg);

  bool check_missingChunkNum(CcnMsg* ccnMsg);
  bool check_missingChunkNum(uint32_t msg_name, uint32_t chunk_num);

  void send_allMisiingChunk(CcnMsg* ccnMsg);
  void send_allMisiingChunk(uint32_t msg_name, uint32_t chunk_num);

  uint32_t video_rate;
  uint32_t common_rate;
  uint32_t sim_time;
  uint32_t max_chunk_num;
  uint32_t tcp_mss;

  double error_rate;

  clocktype videoTimer;
  clocktype packetGenerateTime;
  clocktype commonPacketGenerateTime;

  Cur_WindowSize_t cur_windowSize;
  uint8_t  max_windowSize;

  uint8_t return_windowSize(CcnMsg* ccnMsg);
  bool set_windowSize(Node* node, CcnMsg* ccnMsg);
  void release_windowSize(Node* node, CcnMsg* ccnMsg);

  void NodeInfo_init();
  void NodeInfo_init(Node* node, const NodeInput* nodeInput);
  void StatisticalInfo_init();

  NodeData(Node* node, const NodeInput* nodeInput) {
    this->NodeInfo_init(node, nodeInput);
    this->StatisticalInfo_init();
  }

  NodeData() {
    this->NodeInfo_init();
    this->StatisticalInfo_init();
  }

  //----------------------------
  // statistical information
  // ---------------------------
  StatisticalData* statData;
  void StatisticalInfo_sendCcnMsg(Node* node, CcnMsg* ccnMsg);
  void StatisticalInfo_recvCcnMsg(Node* node, CcnMsg* ccnMsg);
  void StatisticalInfo_recvCcnMsg(Node* node, CcnMsg* ccnMsg, clocktype senToRecv_Delay);
  void StatisticalInfo_cacheHit(Node* node, CcnMsg* ccnMsg);
  void StatisticalInfo_print(Node* node);
  void StatisticalInfo_print_totalNodeInfo(Node* node, int maxActiveNode);
  void StatisticalInfo_print_eachNodeInfo(Node* node);

  void recvRequestData(Node* node, CcnMsg* ccnMsg);

  //----------------------------
  // video information
  // ---------------------------
  VideoMap_t videoMap;
  VideoMap_t unacquiredVideoMap;
  void setVideoMap(int msg_chunk_num);
  bool checkUnacquiredSequenceChunk(int msg_chunk_num);
  void addUnacquiredVideoChunk(int msg_chunk_num);

  void tcpOpenConnection(Node* node, CcnMsg* ccnMsg, messageType msg_type, int remote_nodeId);
  void tcpOpenConnection(Node* node, CcnMsg* ccnMsg, messageType msg_type, int remote_nodeId, clocktype packetGenerateTime);

  // for default CCN to use sending data packet at acutual time
  void sourceDataSend(Node* node, CcnMsg* ccnMsg); 

  clocktype return_packetGenerateTime(uint32_t chunk_num);
  clocktype return_packetGenerateTime(CcnMsg* ccnMsg);

  clocktype return_packetRandomGenerateTime();
  void convertInterest_intoData(CcnMsg* ccnMsg);
  void convertFakeInterest_intoFakeData(CcnMsg* ccnMsg);
  CcnMsg* NewCcnMsg(CcnMsg* ccnMsg);

  //---------------------------
  // default CCN 
  // --------------------------
  CcnMsgBufferMap_t ccnMsgBuffer;
  uint64_t currentBufferNumber;

  FibMap_t fibMap;
  PitMap_t pitMap;
  p_PitMap_t p_pitMap;
  CsMap_t  csMap;
  CsLru_t  csLru;
  RequestMap_t  reqMap;
  p_RequestMap_t  p_reqMap;

  int cs_cacheSize;

  uint64_t BufferPush(Node* node, CcnMsg* ccnMsg);
  CcnMsg* BufferRetrieve_Client(Node* node, Message* msg);
  CcnMsg* BufferRetrieve_Host(Node* node, Message* msg);

  int  fibFind(Node* node, CcnMsg* ccnMsg);
  void fibSend(Node* node, CcnMsg* ccnMsg);
  void fibSend_DEFAULT(Node* node, CcnMsg* ccnMsg);
  void fibSend_PRIOR(Node* node, CcnMsg* ccnMsg);
  void fibSend_PROPOSAL(Node* node, CcnMsg* ccnMsg);
  void fibMapInput(Node* node, const char* filename);

  bool pitInsert(Node* node, CcnMsg* ccnMsg);
  bool pitInsert_DEFAULT(Node* node, CcnMsg* ccnMsg);
  bool pitInsert_PRIOR(Node* node, CcnMsg* ccnMsg);
  bool pitInsert_PROPOSAL(Node* node, CcnMsg* ccnMsg);
  bool pitSend(Node* node, CcnMsg* ccnMsg);
  bool pitSend_DEFAULT(Node* node, CcnMsg* ccnMsg);
  bool pitSend_DEFAULT(Node* node, CcnMsg* ccnMsg, clocktype send_delayTime);
  bool pitSend_PRIOR(Node* node, CcnMsg* ccnMsg);
  bool pitSend_PRIOR(Node* node, CcnMsg* ccnMsg, clocktype send_delayTime);
  bool pitSend_PROPOSAL(Node* node, CcnMsg* ccnMsg);
  bool pitSend_PROPOSAL(Node* node, CcnMsg* ccnMsg, clocktype send_delayTime);

  void lruRefresh(uint64_t msg_full_name);
  void lruDelete(uint64_t msg_full_name);

  void csDelete(uint64_t msg_full_name);
  void csInsert(Node* node, CcnMsg* ccnMsg);
  void csInsert_DEFAULT(Node* node, CcnMsg* ccnMsg);
  void csInsert(uint64_t msg_full_name);
  bool csSend(Node* node, CcnMsg* ccnMsg);
  void csPrint(Node* node);

  void reqMapInput(Node* node, CcnMsg* ccnMsg);
  void reqMapInput_PRIOR(Node* node, CcnMsg* ccnMsg);
  bool reqMapSearch(Node* node, CcnMsg* ccnMsg);
  bool reqMapSearch_PRIOR(Node* node, CcnMsg* ccnMsg);
  

  //// distribution 
  FibModMap_t fibModMap;
  PitModMap_t pitModMap; 
  RequestModMap_t reqModMap;

  //// acquisition check for content store 
  RecvMap_t recvModMap;

  bool isDistReq; // used for checking the node was published interest 

  void fibModInsert(Node* node, CcnMsg* ccnMsg, int nodeId);
  bool fibModSend(Node* node, CcnMsg* ccnMsg);
  bool fibModSend(Node* node, CcnMsg* ccnMsg, clocktype send_delayTime);
  bool pitModInsert(Node* node, CcnMsg* ccnMsg);
  bool pitModSend(Node* node, CcnMsg* ccnMsg);
  void csModInsert(Node* node, CcnMsg* ccnMsg);
  //bool csModSend(Node* node, CcnMsg* ccnMsg);

  void reqModMapInput(Node* node, CcnMsg* ccnMsg);
  bool reqModMapSearch(Node* node, CcnMsg* ccnMsg);
  void recvModMapInsert(Node* node, CcnMsg* ccnMsg);
  void recvModMapRemove(Node* node, CcnMsg* ccnMsg);
  bool isNotSent_fakeData(Node* node, CcnMsg* ccnMsg); 
  bool isRecvAll_fakeData(Node* node, CcnMsg* ccnMsg);
};

#endif 
