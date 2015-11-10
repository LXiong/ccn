#ifndef STATISTIC_H
#define STATISTIC_H

#include "ccn.h"
#include "nodeData.h"

class StatisticalData
{
public:
  // total statistic
  uint64_t Packet_send;
  uint64_t Packet_recv;
  uint64_t PacketSize_send;
  uint64_t PacketSize_recv;
  uint64_t SendToRecv_Delay;
  uint64_t GenerateToRecv_Delay;

  uint64_t cache_hitTimes;          // $B%-%c%C%7%e%R%C%H?t(B
  uint64_t packet_lostTimes;        // $B%Q%1%C%HB;<:2s?t(B
  uint64_t commonPacket_recv;       // $BDL>o%G!<%?<u?.?t(B
  uint64_t videoPacket_recv;        // $BF02h%G!<%?<u?.?t(B
  uint64_t commonPacketSize_recv;   // $BDL>o%G!<%?<u?.%5%$%:(B
  uint64_t videoPacketSize_recv;    // $BF02h%G!<%?<u?.%5%$%:(B
  uint64_t request_commonPacket_recv;       // $BDL>o%G!<%?<u?.?t(B
  uint64_t request_videoPacket_recv;        // $BF02h%G!<%?<u?.?t(B
  uint64_t request_commonPacketSize_recv;   // $BDL>o%G!<%?<u?.%5%$%:(B
  uint64_t request_videoPacketSize_recv;    // $BF02h%G!<%?<u?.%5%$%:(B
  uint64_t commonPacket_send;       // $BDL>o%G!<%?Aw?.?t(B
  uint64_t videoPacket_send;        // $BF02h%G!<%?Aw?.?t(B
  uint64_t commonPacketSize_send;   // $BDL>o%G!<%?Aw?.%5%$%:(B
  uint64_t videoPacketSize_send;    // $BF02h%G!<%?Aw?.%5%$%:(B

  // CCN DEFAULT
  uint64_t interestPacket_send;
  uint64_t dataPacket_send;
  uint64_t interestPacket_recv;
  uint64_t dataPacket_recv;

  uint64_t fakeInterestPacket_send;
  uint64_t fakeDataPacket_send;
  uint64_t fakeInterestPacket_recv;
  uint64_t fakeDataPacket_recv;

  // function
  void sendCcnMsg(Node* node, CcnMsg* ccnMsg);
  void recvCcnMsg(Node* node, CcnMsg* ccnMsg);
  void recvCcnMsg(Node* node, CcnMsg* ccnMsg, clocktype senToRecv_Delay);
  void recvCcnModMsg(Node* node, CcnMsg* ccnMsg, clocktype senToRecv_Delay);
  void cacheHit(Node* node, CcnMsg* ccnMsg);
  void print(Node* node);

  void print_totalNodeInfo(Node* node);

  StatisticalData() {
    Packet_send           = 0;
    Packet_recv           = 0;
    PacketSize_send       = 0;
    PacketSize_recv       = 0;
    SendToRecv_Delay      = 0;
    GenerateToRecv_Delay  = 0;
    cache_hitTimes        = 0;
    packet_lostTimes      = 0;
    commonPacket_recv     = 0;
    videoPacket_recv      = 0;
    commonPacketSize_recv = 0;
    videoPacketSize_recv  = 0;
    request_commonPacket_recv     = 0;
    request_videoPacket_recv      = 0;
    request_commonPacketSize_recv = 0;
    request_videoPacketSize_recv  = 0;
    commonPacket_send     = 0;
    videoPacket_send      = 0;
    commonPacketSize_send = 0;
    videoPacketSize_send  = 0;

    interestPacket_send   = 0;
    dataPacket_send       = 0;
    interestPacket_recv   = 0;
    dataPacket_recv       = 0;

    fakeInterestPacket_send = 0;
    fakeDataPacket_send     = 0;
    fakeInterestPacket_recv = 0;
    fakeDataPacket_recv     = 0;
  }
};

#endif 
