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

#ifndef MAC_DOT16_H
#define MAC_DOT16_H

//
// This is the main header file of the implementation of
// IEEE 802.16 MAC
//
// MACRO to enable and disable the following feature
#define DOT16_CRC_STATUS                        TRUE
#define DOT16_FRAGMENTATION_STATUS              TRUE
#define DOT16e_BS_IDLE_MODE_STATUS              TRUE
#define DOT16e_BS_SLEEP_MODE_STATUS             TRUE
#define DOT16e_TRAFFIC_INDICATION_PREFERENCE    TRUE
#define DOT16e_SS_PS1_TRAFFIC_TRIGGERED_FLAG    TRUE


#define MacDot16eSetPSClassStatus(psClass, status)\
    (psClass->currentPSClassStatus = status)

//--------------------------------------------------------------------------
// type definitions
//--------------------------------------------------------------------------

// /**
// CONSTANT    :: Dot16CIDType : UInt16
// DESCRIPTION :: Type of the CID in 802.16 MAC. It is a 16bit integer
//                So a MAC domain can only have upto 64K CIDs
// **/
typedef UInt16 Dot16CIDType;

// /**
// CONSTANT    :: DOT16_MAC_ADDRESS_LENGTH : 6
// DESCRIPTION :: In 802.16 MAC, each node has a unique MAC address
//                as 48 bits long
// **/
#define DOT16_MAC_ADDRESS_LENGTH   6

// /**
// CONSTANT    :: DOT16_BASE_STATION_ID_LENGTH : 6
// DESCRIPTION :: Length of the base station ID. Should be 48-bit.
//                The most significant 24 bits shall be used as the operator
//                ID.
// **/
#define DOT16_BASE_STATION_ID_LENGTH   6


///////////////////////////////////////////////////////////////////////////
//CONSTANT FOR INVALID -- start
///////////////////////////////////////////////////////////////////////////
// /**
// CONSTANT    :: DOT16_INVALID_CHANNEL : -1
// DESCRIPTION :: Invalid channel index
// **/
#define DOT16_INVALID_CHANNEL   (-1)

// /**
// CONSTANT    :: DOT16_INVALID_BS_NODE_ID : -1
// DESCRIPTION :: Invalid Bs Node Id
// **/
#define DOT16_INVALID_BS_NODE_ID   (unsigned int)(-1)
///////////////////////////////////////////////////////////////////////////
//CONSTANT FOR INVALID -- end
///////////////////////////////////////////////////////////////////////////

// /**
// CONSTANT    :: DOT16_MAX_MGMT_MSG_SIZE : 3000
// DESCRIPTION :: Max size of a management message
// **/
#define DOT16_MAX_MGMT_MSG_SIZE    3000

// /**
// CONSTANT    :: DOT16_DEFAULT_SERVICE_FLOW_TIMEOUT_INTERVAL : 15S
// DESCRIPTION :: Default value of the interval for timeout a
//                service flow
// **/
#define DOT16_DEFAULT_SERVICE_FLOW_TIMEOUT_INTERVAL   (15 * SECOND)

// /**
// CONSTANT    :: DOT16_MAX_T7_INTERVAL: 1 * SECOND
// DESCRIPTION :: Max value of the timeout interval for waitting the
//                DSx response
// **/
#define DOT16_MAX_T7_INTERVAL    (1 * SECOND)

// /**
// CONSTANT    :: DOT16_DEFAULT_T7_INTERVAL: 1 * SECOND
// DESCRIPTION :: Default value of the timeout interval for waitting the
//                DSx response
// **/
#define DOT16_DEFAULT_T7_INTERVAL    (1 * SECOND)

// /**
// CONSTANT    :: DOT16_MAX_T8_INTERVAL: 300MS
// DESCRIPTION :: Max value of the timeout interval for waitting the
//                DSx ACK
// **/
#define DOT16_MAX_T8_INTERVAL    (300 * MILLI_SECOND)

// /**
// CONSTANT    :: DOT16_DEFAULT_T8_INTERVAL: 200MS
// DESCRIPTION :: Default value of the timeout interval for waitting the
//                DSx Ack
// **/
#define DOT16_DEFAULT_T8_INTERVAL    (200 * MILLI_SECOND)

// /**
// CONSTANT    :: DOT16_MAX_T10_INTERVAL: 3S
// DESCRIPTION :: Max value of the timeout interval for waitting the
//                transaction end
// **/
#define DOT16_MAX_T10_INTERVAL    (3 * SECOND)

// /**
// CONSTANT    :: DOT16_DEFAULT_T10_INTERVAL: 3S
// DESCRIPTION :: Default value of the timeout interval for waitting the
//                transaction end
// **/
#define DOT16_DEFAULT_T10_INTERVAL    (3 * SECOND)


//--------------------------------------------------------------------------
// Ranging, Request, and registration related default values
//--------------------------------------------------------------------------

// /**
// CONSTANT    :: DOT16_RANGE_BACKOFF_START : 0
// DESCRIPTION :: Start value of the ranging backoff
// **/
#define DOT16_RANGE_BACKOFF_START    3

// /**
// CONSTANT    :: DOT16_RANGE_BACKOFF_END : 15
// DESCRIPTION :: End value of the ranging backoff
// **/
#define DOT16_RANGE_BACKOFF_END     15

// /**
// CONSTANT    :: DOT16_REQUEST_BACKOFF_START : 3
// DESCRIPTION :: Start value of the request backoff
// **/
#define DOT16_REQUEST_BACKOFF_START    3

// /**
// CONSTANT    :: DOT16_REQUEST_BACKOFF_END : 15
// DESCRIPTION :: End value of the request backoff
// **/
#define DOT16_REQUEST_BACKOFF_END     15

// /**
// CONSTANT    :: DOT16_MAX_INITIAL_RANGE_RETRIES : 15
// DESCRIPTION :: Maximum number of initial ranging retries
// **/
#define DOT16_MAX_INITIAL_RANGE_RETRIES    16

// /**
// CONSTANT    :: DOT16_MAX_PERIODIC_RANGE_RETRIES : 15
// DESCRIPTION :: Maximum number of periodic ranging retries
// **/
#define DOT16_MAX_PERIODIC_RANGE_RETRIES    16

// /**
// CONSTANT    :: DOT16_MAX_REQUEST_RETRIES : 16
// DESCRIPTION :: Maximum number of bandwidth request retries
// **/
#define DOT16_MAX_REQUEST_RETRIES    16

// /**
// CONSTANT    :: DOT16_MAX_REGISTRATION_RETRIES : 3
// DESCRIPTION :: Maximum number of registration retries
// **/
#define DOT16_MAX_REGISTRATION_RETRIES    3

// /**
// CONSTANT    :: DOT16_MAX_INVITED_RANGE_RETRIES : 16
// DESCRIPTION :: Maximum number of invited ranging retries
// **/
#define DOT16_MAX_INVITED_RANGE_RETRIES    16

// /**
// CONSTANT    :: DOT16_MAX_RANGE_CORRECTION_RETRIES : 16
// DESCRIPTION :: Maximum number of ranging correction retries
// **/
#define DOT16_MAX_RANGE_CORRECTION_RETRIES    16

// /**
// CONSTANT    :: DOT16_MAX_SBC_RETRIES    : 16
// DESCRPTION  :: Max number of retirs of SBC request
// **/
#define DOT16_MAX_SBC_RETRIES  16

// /**
// CONSTANT    :: DOT16_MIN_SBC_RETRIES    : 3
// DESCRPTION  :: Min number of retirs of SBC request
// **/
#define DOT16_MIN_SBC_RETRIES  3

// /**
// CONSTANT    :: DOT16_DEFAULT_SBC_RETRIES    : 16
// DESCRPTION  :: Default number of retirs of SBC request
// **/
#define DOT16_DEFAULT_SBC_RETRIES  3


//--------------------------------------------------------------------------
// DSx related default values
//--------------------------------------------------------------------------

// /**
// CONSTANT    :: DOT16_SS_TRANSACTION_ID_START
// DESCRIPTION :: The starting number of transaction id at SS
// **/
#define  DOT16_SS_TRANSACTION_ID_START 0x0000

// /**
// CONSTANT    :: DOT16_SS_TRANSACTION_ID_END
// DESCRIPTION :: The ending number of transaction id at SS
// **/
#define  DOT16_SS_TRANSACTION_ID_END 0x7FFF

// /**
// CONSTANT    :: DOT16_BS_TRANSACTION_ID_START
// DESCRIPTION :: The starting number of transaction id at SS
// **/
#define  DOT16_BS_TRANSACTION_ID_START 0x8000

// /**
// CONSTANT    :: DOT16_BS_TRANSACTION_END
// DESCRIPTION :: The ending number of transaction id at SS
// **/
#define  DOT16_BS_TRANSACTION_ID_END 0xFFFF

// /**
// CONSTANT    :: DOT16_INVALID_TRANSACTION_ID
// DESCRIPTION :: The invlaid transaction id
// **/
#define  DOT16_INVALID_TRANSACTION_ID (unsigned int)-1

// /**
// CONSTANT    :: DOT16_DEFAULT_DSX_REQ_RETRIES : 3
// DESCRIPTION :: default number of DSx request retries
// **/
#define DOT16_DEFAULT_DSX_REQ_RETRIES    3

// /**
// CONSTANT    :: DOT16_DEFAULT_DSX_RSP_RETRIES : 3
// DESCRIPTION :: default number of DSx response retries
// **/
#define DOT16_DEFAULT_DSX_RSP_RETRIES    3


// /**
// MACRO       :: MacDot16CopyStationId(dst, src)
// DESCRIPTION :: Copy a station ID from src to dst
// **/
#define MacDot16CopyStationId(dst, src) \
            (dst)[0] = (src)[0]; \
            (dst)[1] = (src)[1]; \
            (dst)[2] = (src)[2]; \
            (dst)[3] = (src)[3]; \
            (dst)[4] = (src)[4]; \
            (dst)[5] = (src)[5]

// /**
// MACRO       :: MacDot16SameStationId(sid1, sid2)
// DESCRIPTION :: Determine if the two station IDs are exactly same
// **/
#define MacDot16SameStationId(sid1, sid2) \
            ((sid1)[0] == (sid2)[0] && (sid1)[1] == (sid2)[1] && \
             (sid1)[2] == (sid2)[2] && (sid1)[3] == (sid2)[3] && \
             (sid1)[4] == (sid2)[4] && (sid1)[5] == (sid2)[5])

// /**
// MACRO       :: MacDot16CopyMacAddress(dst, src)
// DESCRIPTION :: Copy a MAC address from src to dst
// **/
#define MacDot16CopyMacAddress(dst, src) \
            (dst)[0] = (src)[0]; \
            (dst)[1] = (src)[1]; \
            (dst)[2] = (src)[2]; \
            (dst)[3] = (src)[3]; \
            (dst)[4] = (src)[4]; \
            (dst)[5] = (src)[5]

// /**
// MACRO       :: MacDot16SameMacAddress(addr1, addr2)
// DESCRIPTION :: Determine if the two MAC addresses are exactly same
// **/
#define MacDot16SameMacAddress(addr1, addr2) \
            ((addr1)[0] == (addr2)[0] && (addr1)[1] == (addr2)[1] && \
             (addr1)[2] == (addr2)[2] && (addr1)[3] == (addr2)[3] && \
             (addr1)[4] == (addr2)[4] && (addr1)[5] == (addr2)[5])

// /**
// MACRO       :: MacDot16Ipv4ToMacAddress(macAddr, ipv4Addr)
// DESCRIPTION :: Convert an IPv4 address to MAC address
// **/
#define MacDot16Ipv4ToMacAddress(macAddr, ipv4Addr) \
            (macAddr)[0] = 0; \
            (macAddr)[1] = 0; \
            (macAddr)[2] = (unsigned char)((ipv4Addr) >> 24); \
            (macAddr)[3] = (unsigned char)((ipv4Addr) >> 16); \
            (macAddr)[4] = (unsigned char)((ipv4Addr) >> 8); \
            (macAddr)[5] = (unsigned char)(ipv4Addr)

// /**
// MACRO       :: MacDot16MacAddressToIpv4(macAddr)
// DESCRIPTION :: Convert an IPv4 address to MAC address
// **/
#define MacDot16MacAddressToIpv4(macAddr) \
            ((((unsigned int)((macAddr)[2])) << 24) | \
            (((unsigned int)((macAddr)[3])) << 16) | \
            (((unsigned int)((macAddr)[4])) << 8) | \
            ((unsigned int)((macAddr)[5])))

// /**
// MACRO       :: MacDot16TwoByteToShort(shortVal, byteVal1, byteVal2)
// DESCRIPTION :: Convert two bytes to a short number
// **/
#define MacDot16TwoByteToShort(shortVal, byteVal1, byteVal2) \
            shortVal = (UInt16)byteVal1 * 256 + (UInt16)byteVal2;

// /**
// MACRO       :: MacDot16TwoByteToShort(byteVal, byteVal2, shortVal)
// DESCRIPTION :: Convert a short number to two bytes
// **/
#define MacDot16ShortToTwoByte(byteVal1, byteVal2, shortVal) \
            byteVal1 = (UInt8)(shortVal / 256); \
            byteVal2 = (UInt8)(shortVal % 256);

// /**
// MACRO       :: MacDot16IntToFourByte(byteStr, intVal)
// DESCRIPTION :: Convert an integer to four byte
// **/
#define MacDot16IntToFourByte(byteStr, intVal) \
            (byteStr)[0] = (unsigned char)((intVal) >> 24); \
            (byteStr)[1] = (unsigned char)((intVal) >> 16); \
            (byteStr)[2] = (unsigned char)((intVal) >> 8); \
            (byteStr)[3] = (unsigned char)(intVal)

// /**
// MACRO       :: MacDot16FourByteToInt(byteStr)
// DESCRIPTION :: Convert four bytes to integer
// **/
#define MacDot16FourByteToInt(byteStr) \
            ((((unsigned int)((byteStr)[0])) << 24) | \
            (((unsigned int)((byteStr)[1])) << 16) | \
            (((unsigned int)((byteStr)[2])) << 8) | \
            ((unsigned int)((byteStr)[3])))
//--------------------------------------------------------------------------
// Various CID related constants
//--------------------------------------------------------------------------

// /**
// CONSTANT    :: DOT16_CID_BLOCK_M : 100
// DESCRIPTION :: Block size m in dividing CID space for different purpose
// **/
#define DOT16_CID_BLOCK_M    200

// /**
// CONSTANT    :: DOT16_INITIAL_RANGING_CID : 0x0000
// DESCRIPTION :: CID of the initial ranging
// **/
#define DOT16_INITIAL_RANGING_CID    0x0000


// /**
// CONSTANT    :: DOT16e_SLEEP_MODE_MULTICAST_CID : 0xFFFB
// DESCRIPTION :: CID of the sleep mode multicast packet
// **/
#define DOT16e_SLEEP_MODE_MULTICAST_CID    0xFFFB


// /**
// CONSTANT    :: DOT16_BASIC_CID_START : 0x0001
// DESCRIPTION :: Start number of the basic CIDs
//                Basic CIDs are from 0x0001 to m
// **/
#define DOT16_BASIC_CID_START    0x0001

// /**
// CONSTANT    :: DOT16_BASIC_CID_END : m
// DESCRIPTION :: End number of the basic CIDs
//                Basic CIDs are from 0x0001 to m
// **/
#define DOT16_BASIC_CID_END    DOT16_CID_BLOCK_M

// /**
// CONSTANT    :: DOT16_PRIMARY_CID_START : m + 1
// DESCRIPTION :: Start number of the primary CIDs
//                Primary CIDs are from (m + 1) to (2 * m)
// **/
#define DOT16_PRIMARY_CID_START  (DOT16_CID_BLOCK_M + 1)

// /**
// CONSTANT    :: DOT16_PRIMARY_CID_END : 2m
// DESCRIPTION :: End number of the primary CIDs
//                Primary CIDs are from (m + 1) to (2 * m)
// **/
#define DOT16_PRIMARY_CID_END  (DOT16_CID_BLOCK_M + DOT16_CID_BLOCK_M)

// /**
// CONSTANT    :: DOT16_TRANSPORT_CID_START : 2m + 1
// DESCRIPTION :: Start number of the transport and secondary mgmt CIDs
//                CIDs are from (2m + 1) to 0xFEFE
// **/
#define DOT16_TRANSPORT_CID_START   \
        (DOT16_CID_BLOCK_M + DOT16_CID_BLOCK_M + 1)

// /**
// CONSTANT    :: DOT16_TRANSPORT_CID_END : 0xFEFE
// DESCRIPTION :: End number of the transport and secondary mgmt CIDs
//                CIDs are from (2m + 1) to 0xFEFE
// **/
#define DOT16_TRANSPORT_CID_END    0xFEFE

// /**
// CONSTANT    :: DOT16_AAS_INITIAL_RANGING_CID : 0xFEFF
// DESCRIPTION :: CID of the AAS initial ranging
// **/
#define DOT16_AAS_INITIAL_RANGING_CID    0xFEFF

// /**
// CONSTANT    :: DOT16_MULTICAST_POLLING_CID_START : 0xFF00
// DESCRIPTION :: Start number of multicast pulling CIDs
// **/
#define DOT16_MULTICAST_POLLING_CID_START    0xFF00


// /**
// CONSTANT    :: DOT16_MULTICAST_POLLING_CID_END : 0xFFFD
// DESCRIPTION :: End number of multicast pulling CIDs
// **/
#define DOT16_MULTICAST_POLLING_CID_END    0xFFFD

// /**
// CONSTANT    :: DOT16_MULTICAST_ALL_SS_CID : 0xFFFD
// DESCRIPTION :: The CID used for broadcasting to all SSs
// **/
#define DOT16_MULTICAST_ALL_SS_CID    0xFFFD

// /**
// CONSTANT    :: DOT16_PADDING_CID : 0xFFFE
// DESCRIPTION :: CID of padding
// **/
#define DOT16_PADDING_CID    0xFFFE

// /**
// CONSTANT    :: DOT16_BROADCAST_CID : 0xFFFF
// DESCRIPTION :: CID of broadcast
// **/
#define DOT16_BROADCAST_CID    0xFFFF

// /**
// CONSTANT    :: DOT16_DEFAULT_MAX_NUM_CID_SUPPORT : 255
// DESCRIPTION :: Max number of CID SS can support, 2 byte
// **/
#define  DOT16_DEFAULT_MAX_NUM_CID_SUPPORT    255

// /**
// CONSTANT    :: DOT16_RANGE_REQUEST_SIZE : 20
// DESCRIPTION :: Size of the ranging request. Used to calculate
//                the reange opp size
// **/
#define DOT16_RANGE_REQUEST_SIZE  50

// /**
// CONSTANT    :: DOT16_NUM_INITIAL_RANGE_OPPS : 3
// DESCRIPTION :: The number of ranging opportunities that the BS
//                will allocate each time by default.
// **/
#define DOT16_NUM_INITIAL_RANGE_OPPS  3

// /**
// CONSTANT    :: DOT16_NUM_BANDWIDTH_REQUEST_OPPS : 3
// DESCRIPTION :: The number of contention based BW request tx opps
// **/
#define DOT16_NUM_BANDWIDTH_REQUEST_OPPS  3

// /**
// CONSTANT    :: DOT16_MAX_NUM_MEASUREMENT    6
// DESCRIPTION :: The max number of measurement used to calculate the mean
// **/
#define DOT16_MAX_NUM_MEASUREMNT   6

