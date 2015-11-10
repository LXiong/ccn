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

// /**
// PACKAGE     :: CELLULAR_UMTS
// DESCRIPTION :: Defines constants, enums, structures used in the UMTS
//                RLC sublayer.
// **/

#ifndef UMTS_LAYER2_RLC_H
#define UMTS_LAYER2_RLC_H
#include <utility>
#include <deque>
#include <list>
#include <deque>
#include <vector>
#include <set>

#include "cellular_umts.h"

// /**
// CONSTANT    :: UMTS_RLC_DEF_MAXBUFSIZE
// DESCRIPTION :: Default value of the maximum buffer size 
// **/
const UInt64 UMTS_RLC_DEF_MAXBUFSIZE = 50000;

// /**
// CONSTANT    :: UMTS_RLC_MAXPDUSIZE
// DESCRIPTION :: Default value of the maximum pdu size 
// **/
const unsigned int UMTS_RLC_MAXPDUSIZE = 5000;

// /**
// CONSTANT    :: UMTS_RLC_AM_SNUPPERBOUND
// DESCRIPTION :: Upper bound of AM sequence number 
// **/
const unsigned int UMTS_RLC_AM_SNUPPERBOUND = 4096;

// /**
// CONSTANT    :: UMTS_RLC_UM_SNUPPERBOUND
// DESCRIPTION :: Upper bound of UM sequence number 
// **/
const unsigned int UMTS_RLC_UM_SNUPPERBOUND = 128;

// /**
// CONSTANT    :: UMTS_RLC_LI_XXXXXX
// DESCRIPTION ::
//     Special length indicator values,
//     assume the last bit (E) in the octet is 0.
// **/
const UInt8 UMTS_RLC_LI_7BIT_PREVFILL = 0x0;
const UInt8 UMTS_RLC_LI_7BIT_SDUSTART = 0x7C;
const UInt8 UMTS_RLC_LI_7BIT_SDUFILL = 0x7D;
const UInt8 UMTS_RLC_LI_7BIT_ONEPDU = 0x7E;
const UInt8 UMTS_RLC_LI_7BIT_PADDING = 0x7F;

const UInt16 UMTS_RLC_LI_15BIT_PREVFILL = 0x0;
const UInt16 UMTS_RLC_LI_15BIT_SDUSHORT = 0x7FFA;
const UInt16 UMTS_RLC_LI_15BIT_PREVSHORT = 0x7FFB;
const UInt16 UMTS_RLC_LI_15BIT_SDUSTART = 0x7FFC;
const UInt16 UMTS_RLC_LI_15BIT_SDUFILL = 0x7FFD;
const UInt16 UMTS_RLC_LI_15BIT_ONEPDU = 0x7FFE;
const UInt16 UMTS_RLC_LI_15BIT_PADDING = 0x7FFF;

// /**
// CONSTANT::
// DESCRIPTION::
//      The PDU size threshold used to determine
//      the length of length indicator in UM uplink and AM
// **/
const unsigned int UMTS_RLC_UM_LI_PDUTHRESH = 125;
const unsigned int UMTS_RLC_AM_LI_PDUTHRESH = 126;


// /**
// CONSTANT::
// DESCRIPTION::
//      Constants that indicates the traits of an SDU segment 
//      for re-assembly process.
//      The first two traits are used to identify all segments
//      belonging to the same
//      SDU so that reassembly can be proceeded 
//      when all of them are available.
//      The rest of traits are used to characterize some segments
//      that are needed to
//      reassembly SDUs, of which their previous PDUs consist of the
//      last segment and those PDU does not have LI indicating that.
// **/
const unsigned int UMTS_RLC_SDUSEGMENT_START = 0x0001;       
                                         //The segment starts an SDU

const unsigned int UMTS_RLC_SDUSEGMENT_END = 0x0002;         
                                         //The segment ends an SDU

const unsigned int UMTS_RLC_SDUSEGMENT_PREVSHORT= 0x0008;    
                                         //Previous segment 1-octet-short 
                                         // fills a PDU w/o LI ind. that

// /**
// CONSTANT::
// DESCRIPTION::
//     Control PDU types set in the header
// **/
const UInt8 UMTS_RLC_CONTROLPDU_STATUS = 0x0;
const UInt8 UMTS_RLC_CONTROLPDU_RESET = 0x1;
const UInt8 UMTS_RLC_CONTROLPDU_RESETACK = 0x2;

