#ifndef CCN_H
#define CCN_H

enum NodeRole
{
  SERVER,
  RELAY_NODE,
  CLIENT,
};

enum messageType
{
  //----------------------
  // DEFAULT, PRIOR
  //----------------------
  Interest,
  Data,

  //----------------------
  // PROPOSAL
  //----------------------
  // 配信登録制御
  //distRequest,
  //distAnswer,

  // 動画配信
  fakeInterest,
  fakeData,

  // 再送 
  //trueInterest,
  //trueData,
};

enum contentType
{
  VideoData,
  CommonData,
};

enum CCN_method
{
  NO_VIDEO,
  DEFAULT,    // 既存手法
  DEFAULT_FAST,
  PRIOR,      // 先行手法 INTEREST:DATA = 1:N
  PROPOSAL,   // 提案手法 push-based information delivery
};

class CcnMsg
{
public:
  CCN_method ccn_method;
  uint64_t bufferMapKey;

  messageType msg_type;
  uint32_t msg_name;
  uint32_t msg_chunk_num;
  uint64_t msg_full_name;  // msg_full_name = msg_name + msg_chunk_num

  uint16_t sender_node_id;
  uint16_t source_node_id;
  uint16_t previous_node_id;
  uint16_t payload_length;
  uint16_t hops_limit;
  uint32_t end_chunk_num;
  contentType content_type;
  uint8_t  resent_times;

  clocktype interest_genTime;  // interest generate time
  clocktype data_genTime;      //   data   generate time

  //clocktype distRequest_genTime;    // distribution request  generate time
  //clocktype distAnswer_genTime;     // distribution  answer  generate time
  clocktype fakeInterest_genTime;   //      fake  interest   generate time
  clocktype fakeData_genTime;       //      fake  data       generate time
  //clocktype trueInterest_genTime;   //      true  interest   generate time
  //clocktype trueData_genTime;       //      true  data       generate time

  CcnMsg() {
    resent_times = 0;

    interest_genTime     = CLOCKTYPE_MAX; // CLOCKTYPE_MAX means no data in this variable
    data_genTime         = CLOCKTYPE_MAX; // same above

    //distRequest_genTime  = CLOCKTYPE_MAX; // same above
    //distAnswer_genTime   = CLOCKTYPE_MAX; // same above
    fakeInterest_genTime = CLOCKTYPE_MAX; // same above
    fakeData_genTime     = CLOCKTYPE_MAX; // same above
    //trueInterest_genTime = CLOCKTYPE_MAX; // same above
    //trueData_genTime     = CLOCKTYPE_MAX; // same above
  }

  CcnMsg(Message* msg) {
    this->DecodePayload(msg);
  }

  void makeCcnMsg(messageType);

  void EncodeFullNameMsg();
  void DecodeFullNameMsg();

  char* EncodePayload();
  void DecodePayload(Message* msg);
  void DecodePayload(char* payload);

  void App_TcpSendCcnMsg(Node* node, Message* msg);

  void printState();
  void printSize(Message* msg);
  bool checkHopCount();

  bool operator<(const CcnMsg& compareCcnMsg) const {
    if(compareCcnMsg.msg_name > this->msg_name) {
      return true;
    }
    return false;
  }
};

//---------------------
// Function
// --------------------

//Init Function
void AppCCNNodeInit(Node* node, NodeRole role, const NodeInput* nodeInput);

//Finalize Function
void AppCCNClientFinalize(Node* node, AppInfo* appInfo);
void AppCCNHostFinalize(Node* node, AppInfo* appInfo);

//Process Event Function
void AppLayerCCNClient(Node* node, Message* msg);
void AppLayerCCNHost(Node* node, Message* msg);

#endif 

//// Static header which defined in ccnx 1.x
//uint8_t  version;                 // not used in this simulation
//uint8_t  Message_Type;
//uint16_t Payload_Length;
//uint8_t  Hop_Limit;
//uint8_t  Reserved;                // not used in this simulation
//uint16_t Optional_Header_Length;   // not used in this simulation 

//// CCNMessage which define in ccnx 1.x
//uint16_t TYPE_CCN_Message;
//uint16_t CCN_Message_Length;
//uint16_t TYPE_NAME;
//uint16_t Name_Length;