// /**
// CONSTANT    :: DOT16_MEASUREMENT_VALID_TIME  (3 * SECOND)
// DESCRIPTION :: The valid time of a measurement
// **/
#define DOT16_MEASUREMENT_VALID_TIME  (3 * SECOND)

// /**
// CONSTANT    :: DOT16_EMERGENCY_MEAS_REPORT_INTERVAL  (500 * MILLI_SECOND)
// DESCRIPTION :: The interval of reprot measurement
//                when find DIUC difference
// **/
#define DOT16_EMERGENCY_MEAS_REPORT_INTERVAL  (500 * MILLI_SECOND)

// /**
// CONSTANT    :: DOT16_MAX_EMERGENCY_MEAS_REPORT  3
// DESCRIPTION :: The MAX consecutive reprot measurement
//                when find DIUC difference
// **/
#define DOT16_MAX_EMERGENCY_MEAS_REPORT 3
//--------------------------------------------------------------------------
// Various DIUC and UIUC related constants
//--------------------------------------------------------------------------

// /**
// CONSTANT    :: DOT16_DIUC_BCAST : 0
// DESCRIPTION :: The DIUC indicasting the DL burst profile for broadcast
//                We assume that the 0th DL burst profile is the most
//                reliable one to be used for broadcast and multicast
//                transmissions.
// **/
#define DOT16_DIUC_BCAST    0

// /**
// CONSTANT    :: DOT16_DIUC_GAP_PAPR : 13
// DESCRIPTION :: The DIUC for Gap/PAPR reduction
// **/
#define DOT16_DIUC_GAP_PAPR    13

// /**
// CONSTANT    :: DOT16_DIUC_END_OF_MAP : 14
// DESCRIPTION :: The DIUC of end of map.
// **/
#define DOT16_DIUC_END_OF_MAP    14

// /**
// CONSTANT    :: DOT16_UIUC_MOST_RELIABLE : 1
// DESCRIPTION :: UIUC of the most reliable burst profile
// **/
#define DOT16_UIUC_MOST_RELIABLE    1

// /**
// CONSTANT    :: DOT16_UIUC_END_OF_MAP : 11
// DESCRIPTION :: UIUC of the End of MAP IE
// **/
#define DOT16_UIUC_END_OF_MAP   11

// /**
// CONSTANT    :: DOT16_UIUC_RANGE : 12
// DESCRIPTION :: UIUC for initial ranging
// **/
#define DOT16_UIUC_RANGE    10

// /**
// CONSTANT    :: DOT16_UIUC_REQUEST : 14
// DESCRIPTION :: UIUC for contention based request
// **/
#define DOT16_UIUC_REQUEST    9

// /**
// CONSTANT    :: DOT16_UIUC_CDMA_RANGE : 12
// DESCRIPTION :: UIUC for CDMA initial ranging
// **/
#define DOT16_UIUC_CDMA_RANGE    12

#define DOT16_CDMA_ALLOCATE_RANGEING_CHANNEL    6

#define DOT16_CDMA_INITIAL_RANGING_OVER_2SYMBOL     0
#define DOT16_CDMA_INITIAL_RANGING_OVER_4SYMBOL     1
#define DOT16_CDMA_HANDOVER_RANGING_OVER_2SYMBOL     0
#define DOT16_CDMA_HANDOVER_RANGING_OVER_4SYMBOL     1
#define DOT16_CDMA_BWREQUEST_OVER_1SYMBOL   2
#define DOT16_CDMA_BWREQUEST_OVER_3SYMBOL   3
#define DOT16_CDMA_PERIODIC_RANGING_OVER_2SYMBOL   2
#define DOT16_CDMA_PERIODIC_RANGING_OVER_3SYMBOL   3

#define DOT16_CDMA_START_INITIAL_RANGING_CODE         0
#define DOT16_CDMA_END_INITIAL_RANGING_CODE          63
#define DOT16_CDMA_START_PERIODIC_RANGING_CODE       64
#define DOT16_CDMA_END_PERIODIC_RANGING_CODE        127
#define DOT16_CDMA_START_BWREQUEST_RANGING_CODE     128
#define DOT16_CDMA_END_BWREQUEST_RANGING_CODE       238
#define DOT16_CDMA_START_HANDOVER_RANGING_CODE      239
#define DOT16_CDMA_END_HANDOVER_RANGING_CODE        255

#define isInitialRangingCode(rangingCode)\
    (rangingCode <= (UInt8)DOT16_CDMA_END_INITIAL_RANGING_CODE)

#define isPeriodicRangingCode(rangingCode)\
    (rangingCode >= (UInt8)DOT16_CDMA_START_PERIODIC_RANGING_CODE\
    && rangingCode <= (UInt8)DOT16_CDMA_END_PERIODIC_RANGING_CODE)

#define isBWReqRangingCode(rangingCode)\
    (rangingCode >= (UInt8)DOT16_CDMA_START_BWREQUEST_RANGING_CODE\
    && rangingCode <= (UInt8)DOT16_CDMA_END_BWREQUEST_RANGING_CODE)

#define isHandoverRangingCode(rangingCode)\
    (rangingCode >= (UInt8)DOT16_CDMA_START_HANDOVER_RANGING_CODE)

// /**
// CONSTANT    :: DOT16_UIUC_CDMA_BWREQUEST : 8
// DESCRIPTION :: UIUC for CDMA BW req.
// **/
//#define DOT16_UIUC_CDMA_BWREQUEST    8

// /**
// CONSTANT    :: DOT16_UIUC_CDMA_ALLOCATION_IE : 14
// DESCRIPTION :: UIUC for CDMA Allocation IE
// **/
#define DOT16_UIUC_CDMA_ALLOCATION_IE    14

// /**
// CONSTANT    :: DOT16_UIUC_PAPR : 13
// DESCRIPTION :: UIUC for PAPR reduction allocation, Safety zone
// **/
#define DOT16_UIUC_PAPR    13

// /**
// CONSTANT    :: DOT16_DIUC_UIUC_EXTENDED : 15
// DESCRIPTION :: Extened DIUC or UIUC
// **/
#define DOT16_DIUC_UIUC_EXTENDED    15

// /**
// CONSTANT    :: DOT16_MAX_NUM_UIUC : 16
// DESCRIPTION :: Max number of UIUC supported
// **/
#define DOT16_MAX_NUM_UIUC   16

// /**
// CONSTANT    :: DOT16_MAX_NUM_DIUC : 16
// DESCRIPTION :: Max number of DIUC supported
// **/
#define DOT16_MAX_NUM_DIUC   16

// /**
// CONSTANT    :: DOT16_NUM_DL_BURST_PROFILES : 8
// DESCRIPTION :: Number of downlink burst profiles.
//                With DIUC from 0 to 8
// **/
#define DOT16_NUM_DL_BURST_PROFILES   8

// /**
// CONSTANT    :: DOT16_NUM_UL_BURST_PROFILES : 8
// DESCRIPTION :: Number of uplink burst profiles.
//                With UIUC from 1 to 8

// **/
#define DOT16_NUM_UL_BURST_PROFILES   8

// /**
// CONSTANT    :: DOT16_DEFAULT_MAX_ALLOWED_UPLINK_LOAD_LEVEL : 1.0
// DESCRIPTION :: Uplimit of the load that BS can handle
               // The range of the value is [0,1]
// **/
#define DOT16_DEFAULT_MAX_ALLOWED_UPLINK_LOAD_LEVEL 0.7

// CONSTANT    :: DOT16_DEFAULT_MAX_ALLOWED_DOWNLINK_LOAD_LEVEL : 1.0
// DESCRIPTION :: Downlink load that BS can handle
               // The range of the value is [0,1]
// **/
#define DOT16_DEFAULT_MAX_ALLOWED_DOWNLINK_LOAD_LEVEL 0.7



// /**
// CONSTANT    :: DOT16_VARIABLE_LENGTH_SDU : 0
// DESCRIPTION :: Variable length packing indication
// **/
#define DOT16_VARIABLE_LENGTH_SDU       0

// /**
// CONSTANT    :: DOT16_FIXED_LENGTH_SDU : 1
// DESCRIPTION :: Fixed length packing indication
// **/
#define DOT16_FIXED_LENGTH_SDU          1

// /**
// CONSTANT    :: DOT16_DEFAULT_FIXED_LENGTH_SDU_SIZE : 49
// DESCRIPTION :: Default fixed lengthSDU size
// **/
#define DOT16_DEFAULT_FIXED_LENGTH_SDU_SIZE     49

#define DOT16_CRC_SIZE                          4

#define DOT16_MAX_PDU_SIZE                      2047

#define DOT16_FRAGMENTED_MAX_FIXED_LENGTH_PACKET_SIZE   128
//ARQ related default parameters
////////////////////////////////////////////////////////////////////////////
#define DOT16_ARQ_DEFAULT_WINDOW_SIZE 256

#define DOT16_ARQ_DEFAULT_RETRY_TIMEOUT 4

#define DOT16_ARQ_DEFAULT_RETRY_COUNT 2

#define DOT16_ARQ_DEFAULT_SYNC_LOSS_INTERVAL 32

#define DOT16_ARQ_DEFAULT_RX_PURGE_TIMEOUT 28

#define DOT16_ARQ_DEFAULT_BLOCK_SIZE 64// in bytes

#define DOT16_ARQ_DEFAULT_DELIVER_IN_ORDER 1

// CONSTANT    :: DOT16_SS_DEFAULT_T22_INTERVAL :
//                                  (2 * DOT16_BS_DEFAULT_FRAME_DURATION)
// DESCRIPTION :: Min valuse of time the SS ARQ reset(2 MAC frame)
// /**
#define DOT16_ARQ_DEFAULT_T22_INTERVAL    \
        (2 * DOT16_ARQ_RETRY_TIMEOUT)

#define DOT16_ARQ_MAX_RESET_RETRY   6
////////////////////////////////////////////////////////////////////////////
//ARQ related constant parameters
///////////////////////////////////////////////////////////////////////////
//Max Values.
#define DOT16_ARQ_BSN_MODULUS 2048

#define DOT16_ARQ_MAX_RETRY_TIMEOUT_TRANSMITTER_DELAY (655350 * MICRO_SECOND)

#define DOT16_ARQ_MAX_RETRY_TIMEOUT_RECEIVER_DELAY (655350 * MICRO_SECOND)

#define DOT16_ARQ_MAX_RETRY_TIMEOUT_DELAY\
    (DOT16_ARQ_MAX_RETRY_TIMEOUT_TRANSMITTER_DELAY + \
        DOT16_ARQ_MAX_RETRY_TIMEOUT_RECEIVER_DELAY)

#define DOT16_ARQ_MAX_BLOCK_LIFE_TIME (655350 * MICRO_SECOND)
#define DOT16_ARQ_MAX_SYNC_LOSS_TIMEOUT (655350 * MICRO_SECOND)
#define DOT16_ARQ_MAX_RX_PURGE_TIMEOUT (655350 * MICRO_SECOND)
#define DOT16_ARQ_MAX_BLOCK_SIZE 2040 //in bytes
#define DOT16_ARQ_MAX_BANDWIDTH \
    ((DOT16_ARQ_WINDOW_SIZE * DOT16_ARQ_BLOCK_SIZE)+ \
    sizeof(MacDot16MacHeader) + \
    sizeof(MacDot16ExtendedorARQEnableFragSubHeader)+\
    sizeof(MacDot16DataGrantSubHeader))

//Variable Vlaues
#define DOT16_ARQ_RX_WINDOW_START (sFlow->arqControlBlock->arqRxWindowStart)
#define DOT16_ARQ_RX_HIGHEST_BSN (sFlow->arqControlBlock->arqRxHighestBsn)

#define DOT16_ARQ_BLOCK_LIFETIME (sFlow->arqParam->arqBlockLifeTime)

#define DOT16_ARQ_RETRY_TIMEOUT_RX_DELAY \
                            (sFlow->arqParam->arqRetryTimeoutRxDelay)
#define DOT16_ARQ_RETRY_TIMEOUT_TX_DELAY \
                            (sFlow->arqParam->arqRetryTimeoutTxDelay)
#define DOT16_ARQ_RETRY_TIMEOUT (DOT16_ARQ_RETRY_TIMEOUT_RX_DELAY + \
                                    DOT16_ARQ_RETRY_TIMEOUT_TX_DELAY)
#define DOT16_ARQ_SYNC_LOSS_TIMEOUT (sFlow->arqParam->arqSyncLossTimeout)
#define DOT16_ARQ_RX_PURGE_TIMEOUT  (sFlow->arqParam->arqRxPurgeTimeout)


#define MacDot16ARQNormalizedBsn(bsnId,windowStart)\
                        ((bsnId - windowStart + DOT16_ARQ_BSN_MODULUS)%\
                                             DOT16_ARQ_BSN_MODULUS)

#define MacDot16ARQFeedbackIEGetCID(header) \
            (((Dot16CIDType)(header)->byte1) * 256 + \
             ((Dot16CIDType)(header)->byte2))

#define MacDot16ARQFeedbackIESetCID(header, val) \
            ((header)->byte1 = (unsigned char)((val) / 256)); \
            ((header)->byte2 = (unsigned char)((val) % 256))

#define MacDot16ARQFeedbackIEGetLast(header) \
            (unsigned char)((header)->byte3 >> 7)

#define MacDot16ARQFeedbackIESetLast(header, val) \
            ((header)->byte3 |= (((unsigned char)(val)) << 7))

#define MacDot16ARQFeedbackIEGetAckType(header) \
            (unsigned char)(((header)->byte3 & 0x60) >> 5)

#define MacDot16ARQFeedbackIESetAckType(header, val) \
            ((header)->byte3 |= (((unsigned char)(val)) << 5))


#define MacDot16ARQFeedbackIEGetBSN(subheader) \
            ( ( UInt16)(((arqFeedBack)->byte3 & 0x1C) >> 2) * 256)\
                       + (\
                          ((UInt16)((((arqFeedBack)->byte3 & 0x03) << 6))) |\
                             ((UInt16)( (arqFeedBack)->byte4 >> 2))\
                        )

#define MacDot16ARQFeedbackIESetBSN(subheader, val) \
            (subheader)->byte3 |= ((unsigned char)(val / 256) << 2);\
            (subheader)->byte3 |= ((unsigned char)(val % 256) >> 6);\
            (subheader)->byte4 |= ((unsigned char)(val % 256) << 2)

#define MacDot16ARQFeedbackIEGetNumAckMaps(subheader) \
            (unsigned char)((subheader)->byte4 & 0x03)

#define MacDot16ARQFeedbackIESetNumAckMaps(subheader,val) \
     (subheader)->byte4 |= ((unsigned char)val & 0x03);


#define MacDot16ARQDiscardMsgSetCID(header, val) \
            MacDot16ARQFeedbackIESetCID(header,val)

#define MacDot16ARQDiscardMsgSetBSN(subheader, val) \
            (subheader)->byte3 |= ((unsigned char)((val / 256)& 0x07));\
            (subheader)->byte4 |= ((unsigned char)(val % 256))

#define MacDot16ARQDiscardMsgGetBSN(subheader) \
                (UInt16)((((subheader)->byte3 & 0x07) * 256)+ \
                                ((subheader)->byte4 ))


#define MacDot16ARQDiscardMsgGetCID(header)\
    MacDot16ARQFeedbackIEGetCID(header)




/*#define MacDot16MacHeaderGetEC(header) \
            (((header)->byte1 & 0x40) >> 6)*/

typedef enum
{
    DOT16_ARQ_INVALID_STATE = 0,
    DOT16_ARQ_BLOCK_NOT_SENT = 1,
    DOT16_ARQ_BLOCK_OUTSTANDING = 2,
    DOT16_ARQ_BLOCK_DONE = 3,
    DOT16_ARQ_BLOCK_DISCARDED = 4,
    DOT16_ARQ_BLOCK_WAIT_FOR_RETRANSMISSION = 5,
    DOT16_ARQ_BLOCK_RECEIVED = 6,
    DOT16_ARQ_BLOCK_ACKED = 7
} MacDot16ARQBlockState;

typedef enum
{
    DOT16_ARQ_TX = 0,//The sender or the transmitter
    DOT16_ARQ_RX = 1,//The receiver.
}MacDot16ARQDirectionType;

// /**
// ENUM        :: MacDot16ARQResetStat
// DESCRIPTION :: ARQ reset stat
// **/
typedef enum
{
    // reserved  // 0 - 2
    DOT16_ARQ_RESET_Initiator = 0x00,
    DOT16_ARQ_RESET_ACK_RSP = 0x01,
    DOT16_ARQ_RESET_Confirm_Initiator = 0x02,
    DOT16_ARQ_RESET_Reserved = 0x03,
}MacDot16ARQResetStat;

typedef enum
{
    DOT16_ARQ_SELECTIVE_ACK_TYPE = 0x0,
    DOT16_ARQ_CUMULATIVE_ACK_TYPE = 0x1,
    DOT16_ARQ_CUMULATIVE_SELCTIVE_ACK_TYPE = 0x2,
    DOT16_ARQ_CUMULATIVE_BLOCK_SEQ_ACK_TYPE = 0x1
}MacDot16ARQAckType ;


//--------------------------------------------------------------------------
// Type enum definitions
//--------------------------------------------------------------------------

typedef enum
{
    DOT16_CDMA_INITIAL_RANGING_CODE = 0,
    DOT16_CDMA_PERIODIC_RANGING_CODE,
    DOT16_CDMA_BWREQ_CODE,
    DOT16_CDMA_HANDOVER_CODE
} MacDot16CdmaRangingCodeType;

// /**
// ENUM        :: MacDot16StationType
// DESCRIPTION :: Type of a station
// **/
typedef enum
{
    DOT16_BS,      // Base station (BS)
    DOT16_SS       // Subscriber station (SS)
} MacDot16StationType;

// /**
// ENUM        :: MacDot16Mode
// DESCRIPTION :: Operation mode
// **/
typedef enum
{
    DOT16_PMP,      // Point to Multi-Point mode
    DOT16_MESH      // Mesh mode
} MacDot16Mode;

// /**
// ENUM        :: MacDot16DuplexMode
// DESCRIPTION :: Duplex mode
// **/
typedef enum
{
    DOT16_TDD,      // Time Division Duplex (TDD), uplink and downlink
                    //   share the same frequency.
    DOT16_FDD_HALF, // Half-duplex Frequency Division Duplex (FDD).
                    //   Uplink and downlink use different frequencies.
                    //   But cannot tx and rx at the same time.
    DOT16_FDD_FULL  // Full-duplex Frequency Division Duplex (FDD)
                    //   Uplink and downlink use different frequencies,
                    //   and can tx and rx at the same time.
} MacDot16DuplexMode;

// /**
// ENUM        :: MacDot16SubframeType
// DESCRIPTION :: Downlink or uplink part of the frame
// **/
typedef enum
{
    DOT16_DL, // Downlink subframe
    DOT16_UL  // Uplink subframe
} MacDot16SubframeType;

// /**
// CONSTANT    :: DOT16_MAX_PHY_TYPE : 4
// DESCRIPTION :: Max number of PHY TYPE supported
// **/
#define DOT16_MAX_PHY_TYPE   4