// /**
// CONSTANT::
// DESCRIPTION::
//     SUFI type definitions
// **/
const UInt8 UMTS_RLC_SUFITYPE_NOMORE  =   0x0;
const UInt8 UMTS_RLC_SUFITYPE_WINDOW  =   0x1;
const UInt8 UMTS_RLC_SUFITYPE_ACK     =   0x2;
const UInt8 UMTS_RLC_SUFITYPE_LIST    =   0x3;
const UInt8 UMTS_RLC_SUFITYPE_BIMAP   =   0x4;
const UInt8 UMTS_RLC_SUFITYPE_Rlist   =   0x5;
const UInt8 UMTS_RLC_SUFITYPE_MRW     =   0x6;
const UInt8 UMTS_RLC_SUFITYPE_MRWACK  =   0x7;

// /**
// CONSTANT::
// DESCRIPTION::
//     Flags used to indicate which SUFI should be included
//     in the next STATUS PDU
// **/
const unsigned int UMTS_RLC_STATUSPDU_ACK = 0x01;
const unsigned int UMTS_RLC_STATUSPDU_MRW = 0x02;
const unsigned int UMTS_RLC_STATUSPDU_MRWACK = 0x04;
const unsigned int UMTS_RLC_STATUSPDU_WINDOW = 0x08;

// /**
// CONSTANT::
// DESCRIPTION::
//     Flags used to indicate which poll trigger is used
// **/
const unsigned int UMTS_RLC_POLL_LASTPDU = 0x0001;
const unsigned int UMTS_RLC_POLL_LASTRXPDU = 0x0002;
const unsigned int UMTS_RLC_POLL_TIMERPOLL = 0x0004;
const unsigned int UMTS_RLC_POLL_PDU = 0x0008;
const unsigned int UMTS_RLC_POLL_SDU = 0x0010;
const unsigned int UMTS_RLC_POLL_WND = 0x0020;
const unsigned int UMTS_RLC_POLL_TIMERPERIODIC = 0x0040;

// /**
// ENUM::UmtsRlcEntityState
// DESCRIPTION:: RLC entity states
//
// **/
typedef enum
{
    UMTS_RLC_ENTITYSTATE_READY = 0,          //DATA_TRANSFER_READY
    UMTS_RLC_ENTITYSTATE_RESETPENDING,       //RESET_PENDING
} UmtsRlcEntityState;


// /**
// STRUCT:: UmtsRlcTimerInfoHdr
// DESCRIPTION::
//      The first part of RLC timer info
// **/
typedef struct {
    NodeId ueId;
    char rbId;
    UmtsRlcEntityDirect direction;
} UmtsRlcTimerInfoHdr;

// /**
// STRUCT::     UmtsRlcFreeMessage
// DESCRIPTION::
//      Function object used to free Message pointer in STL container
// **/
class UmtsRlcFreeMessage
{
    Node* node;
public:
    explicit UmtsRlcFreeMessage(Node* ndPtr):node(ndPtr) { }
    void operator() (Message* msg)
    {
        while (msg)
        {
            Message* nextMsg = msg->next;
            MESSAGE_Free(node, msg);
            msg = nextMsg;
        }
    }
};

// /**
// STRUCT::     UmtsRlcPrintPdu
// DESCRIPTION::
//      Function object used to print PDU message contents
//        in a STL container
// **/
inline void UmtsRlcPrintPdu(const Message* msg)
{
    printf("Message contents: ");
#if 0
    while (msg)
    {
        for (int i = 0; i < msg->packetSize; i++)
            printf("%d", (char)msg->packet[i]);
        msg = msg->next;
    }
#endif
    printf("\n");
};

// /**
// STRUCT:: UmtsRlcHeader
// DESCRIPTION::
//      A C++ class for facilitate manipulating RLC PDU header
// **/
class UmtsRlcHeader
{
    union HdrUnit
    {
        UInt8   octet;
        UInt16  doubleOctet;
        HdrUnit() { octet = 0; doubleOctet = 0;}
    };

    struct LiList
    {
        HdrUnit val;
        LiList* next;
    };

    UmtsRlcEntityType mode;         //PDU mode: UM/AM
    unsigned int    liLen;          //The number of octets used to 
                                    // contain a length indicator
    UInt8   oct1;
    UInt8   oct2;
    LiList* liHead;
    int hdrSize;
    LiList* liSeekPtr;

