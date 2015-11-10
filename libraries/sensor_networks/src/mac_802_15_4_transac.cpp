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


#include "mac_802_15_4_transac.h"

// /**
// FUNCTION   :: Mac802_15_4DevLinkInit
// LAYER      :: Mac
// PURPOSE    :: To initialize device link structure.
// PARAMETERS ::
// + devLink    : M802_15_4DEVLINK*     : Device Link structure
// + addr       : MACADDR            : MAC address
// + cap        : UInt8                 : Device capabilities
// RETURN     :: Int32.
// **/
Int32 Mac802_15_4AddDeviceLink(M802_15_4DEVLINK** deviceLink1,
                               M802_15_4DEVLINK** deviceLink2,
                               MACADDR addr,
                               UInt8 cap)
{
    M802_15_4DEVLINK* tmp = NULL;
    if (*deviceLink2 == NULL)        // not exist yet
    {
        Mac802_15_4DevLinkInit(deviceLink2, addr,cap);
        if (*deviceLink2 == NULL)
        {
            return 1;
        }
        *deviceLink1 = *deviceLink2;
    }
    else
    {
        Mac802_15_4DevLinkInit(&tmp, addr,cap);
        if (tmp == NULL)
        {
            return 1;
        }
        tmp->last = *deviceLink2;
        (*deviceLink2)->next = tmp;
        *deviceLink2 = tmp;
    }
    return 0;
}


// /**
// FUNCTION   :: Mac802_15_4UpdateDeviceLink
// LAYER      :: Mac
// PURPOSE    :: Deletes or update device link at the node
// PARAMETERS ::
// + oper       : Int32                   : Operation
// + deviceLink1: M802_15_4DEVLINK*     : Device Link structure
// + deviceLink2: M802_15_4DEVLINK*     : Device Link structure
// + addr       : MACADDR            : MAC address
// + device ...: M802_15_4DEVLINK** : Double pointer to the
//                                    device link structure
// RETURN  :: Int32
// **/
Int32 Mac802_15_4UpdateDeviceLink(Int32 oper,
                                  M802_15_4DEVLINK** deviceLink1,
                                  M802_15_4DEVLINK** deviceLink2,
                                  MACADDR addr,
                                  M802_15_4DEVLINK** device)
{
    M802_15_4DEVLINK* tmp = NULL;
    Int32 rt = 1;
    tmp = *deviceLink1;
    while (tmp != NULL)
    {
        if (tmp->addr64 == addr)
        {
            if (oper == 1)    // delete an element
            {
                if (tmp->last != NULL)
                {
                    tmp->last->next = tmp->next;
                    if (tmp->next != NULL)
                    {
                        tmp->next->last = tmp->last;
                    }
                    else
                    {
                        *deviceLink2 = tmp->last;
                    }
                }
                else if (tmp->next != NULL)
                {
                    *deviceLink1 = tmp->next;
                    tmp->next->last = NULL;
                }
                else
                {
                    *deviceLink1 = NULL;
                    *deviceLink2 = NULL;
                }
                MEM_free((void *)tmp);
            }
            else if (oper == 3)
            {
                *device = tmp;
            }
            rt = 0;
            break;
        }
        tmp = tmp->next;
    }
    return rt;
}


// /**
// FUNCTION   :: Mac802_15_4NumberDeviceLink
// LAYER      :: Mac
// PURPOSE    :: Returns number of current device links at the node
// PARAMETERS ::
// + deviceLink1: M802_15_4DEVLINK*     : Device Link structure
// RETURN  :: Int32
// **/
Int32 Mac802_15_4NumberDeviceLink(M802_15_4DEVLINK** deviceLink1)
{
    M802_15_4DEVLINK* tmp = NULL;
    Int32 num = 0;
    tmp = *deviceLink1;
    while (tmp != NULL)
    {
        num++;
        tmp = tmp->next;
    }
    return num;
}

// /**
// FUNCTION   :: Mac802_15_4ChkAddDeviceLink
// LAYER      :: Mac
// PURPOSE    :: Adds device link to the node
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
                                  UInt8 cap)
{
    Int32 i = 0;
    i = Mac802_15_4UpdateDeviceLink(OPER_CHECK_DEVICE_REFERENCE,
                                    deviceLink1,
                                    deviceLink2,
                                    addr);
    if (i == 0)
    {
        return 1;
    }

    i = Mac802_15_4AddDeviceLink(deviceLink1, deviceLink2, addr, cap);
    if (i == 0)
    {
        return 0;
    }
    else
    {
        return 2;
    }

}

