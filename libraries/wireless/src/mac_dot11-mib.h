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

/*!
 * \file mac_dot11-mib.h
 * \brief MIB structures.
 */

#ifndef MAC_DOT11_MIB_H
#define MAC_DOT11_MIB_H

#include "api.h"
#include "mac_dot11.h"

//-------------------------------------------------------------------------
// #define's
//-------------------------------------------------------------------------
#define DOT11_DEFAULT_SSID                "NONE"
#define DOT11_SSID_MAX_LENGTH             32

//--------------------------------------------------------------------------
// 802.11 MIBs
//--------------------------------------------------------------------------
// Poll save mode
typedef enum {
    DOT11_PM_ACTIVE = 1,
    DOT11_PM_POWERSAVE
} DOT11_PowerManagmentModes;

typedef enum {
    DOT11_BBS_INDEPENDENT = 1,
    DOT11_BBS_BSS,
    DOT11_BBS_BOTH
} DOT11_BBSTypes;

// 802.11 station config table (MIB)
typedef struct {
    Mac802Address                 dot11StationID;
    unsigned int                dot11MediumOccupancyLimit;
    unsigned int                dot11CFPollable;
    unsigned int                dot11CFPPeriod;
    unsigned int                dot11MaxDuration;
    unsigned int                dot11AuthenticationResponseTimeout;
    unsigned int                dot11PrivacyOptionImplemented;
    DOT11_PowerManagmentModes   dot11PowerManagementMode;
    char                        dot11DesiredSSID[DOT11_SSID_MAX_LENGTH];
    DOT11_BBSTypes              dot11DesiredBSSType;
    char                        dot11OperationalRateSet[126];
    clocktype                   dot11BeaconPeriod;
    unsigned int                dot11DTIMPeriod;
    unsigned int                dot11AssociationResponseTimeout;
    unsigned int                dot11DisassociateReason;
    unsigned int                dot11DeauthenticateReason;
    Mac802Address                 dot11DisassociateStation;
    Mac802Address                 dot11DeauthenticateStation;
    unsigned int                dot11AuthenticateFailStatus;
    Mac802Address                 dot11AuthenticateFailStation;

//---------------------DOT11e--Updates------------------------------------//
    unsigned int                dot11QosOptionImplemented;
    unsigned int                dot11ImmediateBlockAckOptionImplemented;
    unsigned int                dot11DelayedBlockAckOptionImplemented;
    unsigned int                dot11DirectOptionImplemented;
    unsigned int                dot11APSDOptionImplemented;
    unsigned int                dot11QAckOptionImplemented;
    unsigned int                dot11QBSSLoadOptionImplemented;
    unsigned int                dot11QueueRequestOptionImpelemented;
    unsigned int                dot11TXOPRequestOptionImplemented;
    unsigned int                dot11MoreDataAckOptionImpelemented;
    unsigned int                dot11AssociateinNQBSS;
    unsigned int                dot11DLSAllowedinQBSS;
    unsigned int                dot11DLSAllowed;
//--------------------DOT11e-End-Updates---------------------------------//

} DOT11_StationConfigTable;

//---------------------DOT11e--Updates------------------------------------//

// /**
// STRUCT      :: DOT11_EDCATable
// DESCRIPTION :: EDCA parameters table
// **/
typedef struct {
    unsigned int dot11EDCATableIndex;
    unsigned int dot11EDCATableCWmin;
    unsigned int dot11EDCATableCWmax;
    unsigned int dot11EDCATableAIFSN;
    unsigned int dot11EDCATableTXOPLimit;
    unsigned int dot11EDCATableMSDULIfetime;
    unsigned int dot11EDCATableMandatory;

} DOT11_EDCATable;

//--------------------DOT11e-End-Updates---------------------------------//

typedef enum {
    DOT11_AUTH_OPEN_SYSTEM,
    DOT11_AUTH_SHARED_KEY
} DOT11_AuthAlgorithmType;


// 802.11 Authentication Algorithm table (MIB)
typedef struct {
    DOT11_AuthAlgorithmType     dot11AuthenticationAlgorithm;
    unsigned int                dot11AuthenticationAlgorithmEnabled;

} DOT11_AuthenticationAlgorithmTable;


typedef struct {
    Mac802Address                 dot11MACAddress;
    unsigned int                dot11RTSThreshold;
    unsigned int                dot11ShortRetryLimit;
    unsigned int                dot11LongRetryLimit;
    unsigned int                dot11FragmentationThreshold;
    unsigned int                dot11MaxTransmitMSDULifetime;
    unsigned int                dot11MaxReceiverLifetime;

} DOT11_OperationTable;

typedef struct {
    unsigned int                dot11TransmittedFragmentCount;
    unsigned int                dot11MulticastTransmittedFrameCount;
    unsigned int                dot11FailedCount;
    unsigned int                dot11RetryCount;
    unsigned int                dot11MultipleRetryCount;
    unsigned int                dot11FrameDuplicateCount;
    unsigned int                dot11RTSSuccessCount;
    unsigned int                dot11RTSFailureCount;
    unsigned int                dot11ACKFailureCount;
    unsigned int                dot11ReceivedFragmentCount;
    unsigned int                dot11MulticastReceivedFrameCount;
    unsigned int                dot11FCSErrorCount;
    unsigned int                dot11TransmittedFrameCount;
    unsigned int                dot11WEPUndecryptableCount;

    unsigned int                dot11QosDiscardedFragmentCount;
    unsigned int                dot11AssociatedStationCount;
    unsigned int                dot11QosCFPollsReceivedCount;
    unsigned int                dot11QosCFPollsUnusedCount;
    unsigned int                dot11QosCFPollsUnusableCount;

} DOT11_CountersTable;


typedef struct {
    unsigned int    dot11QosCountersIndex;
    unsigned int    dot11QosTransmittedFragmentCount;
    unsigned int    dot11QosFailedCount;
    unsigned int    dot11QosRetryCount;
    unsigned int    dot11QosMultipleRetryCount;
    unsigned int    dot11QosFrameDuplicateCount;
    unsigned int    dot11QosRTSSuccessCount;
    unsigned int    dot11QosRTSFailureCount;
    unsigned int    dot11QosAckFailureCount;
    unsigned int    dot11QosReceivedFragmentCount;
    unsigned int    dot11QosTransmittedFrameCount;
} Dot11_QosCounters;


//--------------------------------------------------------------------------
// FUNCTIONS
//--------------------------------------------------------------------------

#endif /*MAC_DOT11_MIB_H*/