    LiList* getLastLi()
    {
        LiList* liPtr = liHead;
        LiList* liNext = liPtr->next;
        while (liNext)
        {
            liPtr = liNext;
            liNext = liNext->next;
        }
        return liPtr;
    }
public:
    explicit UmtsRlcHeader(UmtsRlcEntityType type,
        unsigned int liOctetLen);

    ~UmtsRlcHeader();

    int GetHdrSize() const
    {
        return hdrSize;
    }

    void clear();
    void GetHdr(char* hdr) const;
    void AddLiOctet(UInt8 liVal);
    void AddLiDbOctet(UInt16 liVal);
    void BuildHdrStruct(const char* rlcHeader);
    BOOL ReadNextLi(unsigned int& liVal);
    BOOL PeekNextLi(unsigned int& liVal);

    BOOL nextLiExist()
    {
        return (liSeekPtr!=NULL)? TRUE: FALSE;
    }
    friend std::ostream& operator<<(std::ostream& os,
                                    const UmtsRlcHeader& hdr);
};

struct UmtsRlcEntity;

// /**
// STRUCT:: UmtsRlcUmAmFragmentEntity
// DESCRIPTION::
//      An entity controlling the fragmentations of UM and AM SDUs
// **/
class UmtsRlcUmAmFragmentEntity
{
    Node* node;
    UmtsRlcEntity* rlcEntity;
    Message* lastPdu;
    BOOL  prevPduFill;      //Previous PDU has been exactly filled with
                            //last segment of a SDU, and no length
                            //indicator indicates that in previous PDU
    BOOL  prevPduShort;     //Previous PDU has been one-octet-short filled 
                            //with last segment of a SDU, and no length
                            //indicator indicates that in previous PDU

    void AddRlcHeader(
        Node* node,
        const UmtsRlcHeader& pduHeaderStruct,
        Message* pduMsg);

    void releaseLastPdu()
    {
        if (lastPdu)
        {
            Message* curMsg = lastPdu;
            while (curMsg)
            {
                Message* nextMsg = curMsg->next;
                MESSAGE_Free(node, curMsg);
                curMsg = nextMsg;
            }
        }
        lastPdu = NULL;
    }
public:
    explicit UmtsRlcUmAmFragmentEntity(Node* nodePtr,
                                       UmtsRlcEntity* entity)
        : node(nodePtr), rlcEntity(entity)
    {
        char errorMsg[MAX_STRING_LENGTH] = "UmtsRlcUmAmFragmentEntity: ";
        ERROR_Assert(rlcEntity, errorMsg);
        lastPdu = NULL;
        prevPduFill = FALSE;
        prevPduShort = FALSE;
    }

    ~UmtsRlcUmAmFragmentEntity()
    {
        releaseLastPdu();
    }

    BOOL isLastPduGone() { return lastPdu == NULL; }

    BOOL Fragment(Node* node,
                int iface,
                Message* sduMsg,
                unsigned int umNumPdus = 0,
                unsigned int umPduSize = 0);

    BOOL RequestLastPdu(Node* node);

#if 0
    void Reassembly(Node* node,
                int iface,
                Message* pduMsg);
#endif // 0
    void Store(Node* node, std::string& buffer);
    void Restore(Node* node, const char* buffer, int& index);
};

// /**
// STRUCT::     UmtsRlcSduSegment
// DESCRIPTION::
//      The sdu segment for re-assembly process
// **/
struct UmtsRlcSduSegment
{
    unsigned int seqNum;
    unsigned int traits;
    Message*    message;

    void store(Node* node, std::string& buffer)
    {
        buffer.append((char*)&seqNum, sizeof(seqNum));
        buffer.append((char*)&traits, sizeof(traits));
        UmtsSerializeMessage(node, message, buffer);
    }

    void restore(Node* node, const char* buffer, int& index)
    {
        memcpy(&seqNum, &buffer[index], sizeof(seqNum));
        index += sizeof(seqNum);
        memcpy(&traits, &buffer[index], sizeof(traits));
        index += sizeof(traits);
        message = UmtsUnSerializeMessage(node, buffer, index);
    }
};

// /**
// STRUCT::     UmtsRlcSduSegInfo
// DESCRIPTION::
//      The information stored in the info field
//      of each SDU segment
// **/
struct UmtsRlcSduSegInfo
{
    unsigned int traits;
};

