#ifndef mac_dot11n
#define mac_dot11n

#include "mac_dot11.h"

// AMSDU Size
#define AMSDU_SIZE_1 3839
#define AMSDU_SIZE_2 7935
#define MIN_MPDUS_IN_AMPDU 2
#define MIN_MSDUS_IN_AMSDU 5
#define BLOCK_ACK_POLICY_THRESHOLD 5
#define MAX_MPDUS_IN_AMPDU 64
#define DEFAULT_MAC_QUEUE_SIZE 80
#define MAX_MPDUS_NEGOTIATED_VIA_BA_POLICY 64
#define CATEGORY_BLOCK_ACK 3
#define BLOCK_ACK_POLICY_TIMEOUT 500*MILLI_SECOND
#define MACDOT11N_ADDBS_REQUEST_TIMEOUT (512 * MILLI_SECOND)
#define MACDOT11N_BAP_REINITIATE_TIMEOUT (1 * SECOND)
#define MACDOT11N_BLOCK_ACK_REQUEST_TIMEOUT (512 * MILLI_SECOND)
#define MACDOT11N_INVALID_SEQ_NUM 65535
#define MACDOT11N_INVALID_COUNT 255
#define MACDOT11N_MIN_PKTS_IN_TXOP_BAA 5
#define MACDOT11N_ACK_POLICY_BA 3
#define MACDOT11N_ACK_POLICY_NA 0
#define MACDOT11N_RIFS 2 * MICRO_SECOND
#define MACDOT11N_BLOCK_ACK_BITMAP_SIZE 8
#define MACDOT11N_MPDU_DELIMITER 4  // In Octets
#define MACDOT11N_AMPDU_SUBFRAME_MAX_PADDING 4 // In Octets
#define MACDOT11N_AMPDU_MAX_SIZE 65535 // In Octets
#define MACDOT11N_MAX_SEQUENCE_NUMBER 4095

#define DOT11nMessage_ReturnPacketSize(msg) \
    ((msg->isPacked)? \
    (msg->actualPktSize) : (msg->packetSize + msg->virtualPayloadSize))

typedef struct {
    unsigned short  TID:4,
              EOSP:1,
              AckPolicy:2,
              AmsduPresent:1,
              TXOP:8;
}DOT11n_QoSControl;

 enum FrameState {
        STATE_FRESH,
        STATE_ACKED,
        STATE_AWAITING_ACK
       
    };

typedef struct{
    UInt8             frameType;
    UInt8             frameFlags;
    UInt16            duration;
    Mac802Address     destAddr;
    Mac802Address     sourceAddr;
    Mac802Address     address3;
    UInt16            fragId: 4,
                      seqNo: 12;
    DOT11n_QoSControl qoSControl;
    char              FCS[4];
} DOT11n_FrameHdr; 

typedef struct str_aMsduSubFrameHdr
{
    Mac802Address DA;
    Mac802Address SA;
    UInt16 Length;
}aMsduSubFrameHdr;


typedef struct {
    char   BABitmap[MACDOT11N_BLOCK_ACK_BITMAP_SIZE];
    UInt16 BAStartSeqNumber;
}DOT11n_BAInfoField;


typedef struct {
    unsigned short  BAAckPolicy:1,
              MultiTID:1,
              CompressedBitmap:1,
              Reserved:9,
              TID_INFO:4;

}DOT11n_BAControlField;

 // Block ack Control Frame
typedef struct {
                             
    UInt8                   frameType;
    UInt8                   frameFlags;
    UInt16                  duration;
    Mac802Address           destAddr;
    Mac802Address           sourceAddr;
    DOT11n_BAControlField BAControlField;
    DOT11n_BAInfoField      BAInfoField;
    char                    FCS[4];
} DOT11n_BlockAckControlFrame; 


typedef struct 
{
    unsigned short BARAckPolicy:1,
                   MultiTID:1,
                   CompressedBitmap:1,
                   Reserved:9,
                   TID_INFO:4;
}DOT11n_BARControl;

