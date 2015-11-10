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

#ifndef MAC_802_15_4_TRANSAC_H
#define MAC_802_15_4_TRANSAC_H

#include "mac_802_15_4.h"

// /**
// CONSTANT    :: M802_15_4_MAXNUMTRANSACTIONS : 70
// DESCRIPTION :: The maximum number of transactions that are allowed
// **/
#define M802_15_4_MAXNUMTRANSACTIONS  70

#define M802_15_4_MAXTIMESTANSACTIONEXPIRED 5

//--------------------------------------------------------------------------
// typedefs
//--------------------------------------------------------------------------



//--------------------------------------------------------------------------
// DEVICE LINK FUNCTIONS
//--------------------------------------------------------------------------

// /**
// FUNCTION   :: Mac802_15_4DevLinkInit
// LAYER      :: Mac
// PURPOSE    :: To initialize device link structure.
// PARAMETERS ::
// + devLink    : M802_15_4DEVLINK*     : Device Link structure
// + addr       : MACADDR            : MAC address
// + cap        : UInt8                 : Device capabilities
// RETURN  :: None.
// **/
static
void Mac802_15_4DevLinkInit(M802_15_4DEVLINK** devLink,
                            MACADDR addr,
                            UInt8 cap)
{
    *devLink = (M802_15_4DEVLINK*) MEM_malloc(sizeof(M802_15_4DEVLINK));
    (*devLink)->addr64 = addr;
    (*devLink)->addr16 = (UInt16)addr;
    (*devLink)->capability = cap;
    (*devLink)->numTimesTrnsExpired = 0;
    (*devLink)->last = NULL;
    (*devLink)->next = NULL;
};


// /**
// FUNCTION   :: Mac802_15_4DevLinkInit
// LAYER      :: Mac
// PURPOSE    :: To initialize device link structure.
// PARAMETERS ::
// + devLink    : M802_15_4DEVLINK*     : Device Link structure
// + addr       : MACADDR            : MAC address
// + cap        : UInt8                 : Device capabilities
// RETURN     :: Int32
// **/
Int32 Mac802_15_4AddDeviceLink(M802_15_4DEVLINK** deviceLink1,
                               M802_15_4DEVLINK** deviceLink2,
                               MACADDR addr,
                               UInt8 cap);


// /**
// FUNCTION   :: Mac802_15_4UpdateDeviceLink
// LAYER      :: Mac
// PURPOSE    ::
// PARAMETERS ::
// + oper       : Int32                   : Operation
// + deviceLink1: M802_15_4DEVLINK*     : Device Link structure
// + deviceLink2: M802_15_4DEVLINK*     : Device Link structure
// + addr       : MACADDR            : MAC address
// + device ...: M802_15_4DEVLINK** : Double pointer to the
//                                                         device link structure
// RETURN  :: Int32
// **/
Int32 Mac802_15_4UpdateDeviceLink(Int32 oper,
                                  M802_15_4DEVLINK** deviceLink1,
                                  M802_15_4DEVLINK** deviceLink2,
                                  MACADDR addr,
                                  M802_15_4DEVLINK** device = NULL);


// /**
// FUNCTION   :: Mac802_15_4NumberDeviceLink
// LAYER      :: Mac
// PURPOSE    ::
// PARAMETERS ::
// + deviceLink1: M802_15_4DEVLINK*     : Device Link structure
// RETURN  :: Int32
// **/
Int32 Mac802_15_4NumberDeviceLink(M802_15_4DEVLINK** deviceLink1);

// /**
// FUNCTION   :: Mac802_15_4ChkAddDeviceLink
// LAYER      :: Mac
// PURPOSE    ::
// PARAMETERS ::
// + deviceLink1: M802_15_4DEVLINK*     : Device Link structure
// + deviceLink2: M802_15_4DEVLINK*     : Device Link structure
// + addr       : MACADDR            : MAC address
// + cap        : UInt8                 : Device capabilities
// RETURN  :: Int32
// **/
Int32 Mac802_15_4ChkAddDeviceLink(M802_15_4DEVLINK** deviceLink1,
                                  M802_15_4DEVLINK** deviceLink2,
                                  MACADDR addr,
                                  UInt8 cap);