// /**
// STRUCT::     UmtsRlcAmReTxPduData
// DESCRIPTION::
//      The AM retransmission PDU related data structure
// **/
struct UmtsRlcAmRtxPduData
{
    unsigned int seqNum;
    unsigned int dat;   //VT(DAT), number of times this PDU has been
                        // scheduled to be transmitted
    BOOL     txScheduled; //Whether it's scheduled to be retransmitted
                          // for the next time
    Message* message;

    void store(Node* node, std::string& buffer)
    {
        buffer.append((char*)&seqNum, sizeof(seqNum));
        buffer.append((char*)&dat, sizeof(dat));
        buffer.append((char*)&txScheduled, sizeof(txScheduled));

        std::string msgBuf;
        msgBuf.reserve(500);
        int numMsgs = UmtsSerializeMessageList(
                        node,
                        message,
                        msgBuf);
        buffer.append((char*)&numMsgs, sizeof(numMsgs));
        buffer.append(msgBuf);
    }
    void restore(Node* node, const char* buffer, int& index)
    {
        int numMsgs;
        memcpy(&seqNum, &buffer[index], sizeof(seqNum));
        index += sizeof(seqNum);
        memcpy(&dat, &buffer[index], sizeof(dat));
        index += sizeof(dat);
        memcpy(&txScheduled, &buffer[index], sizeof(txScheduled));
        index += sizeof(txScheduled);

        memcpy(&numMsgs, &buffer[index], sizeof(numMsgs));
        index += sizeof(numMsgs);
        message = UmtsUnSerializeMessageList(
                                   node,
                                   buffer,
                                   index,
                                   numMsgs);
    }
};

// /**
// STRUCT::     UmtsRlcRtxPduSeqNumEqual
// DESCRIPTION::
//      Function object used to find an RtxPduData with sequence
//      Number equal to a value
// **/
class UmtsRlcRtxPduSeqNumEqual
{
    unsigned int sn;
public:
    explicit UmtsRlcRtxPduSeqNumEqual(unsigned int seqNum)
                : sn(seqNum) { }
    bool operator() (const UmtsRlcAmRtxPduData* rtx) const
    {
        return rtx->seqNum == sn;
    }
};

// /**
// STRUCT::     UmtsRlcFreeMessageData
// DESCRIPTION::
//      Function object used to delete sduSegment in the list
// **/
template<typename T>
class UmtsRlcFreeMessageData
{
    Node* node;
public:
    explicit UmtsRlcFreeMessageData(Node* ndPtr):node(ndPtr) { }
    void operator() (T* messageData)
    {
        Message* msg = messageData->message;
        while (msg)
        {
            Message* nextMsg = msg->next;
            MESSAGE_Free(node, msg);
            msg = nextMsg;
        }
        MEM_free(messageData);
    }
};

// /**
// STRUCT:: UmtsRlcSduInfo
// DESCRIPTION::
//      A general RLC SDU info
// **/
struct UmtsRlcSduInfo
{
    unsigned int sduSeqNum;
};

// /**
// STRUCT:: UmtsRlcPollTimerInfo
// DESCRIPTION::
//      A Poll Timer info struct
// **/
struct UmtsRlcPollTimerInfo
{
    unsigned int sndNextSn;
};

// /**
// STRUCT:: UmtsRlcEntity
// DESCRIPTION::
//      A general RLC entity structure
// **/
struct UmtsRlcEntity
{
    NodeId                      ueId;
    char                        rbId;

    unsigned int                sduSeqNum;
    UmtsRlcEntityType           type;
    void*                       entityData;
    clocktype lastDataDequeueTime;

    UmtsRlcEntity(NodeAddress ue, char rb):ueId(ue), rbId(rb)
    {
        type = UMTS_RLC_ENTITY_TM;
        entityData = NULL;
        lastDataDequeueTime = 0;
    }
    bool operator< (const UmtsRlcEntity& cmpEntity) const
    {
        if (rbId == cmpEntity.rbId)
        {
            if (rbId == UMTS_CCCH_RB_ID ||
                rbId == UMTS_BCCH_RB_ID ||
                rbId == UMTS_PCCH_RB_ID ||
                rbId == UMTS_CTCH_RB_ID)
            {
                return FALSE;
            }
            else
            {
                return ueId < cmpEntity.ueId;
            }
        }
        else
        {
            return rbId < cmpEntity.rbId;
        }
    }

    void reset(Node* node);
    void store(Node* node, std::string& buffer);
    void restore(Node* node, const char* buffer, size_t bufSize);
};