typedef struct 
{
    unsigned short FragId: 4,
                   SeqNo:12;
}DOT11n_BAStartingSeqControl;


 // Block ack request Frame
typedef struct {
    UInt8                   frameType;
    UInt8                   frameFlags;
    UInt16                  duration;
    Mac802Address           destAddr;
    Mac802Address           sourceAddr;
    DOT11n_BARControl barControl;
   DOT11n_BAStartingSeqControl startingSeqControl;
    char                    FCS[4];
} DOT11n_BlockAckRequestFrame; 

// QOS Action fields
typedef enum 
{
    DOT11n_ADDBA_Request,
    DOT11n_ADDBA_Response,
    DOT11n_DELBA
} DOT11n_BlockAckActionField;

typedef enum 
{
    SUCCESSFUL,
    FAILURE
} DOT11n_ADDBAStatusCode;

typedef enum {
    STA_LEAVING,
    END_BA,
    UNKNOWN_BA,
    TIMEOUT
} DOT11n_DELBAReasonCode;


typedef struct 
{
   UInt16 AMSDUSupported:1,
          BAPolicy:1,
          TID:4,
          BufferSize: 10;
}DOT11n_BlockAckParameterSet;

typedef struct 
{
   UInt16 reserved:11,
          initiator:1,
          TID:4;
}DOT11n_DelbaParameterSet;


 // ADDBA request Frame
typedef struct {
    UInt8                   frameType;
    UInt8                   frameFlags;
    UInt16                  duration;
    Mac802Address           destAddr;
    Mac802Address           sourceAddr;
    Mac802Address           address3;
   UInt16                   fragId: 4,
                            seqNo: 12;
     int                    category;
    DOT11n_BlockAckActionField action;
    DOT11n_BlockAckParameterSet blockAckParams;
    UInt16 blockAckTimeout;
    DOT11n_BAStartingSeqControl startingSeqControl;
   char                    FCS[4];
} DOT11n_ADDBARequestFrame; 


// DELBA request Frame
typedef struct 
{
    UInt8                   frameType;
    UInt8                   frameFlags;
    UInt16                  duration;
    Mac802Address           destAddr;
    Mac802Address           sourceAddr;
    Mac802Address           address3;
   UInt16                   fragId: 4,
                            seqNo: 12;
     int                    category;
    DOT11n_BlockAckActionField action;
    DOT11n_DelbaParameterSet delbaParams;
    DOT11n_DELBAReasonCode reason;
   char                    FCS[4];
} DOT11n_DELBAFrame;

 // ADDBA response Frame
typedef struct {
    UInt8                   frameType;
    UInt8                   frameFlags;
    UInt16                  duration;
    Mac802Address           destAddr;
    Mac802Address           sourceAddr;
    Mac802Address           address3;
   UInt16                   fragId: 4,
                            seqNo: 12;
     int                    category;
    DOT11n_BlockAckActionField action;
    DOT11n_ADDBAStatusCode statusCode;
    DOT11n_BlockAckParameterSet blockAckParams;
    UInt16 blockAckTimeout;
    char                    FCS[4];
} DOT11n_ADDBAResponseFrame;


enum BlockAckAggStates 
{
    BAA_STATE_DISABLED,
    BAA_STATE_IDLE,
    BAA_STATE_ADDBA_REQUEST_PENDING,
    BAA_STATE_ADDBA_REQUEST_QUEUED,
    BAA_STATE_WF_ADDBA_RESPONSE,
    BAA_STATE_BLOCKACK_REQUEST_QUEUED,
    BAA_STATE_WF_BLOCK_ACK,
    BAA_STATE_DELBA_QUEUED,
    BAA_STATE_TRANSMITTING,
    BAA_STATE_WF_RETRY_TIMER_EXPIRE,
    BAA_STATE_RECEIVER_WF_BA_SETUP,
    BAA_STATE_RECEIVING,
};