// /**
// FUNCTION   :: Mac802_15_4EmptyDeviceLink
// LAYER      :: Mac
// PURPOSE    ::
// PARAMETERS ::
// + deviceLink1: M802_15_4DEVLINK*     : Device Link structure
// + deviceLink2: M802_15_4DEVLINK*     : Device Link structure
// RETURN  :: None
// **/
void Mac802_15_4EmptyDeviceLink(M802_15_4DEVLINK** deviceLink1,
                                M802_15_4DEVLINK** deviceLink2);

// /**
// FUNCTION   :: Mac802_15_4DumpDeviceLink
// LAYER      :: Mac
// PURPOSE    ::
// PARAMETERS ::
// + deviceLink1: M802_15_4DEVLINK*     : Device Link structure
// + coorAddr   : MACADDR            : MAC address of Coordinator
// RETURN     :: None
// **/
void Mac802_15_4DumpDeviceLink(M802_15_4DEVLINK* deviceLink1,
                               MACADDR coorAddr);


//--------------------------------------------------------------------------
// TRANSACTION LINK FUNCTIONS
//--------------------------------------------------------------------------


// /**
// FUNCTION   :: Mac802_15_4TransLink
// LAYER      :: Mac
// PURPOSE    :: To initialize transaction link structure.
// PARAMETERS ::
// + transLink  : M802_15_4TRANSLINK*   : Transaction Link structure
// + pendAM     : UInt8                 :
// + pendAddr   : MACADDR            : MAC address
// + p          : Message*              : Message
// + msduH      : UInt8                 :
// + kpTime     : clocktype             :
// RETURN  :: None.
// **/
static
void Mac802_15_4TransLink(Node* node,
                          M802_15_4TRANSLINK** translnk,
                          UInt8 pendAM,
                          MACADDR pendAddr,
                          Message* p,
                          UInt8 msduH,
                          clocktype kpTime)
{
    *translnk = (M802_15_4TRANSLINK*) MEM_malloc(sizeof(M802_15_4TRANSLINK));

    M802_15_4TRANSLINK* transLink = *translnk;
    transLink->pendAddrMode = pendAM;
    transLink->pendAddr64 = pendAddr;
    transLink->pkt = p;
    transLink->msduHandle = msduH;
    transLink->expTime = node->getNodeTime() + kpTime;
    transLink->last = NULL;
    transLink->next = NULL;
};

// /**
// FUNCTION   :: Mac802_15_4PurgeTransacLink
// LAYER      :: Mac
// PURPOSE    :: To initialize transaction link structure.
// PARAMETERS ::
// + node           : Node*                  : Node pointer
// + transacLink1   : M802_15_4TRANSLINK**   : Transaction Link structure
// + transacLink2   : M802_15_4TRANSLINK**   : Transaction Link structure
// RETURN  :: None.
// **/
void Mac802_15_4PurgeTransacLink(Node* node,
                                 M802_15_4TRANSLINK** transacLink1,
                                 M802_15_4TRANSLINK** transacLink2);

// /**
// FUNCTION   :: Mac802_15_4AddTransacLink
// LAYER      :: Mac
// PURPOSE    ::
// PARAMETERS ::
// + transacLink1   : M802_15_4TRANSLINK**   : Transaction Link structure
// + transacLink2   : M802_15_4TRANSLINK**   : Transaction Link structure
// + pendAM         : UInt8                  :
// + pendAddr       : MACADDR             : MAC address
// + p              : Message*               : Message
// + msduH          : UInt8                  :
// + kpTime         : clocktype              :
// RETURN  :: Int32
// **/
Int32 Mac802_15_4AddTransacLink(Node* node,
                                M802_15_4TRANSLINK **transacLink1,
                                M802_15_4TRANSLINK **transacLink2,
                                UInt8 pendAM,
                                MACADDR pendAddr,
                                Message* p,
                                UInt8 msduH,
                                clocktype kpTime);

// /**
// FUNCTION   :: Mac802_15_4GetPktFrTransacLink
// LAYER      :: Mac
// PURPOSE    ::
// PARAMETERS ::
// + transacLink1   : M802_15_4TRANSLINK**   : Transaction Link structure
// + pendAM         : UInt8                  :
// + pendAddr       : MACADDR             : MAC address
// RETURN  :: Message*
// **/
Message* Mac802_15_4GetPktFrTransacLink(M802_15_4TRANSLINK** transacLink1,
                                        UInt8 pendAM,
                                        MACADDR pendAddr);