// /**
// STRUCT:: UmtsRlcEntityRbEqual
// DESCRIPTION::
//      Function object to find if an entity in the list with given rbId
// **/
class UmtsRlcEntityRbEqual {
    unsigned int rbId;
public:
    explicit UmtsRlcEntityRbEqual(unsigned int rbIdentifier)
                :rbId(rbIdentifier) { }
    bool operator() (const UmtsRlcEntity* lEntity) const
    {
        return (unsigned)(lEntity->rbId) == rbId;
    }
};

// /**
// STRUCT:: UmtsRlcEntityPtrLess
// DESCRIPTION::
//      A function object class comparing RLC entity
// **/
struct UmtsRlcEntityPtrLess {
    bool operator() (const UmtsRlcEntity* lEntity,
                     const UmtsRlcEntity* rEntity) const
    {
        return *lEntity < *rEntity;
    }
};

// /**
//STRUCT:: UmtsTmTransEntity
//DESCRIPTION::
//      RLC TM transmitting entity
//**/
struct UmtsRlcTmTransEntity
{
    UmtsRlcEntity*              entityVar;
    UInt64                      bufSize;
    BOOL                        segIndi;
    std::deque<Message*>*       transSduBuf;

    void reset(Node* node);
    void store(Node* node, std::string& buffer);
    void restore(Node* node, const char* buffer, size_t bufSize);
};

//STRUCT:: UmtsRlcTmRcvEntity
//DESCRIPTION::
//      RLC TM Receiving entity
//**/
struct UmtsRlcTmRcvEntity
{
    UmtsRlcEntity*              entityVar;
    BOOL                        segIndi;

    void reset(Node* node)
    {
    }

    void store(Node* node, std::string& buffer)
    {
    }

    void restore(Node* node, const char* buffer, size_t bufSize)
    {
    }

};

// /**
// STRUCT::     UmtsRlcTmEntity
// DESCRIPTION::
//      TM entity structure
// **/
struct UmtsRlcTmEntity
{
    UmtsRlcTmTransEntity* txEntity;
    UmtsRlcTmRcvEntity* rxEntity;

    void reset(Node* node);
    void store(Node* node, std::string& buffer);
    void restore(Node* node, const char* buffer, size_t bufSize);
};

// /**
//STRUCT:: UmtsRlcUmTransEntity
//DESCRIPTION:: RLC UM transmitting entity
//**/
struct UmtsRlcUmTransEntity
{
    UmtsRlcEntity*              entityVar;

    UInt64                      maxTransBufSize;
    UInt64                      bufSize;
    unsigned long               maxUlPduSize;
    int                         liOctetsLen;
    BOOL                        alterEbit;

    UmtsRlcUmAmFragmentEntity*  fragmentEntity;
    std::deque<Message*>*       transSduBuf;
    std::deque<Message*>*       pduBuf;
    unsigned int                sndNext;

    void reset(Node* node);
    void store(Node* node, std::string& buffer);
    void restore(Node* node, const char* buffer, size_t bufSize);
};

// /**
//STRUCT:: UmtsRlcUmRcvEntity
//DESCRIPTION:: RLC UM receiving entity
//**/
struct UmtsRlcUmRcvEntity
{
    UmtsRlcEntity*              entityVar;

    BOOL                        alterEbit;
    unsigned long               maxUlPduSize;
    int                         liOctetsLen;

    unsigned int                rcvNext;
    std::list<UmtsRlcSduSegment*>*   sduSegmentList;

    void reset(Node* node);
    void store(Node* node, std::string& buffer);
    void restore(Node* node, const char* buffer, size_t bufSize);
};

// /**
// STRUCT::     UmtsRlcUmEntity
// DESCRIPTION::
//      UM entity structure
// **/
struct UmtsRlcUmEntity
{
    UmtsRlcUmTransEntity* txEntity;
    UmtsRlcUmRcvEntity* rxEntity;

    void reset(Node* node);
    void store(Node* node, std::string& buffer);
    void restore(Node* node, const char* buffer, size_t bufSize);
};

// /**
// STRUCT:: UmtsRlcAmEntity
// DESCRIPTION::
//      AM entity (including both sending and receiving function)
// **/
struct UmtsRlcAmEntity
{
    UmtsRlcEntity*              entityVar;
    UmtsRlcEntityState          state;
    UInt64                      maxTransBufSize;
    UInt64                      bufSize;