enum BlockAckAggType 
{
    BA_DELAYED,
    BA_IMMEDIATE,
    BA_NONE
};

enum BlockAckAggRole 
{
    ROLE_NONE,
    ROLE_TRANSMITTER,
    ROLE_RECEIVER
};

typedef struct
{
    BlockAckAggRole role;
    BlockAckAggType type;
    BlockAckAggStates state;
    UInt16 startingSeqNo;
    UInt32 numPktsNegotiated;
    UInt8 numPktsSent;
    UInt8 numPktsLeftToBeSentInCurrentTxop;
    UInt8 numPktsToBeSentInCurrentSession;
    clocktype navDuration;
    UInt16 baTimerSequenceNumber;
    BOOL blockAckSent;
}DOT11n_BlockAckAggrementVrbls;

// Dot11 Amsdu Enqueue Map Structure & Key
struct MapValue
{
    std::list<struct DOT11_FrameInfo*> frameQueue;
    std::list<struct DOT11_FrameInfo*>::iterator vItr;
    int aggSize;
    UInt32 numPackets;
    UInt16 seqNum;
    UInt16 winStarts; //window start
    UInt16 winEnds; //window end
    UInt32 winSizes;  //window size
    Message *timerMsg;
    DOT11n_BlockAckAggrementVrbls *blockAckVrbls;
    UInt16 winStartr; //window start
    UInt16 winEndr; //window end
    UInt32 winSizer;  //window size
    UInt64 BABitmap;
    BOOL BAPActiveAtReceiver;
    clocktype lastActivityTimeOut;
    clocktype creationTime;

    MapValue()
    {
        aggSize = 0;
        numPackets = 0;
        seqNum = 0;
        winStarts = 0;
        winEnds = MACDOT11N_INVALID_SEQ_NUM;
        winSizes = 0;
        timerMsg = NULL;
        blockAckVrbls = NULL;
        winStartr = MACDOT11N_INVALID_SEQ_NUM;
        winEndr = MACDOT11N_INVALID_SEQ_NUM;
        winSizer = 0;
        BABitmap = 0;
        BAPActiveAtReceiver = FALSE;
        lastActivityTimeOut = 0;
        creationTime = 0;
    }
};

struct timerMsg
{
    Mac802Address addr;
    TosType priority;
    UInt16 seqNo;
    BOOL initiator;
};

enum IbssProbeStatus 
{
    IBSS_PROBE_NONE,
    IBSS_PROBE_REQUEST_QUEUED,
    IBSS_PROBE_IN_PROGRESS,
    IBSS_PROBE_COMPLETED
};

struct DOT11n_IBSS_Station_Info
{
    Mac802Address    macAddr; 
    DOT11_HT_CapabilityElement staHtCapabilityElement;
    DOT11e_CapabilityInfo staCapInfo;
    BOOL rifsMode;
    IbssProbeStatus probeStatus;
    Message* timerMessage;
    DOT11n_IBSS_Station_Info *next;
};

class OutputBuffer
{
public:
    // Data Member Declaration
    // MapKey pair Object
    std::pair<Mac802Address,TosType> tmpKey;
    std::map<std::pair<Mac802Address,TosType>,MapValue> OutputBufferMap;
    std::map<std::pair<Mac802Address,TosType>,MapValue>::iterator Mapitr;
    std::map<std::pair<Mac802Address,TosType>,MapValue>::iterator roundRobinKeyItr;
    std::map<std::pair<Mac802Address,TosType>,MapValue>::iterator Mapitr3;
    std::map<std::pair<Mac802Address,TosType>,MapValue>::iterator MapitrAmsdu;
    std::map<std::pair<Mac802Address,TosType>,MapValue>::key_compare keyCmp;
    // Vector Iterator
    std::list<DOT11_FrameInfo*>::iterator tmpVitr;
 