// /**
// ENUM        :: MacDot16PhyType
// DESCRIPTION :: Type of the PHY model
// **/
typedef enum
{
    DOT16_PHY_SC = 0,    // SC PHY
    DOT16_PHY_SCA = 1,   // SCa PHY
    DOT16_PHY_OFDM = 2,  // OFDM PHY
    DOT16_PHY_OFDMA =3,  // OFDMA PHY
} MacDot16PhyType;

// /**
// ENUM        :: MacDot16CidType
// DESCRIPTION :: Type of dot16 CIDs
// **/
typedef enum
{
    DOT16_CID_BROADCAST,  // broadcast mgmt CID
    DOT16_CID_BASIC,      // basic mgmt CID
    DOT16_CID_PRIMARY,    // primary mgmt CID
    DOT16_CID_SECONDARY,  // secondary mgmt CID
    DOT16_CID_TRANSPORT   // transport cid
} MacDot16CidType;

// /**
// ENUM        :: MacDot16MacVer
// DESCRPTION  :: MAC version
typedef enum
{
    DOT16_MAC_VER_2001 = 1,    // IEEE 802.16-2001
    DOT16_MAC_VER_2002 = 2,    // IEEE 802.16c-2002
    DOT16_MAC_VER_2003 = 3,    // IEEE 802.16a-2003
    DOT16_MAC_VER_2004 = 4,    // IEEE 802.16-2004
    //5 -255 reserved
}MacDot16MacVer;

// /**
// CONSTANT    :: DOT16_NUM_SERVICE_TYPE
// DESCRIPTION :: Max number of service type supported
//                ATTENTION: This constant should at least not less than
//                           the # in MacDot16ServiceType
#define DOT16_NUM_SERVICE_TYPES    5

// /**
// ENUM        :: MacDot16ServiceType
// DESCRIPTION :: Data service types
// **/
typedef enum
{
    DOT16_SERVICE_UGS = 0,     // Unsolicited Grant Service
    DOT16_SERVICE_ertPS = 1,   // extened Real-time Polling Service
    DOT16_SERVICE_rtPS = 2,    // Real-time Polling service
    DOT16_SERVICE_nrtPS = 3,   // Non-real-time Polling Service
    DOT16_SERVICE_BE = 4,       // Best Effort service
//    DOT16_SERVICE_INVALID = 0xFFFF
} MacDot16ServiceType;

// /**
// CONSTANT    :: DOT16_ertPS_BW_REQ_ADJUST_THRESHOLD
// DESCRIPTION :: THresold that SS will adjust the
//                bandwidht request for a ertPS serviceflow (in Byte)
#define DOT16_ertPS_BW_REQ_ADJUST_THRESHOLD    1024
// **/

// /**
// ENUM        :: MacDot16FlowStatus
// DESCRIPTION :: Status of a service flow
// REFERENCE   :: IEEE 802.16 2004, Figure 99. Page 230
// **/
typedef enum
{
    DOT16_FLOW_AddingLocal, // initiated local adding operation
    DOT16_FLOW_AddingRemote, // received remotely initiated adding operation
    DOT16_FLOW_AddFailed, // adding operation failed
    DOT16_FLOW_ChangingLocal, // initiated local changing operation
    DOT16_FLOW_ChangingRemote, // received remotely initiated changing
                               // operation
    DOT16_FLOW_Deleting,
    DOT16_FLOW_Deleted,
    DOT16_FLOW_Nominal, // after successfule transaction
    DOT16_FLOW_NULL, //
} MacDot16FlowStatus;

// /**
// ENUM        :: MacDot16TimerType
// DESCRIPTION :: Type of various system timers
// **/
typedef enum
{
    // generial timers
    DOT16_TIMER_OperationStart, // start of the operation

    // BS related timers
    DOT16_TIMER_FrameBegin,    // begin of the frame

    // SS related timers
    DOT16_TIMER_FrameUlBegin,  // Beginning of the UL part
    DOT16_TIMER_FrameEnd,      // End of current frame
    DOT16_TIMER_UlBurstStart,  // start point of an UL burst
    DOT16_TIMER_Ranging,       // send ranging request
    DOT16_TIMER_BandwidthRequest, // send bw request

    DOT16_TIMER_T1,   // Timeout timer for waiting DCD
    DOT16_TIMER_T2,   // Timeout timer for braodcast ranging timeout
    DOT16_TIMER_T3,   // Timeout timer for waiting RNG-RSP
    DOT16_TIMER_T4,   // Timeout timer for waitng unicat ranging opportunity
    DOT16_TIMER_T6,   // Timeout timer for waiting for registration response
    DOT16_TIMER_T7,   // Timeout timer for waiting DSx response
    DOT16_TIMER_T8,   // Timeout timer for waiting for DSx acknowledge
    DOT16_TIMER_T9,   // Timeout timer for waiting SBC-REQ
    DOT16_TIMER_T10,  // Timeout timer for waiting for transaction end
    DOT16_TIMER_T12,  // Timeout timer for UCD descriptor
    DOT16_TIMER_T14,  // timeout timer for DSx=RVD message
    DOT16_TIMER_T16,  // Timeout timer for waiting BW grant after request
    DOT16_TIMER_T17,  // Timeout timer for allowing SS to complete SS
                      // authorization and key exchange
    DOT16_TIMER_T18,  // Timeout timer for waiting SBC-RSP
    DOT16_TIMER_T19,  // Timeout timer for dl-channel remain unusable
    DOT16_TIMER_T20,  // Timeout timer for SS searching for preambles on a
                      // given channel
    DOT16_TIMER_T21,  // Timeout timer of SS searches for DL-MAP on a given
                      // channel
    DOT16_TIMER_T22,  // Timer for ARQ reset
    DOT16_TIMER_T27,  // Timeout for unicast grant to be issued
    DOT16_TIMER_LostDlSync,  // Detection of lost of downlink sync.
    DOT16_TIMER_LostUlSync,   //detection of lost of uplink sync
    DOT16_TIMER_RgnRspProc,

    //UCD/DCD transition timer
    DOT16_TIMER_UcdTransition, // Timeout timer before BS issuing a new
                               // genration UL map
    DOT16_TIMER_DcdTransition, // Timeout timer before BS issuing a new
                               // genration DL map

    // Timer messages for Convergence Sublayer (CS)
    // Note: In order to correct dispatch the messages, timer messages used
    // by CS must be within DOT16_TIMER_CS_Begin and DOT16_TIMER_CS_End
    DOT16_TIMER_CS_Begin,
    DOT16_TIMER_CS_ClassifierTimeout,
    //DOT16_TIMER_CS_InitIdle,
    DOT16_TIMER_CS_End,
    // End of CS related timer message types

    // Timer msg for dot16e (start)
    DOT16e_TIMER_T44, // Timeout timer for SS waiting for MOB_SCN_RSP
    DOT16e_TIMER_T41,
    DOT16e_TIMER_T42,
    DOT16e_TIMER_T29,
    DOT16e_TIMER_ResrcRetain,
    DOT16_ARQ_RETRY_TIMER,
    DOT16_ARQ_BLOCK_LIFETIME_TIMER,
    DOT16_ARQ_RX_PURGE_TIMER,
    DOT16_ARQ_SYNC_LOSS_TIMER,
    DOT16_ARQ_DISCARD_RETRY_TIMER,
    DOT16e_TIMER_T45,
    DOT16e_TIMER_T46,
    DOT16e_TIMER_MgmtRsrcHldg,
    //DOT16e_TIMER_IdleMode,
    DOT16e_TIMER_IdleModeSystem,
    DOT16e_TIMER_MSPaging,
    DOT16e_TIMER_MSPagingUnavlbl,
    DOT16e_TIMER_LocUpd,
    DOT16e_TIMER_DregReqDuration,
    // Timer msg for dot16e (end)
    DOT16e_TIMER_T43,
    DOT16e_TIMER_SlpPowerUp,
    DOT16_TIMER_End   // useless, just indicate the biggest timer value
} MacDot16TimerType;

// /**
// ENUM        :: MacDot16RngReqPurpose
// DESCRIPTION :: purpose of the rangeing request
// **/
typedef enum
{
    DOT16_RNG_REQ_InitJoinOrPeriodicRng = 0,    // initial join the network
    DOT16_RNG_REQ_HO = 1,                       // range request for ho
    DOT16_RNG_REQ_LocUpd = 2,                  // Location update (idle)
    DOt16_RNG_REQ_Association = 4,              // range for association
}MacDot16RngReqPurpose;
// /**
// ENUM        :: MacDot16RngReqLocUpdRsp
// DESCRIPTION :: Location Update Response
// **/
typedef enum
{
    DOT16_RNG_REQ_Failure = 0,
    DOT16_RNG_REQ_Success = 1
}MacDot16RngReqLocUpdRsp;

//./**
// ENUM        :: MacDot16SignalType
// DESCRIPTION :: type of the signal
// **/

enum
{
    NORMAL = 0,               // normal type
    CDMA = 1                      // CDMA based
};

// /**
// ENUM        :: MacDot16RangeType
// DESCRIPTION :: type of the ranging
// **/

typedef enum
{
    DOT16_NORMAL = 0,               // normal type
    DOT16_CDMA                      // CDMA based ranging
}MacDot16RangeType;

// /**
// ENUM        :: MacDot16BWReqType
// DESCRIPTION :: type of the ranging
// **/

typedef enum
{
    DOT16_BWReq_NORMAL = 0,               // normal type
    DOT16_BWReq_CDMA = 1,                  // CDMA based BE request
}MacDot16BWReqType;

// /**
// STRUCT      :: MacDot16Timer
// DESCRIPTION :: Data structure for a timer info field
// **/
typedef struct
{
    MacDot16TimerType timerType;  // type of the timer
    unsigned int info;        // 32bits space to hold some additional info
    unsigned int Info2;       // 32bits space to hold some more info,
                              // basically it is used to distinguish
                              // the dsx transaction tye, for T7/8/10/14
} MacDot16Timer;


//--------------------------------------------------------------------------
// Management message structures, constants, enums, etc.
//--------------------------------------------------------------------------

// /**
// ENUM        :: MacDot16MgmtMsgType
// DESCRIPTION :: Type of various MAC management messages
// **/
typedef enum
{
    DOT16_UCD = 0,       // Unlink Channel Descriptor, Broadcast
    DOT16_DCD = 1,       // Downlink Channel Descriptor, Broadcast
    DOT16_DL_MAP = 2,    // Downlink Access Definition, Broadcast
    DOT16_UL_MAP = 3,    // Uplink Access Definition, Broadcast

    DOT16_RNG_REQ = 4,   // Ranging Request, Initial Ranging or Basic
    DOT16_RNG_RSP = 5,   // Ranging Response, Initial Ranging or Basic

    DOT16_REG_REQ = 6,   // Registration Requeset, Primary Management
    DOT16_REG_RSP = 7,   // Registration Response, Primary Management

    DOT16_PKM_REQ = 9,   // Privacy Key Management Request, Primary Mgmt
    DOT16_PKM_RSP = 10,  // Privacy Key Management Response, Primary Mgmt

    DOT16_DSA_REQ = 11,  // Dynamic Service Addition Request, Primary Mgmt
    DOT16_DSA_RSP = 12,  // Dynamic Service Addition Response, Primary Mgmt
    DOT16_DSA_ACK = 13,  // Dynamic Service Addition Ack, Primary Mgmt
    DOT16_DSC_REQ = 14,  // Dynamic Service Change Request, Primary Mgmt
    DOT16_DSC_RSP = 15,  // Dynamic Service Change Response, Primary Mgmt
    DOT16_DSC_ACK = 16,  // Dynamic Service Change Ack, Primary Mgmt
    DOT16_DSD_REQ = 17,  // Dynamic Service Deletion Request, Primary Mgmt
    DOT16_DSD_RSP = 18,  // Dynamic Service Deletion Response, Primary Mgmt

    DOT16_MCA_REQ = 21,  // Multicast Assignment Request, Primary Mgmt
    DOT16_MCA_RSP = 22,  // Multicast Assignment Response, Primary Mgmt

    DOT16_DBPC_REQ = 23, // Downlink Burst Profile Change Request, Basic
    DOT16_DBPC_RSP = 24, // Downlink Burst Profile Change Response, Basic

    DOT16_RES_CMD = 25,  // Reset Command, Basic

    DOT16_SBC_REQ = 26,  // SS Basic Capability Request, Basic
    DOT16_SBC_RSP = 27,  // SS Basic Capability Response, Basic

    DOT16_CLK_CMP = 28,  // SS network clock comparison, Broadcast

    DOT16_DREG_CMD = 29, // De/Re-register Command, Basic

    DOT16_DSX_RVD = 30,  // DSx Received Message, Primary Mgmt

    DOT16_TFTP_CPLT = 31,// Config File TFTP Complete Message, Primary Mgmt
    DOT16_TFTP_RSP = 32, // Config File TFTP Complete Response, Primary Mgmt

    DOT16_ARQ_FEEDBACK = 33, // Standalone ARQ Feedback, Basic
    DOT16_ARQ_DISCARD = 34, // ARQ Discard message, Basic
    DOT16_ARQ_RESET = 35, // ARQ Reset message, Basic

    DOT16_REP_REQ = 36,  // Channel measurement Report Request, Basic
    DOT16_REP_RSP = 37,  // Channel measurement Report Response, Basic

    DOT16_FPC     = 38,  // Fast Power Control, Broadcast

    DOT16_MSH_NCFG = 39, // Mesh Network Configuration, Broadcast
    DOT16_MSH_NENT = 40, // Mesh Network Entry, Basic
    DOT16_MSH_DSCH = 41, // Mesh Distributed Schedule, Broadcast
    DOT16_MSH_CSCH = 42, // Mesh Centralized Schedule, Broadcast
    DOT16_MSH_CSCF = 43, // Mesh Centralized Schedule Configuration, Bcast

    DOT16_AAS_FBCK_REQ = 44, // AAS Feedback Request, Basic
    DOT16_AAS_FBCK_RSP = 45, // AAS Feedback Response, Basic
    DOT16_AAS_BEAM_SELECT = 46, // AAS Beam Select message, Basic
    DOT16_AAS_BEAM_REQ = 47, // AAS Beam Request message, Basic
    DOT16_AAS_BEAM_RSP = 48, // AAS Beam Response message, Basic

    DOT16_DREG_REQ = 49,  // SS De-registration message, Basic

    DOT16e_MOB_SLP_REQ  = 50,
    DOT16e_MOB_SLP_RSP  = 51,
    DOT16e_MOB_TRF_IND  = 52,
    // mgmt msg types for 802.16e
    DOT16e_MOB_NBR_ADV = 53, // Neighbor advertisement, broadcast
    DOT16e_MOB_SCN_REQ = 54, // Scanning Interval Allocation Request, Basic
    DOT16e_MOB_SCN_RSP = 55, // Scanning Interval Allocation Response, Basic
    DOT16e_MOB_SCN_REP = 60, // scanning result report, primary
    DOT16e_MOB_BSHO_REQ = 56, // BS initiated handover request, basic
    DOT16e_MOB_MSHO_REQ = 57, // MS initiated handover request, basic
    DOT16e_MOB_BSHO_RSP = 58, // BS reply to MOB_MSHO-REQ, basic
    DOT16e_MOB_HO_IND = 59,   // Final indication of HO by MS, basic
    DOT16e_MOB_PAG_ADV = 62,  // Paging advertisement, broadcast
    // end of mgmt msg types for 802.16e

    //msg for OFDMA ranging code,
    //it is canned for implementation CDMA-based communication
    //DOT16_OFDMA_RNG_BW_REQ_CODE,
    //end of msg for OFDMA ranging code

    DOT16_PADDED_DATA = 255, // padded data send in allocated Tx opp if no pdu
                       // to send

    DOT16_MAX_MGMT_MSG_TYPE  // Must be the last one
} MacDot16MgmtMsgType;


//
// Begin of TLV types
//

// TLV type value for common TLV encoding
#define TLV_COMMON_HMacTuple      149
#define TLV_COMMON_MacVersion     148
#define TLV_COMMON_CurrTransPow   147
#define TLV_COMMON_DlServiceFlow  146
#define TLV_COMMON_UlServiceFlow  145
#define TLV_COMMON_VendorId       144
#define TLV_COMMON_VendorSpecInfo 143

// /**
// ENUM        :: MacDot16UcdTlvType
// DESCRIPTION :: Type of various UCD channel encoding TLVs
// **/
typedef enum
{
    // common channel encoding
    TLV_UCD_UlBurstProfile = 1,      // variable size
    TLV_UCD_ContentionRsvTimout = 2, // 1 byte, # of UL-MAPs before timeout
    TLV_UCD_BwReqOppSize = 3,        // 2 byte, size(PS) of BW req opp size
    TLV_UCD_RngReqOppSize = 4,       // 2 byte, size(PS) of rng req opp size
    TLV_UCD_Frequency = 5,           // 4 byte, UL center frequency (kHz)

    // OFDMA PHY specific
    TLV_UCD_OfdmaInitialRngCodes = 150,  // 1 byte, # of init rng CDMA codes
    TLV_UCD_OfdmaPeriodicRngCodes = 151, // 1 byte, # of peri rng CDMA codes
    TLV_UCD_OfdmaBwReqCode = 152,               // 1 byte, # of bw req codes
    TLV_UCD_OfdmaPeriodicRngBackoffStart = 153, // 1 byte
    TLV_UCD_OfdmaPeriodicRngbackoffEnd = 154,   // 1 byte
    TLV_UCD_OfdmaStartRngCodeGroup = 155,       // 1 byte
    TLV_UCD_OfdmaPemutationBase = 156,          // 1 byte
    TLV_UCD_OfdmaUlAllocSubchBitmap = 157,      // 9 bytes
    TLV_UCD_OfdmaPermUlAllocSubchBitmap = 158,  // 13 byte
    TLV_UCD_OfdmaBandAmcAllocThreshold = 159,   // 1 byte
    TLV_UCD_OfdmaBandAmcReleaseThreshold = 160, // 1 byte
    TLV_UCD_OfdmaBandAmcAllocTimer = 161,       // 1 byte
    TLV_UCD_OfdmaBandAmcReleaseTimer = 162,     // 1 byte
    TLV_UCD_OfdmaBandStatusRepMaxPeriod = 163,  // 1 byte
    TLV_UCD_OfdmaBandAmcRetryTimer = 164,       // 1 byte
    TLV_UCD_OfdmaSafetyChAllocThreshold = 165,  // 1 byte
    TLV_UCD_OfdmaSafetyChReleaseThreshold = 166,// 1 byte
    TLV_UCD_OfdmaSafetyChAllocTimer= 167,       // 1 byte
    TLV_UCD_OfdmaSafetyChReleaseTimer = 168,    // 1 byte
    TLV_UCD_OfdmaBinStatusReportMacPeriod = 169,// 1 byte
    TLV_UCD_OfdmaSafetyChRetryTimer = 170,      // 1 byte
    TLV_UCD_OfdmaHarqAckDelayUlBurst = 171,     // 1 byte
    TLV_UCD_OfdmaCqichBandAmcTransDelay = 172,  // 1 byte
    TLV_UCD_SSTG = 173,                         // 1 byte, canned for the
                                                // purpose of implementation

} MacDot16UcdTlvType;