    unsigned int                maxDat;
    unsigned int                maxRst;
    unsigned int                pollTrig;

    std::deque<Message*>*       transPduBuf;
    std::list<UmtsRlcAmRtxPduData*>*  rtxPduBuf;
    unsigned int                sndNextSn;
    UmtsRlcUmAmFragmentEntity*  fragmentEntity;
    unsigned int                ackSn;
    unsigned int                sndWnd;
    unsigned int                sndPduSize;
    int                         sndLiOctetsLen;

    std::deque<BOOL>*           pduRcptFlag;
    std::list<UmtsRlcSduSegment*>*   sduSegmentList;
    unsigned int                rcvNextSn;
    unsigned int                rcvHighestExpSn;
    unsigned int                rcvWnd;
    unsigned int                rcvPduSize;
    int                         rcvLiOctetsLen;

    BOOL                        pollTriggered;
    UmtsRlcPollTimerInfo        pollTimerInfo;
    Message*                    pollTimerMsg;

    unsigned int                statusPduFlag;

    unsigned int                rstSn;
    unsigned int                numRst;


    clocktype                   rstTimerVal;
    Message*                    rstTimerMsg;
    //clocktype                   rstTimerVal;
    Message*                    rstPduMsg;
    unsigned int                lastRcvdRstSn;
    Message*                    rstAckPduMsg;

    void reset(Node* node);
    void store(Node* node, std::string& buffer);
    void restore(Node* node, const char* buffer, size_t bufSize);
};

// /**
// STRUCT:: UmtsRlcStats
// DESCRIPTION:: RLC statistics
// **/
typedef struct
{
    unsigned int numSdusFromUpperLayer;
    unsigned int numSdusToUpperLayer;
    unsigned int numPdusToMac;
    unsigned int numPdusFromMac;
    unsigned int numSdusDiscarded;
    unsigned int numAmStatusPdusToMac;
    unsigned int numAmStatusPdusFromMac;
    unsigned int numAmResetPdusToMac;
    unsigned int numAmResetPdusFromMac;
    unsigned int numAmResetAckPdusToMac;
    unsigned int numAmResetAckPdusFromMac;

    //dyanmic statistics
    UInt64 numInControlByte; // FOR UE In is DL, OUT iS UL
    UInt64 numInDataByte;  // FOR NODEB, in UL, oUT IS DL
    UInt64 numOutControlByte;
    UInt64 numOutDataByte;
    int dlThruputGuiId;
    int ulThruputGuiId;

} UmtsRlcStats;

typedef std::list<UmtsRlcEntity*>  UmtsRlcEntityList;
typedef std::set<UmtsRlcEntity*, UmtsRlcEntityPtrLess>  UmtsRlcEntitySet;
#if 0
typedef std::vector<std::string> UmtsCellSwitchFragVec;
#endif //Old cell switch code

// /**
// STRUCT:: Layer2UmtsRlcData
// DESCRIPTION:: RLC sublayer structure at a UE
// **/
typedef struct {
    UmtsRlcEntityList* ueEntityList;
    UmtsRlcEntitySet* rncEntitySet;

#if 0
    UmtsCellSwitchFragVec* cellSwitchFrags;
    int  cellSwitchFragSum;
    int  cellSwitchFragNum;
#endif //Old cell switch code
    UmtsRlcStats stats;
} UmtsRlcData;

// /**
// STRUCT::     UmtsRlcPackedPduInfo
// DESCRIPTION::
//      The information stored in the info field
//      of each PDU after packing
// **/
typedef struct
{
    unsigned int pduSize;
} UmtsRlcPackedPduInfo;

// /**
// STRUCT::     UmtsRlcPduListInfo
// DESCRIPTION::
//      The information of the PDU List specific for a unique
//      RB, stored in the info field of the head message in
//      the PDU list.
// **/
typedef struct
{
    NodeId       ueId;
    char         rbId;
    int          len;       //number of PDUs to the end of the PDU lists
} UmtsRlcPduListInfo;

// /**
// STRUCT::     UmtsRlcEntityInfoToMac
// DESCRIPTION::
//      Rlc entity status infomation sent to MAC
// **/
typedef struct
{
    UmtsRlcEntityType entityType;
    UInt64            bufOccupancy;
    UInt64            maxBufSize;
    clocktype         lastAccessTime;
}UmtsRlcEntityInfoToMac;