// /**
// FUNCTION   :: Mac802_15_4EmptyDeviceLink
// LAYER      :: Mac
// PURPOSE    :: Deletes all device links
// PARAMETERS ::
// + deviceLink1: M802_15_4DEVLINK*     : Device Link structure
// + deviceLink2: M802_15_4DEVLINK*     : Device Link structure
// RETURN  :: None
// **/
void Mac802_15_4EmptyDeviceLink(M802_15_4DEVLINK** deviceLink1,
                                M802_15_4DEVLINK** deviceLink2)
{
    M802_15_4DEVLINK* tmp = NULL;
    M802_15_4DEVLINK* tmp2 = NULL;

    if (*deviceLink1 != NULL)
    {
        tmp = *deviceLink1;
        while (tmp != NULL)
        {
            tmp2 = tmp;
            tmp = tmp->next;
            MEM_free((void*)tmp2);
        }
        *deviceLink1 = NULL;
    }
    *deviceLink2 = *deviceLink1;
}


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
                                 M802_15_4TRANSLINK** transacLink2)
{
    // purge expired transactions
    M802_15_4TRANSLINK* tmp = NULL;
    M802_15_4TRANSLINK* tmp2 = NULL;

    tmp = *transacLink1;
    while (tmp != NULL)
    {
        if (node->getNodeTime() > tmp->expTime)
        {
            tmp2 = tmp;
            if (tmp->next != NULL)
            {
                tmp = tmp->next->next;
            }
            else
            {
                tmp = NULL;
            }
            // --- delete the transaction ---
            // don't try to call updateTransacLink() -- to avoid re-entries of
            // functions
            if (tmp2->last != NULL)
            {
                tmp2->last->next = tmp2->next;
                if (tmp2->next != NULL)
                {
                    tmp2->next->last = tmp2->last;
                }
                else
                {
                    *transacLink2 = tmp2->last;
                }
            }
            else if (tmp2->next != NULL)
            {
                *transacLink1 = tmp2->next;
                tmp2->next->last = NULL;
            }
            else
            {
                *transacLink1 = NULL;
                *transacLink2 = NULL;
            }
            // free the packet first
            MESSAGE_Free(node, tmp2->pkt);
            MEM_free((void*)tmp2);
            // --------------------------------
        }
        else
        {
            tmp = tmp->next;
        }
    }
}

// /**
// FUNCTION   :: Mac802_15_4AddTransacLink
// LAYER      :: Mac
// PURPOSE    :: Adds transaction link
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
                                M802_15_4TRANSLINK** transacLink1,
                                M802_15_4TRANSLINK** transacLink2,
                                UInt8 pendAM,
                                MACADDR pendAddr,
                                Message* p,
                                UInt8 msduH,
                                clocktype kpTime)
{
    M802_15_4TRANSLINK* tmp = NULL;
    if (*transacLink2 == NULL)       // not exist yet
    {
        Mac802_15_4TransLink(node,
                             transacLink2,
                              pendAM,
                              pendAddr,
                              p,
                              msduH,
                              kpTime);
        if (*transacLink2 == NULL)
        {
            return 1;
        }
        *transacLink1 = *transacLink2;
    }
    else
    {
        Mac802_15_4TransLink(node,
                             &tmp,
                              pendAM,
                              pendAddr,
                              p,
                              msduH,
                              kpTime);
        if (tmp == NULL)
        {
            return 1;
        }
        tmp->last = *transacLink2;
        (*transacLink2)->next = tmp;
        *transacLink2 = tmp;
    }
    return 0;
}

// /**
// FUNCTION   :: Mac802_15_4GetPktFrTransacLink
// LAYER      :: Mac
// PURPOSE    :: Returns a pending packet from transaction link
// PARAMETERS ::
// + transacLink1   : M802_15_4TRANSLINK**   : Transaction Link structure
// + pendAM         : UInt8                  :
// + pendAddr       : MACADDR             : MAC address
// RETURN  :: Message*
// **/
Message* Mac802_15_4GetPktFrTransacLink(M802_15_4TRANSLINK** transacLink1,
                                        UInt8 pendAM,
                                        MACADDR pendAddr)
{
    M802_15_4TRANSLINK* tmp = NULL;

    tmp = *transacLink1;

    while (tmp != NULL)
    {
        if ((tmp->pendAddrMode == pendAM)
            && (((pendAM == M802_15_4DEFFRMCTRL_ADDRMODE16)
            && ((UInt16)pendAddr == tmp->pendAddr16))
            || ((pendAM == M802_15_4DEFFRMCTRL_ADDRMODE64)
            && (pendAddr == tmp->pendAddr64))))
        {
            return tmp->pkt;
        }
        tmp = tmp->next;
    }
    return NULL;
}