// /**
// ENUM        :: MacDot16UcdBurstProfileTlvType
// DESCRIPTION :: Type of various UCD message Burse profile TLVs
// **/
typedef enum
{
    // PHY OFDMA
    TLV_UCD_PROFILE_OfdmaFecCodeModuType = 150, // 1 byte
    TLV_UCD_PROFILE_OfdmaRngDataRatio = 151,    // 1 byte
    TLV_UCD_PROFILE_OfdmaCnOverride = 152,      // 1 byte
} MacDot16UcdBurstProfileTlvType;

// /**
// ENUM        :: MacDot16DcdTlvType
// DESCRIPTION :: Type of various UCD message channel codeing TLVs
// **/
typedef enum
{
    TLV_DCD_DlBurstProfile = 1,     // variable size, all
    TLV_DCD_BsEirp = 2,             // 2 bytes, all
    TLV_DCD_FrameDuration = 3,      // 4 bytes, SC
    TLV_DCD_PhyType = 4,            // 1 byte , SC
    TLV_DCD_PowerAdjustRule = 5,    // 1 byte,  SC, SCa
    TLV_DCD_ChNumber = 6,           // 1 byte,  SCa, OFDM, OFDMA
    TLV_DCD_TTG = 7,                // 1 byte,  SCa, OFDM, OFDMA
    TLV_DCD_RTG = 8,                // 1 byte,  SCa, OFDM, OFDMA
    TLV_DCD_RrsInitRngMax = 9,      // 2 bytes, All
    TLV_DCD_ChSwitchFrameNum = 10,  // 3 bytes, SCa, OFDM, OFDMA
    TLV_DCD_Freqency = 12,          // 4 bytes, All
    TLV_DCD_BsId = 13,              // 6 bytes, SCa, OFDM,OFDMA
    TLV_DCD_FrameDurationCOde = 14, // 1 byte,  OFDM
    TLV_DCD_FrameNumber = 15,       // 3 bytes, OFDM
    TLV_DCD_CqichIdSize = 16,       // 1 byte,  OFDMA
    TLV_DCD_HarqAckDelay = 17,      // 1 byte,  OFDMA

    // extend for dot16e
    TLV_DCD_PagingGroupId = 35, //2 bytes, paging group id
    TLV_DCD_PagingIntervalLength = 45,  // 1 byte, paging
    TLV_DCD_Trigger = 54
} MacDot16DcdTlvType;

// /**
// ENUM        :: MacDot16eDcdTriggerTlvType
// DESCRIPTION :: TLV types for triggers in DCD msg
// **/
typedef enum
{
    TLV_DCD_TriggerTypeFuncAction = 1, // trigger type/func/action
    TLV_DCD_TriggerValue = 2,          // trigger value
    TLV_DCD_TriggerAvgDuration = 3,    // trigger average duration
}MacDot16eDCDTriggerTlvType;

// /**
// ENUM        :: MacDot16DcdBurstProfileTlvType
// DESCRIPTION :: Type of various UCD message burst profile TLVs
// **/
typedef enum
{
    // PHY-common
    TLV_DCD_PROFILE_Frequency = 1, // 4 bytes

    // OFDMA PHY Specific
    TLV_DCD_PROFILE_OfdmaFecCodeModuType = 150,// 1 byte
    TLV_DCD_PROFILE_OfdmaExitThreshold = 151,  // 1 byte
    TLV_DCD_PROFILE_OfdmaEntryThreshold = 152, // 1 byte
} MacDot16DcdBurstProfileTlvType;

// /**
// ENUM        :: MacDot16RngReqTlvType
// DESCRIPTION :: Type of various RNG-REQ TLVs
// **/
typedef enum
{
    TLV_RNG_REQ_DlBurst = 1,     // Requested Downlink Burst Profile, 1 byte
    TLV_RNG_REQ_MacAddr = 2,     // SS MAC Address, 6 bytes
    TLV_RNG_REQ_RngAnomal = 3,   // Ranging Anomalies, 1 byte
    TLV_RNG_REQ_AasBcastCap = 4, // AAS broadcast capability, 1 byte

    // extend for dot16e
    TLV_RNG_REQ_ServBsId = 5,    // the BS Id of serving BS, 1 byte
    TLV_RNG_REQ_RngPurpose = 6,  // the ranging purpose
    TLV_RNG_REQ_HoInd = 7,       // id from target BSfor initial rng, 1 byte
    TLV_RNG_REQ_PagingCtrlId = 9,     // Paging controller Id
    TLV_RNG_REQ_MacHashSkipThshld = 10, //Mac Hash Skip Threshold
    TLV_RNG_REQ_CDMACode = 149, // CDMA code

    // SCa-specific
    TLV_RNG_REQ_AasFeedback = 150 // SCa AAS feedback, 6 bytes
} MacDot16RngReqTlvType;

// /**
// ENUM        :: MacDot16RngRspTlvType
// DESCRIPTION :: Type of various RNG-RSP TLVs
// **/
typedef enum
{
    TLV_RNG_RSP_TimingAdjust = 1,      // Timing Adjust, 4 bytes
    TLV_RNG_RSP_PowerAdjust = 2,       // Power Level Adjust, 1 byte
    TLV_RNG_RSP_FreqAdjust = 3,        // Offset Frequency Adjust, 4 bytes
    TLV_RNG_RSP_RngStatus = 4,         // Ranging Status, 1 byte
    TLV_RNG_RSP_DlFreqOver = 5,        // DL Frequency Override, 4 bytes
    TLV_RNG_RSP_UlChIdOver = 6,        // UL Channel ID Override, 1 byte
    TLV_RNG_RSP_DlOpBurst = 7,         // DL Operational Burst Prof, 2 bytes
    TLV_RNG_RSP_MacAddr = 8,           // SS MAC Address, 6 bytes
    TLV_RNG_RSP_BasicCid = 9,          // Basic CID assigned by BS, 2 bytes
    TLV_RNG_RSP_PrimaryCid = 10,       // Primary Mgmt CID by BS, 2 bytes
    TLV_RNG_RSP_AasBcastPerm = 11,     // AAS broadcast permission, 1 byte
    TLV_RNG_RSP_FrameNumber = 12,      // Frame number of RNG-REQ, 3 bytes
    TLV_RNG_RSP_InitRngOppNumber = 13, // Initial ranging opp number, 1 byte

    // extend for dot16e
    TLV_RNG_RSP_ServLevPredict = 17,   // Level of service MS can expected
    TLV_RNG_RSP_GlobalServClsName = 18, // 4 byte
    // QoS Parameter 145 146
    // TLV_RNG_RSP_SfId
    TLV_RNG_RSP_RsrcRetainFlag = 20,   // old serv BS retain the connection?
    TLV_RNG_RSP_HoProcOptimize = 21,   // HO process optimization support?
    //paging/idle
    TLV_RNG_RSP_LocUpdRsp = 23,     // 0x00=Failure, 0x01=Success
    TLV_RNG_RSP_PagingInfo = 24,    // Paging cycle, offset and group id
    TLV_RNG_RSP_PagingCtrlId = 25,  // Paging controller Id
    TLV_RNG_RSP_MacHashSkipThshld = 28,

    // OFDMA specific
    TLV_RNG_RSP_OfdmaRngCodeAttr = 149, // Ranging code attributes, 4 bytes

} MacDot16RngRspTlvType;

// /**
// ENUM        :: MacDot16SbcReqRspTlvType
// DESCRIPTION :: Type of various SBC-REQ/RSP TLVs
// **/
typedef enum
{
    TLV_SBC_REQ_RSP_BwAllocSupport = 1, // BW allocation support, 1 byte
    TLV_SBC_REQ_RSP_TranstionGaps = 2,  // SS transition gaps, 2 bytes
    TLV_SBC_REQ_MaxTransmitPower = 3,   // max transmit power, 4 bytes
                                        // only for SBC-REQ
    TLV_SBC_REQ_RSP_ConstructTransMacPdu = 4, // capa. for constrcution &
                                              // trans of MAC PDUs, 1 byte
    //current tranmist power  = 147,          // defined as common TLV.
                                              // 1 byte, only for SBC-REQ

    TLV_SBC_REQ_RSP_PowerSaveClassTypesCapability = 26,

    // OFDMA specific param
    TLV_SBC_REQ_RSP_OfdmaFFTSize = 150,    // OFDM SS FFT size, 1 byte
    TLV_SBC_REQ_RSP_OfdmaDemoduType = 151, // OFDM demodulation type, 1 byte
    TLV_SBC_REQ_RSP_OfdmaModuType = 152,   // OFDM modulation type, 1 byte
    TLV_SBC_REQ_RSP_OfdmaNumHARQAckCh = 153, // OFDMA modulator,
                                             // the number of HARQ ACK
                                             // channel, 1 byte

    TLV_SBC_REQ_RSP_OfdmaPermutationSupport = 154 // OFDMA SS permutaiton
                                                  // support
}MacDot16SbcReqRspTlvType;

// /**
// ENUM
// DESCRIPTION :: MacDot16RegReqRspTlvType
// **/
typedef enum
{
    TLV_REG_REQ_RSP_ArqParam = 1,    // ARQ Parameters , 1 Byte
    TLV_REG_REQ_RSP_MgmtSupport = 2, // SS management support,
                                     // ---0, no secondary mgmt support,
                                     // ---1, secondary mgmt support, 1 byte

    TLV_REG_REQ_RSP_IpMgmtMode = 3,  // IP management mode, 1 byte
    TLV_REG_REQ_RSP_IpVersion = 4,   // IP version, bit0, v4, bit1, v6,
                                     // bit 2-7, resevered, 1 byte

    TLV_REG_REQ_RSP_SecondaryCid = 5,    // secondary management CID, 2 byte
    TLV_REG_REQ_RSP_NumUlCidSupport = 6, // # of uplink CID support, 2 byte
                                         // min 3 for managed SS,
                                         // min 2 for unmanaged SS,
                                         // plus 0 or more transport CID

    // CS sublayer capabilities
    TLV_REG_REQ_RSP_CS_ClassificationPhsSduEncapsulationSupport = 7,
    // 2 byte, refer to pg. 668
    TLV_REG_REQ_RSP_CS_MaxClassfier = 8, // max # of classfiers, 2 byte
    TLV_REG_REQ_RSP_CS_PhsSupport = 9,   // PHS support, 2 byte

    // SS capabilites encoding
    TLV_REG_REQ_RSP_ArqAupport = 10,     // ARQ support, 1 byte
    TLV_REG_REQ_RSP_DSxFlowCotrol = 11,  // DSx flow control, 1 byte
                                         // 0-- no limit,
                                         // 1 - 255, max concurrent trans.

    TLV_REG_REQ_RSP_MacCrcSupport = 12,  // MAC CRC support, 1 byte
                                         // 0, no MAC CRC support,
                                         // 1, MAC CRC support

    TLV_REG_REQ_RSP_MacFlowControl = 13,        // max # of concurrent MAC
                                                // trans, 1 byte
    TLV_REG_REQ_RSP_McstPollGrpCidSupport = 14, // Max # of mcst polling
                                                // group CID support, 1 byte
    TLV_REG_REQ_RSP_PkmFlowControl = 15,        // PKM flow control, 1 byte
    TLV_REG_REQ_RSP_AuthPolicySupport = 16,     // Authorization policy
                                                // support, bit 0,
                                                // IEEE 80216 privacy
                                                // supported, 1 byte

    TLV_REG_REQ_RSP_MaxNumSupportSecurityAssociation,
    // Max number of supported security association, 1 byte

    // DOT16e supported parameter
    TLV_REG_REQ_RSP_MobilitySupportedParameters = 31
}MacDot16RegReqRspTlvType;

typedef enum
{
    MOBILITY_SUPPORTED = 0x01,
    SLEEP_MODE_SUPPORTED = 0x02,
    IDLE_MODE_SUPPORTED = 0x04
}MacDot16MobilitySupportedParameters;

// /**
// ENUM        :: MacDot16RepReqTlvType
// DESCRIPTION :: Type of TLVs used in REP-REQ msg
// **/
typedef enum
{
    TLV_REP_REQ_ReportType = 1,   // report type, 1 byte
                                  // bit #0 = 1, DSF basic report
                                  // bit #1 = 1, CINR report
                                  // bit #2 = 1, RSSI report
                                  // bit #3-6, alpha in unit of 1/32
                                  // bit #7 = 1, current transmission power
    TLV_REP_REQ_ChannelNum = 2,   // physical channel number, 1 byte
    TLV_REP_REQ_ChTypeReq = 3,    // channel type request, 1 byte
                                  // 00, normal subchannel
                                  // 01, band AMC channel
                                  // 10, safety channel
                                  // 11, sounding
} MacDot16RepReqTlvType;

// /**
// ENUM        :: MacDot16RepRspTlvType
// DESCRIPTION :: Type of TLVs used in REP-RSP msg
// **/
typedef enum
{
    TLV_REP_RSP_ChaneelNum = 1,   // physica channel number, 1 byte
    TLV_REP_RSP_StartFrame = 2,   // frame number in which meas. started
    TLV_REP_RSP_Duration = 3,     // cumulative meas. duration in unit of Ts
    TLV_REP_RSP_BasicReport = 4,  //
    TLV_REP_RSP_CinrReport = 5,   // 1 byte, mean; 1 byte, standard
                                  // deviation
    TLV_REP_RSP_RssiReport = 6,   // 1 byte, mean; 1 byte, standard
                                  // deviation
    TLV_REP_RSP_NoramlSubChRepor,
    TLV_REP_RSP_BandAmcReport,
    TLV_REP_RSP_SafetyChReport,
    TLV_REP_RSP_SoundReport,

} MacDot16RepRspTlvType;


// /**
// ENUM        :: MacDot16DsxTlvType
// DESCRIPTION :: Type of various DSx TLVs
// **/
typedef enum
{
    TLV_DSX_ServiceFlowId = 1,    // Service Flow Identifier, 4 bytes
    TLV_DSX_Cid = 2,              // CID assigned to the flow, 2 bytes
    TLV_DSX_ServiceClassName = 3, // Service Class Name, 2 to 128 Bytes
    // TLV_DSX_Reserved = 4,      // reserved
    TLV_DSX_QoSParamSetType = 5,  // QoS parameters set type, 1 byte
    TLV_DSX_TrafficPrioriy = 6,   // Traffic priority, 1 byte,
                                  // higher number indicates higher
                                  // priority, default 0

    TLV_DSX_MaxSustainedRate = 7, // Maximum Sustained Traffic Rate,
                                  // 4 byte, unit: bps
    TLV_DSX_MaxTrafficBurst = 8,  // Maximum Traffic burst, 4 bytes
    TLV_DSX_MinReservedRate = 9,  // Minimal Reserved Traffic Rate,
                                  // 4 bytes, unit: bps

    TLV_DSX_MinTolerableRate = 10, // Minimum tolerable traffci rate,
                                   // 4 bytes, unit: bps
    TLV_DSX_ServiceType = 11,      // Service Flow Scheduling Type, 1 byte
    TLV_DSX_ReqTransPolicy = 12,   // Request/Transmission Policy, 1 bytes
    TLV_DSX_ToleratedJitter = 13,  // Tolerated Jitter, 4 bytes, unit: ms
    TLV_DSX_MaxLatency = 14,       // Maximum Latency, 4 bytes, unit: ms
    TLV_DSX_FixLenVarLanSDU = 15,  // Fixed length versus variable-length
                                   // SDU Indicator, 1 byte,
                                   // 0 = variant len, 1 = fixed len

    TLV_DSX_SduSize = 16,          // SDU Size, 1 byte
    TLV_DSX_TargetSaid = 17,       // Target SAID, 2 byte
    // 18 - 26 ARQ related
    TLV_DSX_ARQ_Enable = 18,             // ARQ enable, 1 byte
    TLV_DSX_ARQ_WindowSize = 19,         // ARQ window size, 2 bytes
    TLV_DSX_ARQ_RetryTimeoutTxDelay = 20,// ARQ retry time out Tx. delay, 2 bytes
    TLV_DSX_ARQ_RetryTimeoutRxDelay = 21,// ARQ retry time out Rx delay, 2 bytes
    TLV_DSX_ARQ_BlockLifetime = 22,      // ARQ block life time, 2 bytes
    TLV_DSX_ARQ_SyncLossTimeout = 23,    // ARQ sync. loss, 2 bytes
    TLV_DSX_ARQ_DeliverInOrder = 24,     // ARQ deliver in order, 1 byte
    TLV_DSX_ARQ_RxPurgeTimeout = 25,     // ARQ purge time out, 2 bytes
    TLV_DSX_ARQ_Blocksize = 26,          // ARQ block size, 2 bytes



    // 27 reserved
    TLV_DSX_CSSpec = 28,           // CS specification, 1 byte, refer to
                                   // pg.706 for detail

    TLV_DSX_PagingPref = 32,       // Paging preference
    TLV_DSX_TrafficIndicationPreference = 34, //sflow should generate
                            // MOB_TRF-IND messages to an MS in sleep mode.

    // TLV_COMMON_VendorSpecInfo = 143
    // 99 - 107 Convergence Sublayer Types

    TLV_DSX_CS_TYPE_ATM = 99,      // ATM
    TLV_DSX_CS_TYPE_IPv4 = 100,    // Pakcet, IPv4
    TLV_DSX_CS_TYPE_IPv6 = 101,    // Packet, IPv6
    // more types

} MacDot16DsxTlvType;
// /**
// ENUM        :: MacDot16PacketCsTlvType
// DESCRIPTION :: Type of various packet CS TLVs
// **/
typedef enum
{
    TLV_CS_PACKET_ClassifierDscAction = 1, // classfier DSC action, 1 byte,
                                           // 0 = add classifier,
                                           // 1 = replace classifier,
                                           // 2 = delete classifier

    TLV_CS_PACKET_ClassifierErrorParamSet = 2, // variable length,
                                               // calssifier error parameter
    TLV_CS_PACKET_ClassifierRule = 3,          // set packet classifier
                                               // rule, variable langth
}MacDot16PacketCsTlvType;

// /**
// ENUM        :: MacDot16PacketCsErrorParamTlvSubType
// DESCRIPTION :: Subtype of various packet CS error parameter TLVs
// **/
typedef enum
{
    TLV_CS_PACKET_ErrorParam = 1, // n bytes, errored parameter
    TLV_CS_PACKET_ErrorCode = 2,  // 1 byte, error code
    TLV_CS_PACKET_ErrorMsg = 3,   // n bytes, error message
}MacDot16PacketCsErrorParamTlvSubType;

// /**
// ENUM        :: MacDot16PacketCsClassifierRuleTlvSubType
// DESCRIPTION :: Subtype of various packet CS classfier rule TLVs
// **/
typedef enum
{
    TLV_CS_PACKET_ClassifierRulePriority = 1, // classifier rule priority,
                                              // 1 bytes
    TLV_CS_PACKET_TosDscpRangeMask = 2,       // IP type of service/
                                              // DiffServ codepoint(DSCP),
                                              // 3 bytes
                                              // range and mask, tos-low,
                                              // tos-high, tos-mask

    TLV_CS_PACKET_Protocol = 3,               // n bytes, protocol
    TLV_CS_PACKET_IpSrcAddr = 4,              // n*8 ipv4, n*32 ipv6, src1,
                                              // mask1....srcn, maskn
    TLV_CS_PACKET_IpDestAddr = 5,             // n*8 ipv4, n*32 ipv6, dest1,
                                              // mask1....destn, maskn
    TLV_CS_PACKET_SrcPortRange = 6,           // n*4, protocol source port
                                              // range; instead 2 byte as
                                              // short type is used
    TLV_CS_PACKET_DestPortRange = 7,          // n*4, protocol destination
                                              // port range, instead 2 as
                                              // short bytes is used

    TLV_CS_PACKET_Ethernet_DestinationMacAddress = 8, // n*12
    TLV_CS_PACKET_Ethernet_SourceMacAddress = 9, // n*12

    // more
    TLV_CS_PACKET_ClassifierRuleIndex = 14,   // packet classifier rule
                                              // index, 2 bytes
    // more
}MacDot16PacketCsClassifierRuleTlvSubType;