// /**
// FUNCTION::       UmtsRlcUpperLayerHasSduToSend
// LAYER::          UMTS Layer2 RLC
// PURPOSE::        Called by upper layers to request
//                  RLC to send SDU
// PARAMETERS::
//    + node : Node* : pointer to the network node
//    + interfaceIndex : int : index of interface
//    + rbId : unsigned int : radio bearer Id assc. with this RLC entity
//    + sduMsg : Message* : The SDU message from upper layer
//    + ueId : NodeAddress : UE identifier, used to look up RLC entity
//                           at UTRAN side
// RETURN::         NULL
// **/
void  UmtsRlcUpperLayerHasSduToSend(
    Node* node,
    int interfaceIndex,
    unsigned int rbId,
    Message* sduMsg,
    NodeAddress ueId);

// /**
// FUNCTION::       UmtsRlcHandleInterLayerCommand
// LAYER::          UMTS Layer2 RLC
// PURPOSE::        Handle interlayer command
// PARAMETERS::
//    +node : Node* : Node pointer
//    +iface : int : index of interface
//    +cmdType : UmtsInterlayerCommandType : RRC control command type
//    +cmdArgs : const void* : Command arguments
// RETURN::         BOOL
// **/
BOOL UmtsRlcHandleInterLayerCommand(
    Node* node,
    int iface,
    UmtsInterlayerCommandType cmdType,
    const void* cmdArgs);

// /**
// FUNCTION::       UmtsRlcReceivePduFromMac
// LAYER::          UMTS Layer2 RLC
// PURPOSE::        Called by MAC to submit an received RLC PDU
// PARAMETERS::
//    + node : Node* : pointer to the network node
//    + interfaceIndex : int : interdex of interface
//    + rbId : unsigned int : radio bearer Id asscociated 
//                            with this RLC entity
//    + msgList : std::list<Message*>& : Msg list containing PDUs from MAC
//    + errorIndi : BOOL : Error indication, whether some packets are
//                         lost in the TTI
//    + ueId: NodeAddress : UE identifier, used to look up RLC 
//                          entity at UTRAN side
// RETURN:: NULL
// **/
void UmtsRlcReceivePduFromMac(
    Node* node,
    int interfaceIndex,
    unsigned int rbId,
    std::list<Message*>& msgList,
    BOOL errorIndi,
    NodeAddress ueId = 0);

// /**
// FUNCTION::       UmtsRlcReceivePduFromMac
// LAYER::          UMTS Layer2 RLC
// PURPOSE::        Called by MAC to submit an received RLC PDU
// PARAMETERS::
//    + node : Node* : pointer to the network node
//    + interfaceIndex : int: interdex of interface
//    + msgList : std::list<Message*>& : Msg list containing PDUs from MAC
//    + errorIndi:  BOOL : Error indication, whether some packets
//                         are lost in the TTI
// RETURN:: NULL
// **/
void UmtsRlcReceivePduFromMac(
    Node* node,
    int interfaceIndex,
    std::list<Message*>& msgList,
    BOOL errorIndi);

// /**
// FUNCTION::       UmtsRlcHasPacketSentToMac
// LAYER::          UMTS Layer2 RLC
// PURPOSE::        Called by MAC to send RLC PDUs to MAC
// PARAMETERS::
//    + node : Node* : pointer to the network node
//    + interfaceIndex : int : index of interface
//    + rbId : unsigned int : radio bearer Id asscociated 
//                           with this RLC entity
//    + numPdus : int : number of PDUs the MAC allows this RLC to send
//    + pduSize : int : PDU size the MAC indicates to the RLC
//    + pduList : std::list<Message*>& :
//                A list containing the pdus to be sent
//    + ueId : NodeAddress: UE identifier, used to look up RLC
//                          entity at UTRAN side
// RETURN:: NULL
// **/
void UmtsRlcHasPacketSentToMac(
    Node* node,
    int interfaceIndex,
    unsigned int rbId,
    int numPdus,
    int pduSize,
    std::list<Message*>& pduList,
    NodeAddress ueId = 0);

// /**
// FUNCTION::       UmtsRlcInitPrimCellChange
// LAYER::          UMTS Layer2 RLC
// PURPOSE::        Start to move the RLC entity (to a UE) status 
//                  in this Cell to another Cell
// PARAMETERS::
//    + node : Node* : pointer to the network node
//    + interfaceIndex : int : index of interface
//    + buffer : std::string& : String buffer to store RLC status
//    + ueId : NodeId : UE identifier, used to look up 
//                      RLC entity at UTRAN side
// RETURN:: NULL
// **/
void UmtsRlcInitPrimCellChange(
    Node* node,
    int interfaceIndex,
    std::string& buffer,
    NodeId ueId);