// /**
// FUNCTION   :: Mac802_15_4UpdateTransacLink
// LAYER      :: Mac
// PURPOSE    :: Deletes or updates transaction link
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
                                   MACADDR pendAddr)
{
    M802_15_4TRANSLINK* tmp = NULL;
    Int32 rt = 0;
    // purge first if (oper == tr_oper_est)
    if (oper == 2)
    {
        Mac802_15_4PurgeTransacLink(node, transacLink1, transacLink2);
    }

    rt = 1;

    tmp = *transacLink1;

    if (oper == 3)
    {
        while (tmp != NULL)
        {
            if ((tmp->pendAddrMode == pendAM)
                && (((pendAM == M802_15_4DEFFRMCTRL_ADDRMODE16)
                && ((UInt16)pendAddr == tmp->pendAddr16))
                || ((pendAM == M802_15_4DEFFRMCTRL_ADDRMODE64)
                && (pendAddr == tmp->pendAddr64))))
            {
                rt = 0;
                tmp = tmp->next;
                break;
            }
            tmp = tmp->next;
        }
        if (rt)
        {
            return 1;
        }
    }

    rt = 1;

    while (tmp != NULL)
    {
        if ((tmp->pendAddrMode == pendAM)
            && (((pendAM == M802_15_4DEFFRMCTRL_ADDRMODE16)
            && ((UInt16)pendAddr == tmp->pendAddr16))
            || ((pendAM == M802_15_4DEFFRMCTRL_ADDRMODE64)
            && (pendAddr == tmp->pendAddr64))))
        {
            if (oper == 1)    // delete an element
            {
                if (tmp->last != NULL)
                {
                    tmp->last->next = tmp->next;
                    if (tmp->next != NULL)
                    {
                        tmp->next->last = tmp->last;
                    }
                    else
                    {
                        *transacLink2 = tmp->last;
                    }
                }
                else if (tmp->next != NULL)
                {
                    *transacLink1 = tmp->next;
                    tmp->next->last = NULL;
                }
                else
                {
                    *transacLink1 = NULL;
                    *transacLink2 = NULL;
                }
                // free the packet first
                MESSAGE_Free(node, tmp->pkt);
                MEM_free((void*)tmp);
//                 Packet::free(tmp->pkt);
//                 delete tmp;
            }
            rt = 0;
            break;
        }
        tmp = tmp->next;
    }
    return rt;
}

// /**
// FUNCTION   :: Mac802_15_4UpdateTransacLinkByPktOrHandle
// LAYER      :: Mac
// PURPOSE    :: To update or delete transaction list
// PARAMETERS ::
// + node           : Node*                  : Node pointer
// + oper           : Int32                    : Operation
// + transacLink1   : M802_15_4TRANSLINK**   : Transaction Link structure
// + transacLink2   : M802_15_4TRANSLINK**   : Transaction Link structure
// + p              : Message*               : Message
// + msduH          : UInt8                  :
// RETURN  :: Int32
// **/
Int32 Mac802_15_4UpdateTransacLinkByPktOrHandle(
    Node* node,
    Int32 oper,
    M802_15_4TRANSLINK** transacLink1,
    M802_15_4TRANSLINK** transacLink2,
    Message* pkt,
    UInt8 msduH)
{
    M802_15_4TRANSLINK* tmp = NULL;
    Int32 rt = 0;

    // purge first if (oper == tr_oper_est)
    if (oper == 2)
    {
        Mac802_15_4PurgeTransacLink(node, transacLink1, transacLink2);
    }

    rt = 1;

    tmp = *transacLink1;

    while (tmp != NULL)
    {
        if (((pkt != NULL) && (tmp->pkt == pkt)) ||
             ((pkt == NULL) && (tmp->msduHandle == msduH)))
        {
            if (oper == 1)    // delete an element
            {
                if (tmp->last != NULL)
                {
                    tmp->last->next = tmp->next;
                    if (tmp->next != NULL)
                    {
                        tmp->next->last = tmp->last;
                    }
                    else
                    {
                        *transacLink2 = tmp->last;
                    }
                }
                else if (tmp->next != NULL)
                {
                    *transacLink1 = tmp->next;
                    tmp->next->last = NULL;
                }
                else
                {
                    *transacLink1 = NULL;
                    *transacLink2 = NULL;
                }
                // free the packet first
                MESSAGE_Free(node, tmp->pkt);
                MEM_free(tmp);
//                 Packet::free(tmp->pkt);
//                 delete tmp;
            }
            rt = 0;
            break;
        }
        tmp = tmp->next;
    }
    return rt;
}