// /**
// STRUCT      :: MacDot16HmacTuple
// DESCRIPTION :: HMAC (hash message authentication ocde) Key seq number
//                and HMAC-digest for message authentication
typedef struct
{
    unsigned char hmacKeySeqNum;
    unsigned char hmacDigest[20];
}MacDot16HmacTuple;

// /**
// ENUM        :: MacDot16RegResponse
// DESCRIPTION :: Type of response from BS carried in REG-RSP
// **/
typedef enum
{
    DOT16_REG_RSP_OK = 0,
    DOT16_REG_RSP_FAILURE = 1,
}MacDot16RegResponse;

// /**
// ENUM        :: MacDot16PKMCode
// DESCRIPTION :: PKM message ode
// **/
typedef enum
{
    // reserved  // 0 - 2
    PKM_CODE_SAAdd = 3,
    PKM_CODE_AuthRequest = 4,
    PKM_CODE_AuthReply = 5,
    PKM_CODE_AuthReject = 6,
    PKM_CODE_KeyRequest = 7,
    PKM_CODE_KeyReply = 8,
    PKM_CODE_KeyReject = 9,
    PKM_CODE_AuthInvalid = 10,
    PKM_CODE_TEKInvalid = 11,
    PKM_CODE_AuthInfo = 12,
    // reserved 13 - 255
}MacDot16PKMCode;

// /**
// ENUM        :: MacDot16FlowTransactionStatus
// DESCRIPTION :: Status of a service flow transacction
// **/
typedef enum
{
    DOT16_FLOW_DSX_NULL,
    // Locally initated DSA transaction
    DOT16_FLOW_DSA_LOCAL_Begin,
    DOT16_FLOW_DSA_LOCAL_DsaRspPending,
    DOT16_FLOW_DSA_LOCAL_RetryExhausted,
    DOT16_FLOW_DSA_LOCAL_HoldingDown,
    DOT16_FLOW_DSA_LOCAL_DeleteFlow,
    DOT16_FLOW_DSA_LOCAL_End,

    // Remotely initiated DSA transaction
    DOT16_FLOW_DSA_REMOTE_Begin,
    DOT16_FLOW_DSA_REMOTE_DsaAckPending,
    DOT16_FLOW_DSA_REMOTE_HoldingDown,
    DOT16_FLOW_DSA_REMOTE_DeleteFlow,
    DOT16_FLOW_DSA_REMOTE_End,

    // Locally initiated DSC transaction
    DOT16_FLOW_DSC_LOCAL_Begin,
    DOT16_FLOW_DSC_LOCAL_DscRspPending,
    DOT16_FLOW_DSC_LOCAL_RetryExhausted,
    DOT16_FLOW_DSC_LOCAL_HoldingDown,
    DOT16_FLOW_DSC_LOCAL_DeleteFlow,
    DOT16_FLOW_DSC_LOCAL_End,

    // Remotely initiated DSC transaction
    DOT16_FLOW_DSC_REMOTE_Begin,
    DOT16_FLOW_DSC_REMOTE_DscAckPending,
    DOT16_FLOW_DSC_REMOTE_HoldingDown,
    DOT16_FLOW_DSC_REMOTE_DeleteFlow,
    DOT16_FLOW_DSC_REMOTE_End,

    // Locally initiated DSD transaction
    DOT16_FLOW_DSD_LOCAL_Begin,
    DOT16_FLOW_DSD_LOCAL_DsdRspPending,
    DOT16_FLOW_DSD_LOCAL_HoldingDown,
    DOT16_FLOW_DSD_LOCAL_End,

    // Remotely initiated DSD transaction
    DOT16_FLOW_DSD_REMOTE_Begin,
    DOT16_FLOW_DSD_REMOTE_HoldingDown,
    DOT16_FLOW_DSD_REMOTE_End,
} MacDot16FlowTransactionStatus;

// /**
// ENUM        :: MacDot16FlowTransConfirmCode
// DESCRIPTION :: Confirm code of a service flow transacction
// **/
typedef enum
{
    DOT16_FLOW_DSX_CC_OKSUCC = 0,      // OK/success
    DOT16_FLOW_DSX_CC_RejectOther = 1, // rejet other
    DOT16_FLOW_DSX_CC_RejectUnrecognConfigSetting = 2,
    // reject-unrecognized-configuration-setting
    DOT16_FLOW_DSX_CC_RejectTempRejectResource = 3,
    // reject-temporary/reject-resource
    DOT16_FLOW_DSX_CC_RejejctPermRejectAdmin = 4,
    // reject-permanant/reject-admin
    DOT16_FLOW_DSX_CC_RejectNotOwner = 5,              // reject-not-owner
    DOT16_FLOW_DSX_CC_RejectServiceFlowNotFound = 6,
    // Reject-service-flow-not-found
    DOT16_FLOW_DSX_CC_RejectServiceFlowExist = 7,
    // Reject-service-flow-exists
    DOT16_FLOW_DSX_CC_RejectRequiredParamNotPresent = 8,
    // Reject-required-parameter-not-present
    DOT16_FLOW_DSX_CC_RejectHeadSuppression = 9,
    // Reject-header-suppression
    DOT16_FLOW_DSX_CC_RejectUnknownTransId = 10,
    // Reject-unknown-transaction-id
    DOT16_FLOW_DSX_CC_RejectAuthFailure = 11,
    // Reject-authentication-failure
    DOT16_FLOW_DSX_CC_RejectAddAbort = 12,             // Reject-add-abort
    DOT16_FLOW_DSX_CC_RejectExceedDynamicServiceLimit = 13,
    // Reject-exceeded-dynamic-service-limit
    DOT16_FLOW_DSX_CC_RejectNotAuthForReqSAID = 14,
    // Reject-not-authorized-for-the-request-SAID
    DOT16_FLOW_DSX_CC_RejectFailedEstReqSA = 15,
    // Reject-fail-to-establish-the-request-SA
    DOT16_FLOW_DSX_CC_RejectNotSupportedParam = 16,
    // Reject-not-supported-parameter
    DOT16_FLOW_DSX_CC_RejectNotSupportedParamValue = 17,
    // Reject-not-supported-parameter-value
}MacDot16FlowTransConfirmCode;
// /**
// STRUCT      :: MacDot16MacHeader
// DESCRIPTION :: MAC header of the 802.16 MAC. It could be the generic
//                  MAC header or the bandwidth request header
//                We define them as bytes in order to prevent alignment
//                  of Windows platform and different fields for generic
//                  MAC header and bandwidth request header.
// **/
typedef struct
{
    unsigned char byte1;  // first byte, HT, EC, Type (and 3 bits of BR MSB)

    unsigned char byte2;  // second byte, Rsv, CI, EKS, Rsv, LEN MSB of
                          // generic MAC header, or 8 bits of BR MSB of
                          // bandwidth request header

    unsigned char byte3;  // third byte, LEN LSB of generic MAC header or
                          // BR LSB of bandwidth request header

    unsigned char byte4;  // fourth byte, CID MSB field

    unsigned char byte5;  // fifth byte, CID LSB field

    unsigned char byte6;  // sixth byte, HCS field
} MacDot16MacHeader;

// A few macros are defined to get/set different fields of the header.
// For set operation, we assume that the bit fields are originally 0
//   in order to avoid very complex macros.
#define MacDot16MacHeaderGetHT(header) \
            ((header)->byte1 >> 7)
#define MacDot16MacHeaderSetHT(header, val) \
            ((header)->byte1 |= (((unsigned char)(val)) << 7))
#define MacDot16MacHeaderGetEC(header) \
            (((header)->byte1 & 0x40) >> 6)
#define MacDot16MacHeaderSetEC(header, val) \
            ((header)->byte1 |= ((((unsigned char)(val)) << 6) & 0x40))
#define MacDot16MacHeaderGetGeneralType(header) \
            ((header)->byte1 & 0x3F)
#define MacDot16MacHeaderSetGeneralType(header, val) \
            (header)->byte1 |= (((unsigned char)(val)) & 0x3F)
#define MacDot16MacHeaderGetBandwidthType(header) \
            (((header)->byte1 & 0x38) >> 3)
#define MacDot16MacHeaderSetBandwidthType(header, val) \
            ((header)->byte1 |= ((((unsigned char)(val)) << 3) & 0x38))
#define MacDot16MacHeaderGetCI(header) \
            (((header)->byte2 & 0x40) >> 6)
#define MacDot16MacHeaderSetCI(header, val) \
            ((header)->byte2 |= ((((unsigned char)(val)) << 6) & 0x40))
#define MacDot16MacHeaderGetEKS(header) \
            (((header)->byte2 & 0x30) >> 4)
#define MacDot16MacHeaderSetEKS(header, val) \
            (header)->byte2 |= ((((unsigned char)(val)) << 4) & 0x30)
#define MacDot16MacHeaderGetLEN(header) \
            (((unsigned int)((header)->byte2 & 0x07)) * 256 + \
            ((unsigned int)((header)->byte3)))
#define MacDot16MacHeaderSetLEN(header, val) \
            ((header)->byte2 &= 0xF8);\
            ((header)->byte2 |= (((unsigned char)((val) / 256)) & 0x07)); \
            ((header)->byte3 = (unsigned char)((val) % 256))

#define MacDot16MacHeaderGetBR(header) \
            (((unsigned int)((header)->byte1 & 0x07)) * 65536 + \
            ((unsigned int)((header)->byte2)) * 256 + \
             ((unsigned int)((header)->byte3)))
#define MacDot16MacHeaderSetBR(header, val) \
            ((header)->byte1 |= (((unsigned char)((val) / 65536)) & 0x07)); \
            ((header)->byte2 = (unsigned char)(((val) / 256) % 256)); \
            ((header)->byte3 = (unsigned char)((val) % 256))
#define MacDot16MacHeaderGetCID(header) \
            (((Dot16CIDType)(header)->byte4) * 256 + \
             ((Dot16CIDType)(header)->byte5))
#define MacDot16MacHeaderSetCID(header, val) \
            ((header)->byte4 = (unsigned char)((val) / 256)); \
            ((header)->byte5 = (unsigned char)((val) % 256))
#define MacDot16MacHeaderGetHCS(header) \
            ((header)->byte6)
#define MacDot16MacHeaderSetHCS(header, val) \
            ((header)->byte6 = (unsigned char)(val))

#define DOT16_BANDWIDTH_AGG  0x001
#define DOT16_BANDWIDTH_INC  0x000

#define DOT16_MESH_SUBHEADER    0x20
#define DOT16_ARQ_FEEDBACK_PAYLOAD    0x10
#define DOT16_EXTENDED_TYPE    0x08
#define DOT16_FRAGMENTATION_SUBHEADER    0x04
#define DOT16_PACKING_SUBHEADER    0x02
#define DOT16_FAST_FEEDBACK_ALLOCATION_SUBHEADER    0x01
// /**
// STRUCT      :: MacDot16DataGrantSubHeader
// DESCRIPTION :: Data grant subheader of the 802.16 MAC.
// **/
typedef struct
{
    unsigned char byte1;  // first byte,
    unsigned char byte2;  // second byte
} MacDot16DataGrantSubHeader;
#define MacDot16DataGrantSubHeaderSetSI(subheader) \
            ((subheader)->byte1 |= 0x80)
#define MacDot16DataGrantSubHeaderGetSI(subheader) \
            ((subheader)->byte1 & 0x80 >> 7)
#define MacDot16DataGrantSubHeaderSetPM(subheader) \
            ((subheader)->byte1 |= 0x40)
#define MacDot16DataGrantSubHeaderGetPM(subheader) \
            ((subheader)->byte1 & 0x40 >> 6)
#define MacDot16DataGrantSubHeaderSetPBR(subheader, val) \
            ((subheader)->byte1 = (unsigned char)((val) / 256)); \
            ((subheader)->byte2 = (unsigned char)((val) % 256))
#define MacDot16DataGrantSubHeaderGetPBR(subheader) \
            (((unsigned int)(subheader)->byte1) * 256 + \
             ((unsigned int)(subheader)->byte2))


#define DOT16_NO_FRAGMENTATION      0x00
#define DOT16_LAST_FRAGMENT         0x40
#define DOT16_FIRST_FRAGMENT        0x80
#define DOT16_MIDDLE_FRAGMENT       0xC0

// if this parameter value is increased more then 8 then
// support the extended sub header during fragmentation
#define DOT16_MAX_NO_FRAGMENTED_PACKET  8

// /**
// STRUCT      :: MacDot16NotExtendedARQDisableFragSubHeader
// DESCRIPTION :: Fragmentation subheader of the 802.16 MAC.
//                2bit FC + 3bit FSN + 3bit RFU
// **/
typedef struct
{
    unsigned char byte1;  // first byte,
} MacDot16NotExtendedARQDisableFragSubHeader;

// /**
// STRUCT      :: MacDot16ExtendedorARQEnableFragSubHeader
// DESCRIPTION :: Fragmentation subheader of the 802.16 MAC.
//                2bit FC + 11bit FSN or BSN + 3bit RFU
// **/
typedef struct
{
    unsigned char byte1;  // first byte,
    unsigned char byte2;  // second byte
} MacDot16ExtendedorARQEnableFragSubHeader;


#define MacDot16Set11bit(subheader, val) \
            (subheader)->byte1 |= ((unsigned char)(val / 256) << 3);\
            (subheader)->byte1 |= ((unsigned char)(val % 256) >> 5);\
            (subheader)->byte2 |= ((unsigned char)(val % 256) << 3)

#define MacDot16Get11bit(subheader) \
            ((UInt16)(((subheader)->byte1 & 0x38) >> 3) * 256)\
            + (UInt8)(((subheader)->byte1 << 5) |\
                ((subheader)->byte2 >> 3))

#define MacDot16FragSubHeaderSetFC(subheader,val) \
            ((subheader)->byte1 = ((subheader)->byte1 & 0x3f ) | val)
#define MacDot16FragSubHeaderSet3bitFSN(subheader, val) \
            ((subheader)->byte1 |= (val << 3))
#define MacDot16FragSubHeaderSet11bitFSN(subheader, val) \
            MacDot16Set11bit(subheader, val)
#define MacDot16FragSubHeaderSet11bitBSN(subheader, val) \
            MacDot16Set11bit(subheader, val)

#define MacDot16FragSubHeaderGetFC(subheader) \
            ((subheader)->byte1 & 0xC0)
#define MacDot16FragSubHeaderGet3bitFSN(subheader) \
            ((subheader->byte1 & 0x38) >> 3)
#define MacDot16FragSubHeaderGet11bitFSN(subheader) \
            MacDot16Get11bit(subheader)
#define MacDot16FragSubHeaderGet11bitBSN(subheader) \
            MacDot16Get11bit(subheader)


// /**
// STRUCT      :: MacDot16NotExtendedARQDisablePackSubHeader
// DESCRIPTION :: Fragmentation subheader of the 802.16 MAC.
//                2bit FC + 3bit FSN + 11bit Length
// **/
typedef struct
{
    unsigned char byte1;  // first byte,
    unsigned char byte2;  // second byte
} MacDot16NotExtendedARQDisablePackSubHeader;

// /**
// STRUCT      :: MacDot16ExtendedorARQEnablePackSubHeader
// DESCRIPTION :: Fragmentation subheader of the 802.16 MAC.
//                2bit FC + 11bit FSN or BSN + 11bit Length
// **/
typedef struct
{
    unsigned char byte1;  // first byte,
    unsigned char byte2;  // second byte,
    unsigned char byte3;  // third byte
} MacDot16ExtendedorARQEnablePackSubHeader;

#define MacDot16PackSubHeaderSetFC(subheader, val) \
            (subheader->byte1 |= val)
#define MacDot16PackSubHeaderSet3bitFSN(subheader, val) \
            (subheader->byte1 |= (val << 3))
#define MacDot16PackSubHeaderSet11bitFSN(subheader, val) \
            MacDot16Set11bit(subheader, val)
#define MacDot16PackSubHeaderSet11bitBSN(subheader, val) \
            MacDot16Set11bit(subheader, val)
#define MacDot16NotExtendedARQDisablePackSubHeaderSet11bitLength(subheader, val) \
            (subheader->byte1 |= ((unsigned char)(val / 256)));\
            (subheader->byte2 |= ((unsigned char)(val % 256)));
#define MacDot16ExtendedARQEnablePackSubHeaderSet11bitLength(subheader, val) \
            (subheader->byte2 |= ((unsigned char)(val / 256)));\
            (subheader->byte3 |= ((unsigned char)(val % 256)));

#define MacDot16PackSubHeaderGetFC(subheader) \
            (subheader->byte1 & 0xC0)
#define MacDot16PackSubHeaderGet3bitFSN(subheader) \
            ((subheader->byte1 & 0x38) >> 3)
#define MacDot16PackSubHeaderGet11bitFSN(subheader) \
            MacDot16Get11bit(subheader)
#define MacDot16PackSubHeaderGet11bitBSN(subheader) \
            MacDot16Get11bit(subheader)

#define MacDot16NotExtendedARQDisablePackSubHeaderGet11bitLength(subheader) \
            ((UInt16)((subheader->byte1 & 0x07) * 256) + subheader->byte2)
#define MacDot16ExtendedARQEnablePackSubHeaderGet11bitLength(subheader) \
            ((UInt16)((subheader->byte2 & 0x07) * 256) + subheader->byte3)


typedef struct
{
    clocktype       arqRetryTimeoutTxDelay;
    clocktype       arqRetryTimeoutRxDelay;
    clocktype       arqBlockLifeTime;
    clocktype       arqSyncLossTimeout;
    clocktype       arqRxPurgeTimeout;
    unsigned int    arqBlockSize;
    unsigned int    arqWindowSize;
    Int8            arqDeliverInOrder;
} MacDot16ARQParam;

typedef struct
{
    MacDot16ExtendedorARQEnableFragSubHeader arqFragSubHeader;
    MacDot16ARQBlockState arqBlockState ;
    Message* arqRxPurgeTimer ;
    Message* blockPtr ;
}MacDot16ARQBlock ;

typedef struct
{
    Int16 arqTxWindowStart;
    Int16 arqTxNextBsn;
    Int16 arqRxWindowStart;
    Int16 arqRxHighestBsn;
    MacDot16ARQDirectionType direction;
    Int16 front;
    Int16 rear;
    BOOL isARQBlockTransmisionEnable;
    int numOfARQResetRetry;
    UInt8 waitForARQResetType;
    MacDot16ARQBlock* arqBlockArray;
}MacDot16ARQControlBlock;

typedef struct
{
    unsigned char type;
    unsigned char byte1;  // first byte
    unsigned char byte2;  // second byte,
    unsigned char byte3;  // third byte
    unsigned char byte4;  // fourth byte
}MacDot16ARQFeedbackMsg;