    // Member Function Declaration
    void Enqueue(MacDataDot11* dot11,Node* node, 
         Mac802Address ipDestAddr,
         DOT11_FrameInfo *frameInfo,
         BOOL isKeyAlreadyPresent);

    void MacDot11nInitializeBap(Node *node, 
                                MacDataDot11* dot11,
                                MapValue *keyValues, 
                                Mac802Address destAddr);

    void InitializeRoundRobinIterator()
    {
      roundRobinKeyItr = OutputBufferMap.begin();
    }

    OutputBuffer()
    {
      roundRobinKeyItr = OutputBufferMap.begin();
    }

    BOOL checkAndGetKeyPosition()
    {
         Mapitr = OutputBufferMap.find(tmpKey);
         if (Mapitr == OutputBufferMap.end())
         {
           return FALSE;
         }
         else
         {
           return TRUE;
         }
    }
};

void MacDot11nClassifyPacket(Node* node,
                             MacDataDot11* dot11);

void MacDot11nInit(Node *node,
                   NetworkType networkType,
                   const NodeInput* nodeInput,
                   MacDataDot11* dot11);

void MacDot11nHandleDIFSorEIFSTimerExpire(Node *node,
                                          MacDataDot11* dot11);

void MacDot11nHandleBOTimerExpire(Node *node,
                                  MacDataDot11* dot11);

BOOL MacDot11nStaMoveAPacketFromTheMgmtQueueToTheLocalBuffer(
        Node* node,
        MacDataDot11* dot11);

BOOL MacDot11nIsACsHasMessage(MacDataDot11* dot11);

 void MacDot11nSetCurrentAC(MacDataDot11* dot11);

 void MacDot11nHandleTimeout(Node* node,
                             MacDataDot11* dot11,
                             Message* msg);

BOOL MacDot11nDequeuePacketFromOutputBuffer(Node* node,
                                MacDataDot11* dot11,
                                int acIndex);

void Macdot11nReceivePacketFromPhy(Node *node, 
                                   MacDataDot11 *dot11, 
                                   Message* msg);

void MacDot11nProcessDataFrame(Node* node,
                               MacDataDot11* dot11, 
                               Message *frame);

class AmsduBuffer
{
    public:
    // Data Member Declaration
    // MapKey pair Object
    std::pair<Mac802Address,TosType> tmpKey;
    std::map<std::pair<Mac802Address,TosType>,MapValue> AmsduBufferMap;
    std::map<std::pair<Mac802Address,TosType>,MapValue>::iterator Mapitr;
    // Vector Iterator
    std::list<DOT11_FrameInfo*>::iterator tmpVitr;
    // Member Function Declaration
    void Enqueue(MacDataDot11* dot11,
                 Node* node, 
                 Mac802Address ipDestAddr,
                 TosType priority,
                 DOT11_FrameInfo *frameInfo)
    {
        tmpKey.first = ipDestAddr;
        tmpKey.second = priority;
        Mapitr = AmsduBufferMap.find(tmpKey);
        // Value to this Key already Exist
        if (Mapitr != AmsduBufferMap.end())
        {
            Mapitr->second.frameQueue.push_back(frameInfo);
            Mapitr->second.aggSize 
                = Mapitr->second.aggSize 
                    + MESSAGE_ReturnPacketSize(frameInfo->msg);
         Mapitr->second.numPackets++;
        }
        else
        {
            MapValue tmpMapValue;  
            //memset(&tmpMapValue, 0, sizeof(MapValue));
            tmpMapValue.aggSize = MESSAGE_ReturnPacketSize(frameInfo->msg);
            tmpMapValue.numPackets++;
            // Set the Msg in MapValue Message Vector
            tmpMapValue.frameQueue.push_back(frameInfo);
            AmsduBufferMap.insert(std::pair<std::pair<Mac802Address,TosType>,
                 MapValue>(tmpKey,tmpMapValue));
        }
    }
};