// /**
// FUNCTION   :: Mac802_15_4UpdateTransacLink
// LAYER      :: Mac
// PURPOSE    ::
// PARAMETERS ::
// + node           : Node*                  : Node pointer
// + oper           : Int32                    : Operation
// + transacLink1   : M802_15_4TRANSLINK**   : Transaction Link structure
// + transacLink2   : M802_15_4TRANSLINK**   : Transaction Link structure
// + pendAM         : UInt8                  :
// + pendAddr       : MACADDR             : MAC address
// RETURN  :: Int32
// **/
Int32 Mac802_15_4UpdateTransacLink(Node* node,
                                   Int32 oper,
                                   M802_15_4TRANSLINK** transacLink1,
                                   M802_15_4TRANSLINK** transacLink2,
                                   UInt8 pendAM,
                                   MACADDR pendAddr);

// /**
// FUNCTION   :: Mac802_15_4UpdateTransacLinkByPktOrHandle
// LAYER      :: Mac
// PURPOSE    ::
// PARAMETERS ::
// + node           : Node*                  : Node pointer
// + oper           : Int32                    : Operation
// + transacLink1   : M802_15_4TRANSLINK**   : Transaction Link structure
// + transacLink2   : M802_15_4TRANSLINK**   : Transaction Link structure
// + p              : Message*               : Message
// + msduH          : UInt8                  :
// RETURN  :: Int32
// **/
Int32 Mac802_15_4UpdateTransacLinkByPktOrHandle(Node* node,
                                                Int32 oper,
                                                M802_15_4TRANSLINK** transacLink1,
                                                M802_15_4TRANSLINK** transacLink2,
                                                Message* pkt,
                                                UInt8 msduH);

// /**
// FUNCTION   :: Mac802_15_4NumberTransacLink
// LAYER      :: Mac
// PURPOSE    ::
// PARAMETERS ::
// + node           : Node*                  : Node pointer
// + transacLink1   : M802_15_4TRANSLINK**   : Transaction Link structure
// + transacLink2   : M802_15_4TRANSLINK**   : Transaction Link structure
// RETURN  :: Int32
// **/
Int32 Mac802_15_4NumberTransacLink(Node* node,
                                   M802_15_4TRANSLINK** transacLink1,
                                   M802_15_4TRANSLINK** transacLink2);

// /**
// FUNCTION   :: Mac802_15_4ChkAddTransacLink
// LAYER      :: Mac
// PURPOSE    ::
// PARAMETERS ::
// + node           : Node*                  : Node pointer
// + transacLink1   : M802_15_4TRANSLINK**   : Transaction Link structure
// + transacLink2   : M802_15_4TRANSLINK**   : Transaction Link structure
// + pendAM         : UInt8                  :
// + pendAddr       : MACADDR             : MAC address
// + p              : Message*               : Message
// + msduH          : UInt8                  :
// + kpTime         : clocktype                 :
// RETURN  :: Int32
// **/
Int32 Mac802_15_4ChkAddTransacLink(Node* node,
                                   M802_15_4TRANSLINK** transacLink1,
                                   M802_15_4TRANSLINK** transacLink2,
                                   UInt8 pendAM,
                                   MACADDR pendAddr,
                                   Message* p,
                                   UInt8 msduH,
                                   clocktype kpTime);

// /**
// FUNCTION   :: Mac802_15_4EmptyTransacLink
// LAYER      :: Mac
// PURPOSE    ::
// PARAMETERS ::
// + node           : Node*                  : Node pointer
// + transacLink1   : M802_15_4TRANSLINK**   : Transaction Link structure
// + transacLink2   : M802_15_4TRANSLINK**   : Transaction Link structure
// RETURN  :: None
// **/
void Mac802_15_4EmptyTransacLink(Node* node,
                                 M802_15_4TRANSLINK** transacLink1,
                                 M802_15_4TRANSLINK** transacLink2);

// /**
// FUNCTION   :: Mac802_15_4DumpTransacLink
// LAYER      :: Mac
// PURPOSE    ::
// PARAMETERS ::
// + transacLink1   : M802_15_4TRANSLINK*    : Transaction Link structure
// + coorAddr       : MACADDR             : MAC address of Coordinator
// RETURN  :: None
// **/
void Mac802_15_4DumpTransacLink(M802_15_4TRANSLINK* transacLink1,
                                MACADDR coorAddr);

#endif /*MAC_802_15_4_TRANSAC_H*/