typedef struct
{
    unsigned char type;
    unsigned char byte1;  // first byte
    unsigned char byte2;  // second byte,
    unsigned char byte3;  // third byte
    unsigned char byte4;  // fourth byte
}MacDot16ARQDiscardMsg;
/*
typedef struct
{
    MacDot16TimerType timerType; // Block Life Time / Retry timer
    UInt16 BSNId;
    UInt8 noOfBlocks;
}MacDot16ARQTimerInfo;
*/

// /**
// MACRO       :: MacDot16GetTransId(mgmtMsg)
// DESCRIPTION :: Get transaction ID from a mgmt message structure above
// **/
#define MacDot16GetTransId(mgmtMsg) \
            ((msmtMsg)->transId[0] * 256 + (mgmtMsg)->transId[1])

// /**
// MACRO       :: MacDot16SetTransId(mgmtMsg, val)
// DESCRIPTION :: Set transaction ID from a mgmt message structure above
// **/
#define MacDot16SetTransId(mgmtMsg, val) \
            (mgmtMsg)->transId[0] = (unsigned char)((val) >> 8); \
            (mgmtMsg)->transId[1] = (unsigned char)(val)


#define DOT16e_MAX_POWER_SAVING_CLASS                3
// Node will send the Power saving class activation request
// if it does not transmit/receive any PDU last no of frames
#define DOT16e_DEFAULT_PS1_IDLE_TIME                        6
#define DOT16e_DEFAULT_PS2_IDLE_TIME                        6
#define DOT16e_DEFAULT_PS3_IDLE_TIME                        6

#define DOT16e_DEFAULT_PS3_SLEEP_WINDOW                    10
#define DOT16e_DEFAULT_PS3_FINAL_SLEEP_WINDOW_EXPONENT      3
#define DOT16e_DEFAULT_PS3_FINAL_SLEEP_WINDOW_BASE        576

// /**
// ENUM        :: MacDot16PSClassType
// DESCRIPTION :: Possible power saving class type
// **/
typedef enum
{
    POWER_SAVING_CLASS_NONE = 0,
    POWER_SAVING_CLASS_1    = 1,
    POWER_SAVING_CLASS_2    = 2,
    POWER_SAVING_CLASS_3    = 3
}MacDot16ePSClassType;

typedef enum
{
    POWER_SAVING_CLASS_STATUS_NONE      = 0,
    POWER_SAVING_CLASS_NOT_SUPPORTED    = 1,
    POWER_SAVING_CLASS_SUPPORTED        = 2,
    POWER_SAVING_CLASS_DEACTIVATE       = 3,
    POWER_SAVING_CLASS_ACTIVATE         = 4,
}MacDot16ePSClassStatus;

// /**
// STRUCT      :: MacDot16ePSClasses
// DESCRIPTION :: Power saving classes
// **/
typedef struct mac_dot16e_ps_classes
{
    MacDot16ePSClassType classType;// 1, 2, 3
    MacDot16ePSClassStatus psClassStatus;
    MacDot16ePSClassStatus currentPSClassStatus;
    UInt16 slipId;
    UInt8 psClassId;
    BOOL statusNeedToChange;
    UInt8 psDirection;
    // relevant parameters
    UInt8 initialSleepWindow;
    UInt8 listeningWindow;
    UInt8 finalSleepWindowExponent; // value can not be more than 2^3
    UInt16 finalSleepWindowBase; // value can not be more than 2^10
    BOOL trafficIndReq;
    BOOL trafficTriggeredFlag; // if TRUE, deactivate the power saving
                               //class after appearing the traffic indication
    int startFrameNum;    // value can not be more than 2^6
    BOOL isSleepDuration;
    //clocktype sleepEndTime;
    int sleepDuration; // in no of frames
// No of service flow is exists in this PS class
    int numOfServiceFlowExists;

// PS class does not received/Transmit any PDU, last no of frames
// whenever node transmit/received any pdu for this class
//  set this variable value is 0
//  When this variable value is exceeds the limit respective clss idle time
//  node will generate the MOB-SLP-REQ message with activation
    int psClassIdleLastNoOfFrames;
    BOOL isWaitForSlpRsp;
    BOOL isMobTrfIndReceived;
} MacDot16ePSClasses;


// /**
// STRUCT      :: MacDot16DcdMsg
// DESCRIPTION :: Structure of the DCD message.
// **/
typedef struct
{
    unsigned char type;  // message type, must be DOT16_DCD
    unsigned char dlChannelId; // identifier of the downlink channel
    unsigned char changeCount; // configuraton change count

    // rests are variable size information for the overall channel
    // and downlink burst profiles, which will be filled at runtime.
} MacDot16DcdMsg;

// /**
// STRUCT      :: MacDot16UcdMsg
// DESCRIPTION :: Structure of the UCD message.
// **/
typedef struct
{
    unsigned char type;         // message type, must be DOT16_UCD
    unsigned char changeCount;  // configuration change count
    unsigned char rangeBOStart; // Ranging Backoff Start
    unsigned char rangeBOEnd;   // Ranging Backoff End
    unsigned char requestBOStart; // BW request backoff start
    unsigned char requestBOEnd; // BW requeset backoff end

    // rests are variable size information for the overall channel
    // and uplink burst profiles, which will be filled at runtime.
} MacDot16UcdMsg;

// /**
// STRUCT      :: MacDot16DlMapMsg
// DESCRIPTION :: Structure of the DL-MAP message.
//                Note: We moved the "PHY Synchronization Field" to be after
//                      Base Station ID, before DL-MAP IEs as it is PHY
//                      specific as DL-MAP IEs. This won't affect the
//                      simulation results as packet size won't be changed.
// **/
typedef struct
{
    unsigned char type;    // must be DOT16_DL_MAP type
    // phySynchronizationField moved to later
    unsigned char dcdCount; // Identify the effective DCD
    unsigned char baseStationId[DOT16_BASE_STATION_ID_LENGTH]; // BS ID
    // variable size bytes for phy syn field, DL-MAP IEs, Padding
    // NIbble etc. Will be filled at runtime as they are variable size
} MacDot16DlMapMsg;

// /**
// STRUCT      :: MacDot16UlMapMsg
// DESCRIPTION :: Structure of the UL-MAP message.
// **/
typedef struct
{
    unsigned char type;          // message type, must be DOT16_UL_MAP type
    unsigned char ulChannelId;   // uplink channel ID that this msg refers
    unsigned char ucdCount;      // Identify the effective UCD
    unsigned char allocStartTime[4]; // Effective start time of the
                                     // uplink alloc

    // rests are variable size UL-MAP IEs and possible Padding NIbble
} MacDot16UlMapMsg;

// OFDMA CDMA raning related msg
// /**
// STRUCT      :: MacDot16OfdmaRngBwReqCodeMsg
// DESCRIPTION :: This msg is canned to implement CDMA range
// **/
typedef struct
{
    unsigned char type;      // message type, DOT16_OFDMA_RNG_BW_REQ_CODE
    unsigned char codeIndex; // instead use concreter code, use code index
    unsigned char symbolOffset;     // OFDMA symbol number
    unsigned char subchannelOffset; // OFDMA symbol subchannel
} MacDot16OfdmaRngBwReqCodeMsg;

// /**
// STRUCT      :: MacDot16RngReqMsg
// DESCRIPTION :: Structure of the ranging request message
// **/
typedef struct
{
    unsigned char type;    // must be DOT16_RNG_REQ
    unsigned char dlChannelId;

    // variable size TLVs
} MacDot16RngReqMsg;

// /**
// ENUM        :: MacDot16RangeStatus
// DESCRIPTION :: Ranging status to be included in RNG-RSP message
//                as one TLV value
// **/
typedef enum
{
    RANGE_CONTINUE = 1,  // continue ranging
    RANGE_ABORT    = 2,  // abort ranging
    RANGE_SUCC     = 3,  // raning success
    RANGE_RERANGE  = 4   // re-ranging
} MacDot16RangeStatus;

// /**
// STRUCT      :: MacDot16RngRspMsg
// DESCRIPTION :: Structure of the ranging request message
// **/
typedef struct
{
    unsigned char type;  // message type, must be DOT16_RNG_RSP
    unsigned char ulChannelId; // Uplink Channel ID

    // variable size TLVs
} MacDot16RngRspMsg;

// /**
// STRUCT      :: MacDot16RngCdmaInfo
// DESCRIPTION :: Structure of the cdma information
// **/
typedef struct
{
    unsigned char ofdmaFrame; //OFDMA frame where SS sent the ranging code
    unsigned char rangingCode; //Ranging code index that was sent by SS
    // OFDMA subchannel reference that was used to transmit the ranging code
    unsigned char ofdmaSubchannel;
    unsigned char ofdmaSymbol; // Used to indicate the OFDM time symbol reference
} MacDot16RngCdmaInfo;


typedef struct mac_dot16_cdma_info
{
    MacDot16RngCdmaInfo cdmaInfo;
    BOOL rngRspSentFlag;
    MacDot16CdmaRangingCodeType codetype;
    MacDot16RangeStatus rngStatus;
    mac_dot16_cdma_info* next;
} MacDot16RngCdmaInfoList;
// /**
// ENUM        :: MacDot16SbcReqMsg
// DESCRIPTION :: Structure of the SS's basic capability request message
// **/
typedef struct
{
    unsigned char type;   // message type, must be DOT16_SBC_REQ

    // variable size TLVs
} MacDot16SbcReqMsg;

// /**
// ENUM        :: MacDot16SbcRspMsg
// DESCRIPTION :: Structure of the basic capability response message
// **/
typedef struct
{
    unsigned char type;   // message type, must be DOT16_SBC_REQ

    // variable size TLVs
} MacDot16SbcRspMsg;

// /**
// ENUM        :: MacDot16PkmReqMsg
// DESCRIPTION :: Structure of the public key management request message
// **/
typedef struct
{
    unsigned char type;   // message type, must be DOT16_PKM_REQ
    unsigned char code;
    unsigned char pkmId;
    // variable size TLVs
} MacDot16PkmReqMsg;

// /**
// ENUM        :: MacDot16PkmRspMsg
// DESCRIPTION :: SStructure of the public key management response message
// **/
typedef struct
{
    unsigned char type;   // message type, must be DOT16_PKMREQ
    unsigned char code;
    unsigned char pkmId;
    // variable size TLVs
} MacDot16PkmRspMsg;


// /**
// STRUCT      :: MacDot16RegReqMsg
// DESCRIPTION :: Structure of the registration request message
// **/
typedef struct
{
    unsigned char type;  // message type, must be DOT16_REG_REQ

    // variable size TLVs
} MacDot16RegReqMsg;

// /**
// STRUCT      :: MacDot16RegRspMsg
// DESCRIPTION :: Structure of the registration response message
// **/
typedef struct
{
    unsigned char type;  // message type, must be DOT16_REG_RSP
    unsigned char response;
    // variable size TLVs
} MacDot16RegRspMsg;

// /**
// STRUCT      :: MacDot16RepReqMsg
// DESCRIPTION :: Structure of the REP-REQ message
// **/
typedef struct
{
    unsigned char type; // message type, must be DOT16_REP_REQ
} MacDot16RepReqMsg;

// /**
// STRUCT      :: MacDot16RepRspMsg
// DESCRIPTION :: Structure of the REP-REQ message
// **/
typedef struct
{
    unsigned char type; // message type, must be DOT16_REP_RSP
} MacDot16RepRspMsg;

// /**
// STRUCT      :: MacDot16PaddedMsg
// DESCRIPTION :: Structure of the padded msg
// **/
typedef struct
{
    unsigned char type;  // message type, must be DOT16_PADDED_DATA
} MacDot16PaddedMsg;

// /**
// STRUCT      :: MacDot16DsaReqMsg
// DESCRIPTION :: Structure of the DSA request message
// **/
typedef struct
{
    unsigned char type; // message type, must be DOT16_DSA_REQ
    unsigned char transId[2]; // Transaction ID

    // variable size TLVs
} MacDot16DsaReqMsg;

// /**
// STRUCT      :: MacDot16DsaRspMsg
// DESCRIPTION :: Structure of the DSA response message
// **/
typedef struct
{
    unsigned char type; // message type, must be DOT16_DSA_RSP
    unsigned char transId[2];  // Transaction ID
    unsigned char confirmCode; // Confirmation Code

    // variable size TLVs
} MacDot16DsaRspMsg;

// /**
// STRUCT      :: MacDot16DsaAckMsg
// DESCRIPTION :: Structure of the DSA ACK message
//                This is a response to DSA-RSP
// **/
typedef struct
{
    unsigned char type;  // message type, must be DOT16_DSA_ACK
    unsigned char transId[2];  // Transaction ID
    unsigned char confirmCode; // Confirmation Code

    // variable size TLVs
} MacDot16DsaAckMsg;

// /**
// STRUCT      :: MacDot16DscReqMsg
// DESCRIPTION :: Structure of the DSC request message
// **/
typedef struct
{
    unsigned char type;  // message type, must be DOT16_DSC_REQ
    unsigned char transId[2]; // Transaction ID

    // variable size TLVs
} MacDot16DscReqMsg;

// /**
// STRUCT      :: MacDot16DscRspMsg
// DESCRIPTION :: Structure of the DSC response message
// **/
typedef struct
{
    unsigned char type;  // message type, must be DOT16_DSC_RSP
    unsigned char transId[2];  // Transaction ID
    unsigned char confirmCode; // Confirmation code

    // variable size TLVs
} MacDot16DscRspMsg;

// /**
// STRUCT      :: MacDot16DscAckMsg
// DESCRIPTION :: Structure of the DSC ACK message
// **/
typedef struct
{
    unsigned char type;  // message type, must be DOT16_DSC_ACK
    unsigned char transId[2];  // Transaction ID
    unsigned char confirmCode; // Confirmation code

    // variable size TLVs
} MacDot16DscAckMsg;

// /**
// STRUCT      :: MacDot16DsdReqMsg
// DESCRIPTION :: Structure of the DSD request message
// **/
typedef struct
{
    unsigned char type;  // message type, must be DOT16_DSD_REQ
    unsigned char transId[2]; // Transaction ID
    // unsigned int sfid;  // Service Flow ID
    unsigned char sfid[4];  // Service Flow ID

    // variable size TLVs
} MacDot16DsdReqMsg;

// /**
// STRUCT      :: MacDot16DsdRspMsg
// DESCRIPTION :: Structure of the DSD response message
// **/
typedef struct
{
    unsigned char type;  // message type, must be DOT16_DSD_RSP
    unsigned char transId[2]; // Transaction ID
    unsigned char confirmCode; // Confirmation Code
    //unsigned int sfid;  // Service Flow ID
    unsigned char sfid[4];  // Service Flow ID
    // variable size TLVs
} MacDot16DsdRspMsg;

// /**
// STRUCT      :: MacDot16DsxRvdMsg
// DESCRIPTION :: Structure of the DSX-RVD message
// **/
typedef struct
{
    unsigned char type; // message type, must be DOT16_DSX_RVD
    unsigned char transId[2]; // Transaction ID
    unsigned char confirmCode; // Confirmation Code
    // no TLVs present
} MacDot16DsxRvdMsg;

// /**
// STRUCT      :: MacDot16McaReqMsg
// DESCRIPTION :: Structure of the MCA request message
// **/
typedef struct
{
    unsigned char type;  // message type, must be DOT16_MCA_REQ
    unsigned char transId[2]; // Transaction ID

    // variable size TLVs
} MacDot16MacReqMsg;

// /**
// STRUCT      :: MacDot16McaRspMsg
// DESCRIPTION :: Structure of the MCA response message
// **/
typedef struct
{
    unsigned char type;  // message type, must be DOT16_MCA_RSP
    unsigned char transId[2];  // Transaction ID
    unsigned char confirmCode; // Confirmation Code

    // variable size TLVs
} MacDot16MacRspMsg;

// /**
// STRUCT      :: MacDot16SsBasicCapability
// DESCRIPTION :: Structure of SS basic capbility
// **/
typedef struct
{
    // general
    unsigned char bwAllocSupport;
    unsigned char transtionGaps[2]; //  bit 0-7 SSTTG, bit 8-15 SSRT
    unsigned char maxTxPower[4];
    unsigned char currentTxPower;   // This one could be move to other place

    unsigned char supportedPSMOdeClasses;
    // PHY specific
} MacDot16SsBasicCapability;

// /**
// STRUCT      :: MacDot16SsAuthKeyInfo
// DESCRIPTION :: Infomation for Authentication and Key
// **/
typedef struct
{
    MacDot16PKMCode lastMsgCode;
    unsigned char pkmId;
    unsigned char hmacKeySeq;
    // ToDo: add more auth related
    // TODO: add Key related

} MacDot16SsAuthKeyInfo;

//
// Data structure for burst profiles
//

// /**
// STRUCT      :: MacDot16OfdmaUlBurstProfile
// DESCRIPTION :: UL burst profile for OFDMA PHY
// **/
typedef struct
{
    unsigned char fecCodeModuType;

    // two parameters below are not supported right now
    //unsigned char rngDataRatio;
    //unsigned char cnOverride[5];

    // we actually use the two below in this imp.
    unsigned char exitThreshold;
    unsigned char entryThreshold;
} MacDot16OfdmaUlBurstProfile;

// /**
// UNION       :: MacDot16UlBurstProfile
// DESCRIPTION :: UL burst profile
// **/
typedef union
{
    MacDot16OfdmaUlBurstProfile ofdma;
} MacDot16UlBurstProfile;

// /**
// STRUCT      :: MacDot16OfdmaDlBurstProfile
// DESCRIPTION :: DL burst profile for OFDMA PHY
// **/
typedef struct
{
    unsigned char fecCodeModuType;
    unsigned char exitThreshold;
    unsigned char entryThreshold;
} MacDot16OfdmaDlBurstProfile;

// /**
// UNION       :: MacDot16DlBurstProfile
// DESCRIPTION :: DL burst profile. Union of different PHY specific
// **/
typedef union
{
    MacDot16OfdmaDlBurstProfile ofdma;
} MacDot16DlBurstProfile;

typedef struct dot16_burst_info_str
{
     unsigned int       burstIndex : 16, // index of burst in this sub-frame
                        modCodeType: 16; // FEC modulation coding type
     unsigned int subchannelOffset : 16, // start point of subchannels used
                      symbolOffset : 16; // start point of symbols used
     unsigned int numSubchannels : 16,   // # of sub-channels of this burst
                      numSymbols : 16;   // # of symbols of this burst,

    unsigned char signalType;      // type of the signal normal / cdma
                                   // code used for CDMA signal
    unsigned char cdmaCodeIndex;
    unsigned char cdmaSpreadingFactor;
} Dot16BurstInfo;

//--------------------------------------------------------------------------
// Data structures for CS classifier
//--------------------------------------------------------------------------

typedef enum
{
    CLASSIFIER_TYPE_1 = 1,  // queue for each class

    // TEMP MULTICAST IMPLEMETATION
    CLASSIFIER_TYPE_MCAST_UGS,
    CLASSIFIER_TYPE_MCAST_ertPS,
    CLASSIFIER_TYPE_MCAST_rtPS,
    CLASSIFIER_TYPE_MCAST_nrtPS,
    CLASSIFIER_TYPE_MCAST_BE,
} MacDot16ClassifierType;

// /**
// STRUCT      :: MacDot16CsClassifier
// DESCRIPTION :: Structure to store an IPv4 or IPv6 classifier
//                An application data flow can be uniquely identified
//                by the tuple of <source-address, dest-address, flow,
//                traffic class>
// **/