// /**
// FUNCTION::       UmtsRlcCompletePrimCellChange
// LAYER::          UMTS Layer2 RLC
// PURPOSE::        Finishing moving the RLC entity (to a UE) 
//                  status in this Cell to another Cell
// PARAMETERS::
//    + node : Node* : pointer to the network node
//    + iface : int index of interface
//    + buffer :  const char* : String buffer stored with RLC status
//                             to be moved here
//    + bufSize : UInt64 : The size of the buffer
//    + ueId : NodeId : UE identifier, used to look up 
//                      RLC entity at UTRAN side
//    + restoreDone : BOOL* : restore has been done or not?
// RETURN:: NULL
// **/
#if 0
void UmtsRlcCompletePrimCellChange(
    Node* node,
    int iface,
    BOOL fragHead,
    unsigned int fragNum,
    const char* fragment,
    size_t fragSize,
    NodeId ueId,
    BOOL* restoreDone);
#endif //Old cell switch code

void UmtsRlcCompletePrimCellChange(
    Node* node,
    int iface,
    const char* buffer,
    UInt64 bufSize,
    NodeId ueId,
    BOOL* restoreDone);

// /**
// FUNCTION::       UmtsRlcIndicateStatusToMac
// LAYER::          UMTS Layer2 RLC
// PURPOSE::        Called by MAC to get RLC sending entity info
// PARAMETERS::
//    + node : Node* : pointer to the network node
//    + interfaceIndex: int : index of interface
//    + rbId : unsigned int : radio bearer ID, used to find the RLC entity
//    + info : UmtsRlcEntityInfoToMac& : The RLC information MAC needs
//    + ueId : nodeAddress : UE identifier, used to look up RLC 
//                           entity at UTRAN side
// RETURN:: BOOL:   FALSE if there is no transmission entity for this RB
// **/
BOOL UmtsRlcIndicateStatusToMac(
    Node* node,
    int interfaceIndex,
    unsigned int rbId,
    UmtsRlcEntityInfoToMac& info,
    NodeAddress ueId = 0);


// /**
// FUNCTION::       UmtsRlcInit
// LAYER::          UMTS Layer2 RLC
// PURPOSE::        RLC Initalization
//
// PARAMETERS::
//    + node:       Node* : pointer to the network node
//    + interfaceIndex: unsigned int : interdex of interface
//    + nodeInput:  const NodeInput* : Input from configuration file
// RETURN::         NULL
// **/
void UmtsRlcInit(Node* node, 
                 unsigned int interfaceIndex, 
                 const NodeInput* nodeInput);

// /**
// FUNCTION::       UmtsRlcFinalize
// LAYER::          UMTS Layer2 RLC
// PURPOSE::        RLC finalization function
// PARAMETERS::
//    + node:       Node* : pointer to the network node
//    + interfaceIndex: unsigned int : interdex of interface
// RETURN::         NULL
// **/
void UmtsRlcFinalize(Node* node, unsigned int interfaceIndex);

// /**
// FUNCTION::       UmtsRlcLayer
// LAYER::          UMTS Layer2 RLC
// PURPOSE::        RLC event handling function
// PARAMETERS::
//    + node:       Node * : pointer to the network node
//    + interfaceIndex: unsigned int : interdex of interface
//    + message     Message* : Message to be handled
// RETURN::         NULL
// **/
void UmtsRlcLayer(Node* node, 
                  unsigned int interfaceIndex,
                  Message* message);

// /**
// FUNCTION::       UmtsRlcAmUpdateEmptyTransPduBuf
// LAYER::          UMTS Layer2 RLC
// PURPOSE::        Get the last pdu from fragment buffer,
//                  when amEntity buffer size is zero
// PARAMETERS::
//      +node:              Node pointer
//      +interfaceIndex    interface Index
//      +rbId:              radio bearer Id
//      +ueId              UE identifier, used to look up RLC
// RETURN::                 NULL
// **/
void UmtsRlcAmUpdateEmptyTransPduBuf(Node* node,
                                     UInt32 interfaceIndex,
                                     unsigned int rbId,
                                     NodeAddress ueId);


#endif //UMTS_LAYER2_RLC_H