// /**
// FUNCTION   :: Mac802_15_4NumberTransacLink
// LAYER      :: Mac
// PURPOSE    :: Returns number of current transaction links at the node
// PARAMETERS ::
// + node           : Node*                  : Node pointer
// + transacLink1   : M802_15_4TRANSLINK**   : Transaction Link structure
// + transacLink2   : M802_15_4TRANSLINK**   : Transaction Link structure
// RETURN  :: Int32
// **/
Int32 Mac802_15_4NumberTransacLink(Node* node,
                                   M802_15_4TRANSLINK** transacLink1,
                                   M802_15_4TRANSLINK** transacLink2)
{
    // return the number of transactions in the link
    M802_15_4TRANSLINK* tmp = NULL;
    Int32 num = 0;

    // purge first
    Mac802_15_4PurgeTransacLink(node, transacLink1, transacLink2);
    tmp = *transacLink1;
    while (tmp != NULL)
    {
        num++;
        tmp = tmp->next;
    }
    return num;
}

// /**
// FUNCTION   :: Mac802_15_4ChkAddTransacLink
// LAYER      :: Mac
// PURPOSE    :: Checks and add transaction link at the node
// PARAMETERS ::
// + node           : Node*                  : Node pointer
// + transacLink1   : M802_15_4TRANSLINK**   : Transaction Link structure
// + transacLink2   : M802_15_4TRANSLINK**   : Transaction Link structure
// + pendAM         : UInt8                  :
// + pendAddr       : MACADDR             : MAC address
// + p              : Message*               : Message
// + msduH          : UInt8                  :
// + kpTime         : clocktype              :
// RETURN  :: Int32
// **/
Int32 Mac802_15_4ChkAddTransacLink(Node* node,
                                   M802_15_4TRANSLINK** transacLink1,
                                   M802_15_4TRANSLINK** transacLink2,
                                   UInt8 pendAM,
                                   MACADDR pendAddr,
                                   Message* p,
                                   UInt8 msduH,
                                   clocktype kpTime)
{
    Int32 i = 0;
    Mac802_15_4PurgeTransacLink(node, transacLink1, transacLink2);
    i = Mac802_15_4NumberTransacLink(node, transacLink1, transacLink2);
    if (i >= M802_15_4_MAXNUMTRANSACTIONS)
    {
        return 1;
    }

    i = Mac802_15_4AddTransacLink(node,
                                  transacLink1,
                                  transacLink2,
                                  pendAM,
                                  pendAddr,
                                  p,
                                  msduH,
                                  kpTime);

    if (i == 0)
    {
        return 0;
    }
    else
    {
        return 2;
    }
}

// /**
// FUNCTION   :: Mac802_15_4EmptyTransacLink
// LAYER      :: Mac
// PURPOSE    :: Deletes transaction link list
// PARAMETERS ::
// + node           : Node*                  : Node pointer
// + transacLink1   : M802_15_4TRANSLINK**   : Transaction Link structure
// + transacLink2   : M802_15_4TRANSLINK**   : Transaction Link structure
// RETURN  :: Int32
// **/
void Mac802_15_4EmptyTransacLink(Node* node,
                                 M802_15_4TRANSLINK** transacLink1,
                                 M802_15_4TRANSLINK** transacLink2)
{
    M802_15_4TRANSLINK* tmp = NULL;
    M802_15_4TRANSLINK* tmp2 = NULL;

    if (*transacLink1 != NULL)
    {
        tmp = *transacLink1;
        while (tmp != NULL)
        {
            tmp2 = tmp;
            tmp = tmp->next;
            // free the packet first
            MESSAGE_Free(node, tmp2->pkt);
            MEM_free(tmp2);
        }
        *transacLink1 = NULL;
    }
    *transacLink2 = *transacLink1;
}