typedef struct
{
    unsigned char           ipProtocol; // IP protocol, eg. UDP/TCP/ICMP...
    MacDot16ClassifierType  type;       // Type 1 or Type 2
                                        // (not based on tuple)

    Address srcAddr;    // Address of the source node (IPv4, IPv6 or ATM)
    Address dstAddr;    // Address of the dest node (IPv4, IPv6 or ATM)

    unsigned short srcPort;  // Port number at source node
    unsigned short dstPort;  // Port number at dest node

    Mac802Address nextHopAddr; // address of next hop
} MacDot16CsClassifier;

//--------------------------------------------------------------------------
// Data structures for data service flow
//--------------------------------------------------------------------------

// /**
// STRUCT      :: MacDot16QoSParameter
// DESCRIPTION :: QoS parameters of a data service flow
// **/
typedef struct mac_dot16_qos_parameter_str
{
    unsigned char priority;     // priority of the service flow
    int minPktSize;             // smallest packet size
    int maxPktSize;             // largest packet size
    int maxSustainedRate;       // maximum sustained traffic, bits per
                                // second
    int minReservedRate;        // minimum reserved traffic rate, bits per
                                // second
    clocktype maxLatency;       // maximum latency
    clocktype toleratedJitter;  // tolerated jitter
} MacDot16QoSParameter;

// /**
// ENUM        :: MacDot16ServiceFlowDirection
// DESCRIPTION :: The direction of the service flow
// **/
typedef enum
{
    DOT16_UPLINK_SERVICE_FLOW = 0,
    DOT16_DOWNLINK_SERVICE_FLOW = 1
}MacDot16ServiceFlowDirection;

// /**
// ENUM        :: MacDot16ServiceFlowInitial
// DESCRIPTION :: The direction of the service flow
// **/
typedef enum
{
    DOT16_SERVICE_FLOW_LOCALLY_INITIATED = 0,
    DOT16_SERVICE_FLOW_REMOTELY_INITIATED = 1,
}MacDot16ServiceFlowInitial;

// /**
// ENUM        :: MacDot16StationClass
// DESCRIPTION :: Station class for a SS
// **/
typedef enum
{
    // SS Station Class
    DOT16_STATION_CLASS_UNKNOWN = 0,
    DOT16_STATION_CLASS_GOLD = 1,
    DOT16_STATION_CLASS_SILVER = 2,
    DOT16_STATION_CLASS_BRONZE = 3,

    DOT16_STATION_CLASS_APP_SPECIFIED = 98,
    DOT16_STATION_CLASS_DEFAULT = 99
}MacDot16StationClass;

// /**
// ENUM        :: MacDot16ServiceClass
// DESCRIPTION :: Service classe
// **/
typedef enum
{
    // SS service Class
    DOT16_S_CLASS_BE = 0,
    DOT16_S_CLASS_nrtPS = 1,
    DOT16_S_CLASS_rtPS = 2,
    DOT16_S_CLASS_UGS = 3,
    DOT16_S_CLASS_ertPS = 4,

    DOT16_S_CLASS_DEFAULT = 99
}MacDot16ServiceClass;

// /**
// ENUM        :: MacDot16ServieFlowXactType
// DESCRIPTION :: Service flow ttransaction type
// **/
typedef enum
{
    DOT16_FLOW_XACT_Add,
    DOT16_FLOW_XACT_Change,
    DOT16_FLOW_XACT_Delete,
}MacDot16ServieFlowXactType;

// /**
// STRUCT     :: MacDot16DsxInfo
// DESCRPTION :: Information for Dsx
// **/
typedef struct
{
    unsigned int dsxTransId;             // transactionId for the xAct
    MacDot16ServieFlowXactType xactType; // transaction type;
    MacDot16FlowTransConfirmCode dsxCC;  // comfirm code for the transaction
    unsigned char dsxRetry;              // num of dsx retry has been made
    MacDot16FlowTransactionStatus dsxTransStatus; // xAct status, not flow
                                                  // status
    MacDot16ServiceFlowInitial sfInitial; // remotely/locally initiated

    Message* timerT10;   // timer for waiting transaction end
    Message* timerT8;    // timer for waiting for DSA/DSC-ACK
    Message* timerT7;    // timer for waiting DSA.DSC.DSD response
    Message* timerT14;   // timer for waiting DSX-RVD

    Message* dsxReqCopy; // a copy of dsx-req DSA/DSC/DSD
    Message* dsxAckCopy; // a copy of dsx-ack DSA/DSC/DSD
    Message* dsxRspCopy; // a copy of dsx-ack DSA/DSC/DSD

    // DSC only
    MacDot16QoSParameter *dscOldQosInfo; // a saved copy of old QoS state
    MacDot16QoSParameter *dscNewQosInfo; // a saved copy of new QoS state
    MacDot16QoSParameter *dscPendingQosInfo;

} MacDot16DsxInfo;

// /**
// STRUCT      :: MacDot16ServiceFlow
// DESCRIPTION :: Information of one service flow
// **/
typedef struct mac_dot16_service_flow_str
{
    // basic service flow properties
    BOOL admitted;      // indicate whether the flow has been admitted
    BOOL activated;     // indicate whether the flow has been activated
    BOOL provisioned;   // indicate whether the flow has been provisioned
    MacDot16FlowStatus status; // indicate the current status of the flow

    unsigned int sfid;  // Service Flow Identifier
    MacDot16ServiceFlowDirection sfDirection; // the ul/dl flow
    Dot16CIDType cid;   // CID assigned to this flow
    unsigned int csfId; // ID of the classifier kept at CS sublayer

    // QoS parameters
    MacDot16ServiceType serviceType; // Type of the service
    MacDot16QoSParameter qosInfo;    // QoS info of the flow

    // Currently qosInfo is the only QoS Params kept in SS/BS,
    // Assume the following three set are the same
    //MacDot16QoSParameter* provisionedQoSInfo; // provisioned
    //MacDot16QoSParameter* admittedQoSInfo; // for reserving resources
    //MacDot16QoSParameter* activeQoSInfo;  // actually provided to the flow

    unsigned char numXact;   // Max 3, # of trans. concurrently
    MacDot16DsxInfo dsaInfo; // dsa transaction information
    MacDot16DsxInfo dscInfo; // dsc transaction information
    MacDot16DsxInfo dsdInfo; // dsd transaction information

    unsigned char reqTransPolicy; // request/transmission policy

    // bandwidth request and allocations
    int bwRequested;  // Bandwidth requested will be sent out
    int lastBwRequested; // bandwidth request sent out at last time
    int bwGranted;    // Bandwidth granted by BS in current ul frame
    int maxBwGranted;
    BOOL needPiggyBackBwReq; // BW can be piggyback with the data PDU

    // packet queue. The actual packet queue is inside the scheduler
    int queuePriority;        // priority of the queue in the scheduler
    double queueWeight;       // raw weight of the queue
    Message* tmpMsgQueueHead; // tmp queue to hold packet before admitted
    Message* tmpMsgQueueTail; // tmp queue to hold packet before admitted
    int numBytesInTmpMsgQueue;// # of bytes in tmp message queue
    BOOL isEmpty;             // whether the queue is empty
    clocktype emptyBeginTime; // time when the flow is idle

    // scheduling information
    clocktype lastAllocTime;  // time of the last allocation to this flow

    //paging
    BOOL pagingPref;        //Paging preference
    BOOL isPackingEnabled;
    BOOL isFixedLengthSDU;

    BOOL isARQEnabled;//ARQ info
    MacDot16ARQParam* arqParam;
    BOOL arqDiscardMsgSent;
    UInt16 arqDiscardBlockBSNId;
    MacDot16ARQControlBlock* arqControlBlock ;
    Message* arqSyncLossTimer;
    unsigned int fixedLengthSDUSize;
    int fragFSNno;
    BOOL isFragStart;
    UInt16 bytesSent;
    TraceProtocolType appType;
    // Will used to stored the received fragmented msg
    Message* fragMsg;
    UInt16 noOfFragPacketReceived;
    UInt16 fragFSNNoReceived[DOT16_MAX_NO_FRAGMENTED_PACKET * 256];

    //void* psClassParameter;
    BOOL TrafficIndicationPreference; // in sleep mode
    MacDot16ePSClassType psClassType;
    //MacDot16ePSClassStatus currentPSClassStatus;
    Message* timerT22; // Timer for ARQ Reset
    mac_dot16_service_flow_str* next; // pointer to next service flow
} MacDot16ServiceFlow;

// /**
// STRUCT      :: MacDot16ServiceFlowList
// DESCRIPTION :: Structure to keep a list of service flows
// **/
typedef struct
{
    MacDot16ServiceFlow* flowHead;
    int numFlows;

    // information for admission control
    int maxSustainedRate; // sum of max sustained traffic of all flows
    int minReservedRate;  // sum of min reserved raffic rate of all flows

    // Scheduling information
    int bytesRequested;  // # of bytes requested by all flows in this list
} MacDot16ServiceFlowList;

#define MacDot16ServiceFlowQoSChanged(qosInfo1, qosInfo2) \
                (qosInfo1.maxLatency != qosInfo2.maxLatency  || \
                 qosInfo1.maxPktSize != qosInfo2.maxPktSize|| \
                 qosInfo1.maxSustainedRate != qosInfo2.maxSustainedRate || \
                 qosInfo1.minPktSize != qosInfo2.minPktSize|| \
                 qosInfo1.minReservedRate != qosInfo2.minReservedRate || \
                 qosInfo1.toleratedJitter != qosInfo2.toleratedJitter)



// -------------------------------------------------------------------------
// Basic structure for signal measurement
// -------------------------------------------------------------------------
// /**
// STRUCT      :: MacDot16SignalMeasurementInfo
// DESCRIPTION :: data structure for signal measurement
// **/
typedef struct struct_mac_dot16_singal_measurement_info
{
    double rssi;
    double cinr;
    double relativeDelay;
    double roubdTripDelay;
    clocktype measTime;
    BOOL isActive;
}MacDot16SignalMeasurementInfo;

//--------------------------------------------------------------------------
// Basic data structs such as parameters, statistics, protocol data struc
//--------------------------------------------------------------------------

// /**
// STRUCT      :: MacDot16Stats
// DESCRIPTION :: Data structure for storing dot16 statistics
// **/
typedef struct
{
} MacDot16Stats;

// /**
// STRUCT      :: MacDataDot16
// DESCRIPTION :: Data structure of Dot16 model
// **/
typedef struct struct_mac_dot16_str
{
    MacData* myMacData;

    // we use the mac addresss as BS id
    unsigned char macAddress[DOT16_MAC_ADDRESS_LENGTH];

    // random seed
    RandomSeed seed;

    // some basic properties
    MacDot16StationType stationType;  // type of the station
    MacDot16Mode mode;                // operation mode
    MacDot16DuplexMode duplexMode;    // duplex mode
    MacDot16PhyType phyType;          // type of the PHY

    // whether 802.16e features are supported
    BOOL dot16eEnabled;

    // channel related variables
    int *channelList;                // a list of operation channels
    short numChannels;               // # of channels in the channel list
    int lastListeningChannel;        // The channel last listen to

    // PHY related properties
    clocktype dlPsDuration;
    clocktype ulPsDuration;

    //paging
    UInt16 macHashSkipThshld;// MAC Hash Skip Threshold
    // general statistics
    MacDot16Stats stats;

    void* bsData;              // point to data structure of BS
    void* ssData;              // point to data structure of SS
    void* csData;              // point to convergence sublayer
    void* phyData;             // point to data structure of phy sublayer
    void* qosData;             // point to QOS data structure
    void* tcData;              // point to traffic conditioner data
                               // structure

} MacDataDot16;

//--------------------------------------------------------------------------
//  Utility functions
//--------------------------------------------------------------------------
// /**
// FUNCTION   :: MacDot16SetTimer
// LAYER      :: MAC
// PURPOSE    :: Set a timer message. If a non-NULL message pointer is
//               passed in, this function will just send out that msg.
//               If NULL message is passed in, it will create a new
//               message and send it out. In either case, a pointer to
//               the message is returned, in case the caller wants to
//               cancel the message in the future.
// PARAMETERS ::
// + node      : Node*             : Pointer to node
// + dot16     : MacDataDot16*     : Pointer to DOT16 structure
// + timerType : MacDot16TimerType : Type of the timer
// + delay     : clocktype         : Delay of this timer
// + msg       : Message*          : If non-NULL, use this message
// + infoVal   : unsigned int      : Additional info if needed
// RETURN     :: Message*          : Pointer to the timer message
// **/
Message* MacDot16SetTimer(Node* node,
                          MacDataDot16* dot16,
                          MacDot16TimerType timerType,
                          clocktype delay,
                          Message* msg,
                          unsigned int infoVal);
// /**
// FUNCTION   :: MacDot16SetTimer
// LAYER      :: MAC
// PURPOSE    :: Set a timer message. If a non-NULL message pointer is
//               passed in, this function will just send out that msg.
//               If NULL message is passed in, it will create a new
//               message and send it out. In either case, a pointer to
//               the message is returned, in case the caller wants to
//               cancel the message in the future.
// PARAMETERS ::
// + node      : Node*             : Pointer to node
// + dot16     : MacDataDot16*     : Pointer to DOT16 structure
// + timerType : MacDot16TimerType : Type of the timer
// + delay     : clocktype         : Delay of this timer
// + msg       : Message*          : If non-NULL, use this message
// + infoVal   : unsigned int      : Additional info if needed
// + infoVal2  : unsigned int      : additional info if needed
// RETURN     :: Message*          : Pointer to the timer message
// **/
Message* MacDot16SetTimer(Node* node,
                          MacDataDot16* dot16,
                          MacDot16TimerType timerType,
                          clocktype delay,
                          Message* msg,
                          unsigned int infoVal,
                          unsigned int infoVal2);
// /**
// FUNCTION   :: MacDot16GetIndexInChannelList
// LAYER      :: MAC
// PURPOSE    :: Get the index of a channel in the channel list
// PARAMETERS ::
// + node      : Node*     : Pointer to node.
// + dot16     : MacDot16* : Pointer to DOT16 data struct
// + channelIndex : int    : Channel index of the channel
// RETURN     :: int       : The index of the channel in my channel list
// **/
int MacDot16GetIndexInChannelList(Node* node,
                                  MacDataDot16* dot16,
                                  int channelIndex);

// /**
// FUNCTION   :: MacDot16StartListening
// LAYER      :: MAC
// PURPOSE    :: Start listening to a channel
// PARAMETERS ::
// + node : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to dot16 structure
// + channelIndex : int : index of the channel
// RETURN     :: void : NULL
// **/
void MacDot16StartListening(
         Node* node,
         MacDataDot16* dot16,
         int channelIndex);

// /**
// FUNCTION   :: MacDot16StopListening
// LAYER      :: MAC
// PURPOSE    :: Stop listening to a channel
// PARAMETERS ::
// + node : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to Dot16 structure
// + channelIndex : int : index of the channel
// RETURN     :: void : NULL
// **/
void MacDot16StopListening(
         Node* node,
         MacDataDot16* dot16,
         int channelIndex);

// /**
// FUNCTION   :: MacDot16IsMyCid
// LAYER      :: MAC
// PURPOSE    :: Check if a CID belongs or applicable to me
// PARAMETERS ::
// + node : Node* : Pointer to node
// + dot16: MacDataDot16* : Pointer to Dot16 structure
// + cid  : Dot16CIDType  : CID value
// RETURN     :: BOOL : TRUE, if my CID, FALSE, else
// **/
BOOL MacDot16IsMyCid(
         Node* node,
         MacDataDot16* dot16,
         Dot16CIDType cid);

// /**
// FUNCTION   :: MacDot16IsBasicCid
// LAYER      :: MAC
// PURPOSE    :: Check if a CID is a basic CID
// PARAMETERS ::
// + cid  : Dot16CIDType  : CID value
// RETURN     :: BOOL : TRUE, if basic CID, FALSE, else
// **/
static inline
BOOL MacDot16IsBasicCid(Dot16CIDType cid)
{
    if (cid >= DOT16_BASIC_CID_START && cid <= DOT16_BASIC_CID_END)
    {
        return TRUE;
    }

    return FALSE;
}

// /**
// FUNCTION   :: MacDot16IsPrimaryCid
// LAYER      :: MAC
// PURPOSE    :: Check if a CID is a primary CID
// PARAMETERS ::
// + cid  : Dot16CIDType  : CID value
// RETURN     :: BOOL : TRUE, if primary CID, FALSE, else
// **/
static inline
BOOL MacDot16IsPrimaryCid(Dot16CIDType cid)
{
    if (cid >= DOT16_PRIMARY_CID_START && cid <= DOT16_PRIMARY_CID_END)
    {
        return TRUE;
    }

    return FALSE;
}

// /**
// FUNCTION   :: MacDot16IsSecondaryCid
// LAYER      :: MAC
// PURPOSE    :: Check if a CID is a secondary CID
// PARAMETERS ::
// + cid  : Dot16CIDType  : CID value
// RETURN     :: BOOL : TRUE, if secondary CID, FALSE, else
// **/
static inline
BOOL MacDot16IsSecondaryCid(Dot16CIDType cid)
{
    // secondary cid is supported.
    return FALSE;
}

// /**
// FUNCTION   :: MacDot16IsManagementCid
// LAYER      :: MAC
// PURPOSE    :: Check if a CID is a management CID
// PARAMETERS ::
// + node : Node* : Pointer to node
// + dot16: MacDataDot16* : Pointer to Dot16 structure
// + cid  : Dot16CIDType  : CID value
// RETURN     :: BOOL : TRUE, if mgmt CID, FALSE, else
// **/
BOOL MacDot16IsManagementCid(
         Node* node,
         MacDataDot16* dot16,
         Dot16CIDType cid);

// /**
// FUNCTION   :: MacDot16IsTransportCid
// LAYER      :: MAC
// PURPOSE    :: Check if a CID is a transport CID
// PARAMETERS ::
// + node : Node* : Pointer to node
// + dot16: MacDataDot16* : Pointer to Dot16 structure
// + cid  : Dot16CIDType  : CID value
// RETURN     :: BOOL : TRUE, if mgmt CID, FALSE, else
// **/
BOOL MacDot16IsTransportCid(
         Node* node,
         MacDataDot16* dot16,
         Dot16CIDType cid);

// /**
// FUNCTION   :: MacDot16IsMulticastCid
// LAYER      :: MAC
// PURPOSE    :: Check if a CID is a multicast CID
// PARAMETERS ::
// + node : Node* : Pointer to node
// + dot16: MacDataDot16* : Pointer to Dot16 structure
// + cid  : Dot16CIDType  : CID value
// RETURN     :: BOOL : TRUE, if mgmt CID, FALSE, else
// **/
BOOL MacDot16IsMulticastCid(
         Node* node,
         MacDataDot16* dot16,
         Dot16CIDType cid);

// /**
// FUNCTION   :: MacDot16IsMulticastMacAddress
// LAYER      :: MAC
// PURPOSE    :: Check if a given MAC address is multicast address
// PARAMETERS ::
// + node : Node* : Pointer to node
// + dot16: MacDataDot16* : Pointer to Dot16 structure
// + macAddress : unsigned char* : MAC address
// RETURN     :: BOOL : TRUE, if mgmt CID, FALSE, else
// **/
BOOL MacDot16IsMulticastMacAddress(
         Node* node,
         MacDataDot16* dot16,
         unsigned char* macAddress);