class InputBuffer
{
    public:
    // Data Member Declaration
    // MapKey pair Object
    std::pair<Mac802Address,TosType> tmpKey;
    std::map<std::pair<Mac802Address,TosType>,MapValue> InputBufferMap;
    std::map<std::pair<Mac802Address,TosType>,MapValue>::iterator Mapitr;
    std::map<std::pair<Mac802Address,TosType>,MapValue>
        ::iterator roundRobinKeyItr;
    // Vector Iterator
    std::list<DOT11_FrameInfo*>::iterator tmpVitr;\
    // Member Function Declaration
    void Enqueue(MacDataDot11* dot11,
                 Node* node,
                 Mac802Address ipDestAddr,
                 TosType priority,
                 DOT11_FrameInfo *frameInfo,
                 BOOL isKeyAlreadyPresent);

    InputBuffer()
    {
      //roundRobinKeyItr = OutputBufferMap.begin();
    }

    BOOL checkAndGetKeyPosition()
    {
        Mapitr = InputBufferMap.find(tmpKey);
        if (Mapitr == InputBufferMap.end())
        {
        return FALSE;
        }
        else
        {
        return TRUE;
        }
    }
};

void MacDot11nDequeuePacketFromAmsduBuffer(Node* node,
                                           MacDataDot11* dot11,
                                           TosType priority,
                                           Mac802Address NextHopAddress,
                                           BOOL seqNoAdvance,
                                           UInt16 seqNo,
                                           BOOL timerExpire);

 void MacDot11nProcessManagementFrame(Node *node,
                                      MacDataDot11 *dot11,
                                      Message *msg);

BOOL MacDot11nCanAmpduBeCreated(Node *node,
                                MacDataDot11 *dot11, 
                                int acIndex,
                                UInt8 *numPackets);

BOOL MacDot11nCanPacketBeQueuedInOutputBuffer(MacDataDot11* dot11,
                                              Mac802Address nextHopAddress,
                                              TosType priority,
                                              BOOL *isKeyAlreadyPresent);

void MacDot11nCreateAmpdu(Node *node,
                          MacDataDot11 *dot11,
                          int acIndex,
                          UInt8 numPackets);

void MacDot11nProcessAck(Node *node,
                         MacDataDot11* dot11);

void MacDot11nReceivePacketFromPhy(Node* node,
                                   MacDataDot11* dot11,
                                   Message* msg);

BOOL MacDot11nOtherAcsHaveData(MacDataDot11* dot11,
                               int AcIndex);

void MacDot11ProcessAnyFrame(Node* node,
                             MacDataDot11* dot11,
                             Message* msg);

void MacDot11nHandleInputBufferTimer(Node *node,
                                     MacDataDot11* dot11,
                                     Message *msg);

void MacDot11nHandleAmsduBufferTimer(Node *node,
                                     MacDataDot11* dot11,
                                     Message *msg);

void MacDot11nPauseOtherAcsBo(Node* node,
                              MacDataDot11* dot11,
                              clocktype backoff);

void MacDot11nResetOutputBuffer(Node *node,
                                MacDataDot11* dot11,
                                BOOL updateStats,
                                BOOL freeOutputBuffer);

void MacDot11nResetAmsduBuffer(Node *node,
                               MacDataDot11* dot11,
                               BOOL updateStats,
                               BOOL freeAmsduBuffer);

void MacDot11nResetInputBuffer(Node *node,
                               MacDataDot11* dot11,
                               BOOL updateStats,
                               BOOL freeInputBuffer);

void MacDot11nSendMpdu(Node* node,
                       MacDataDot11* dot11,
                       BOOL useRifs = FALSE);

void MacDot11nCheckForOutgoingPacket(Node* node,
                                     MacDataDot11* dot11,
                                     BOOL backoff);

void MacDot11nBroadcastTramsmitted(Node *node,
                                   MacDataDot11* dot11);

BOOL MacDot11nBlockAckPolicyExists(MacDataDot11* dot11,
                                   int acIndex);

