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
// PACKAGE     :: UMTS_QOS
// DESCRIPTION :: Defines the data structures used in the UMTS Model
//                Most of the time the data structures and functions start 
//                with UmtsQoS**         
// **/

#ifndef _UMTS_QOS_H_
#define _UMTS_QOS_H_

const UInt64 UMTS_UE_MAXBUFSIZE_PERFLOW =  50000;
const UInt64 UMTS_GGSN_MAXBUFSIZE_PERFLOW = 50000;
const UInt64 UMTS_GGSN_MAXBUFSIZE  = UMTS_GGSN_MAXBUFSIZE_PERFLOW * 1000;

////////////////////////////////////////////////////////////////////////////
// MACROS
///////////////////////////////////////////////////////////////////////////
// /**
// ENUM:: UmtsQoSTrafficClass
// DESCRIPTION :: UMTS Traffic Class
// REFERENCE   :: 3GGP TS 25.993
// **/ 
typedef enum
{
    // Conversational Class
    // conversational RT
    // Preserve time relation (variation) between 
    // information entities of the stream
    // Conversational pattern (stringent and low delay)
    // Eg. Speech & Video
    UMTS_QOS_CLASS_CONVERSATIONAL = 1,   

    // Streaming class
    // streaming RT
    // Preserve time relation (variation) between 
    // information entities of the stream 
    // (i.e. some but constant delay)
    // Eg. facsimile (NT)streaming audio and video
    UMTS_QOS_CLASS_STREAMING,

    // Interactive class
    // Interactive best effort
    // Request response pattern 
    // Preserve payload content
    // Eg. Web browsing
    UMTS_QOS_CLASS_INTERACTIVE_3, // handling pirority 3
    UMTS_QOS_CLASS_INTERACTIVE_2, // handling pirority 2
    UMTS_QOS_CLASS_INTERACTIVE_1, // handling pirority 1

    // Background
    // Background best effort
    // Destination is not expecting the data within a certain time
    // Preserve payload content
    // Eg.background download of  emails
    UMTS_QOS_CLASS_BACKGROUND
}UmtsQoSTrafficClass;

// /**
// ENUM        :: UmtsQosHandlingPrior
// DESCRIPTION :: UMTS QOS traffic handling priority
// **/
typedef enum
{
    UMTS_QOS_HANDLING1 = 1,
    UMTS_QOS_HANDLING2 = 2,
    UMTS_QOS_HANDLING3 = 3
} UmtsQosHandlingPrior;

// /**
// CONSTANT    :: UMTS_QOS_PEAK_TPUT
// DESCRIPTION :: UMTS QOS peak throughput  (up to  x octet/s)
// **/
// The value of corresponding field in QOS IE should be set as 
// array index + 1
const int UMTS_QOS_PEAK_TPUT[ ] = { 1000, 2000, 4000, 8000, 16000,
                                   32000, 64000, 128000, 256000 };
    
// /**
// CONSTANT    :: UMTS_QOS_MEAN_TPUT
// DESCRIPTION :: UMTS QOS mean throughput  (up to  x octet/h)
// **/
const int UMTS_QOS_MEAN_TPUT[ ] = { 100, 200, 500, 1000, 2000, 5000,
                                   10000, 20000, 50000, 100000, 200000, 
                                   500000, 1000000, 2000000, 5000000,
                                   10000000, 20000000, 50000000 };
const int UMTS_QOS_MEAN_TPUT_BESTEFFORT = 31;

// /**
// STRUCT      :: UmtsSmIeQos
// DESCRIPTION :: SM signalling IE :: Quality of service
// **/
struct UmtsSmIeQos
{
    UInt8 iei;
    UInt8 len;
    UInt8 val[14];
};

// /**
// STRUCT      :: UmtsRABServiceInfo
// DESCRIPTION :: RAB service infomation
//                including traffic class, bit rate, ber and delay
// **/
typedef struct
{
    UmtsQoSTrafficClass trafficClass;
    unsigned int maxBitRate;
    unsigned int guaranteedBitRate;
    unsigned int sduErrorRatio;
    double residualBER;
    clocktype transferDelay;  
}UmtsRABServiceInfo;

// /**
// STRUCT      :: UmtsFlowQoSInfo
// DESCRIPTION :: Flow QoS service infomation
//                including traffic class, bit rate, 
//                ber and delay for both UL/DL
// **/
struct UmtsFlowQoSInfo
{
    UmtsQoSTrafficClass trafficClass;
    unsigned int ulMaxBitRate;
    unsigned int ulGuaranteedBitRate;
    unsigned int dlMaxBitRate;
    unsigned int dlGuaranteedBitRate;
    unsigned int sduErrorRatio;
    double residualBER;
    clocktype transferDelay;  

    UmtsFlowQoSInfo()
    {
    }
    UmtsFlowQoSInfo(UmtsQoSTrafficClass tfClass,
                   unsigned int ulMaxRate,
                   unsigned int ulGuarRate,
                   unsigned int dlMaxRate,
                   unsigned int dlGuarRate,
                   unsigned int sduErrRatio,
                   double resBER,
                   clocktype delay)
    {
        trafficClass = tfClass;
        ulMaxBitRate = ulMaxRate;
        ulGuaranteedBitRate = ulGuarRate;
        dlMaxBitRate = dlMaxRate;
        dlGuaranteedBitRate = dlGuarRate;
        sduErrorRatio = sduErrRatio;
        residualBER = resBER;
        transferDelay = delay;
    }

};

// Could define some typical RAB services for fifferent type of class

#endif /* UMTS_QOS_H_ */