// /**
// FUNCTION   :: MacDot16AddDataGrantMgmtSubHeader
// LAYER      :: MAC
// PURPOSE    :: Check if a given types of burst is allocated to SS
// PARAMETERS ::
// + node : Node* : Pointer to node
// + dot16: MacDataDot16* : Pointer to Dot16 structure
// + msgIn : Message** : Pointer to the msg to add the data grant subheader
// + serviceType : MacDot16ServiceType : servive type of the msg belongs to
// + bwReq : int : bandwidth request if non UGS service flow
//                 poll me or slip indicate for UGS service
// RETURN     :: Message* : the new message with the GrantMgmt subhead
// **/
Message* MacDot16AddDataGrantMgmtSubHeader(
         Node* node,
         MacDataDot16* dot16,
         Message* msgIn,
         MacDot16ServiceType serviceType,
         int bwReq);

// /**
// FUNCTION   :: MacDot16PrintRunTimeInfo
// LAYER      :: MAC
// PURPOSE    :: printf out the node id and  simulation time
// PARAMETERS ::
// + node : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to dot16 structure
// RETURN     :: void :
// **/
void MacDot16PrintRunTimeInfo(Node* node, MacDataDot16* dot16);

// /**
// FUNCTION   :: MacDot16GetProfIndexFromCodeModType
// LAYER      :: MAC
// PURPOSE    :: get profile index from the modulation & coding information
// PARAMETERS ::
// + node  : Node*         : Pointer to node
// + dot16 : MacDataDot16* : Pointer to dot16 structure
// + frameType : MacDot16SubframeType : UL or DL frame
// + codeModType : coding and modulation type
// + profIndex : int* : Pointer to the profile index
// RETURN     :: BOOL : NULL
// **/
BOOL MacDot16GetProfIndexFromCodeModType(Node* node,
                                         MacDataDot16* dot16,
                                         MacDot16SubframeType frameType,
                                         unsigned char codeModType,
                                         int* profIndex);
// /**
// FUNCTION   :: MacDot16GetLeastRobustBurst
// LAYER      :: MAC
// PURPOSE    :: Get the least robust downlink burst profile
// PARAMETERS ::
// + node  : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to DOT16 structure
// + frameType : MacDot16SubframeType  : Frame type, either U: or DL
// + signalMea : double : SNR in dB
// + unsigned char* : leastRobustBurst : Pointer for the least robust DIUC
// RETURN     :: BOOL : TRUE, if find one; FALSE, if not find one
// **/
BOOL MacDot16GetLeastRobustBurst(Node* node,
                                 MacDataDot16* dot16,
                                 MacDot16SubframeType frameType,
                                 double signalMea,
                                 unsigned char* leastRobustBurst);

// /**
// FUNCTION   :: MacDot16UpdateMeanMeasurement
// LAYER      :: MAC
// PURPOSE    :: Update the measurement histroy and recalcualte the
//               mean value
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + dot16     : MacDataDot16*: Pointer to DOT16 structure
// + measInfo  : MacDot16SignalMeasurementInfo* : Pointer to the meas. hist.
// + signalMea : PhySignalMeasurement* : Pointer to the current measurement
// + meanMeas  : MacDot16SignalMeasurementInfo* : Pinter to the mean of meas
// RETURN     :: void : NULL
void MacDot16UpdateMeanMeasurement(Node* node,
                                   MacDataDot16* dot16,
                                   MacDot16SignalMeasurementInfo* measInfo,
                                   PhySignalMeasurement* signalMea,
                                   MacDot16SignalMeasurementInfo* meanMeas);

// /**
// FUNCTION   :: MacDot16AddBurstInfo
// LAYER      :: MAC
// PURPOSE    :: Put burst information in the info field of the message
// PARAMETERS ::
// + node      : Node*    : Pointer to node
// + msg       : Message* : Pointer to the message of the burst
// + burstInfo : Dot16BurstInfo* : Information of the burst
// RETURN     :: void : NULL
// **/
static
void inline MacDot16AddBurstInfo(Node* node,
                                 Message* msg,
                                 Dot16BurstInfo* burstInfo)
{
    MESSAGE_AddInfo(node,
                    msg,
                    sizeof(Dot16BurstInfo),
                    INFO_TYPE_Dot16BurstInfo);
    memcpy(MESSAGE_ReturnInfo(msg, INFO_TYPE_Dot16BurstInfo),
           burstInfo,
           sizeof(Dot16BurstInfo));
}

// /**
// FUNCTION   :: MacDot16PrintModulationCodingInfo
// LAYER      :: MAC
// PURPOSE    :: printf out the modulation and codeing information
// PARAMETERS ::
// + node : Node* : Pointer to node
// + dot16 : MacDataDot16* : Pointer to dot16 structure
// RETURN     :: void :
// **/
void MacDot16PrintModulationCodingInfo(Node* node,
                                 MacDataDot16* dot16);

//--------------------------------------------------------------------------
// DSX functions
//--------------------------------------------------------------------------
///**
// FUNCTION   :: MacDot16BuildDsaReqMsg
// LAYER      :: MAC
// PURPOSE    :: Build the DSA-REQ mgmt msg for a service flow
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + dot16     : MacDataDot16*: Pointer to DOT16 structure
// + serviceFlow : MacDot16ServiceFlow* : Pointer to the service flow
// + pktInfo   : MacDot16CsIpv4Classifier* : Pointer to classifier
// + sfDirection : MacDot16ServiceFlowDirection  : Direction of flow
// + cid       : Dot16CIDType : CID message shall use
// RETURN     :: Message* : Point to the message containing the DSA-REQ PDU
// **/
Message* MacDot16BuildDsaReqMsg(
             Node* node,
             MacDataDot16* dot16,
             MacDot16ServiceFlow* serviceFlow,
             MacDot16CsClassifier* pktInfo,
             MacDot16ServiceFlowDirection sfDirection,
             Dot16CIDType cid);

// /**
// FUNCTION   :: MacDot16BuildDsaAckMsg
// LAYER      :: MAC
// PURPOSE    :: Build the DSA-REQ mgmt msg for a service flow
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + dot16     : MacDataDot16*: Pointer to DOT16 structure
// + serviceFlow : MacDot16ServiceFlow* : Pointer to the service flow
// + cid       : Dot16CIDType : CID message shall use
// + transId   : unsigned int : transaction Id
// RETURN     :: Message* : Point to the message containing the DSA-ACK PDU
// **/
Message* MacDot16BuildDsaAckMsg(
             Node* node,
             MacDataDot16* dot16,
             MacDot16ServiceFlow* serviceFlow,
             Dot16CIDType cid,
             unsigned int transId);

// /**
// FUNCTION   :: MacDot16BuildDsaRspMsg
// LAYER      :: MAC
// PURPOSE    :: Build the DSA-RSP mgmt msg for a service flow
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + dot16     : MacDataDot16*: Pointer to DOT16 structure
// + serviceFlow : MacDot16ServiceFlow* : Pointer to the service flow
// + transId   : unsigned int : transId ofr this DSA-RSP
// + cid       : Dot16CIDType : CID message shall use
// + qosChanged : BOOL        : QoS parameters changed or not
// RETURN     :: Message* : Point to the message containing the DSA-REQ PDU
// **/
Message* MacDot16BuildDsaRspMsg(
             Node* node,
             MacDataDot16* dot16,
             MacDot16ServiceFlow* sFlow,
             unsigned int transId,
             BOOL qosChanged,
             Dot16CIDType cid);

// /**
// FUNCTION   :: MacDot16BuildDscReqMsg
// LAYER      :: MAC
// PURPOSE    :: Build the DSC-REQ mgmt msg for a service flow
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + dot16     : MacDataDot16*: Pointer to DOT16 structure
// + sFlow     : MacDot16ServiceFlow* : Pointer to the service flow
// + newQoSInfo : MacDot16QoSParameter* : Poniter to the new QoS info
// + cid       : Dot16CIDType : CID message shall use
// RETURN     :: Message* : Point to the message containing the DSA-REQ PDU
// **/
Message* MacDot16BuildDscReqMsg(
             Node* node,
             MacDataDot16* dot16,
             MacDot16ServiceFlow* sFlow,
             MacDot16QoSParameter* newQoSInfo,
             Dot16CIDType cid);

// /**
// FUNCTION   :: MacDot16BuildDscAckMsg
// LAYER      :: MAC
// PURPOSE    :: Build the DSC-REQ mgmt msg for a service flow
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + dot16     : MacDataDot16*: Pointer to DOT16 structure
// + serviceFlow : MacDot16ServiceFlow* : Pointer to the service flow
// + cid       : Dot16CIDType : CID message shall use
// + transId   : unsigned int : transaction Id
// RETURN     :: Message* : Point to the message containing the DSA-REQ PDU
// **/
Message* MacDot16BuildDscAckMsg(
             Node* node,
             MacDataDot16* dot16,
             MacDot16ServiceFlow* serviceFlow,
             Dot16CIDType cid,
             unsigned int transId);

// /**
// FUNCTION   :: MacDot16BuildDscRspMsg
// LAYER      :: MAC
// PURPOSE    :: Build the DSC-Rsp mgmt msg for a service flow
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + dot16     : MacDataDot16*: Pointer to DOT16 structure
// + serviceFlow : MacDot16ServiceFlow* : Pointer to the service flow
// + transId   : unsigned int : transaction Id
// + cid       : Dot16CIDType : CID message shall use
// RETURN     :: Message* : Point to the message containing the DSA-REQ PDU
// **/
Message* MacDot16BuildDscRspMsg(
             Node* node,
             MacDataDot16* dot16,
             MacDot16ServiceFlow* serviceFlow,
             unsigned int transId,
             Dot16CIDType cid);

// /**
// FUNCTION   :: MacDot16BuildDsdReqMsg
// LAYER      :: MAC
// PURPOSE    :: Build the DSD-REQ mgmt msg for a service flow
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + dot16     : MacDataDot16*: Pointer to DOT16 structure
// + serviceFlow : MacDot16ServiceFlow* : Pointer to the service flow
// + cid       : Dot16CIDType : CID message shall use
// RETURN     :: Message* : Point to the message containing the DSD-REQ PDU
// **/
Message* MacDot16BuildDsdReqMsg(
             Node* node,
             MacDataDot16* dot16,
             MacDot16ServiceFlow* serviceFlow,
             Dot16CIDType cid);

// /**
// FUNCTION   :: MacDot16BuildDsdRspMsg
// LAYER      :: MAC
// PURPOSE    :: Build the DSD_RSP mgmt msg for a service flow
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + dot16     : MacDataDot16*: Pointer to DOT16 structure
// + serviceFlow : MacDot16ServiceFlow* : Pointer to the service flow
// + cid       : Dot16CIDType : CID message shall use
// + transId   : unsigned int : transaction Id
// RETURN     :: Message* : Point to the message containing DSD-RSP PDU
// **/
Message* MacDot16BuildDsdRspMsg(
             Node* node,
             MacDataDot16* dot16,
             MacDot16ServiceFlow* serviceFlow,
             Dot16CIDType cid,
             unsigned int transId);

// /**
// FUNCTION   :: MacDot16ResetDsxInfo
// LAYER      :: MAC
// PURPOSE    :: Build the DSA-RSP mgmt msg for a service flow
// PARAMETERS ::
// + node      : Node*        : Pointer to node.
// + dot16     : MacDataDot16*: Pointer to DOT16 structure
// + serviceFlow : MacDot16ServiceFlow* : Pointer to the service flow
// + dsxInfo   : MacDot16DsxInfo* : Pointer to the dsx information
// RETURN     :: void : NULL
// **/
void MacDot16ResetDsxInfo(Node* node,
                          MacDataDot16* dot16,
                          MacDot16ServiceFlow* sFlow,
                          MacDot16DsxInfo* dsxInfo);

//--------------------------------------------------------------------------
//  API functions
//--------------------------------------------------------------------------

// /**
// FUNCTION   :: MacDot16Init
// LAYER      :: MAC
// PURPOSE    :: Initialize dot16 MAC protocol at a given interface.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + nodeInput        : const NodeInput*  : Pointer to node input.
// + macProtocolName  : char*             : Name of different 802.16
//                                          variants
// + numNodesInSubnet : int               : Number of nodes in subnet.
// + nodeIndexInSubnet: int               : index of the node in subnet.
// RETURN     :: void : NULL
// **/
void MacDot16Init(Node* node,
                  int interfaceIndex,
//                  NodeAddress interfaceAddress,
                  const NodeInput* nodeInput,
                  char* macProtocolName,
                  int numNodesInSubnet,
                  int nodeIndexInSubnet);

// /**
// FUNCTION   :: MacDot16Layer
// LAYER      :: MAC
// PURPOSE    :: Handle timers and layer messages.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// + msg              : Message*          : Message for node to interpret.
// RETURN     :: void : NULL
// **/
void MacDot16Layer(Node *node,
                   int interfaceIndex,
                   Message *msg);

// /**
// FUNCTION   :: MacDot16Finalize
// LAYER      :: MAC
// PURPOSE    :: Print stats and clear protocol variables.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + interfaceIndex   : int               : Interface index
// RETURN     :: void : NULL
// **/
void MacDot16Finalize(Node *node, int interfaceIndex);

// /**
// FUNCTION   :: MacDot16NetworkLayerHasPacketToSend
// LAYER      :: MAC
// PURPOSE    :: Called when network layer buffers transition from empty.
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + dot16            : MacDataDot16*     : Pointer to Dot16 structure
// RETURN     :: void : NULL
// **/
void MacDot16NetworkLayerHasPacketToSend(Node *node, MacDataDot16 *dot16);


// /**
// FUNCTION   :: MacDot16ReceivePacketFromPhy
// LAYER      :: MAC
// PURPOSE    :: Receive a packet from physical layer
// PARAMETERS ::
// + node             : Node*             : Pointer to node.
// + dot16            : MacDataDot16*     : Pointer to DOT16 structure
// + msg              : Message*          : Packet received from PHY
// RETURN     :: void : NULL
// **/
void MacDot16ReceivePacketFromPhy(Node* node,
                                  MacDataDot16* dot16,
                                  Message* msg);

// /**
// FUNCTION   :: MacDot16ReceivePhyStatusChangeNotification
// LAYER      :: MAC
// PURPOSE    :: React to change in physical status.
// PARAMETERS ::
// + node             : Node*         : Pointer to node.
// + dot16            : MacDataDot16* : Pointer to Dot16 structure
// + oldPhyStatus     : PhyStatusType : The previous physical state
// + newPhyStatus     : PhyStatusType : New physical state
// + receiveDuration  : clocktype     : The receiving duration
// RETURN     :: void : NULL
// **/
void MacDot16ReceivePhyStatusChangeNotification(
         Node* node,
         MacDataDot16* dot16,
         PhyStatusType oldPhyStatus,
         PhyStatusType newPhyStatus,
         clocktype receiveDuration);

void MacDot16Ipv6ToMacAddress(unsigned char* macAddr, in6_addr addr);
void MacDot16MacAddressToIpv6(in6_addr addr, unsigned char* macAddr);

// /**
// FUNCTION       :: MacDot16HandleFirstFragDataPdu
// DESCRIPTION :: Handle first fargment PDu
// **/
int MacDot16HandleFirstFragDataPdu(
    Node* node,
    MacDataDot16* dot16,
    Message* msg,
    MacDot16ServiceFlow* sFlow,
    UInt16 firstMsgFSN);

// /**
// FUNCTION       :: MacDot16HandleLastFragDataPdu
// DESCRIPTION :: Handle last fargment PDu
// **/
int MacDot16HandleLastFragDataPdu(
    Node* node,
    MacDataDot16* dot16,
    Message* msg,
    MacDot16ServiceFlow* sFlow,
    UInt16 lastMsgFSN,
    unsigned char* lastHopMacAddr);

// /**
// FUNCTION       :: MacDot16ARQSetRetryAndBlockLifetimeTimer
// DESCRIPTION :: ARQ set Retry and Block life time timer
// **/
void  MacDot16ARQSetRetryAndBlockLifetimeTimer(Node* node,
                                               MacDataDot16* dot16,
                                               MacDot16ServiceFlow* sFlow,
                                               char* infoPtr,
                                               UInt16 numArqBlocks ,
                                               UInt16 bsnId);

// /**
// FUNCTION       :: MacDot16ARQHandleTimeout
// DESCRIPTION :: ARQ handle time out timer message
// **/
void MacDot16ARQHandleTimeout(Node* node,
                              MacDataDot16* dot16,
                              MacDot16Timer* timerInfo);

// /**
// FUNCTION       :: MacDot16ARQFindBSNInWindow
// DESCRIPTION :: ARQ find BSN in window
// **/
int MacDot16ARQFindBSNInWindow(MacDot16ServiceFlow* sFlow,
                               Int16 incomingBsnId,
                               UInt16 numArqBlocks,
                               BOOL &isPresent,
                               UInt16 &numUnACKArqBloacks);

// /**
// FUNCTION       :: MacDot16ARQHandleResetMsg
// DESCRIPTION :: ARQ handle reset message
// **/
int MacDot16ARQHandleResetMsg(Node* node,
                              MacDataDot16* dot16,
                              unsigned char* macFrame,
                              int startIndex);

// /**
// FUNCTION       :: MacDot16ARQConvertParam
// DESCRIPTION :: ARQ convert parameter in to frame duration
// **/
void MacDot16ARQConvertParam(MacDot16ARQParam *arqParam,
                             clocktype frameDuration);

// /**
// FUNCTION       :: MacDot16ARQResetWindow
// DESCRIPTION :: ARQ reset window
// **/
void MacDot16ARQResetWindow(Node* node,
                            MacDataDot16* dot16,
                            MacDot16ServiceFlow* sFlow);

// /**
// FUNCTION       :: MacDot16ARQCalculateBwReq
// DESCRIPTION :: ARQ calculate bandwidth request
// **/
int MacDot16ARQCalculateBwReq(MacDot16ServiceFlow* sFlow,
                              UInt16 crcSize);

// /**
// FUNCTION       :: MacDot16SkipPdu
// DESCRIPTION :: Skip PDu from the message list
// **/
Message* MacDot16SkipPdu(Message* pdu);

// /**
// FUNCTION       :: MacDot16ARQBuildandSendResetMsg
// DESCRIPTION :: ARQ build and send reset message
// **/
void MacDot16ARQBuildandSendResetMsg(Node* node,
                                   MacDataDot16* dot16,
                                   UInt16 cid,
                                   UInt8 resetMsgFlag);

// /**
// FUNCTION       :: MacDot16ARQHandleResetRetryTimeOut
// DESCRIPTION :: ARQ handle Reset retry time out timer
// **/
void MacDot16ARQHandleResetRetryTimeOut(Node* node,
                                        MacDataDot16* dot16,
                                        UInt16 cid,
                                        UInt8 resetMsgType);

// /**
// FUNCTION       :: MacDot16ResetARQBlockPtr
// DESCRIPTION :: ARQ reset ARQ block pointer
// **/
void MacDot16ResetARQBlockPtr(Node* node,
                              MacDot16ARQBlock* arqBlockPtr,
                              BOOL flag);

// /**
// FUNCTION       :: MacDot16eGetPSClassTypebyServiceType
// DESCRIPTION :: Return PS class type as per the service type
// **/
MacDot16ePSClassType MacDot16eGetPSClassTypebyServiceType(int serviceType);

#endif // MAC_DOT16_H