BOOL MacDot11nBlockAckPolicyAvailable(Node *node,
                                      int acIndex);

BOOL MacDot11nBlockAckPolicyUsable(Node *node,
                                  MacDataDot11 *dot11,
                                  std::map<std::pair<Mac802Address,
                                  TosType>,
                                  MapValue>::iterator keyItr,
                                  UInt8 *numPackets,
                                  BOOL reCalculating);

void MacDot11nSendAddbaRequest(Node *node,
                               MacDataDot11 *dot11,
                               std::map<std::pair<Mac802Address,
                               TosType>,
                               MapValue>::iterator keyItr,
                               UInt8 numPackets);

void MacDot11nHandleBapTimer(Node *node,
                             MacDataDot11 *dot11, 
                             timerMsg *timer2);

void MacDot11nHandleDequeuePacketFromInputBuffer(Node *node, 
                                                 MacDataDot11 *dot11, 
                                                 DOT11_FrameInfo *frameInfo);

void MacDot11nSendPacketsUnderBaa(Node *node, 
                                  MacDataDot11 *dot11,
                                  int acIndex,
                                  std::map<std::pair<Mac802Address,
                                  TosType>,
                                  MapValue>::iterator &keyItr);

void MacDot11nUnicastTransmitted(Node* node,
                                 MacDataDot11* dot11);

BOOL MacDot11nCalcNumPacketsSentUnderBap(Node* node,
                                         MacDataDot11* dot11,
                                         int acIndex,
                                         UInt8 *numpkts,
                                         std::map<std::pair<Mac802Address,
                                         TosType>,
                                         MapValue>::iterator *keyItrPtr);

BlockAckAggStates MacDot11nGetBapState(Node* node,
                                       MacDataDot11* dot11);

void MacDot11nSendBlockAckRequest(Node *node,
                                  MacDataDot11 *dot11,
                                  std::map<std::pair<Mac802Address,
                                  TosType>,
                                  MapValue>::iterator keyItr);


void MacDot11nUpdateBapForBar(Node *node,
                              MacDataDot11 *dot11, 
                              DOT11_FrameInfo *frameInfo,
                              BOOL success);

void MacDot11nUpdateBapForActionFrames(Node *node, 
                                       MacDataDot11 *dot11, 
                                       DOT11_FrameInfo *frameInfo,
                                       BOOL success);

clocktype MacDot11nGetNavDuration(Node *node, 
                                  MacDataDot11 *dot11,
                                  DOT11_FrameInfo *frameInfo);

BOOL MacDot11nSetMessageInAC(Node *node,
                             MacDataDot11 *dot11, 
                             int acIndex);

void MacDot11nCalculateNumPacketsToBeSentInTxOp(
                                    Node *node,
                                    MacDataDot11* dot11,
                                    int acIndex,
                                    std::map<std::pair<Mac802Address,
                                    TosType>,
                                    MapValue>::iterator keyItr,
                                    BOOL reCalculating);

void MacDot11nResetCurrentMessageVariables(MacDataDot11* const dot11);

void
MacDot11nResetACVariables(MacDataDot11* dot11,
                          int acIndex,
                          BOOL free = TRUE);

void MacDot11nResetbap(std::map<std::pair<Mac802Address,
                       TosType>,
                       MapValue>::iterator keyItr);

DOT11n_IBSS_Station_Info* MacDot11nGetIbssStationInfo(
                            MacDataDot11 *dot11,
                            Mac802Address destAddr);

DOT11n_IBSS_Station_Info* MacDot11nAddIbssStationInfo(
                            MacDataDot11 *dot11,
                            Mac802Address destAddr);

void MacDot11nIbssHandleProbeRequestTimer(Node *node,
                                          MacDataDot11 *dot11,
                                          Message *msg);

void MacDot11nResetIbssStationInfo(Node *node,
                                   MacDataDot11 *dot11);

#endif
