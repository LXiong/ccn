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

// ASSUMPTION:
// 1. It has been assumed that all sites want to receive media data in the same
//    format and no application level firewall is installed. So Mixer and
//    Translator were not implemented.
//
// 2. RTCP APP types of packets are to be provided by application. As VOIP
//    application currently does not provide any APP type of packets so RTCP
//    currently can not provide any APP type of packets and hence no sending
//    and receiving routine are provided.
//
// 3. CNAME is used for Source description and is taken from terminal end
//    point file.
//
// 4. The session management bandwith is a constant one and set its
//    value to 64kbps as per the normal telephony standard.
//
// 5. We have handled only fixed RTP header. Also we have assumed that
//    CSRC List is empty.


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "api.h"
#include "app_util.h"
#include "app_voip.h"
#include "transport_rtp.h"

#ifdef ADDON_DB
#include "dbapi.h"
#endif

#define DEBUG_RTP  0
#define DEBUG_RTCP 0
#define DEBUG_RTPJITTER 0

// NTP timestamp calculation related stuff
static
clocktype RtpCalculateNtp64ToNtp32(clocktype ntpSec, clocktype ntpFrac)
{
    return (((ntpSec  & 0x0000ffff) << 16) | ((ntpFrac & 0xffff0000) >> 16));
}


static
clocktype RtpSubstractNtp32(clocktype now, clocktype then)
{
    if (now > then)
    {
        return (now - then);
    }
    return ((now - then) + 0x7fffffff);
}
//---------------------------------------------------------------------------
// NAME         : RtpMappingAddNewEntry()
// PURPOSE      : Keeps a mapping of RTP to application port
// Parameter    : RTP data structure,port to be stored in the D/B
// RETURN VALUE : void
//---------------------------------------------------------------------------
static
void RtpMappingAddNewEntry(RtpData* rtp, unsigned short localport)
{
    RtpMappingPort* rtpMap
            = (RtpMappingPort*) MEM_malloc(sizeof(RtpMappingPort));
    memset (rtpMap, 0, sizeof(RtpMappingPort));
    rtpMap->applicationPort = localport;
    rtpMap->rtpPort = rtp->rtpSession->localPort;
    if (rtp->rtpMapping != NULL)
    {
        rtpMap->next = rtp->rtpMapping;
        rtp->rtpMapping->prev = rtpMap;
    }
    rtp->rtpMapping = rtpMap;
}


//---------------------------------------------------------------------------
// NAME         : RtpGetNtpTimestampFromCurSimTime()
// PURPOSE      : Convert current time into NTP format.
// RETURN VALUE : None.
//---------------------------------------------------------------------------
static
void RtpGetNtpTimestampFromCurSimTime(Node* node,
                                      clocktype* ntpSec,
                                      clocktype* ntpFrac)
{
    clocktype now = node->getNodeTime();
    clocktype timeValInSec = 0;
    clocktype timeValInMicroSec = 0;

    // ntpFrac is in units of 1 / (2^32 - 1) secs.
    timeValInSec = (now / SECOND); // second portion of nano-second

    // micro-second portion of nano-second
    timeValInMicroSec = ((now % SECOND) * MILLI_SECOND);

    *ntpSec = timeValInSec + RTP_SECS_BETWEEN_1900_1970;
    *ntpFrac = (timeValInMicroSec << 12) +
               (timeValInMicroSec << 8)  - ((timeValInMicroSec * 3650) >> 6);
}


//---------------------------------------------------------------------------
// NAME         : RtpCheckSourceInfoInRtpDatabase()
// PURPOSE      :
// RETURN VALUE : None.
//---------------------------------------------------------------------------
static
void RtpCheckSourceInfoInRtpDatabase(RtcpSourceInfo* srcInfo)
{
    ERROR_Assert(srcInfo != NULL, "Source Information not found\n");
}


//---------------------------------------------------------------------------
// NAME         : RtpGetHashvalueBySsrc()
// PURPOSE      :
// RETURN VALUE : Hash from an ssrc to a position in the source database.
//---------------------------------------------------------------------------
static
int RtpGetHashvalueBySsrc(unsigned int ssrc)
{
    // Hash from an ssrc to a position in the source database.
    // Assumes that ssrc values are uniformly distributed, which
    // should be true but probably isn't (Rosenberg has reported
    // that many implementations generate ssrc values which are
    // not uniformly distributed over the space, and the H.323
    // spec requires that they are non-uniformly distributed).
    // This routine is written as a function rather than inline
    // code to allow it to be made smart in future:
    return ssrc % RTP_DB_SIZE;
}


//---------------------------------------------------------------------------
// NAME         : RtpCheckRtpSourceDatabase()
// PURPOSE      :
// RETURN VALUE : NONE.
//---------------------------------------------------------------------------
static
void RtpCheckRtpSourceDatabase(RtpSession* session)
{
    // This routine performs a sanity check on the database.
    // This should not call any of the other routines which
    // manipulate the database, to avoid common failures.

    RtcpSourceInfo* srcInfo     = NULL;
    int             sourceCount = 0;
    int             chain       = 0;

    ERROR_Assert(session != NULL, "Session Pointer NULL");

    // Check that we have a database entry for our ssrc
    // We only do this check if ssrcCount > 0 since it is
    // performed during initialisation whilst creating the
    // source entry for ownSsrc.
    if (session->ssrcCount > 0)
    {
        for (srcInfo = session->rtpDatabase[
              RtpGetHashvalueBySsrc(session->ownSsrc)];
                    srcInfo != NULL; srcInfo = srcInfo->next)
        {
            if (srcInfo->ssrc == session->ownSsrc)
            {
                break;
            }
        }
        ERROR_Assert(srcInfo != NULL, "Empty Source Info");
    }

    sourceCount = 0;
    for (chain = 0; chain < RTP_DB_SIZE; chain++) {
        // Check that the linked lists making up the chains in
        // the hash table are correctly linked together
        for (srcInfo = session->rtpDatabase[chain];
                srcInfo != NULL; srcInfo = srcInfo->next)
        {
            RtpCheckSourceInfoInRtpDatabase(srcInfo);
            sourceCount++;
            if (srcInfo->prev == NULL)
            {
                ERROR_Assert(srcInfo == session->rtpDatabase[chain],
                    "Sanity Check: It must be true");
            }
            else
            {
                ERROR_Assert(srcInfo->prev->next == srcInfo,
                    "Sanity Check: It must be true");
            }
            if (srcInfo->next != NULL)
            {
                ERROR_Assert(srcInfo->next->prev == srcInfo,
                    "Sanity Check: It must be true");
            }
            // Check that the SR is for this source
            if (srcInfo->sr != NULL)
            {
                ERROR_Assert(srcInfo->sr->ssrc == srcInfo->ssrc,
                    "Sender Report not meant for this source\n");
            }
        }
    }
    // Check that the number of entries in the hash table
    // matches session->ssrcCount
    ERROR_Assert(sourceCount == session->ssrcCount,
        "No of entries in RTP database does not match session SSRC count\n");
}


//---------------------------------------------------------------------------
// NAME         : RtpGetOwnSsrc()
// PURPOSE      :
// Parameter    : The RTP Session pointer
// RETURN VALUE : (unsigned int)
//                The SSRC we are currently using in this session.
//                Our SSRC can change at any time (due to collisions)so
//                applications must not store the value returned, but rather
//                should call this function each time they need it
//---------------------------------------------------------------------------
static
unsigned int RtpGetOwnSsrc(RtpSession* session)
{
    RtpCheckRtpSourceDatabase(session);
    return session->ownSsrc;
}


//---------------------------------------------------------------------------
// NAME         : RtpGetTimevalueDiff()
// PURPOSE      :
// Parameter    :
// RETURN VALUE : returns difference of two time in clocktype unit.
//---------------------------------------------------------------------------
static
clocktype RtpGetTimevalueDiff(clocktype currTime, clocktype prevTime)
{
    // Return currTime - prevTime
    return (clocktype)(currTime - prevTime);
}


//---------------------------------------------------------------------------
// NAME         : RtpAddTimevalue()
// PURPOSE      : adds offset amount to current time.
// Parameter    :
// RETURN VALUE : returns newtime after addition of offset in clocktype unit.
//---------------------------------------------------------------------------
static
clocktype RtpAddTimevalue(clocktype timeStamp, clocktype offset)
{
    // Add offset seconds to timeStamp
    return (timeStamp + offset);
}


//---------------------------------------------------------------------------
// NAME         : RtpGetMaxTimeVal()
// PURPOSE      : find if timeval a is greater than b or not.
// Parameter    :
// RETURN VALUE : BOOLEAN (TRUE or FALSE).
//---------------------------------------------------------------------------
static
BOOL RtpGetMaxTimeVal(clocktype a, clocktype b)
{
    // Returns TRUE if (a>b) else FALSE
    if (a > b)
    {
        return TRUE;
    }

    return FALSE;
}


//---------------------------------------------------------------------------
// NAME         : RtcpCalculateRoundTripTimeFromReports()
// PURPOSE      : find the Round Trip Time from received reports.
// Parameter    :
// RETURN VALUE : returns Round Trip Time in clocktype unit.
//---------------------------------------------------------------------------
static
clocktype RtcpCalculateRoundTripTimeFromReports(clocktype ntp32BitTime,
                                                clocktype lastSenderReport,
                                                clocktype dlsr)
{
    clocktype delta = 0;

    delta = RtpSubstractNtp32(ntp32BitTime, lastSenderReport);

    if (delta >= dlsr)
    {
        delta -= dlsr;
    }
    else
    {
        // Clock skew bigger than transit delay
        // or garbage dlsr value
         delta = 0;
    }
    return (clocktype) (delta / 65536.0);
}


//---------------------------------------------------------------------------
// NAME         : RtpGetNextCsrc()
// PURPOSE      : .
// Parameter    :
// RETURN VALUE : .
//---------------------------------------------------------------------------
static
unsigned int RtpGetNextCsrc(RtpSession* session)
{
    // This returns each source marked "should_advertise_sdes"
    // in turn.
    int         chain   = 0;
    int         cc      = 0;
    RtcpSourceInfo* srcInfo = NULL;

    for (chain = 0; chain < RTP_DB_SIZE; chain++)
    {
        // Check that the linked lists making up the chains in
        // the hash table are correctly linked together...
        for (srcInfo = session->rtpDatabase[chain]; srcInfo != NULL;
                                                    srcInfo = srcInfo->next)
        {
            if (srcInfo->shouldAdvertiseSdes)
            {
                if (cc == session->lastAdvertisedCsrc)
                {
                    session->lastAdvertisedCsrc++;
                    if (session->lastAdvertisedCsrc == session->csrcCount)
                    {
                        session->lastAdvertisedCsrc = 0;
                    }
                    return srcInfo->ssrc;
                }
                else
                {
                    cc++;
                }
            }
        }
    }
    // We should never reach here
    ERROR_Assert(FALSE, "We should never reach here\n");
    return 0;
}


//---------------------------------------------------------------------------
// NAME         : RtcpInsertRrIntoRtcpRrTable()
// PURPOSE      : inserts RR report information into the receiver report
//                database.
// Parameter    :
// RETURN VALUE : NONE.
//---------------------------------------------------------------------------
static
void RtcpInsertRrIntoRtcpRrTable(RtpSession* session,
                                 unsigned int reporterSsrc,
                                 RtcpRrPacket* rr,
                                 clocktype timeStamp)
{
    // Insert the reception report into the receiver report
    // database. This database is a two dimensional table of
    // RR indexed by hashes of reporterSsrc and reportee_src.
    // The rr_wrappers in the database are sentinels to reduce
    // conditions in list operations.The timeStamp is used to
    // determine when to timeout this rr.
    RtcpRrTable* start = &session->rr[RtpGetHashvalueBySsrc(reporterSsrc)]
                            [RtpGetHashvalueBySsrc(rr->ssrc)];
    RtcpRrTable* cur = start->next;

    while (cur != start)
    {
        if (cur->reporterSsrc == reporterSsrc && cur->rr->ssrc == rr->ssrc)
        {
            // Replace existing entry in the database
            MEM_free(cur->rr);
            cur->rr = rr;
            cur->timeStamp = timeStamp;
            return;
        }
        cur = cur->next;
    }
    // No entry in the database so create one now.
    cur               = (RtcpRrTable*) MEM_malloc(sizeof(RtcpRrTable));
    memset (cur, 0, sizeof(RtcpRrTable));
    cur->reporterSsrc = reporterSsrc;
    cur->rr           = rr;
    cur->timeStamp    = timeStamp;

    cur->next       = start->next;
    cur->next->prev = cur;
    cur->prev       = start;
    cur->prev->next = cur;

    if (DEBUG_RTCP)
    {
        printf("Created new rr entry for %u from source %u\n",
            rr->ssrc, reporterSsrc);
    }
    return;
}


//---------------------------------------------------------------------------
// NAME         : RtcpRemoveRrFromRtcpRrTable()
// PURPOSE      : removes RR report information from the receiver report
//                database .
// Parameter    :
// RETURN VALUE : NONE.
//---------------------------------------------------------------------------
static
void RtcpRemoveRrFromRtcpRrTable(RtpSession* session,
                                        unsigned int ssrc)
{
    // Remove any RRs from source which refer to "ssrc" as either
    // reporter or reportee.
    int          tableSize = 0;
    RtcpRrTable* start = NULL;
    RtcpRrTable* cur   = NULL;
    RtcpRrTable* tmp   = NULL;

    // Remove rows, i.e. ssrc == reporterSsrc
    for (tableSize = 0; tableSize < RTP_DB_SIZE; tableSize++)
    {
        start = &session->rr[RtpGetHashvalueBySsrc(ssrc)][tableSize];
        cur   = start->next;
        while (cur != start)
        {
            if (cur->reporterSsrc == ssrc)
            {
                tmp = cur;
                cur = cur->prev;
                tmp->prev->next = tmp->next;
                tmp->next->prev = tmp->prev;
                tmp->timeStamp = 0;
                MEM_free(tmp->rr);
                MEM_free(tmp);
            }
            cur = cur->next;
        }
    }

    // Remove columns, i.e.  ssrc == reporterSsrc
    for (tableSize = 0; tableSize < RTP_DB_SIZE; tableSize++)
    {
        start = &session->rr[tableSize][RtpGetHashvalueBySsrc(ssrc)];
        cur   = start->next;
        while (cur != start)
        {
            if (cur->rr->ssrc == ssrc)
            {
                tmp = cur;
                cur = cur->prev;
                tmp->prev->next = tmp->next;
                tmp->next->prev = tmp->prev;
                tmp->timeStamp = 0;
                MEM_free(tmp->rr);
                MEM_free(tmp);
            }
            cur = cur->next;
        }
    }
}


//---------------------------------------------------------------------------
// NAME         : RtcpTimeoutRrEntryFromRtcpRrTable()
// PURPOSE      : Timeout any reception reports which have been in the
//                database for more than 3 times the RTCP reporting interval
//                without refresh.
//
// Parameter    :
// RETURN VALUE : NONE.
//---------------------------------------------------------------------------
static
void RtcpTimeoutRrEntryFromRtcpRrTable(RtpSession* session,
                                       clocktype currTs)
{
    // Timeout any reception reports which have been in the database
    // for more than 3 times the RTCP reporting interval without refresh.
    RtcpRrTable* start = NULL;
    RtcpRrTable* cur   = NULL;
    RtcpRrTable* tmp   = NULL;
    int          row   = 0;
    int          column = 0;

    for (row = 0; row < RTP_DB_SIZE; row++)
    {
        for (column = 0; column < RTP_DB_SIZE; column++)
        {
            start = &session->rr[row][column];
            cur   = start->next;
            while (cur != start)
            {
                if (RtpGetTimevalueDiff(currTs, cur->timeStamp) >
                                    (session->rtcpPacketSendingInterval * 3))
                {
                    // Delete this reception report
                    tmp = cur;
                    cur = cur->prev;
                    tmp->prev->next = tmp->next;
                    tmp->next->prev = tmp->prev;
                    tmp->timeStamp = 0;
                    MEM_free(tmp->rr);
                    MEM_free(tmp);
                }
                cur = cur->next;
            }
        }
    }
}


//---------------------------------------------------------------------------
// NAME         : RtpGetSourceInfoFromRtpDatabase()
// PURPOSE      : retrieve source information from RTP database.
//
// Parameter    :
// RETURN VALUE : pointer to SourceInfo structure.
//---------------------------------------------------------------------------
static
RtcpSourceInfo* RtpGetSourceInfoFromRtpDatabase(RtpSession* session,
                                                   unsigned int ssrc)
{
    RtcpSourceInfo* srcInfo = NULL;
    RtpCheckRtpSourceDatabase(session);
    for (srcInfo = session->rtpDatabase[RtpGetHashvalueBySsrc(ssrc)];
         srcInfo != NULL; srcInfo = srcInfo->next)
    {
        if (srcInfo->ssrc == ssrc)
        {
            RtpCheckSourceInfoInRtpDatabase(srcInfo);
            return srcInfo;
        }
    }
    return NULL;
}


//---------------------------------------------------------------------------
// NAME         : RtpCreateSourceInfoInRtpDatabase()
// PURPOSE      : creates source information in RTP database.
// Parameter    :
// RETURN VALUE : pointer to SourceInfo structure.
//---------------------------------------------------------------------------
static
RtcpSourceInfo* RtpCreateSourceInfoInRtpDatabase(RtpSession* session,
                                                    unsigned int ssrc,
                                                    int probation,
                                                    Node* node)
{
    // Create a new source entry, and add it to the database.
    // The database is a hash table, using the separate chaining
    // algorithm.

    if (DEBUG_RTCP)
    {
        printf("RTP: In RtpCreateSourceInfoInRtpDatabase for node %d"
                       " ssrc %d \n",node->nodeId,ssrc);
    }
    RtcpSourceInfo* srcInfo = RtpGetSourceInfoFromRtpDatabase(session, ssrc);
    int hash = 0;

    if (srcInfo != NULL)
    {
        // Source is already in the database... Mark it as
        // active and exit (this is the common case...)
        if (DEBUG_RTCP)
        {
            printf("RTP: Not Creating source info for node %d"
                   "as it already exists\n",node->nodeId);
        }
        srcInfo->lastActive = node->getNodeTime();
        return srcInfo;
    }
    RtpCheckRtpSourceDatabase(session);

    // This is a new source, we have to create it
    hash             = RtpGetHashvalueBySsrc(ssrc);
    srcInfo       = (RtcpSourceInfo*) MEM_malloc(sizeof(RtcpSourceInfo));
    memset(srcInfo, 0, sizeof(RtcpSourceInfo));
    srcInfo->next = session->rtpDatabase[hash];
    srcInfo->ssrc = ssrc;

    if (probation)
    {
        // This is a probationary source, which only counts as
        // valid once several consecutive packets are received
        srcInfo->probation = -1;
    }

    srcInfo->lastActive = node->getNodeTime();

    // Now, add it to the database...
    if (session->rtpDatabase[hash] != NULL)
    {
        session->rtpDatabase[hash]->prev = srcInfo;
    }
    session->rtpDatabase[RtpGetHashvalueBySsrc(ssrc)] = srcInfo;
    session->ssrcCount++;
    RtpCheckRtpSourceDatabase(session);

    if (DEBUG_RTP)
    {
        printf("Created database entry for ssrc %u (%d valid sources)\n",
                                                  ssrc, session->ssrcCount);
    }
    return srcInfo;
}


//---------------------------------------------------------------------------
// NAME         : RtpDeleteSourceInfoFromRtpDatabase()
// PURPOSE      : deletes source information from RTP database.
// Parameter    :
// RETURN VALUE : None.
//---------------------------------------------------------------------------
static
void RtpDeleteSourceInfoFromRtpDatabase(RtpSession* session,
                                               unsigned int ssrc,
                                               Node* node)
{
    // Remove a source from the RTP database...
    if (DEBUG_RTCP)
    {
        printf("In delete source info Get Source info"
                 "for ssrc %d for node %d\n",
                  ssrc,node->nodeId);
    }
    RtcpSourceInfo* srcInfo = RtpGetSourceInfoFromRtpDatabase(session, ssrc);
    int hash = RtpGetHashvalueBySsrc(ssrc);
    clocktype eventTime = node->getNodeTime();

    // Deleting a source which doesn't exist is an error
    if (srcInfo == NULL)
    {
        if (DEBUG_RTP)
        {
            printf("Attempting to Delete a source that does not exist\n");
        }
        return;
    }
    RtpCheckSourceInfoInRtpDatabase(srcInfo);
    RtpCheckRtpSourceDatabase(session);

    if (session->rtpDatabase[hash] == srcInfo)
    {
        // It is the first entry in this chain.
        session->rtpDatabase[hash] = srcInfo->next;
        if (srcInfo->next != NULL)
        {
            srcInfo->next->prev = NULL;
        }
    }
    else
    {
        // else it would be the first in the chain
        ERROR_Assert(srcInfo->prev != NULL,
            "Not the first entry in the chain\n");

        srcInfo->prev->next = srcInfo->next;
        if (srcInfo->next != NULL)
        {
            srcInfo->next->prev = srcInfo->prev;
        }
    }
    // Free the memory allocated to a source.
    if (srcInfo->cname != NULL)
    {
        MEM_free(srcInfo->cname);
        srcInfo->cname = NULL;
    }
//Currently, we support only cname.
#if 0
    if (srcInfo->name  != NULL)
    {
        MEM_free(srcInfo->name);
        srcInfo->name = NULL;
    }

    if (srcInfo->email != NULL)
    {
        MEM_free(srcInfo->email);
        srcInfo->email = NULL;
    }

    if (srcInfo->phone != NULL)
    {
        MEM_free(srcInfo->phone);
        srcInfo->phone = NULL;
    }

    if (srcInfo->loc   != NULL)
    {
        MEM_free(srcInfo->loc);
        srcInfo->loc = NULL;
    }

    if (srcInfo->tool  != NULL)
    {
        MEM_free(srcInfo->tool);
        srcInfo->tool = NULL;
    }

    if (srcInfo->note  != NULL)
    {
        MEM_free(srcInfo->note);
        srcInfo->note = NULL;
    }

    if (srcInfo->priv  != NULL)
    {
        MEM_free(srcInfo->priv);
        srcInfo->priv = NULL;
    }
#endif
    if (srcInfo->sr    != NULL)
    {
        MEM_free(srcInfo->sr);
        srcInfo->sr = NULL;
    }

    RtcpRemoveRrFromRtcpRrTable(session, ssrc);
    // Reduce our SSRC count, and perform reverse reconsideration on the RTCP
    // reporting interval . To make the transmission rate of RTCP packets
    // more adaptive to changes in group membership, the following
    // "reverse reconsideration" algorithm SHOULD be executed when a BYE
    // packet is received that reduces members to a value less than pmembers:
    //   The value for tn is updated according to the following formula:
    //       tn = tc + (members/pmembers)(tn - tc)
    //   The value for tp is updated according the following formula:
    //       tp = tc - (members/pmembers)(tc - tp).
    //   The next RTCP packet is rescheduled for transmission at time tn,
    //    which is now earlier.
    //   The value of pmembers is set equal to members.
    session->ssrcCount--;
    if (session->ssrcCount < session->ssrcPrevCount)
    {

        session->nextRtcpPacketSendTime = node->getNodeTime();
        session->lastRtcpPacketSendTime = node->getNodeTime();

        session->nextRtcpPacketSendTime = RtpAddTimevalue(
                session->nextRtcpPacketSendTime,
                (clocktype)(session->ssrcCount / session->ssrcPrevCount) *
                RtpGetTimevalueDiff(session->nextRtcpPacketSendTime,
                eventTime));

        session->lastRtcpPacketSendTime =
                    RtpAddTimevalue(
                        session->lastRtcpPacketSendTime,
                        -((session->ssrcCount / session->ssrcPrevCount) *
                        RtpGetTimevalueDiff(
                            eventTime,
                            session->lastRtcpPacketSendTime)));

        session->ssrcPrevCount = session->ssrcCount;
    }

    // Reduce our csrc count...
    if (srcInfo->shouldAdvertiseSdes == TRUE)
    {
        session->csrcCount--;
    }

    if (session->lastAdvertisedCsrc == session->csrcCount)
    {
        session->lastAdvertisedCsrc = 0;
    }

    MEM_free(srcInfo);
    RtpCheckRtpSourceDatabase(session);
}


//---------------------------------------------------------------------------
// NAME         : RtpInitializeSequenceNumber()
// PURPOSE      : .
//
// Parameter    :
// RETURN VALUE : None.
//---------------------------------------------------------------------------
static
void RtpInitializeSequenceNumber(RtcpSourceInfo* srcInfo,
                                        unsigned short seqNum)
{
    RtpCheckSourceInfoInRtpDatabase(srcInfo);

    srcInfo->baseSeqNum     = seqNum;
    srcInfo->maxSeqNum      = seqNum;
    srcInfo->badSeqNum      = RTP_SEQ_MOD + 1;
    srcInfo->cycles         = 0;
    srcInfo->received       = 0;
    srcInfo->recvdPrior     = 0;
    srcInfo->expectedPrior  = 0;
}


//---------------------------------------------------------------------------
// NAME         : RtpUpdateSequenceNumber()
// PURPOSE      : .
//
// Parameter    :
// RETURN VALUE : BOOLEAN.
//---------------------------------------------------------------------------
static
BOOL RtpUpdateSequenceNumber(RtcpSourceInfo* srcInfo,
                             unsigned short seqNum)
{
    unsigned short udelta = (unsigned short) (seqNum - srcInfo->maxSeqNum);

    // Source is not valid until RTP_MIN_SEQUENTIAL packets with
    // sequential sequence numbers have been received.
    RtpCheckSourceInfoInRtpDatabase(srcInfo);

    if (srcInfo->probation)
    {
        if (DEBUG_RTP)
        {
            printf("RtpUpdateSequenceNumber 1\n");
        }
        // packet is in sequence
        if (seqNum == srcInfo->maxSeqNum + 1)
        {
            srcInfo->probation--;
            srcInfo->maxSeqNum = seqNum;
            if (srcInfo->probation == 0)
            {
                RtpInitializeSequenceNumber(srcInfo, seqNum);
                srcInfo->received++;
                if (DEBUG_RTP)
                {
                    printf("RTP:srcinfo rcvd %d\n",srcInfo->received);
                }
                return TRUE;
            }
        }
        else
        {
            srcInfo->probation = RTP_MIN_SEQUENTIAL - 1;
            srcInfo->maxSeqNum = seqNum;
        }
        if (DEBUG_RTP)
        {
            printf("RTP:srcinfo rcvd not incremented,Return\n");
        }
        return FALSE;
    }
    else if (udelta < RTP_MAX_DROPOUT)
    {
        // in order, with permissible gap
        if (seqNum < srcInfo->maxSeqNum)
        {
            // Sequence number wrapped - count another 64K cycle.
            srcInfo->cycles += RTP_SEQ_MOD;
        }
        srcInfo->maxSeqNum = seqNum;
        if (DEBUG_RTP)
        {
            printf("RtpUpdateSequenceNumber:pkts recvd in order "
                   "but with permissible gap\n");
        }
    }
    else if (udelta <= RTP_SEQ_MOD - RTP_MAX_MISORDER)
    {
        // the sequence number made a very large jump
        if (seqNum == srcInfo->badSeqNum)
        {
            // Two sequential packets -- assume that the other side
            // restarted without telling us so just re-sync
            // (i.e., pretend this was the first packet).
            RtpInitializeSequenceNumber(srcInfo, seqNum);
            if (DEBUG_RTP)
            {
                printf("Reinitializing as seq no. madea large jump\n");
            }
        }
        else
        {
            srcInfo->badSeqNum = (seqNum + 1) & (RTP_SEQ_MOD-1);
            // As we are not allowing any packet loss we are
            // returning TRUE, It should return FALSE here. FIXME
            if (DEBUG_RTP)
            {
                printf("RtpUpdateSequenceNumber 5\n");
            }
            return TRUE;
        }
    }
    else
    {
        // duplicate or reordered packet
        // discard the packet
    }
    srcInfo->received++;
    if (DEBUG_RTP)
    {
        printf("RTP:srcinfo rcvd %d\n",srcInfo->received);
    }
    return TRUE;
}


//---------------------------------------------------------------------------
// NAME         : RtcpCalculatePktSendInterval()
// PURPOSE      : calculates the interval between transmission of two
//                cosecutive RTCP packets.
//
// Parameter    :
// RETURN VALUE : time value of interval in double.
//---------------------------------------------------------------------------
static
clocktype RtcpCalculatePktSendInterval(RtpSession* session,
                                              Node* node)
{
    double intervalTime = 0.0;    // interval
    int n = 0;  // no. of members for computation
    double rtcpMinTime = RTCP_MIN_TIME;
    double rtcpBandWidth = session->rtcpBandWidth;
    RtpData* rtpData = (RtpData*) node->appData.rtpData;
    // Very first call at application start-up uses half the min
    // delay for quicker notification while still allowing some time
    // before reporting for randomization and to learn about other
    // sources so the report interval will converge to the correct
    // interval more quickly.
    if (session->intialRtcp != 0)
    {
        rtcpMinTime /= 2;
    }

    // If there were active senders, give them at least a minimum
    // share of the RTCP bandwidth.  Otherwise all participants share
    // the RTCP bandwidth equally.
    if (session->sendingBye != 0)
    {
        n = session->byeCountReceived;
    }
    else
    {
        n = session->ssrcCount;
    }

    if (session->senderCount > 0 &&
        session->senderCount < n * RTCP_SENDER_BW_FRACTION)
    {
        if (session->weSentFlag)
        {
            rtcpBandWidth *= RTCP_SENDER_BW_FRACTION;
            n = session->senderCount;
        }
        else
        {
            rtcpBandWidth *= RTCP_RCVR_BW_FRACTION;
            n -= session->senderCount;
        }
    }

    // The effective number of sites times the average packet size is
    // the total number of octets sent when each site sends a report.
    // Dividing this by the effective bandwidth gives the time
    // interval over which those packets must be sent in order to
    // meet the bandwidth target, with a minimum enforced.  In that
    // time interval we send one report so this time is also our
    // average time between reports.
    intervalTime = (double) (session->avgRtcpPktSize * n / rtcpBandWidth);

    if (intervalTime < rtcpMinTime)
    {
        intervalTime = rtcpMinTime;
    }
    session->rtcpPacketSendingInterval = intervalTime;

    // To avoid traffic bursts from unintended synchronization with other
    // sites, we then pick our actual next report interval as a random number
    // uniformly distributed between 0.5*intervalTime and 1.5*intervalTime.
    return (clocktype) (((intervalTime * (RANDOM_erand(rtpData->seed) + 0.5))
                                            / RTCP_COMPENSATION) * SECOND);
}

//---------------------------------------------------------------------------
// NAME         : RtcpScheduleChkSendTimeOfNextRtcpPkt()
// PURPOSE      : schedule the next checking for sending RTP control packets
//                i.e RTCP packets. It sets a timer to be fired at every one
//                second.
// Parameters
// node         : pointer to the node data structure
// sessionPtr   : pointer to the RTP session structure.
// RETURN VALUE : None.
//--------------------------------------------------------------------------
static
void RtcpScheduleChkSendTimeOfNextRtcpPkt(Node* node,
                                                 RtpSession* sessionPtr)
{
    AppGenericTimer* timer = NULL;
    clocktype nextTime = 0;
    Message* timerMsg = MESSAGE_Alloc(node,
                                     APP_LAYER,
                                     APP_RTCP,
                                     MSG_APP_TimerExpired);

    MESSAGE_InfoAlloc(node, timerMsg, sizeof(AppGenericTimer));

    timer = (AppGenericTimer*) MESSAGE_ReturnInfo(timerMsg);
    timer->sourcePort = sessionPtr->rtcpLocalPort;
    memcpy(&(timer->address),&(sessionPtr->remoteAddr),sizeof(Address));
    timer->type = APP_TIMER_SEND_PKT;

    // This timer is set every one second to check
    // whether the time has come to send rtcp packet
    nextTime = SECOND;
    MESSAGE_Send(node, timerMsg, nextTime);
}


//--------------------------------------------------------------------------
// NAME         : RtpGetSessionPtrByDestAddr()
// PURPOSE      : Get pointer to thew structure for the current session.
// ASSUMPTION   : Assuming a single session is active between any two pair
//                of nodes. Thus ignoring rtpPort.
// RETURN VALUE : pointer to the RtpSession struct.
//--------------------------------------------------------------------------
static
RtpSession* RtpGetSessionPtrByDestAddr(RtpData* rtp,
                                       Address destAddr,
                                       unsigned short initiatorPort,
                                       BOOL isRtpProcessed)
{
    RtpSession* session = rtp->rtpSession;
    RtpMappingPort* rtpMap = rtp->rtpMapping;
    while (rtpMap != NULL)
    {
        if ((isRtpProcessed && (rtpMap->rtpPort == initiatorPort)) ||
          (!isRtpProcessed && (rtpMap->applicationPort == initiatorPort)))
        {
            if (!session)
            {
                break;
            }
            while (session != NULL)
            {
                if (DEBUG_RTP)
                {
                    char Addr1[MAX_STRING_LENGTH],Addr2[MAX_STRING_LENGTH];

                    IO_ConvertIpAddressToString(&session->localAddr, Addr1);
                    IO_ConvertIpAddressToString(&session->remoteAddr, Addr2);
                    printf("\n\n Returning RTP session"
                        " (local ip %s,local port %d,"
                         "remote ip %s)\n",Addr1,session->localPort,Addr2);
                         IO_ConvertIpAddressToString(&destAddr, Addr2);
                    printf("\n\n Comparing with "
                        " %s\n",Addr2);
                }
                if (IO_CheckIsSameAddress(
                    session->remoteAddr, destAddr) && session->activeNow
                    && session->localPort == rtpMap->rtpPort)
                {
                    if (DEBUG_RTP)
                    {
                        char Addr1[MAX_STRING_LENGTH];
                        char Addr2[MAX_STRING_LENGTH];

                        IO_ConvertIpAddressToString(&session->localAddr,
                                                    Addr1);
                        IO_ConvertIpAddressToString(&session->remoteAddr,
                                                    Addr2);
                        printf("\n\n Returning RTP session (local ip %s,"
                        "local port %d,remote ip %s)\n",Addr1,
                        session->localPort,Addr2);
                    }
                    return session;
                }
                if (DEBUG_RTP)
                {
                    printf("\nProceed to next session\n");
                }
                session = session->next;
            }
        }
        else
        {
            if (DEBUG_RTP)
            {
                printf("\nProceed to next rtpmap\n");
            }
            rtpMap = rtpMap->next;
        }
    }

    if (DEBUG_RTP)
    {
        printf("Returning NULL Session\n");
    }

    return session;
}

static
RtpSession* RtcpGetSessionPtrByDestAddr(RtpData* rtp,
                                       Address destAddr,
                                       unsigned short rtcpPort,
                                       BOOL isRtpProcessed)
                                        //whether manipulated for
                                        //checking it as odd
{
    RtpSession* session = rtp->rtpSession;
    RtpMappingPort* rtpMap = rtp->rtpMapping;
    while (rtpMap != NULL)
    {
        if ((isRtpProcessed && ((rtpMap->rtpPort + 1) == rtcpPort)) ||
          (!isRtpProcessed && ((rtpMap->applicationPort + 1) == rtcpPort)))
        {
            if (!session)
            {
                break;
            }
            while (session)
            {
                if (IO_CheckIsSameAddress(
                   session->remoteAddr, destAddr) && session->activeNow
                   && session->rtcpLocalPort == (rtpMap->rtpPort + 1))
                {
                    return session;
                }
                session = session->next;
            }
        }
        else
        {
            rtpMap = rtpMap->next;
        }
    }
    return session;
}


//---------------------------------------------------------------------------
// NAME         : RtcpFormatReportBlocks()
// PURPOSE      : Build the report blocks to be inserted in compound
//                RTCP packet.
// Parameters
// node              : pointer to the node data struct
// rrp               : pointer to the RR packet struct
// remaininingLength :
// session           : pointer to the RTP session structure.
// RETURN VALUE : None.
//---------------------------------------------------------------------------
static
int RtcpFormatReportBlocks(Node* node,
                                  RtcpRrPacket* rtcpReportPacket,
                                  int remaininingLength,
                                  RtpSession* session)
{
    int numOfBlocks = 0;
    int h = 0;
    RtcpSourceInfo* srcInfo = NULL;
    clocktype now = node->getNodeTime();

    for (h = 0; h < RTP_DB_SIZE; h++)
    {
        for (srcInfo = session->rtpDatabase[h]; srcInfo != NULL;
                                                srcInfo = srcInfo->next)
        {
            RtpCheckSourceInfoInRtpDatabase(srcInfo);
            if ((numOfBlocks == RTCP_MAX_NUM_BLOCK) ||
                (remaininingLength < RTCP_RR_BLOCK_IN_BYTE))
            {
                break; // Insufficient space for more report blocks
            }

            if (srcInfo->sender)
            {
                int fraction = 0;
                unsigned int lsr = 0;
                unsigned int dlsr = 0;
                int extendedMax      = srcInfo->cycles + srcInfo->maxSeqNum;
                int expected         = extendedMax - srcInfo->baseSeqNum + 1;
                int lost             = expected - srcInfo->received;
                int expectedInterval = expected - srcInfo->expectedPrior;
                int receivedInterval = srcInfo->received
                                            - srcInfo->recvdPrior;
                int lostInterval     = expectedInterval - receivedInterval;
                srcInfo->expectedPrior = expected;
                srcInfo->recvdPrior = srcInfo->received;

                if (expectedInterval == 0 || lostInterval <= 0)
                {
                    fraction = 0;
                }
                else
                {
                    fraction = ((lostInterval << 8) / expectedInterval);
                }

                if (srcInfo->sr == NULL)
                {
                    lsr = 0;
                    dlsr = 0;
                }
                else
                {
                    lsr  = (unsigned int)RtpCalculateNtp64ToNtp32(
                                srcInfo->sr->ntpSec, srcInfo->sr->ntpFrac);

                    dlsr = (unsigned int)((RtpGetTimevalueDiff(now,
                            srcInfo->lastSenderReport) * 65536) / SECOND);
                }

                rtcpReportPacket->ssrc          = srcInfo->ssrc;
                RtcpRrPacketSetFracLost(&(rtcpReportPacket->RrPktLost),
                    fraction);
                RtcpRrPacketSetTotLost(&(rtcpReportPacket->RrPktLost),
                    (lost & 0x00ffffff));
                rtcpReportPacket->lastSeqNum    = extendedMax;
                rtcpReportPacket->jitter        = (srcInfo->jitter / 16);
                rtcpReportPacket->lsr           = lsr;
                rtcpReportPacket->dlsr          = dlsr;
                rtcpReportPacket++;
                remaininingLength              -= RTCP_RR_BLOCK_IN_BYTE;
                numOfBlocks++;
                srcInfo->sender                 = FALSE;
                session->senderCount--;

                if (session->senderCount == 0)
                {
                    break; // No point continuing, since we've reported
                           // on all senders...
                }
            }
        }
    }
    return numOfBlocks;
}


//---------------------------------------------------------------------------
// NAME         : RtcpFormatSr()
// PURPOSE      : Write an RTCP SR into buffer
//
// Parameters
// node                 : pointer to the node data struct
// buffer               : pointer to char buffer
// buflen               : buffer size
// session              : pointer to the RTP session structure.
// rtptimeStamp         : RTP timestamp.
// RETURN VALUE : a pointer to the next byte after the header we have just
//                written.
//---------------------------------------------------------------------------
static
unsigned char* RtcpFormatSr(Node* node,
                                   unsigned char* buffer,
                                   int buflen,
                                   RtpSession* session,
                                   clocktype rtptimeStamp)
{
    // Write an RTCP SR into buffer, returning a pointer to
    // the next byte after the header we have just written.
    RtcpPacket*     packet = (RtcpPacket*) buffer;
    int remaininingLength = 0;
    clocktype ntpSec = 0;
    clocktype ntpFrac = 0;

    // else there is not space for the header and sender report
    ERROR_Assert(buflen >= RTCP_SR_FIXED_SIZE,
                "There is not space for the header and sender report\n");

    RtcpCommonHeaderSetVersion(&(packet->common.rtcpCh), RTP_VERSION) ;
    RtcpCommonHeaderSetP(&(packet->common.rtcpCh), 0);
    RtcpCommonHeaderSetCount(&(packet->common.rtcpCh), 0);
    RtcpCommonHeaderSetPt(&(packet->common.rtcpCh), RTCP_SR);
    packet->common.length  = 1;

    RtpGetNtpTimestampFromCurSimTime(node, &ntpSec, &ntpFrac);

    packet->rtcpPacket.sr.sr.ssrc            = RtpGetOwnSsrc(session);
    packet->rtcpPacket.sr.sr.ntpSec          = (unsigned int) ntpSec;
    packet->rtcpPacket.sr.sr.ntpFrac         = (unsigned int) ntpFrac;

    packet->rtcpPacket.sr.sr.rtptimeStamp    =
                        (unsigned int) (rtptimeStamp / MILLI_SECOND);
    packet->rtcpPacket.sr.sr.senderPktCount  = session->rtpPacketCount;
    packet->rtcpPacket.sr.sr.senderByteCount = session->rtpByteCount;

    // Add report blocks, until we either run out of senders
    // to report upon or we run out of space in the buffer.
    remaininingLength = buflen - RTCP_SR_FIXED_SIZE;
    RtcpCommonHeaderSetCount(&(packet->common.rtcpCh),
        RtcpFormatReportBlocks(node,packet->rtcpPacket.sr.rr,
        remaininingLength,session));

    packet->common.length = (unsigned short) (RTCP_RR_BLOCK_SIZE +
                                (RtcpCommonHeaderGetCount(
                                packet->common.rtcpCh)* RTCP_RR_BLOCK_SIZE));
    return buffer + RTCP_SR_FIXED_SIZE +
        (RTCP_RR_BLOCK_IN_BYTE * RtcpCommonHeaderGetCount(
        packet->common.rtcpCh));
}


//---------------------------------------------------------------------------
// NAME         : RtcpFormatRr()
// PURPOSE      : Write an RTCP RR into buffer
//
// Parameters
// node         : pointer to the node data struct
// buffer       : pointer to char buffer
// buflen       : buffer size
// session      : pointer to the RTP session structure.
// RETURN VALUE : a pointer to the next byte after the header we have just
//                written.
//---------------------------------------------------------------------------
static
unsigned char* RtcpFormatRr(Node* node,
                                   unsigned char* buffer,
                                   int buflen,
                                   RtpSession* session)
{
    // Write an RTCP RR into buffer, returning a pointer to
    // the next byte after the header we have just written.
    RtcpPacket* packet = (RtcpPacket*) buffer;
    int remaininingLength = 0;

    // else there isn't space for the header
    ERROR_Assert(buflen >= 8, "There isn't space for the header\n");

    RtcpCommonHeaderSetVersion(&(packet->common.rtcpCh), RTP_VERSION);
    RtcpCommonHeaderSetP(&(packet->common.rtcpCh), 0);
    RtcpCommonHeaderSetCount(&(packet->common.rtcpCh), 0);
    RtcpCommonHeaderSetPt(&(packet->common.rtcpCh), RTCP_RR);
    packet->common.length  = 1;
    packet->rtcpPacket.rr.ssrc = session->ownSsrc;

    // Add report blocks, until we either run out of senders
    // to report upon or we run out of space in the buffer.
    remaininingLength       = buflen - 8;
    RtcpCommonHeaderSetCount(&(packet->common.rtcpCh),
        RtcpFormatReportBlocks(node,packet->rtcpPacket.rr.rr,
        remaininingLength,session));
    packet->common.length   = (unsigned short)(1 + (
        RtcpCommonHeaderGetCount(packet->common.rtcpCh)
        * RTCP_RR_BLOCK_SIZE));

    return buffer + 8 + (RTCP_RR_BLOCK_IN_BYTE *
        RtcpCommonHeaderGetCount(packet->common.rtcpCh));
}


//---------------------------------------------------------------------------
// NAME         : RtcpGetSdesInfoFromRtpDatabase()
// PURPOSE      : Recovers session description (SDES) information on
//                participant identified with ssrc.  The SDES information
//                associated with a source is updated when receiver reports
//                are received.  There are several different types of SDES
//                information, e.g. username,location, phone, email.
//                These are enumerated by #RtcpSdesType.
//
// Parameters
// session : the session pointer
// ssrc    : the SSRC identifier of a participant
// type    : the SDES information to retrieve
// RETURN VALUE : pointer to string containing SDES description if received,
//                NULL otherwise.
//---------------------------------------------------------------------------
static
const char* RtcpGetSdesInfoFromRtpDatabase(RtpSession* session,
                                           unsigned int ssrc,
                                           RtcpSdesType type)
{
    RtcpSourceInfo* srcInfo = NULL;
    char localAddr[MAX_STRING_LENGTH];

    RtpCheckRtpSourceDatabase(session);
    if (DEBUG_RTCP)
    {
        IO_ConvertIpAddressToString(&session->localAddr, localAddr);
        printf("Get Source info for ssrc %d for node ip %s\n",
                  ssrc,localAddr);
    }
    srcInfo = RtpGetSourceInfoFromRtpDatabase(session, ssrc);

    if (srcInfo == NULL)
    {
        if (DEBUG_RTCP)
        {
            printf("Invalid source 0x%08x\n", ssrc);
        }
        return NULL;
    }
    RtpCheckSourceInfoInRtpDatabase(srcInfo);

    switch (type)
    {
        case RTCP_SDES_CNAME:
        {
            return srcInfo->cname;
        }
        case RTCP_SDES_NAME:
        {
            return srcInfo->name;
        }
        case RTCP_SDES_EMAIL:
        {
            return srcInfo->email;
        }
        case RTCP_SDES_PHONE:
        {
            return srcInfo->phone;
        }
        case RTCP_SDES_LOC:
        {
            return srcInfo->loc;
        }
        case RTCP_SDES_TOOL:
        {
            return srcInfo->tool;
        }
        case RTCP_SDES_NOTE:
        {
            return srcInfo->note;
        }
        case RTCP_SDES_PRIV:
        {
            return srcInfo->priv;
        }
        default:
        {
            // This includes RTCP_SDES_PRIV and RTCP_SDES_END
            if (DEBUG_RTP)
            {
                printf("Unknown SDES item (type=%d)\n", type);
            }
        }
    }
    return NULL;
}


//---------------------------------------------------------------------------
// NAME         : RtcpAddSdesItem()
// PURPOSE      : Fill out an SDES item. It is assumed that the item is a
//                NULL terminated string.
// Parameters
// RETURN VALUE :
//---------------------------------------------------------------------------
static
int RtcpAddSdesItem(unsigned char* buf,
                           int buflen,
                           int type,
                           const char* val)
{
    // Fill out an SDES item. It is assumed that the item is a NULL
    // terminated string.
    RtcpSdesItem* shdr = (RtcpSdesItem*) buf;
    int namelen = 0;

    if (val == NULL)
    {
        if (DEBUG_RTCP)
        {
            printf("Cannot format SDES item. type=%d val=%s\n", type, val);
        }
        return 0;
    }
    shdr->type   = (unsigned char) type;
    namelen      = (int)strlen(val);
    shdr->length = (unsigned char) namelen;
    // The "-2" accounts for the other shdr fields
    strncpy(shdr->data, val, buflen - 2);
    return namelen + 2;
}


//---------------------------------------------------------------------------
// NAME         : RtcpFormatSdes()
// PURPOSE      : builds SDES items.
// Parameters
// RETURN VALUE :
//---------------------------------------------------------------------------
static
unsigned char* RtcpFormatSdes(unsigned char* buffer,
                              int buflen,
                              unsigned int ssrc,
                              RtpSession* session)
{
    // "Applications may use any of the SDES items described in the
    // RTP specification. While CNAME information is sent every
    // reporting interval, other items should be sent only every third
    // reporting interval, with NAME sent seven out of eight times
    // within that slot and the remaining SDES items cyclically taking
    // up the eighth slot, as defined in Section 6.2.2 of the RTP
    // specification. In other words, NAME is sent in RTCP packets 1,
    // 4, 7, 10, 13, 16, 19, while, say, EMAIL is used in RTCP packet
    // 22".
    unsigned char*      packet  = buffer;
    RtcpCommonHeader*   common  = (RtcpCommonHeader*) buffer;
    const char*         item    = NULL;
    unsigned int remainingLength = 0;
    int pad = 0;

    ERROR_Assert(buflen > (int)sizeof(RtcpCommonHeader),
        "Buffer length should be larger than RTCP common header size");

    RtcpCommonHeaderSetVersion(&(common->rtcpCh), RTP_VERSION);
    RtcpCommonHeaderSetP(&(common->rtcpCh), 0);
    RtcpCommonHeaderSetCount(&(common->rtcpCh), 1);
    RtcpCommonHeaderSetPt(&(common->rtcpCh), RTCP_SDES);
    common->length  = 0;
    packet += sizeof(RtcpCommonHeader);
    *((unsigned int*) packet) = ssrc;
    packet += 4;
    remainingLength = (int)(buflen - (packet - buffer));
    item = RtcpGetSdesInfoFromRtpDatabase(session, ssrc, RTCP_SDES_CNAME);

    if ((item != NULL) &&
        ((strlen(item) + (unsigned int) 2) <= remainingLength))
    {
        packet += RtcpAddSdesItem(packet,
                                  remainingLength,
                                  RTCP_SDES_CNAME,
                                  item);
    }
    remainingLength = (int)(buflen - (packet - buffer));
    item = RtcpGetSdesInfoFromRtpDatabase(session, ssrc, RTCP_SDES_NOTE);

    if ((item != NULL) &&
        ((strlen(item) + (unsigned int) 2) <= remainingLength))
    {
        packet += RtcpAddSdesItem(packet,
                                  remainingLength,
                                  RTCP_SDES_NOTE,
                                  item);
    }
    remainingLength = (int)(buflen - (packet - buffer));

    if ((session->sdesPrimaryCount % 3) == 0)
    {
        session->sdesSecondaryCount++;

        if ((session->sdesSecondaryCount % 8) == 0)
        {
            // Note that the following is supposed to fall-through the cases
            // until one is found to send.The lack of break statements in
            // the switch is not a bug.
            switch (session->sdesTernaryCount % 5)
            {
                case 0:
                {
                    item = RtcpGetSdesInfoFromRtpDatabase(session,
                                                          ssrc,
                                                          RTCP_SDES_TOOL);
                    if ((item != NULL)
                        && ((strlen(item) + (unsigned int) 2)
                                <= remainingLength))
                    {
                        packet += RtcpAddSdesItem(packet,
                                                  remainingLength,
                                                  RTCP_SDES_TOOL,
                                                  item);
                        break;
                    }
                }
                case 1:
                {
                    item = RtcpGetSdesInfoFromRtpDatabase(session,
                                                          ssrc,
                                                          RTCP_SDES_EMAIL);
                    if ((item != NULL) &&
                        ((strlen(item) + (unsigned int) 2)
                                <= remainingLength))
                    {
                        packet += RtcpAddSdesItem(packet,
                                                  remainingLength,
                                                  RTCP_SDES_EMAIL,
                                                  item);
                        break;
                    }
                }
                case 2:
                {
                    item = RtcpGetSdesInfoFromRtpDatabase(session,
                                                          ssrc,
                                                          RTCP_SDES_PHONE);
                    if ((item != NULL)
                        && ((strlen(item) + (unsigned int) 2)
                                <= remainingLength))
                    {
                        packet += RtcpAddSdesItem(packet,
                                                  remainingLength,
                                                  RTCP_SDES_PHONE,
                                                  item);
                        break;
                    }
                }
                case 3:
                {
                    item = RtcpGetSdesInfoFromRtpDatabase(session,
                                                          ssrc,
                                                          RTCP_SDES_LOC);
                    if ((item != NULL)
                        && ((strlen(item) + (unsigned int) 2)
                                <= remainingLength))
                    {
                        packet += RtcpAddSdesItem(packet,
                                                  remainingLength,
                                                  RTCP_SDES_LOC,
                                                  item);
                        break;
                    }
                }
                case 4:
                {
                    item = RtcpGetSdesInfoFromRtpDatabase(session,
                                                          ssrc,
                                                          RTCP_SDES_PRIV);
                    if ((item != NULL)
                        && ((strlen(item) + (unsigned int) 2)
                                <= remainingLength))
                    {
                        packet += RtcpAddSdesItem(packet,
                                                  remainingLength,
                                                  RTCP_SDES_PRIV,
                                                  item);
                        break;
                    }
                }
            }
            session->sdesTernaryCount++;
        }
        else
        {
            item = RtcpGetSdesInfoFromRtpDatabase(session,
                                                  ssrc,
                                                  RTCP_SDES_NAME);
            if (item != NULL)
            {
                packet += RtcpAddSdesItem(packet,
                                          remainingLength,
                                          RTCP_SDES_NAME,
                                          item);
            }
        }
    }
    session->sdesPrimaryCount++;

    // Pad to a multiple of 4 bytes
    pad = (int)(4 - ((packet - buffer) & 0x3));
    while (pad--)
    {
        *packet++ = RTCP_SDES_END;
    }
    common->length = (unsigned short) (((int) (packet - buffer) / 4) - 1);
    return packet;
}



//---------------------------------------------------------------------------
// NAME         : RtcpProcessReportBlocks()
// PURPOSE      : process received Report Blocks.
// Parameters
// RETURN VALUE :
//---------------------------------------------------------------------------
static
void RtcpProcessReportBlocks(Node* node,
                                   RtpSession* session,
                                   RtcpPacket* packet,
                                   unsigned int ssrc,
                                   RtcpRrPacket* rrPacket,
                                   clocktype eventTime)
{
    int              reportCount    = 0;
    RtcpRrPacket*    receiverReport = NULL;
    clocktype        ntpSec         = 0;
    clocktype        ntpFrac        = 0;
    clocktype        ntp32          = 0;
    clocktype        roundTripTime  = 0;

    // Process RRs...
    if (RtcpCommonHeaderGetCount(packet->common.rtcpCh) == 0)
    {
        if (DEBUG_RTCP)
        {
             printf("Receiver Report Empty\n");
        }
    }
    else
    {
        for (reportCount = 0; reportCount <
            RtcpCommonHeaderGetCount(packet->common.rtcpCh);
                                                   reportCount++, rrPacket++)
        {
            receiverReport =
                        (RtcpRrPacket*) MEM_malloc(sizeof(RtcpRrPacket));
            memset(receiverReport, 0, sizeof(RtcpRrPacket));

            receiverReport->ssrc        = rrPacket->ssrc;
            RtcpRrPacketSetFracLost(&(receiverReport->RrPktLost),
                RtcpRrPacketGetFracLost(rrPacket->RrPktLost));
            RtcpRrPacketSetTotLost(&(receiverReport->RrPktLost),
                RtcpRrPacketGetTotLost(rrPacket->RrPktLost));
            receiverReport->lastSeqNum  = rrPacket->lastSeqNum;
            receiverReport->jitter      = rrPacket->jitter;
            receiverReport->lsr         = rrPacket->lsr;
            receiverReport->dlsr        = rrPacket->dlsr;

            // Create a database entry for this SSRC, if one doesn't
            // already exist
            if (DEBUG_RTCP)
            {
                printf("RTP: Creating source info for node %d\n",
                       node->nodeId);
            }
            RtpCreateSourceInfoInRtpDatabase(session,
                                             receiverReport->ssrc,
                                             FALSE,
                                             node);
            // Store the RR for later use
            RtcpInsertRrIntoRtcpRrTable(session,
                                        ssrc,
                                        receiverReport,
                                        eventTime);

            RtpGetNtpTimestampFromCurSimTime(node, &ntpSec, &ntpFrac);

            ntp32 = RtpCalculateNtp64ToNtp32(ntpSec, ntpFrac);

            roundTripTime = RtcpCalculateRoundTripTimeFromReports(
                            (clocktype)ntp32,
                            (clocktype)receiverReport->lsr,
                            (clocktype)receiverReport->dlsr);

            session->rndTripTime += roundTripTime;
            if (session->maxRoundTripTime < roundTripTime)
            {
                session->maxRoundTripTime = roundTripTime;
            }

            if (session->minRoundTripTime > roundTripTime)
            {
                session->minRoundTripTime = roundTripTime;
            }
        }
    }
}


//---------------------------------------------------------------------------
// NAME         : RtcpProcessReceivedSr()
// PURPOSE      : process received Sender Report.
// Parameters
// RETURN VALUE :
//---------------------------------------------------------------------------
static
void RtcpProcessReceivedSr(Node* node,
                           RtpSession* session,
                           RtcpPacket* packet,
                           clocktype eventTime)
{
    unsigned int    ssrc    = packet->rtcpPacket.sr.sr.ssrc;
    RtcpSrPacket*   sr      = NULL;
    if (DEBUG_RTCP)
    {
        printf("RTP: Creating source info for node %d\n",node->nodeId);
    }
    RtcpSourceInfo*     srcInfo =
                RtpCreateSourceInfoInRtpDatabase(session, ssrc, FALSE, node);

    if (srcInfo == NULL)
    {
        if (DEBUG_RTCP)
        {
            printf("Source 0x%08x invalid, skipping\n", ssrc);
        }
        return;
    }

    // Mark as an active sender, if we get a sender report...
    if (srcInfo->sender == FALSE)
    {
        srcInfo->sender = TRUE;
        session->senderCount++;
    }

    // Process the SR
    sr = (RtcpSrPacket*) MEM_malloc(sizeof(RtcpSrPacket));
    memset(sr, 0, sizeof(RtcpSrPacket));
    sr->ssrc            = ssrc;
    sr->ntpSec          = packet->rtcpPacket.sr.sr.ntpSec;
    sr->ntpFrac         = packet->rtcpPacket.sr.sr.ntpFrac;
    sr->rtptimeStamp    = packet->rtcpPacket.sr.sr.rtptimeStamp;
    sr->senderPktCount  = packet->rtcpPacket.sr.sr.senderPktCount;
    sr->senderByteCount = packet->rtcpPacket.sr.sr.senderByteCount;

    // Store the SR for later retrieval...
    if (srcInfo->sr != NULL)
    {
        MEM_free(srcInfo->sr);
        srcInfo->sr = NULL;
    }

    srcInfo->sr = sr;
    srcInfo->lastSenderReport = eventTime;
    RtcpProcessReportBlocks(node,
                            session,
                            packet,
                            ssrc,
                            packet->rtcpPacket.sr.rr,
                            eventTime);

    if (((RtcpCommonHeaderGetCount(packet->common.rtcpCh) *
        RTCP_RR_BLOCK_SIZE) + 1) <((packet->common.length) - 5))
    {
        if (DEBUG_RTCP)
        {
            printf("Profile specific SR extension ignored\n");
        }
        return;
    }
}


//---------------------------------------------------------------------------
// NAME         : RtcpProcessReceivedRr()
// PURPOSE      : process received Receiver Report.
// Parameters
// RETURN VALUE :
//---------------------------------------------------------------------------
static
void RtcpProcessReceivedRr(Node* node,
                                  RtpSession* session,
                                  RtcpPacket* packet,
                                  clocktype eventTime)
{
    unsigned int ssrc = packet->rtcpPacket.rr.ssrc;
    if (DEBUG_RTCP)
    {
        printf("RTP: Creating source info for node %d\n",node->nodeId);
    }
    RtcpSourceInfo* srcInfo =
                RtpCreateSourceInfoInRtpDatabase(session, ssrc, FALSE, node);

    if (srcInfo == NULL)
    {
        if (DEBUG_RTCP)
        {
            printf("Source 0x%08x invalid, skipping...\n", ssrc);
        }
        return;
    }

    RtcpProcessReportBlocks(node,
                            session,
                            packet, ssrc,
                            packet->rtcpPacket.rr.rr,
                            eventTime);

    if (((RtcpCommonHeaderGetCount(packet->common.rtcpCh) *
        RTCP_RR_BLOCK_SIZE) + 1) < (packet->common.length))
    {
        if (DEBUG_RTCP)
        {
            printf("Profile specific RR extension ignored\n");
        }
    }
}


//--------------------------------------------------------------------------
// NAME         : RtcpAssociateSdesInfoWithSource()
// PURPOSE      : Sets session description information associated with
//                participant ssrc. Under normal circumstances applications
//                always use the ssrc of the local participant, this SDES
//                information is transmitted in receiver reports. Setting
//                SDES information forother participants affects the local
//                SDES entries, but are not transmitted onto the network.
// Parameters
// ssrc   : the SSRC identifier of a participant
// type   : the SDES type represented by value
// value  : the SDES description
// length : the length of the description
// RETURN VALUE : Returns TRUE if participant exists, FALSE otherwise.
//--------------------------------------------------------------------------
static
BOOL RtcpAssociateSdesInfoWithSource(RtpSession* session,
                                            unsigned int ssrc,
                                            RtcpSdesType type,
                                            const char* value,
                                            int length)
{
    RtcpSourceInfo* srcInfo = NULL;
    char* val           = NULL;
    char localAddr[MAX_STRING_LENGTH];

    RtpCheckRtpSourceDatabase(session);
    if (DEBUG_RTCP)
    {
        IO_ConvertIpAddressToString(&session->localAddr, localAddr);
        printf("In RtcpAssociateSdesInfoWithSource:"
               "Get Source info for ssrc %d with node ip %s\n",
                  ssrc,localAddr);
    }
    srcInfo = RtpGetSourceInfoFromRtpDatabase(session, ssrc);

    if (srcInfo == NULL)
    {
        if (DEBUG_RTCP)
        {
            printf("Invalid source 0x%08x\n", ssrc);
        }
        return FALSE;
    }
    RtpCheckSourceInfoInRtpDatabase(srcInfo);

    val = (char*) MEM_malloc(length + 1);
    memcpy(val, value, length);
    val[length] = '\0';

    switch (type)
    {
        case RTCP_SDES_CNAME:
            if (srcInfo->cname)
            {
                MEM_free(srcInfo->cname);
            }
            srcInfo->cname = val;
            break;
            //Currently, we support only cname
#if 0
        case RTCP_SDES_NAME:
            if (srcInfo->name)
            {
                MEM_free(srcInfo->name);
            }
            srcInfo->name = val;
            break;
        case RTCP_SDES_EMAIL:
            if (srcInfo->email)
            {
                MEM_free(srcInfo->email);
            }
            srcInfo->email = val;
            break;
        case RTCP_SDES_PHONE:
            if (srcInfo->phone)
            {
                MEM_free(srcInfo->phone);
            }
            srcInfo->phone = val;
            break;
        case RTCP_SDES_LOC:
            if (srcInfo->loc)
            {
                MEM_free(srcInfo->loc);
            }
            srcInfo->loc = val;
            break;
        case RTCP_SDES_TOOL:
            if (srcInfo->tool)
            {
                MEM_free(srcInfo->tool);
            }
            srcInfo->tool = val;
            break;
        case RTCP_SDES_NOTE:
            if (srcInfo->note)
            {
                MEM_free(srcInfo->note);
            }
            srcInfo->note = val;
            break;
        case RTCP_SDES_PRIV:
            if (srcInfo->priv)
            {
                MEM_free(srcInfo->priv);
            }
            srcInfo->priv = val;
            break;
#endif
        default :
        {
            if (DEBUG_RTCP)
            {
                printf("Unknown SDES item (type=%d, value=%s)\n", type, val);
            }
            MEM_free(val);
            RtpCheckRtpSourceDatabase(session);
            return FALSE;
        }
    }
    RtpCheckRtpSourceDatabase(session);
    return TRUE;
}


//---------------------------------------------------------------------------
// NAME         : RtcpProcessReceivedSdesPacket()
// PURPOSE      : process received SDES packets.
// Parameters
// RETURN VALUE :
//---------------------------------------------------------------------------
static
void RtcpProcessReceivedSdesPacket(Node* node,
                                   RtpSession* session,
                                   RtcpPacket* packet)
{
    int count = RtcpCommonHeaderGetCount(packet->common.rtcpCh);
    struct rtcp_sdes_struct* sd = (struct rtcp_sdes_struct*)
                                        &packet->rtcpPacket.sdes;
    RtcpSdesItem* rsp = NULL;
    RtcpSdesItem* rspn = NULL;
    RtcpSdesItem* end = (RtcpSdesItem*)((unsigned int*)packet +
                            packet->common.length + 1);
    RtcpSourceInfo* srcInfo = NULL;

    while (--count >= 0)
    {
        rsp = &sd->item[0];

        if (rsp >= end)
        {
            break;
        }
        if (DEBUG_RTCP)
        {
            printf("RTP: Creating source info for node %d\n",node->nodeId);
        }
        srcInfo = RtpCreateSourceInfoInRtpDatabase(session,
                                                   sd->ssrc,
                                                   FALSE,
                                                   node);

        if (srcInfo == NULL)
        {
            if (DEBUG_RTCP)
            {
                printf("Can't get valid source entry for "
                       "%u, hence skipping\n", sd->ssrc);
            }
        }
        else
        {
            for (; rsp->type; rsp = rspn)
            {
                rspn = (RtcpSdesItem*)((char*) rsp + rsp->length+2);

                if (rspn >= end)
                {
                    rsp = rspn;
                    break;
                }

                if (RtcpAssociateSdesInfoWithSource(session,
                                                    sd->ssrc,
                                                    (RtcpSdesType)rsp->type,
                                                    rsp->data,
                                                    rsp->length))
                {
                    if (DEBUG_RTCP)
                    {
                        printf("Valid SDES item received for "
                               "source %u\n\n", sd->ssrc);
                    }
                }
                else
                {
                    if (DEBUG_RTCP)
                    {
                        printf("Invalid sdes item for source "
                               "%u, hence skipping\n",
                               sd->ssrc);
                    }
                }
            }
        }
        sd = (struct rtcp_sdes_struct*)
                  ((unsigned int*) sd + (((char*) rsp - (char*) sd) >> 2)+1);
    }

    if (count >= 0)
    {
        if (DEBUG_RTCP)
        {
            printf("Invalid RTCP SDES packet, some items ignored.\n");
        }
    }
}

//--------------------------------------------------------------------------
// NAME         : RtcpSendByePacketNow()
// PURPOSE      : send RTCP BYE packet immediately.
// Parameters
// node   : pointer to node struct.
// session: the RTP session to finish.
// RETURN VALUE : None.
//--------------------------------------------------------------------------
static
void RtcpSendByePacketNow(Node* node, RtpSession* session)
{
    // Send a BYE packet immediately. This is an internal function,  */
    RtpData* rtp = (RtpData*) node->appData.rtpData;
    unsigned char      buffer[RTP_MAX_PACKET_LEN];
    unsigned char*     ptr = buffer;
    RtcpCommonHeader* common = NULL;

    RtpCheckRtpSourceDatabase(session);

    // "A participant which never sent an RTP or RTCP packet MUST NOT send
    // a BYE packet when they leave the group." (section 6.3.7 of RTP spec)
    if ((session->weSentFlag == FALSE) && (session->intialRtcp == TRUE))
    {
        if (DEBUG_RTCP)
        {
            printf("Node %d sending Silent BYE,setting"
                " session activeNow as false\n", node->nodeId);
        }
        session->sessionEnd = node->getNodeTime();
        session->activeNow = FALSE;
        rtp->sessionOpened = FALSE;
        return;
    }
    ptr    = RtcpFormatRr(node,
                          ptr,
                          (int)(RTP_MAX_PACKET_LEN - (ptr - buffer)),
                          session);
    session->stats.rtcpRRSent++;
    if (DEBUG_RTCP)
    {
        printf("RTCP RR sent %d\n",session->stats.rtcpRRSent);
    }
    common = (RtcpCommonHeader*) ptr;
    RtcpCommonHeaderSetVersion(&(common->rtcpCh), RTP_VERSION);
    RtcpCommonHeaderSetP(&(common->rtcpCh), 0);
    RtcpCommonHeaderSetCount(&(common->rtcpCh), 1);
    RtcpCommonHeaderSetPt(&(common->rtcpCh), RTCP_BYE);
    common->length  = 1;
    ptr += sizeof(RtcpCommonHeader);

    *((unsigned int*) ptr) = session->ownSsrc;
    ptr += 4;

    session->stats.rtcpBYESent += 1;
    if (DEBUG_RTCP)
    {
        printf("RTCP Bye sent %d\n",session->stats.rtcpBYESent);
    }
    // Send UDP packets by calling interface functions
    Message *msg = APP_UdpCreateMessage(
        node,
        session->localAddr,
        (short)session->rtcpLocalPort,
        session->remoteAddr,
        session->rtcpRemotePort,
        TRACE_RTCP);

    APP_AddPayload(
        node,
        msg,
        buffer,
        ptr - buffer);

    APP_UdpSend(node, msg);

    session->rtcppktSent++;

    RtpCheckRtpSourceDatabase(session);
}

//---------------------------------------------------------------------------
// NAME         : RtcpProcessReceivedBye()
// PURPOSE      : process received BYE packets.
// Parameters
// RETURN VALUE :
//---------------------------------------------------------------------------
static
void RtcpProcessReceivedBye(Node* node,
                            RtpSession* session,
                            RtcpPacket* packet)
{
    int count = 0;
    unsigned int ssrc = 0;
    RtcpSourceInfo* srcInfo = NULL;
    RtpData* rtp = (RtpData*) node->appData.rtpData;

    for (count = 0;
        count < RtcpCommonHeaderGetCount(packet->common.rtcpCh); count++)
    {
        ssrc = packet->rtcpPacket.bye.ssrc[count];

        // Mark the source as ready for deletion. Sources are not deleted
        // immediately since some packets may be delayed and arrive after
        // the BYE
        if (DEBUG_RTCP)
        {
           printf("Get Source info for ssrc %d for node %d\n",
                  ssrc,node->nodeId);
        }
        srcInfo = RtpGetSourceInfoFromRtpDatabase(session, ssrc);
        srcInfo->gotBye = TRUE;
        //session->gotBye = TRUE;
        RtpCheckSourceInfoInRtpDatabase(srcInfo);
        session->byeCountReceived++;

        // Terminate this session
        if (rtp->jitterBufferEnable == TRUE)
        {
            RtpClearJitterBufferOnCloseSession(node, session);
        }

        // Send BYE packet to indicate that a participant leaves a session
        RtcpSendByePacketNow(node, session);

        if (DEBUG_RTP)
        {
            char clockStr[MAX_STRING_LENGTH];
            char  addrStr[MAX_STRING_LENGTH];

            TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
            IO_ConvertIpAddressToString(&(session->remoteAddr),
                            addrStr);

            printf("RTP: Node %u terminate RTP session"
              " with node %s at %s, setting activeNow false\n",
              node->nodeId, addrStr, clockStr);
        }
        session->sessionEnd = node->getNodeTime();
        session->activeNow  = FALSE;
        rtp->sessionOpened  = FALSE;

    }
}


//---------------------------------------------------------------------------
// NAME         : RtcpValidateReceivedControlPacket()
// PURPOSE      : validation of received RTCP packets before
//                they are processed
// Parameters
// RETURN VALUE :
//---------------------------------------------------------------------------
static
int RtcpValidateReceivedControlPacket(unsigned char* packet, int len)
{
    // Validity check for a compound RTCP packet. This function returns
    // TRUE if the packet is okay, FALSE if the validity check fails.
    //
    // The following checks can be applied to RTCP packets [RFC1889]:
    // RTP version field must equal 2.
    // The payload type field of the first RTCP packet in a compound
    // packet must be equal to SR or RR.
    // The padding bit (P) should be zero for the first packet of a
    // compound RTCP packet because only the last should possibly
    // need padding.
    // The length fields of the individual RTCP packets must total to
    // the overall length of the compound RTCP packet as received.

    RtcpPacket* pkt     = (RtcpPacket*) packet;
    RtcpPacket* end     = (RtcpPacket*) (((char*) pkt) + len);
    RtcpPacket* recvd   = pkt;
    int  length         = 0;
    int  pc             = 1;
    int  pad            = 0;

    // All RTCP packets must be compound packets (RFC1889, section 6.1)
    if ((((pkt->common.length) + 1) * 4) == len)
    {
        if (DEBUG_RTCP)
        {
            printf("Bogus RTCP packet: not a compound packet\n");
        }
        return FALSE;
    }

    // Check the RTCP version, payload type and padding of the first in
    // the compund RTCP packet...
    if (RtcpCommonHeaderGetVersion(pkt->common.rtcpCh) != RTP_VERSION)
    {
        if (DEBUG_RTCP)
        {
            printf("Bogus RTCP packet: version number != 2 in "
                "the first sub-packet\n");
        }
        return FALSE;
    }
    if (RtcpCommonHeaderGetP(pkt->common.rtcpCh) != 0)
    {
        if (DEBUG_RTCP)
        {
            printf("Bogus RTCP packet: padding bit is set on "
                "first packet in compound\n");
        }
        return FALSE;
    }

    UInt8 rtcpPktType = RtcpCommonHeaderGetPt(pkt->common.rtcpCh);

    if ((rtcpPktType != RTCP_SR) && (rtcpPktType != RTCP_RR))
    {
        if (DEBUG_RTCP)
        {
            printf("Bogus RTCP packet: compound packet does not "
                "start with SR or RR\n");
        }
        return FALSE;
    }

    // Check all following parts of the compund RTCP packet. The RTP version
    // number must be 2, and the padding bit must be zero on all apart from
    // the last packet.
    do
    {
        if (pad == 1)
        {
            if (DEBUG_RTCP)
            {
                printf("Bogus RTCP packet: padding bit set before last "
                    "in compound (sub-packet %d)\n", pc);
            }
            return FALSE;
        }
        if (RtcpCommonHeaderGetP(recvd->common.rtcpCh))
        {
            pad = 1;
        }
        if (RtcpCommonHeaderGetVersion(recvd->common.rtcpCh) != RTP_VERSION)
        {
            if (DEBUG_RTCP)
            {
                printf("Bogus RTCP packet: version number != 2 in "
                    "sub-packet %d\n", pc);
            }
            return FALSE;
        }
        length += ((recvd->common.length) + 1) * 4;
        recvd  = (RtcpPacket*) (((unsigned int*) recvd) +
                    (recvd->common.length) + 1);
        pc++;   // count of sub-packets, for debugging...
    } while (recvd < end);

    // Check that the length of the packets matches the length of the UDP
    // packet in which they were received...
    if (length != len)
    {
        if (DEBUG_RTCP)
        {
            printf("Bogus RTCP packet: \n"
                   "RTCP packet length does not match "
                   "UDP packet length (%d != %d)\n", length, len);
        }
        return FALSE;
    }
    if (recvd != end)
    {
        if (DEBUG_RTCP)
        {
            printf("Bogus RTCP packet: \n"
                   "RTCP packet length does not match "
                   "UDP packet length (%p != %p)\n", recvd, end);
        }
        return FALSE;
    }
    return TRUE;
}


//---------------------------------------------------------------------------
// NAME         : RtcpProcessControlPacket()
// PURPOSE      : process RTP control packets i.e, RTCP packets
//
// Parameters
// RETURN VALUE : None.
//---------------------------------------------------------------------------
static
void RtcpProcessControlPacket(Node* node,
                                     RtpSession* session,
                                     unsigned char* buffer,
                                     int buflen)
{
    // This routine processes incoming RTCP Compund packets
    clocktype eventTime = node->getNodeTime();
    RtcpPacket* packet = NULL;

    if (buflen > 0)
    {
        session->rtcppktRecvd++;


        if (RtcpValidateReceivedControlPacket(buffer, buflen))
        {
            packet = (RtcpPacket*) buffer;
            while (packet < (RtcpPacket*) (buffer + buflen))
            {
                switch (RtcpCommonHeaderGetPt(packet->common.rtcpCh))
                    {
                    case RTCP_SR:
                    {
                        session->stats.rtcpSRRecvd += 1;
                        if (DEBUG_RTCP)
                        {
                          printf("SR RCVD Count %d\n",
                                 session->stats.rtcpSRRecvd);
                        }
                        RtcpProcessReceivedSr(node,
                                              session,
                                              packet,
                                              eventTime);
                        break;
                    }
                    case RTCP_RR:
                    {
                        session->stats.rtcpRRRecvd += 1;
                        if (DEBUG_RTCP)
                        {
                          printf("RR RCVD Count %d\n",
                                  session->stats.rtcpRRRecvd);
                        }
                        RtcpProcessReceivedRr(node,
                                              session,
                                              packet,
                                              eventTime);
                        break;
                    }
                    case RTCP_SDES:
                    {
                        session->stats.rtcpSDESRecvd += 1;
                        if (DEBUG_RTCP)
                        {
                          printf("SDES RCVD Count %d\n",
                                 session->stats.rtcpSDESRecvd);
                        }
                        RtcpProcessReceivedSdesPacket(node,
                                                      session,
                                                      packet);
                        break;
                    }
                    case RTCP_BYE:
                    {
                        session->stats.rtcpBYERecvd += 1;
                        if (DEBUG_RTCP)
                        {
                          printf("Bye RCVD Count %d\n",
                                 session->stats.rtcpBYERecvd);
                        }
                        RtcpProcessReceivedBye(node,
                                               session,
                                               packet);
                        // We only close the session after confirming
                        // we have recived a BYE packet from Remote Host
                        break;
                    }
                    case RTCP_APP:
                    {
                        // TBD
                        break;
                    }
                    default:
                    {
                        if (DEBUG_RTCP)
                        {
                            printf("RTCP packet with unknown type (%d) "
                                "ignored.\n",
                               RtcpCommonHeaderGetPt(packet->common.rtcpCh));
                        }
                        break;
                    }
                }
                packet = (RtcpPacket*) ((char*) packet +
                            (4 * ((packet->common.length) + 1)));
            }
            if (session->avgRtcpPktSize < 0)
            {
                // This is the first RTCP packet we've received, set our
                // initial estimate of the average  packet size to be the
                // size of this packet.
                session->avgRtcpPktSize = buflen + RTP_LOWER_LAYER_OVERHEAD;
            }
            else
            {
                // Update our estimate of the average RTCP packet size.
                // The constants are 1/16 and 15/16 .
                session->avgRtcpPktSize =
                        ((0.0625 * (buflen + RTP_LOWER_LAYER_OVERHEAD)) +
                            (0.9375 * session->avgRtcpPktSize));
            }
        }
        else
        {
            if (DEBUG_RTCP)
            {
                printf("Invalid RTCP packet discarded\n");
            }
            session->invalidRtcpPktCount++;
        }
    }
    else
    {
       if (DEBUG_RTCP)
       {
           printf("A RTCP Packet of size zero has come\n");
       }
       return;
    }
}


//---------------------------------------------------------------------------
// NAME         : RtcpSendControlPacket()
// PURPOSE      : send RTP control packets i.e, RTCP packets
//
// Parameters
// RETURN VALUE : None.
//---------------------------------------------------------------------------
static
void RtcpSendControlPacket(Node* node,
                                  RtpSession* session,
                                  clocktype rtptimeStamp)
{
    // Construct and send an RTCP packet. The order in which packets are
    // packed into a compound packet is defined in RTP spec and we follow the
    // recommended order.
    unsigned char   buffer[RTP_MAX_PACKET_LEN];
    unsigned char*  ptr = buffer;
    unsigned char* oldPtr = NULL;
    unsigned char* lpt = NULL; // the last packet in the compound

    RtpCheckRtpSourceDatabase(session);

    // The first RTCP packet in the compound packet MUST always be a report
    // packet
    if (session->weSentFlag)
    {
        ptr = RtcpFormatSr(node,
                           ptr,
                           (int)(RTP_MAX_PACKET_LEN - (ptr - buffer)),
                           session,
                           rtptimeStamp);

        session->stats.rtcpSRSent++;
        if (DEBUG_RTCP)
        {
            printf("A RTCP SR packet sent from NODE %d, SRsent %d\n",
                node->nodeId,session->stats.rtcpSRSent);
        }

    }
    else
    {
      ptr = RtcpFormatRr(node, ptr,
                        (int)(RTP_MAX_PACKET_LEN - (ptr - buffer)),
                        session);
      session->stats.rtcpRRSent++;

      if (DEBUG_RTCP)
      {
          printf("A RTCP RR packet sent from NODE %d, RRsent %d\n",
              node->nodeId,session->stats.rtcpRRSent);
      }
    }

    // Add the appropriate SDES items to the packet... This should really be
    // after the insertion of the additional report blocks, but if we do
    // that there are problems with us being unable to fit the SDES packet
    // in when we run out of buffer space adding RRs. The correct fix would
    // be to calculate the length of the SDES items in advance and subtract
    // this from the buffer length but this is non-trivial and probably not
    // worth it.
    lpt = ptr;
    ptr = RtcpFormatSdes(ptr, (int)(RTP_MAX_PACKET_LEN - (ptr - buffer)),
                                            RtpGetOwnSsrc(session), session);
    session->stats.rtcpSDESSent++;
    if (DEBUG_RTCP)
    {
        printf("A RTCP SDES packet sent from NODE %d SDESSent %d\n",
            node->nodeId,session->stats.rtcpSDESSent);
    }
    // Currently, we support only Fixed RTP header.
#if 0
    // If we have any CSRCs, we include SDES items for each of them in turn.
    if (session->csrcCount > 0)
    {
        ptr = RtcpFormatSdes(ptr, (int)(RTP_MAX_PACKET_LEN - (ptr - buffer)),
                                    RtpGetNextCsrc(session), session);
        session->stats.rtcpSDESSent++;
        if (DEBUG_RTCP)
        {
            printf("A RTCP SDES packet sent from NODE %d SDESsent %d\n",
                   node->nodeId,session->stats.rtcpSDESSent);
        }
    }
#endif
    // Following that, additional RR packets SHOULD follow if there are more
    // than 31 senders, such that the reports do not fit into the initial
    // packet. We give up if there is insufficient space in the buffer:
    // this is bad, since we always drop the reports from the same sources
    // (those at the end of the hash table).
    while ((session->senderCount > 0)  &&
        ((RTP_MAX_PACKET_LEN - (ptr - buffer)) >= 8))
    {
        lpt = ptr;
        ptr = RtcpFormatRr(node, ptr,
                          (int)(RTP_MAX_PACKET_LEN - (ptr - buffer)),
                          session);
        session->stats.rtcpRRSent++;
        if (DEBUG_RTCP)
        {
            printf("A RTCP RR packet sent from NODE %u RRsent %d\n",
                   node->nodeId,session->stats.rtcpRRSent);
        }
    }

    // Finish with as many APP packets as the application will provide. */
    oldPtr = ptr;
    // TBD

    // Send UDP packets by calling interface functions
    session->rtcppktSent++;

    Message *msg = APP_UdpCreateMessage(
        node,
        session->localAddr,
        session->rtcpLocalPort,
        session->remoteAddr,
        session->rtcpRemotePort,
        TRACE_RTCP);

    APP_AddPayload(
        node,
        msg,
        buffer,
        ptr - buffer);

    APP_UdpSend(node, msg);

    RtpCheckRtpSourceDatabase(session);
}


//---------------------------------------------------------------------------
// NAME         : RtcpGetNextControlPacketSendTime()
// PURPOSE      : Checks RTCP timer and sends RTCP data when nececessary.
//                The interval between RTCP packets is randomized over an
//                interval that depends on the session bandwidth, the number
//                of participants, and whether the local participant is a
//                sender.  This function should be called at least once per
//                second.
//
// Parameters
// RETURN VALUE : None.
//---------------------------------------------------------------------------
static
void RtcpGetNextControlPacketSendTime(Node* node,
                                      RtpSession* session,
                                      clocktype rtptimeStamp)
{
    // Send an RTCP packet, if one is due...
    clocktype currTime = node->getNodeTime();
    char      buf[RTP_MAX_PACKET_LEN];

    RtpCheckRtpSourceDatabase(session);
    if (DEBUG_RTP)
    {
       TIME_PrintClockInSecond(currTime, buf);
       printf("In RtcpGetNextControlPacketSendTime at %sS:\n",buf);

       TIME_PrintClockInSecond(session->nextRtcpPacketSendTime, buf);
       printf("In RtcpGetNextControlPacketSendTime,"
              "nextRtcpPacketSendTime %sS:\n",buf);
    }
    if (RtpGetMaxTimeVal(currTime, session->nextRtcpPacketSendTime))
    {
        // The RTCP transmission timer has expired.
        if (DEBUG_RTP)
        {
             printf("In RtcpGetNextControlPacketSendTime: "
             "RTCP transmission timer has expired\n");
        }
        int hash = 0;
        RtcpSourceInfo* srcInfo = NULL;
        clocktype newInterval = ((RtcpCalculatePktSendInterval
                                    (session, node) /
                                        (session->csrcCount + 1)));

        clocktype newSendTime = RtpAddTimevalue(
                            session->lastRtcpPacketSendTime, newInterval);

        if (DEBUG_RTP)
        {
            TIME_PrintClockInSecond(newSendTime, buf);
            printf("In RtcpGetNextControlPacketSendTime,newSendTime"
                   " %sS:\n",buf);
        }
        if (RtpGetMaxTimeVal(currTime, newSendTime))
        {
            if (DEBUG_RTP)
            {
                 printf("In RtcpGetNextControlPacketSendTime: "
                        "Sending control pkt\n");
            }
            RtcpSendControlPacket(node, session, rtptimeStamp);
            session->intialRtcp = FALSE;
            session->lastRtcpPacketSendTime = currTime;
            session->nextRtcpPacketSendTime = currTime;
            session->nextRtcpPacketSendTime = RtpAddTimevalue(
                            session->nextRtcpPacketSendTime,
                           ((RtcpCalculatePktSendInterval(
                                                            session, node) /
                           (session->csrcCount + 1))));

            // We are starting a new RTCP reporting interval, zero out
            // the per-interval statistics.
            session->senderCount = 0;
            for (hash = 0; hash < RTP_DB_SIZE; hash++)
            {
                for (srcInfo = session->rtpDatabase[hash]; srcInfo != NULL;
                    srcInfo = srcInfo->next)
                {
                    RtpCheckSourceInfoInRtpDatabase(srcInfo);
                    srcInfo->sender = FALSE;
                }
            }
        }
        else
        {
            session->nextRtcpPacketSendTime = newSendTime;
        }
        session->ssrcPrevCount = session->ssrcCount;
    }
    RtpCheckRtpSourceDatabase(session);
}


//--------------------------------------------------------------------------
// NAME         : RtcpInit()
// PURPOSE      : Initializes RTCP model.
// RETURN VALUE : None.
//--------------------------------------------------------------------------
void RtcpInit(Node* node,
              RtpSession* session,
              char* srcAliasAddr)
{
    char* cname   = NULL;

    // Initialization of rtcp related session members
    session->intialRtcp             = TRUE;
    session->sendingBye             = FALSE;

    // Invalid value: reception of first packet starts initial value
    session->avgRtcpPktSize         = -1;
    session->rtcpBandWidth
        = ((RtpData*)(node->appData.rtpData))->rtcpSessionBandwidth;
    session->invalidRtcpPktCount    = 0;
    session->byeCountReceived       = 0;
    session->sdesPrimaryCount       = 0;
    session->sdesSecondaryCount     = 0;
    session->sdesTernaryCount       = 0;
    session->lastRtcpPacketSendTime = node->getNodeTime();
    session->nextRtcpPacketSendTime = node->getNodeTime();

    // Initialize differnt RTCP packet related stat
    memset(&session->stats, 0, sizeof(RtcpStats));

    // Calculate when we're supposed to send our first RTCP packet
    session->nextRtcpPacketSendTime = RtpAddTimevalue(
                                session->nextRtcpPacketSendTime,
                                RtcpCalculatePktSendInterval(
                                                            session, node));

    // for the time being we take the endpoint table as a source of CNAME,
    // and we compromise on the fact that email name is now
    // considered as CNAME
    cname = (char*) MEM_malloc(RTP_MAX_CNAME_LEN + 1);
    memset(cname, 0, sizeof(RTP_MAX_CNAME_LEN + 1));
    strcpy(cname, srcAliasAddr);

    RtcpAssociateSdesInfoWithSource(session,
                                    session->ownSsrc,
                                    RTCP_SDES_CNAME,
                                    cname,
                                    (int)strlen(cname));

    // cname is copied by RtcpAssociateSdesInfoWithSource()
    // so we can release the memory
    MEM_free(cname);

    if (DEBUG_RTCP)
    {
        char clockStr[MAX_STRING_LENGTH];
        printf("Node %d checking timer to send first RTCP packet: ",
                                                            node->nodeId);

        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
        printf("    at time %s\n", clockStr);
    }
    // Added to count no. of RTCP pkts rcvd/sent
    session->rtcppktSent = 0;
    session->rtcppktRecvd = 0;

    // Check whether RTP session is valid and active then
    // first time call the function to check whether to send
    // rtcp packet or not
    if (session && session->activeNow)
    {
        RtcpGetNextControlPacketSendTime(node,
                                         session,
                                         node->getNodeTime());
    }
    else
    {
        return;
    }

    // Schedule a timer with an interval of 1 second to
    // check when to next call the function RtcpGetNextControlPacketSendTime
    RtcpScheduleChkSendTimeOfNextRtcpPkt(node, session);
}


//--------------------------------------------------------------------------
// NAME         : RtpClearRtpDatabase()
// PURPOSE      : Free the state associated with the given RTP session.
// Parameters
// session: the RTP session to finish
// RETURN VALUE : None.
//--------------------------------------------------------------------------
static
void RtpClearRtpDatabase(Node* node, RtpSession* session)
{
    int count = 0;
    RtcpSourceInfo* srcInfo = NULL;
    RtcpSourceInfo* next = NULL;

    RtpCheckRtpSourceDatabase(session);
    // In RtpDeleteSourceInfoFromRtpDatabase, check_database gets called
    // and this assumes first added and last removed is us.
    for (count = 0; count < RTP_DB_SIZE; count++)
    {
        srcInfo = session->rtpDatabase[count];
        while (srcInfo != NULL)
        {
            next = srcInfo->next;
            if (srcInfo->ssrc != session->ownSsrc)
            {
                if (DEBUG_RTCP)
                {
                    printf("RTP: Deleting sourec info from RTP database"
                           " for node %d for ssrc %d\n",
                           node->nodeId,session->rtpDatabase[count]->ssrc);
                }
                RtpDeleteSourceInfoFromRtpDatabase(
                                           session,
                                           session->rtpDatabase[count]->ssrc,
                                           node);
            }
            srcInfo = next;
        }
    }
        if (DEBUG_RTCP)
        {
       printf("RTP: Deleting sourec info from RTP database"
       " for node %d ssrc %u\n",node->nodeId,session->ownSsrc);
        }
    RtpDeleteSourceInfoFromRtpDatabase(session, session->ownSsrc, node);
    }



//--------------------------------------------------------------------------
// NAME         : RtpInit()
// PURPOSE      : Initializes RTP model.
// PARAMETERS       :
//          node    : A pointer to Node
//        nodeInput : A pointer to NodeInput
//
// RETURN VALUE : None.
//--------------------------------------------------------------------------
void RtpInit(Node* node, const NodeInput* nodeInput)
{
    RtpData* rtpData = NULL;

    rtpData = (RtpData*) MEM_malloc(sizeof(RtpData));
    memset(rtpData, 0, sizeof(RtpData));
    node->appData.rtpData = (void*) rtpData;

    rtpData->rtpSession = NULL;
    rtpData->sessionOpened = FALSE;
    rtpData->totalSession = 0;
    // Initialize Pointer also
    rtpData->rtpMapping = NULL;

    RANDOM_SetSeed(rtpData->seed,
                   node->globalSeed,
                   node->nodeId,
                   APP_RTP);

    RtpInitializeRtpDataFromConfigFile(node, rtpData, nodeInput);

#if 0
    RTPInitTrace(node, nodeInput);
    RTCPInitTrace(node, nodeInput);
#endif
}


//--------------------------------------------------------------------------
// NAME         : RtpUpdateRtpDatabase()
// PURPOSE      : Trawls through the internal data structures and performs
//                housekeeping. It uses an internal timer to limit the number
//                of passes through the data structures to once per second.
// Parameters
// node   : pointer to node struct.
// session: the RTP session to finish.
// RETURN VALUE : None.
//--------------------------------------------------------------------------
static
void RtpUpdateRtpDatabase(Node* node, RtpSession* session)
{
    // Perform housekeeping on the source database
    int h = 0;
    RtcpSourceInfo* srcInfo = NULL;
    RtcpSourceInfo* n = NULL;
    clocktype currTime = node->getNodeTime();
    clocktype delay = 0;

    if (RtpGetTimevalueDiff(currTime, session->lastUpdate) < (1 * SECOND))
    {
        // We only perform housekeeping once per second.
        return;
    }
    session->lastUpdate = currTime;

    // Update weSentFlag (section 6.3.8 of RTP spec)
    delay = RtpGetTimevalueDiff(currTime, session->lastRtpPacketSendTime);
    if (delay >= 2 * RtcpCalculatePktSendInterval(session, node))
    {
        session->weSentFlag = FALSE;
    }
    RtpCheckRtpSourceDatabase(session);

    for (h = 0; h < RTP_DB_SIZE; h++)
    {
        for (srcInfo = session->rtpDatabase[h]; srcInfo != NULL; srcInfo = n)
        {
            RtpCheckSourceInfoInRtpDatabase(srcInfo);
            n = srcInfo->next;
            // Expire sources which haven't been heard from for a long time.
            // Section 6.2.1 of the RTP specification details the timer used.

            // How long since we last heard from this source?
            delay = RtpGetTimevalueDiff(currTime, srcInfo->lastActive);

            // Check if we've received a BYE packet from this source.
            // If we have, and it was received more than 2 seconds ago
            // then the source is deleted. The arbitrary 2 second delay
            // is to ensure that all delayed packets are received before
            // the source is timed out.
            if (srcInfo->gotBye && (delay > 2))
            {
                if (DEBUG_RTCP)
                {
                    printf("Deleting source %u due to reception of BYE %f\n"
                           "seconds ago\n", srcInfo->ssrc, (float) delay);
                }
                RtpDeleteSourceInfoFromRtpDatabase(session,
                                                   srcInfo->ssrc,
                                                   node);
            }

            // Sources are marked as inactive if they haven't been heard
            // from for more than 2 intervals (RTP section 6.3.5)
            if ((srcInfo->ssrc != RtpGetOwnSsrc(session)) &&
                (delay > (session->rtcpPacketSendingInterval * 2)))
            {
                if (srcInfo->sender)
                {
                    srcInfo->sender = FALSE;
                    session->senderCount--;
                }
            }

            // If a source hasn't been heard from for more than 5 RTCP
            // reporting intervals, we delete it from our database...
            if ((srcInfo->ssrc != RtpGetOwnSsrc(session)) &&
                 (delay > (session->rtcpPacketSendingInterval * 5 * SECOND)))
            {
                if (DEBUG_RTCP)
                {
                    printf("Deleting source %u due to timeout...\n",
                                                              srcInfo->ssrc);
                }
                RtpDeleteSourceInfoFromRtpDatabase(session,
                                                   srcInfo->ssrc,
                                                   node);
            }
        }
    }
    // Timeout those reception reports which have not been refreshed for a
    // long time
    RtcpTimeoutRrEntryFromRtcpRrTable(session, currTime);

    RtpCheckRtpSourceDatabase(session);
}


//--------------------------------------------------------------------------
// NAME         : RtpAddNewSession()
//
// PURPOSE      : Add new session to list.
//
// RETURN VALUE : None.
//--------------------------------------------------------------------------
static
void RtpAddNewSession(RtpData* rtp, RtpSession* newSession)
{
    if (rtp->rtpSession)
    {
        newSession->next = rtp->rtpSession;
        rtp->rtpSession->prev = newSession;
        rtp->rtpSession = newSession;
    }
    else
    {
        rtp->rtpSession = newSession;
    }
}


//--------------------------------------------------------------------------
// NAME         : RtpInitiateNewSession()
//
// PURPOSE      : A new session is begin. Initiate data structure for it.
//
// RETURN VALUE : None.
//--------------------------------------------------------------------------
void RtpInitiateNewSession(
     Node* node,
     Address localAddr,
     char*       srcAliasAddress,
     Address remoteAddr,
     clocktype packetizationInterval,
     unsigned short localPort,
     unsigned short applicationPayloadType,
     BOOL           initiator,
     void*          RTPSendFunc)
{
    int row = 0;
    int column = 0;

    RtpSession* session = NULL;
    char addr[MAX_STRING_LENGTH];
    RtpData* rtp = (RtpData*) node->appData.rtpData;

    // Session already opened so abort
    if (rtp == NULL)
    {
        char error[MAX_STRING_LENGTH];

        sprintf(error, "Initiating RTP session for RTP Disabled.");
        ERROR_ReportError(error);
    }
    if (rtp->sessionOpened)
    {
        if (DEBUG_RTCP)
        {
            IO_ConvertIpAddressToString(&remoteAddr,addr);
            printf("\n\nNode %u, remoteAddr %s\n\n",
                node->nodeId, addr);
        }
    }

    session = (RtpSession*) MEM_malloc(sizeof(RtpSession));
    memset (session, 0, sizeof(RtpSession));
    session->payloadType = (char)applicationPayloadType;
    session->remoteAddr = remoteAddr;
    session->localAddr = localAddr;
    if (initiator == TRUE)
    {
      session->call_initiator = TRUE;
    }
    else
    {
      session->call_initiator = FALSE;
    }

    // Use same port for both local and remote
    //RTP port
    session->localPort = APP_RTP;
    session->remotePort     = APP_RTP;
    //RTCP port
    ERROR_Assert(APP_RTCP == APP_RTP + 1, "RTCP port should be RTP port plus one\n");
    session->rtcpLocalPort = APP_RTCP;
    session->rtcpRemotePort = APP_RTCP;


    session->activeNow          = TRUE;
    session->ownSsrc            = (unsigned int) RANDOM_nrand(rtp->seed);
    session->invalidRtpPktCount = 0;
    session->probPktDropCount   = 0;
    session->csrcCount          = 0;
    session->ssrcCount          = 0;
    session->ssrcPrevCount      = 0;
    session->senderCount        = 0;
    session->weSentFlag         = FALSE;
    session->rtpSeqNum          = (unsigned short) RANDOM_nrand(rtp->seed);
    session->rtpPacketCount     = 0;
    session->rtpByteCount       = 0;
    session->pktReceived= 0;
    session->bytesReceived= 0;
    session->sessionStart       = node->getNodeTime();
    session->lastUpdate         = session->sessionStart;
    session->maxRoundTripTime = 0;
    session->minRoundTripTime = 200 * MILLI_SECOND;
    session->rndTripTime = 0;
    session->sendFuncPtr = RTPSendFunc;
    session->jitterBuffer = NULL;

    session->jitter_Buffer_Enable = rtp->jitterBufferEnable;

    //Initialize Jitter buffer if it is Enabled
    if (session->jitter_Buffer_Enable == TRUE)
    {
        //Initialize the jitterBuffer
        session->jitterBuffer =
            (RtpJitterBuffer*)MEM_malloc(sizeof(RtpJitterBuffer));
        memset(session->jitterBuffer, 0, sizeof(RtpJitterBuffer));

        RtpJitterBuffer* sessionJitterBuffer = session->jitterBuffer;

        // Initialize the List
        ListInit(node,&sessionJitterBuffer->listJitterNodes);

        sessionJitterBuffer->maxNoNodesInJitter = rtp->maxNoPacketInJitter;
        sessionJitterBuffer->nominalDelay = packetizationInterval;
        sessionJitterBuffer->maxDelay  = rtp->maximumDelayInJitter;
        sessionJitterBuffer->talkspurtDelay = rtp->talkspurtDelayInJitter;

        sessionJitterBuffer->packetDelay = 0;
        sessionJitterBuffer->packetVariance = 0;
        sessionJitterBuffer->maxConsecutivePacketDropped = 0;

        //Initialize the first expected sequence of own Node
        //sessionJitterBuffer->expectedSeq = session->rtpSeqNum + 1;
        sessionJitterBuffer->expectedSeq = 0;

        //Initialize number of packet dropped due to jitter Buffer
        sessionJitterBuffer->packetDropped = 0;

        //Initialize the firstPacket variable
        sessionJitterBuffer->firstPacket = TRUE;
        sessionJitterBuffer->msgNominalDelay = NULL;
        sessionJitterBuffer->msgTalkSpurtDelay = NULL;


        if (DEBUG_RTPJITTER)
        {
            printf("Jitter first expected sequence number is %d\n",
                            sessionJitterBuffer->expectedSeq);
        }
    }
    if (DEBUG_RTPJITTER)
    {
        if (session->jitter_Buffer_Enable == TRUE)
        {
            printf("Voip jitter is %s\n","Enabled");
        }
        else
        {
            printf("Voip jitter is %s\n","Disabled");
        }
    }

    // Initialise the source database... */
    for (row = 0; row < RTP_DB_SIZE; row++)
    {
        session->rtpDatabase[row] = NULL;
    }
    session->lastAdvertisedCsrc = 0;

    // Initialize sentinels in rr table */
    for (row = 0; row < RTP_DB_SIZE; row++)
    {
        for (column = 0; column < RTP_DB_SIZE; column++)
        {
            session->rr[row][column].next = &session->rr[row][column];
            session->rr[row][column].prev = &session->rr[row][column];
        }
    }

    // Create a database entry for self.
    if (DEBUG_RTP)
    {
      printf("RTP: Creating source info for node %d\n",node->nodeId);
    }
    RtpCreateSourceInfoInRtpDatabase(session, session->ownSsrc, FALSE, node);

    // Initialize valiables and flags for RTCP session.
    // Calculate when we are supposed to send our first RTCP packet.
    RtcpInit(node, session, srcAliasAddress);
    rtp->sessionOpened = TRUE;
    RtpAddNewSession(rtp, session);
    RtpMappingAddNewEntry(rtp, localPort);
    rtp->totalSession += 1;
}


//--------------------------------------------------------------------------
// NAME         : RtpProcessReceivedRtpDataPacket()
//
// PURPOSE      : process RTP data packets after validation is over.
//
// RETURN VALUE : None.
//--------------------------------------------------------------------------
static
void RtpProcessReceivedRtpDataPacket(Node* node,
                                            RtpSession* session,
                                            clocktype currRtpTs,
                                            char* packet,
                                            RtcpSourceInfo* srcInfo)
{
    int diff = 0;
    unsigned int transit = 0;
    RtpPacket* rtpHeader = (RtpPacket*) packet;

    // Update the source database
    if (srcInfo->sender == FALSE)
    {
        srcInfo->sender = TRUE;
        session->senderCount++;
    }

    transit    = (unsigned int)
                    ((currRtpTs / MILLI_SECOND) - rtpHeader->timeStamp);
    diff          = transit - srcInfo->transit;
    srcInfo->transit = transit;

    if (diff < 0)
    {
        diff = -diff;
    }
    srcInfo->jitter += diff - ((srcInfo->jitter + 8) / 16);
    srcInfo->totalJitter += srcInfo->jitter;
    //Currently, we support only fixed rtp header
#if 0
    int csrcCount = 0;
    char* csrcPtr = NULL;
    if (rtpHeader->cc > 0)
    {
        // Skip from RTP Header.
        csrcPtr = packet + sizeof(RtpPacket);

        for (csrcCount = 0; csrcCount < RtpPacketGetCc(rtpHeader->rtpPkt);
            csrcCount++)
        {
            RtpCreateSourceInfoInRtpDatabase(
                session, *(unsigned int*)(csrcPtr + csrcCount), FALSE, node);
        }
    }
#endif
}


//--------------------------------------------------------------------------
// NAME         : RtpValidateReceivedRtpDataPacketAgain()
//
// PURPOSE      : second stage validation of received RTP data packets.
//
// RETURN VALUE : BOOLEAN.
//--------------------------------------------------------------------------
static
BOOL RtpValidateReceivedRtpDataPacketAgain(RtpPacket* rtpHeader,
                                                  int len)
{
    // Check for valid payload types..... 72-76 are RTCP payload type
    // numbers, with the high bit missing so we report that someone is
    // running on the wrong port.

    if (RtpPacketGetPt(rtpHeader->rtpPkt) >= 72 &&
        RtpPacketGetPt(rtpHeader->rtpPkt) <= 76)
    {
        if (DEBUG_RTP)
        {
            printf("RTP HEADER VALIDATION: payload-type invalid\n");
        }
        return FALSE;
    }

    // Check that the length of the packet is sensible.
    if (len < (int) (sizeof(RtpPacket) + (RTP_CSRC_SIZE *
        RtpPacketGetCc(rtpHeader->rtpPkt))))
    {
        if (DEBUG_RTP)
        {
            printf("RTP HEADER VALIDATION: packet length is "
                   "smaller than the header\n");
        }
        return FALSE;
    }

    // Check that the amount of padding specified is sensible.
    // Note: have to include the size of any extension header
    if (RtpPacketGetP(rtpHeader->rtpPkt))
    {
        // NOT implemented

      //  int payloadLength = len - sizeof(RtpPacket) -
      //                          (RTP_CSRC_SIZE * rtpHeader->cc);

//        if (rtpHeader->x)
//        {
            // extension header and data.
            // payloadLength -= 4 * (1 + packet->extnLength);
//        }
//        if (packet->data[packet->dataLength - 1] > payloadLength)
//        {
//            if (DEBUG_RTP)
//            {
//                printf("RTP HEADER VALIDATION: padding greater than payload "
//                       "length\n");
//            }
//            return FALSE;
//        }
//        if (packet->data[packet->dataLength - 1] < 1)
//        {
//            if (DEBUG_RTP)
//            {
//                printf("RTP HEADER VALIDATION: padding zero\n");
//            }
//            return FALSE;
//        }
    }
    return TRUE;
}


//--------------------------------------------------------------------------
// NAME         : RtpValidateReceivedRtpDataPacket()
//
// PURPOSE      : first stage validation of received RTP data packets.
//
// RETURN VALUE : BOOLEAN.
//--------------------------------------------------------------------------
static
BOOL RtpValidateReceivedRtpDataPacket(RtpSession* session,
                                              RtpPacket* rtpHeader,
                                              int len)
{
    // This function checks the header info to make sure that the packet
    // is valid. We return TRUE if the packet is valid, FALSE otherwise.
    // See Appendix A.1 of the RTP specification.

    // We only accept RTPv2 packets.
    if (RtpPacketGetV(rtpHeader->rtpPkt) != RTP_VERSION)
    {
        if (DEBUG_RTP)
        {
            printf("RTP HEADER VALIDATION: v != 2\n");
        }
        return FALSE;
    }

    if (session->payloadType != RtpPacketGetPt(rtpHeader->rtpPkt))
    {
        if (DEBUG_RTP)
        {
            printf("RTP HEADER VALIDATION: "
                "Invalid paylode, not supported by the session\n");
        }
        return FALSE;
    }
    return RtpValidateReceivedRtpDataPacketAgain(rtpHeader, len);
}


//--------------------------------------------------------------------------
// NAME         : RtpHandleDataFromApp()
//
// PURPOSE      : This is a interface function provided for handling data
//                from voip app. RTP will now take the responsibility to
//                transmit data to the receiver end.
//
// RETURN VALUE : Message* : pointer to the sent message.
//--------------------------------------------------------------------------
Message* RtpHandleDataFromApp(
    Node* node,
    Address srcAddr,
    Address destAddr,
    char* payload,
    UInt32 payloadSize,
    UInt8 payloadType,
    UInt32 samplingTime,
    TosType tos,
    UInt16 localPort
#ifdef ADDON_DB
    ,
    StatsDBAppEventParam* appParam
#endif
    )
{
    // App currently does not provide any specific values
    // When it will decide to do so we will populate them accordingly
    RtpPacket*      rtpHeader   = NULL;
    int             m           = 0;
    int             cc          = 0;
    char*           packet      = NULL;
    char*           data        = NULL;
    unsigned char*  extn        = NULL;
    unsigned int    dataLength  = payloadSize;
    RtpData*        rtp         = (RtpData*) node->appData.rtpData;

    if (DEBUG_RTP)
       printf("Calling 1 RtpGetSessionPtrByDestAddr\n");
    RtpSession* session = RtpGetSessionPtrByDestAddr(rtp, destAddr,
                                                     localPort, FALSE);
    if (!session)
    {
        return NULL;
    }

    RtpCheckRtpSourceDatabase(session);
    ERROR_Assert(dataLength > 0,
        "Payload size should be greater than zero\n");

    // Allocate memory for the packet
    if (cc == 0 && extn == NULL)
    {
        packet = (char*) MEM_malloc(sizeof(RtpPacket) + payloadSize);
        memset(packet, 0, (sizeof(RtpPacket)+payloadSize));
        dataLength += sizeof(RtpPacket);
    }
    else
    {
        ERROR_Assert(FALSE, "Not supported CSRC list and Hader extn\n");
    }
//    else
//    {
//        packet = (char* ) MEM_malloc(sizeof(RtpPacket) + payloadSize +
//                    (cc * sizeof(CSRC)));
//        dataLength += sizeof(RtpPacket);
//    }

    // The fixed RTP Packet header.
    rtpHeader = (RtpPacket*) packet;
    RtpPacketSetV(&(rtpHeader->rtpPkt), RTP_VERSION);
    RtpPacketSetP(&(rtpHeader->rtpPkt), FALSE);
    RtpPacketSetX(&(rtpHeader->rtpPkt), FALSE);
    RtpPacketSetCc(&(rtpHeader->rtpPkt), cc);
    RtpPacketSetM(&(rtpHeader->rtpPkt), m);
    RtpPacketSetPt(&(rtpHeader->rtpPkt), payloadType);
    rtpHeader->seqNum = session->rtpSeqNum++;
    rtpHeader->timeStamp = samplingTime;
    rtpHeader->ssrc = RtpGetOwnSsrc(session);

    // Skip RTP header
    data = packet + sizeof(RtpPacket);

    // Copy Application data
    memcpy(data, payload, payloadSize);

    if (DEBUG_RTP)
    {
        char addrStr[MAX_STRING_LENGTH];
        IO_ConvertIpAddressToString(&destAddr, addrStr);
        printf("A RTP packet from source node %u of size %d sent\n"
               "to destination address %s with seq no %d,rtp header size %"
               TYPES_SIZEOFMFT "u \n",
                              node->nodeId,
                              dataLength,
                              addrStr,rtpHeader->seqNum,
                              sizeof(RtpPacket));
        printf("RTP pkt being sent from %d to %d\n",session->localPort,
                                                    session->remotePort);
    }

    session->rtpPacketCount += 1;
    session->rtpByteCount += dataLength;
    //for creating Test environment
    clocktype randomDelay = 0;

    // To simulate a random delay to the packets, to send them with
    // different delay. This will be useful for Jitter buffer testing
    // if (session->jitter_Buffer_Enable == TRUE)
    // {
    //    randomDelay = RANDOM_erand(rtpData->seed) * 100 * MILLI_SECOND;
    // }

    // Call interface function to send packet to UDP
    Message *sentMsg = APP_UdpCreateMessage(
        node,
        srcAddr,
        session->localPort,
        destAddr,
        (AppType)session->remotePort,
        TRACE_RTP,
        tos);

    APP_AddPayload(
        node,
        sentMsg,
        packet,
        dataLength);

    APP_UdpSend(
        node,
        sentMsg,
        randomDelay
#ifdef ADDON_DB
        ,
        appParam
#endif
    );

    MEM_free(packet);

    // Update the RTCP statistics.
    session->weSentFlag = TRUE;
    if (DEBUG_RTP)
    {
        printf("Node %u making weSent = true\n",node->nodeId);
    }
    session->lastRtpPacketSendTime = node->getNodeTime();

    return sentMsg;
}

//--------------------------------------------------------------------------
// NAME         : RtpGetSessionControlStats()
//
// PURPOSE      : This is a interface function to return the RTP stats
//                collected for the session.
//
// RETURN VALUE : None.
//--------------------------------------------------------------------------
void RtpGetSessionControlStats(
    Node* node,
    Address destAddr,
    unsigned short localPort,
    RtpStats *stats)
{

    RtpData*  rtp= (RtpData*) node->appData.rtpData;
    RtcpSourceInfo* srcInfo = NULL;

    if (DEBUG_RTP)
    {
        printf("In RtpGetSessionControlStats\n");
    }
    RtpSession* session = RtpGetSessionPtrByDestAddr(rtp, destAddr,
                                                     localPort, FALSE);

    RtpCheckRtpSourceDatabase(session);
    ERROR_Assert(session, "No active RTP session found\n");

    stats->rtpPacketCount = session->rtpPacketCount;
    stats->rtpByteCount = session->rtpByteCount;
    stats->pktReceived = session->pktReceived;
    stats->bytesReceived = session->bytesReceived;
    stats->probPktLostCount = session->probPktDropCount;
    stats->invalidRtpPktCount = session->invalidRtpPktCount;
    stats->invalidRtcpPktCount = session->invalidRtcpPktCount;

    if (session->pktReceived == 0)
    {
        stats->delay = 0;
    }
    else
    {
        stats->delay =
            session->totalEndToEndDelay / session->pktReceived;
    }

    if ((srcInfo = RtpGetSourceInfoFromRtpDatabase(session,
                                              session->remoteSsrc)) != NULL)
    {
        if (srcInfo->received == 0)
        {
            stats->avgJitter =  0;

            if (DEBUG_RTP)
            {
                printf("In RtpGetSessionControlStats:srcinfo->rcvd is 0\n");
            }
        }
        else
        {
            if (DEBUG_RTP)
            {
                printf("In RtpGetSessionControlStats:srcinfo->rcvd "
                       "is non-0\n");
            }
            stats->avgJitter = (unsigned int) ((srcInfo->totalJitter /
                                srcInfo->received) * MILLI_SECOND);
        }
    }
    else
    {
        if (DEBUG_RTP)
        {
            printf("In RtpGetSessionControlStats:srcinfo is null\n");
        }
    }
    if (rtp && rtp->jitterBufferEnable == TRUE)
    {
        RtpJitterBuffer* sessionJitterBuffer = session->jitterBuffer;
        stats->packetDropped = sessionJitterBuffer->packetDropped;
        stats->maxConsecutivePacketDropped =
               sessionJitterBuffer->maxConsecutivePacketDropped;
    }
    return;
}
//--------------------------------------------------------------------------
// NAME         : RtpLayer()
// PURPOSE      : Handle RTP protocol event.
// RETURN VALUE : None.
//--------------------------------------------------------------------------
void RtpLayer(Node* node, Message* msg)
{
    RtpData* rtp = (RtpData*) node->appData.rtpData;

    if (rtp == NULL)
    {
        MESSAGE_Free(node, msg);
        return;
    }

    switch (msg->eventType)
    {
        case MSG_APP_FromTransport:
        {
            // This routine preprocesses an incoming RTP packet, deciding
            // whether to process it.
            RtpPacket*  rtpHeader       = NULL;
            char*       packet          = (char*) MESSAGE_ReturnPacket(msg);
            int         packetLength    = MESSAGE_ReturnPacketSize(msg);
            int         payloadLen      = 0;
            RtcpSourceInfo* srcInfo         = NULL;

            // Get the current simulation time
            clocktype currRtpTs = node->getNodeTime();
            UdpToAppRecv* info  = (UdpToAppRecv*) MESSAGE_ReturnInfo(msg);

            if (DEBUG_RTP)
            {
              printf("\n\n Calling RtpGetSessionPtrByDestAddr\n");
            }
            RtpSession* session = RtpGetSessionPtrByDestAddr(
                                    rtp, info->sourceAddr,
                                    info->sourcePort, TRUE);

            if (session == NULL)
            {
               break;
            }
            ERROR_Assert(session->activeNow, "Session not active now\n");

            if (DEBUG_RTP)
            {
                char addrStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(&info->sourceAddr, addrStr);
                printf("A RTP packet from source %s of size %d came\n"
                       "on node %u\n", addrStr, packetLength, node->nodeId);
            }

            rtpHeader = (RtpPacket*) packet;

            if (RtpPacketGetCc(rtpHeader->rtpPkt))
            {
                // NOT implemented
            }

            if (RtpPacketGetX(rtpHeader->rtpPkt))
            {
                // NOT implemented
            }

            if (RtpValidateReceivedRtpDataPacket(
                    session, rtpHeader, packetLength))
            {
                if (DEBUG_RTP)
                {
                    printf("RTP: Creating source info for node %d\n",
                        node->nodeId);
                }
                srcInfo = RtpCreateSourceInfoInRtpDatabase(session,
                                                     rtpHeader->ssrc,
                                                     TRUE,
                                                     node);

                if (DEBUG_RTP)
                {
                    printf("NODE %d: Packet from "
                        " source received with seq no %d\n", node->nodeId,
                                                          rtpHeader->seqNum);
                }
                if (srcInfo->probation == -1)
                {
                    srcInfo->probation = RTP_MIN_SEQUENTIAL;
                    srcInfo->maxSeqNum =
                        (unsigned short)(rtpHeader->seqNum - 1);
                }

                if (session->jitter_Buffer_Enable == TRUE)
                {
                    RtpIsJitterBufferEnabled(node,
                                             session,
                                             rtpHeader,
                                             msg);
                    srcInfo->received++;
                    return; //Can not Free Msg,
                }

                if (RtpUpdateSequenceNumber(srcInfo,
                                            rtpHeader->seqNum))
                {
                    session->pktReceived += 1;
                    session->bytesReceived += packetLength;
                    session->totalEndToEndDelay += (node->getNodeTime() -
                            (rtpHeader->timeStamp * MILLI_SECOND));
                    session->remoteSsrc = rtpHeader->ssrc;
                    if (DEBUG_RTPJITTER)
                    {
                      printf("Updated ssrc W/o jitter %u for node %d\n",
                      session->remoteSsrc,node->nodeId);
                    }
                    RtpProcessReceivedRtpDataPacket(node,
                                                    session,
                                                    currRtpTs,
                                                    packet,
                                                    srcInfo);

                    if (!(RtpPacketGetX(rtpHeader->rtpPkt)))
                    {
                        payloadLen = packetLength - sizeof(RtpPacket) -
                                        (RTP_CSRC_SIZE *
                                        RtpPacketGetCc(rtpHeader->rtpPkt));
                        packet = packet + sizeof(RtpPacket) +
                                        (RTP_CSRC_SIZE *
                                        RtpPacketGetCc(rtpHeader->rtpPkt));
                    }
                    else
                    {
                        ERROR_Assert(FALSE,
                            "RTP header extension NOT implemented\n");
                    }

                    if (DEBUG_RTP)
                    {
                        printf("RTP packet Node %d "
                            "Sequence Number Without Jitter Buffer"
                            "%u\n", node->nodeId, rtpHeader->seqNum);
                        char addrStr[MAX_STRING_LENGTH];
                        IO_ConvertIpAddressToString(&info->sourceAddr,
                                                    addrStr);
                        printf("Sending data to App w/o jitter, src add %s\n",
                               addrStr);
                    }

                    Message* newMsg = MESSAGE_Alloc(node,
                                                    msg->layerType,
                                                    msg->protocolType,
                                                    msg->eventType);
                    MESSAGE_PacketAlloc(node,
                                        newMsg,
                                        payloadLen,
                                        (TraceProtocolType)msg->protocolType);
                    memcpy(newMsg->packet, packet, payloadLen);
                    MESSAGE_CopyInfo(node, newMsg, msg);

                    (*(void (*)(Node*, Message*, Address, Int32, Int32))session->
                            sendFuncPtr)
                    (node,
                    newMsg,
                    info->sourceAddr,
                    payloadLen,
                    session->call_initiator);

                    RtpUpdateRtpDatabase(node, session);
                    break;
                }
                else
                {
                    session->probPktDropCount++;
                    // This source is still on probation
                    if (DEBUG_RTP)
                    {
                        printf("RTP packet from probationary source "
                               "ignored\n");
                    }
                    break;
                }
            }
            else
            {
                session->invalidRtpPktCount++;
                if (DEBUG_RTP)
                {
                    printf("Invalid RTP packet discarded\n");
                }
            }
            break;
        }

        case MSG_APP_JitterNominalTimer:
        {
            //Nominal Delay timer Expired
            unsigned short localPort = 0;
            Address srcAddr;
            char* info  = MESSAGE_ReturnInfo(msg);

            memcpy(&srcAddr, info, sizeof(Address));
            info += sizeof(Address);
            memcpy(&localPort, info, sizeof(unsigned short));
            if (DEBUG_RTP)
            {
              printf("\n\n Calling 2 RtpGetSessionPtrByDestAddr\n");
            }
            RtpSession* session = RtpGetSessionPtrByDestAddr(
                    rtp, srcAddr, localPort, TRUE);

            if (session == NULL)
            {
               return;
            }

            RtpOnNominalDelayExpired(node, session, msg);

            return;
        }

        case MSG_APP_TalkspurtTimer:
        {
            //Talk Spurt Timer Expired
            unsigned short localPort = 0;
            Address srcAddr;
            char* info  = MESSAGE_ReturnInfo(msg);

            memcpy(&srcAddr, info, sizeof(Address));
            info += sizeof(Address);
            memcpy(&localPort, info, sizeof(unsigned short));
            if (DEBUG_RTP)
            {
              printf("\n\n Calling 3 RtpGetSessionPtrByDestAddr\n");
            }

            RtpSession* session = RtpGetSessionPtrByDestAddr(
                    rtp, srcAddr, localPort, TRUE);

            if (session == NULL)
            {
              return;
            }

            RtpOnTalkSpurtTimerExpired(node, session, msg);
            return;
        }
        case MSG_RTP_TerminateSession:
        {
            Address remoteAddr;
            unsigned short initiatorPort = 0;
            char* msgInfo = MESSAGE_ReturnInfo(msg);

            memcpy(&remoteAddr, msgInfo, sizeof(Address));
            msgInfo += sizeof(Address);
            memcpy(&initiatorPort, msgInfo, sizeof(unsigned short));
            RtpTerminateSession(node, remoteAddr, initiatorPort);
            break;
        }
        case MSG_RTP_InitiateNewSession:
        {
            Address localAddr;
            char srcAliasAddress[MAX_STRING_LENGTH];
            Address remoteAddr;
            clocktype packetizationInterval = 0;
            unsigned short localPort = 0;
            unsigned short applicationPayloadType = 0;
            BOOL initiator = FALSE;
            void* RTPsendfuncptr=NULL;
            char* msgInfo = MESSAGE_ReturnInfo(msg);

            memcpy(&localAddr, msgInfo, sizeof(Address));
            msgInfo += sizeof(Address);
            memcpy(srcAliasAddress, msgInfo, MAX_STRING_LENGTH);
            msgInfo += MAX_STRING_LENGTH;
            memcpy(&remoteAddr, msgInfo, sizeof(Address));
            msgInfo += sizeof(Address);
            memcpy(&packetizationInterval, msgInfo, sizeof(clocktype));
            msgInfo += sizeof(clocktype);
            memcpy(&localPort, msgInfo, sizeof(unsigned short));
            msgInfo += sizeof(unsigned short);
            memcpy(&applicationPayloadType, msgInfo, sizeof(unsigned short));
            msgInfo += sizeof(unsigned short);
            memcpy(&initiator, msgInfo, sizeof(int));
            msgInfo += sizeof(int);
            memcpy(&RTPsendfuncptr, msgInfo, sizeof(void*));

            RtpInitiateNewSession(node, localAddr, srcAliasAddress,
                                remoteAddr, packetizationInterval,
                                localPort,
                                applicationPayloadType,
                                initiator,
                                RTPsendfuncptr);

            break;
        }
        case MSG_RTP_SetJitterBuffFirstPacketAsTrue:
        {
            Address remoteAddr;
            unsigned short initiatorPort = 0;
            char* msgInfo = MESSAGE_ReturnInfo(msg);

            memcpy(&remoteAddr, msgInfo, sizeof(Address));
            msgInfo += sizeof(Address);
            memcpy(&initiatorPort, msgInfo, sizeof(unsigned short));
            RtpSetJitterBufferFirstPacketAsTrue(node, remoteAddr,
                                                initiatorPort);
            if (DEBUG_RTP)
            {
                 printf("In function Rtplayergot event"
                     " setjitterbufffirstpcktrue\n");
            }
            break;
        }
        default:
        {
            ERROR_Assert(FALSE, "Packet other than RTP type\n");
        }
    }
    MESSAGE_Free(node, msg);
}


//--------------------------------------------------------------------------
// NAME         : RtcpLayer()
// PURPOSE      : Handle RTCP protocol event.
// RETURN VALUE : None.
//--------------------------------------------------------------------------
void RtcpLayer(Node* node, Message* msg)
{
    RtpData* rtp = (RtpData*) node->appData.rtpData;

    if (rtp == NULL)
    {
        MESSAGE_Free(node, msg);
        return;
    }

    switch (msg->eventType)
    {
        case MSG_APP_TimerExpired:
        {
            AppGenericTimer* timer =
                (AppGenericTimer*) MESSAGE_ReturnInfo(msg);
            RtpSession* session = RtcpGetSessionPtrByDestAddr(
                                         rtp,
                                         timer->address,
                                         timer->sourcePort, TRUE);

            if (session && session->activeNow)
            {
                RtcpGetNextControlPacketSendTime(node,
                                                 session,
                                                 node->getNodeTime());
            }
            else
            {
                break;
            }

            RtcpScheduleChkSendTimeOfNextRtcpPkt(node, session);
            break;
        }

        case MSG_APP_FromTransport:
        {
            unsigned char    buffer[RTP_MAX_PACKET_LEN];
            UdpToAppRecv* info = (UdpToAppRecv*) MESSAGE_ReturnInfo(msg);
            RtpSession* session = RtcpGetSessionPtrByDestAddr(
                                         rtp,
                                         info->sourceAddr,
                                         info->sourcePort, TRUE);
            int buflen = MESSAGE_ReturnPacketSize(msg);

            if (DEBUG_RTCP)
            {
                char addrStr[MAX_STRING_LENGTH];
                IO_ConvertIpAddressToString(&info->sourceAddr, addrStr);
                printf("A RTCP packet from source %s of size %d came\n"
                       "on node %u\n", addrStr, buflen, node->nodeId);
            }
            memcpy(buffer, MESSAGE_ReturnPacket(msg), buflen);

            if (session && session->activeNow)
            {
                RtcpProcessControlPacket(node, session, buffer, buflen);
            }
            else
            {
               if (DEBUG_RTCP)
               {
                  printf("RTCP: pkt recvd but not incrementing stats and"
                      "processing as session no longer active\n");
               }
            }
            break;
        }
        default:
        {
            if (DEBUG_RTCP)
            {
                printf("Unknown TimerType came on node %d\n", node->nodeId);
            }
            ERROR_Assert(FALSE, "Unknown TimerType\n");
        }
    }
    MESSAGE_Free(node, msg);
}


//---------------------------------------------------------------------------
// NAME         : RtpPrintSessionStats()
// PURPOSE      : Print statistics for rtp session.
// ASSUMPTION   : None.
// RETURN VALUE : None.
//---------------------------------------------------------------------------
static
void RtpPrintSessionStats(Node* node, RtpSession* session)
{
    char startStr[MAX_STRING_LENGTH];
    char closeStr[MAX_STRING_LENGTH];
    char sessionStatusStr[MAX_STRING_LENGTH];
    char clockStr[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    char remoteAddr[MAX_STRING_LENGTH];
    char localAddr[MAX_STRING_LENGTH];

    RtcpSourceInfo* srcInfo = NULL;
    clocktype avgJitter = 0;
    unsigned short maxSeq = 0;
    unsigned int remoteSsrc = RTP_INVALID_SSRC;
    unsigned int pktReceived = 0;
    unsigned int octetsReceived = 0;

    TIME_PrintClockInSecond(session->sessionStart, startStr);
    if (session->activeNow)
    {
        sprintf(sessionStatusStr, "Active");
        TIME_PrintClockInSecond(node->getNodeTime(), closeStr);
    }
    else
    {
        sprintf(sessionStatusStr, "Closed");
        TIME_PrintClockInSecond(session->sessionEnd, closeStr);
    }

    if (session->pktReceived == 0)
    {
        TIME_PrintClockInSecond(0, clockStr);
    }
    else
    {
        TIME_PrintClockInSecond(
            session->totalEndToEndDelay / session->pktReceived,
            clockStr);
    }

    IO_ConvertIpAddressToString(&session->remoteAddr, remoteAddr);
    IO_ConvertIpAddressToString(&session->localAddr, localAddr);

    if (DEBUG_RTP)
    {
       printf("In RTP print stats:"
                 "Get Source info for ssrc %d for node %d\n",
                  session->remoteSsrc,node->nodeId);
    }
    if ((srcInfo = RtpGetSourceInfoFromRtpDatabase(session,
                                               session->remoteSsrc)) != NULL)
    {
        if (srcInfo->received == 0)
        {
            avgJitter      =  0;
            maxSeq         =  0;
            pktReceived    =  0;
            octetsReceived =  0;
            if (DEBUG_RTP)
            {
              printf("srcinfo->rcvd is 0\n");
            }
        }
        else
        {
            if (DEBUG_RTP)
            {
              printf("srcinfo->rcvd is non-0\n");
            }

            avgJitter = (clocktype) ((srcInfo->totalJitter /
                                srcInfo->received) * MILLI_SECOND);

            maxSeq         = srcInfo->maxSeqNum;
            remoteSsrc     = srcInfo->ssrc;
            if (DEBUG_RTPJITTER)
            {
                printf("Updating ssrc %u for node %d\n",
                   remoteSsrc,node->nodeId);
            }
            pktReceived    = srcInfo->received;
            octetsReceived = session->bytesReceived;
        }
    }
    else
    {
       if (DEBUG_RTP)
       {
           printf("srcinfo is null\n");
       }
    }
    sprintf(buf, "Remote Address = %s", remoteAddr);
    IO_PrintStat(
        node,
        "Application",
        "RTP",
        ANY_DEST,
        session->ownSsrc,
        buf);

    if (remoteSsrc == RTP_INVALID_SSRC)
    {
        sprintf(buf, "Remote SSRC = %s", "INVALID SSRC");
            IO_PrintStat(
            node,
            "Application",
            "RTP",
            ANY_DEST,
            session->ownSsrc,
            buf);
    }
    else
    {
        sprintf(buf, "Remote SSRC = %u", remoteSsrc);
        IO_PrintStat(
            node,
            "Application",
            "RTP",
            ANY_DEST,
            session->ownSsrc,
            buf);
    }

    sprintf(buf, "Session Started At (s) = %s", startStr);
    IO_PrintStat(
        node,
        "Application",
        "RTP",
        ANY_DEST,
        session->ownSsrc,
        buf);

    sprintf(buf, "Session Closed At (s) = %s", closeStr);
    IO_PrintStat(
        node,
        "Application",
        "RTP",
        ANY_DEST,
        session->ownSsrc,
        buf);

    sprintf(buf, "Session Status = %s", sessionStatusStr);
    IO_PrintStat(
        node,
        "Application",
        "RTP",
        ANY_DEST,
        session->ownSsrc,
        buf);

    sprintf(buf, "Number of Packets Sent = %u", session->rtpPacketCount);
    IO_PrintStat(
        node,
        "Application",
        "RTP",
        ANY_DEST,
        session->ownSsrc,
        buf);

    sprintf(buf, "Number of Bytes Sent = %u", session->rtpByteCount);
    IO_PrintStat(
        node,
        "Application",
        "RTP",
        ANY_DEST,
        session->ownSsrc,
        buf);

    sprintf(buf, "Number of Packets Received = %u", session->pktReceived);
    IO_PrintStat(
        node,
        "Application",
        "RTP",
        ANY_DEST,
        session->ownSsrc,
        buf);

    sprintf(buf, "Number of Bytes Received = %u", session->bytesReceived);
    IO_PrintStat(
        node,
        "Application",
        "RTP",
        ANY_DEST,
        session->ownSsrc,
        buf);

    sprintf(buf, "Number of Packets Dropped Due To Source Validation = %u",
        session->probPktDropCount);
    IO_PrintStat(
        node,
        "Application",
        "RTP",
        ANY_DEST,
        session->ownSsrc,
        buf);

    sprintf(buf, "Number of Invalid Packets Discard = %u",
        session->invalidRtpPktCount);
    IO_PrintStat(
        node,
        "Application",
        "RTP",
        ANY_DEST,
        session->ownSsrc,
        buf);

    sprintf(buf, "Average End-to-End Delay (s) = %s", clockStr);
    IO_PrintStat(
        node,
        "Application",
        "RTP",
        ANY_DEST,
        session->ownSsrc,
        buf);

    TIME_PrintClockInSecond(avgJitter, clockStr);
    sprintf(buf, "Average Jitter (s) = %s", clockStr);
    IO_PrintStat(
        node,
        "Application",
        "RTP",
        ANY_DEST,
        session->ownSsrc,
        buf);

    RtpData* rtp = (RtpData*) node->appData.rtpData;
    if (rtp->jitterBufferEnable == TRUE)
    {
        RtpJitterBuffer* sessionJitterBuffer = session->jitterBuffer;
        sprintf(buf, "Total No. of Packet dropped in Jitter Buffer = %u",
                    sessionJitterBuffer->packetDropped);
        IO_PrintStat(
            node,
            "Application",
            "RTP",
            ANY_DEST,
            session->ownSsrc,
            buf);

        sprintf(buf, "Max. No. Consecutive Packet dropped in Jitter = %u",
                    sessionJitterBuffer->maxConsecutivePacketDropped);
        IO_PrintStat(
            node,
            "Application",
            "RTP",
            ANY_DEST,
            session->ownSsrc,
            buf);
    }
    else
    {
        sprintf(buf, "Max sequence received = %u", maxSeq);
        IO_PrintStat(
        node,
        "Application",
        "RTP",
        ANY_DEST,
        session->ownSsrc,
        buf);
    }
}


//--------------------------------------------------------------------------
// NAME         : RtpPrintStats()
// PURPOSE      : Print statistics for each RTP session.
// RETURN VALUE : None.
//--------------------------------------------------------------------------
static
void RtpPrintStats(Node* node)
{
    RtpData* rtp = (RtpData*) node->appData.rtpData;
    RtpSession* currentSession;
    char buf[MAX_STRING_LENGTH];

    currentSession = rtp->rtpSession;

    sprintf(buf, "Total Sessions = %u", rtp->totalSession);
    IO_PrintStat(
        node,
        "Application",
        "RTP",
        ANY_DEST,
        -1,
        buf);

    while (currentSession)
    {
        RtpPrintSessionStats(node, currentSession);
        currentSession = currentSession->next;
    }
}


//--------------------------------------------------------------------------
// NAME         : RtcpPrintSessionStats()
// PURPOSE      : Print statistics related to RTCP for one rtp session.
// ASSUMPTION   : None.
// RETURN VALUE : None.
//--------------------------------------------------------------------------
static
void RtcpPrintSessionStats(Node* node, RtpSession* session)
{
    char startStr[MAX_STRING_LENGTH];
    char closeStr[MAX_STRING_LENGTH];
    char sessionStatusStr[MAX_STRING_LENGTH];
    char clockStr[MAX_STRING_LENGTH];
    char clockRttStr[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    TIME_PrintClockInSecond(session->sessionStart, startStr);

    if (session->activeNow)
    {
        sprintf(sessionStatusStr, "Active");
        TIME_PrintClockInSecond(node->getNodeTime(), closeStr);
    }
    else
    {
        sprintf(sessionStatusStr, "Closed");
        TIME_PrintClockInSecond(session->sessionEnd, closeStr);
    }

    if ((session->stats.rtcpRRRecvd) == 0)
    {
        TIME_PrintClockInSecond(0, clockRttStr);
    }
    else
    {
       TIME_PrintClockInSecond((session->rndTripTime
                           / session->stats.rtcpRRRecvd), clockRttStr);
    }

   sprintf(buf, "Session Started At (s) = %s", startStr);
    IO_PrintStat(
        node,
        "Application",
        "RTCP",
        ANY_DEST,
        session->ownSsrc,
        buf);

    sprintf(buf, "Session Closed At (s) = %s", closeStr);
    IO_PrintStat(
        node,
        "Application",
        "RTCP",
        ANY_DEST,
        session->ownSsrc,
        buf);

    sprintf(buf, "Session Status = %s", sessionStatusStr);
    IO_PrintStat(
        node,
        "Application",
        "RTCP",
        ANY_DEST,
        session->ownSsrc,
        buf);

    sprintf(buf, "Session Management BandWidth = %f",
        session->rtcpBandWidth);
    IO_PrintStat(
        node,
        "Application",
        "RTCP",
        ANY_DEST,
        session->ownSsrc,
        buf);

    sprintf(buf, "Session SSRC Count = %d", session->ssrcCount);
    IO_PrintStat(
        node,
        "Application",
        "RTCP",
        ANY_DEST,
        session->ownSsrc,
        buf);

    sprintf(buf, "Total Bye Count Received = %u",
        session->byeCountReceived);
    IO_PrintStat(
        node,
        "Application",
        "RTCP",
        ANY_DEST,
        session->ownSsrc,
        buf);

    if (RtcpGetSdesInfoFromRtpDatabase(session,
                                       session->ownSsrc,
                                       RTCP_SDES_CNAME) == NULL)
    {
        sprintf(buf, "Canonical Name of Source = %s", "NONE");
    }
    else
    {
        sprintf(buf, "Canonical Name of Source = %s",
                                         RtcpGetSdesInfoFromRtpDatabase(
                                                           session,
                                                           session->ownSsrc,
                                                           RTCP_SDES_CNAME));
    }

    IO_PrintStat(
        node,
        "Application",
        "RTCP",
        ANY_DEST,
        session->ownSsrc,
        buf);

    if (RtcpGetSdesInfoFromRtpDatabase(session,
                                       session->remoteSsrc,
                                       RTCP_SDES_CNAME) == NULL)
    {
        sprintf(buf, "Canonical Name of Remote Source = %s", "NONE");
    }
    else
    {
        sprintf(buf, "Canonical Name of Remote Source = %s",
                     RtcpGetSdesInfoFromRtpDatabase(session,
                                                    session->remoteSsrc,
                                                    RTCP_SDES_CNAME));
    }

    IO_PrintStat(
        node,
        "Application",
        "RTCP",
        ANY_DEST,
        session->ownSsrc,
        buf);

    TIME_PrintClockInSecond(session->lastRtcpPacketSendTime, clockStr);
    sprintf(buf, "Last RTCP Packet Sent Time At (s) = %s", clockStr);
    IO_PrintStat(
        node,
        "Application",
        "RTCP",
        ANY_DEST,
        session->ownSsrc,
        buf);

    sprintf(buf, "Session Average RTCP Packet Size = %f",
                                                    session->avgRtcpPktSize);
    IO_PrintStat(
        node,
        "Application",
        "RTCP",
        ANY_DEST,
        session->ownSsrc,
        buf);

   if (&session->stats == NULL)
   {
       return;
   }

    sprintf(buf, "Number of RTCP SR Sent = %u", session->stats.rtcpSRSent);
    IO_PrintStat(
        node,
        "Application",
        "RTCP",
        ANY_DEST,
        session->ownSsrc,
        buf);

    sprintf(buf, "Number of RTCP SR Received = %u",
                                session->stats.rtcpSRRecvd);
    IO_PrintStat(
        node,
        "Application",
        "RTCP",
        ANY_DEST,
        session->ownSsrc,
        buf);

    sprintf(buf, "Number of RTCP RR Sent = %u",
            session->stats.rtcpRRSent);
    IO_PrintStat(
        node,
        "Application",
        "RTCP",
        ANY_DEST,
        session->ownSsrc,
        buf);

    sprintf(buf, "Number of RTCP RR Received = %u",
                   session->stats.rtcpRRRecvd);
    IO_PrintStat(
        node,
        "Application",
        "RTCP",
        ANY_DEST,
        session->ownSsrc,
        buf);

    sprintf(buf, "Number of RTCP SDES Sent = %u",
                  session->stats.rtcpSDESSent);
    IO_PrintStat(
        node,
        "Application",
        "RTCP",
        ANY_DEST,
        session->ownSsrc,
        buf);

    sprintf(buf, "Number of RTCP SDES Received = %u",
                   session->stats.rtcpSDESRecvd);
    IO_PrintStat(
        node,
        "Application",
        "RTCP",
        ANY_DEST,
        session->ownSsrc,
        buf);

    sprintf(buf, "Number of RTCP BYE Sent = %u",
                  session->stats.rtcpBYESent);
    IO_PrintStat(
        node,
        "Application",
        "RTCP",
        ANY_DEST,
        session->ownSsrc,
        buf);

    sprintf(buf, "Number of RTCP BYE Received = %u",
                 session->stats.rtcpBYERecvd);
    IO_PrintStat(
        node,
        "Application",
        "RTCP",
        ANY_DEST,
        session->ownSsrc,
        buf);

    sprintf(buf, "Number of RTCP APP Sent = %u",
                  session->stats.rtcpAPPSent);
    IO_PrintStat(
        node,
        "Application",
        "RTCP",
        ANY_DEST,
        session->ownSsrc,
        buf);

    sprintf(buf, "Number of RTCP APP Received = %u",
                  session->stats.rtcpAPPRecvd);
    IO_PrintStat(
        node,
        "Application",
        "RTCP",
        ANY_DEST,
        session->ownSsrc,
        buf);

    TIME_PrintClockInSecond(session->maxRoundTripTime, clockStr);
    sprintf(buf, "Session Maximum Round Trip Delay Time (s) = %s", clockStr);
    IO_PrintStat(
        node,
        "Application",
        "RTCP",
        ANY_DEST,
        session->ownSsrc,
        buf);

    TIME_PrintClockInSecond(session->minRoundTripTime, clockStr);
    sprintf(buf, "Session Minimum Round Trip Delay Time (s) = %s", clockStr);
    IO_PrintStat(
        node,
        "Application",
        "RTCP",
        ANY_DEST,
        session->ownSsrc,
        buf);

    sprintf(buf, "Session Average Round Trip Time (s) = %s", clockRttStr);
    IO_PrintStat(
        node,
        "Application",
        "RTCP",
        ANY_DEST,
        session->ownSsrc,
        buf);

    sprintf(buf, "Number of RTCP Packets Received = %u",
        session->rtcppktRecvd);
    IO_PrintStat(
        node,
        "Application",
        "RTCP",
        ANY_DEST,
        session->ownSsrc,
        buf);

    sprintf(buf, "Number of Total RTCP Packets Sent= %u",
        session->rtcppktSent);
    IO_PrintStat(
        node,
        "Application",
        "RTCP",
        ANY_DEST,
        session->ownSsrc,
        buf);
}


//--------------------------------------------------------------------------
// NAME         : RtcpPrintStats()
// PURPOSE      : Print statistics for each RTP session.
// RETURN VALUE : None.
//--------------------------------------------------------------------------
static
void RtcpPrintStats(Node* node)
{
    RtpData* rtp = (RtpData*) node->appData.rtpData;
    RtpSession* currentSession = NULL;
    char buf[MAX_STRING_LENGTH];

    currentSession = rtp->rtpSession;
    sprintf(buf, "Total Sessions = %u", rtp->totalSession);
    IO_PrintStat(
        node,
        "Application",
        "RTCP",
        ANY_DEST,
        -1,
        buf);

    while (currentSession)
    {
        // Print session stats for this session
        RtcpPrintSessionStats(node, currentSession);
        // After printing clear the Rtp Database for this session
        RtpClearRtpDatabase(node, currentSession);
        // switch to next RTP session if any
        currentSession = currentSession->next;
    }
}


//--------------------------------------------------------------------------
// NAME         : RtpFinalize()
// PURPOSE      : Collect RTP statistics.
// RETURN VALUE : None.
//--------------------------------------------------------------------------
void RtpFinalize(Node* node)
{
        RtpPrintStats(node);
    }


//--------------------------------------------------------------------------
// NAME         : RtcpFinalize()
// PURPOSE      : Collect RTCP statistics.
// RETURN VALUE : None.
//--------------------------------------------------------------------------
void RtcpFinalize(Node* node)
{

        RtcpPrintStats(node);

    }


//---------------------------------------------------------------------------
// NAME         : RtpTerminateSession()
//
// PURPOSE      : Terminate a RTP session.
//
// ASSUMPTION   : remoteRtpPort is not used.
//
// RETURN VALUE : None.
//---------------------------------------------------------------------------
void RtpTerminateSession(
     Node* node,
     Address remoteAddr,
     unsigned short initiatorPort)
{
    RtpData* rtp = (RtpData*) node->appData.rtpData;
    RtpSession* session = NULL;

    session = RtpGetSessionPtrByDestAddr(rtp, remoteAddr, initiatorPort,
                                         FALSE);
    if (session == NULL)
    {
        return;
    }

    if (rtp->jitterBufferEnable == TRUE)
    {
        RtpClearJitterBufferOnCloseSession(node, session);
    }

    // Send BYE packet to indicate that a participant leaves a session
    RtcpSendByePacketNow(node, session);

    if (DEBUG_RTP)
    {
        char clockStr[MAX_STRING_LENGTH];
        char  addrStr[MAX_STRING_LENGTH];

        TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
        IO_ConvertIpAddressToString(&remoteAddr, addrStr);

        printf("RTP: Node %u terminate RTP session"
            " with node %s at %s, setting activeNow false\n",
            node->nodeId, addrStr, clockStr);
    }
    session->sessionEnd = node->getNodeTime();
    session->activeNow  = FALSE;
    rtp->sessionOpened  = FALSE;

    /* Free up the RTP/RTCP ports that were in use */
    if (DEBUG_RTP)
    {
      printf("Freeing up RTP/RTCP ports %d,%d\n",initiatorPort,
                                                initiatorPort+1);
    }
    if (APP_IsFreePort(node,initiatorPort) == FALSE)
    {
        APP_RemoveFromPortTable(node,initiatorPort);
    }
    if (APP_IsFreePort(node,initiatorPort+1) == FALSE)
    {
        APP_RemoveFromPortTable(node,initiatorPort+1);
    }

}

//-------------------------------------------------------------------------
// NAME             : RtpInitializeRtpDataFromConfigFile()
// PURPOSE          : Configure the RtpData structure for Jitter Buffer .
//
// PARAMETERS       :
//          node    : A pointer to Node
//        nodeInput : A pointer to NodeInput
//          rtpData : A Pointer to rtpData
// ASSUMPTIONS      : None
//
// RETURN           : Nothing
//-------------------------------------------------------------------------
void RtpInitializeRtpDataFromConfigFile(Node* node,
                                RtpData* rtpData,
                                const NodeInput* nodeInput)
{
    BOOL wasFound = FALSE;
    int readVal = 0;
    char buf[MAX_STRING_LENGTH];

    // Read the Jitter Buffer Enable from Config file
    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "RTP-JITTER-BUFFER-ENABLED",
        &wasFound,
        buf);

    if ((wasFound == TRUE) && (!(strcmp(buf,"YES"))))
    {
        rtpData->jitterBufferEnable  = TRUE;
    }
    else
    {
        rtpData->jitterBufferEnable = FALSE;
    }

    if (rtpData->jitterBufferEnable == TRUE)
    {
        //read the Maximum Packet of Jitter Buffer from Config file
        IO_ReadInt(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
           "RTP-JITTER-BUFFER-MAXNO-PACKET",
        &wasFound,
        &readVal);

        // Zero nos of Packet is not permitted
        if (wasFound == TRUE && readVal > 0)
        {
            rtpData->maxNoPacketInJitter = readVal;
        }
        else if (wasFound == TRUE && readVal <= 0)
        {
            char warning[MAX_STRING_LENGTH];

            sprintf(warning, "Negative or Zero value of "
               "RTP-JITTERBUFFER-MAXNO-PACKET in Config File."
               "\nTherefore default value has been taken.");
            ERROR_ReportWarning(warning);
            rtpData->maxNoPacketInJitter = DEFAULT_JITTERBUFFER_MAXNO_PACKET;
        }
        else //If Parameter is not specified or enabled in the configuration file
        {
            rtpData->maxNoPacketInJitter = DEFAULT_JITTERBUFFER_MAXNO_PACKET;
        }
        // Read simulation time for putting boundary checks
        BOOL      retVal = FALSE;
        char      endTimeStr[MAX_STRING_LENGTH];
        clocktype endTime = 0;

        IO_ReadString(node->nodeId, ANY_ADDRESS, nodeInput,
                     "SIMULATION-TIME", &retVal, endTimeStr);

        if (retVal == TRUE)
        {
            endTime = TIME_ConvertToClock(endTimeStr);
        }
        else
        {
            ERROR_Assert(FALSE,
                        "SIMULATION-TIME not found in config file.\n");
        }
        //read the Jitter Maximum Delay from Config file
        IO_ReadTime(node->nodeId,
                    ANY_ADDRESS,
                    nodeInput,
                    "RTP-JITTER-BUFFER-MAXIMUM-DELAY",
                    &wasFound,
                    &rtpData->maximumDelayInJitter);

        if (wasFound == FALSE)
        {
            rtpData->maximumDelayInJitter = DEFAULT_JITTER_MAX_DELAY;
        }
        else if (rtpData->maximumDelayInJitter <= 0)
        {
            char warning[MAX_STRING_LENGTH];

            sprintf(warning, "Negative value of "
                    "Jitter Maximum Delay in Config File."
                    "\nTherefore default value has been taken.");
            ERROR_ReportWarning(warning);
            rtpData->maximumDelayInJitter = DEFAULT_JITTER_MAX_DELAY;
        }
        else if (rtpData->maximumDelayInJitter > endTime)
        {
            char error[MAX_STRING_LENGTH];

            sprintf(error, "Jitter Maximum Delay value in Config File "
                              "exceeds simulation time."
                          "\nTherefore exiting.");
            ERROR_ReportError(error);
        }

        //read the talk Spurt Delay from Config file
        IO_ReadTime(node->nodeId,
                    ANY_ADDRESS,
                    nodeInput,
                    "RTP-JITTER-BUFFER-TALKSPURT-DELAY",
                    &wasFound,
                    &rtpData->talkspurtDelayInJitter);

        if (wasFound == FALSE)
        {
            rtpData->talkspurtDelayInJitter = DEFAULT_JITTER_TALKSPURT_DELAY;
        }
        else if (rtpData->talkspurtDelayInJitter <= 0)
        {
            char warning[MAX_STRING_LENGTH];

            sprintf(warning, "Negative value of "
               "Talk Spurt Delay in Config File."
               "\nTherefore default value has been taken.");
            ERROR_ReportWarning(warning);
            rtpData->talkspurtDelayInJitter = DEFAULT_JITTER_TALKSPURT_DELAY;
        }
        else if (rtpData->talkspurtDelayInJitter > endTime)
        {
            char error[MAX_STRING_LENGTH];

            sprintf(error, "Talkspurt Delay value in Config File "
                              "exceeds simulation time."
                          "\nTherefore exiting.");
            ERROR_ReportError(error);
        }
    }

    IO_ReadString(node->nodeId,
                  ANY_ADDRESS,
                  nodeInput,
                  "RTP-STATISTICS",
                  &wasFound,
                  buf);
    if (wasFound && !(strcmp(buf,"YES")))
    {
        rtpData->rtpStats = TRUE;
    }
    else
    {
        rtpData->rtpStats = FALSE;
    }

    IO_ReadInt(node->nodeId,
               ANY_ADDRESS,
               nodeInput,
              "RTCP-SESSION-MANAGEMENT-BANDWIDTH",
              &wasFound,
              &readVal);
    if (wasFound && readVal > 0)
    {
        rtpData->rtcpSessionBandwidth = readVal;
    }
    else if (wasFound && readVal < 0)
    {
        rtpData->rtcpSessionBandwidth = DEFAULT_SESSION_MANAGEMENT_BANDWIDTH;
    }
    else if (!wasFound)
    {
        rtpData->rtcpSessionBandwidth = DEFAULT_SESSION_MANAGEMENT_BANDWIDTH;
    }
}

//-------------------------------------------------------------------------
// NAME         : RtpIsJitterBufferEnabled()
//
// PURPOSE      : Check Whether it is the first Packet.
//                  Calculate Maxdelay in Jitter Buffer. Insert
//                  RTP Packet in Jitter Buffer.
//
// PARAMETERS   :
//        node  : A pointer to NODE type.
//      session : A pointer to RtpSession.
//    rtpHeader : A pointer to RtpPacket.
//       msg    : A Pointer to Message.
//
// ASSUMPTIONS  : None.
//
// RETURN VALUE : void
//-------------------------------------------------------------------------
void RtpIsJitterBufferEnabled(Node* node,
                              RtpSession* session,
                              RtpPacket* rtpHeader,
                              Message* msg)
{
    RtpJitterBuffer* sessionJitterBuffer = NULL;
    int packetLength = MESSAGE_ReturnPacketSize(msg);
    sessionJitterBuffer = session->jitterBuffer;
    ERROR_Assert(sessionJitterBuffer != NULL,
               "JitterBuffer is NULL from RtpIfJitterBufferEnabled()");
    //check whether it is first packet
    if (sessionJitterBuffer->firstPacket == TRUE)
    {
       if (DEBUG_RTPJITTER)
       {
            printf("First RTP pkt recvd\n");
       }
        RtpIsFirstPacketInJitterBuffer(node,
                                        sessionJitterBuffer,
                                        rtpHeader,
                                        msg);
    }
    //Insert the packet into the Buffer
    session->pktReceived += 1;
    session->bytesReceived += packetLength;
    //Dynamically set Jitter Buffer Maximum delay
    clocktype arrivalTime = node->getNodeTime();
    clocktype rtpTimestamp
                = rtpHeader->timeStamp * MILLI_SECOND;
    RtpCalculateAdaptiveJitterBufferMaxDelay(node,
                                     session,
                                     arrivalTime,
                                     rtpTimestamp);

    RtpPacketInsertInJitterBuffer(node,
                                   session,
                                   sessionJitterBuffer,
                                   msg);
    return;
}

//-------------------------------------------------------------------------
// NAME                 : RtpIsFirstPacketInJitterBuffer()
//
// PURPOSE              : Create Message for Nominal Delay
//                        Create Message for TalkSpurt Delay
//
// PARAMETERS           :
//        node          : A pointer to NODE type.
// sessionJitterBuffer  : A pointer to RtpJitterBuffer.
//    rtpHeader         : A Pointer to RtpPacket.
//      msg             : A Pointer to Message.
//
// ASSUMPTIONS  : None.
//
// RETURN VALUE : void
//-------------------------------------------------------------------------
void RtpIsFirstPacketInJitterBuffer(Node* node,
                                    RtpJitterBuffer* sessionJitterBuffer,
                                    RtpPacket* rtpHeader,
                                    Message* msg)
{
    Message* msgNominalDelay = NULL;
    Message* msgTalkSpurt = NULL;
    UdpToAppRecv* info  = (UdpToAppRecv*) MESSAGE_ReturnInfo(msg);
    sessionJitterBuffer->expectedSeq = rtpHeader->seqNum;
    //Create the Nominal Timer Message

    msgNominalDelay = MESSAGE_Alloc(node,
                            APP_LAYER,
                            APP_RTP,
                            MSG_APP_JitterNominalTimer);
    MESSAGE_InfoAlloc(node, msgNominalDelay,
            sizeof(Address) + sizeof(unsigned short));
    Address srcAddr = info->sourceAddr;
    char* msgInfo  = MESSAGE_ReturnInfo(msgNominalDelay);
    memcpy(msgInfo, &srcAddr, sizeof(Address));
    msgInfo += sizeof(Address);
    memcpy(msgInfo, &(info->sourcePort), sizeof(unsigned short));

    MESSAGE_Send(node,
                msgNominalDelay,
                sessionJitterBuffer->nominalDelay);
    sessionJitterBuffer->msgNominalDelay = msgNominalDelay;

    //Create the TalkSpurt Timer Message

    msgTalkSpurt = MESSAGE_Alloc(node,
                          APP_LAYER,
                          APP_RTP,
                          MSG_APP_TalkspurtTimer);
    MESSAGE_InfoAlloc(node,
                      msgTalkSpurt,
                      sizeof(Address) + sizeof(unsigned short));
    msgInfo = MESSAGE_ReturnInfo(msgTalkSpurt);
    memcpy(msgInfo, &srcAddr, sizeof(Address));
    msgInfo += sizeof(Address);
    memcpy(msgInfo, &(info->sourcePort), sizeof(unsigned short));

    MESSAGE_Send(node,
              msgTalkSpurt,
              sessionJitterBuffer->talkspurtDelay);
    sessionJitterBuffer->msgTalkSpurtDelay = msgTalkSpurt;
    //set firstPacket as FALSE
    sessionJitterBuffer->firstPacket = FALSE;
    return;
}

//-------------------------------------------------------------------------
// NAME             : RtpOnNominalDelayExpired()
//
// PURPOSE          : Send next Sequence Message From Jitter buffer to VOIP
//
// PARAMETERS       :
//        node      : A pointer to NODE type.
//     session      : A pointer to RtpSession.
//      msg         : A Pointer to Message.
//
// ASSUMPTIONS  : None.
//
// RETURN VALUE : void
//-------------------------------------------------------------------------
void RtpOnNominalDelayExpired(Node* node,
                              RtpSession* session,
                              Message* msg)
{
    RtpJitterBuffer *sessionJitterBuffer;
    sessionJitterBuffer = session->jitterBuffer;

    //get the packet from jitterBuffer for the Sequence Number
    BOOL voiceOver = FALSE;
    if (DEBUG_RTPJITTER)
    {
        printf("Sending data to App as nominal timer expired"
            " on node %d\n",node->nodeId);
    }
    voiceOver = RtpGetPacketFromJitterBufferListAndSendIt(node, session);
    if (voiceOver == FALSE)
    {
        //scedule the timer
        MESSAGE_Send(node, msg, sessionJitterBuffer->nominalDelay);
    }

    return;
}

//-------------------------------------------------------------------------
// NAME             : RtpOnTalkSpurtTimerExpired()
//
// PURPOSE          : Update Maximum delay in jitter Buffer
//
// PARAMETERS       :
//        node      : A pointer to NODE type.
//     session      : A pointer to RtpSession.
//      msg         : A Pointer to Message.
//
// ASSUMPTIONS  : None.
//
// RETURN VALUE : void
//-------------------------------------------------------------------------
void RtpOnTalkSpurtTimerExpired(Node* node,
                              RtpSession* session,
                              Message* msg)
{
    RtpJitterBuffer *sessionJitterBuffer;
    sessionJitterBuffer = session->jitterBuffer;

    //pi = Di + B * Vi
    double constB = 3.0;
    if (DEBUG_RTPJITTER)
    {
        char clockStr[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(sessionJitterBuffer->maxDelay, clockStr);
        printf("Before Talk Spurt timer expiry Max delay %s ",clockStr);
    }
    sessionJitterBuffer->maxDelay = (clocktype)
                    (sessionJitterBuffer->packetDelay + constB
                        * sessionJitterBuffer->packetVariance);
    if (DEBUG_RTPJITTER)
    {
        char clockStr[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond(sessionJitterBuffer->maxDelay, clockStr);
        printf("Max Delay after Talk spurt timer expiry %s \n ",clockStr);
    }

    //scedule the timer
    MESSAGE_Send(node, msg, sessionJitterBuffer->talkspurtDelay);
    return;
}

//-------------------------------------------------------------------------
// NAME         : RtpGetSequenceNumberofPacket()
//
// PURPOSE      : Get the sequence number of a RTP Packet
//
// PARAMETERS   :
//        data  : A pointer to void type. Normally Contains Message
//
// ASSUMPTIONS  : None.
//
// RETURN VALUE : Sequence number of the packet
//-------------------------------------------------------------------------
unsigned short RtpGetSequenceNumberofPacket(void* data)
{
    Message* msg = (Message*) data;
    char* packet = (char*) MESSAGE_ReturnPacket(msg);
    RtpPacket* rtpHeader = (RtpPacket*) packet;
    unsigned short sequenceNo = rtpHeader->seqNum;

    if (DEBUG_RTPJITTER)
    {
        printf("seq number = %u\n",sequenceNo);
    }
    //return sequence no
    return sequenceNo;
}

//-------------------------------------------------------------------------
// NAME             : RtpPacketInsertInJitterBuffer()
// PURPOSE          : Insert the packet into Jitter Buffer
// RETURNS          : Nothing
// PARAMETRS        :
//             node : The node
//          session : a Pointer to RtpSession Type
//     jitterBuffer : Jitter buffer associated with session
//              msg : Message
// ASSUMPTION       : None
//-------------------------------------------------------------------------

void RtpPacketInsertInJitterBuffer(
    Node* node,
    RtpSession* session,
    RtpJitterBuffer* jitterBuffer,
    Message* msg)
{
    //Insert Message packet into the jitter buffer
    // insert msg pointer along with arrival time
    // when packet comes

    BOOL voiceOver = FALSE;
    LinkedList* listNodeInJitter = jitterBuffer->listJitterNodes;

    if (listNodeInJitter == NULL)
    {
        return;
    }

    // check whether buffer is full
    if (jitterBuffer->maxNoNodesInJitter <= listNodeInJitter->size)
    {
        unsigned short lowestSequenceNo = 0;
        ListItem* packetWithLowestSequenceNo = NULL;
        //Get the Lowest sequence No. Packet
        packetWithLowestSequenceNo =
                     RtpGetLowestSequenceNoPacketFromJitterBufferList(node,
                                       jitterBuffer,
                                       lowestSequenceNo);

        if (packetWithLowestSequenceNo != NULL)
        {
            //Send the Lowest sequence No. Packet to the Application
            if (DEBUG_RTPJITTER)
            {
               printf("Sending packet seq %u to App from Jitter buffer"
                   " as buffer is full\n",lowestSequenceNo);
            }
            voiceOver = RtpSendPacketFromJitterBuffer(node,
                                        session,
                                        packetWithLowestSequenceNo);
            if (voiceOver == TRUE)
            {
                jitterBuffer->expectedSeq += 1;
            }
            else
            {
                //Set the Next Sequence Number
                jitterBuffer->expectedSeq = lowestSequenceNo + 1;
            }

            // Drop the packet From the List of Jitter Buffer
            ListGet(node,
                listNodeInJitter,
                packetWithLowestSequenceNo,
                TRUE,
                TRUE);

            if (DEBUG_RTPJITTER)
            {
                printf("Voip one packet moved out from Jitter to App,"
                    "Now Size is %d\n", listNodeInJitter->size);
            }
        }
    }
    //Check whether the Sequence number of current Message is Greater than
    //  expectedSeq of Jitter Buffer
    unsigned short packetSequenceNo =
        RtpGetSequenceNumberofPacket((void*) msg);

    //Add the message at the end of list
    if ((packetSequenceNo >= jitterBuffer->expectedSeq)
        || ((jitterBuffer->expectedSeq - packetSequenceNo) > 50000))
    {
        ListInsert(node,
            listNodeInJitter,
            node->getNodeTime(),    // current time
            (void*)msg);

        if (DEBUG_RTPJITTER)
        {
            char clockStr[MAX_STRING_LENGTH];
            TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

            printf("RTP packetNode %d,  Sequence Number Inserted at %s "
                        "in Jitter Buffer %u,expected seq no. %u\n",
                        node->nodeId, clockStr,packetSequenceNo,
                        jitterBuffer->expectedSeq);
        }
    }
}

//-------------------------------------------------------------------------
// NAME             : RtpGetLowestSequenceNoPacketFromJitterBufferList()
// PURPOSE          : Get Lowest Sequence Number Packet
//                          From Jitter Buffer List
// RETURNS          : ListItem Pointer
// PARAMETRS        :
//             node : the node
//     jitterBuffer : Jitter buffer associated with session
//              msg : Message
// lowestSequenceNo : Lowest Sequence Number
// ASSUMPTION       : None
//-------------------------------------------------------------------------

ListItem* RtpGetLowestSequenceNoPacketFromJitterBufferList(Node* node,
            RtpJitterBuffer* jitterBuffer, unsigned short& lowestSequenceNo)
{
     // packet having less than seqNo is to be dropped
    LinkedList* listNodeInJitter = jitterBuffer->listJitterNodes;
    ListItem* temp = listNodeInJitter->first;
    ListItem* packetWithLowestSequenceNo = temp;
    unsigned short nodeSequenceNo = 0;

    node = node;

    if (temp == NULL)
    {
        return NULL;
    }

    lowestSequenceNo = RtpGetSequenceNumberofPacket(temp->data);
    while (temp != NULL)
    {
        nodeSequenceNo = RtpGetSequenceNumberofPacket(temp->data);
        if (nodeSequenceNo  < lowestSequenceNo)
        {
            packetWithLowestSequenceNo = temp;
            lowestSequenceNo = nodeSequenceNo;
        }
        temp = temp->next;
    }
    return packetWithLowestSequenceNo;
}


//-------------------------------------------------------------------------
// NAME             : RtpSendPacketFromJitterBuffer()
// PURPOSE          : Send the Packet From JitterBuffer to VOIP according
//                  :        to given Sequence Number
// RETURNS          : BOOL
//                  :   If send packet is Voice over then return TRUE
//                      else FALSE
// PARAMETRS        :
//             node : the node
//          session : a Pointer to RtpSession Type
// packetWithLowestSequenceNo : a pointer to Lowest sequence Packet
// ASSUMPTION       : None
//-------------------------------------------------------------------------

BOOL RtpSendPacketFromJitterBuffer(Node* node,
                     RtpSession* session,
                     ListItem* packetWithLowestSequenceNo)
{

    Message* msg = (Message*) packetWithLowestSequenceNo->data;
    Address add = *((Address *)MESSAGE_ReturnInfo(msg));
    RtpPacket*  rtpHeader       = NULL;
    RtcpSourceInfo* srcInfo     = NULL;
    BOOL voiceOver = FALSE;
    RtpJitterBuffer* sessionJitterBuffer = session->jitterBuffer;

    if (msg != NULL)
    {
        char*       packet          = (char*) MESSAGE_ReturnPacket(msg);
        int         packetLength    = MESSAGE_ReturnPacketSize(msg);
        int         payloadLen      = 0;
        rtpHeader = (RtpPacket*) packet;

        //Whether the last packet is voiceover or not
        voiceOver = RtpIsVoiceOverMessage(node,
                                packetWithLowestSequenceNo);

        //Increment expected sequence no
        if (voiceOver == TRUE)
        {
            if (sessionJitterBuffer->msgNominalDelay != NULL)
            {
                MESSAGE_CancelSelfMsg(node,
                            sessionJitterBuffer->msgNominalDelay);
                sessionJitterBuffer->msgNominalDelay = NULL;
            }

            if (sessionJitterBuffer->msgTalkSpurtDelay != NULL)
            {
                MESSAGE_CancelSelfMsg(node,
                        sessionJitterBuffer->msgTalkSpurtDelay);
                sessionJitterBuffer->msgTalkSpurtDelay = NULL;
            }
        }

        clocktype currRtpTs = node->getNodeTime();
        session->totalEndToEndDelay += currRtpTs
                                - (rtpHeader->timeStamp * MILLI_SECOND);

        session->remoteSsrc = rtpHeader->ssrc;
        if (DEBUG_RTPJITTER)
        {
            printf("Updated ssrc %u for node %d\n",
                  session->remoteSsrc,node->nodeId);
        }
        srcInfo = RtpGetSourceInfoFromRtpDatabase(session,
                                                  rtpHeader->ssrc);
        if (srcInfo != NULL)
        {
            RtpProcessReceivedRtpDataPacket(node,
                                            session,
                                            currRtpTs,
                                            packet,
                                            srcInfo);
        }

        if (!(RtpPacketGetX(rtpHeader->rtpPkt)))
        {
            payloadLen = packetLength - sizeof(RtpPacket)
                              - (RTP_CSRC_SIZE *
                              RtpPacketGetCc(rtpHeader->rtpPkt));
            packet = packet + sizeof(RtpPacket)
                             + (RTP_CSRC_SIZE *
                             RtpPacketGetCc(rtpHeader->rtpPkt));
        }
        else
        {
            ERROR_Assert(FALSE,
                       "RTP header extension NOT implemented\n");
        }
        if (DEBUG_RTPJITTER)
        {
            char clockStr[MAX_STRING_LENGTH];
            TIME_PrintClockInSecond(
                session->jitterBuffer->maxDelay, clockStr);

            printf("Max Delay %s ", clockStr);
            TIME_PrintClockInSecond(node->getNodeTime(), clockStr);
            printf("RTP Node %d,Sequence Number Goes out at %s From Jitter "
              "Buffer %u\n", node->nodeId, clockStr, rtpHeader->seqNum);
            char addrStr[MAX_STRING_LENGTH];
            IO_ConvertIpAddressToString(&add, addrStr);
            printf("Sending data to App from jitter buffer, src add %s\n",
                addrStr);
        }

        Message* newMsg = MESSAGE_Alloc(node,
                           msg->layerType,
                           msg->protocolType,
                           msg->eventType);
        MESSAGE_PacketAlloc(node,
                            newMsg,
                            payloadLen,
                            (TraceProtocolType)msg->protocolType);
        memcpy(newMsg->packet, packet, payloadLen);
        MESSAGE_CopyInfo(node, newMsg, msg);

        (*(void (*)(Node*, Message*, Address, Int32, Int32))session->
                sendFuncPtr)
                   (node,
                   newMsg,
                   add,
                   payloadLen,
                   session->call_initiator);

        RtpUpdateRtpDatabase(node, session);
    }

    return voiceOver;
}

//-------------------------------------------------------------------------
// NAME         : RtpGetPacketFromJitterBufferListAndSendIt()
// PURPOSE      : Get the packet from Jitter Buffer List according
//                  to Sequence Number  and Send it
// RETURNS      : BOOL
// PARAMETERS   :
//         node : the Node
//      session : A Pointer of RtpSession
// ASSUMPTION   : First Packet of the Jitter Buffer List
//                  should be Oldest one  as Timestamp
//-------------------------------------------------------------------------
BOOL RtpGetPacketFromJitterBufferListAndSendIt(Node* node,
                                               RtpSession* session)
{
    // Check Timestamp of First Packet ie. oldest packet
    //  in Jitter Buffer List, is greater than Jitter Buffer MaxDelay
    //  Value. If it is, delete all packets with sequence no less than
    //  the sequence no of First Packet. So no one packet with time
    //  stamp more than Maxdelay, can not stay in Jitter Buffer List.

    // get a packet from jitter buffer with seqNo specified
    // delete the packet afterward

    clocktype  currentTime = node->getNodeTime();
    BOOL voiceOver = FALSE;
    RtpJitterBuffer *sessionJitterBuffer = NULL;
    sessionJitterBuffer = session->jitterBuffer;
    unsigned short seqNo = sessionJitterBuffer->expectedSeq;

    ListItem* temp = sessionJitterBuffer->listJitterNodes->first;
    ListItem* listItemPacketData;

    if (temp == NULL)
    {
        if (DEBUG_RTPJITTER)
        {
            printf("No action.Returning\n");
        }
        return voiceOver;
    }

    voiceOver = RtpIsVoiceOverMessage(node,temp);

    // check the first packet(Old one) whether it exceeds maxTime
    if (((currentTime - temp->timeStamp) > sessionJitterBuffer->maxDelay)
                        && (!voiceOver))
    {
        unsigned short sequenceNoOfFirstPacket
                    = RtpGetSequenceNumberofPacket(temp->data);

        if (DEBUG_RTPJITTER)
        {
            char clockStr[MAX_STRING_LENGTH];
            TIME_PrintClockInSecond(node->getNodeTime(), clockStr);

            printf("SimTime %s --Node %d :",clockStr,node->nodeId);
            TIME_PrintClockInSecond((currentTime - temp->timeStamp),
                                clockStr);
            printf("Time Stamp %s ",clockStr);
            printf(",For seq no %u ",sequenceNoOfFirstPacket);
            TIME_PrintClockInSecond(sessionJitterBuffer->maxDelay,
                            clockStr);
            printf(",exceed MaxDelay %s \n ",clockStr);
            printf("Voip Jitter:a  packet exceeds MaxDelay seq no = %u\n",
                     sequenceNoOfFirstPacket);
        }
        //Drop all packet
        RtpDropMaxDelayPacketAndItsFollower(node,
                                        sessionJitterBuffer,
                                        sequenceNoOfFirstPacket);
        //Set New Sequence No after changing it
        // from RtcpJitterBufferDropPacket Metod
        seqNo = sessionJitterBuffer->expectedSeq;
    }

    // get a packet from jitter buffer with seqNo specified
    // if it's match, Get Message Part and return it and also
    // delete that Node from Jitter Buffer List
    temp = sessionJitterBuffer->listJitterNodes->first;
    while (temp !=NULL)
    {
        if (RtpGetSequenceNumberofPacket(temp->data) == seqNo)
        {
            //This Message is the current Sequence Number Message
            listItemPacketData = (ListItem*) temp;

            if (listItemPacketData != NULL)
            {
                //Send the Packet From Jitter Buffer
                voiceOver = RtpSendPacketFromJitterBuffer(node,
                                            session,
                                            listItemPacketData);

                // delete the message from buffer and free the message
                ListGet(node,
                    sessionJitterBuffer->listJitterNodes,
                    temp,
                    TRUE,
                    TRUE);

                //Increment expected sequence no
                sessionJitterBuffer->expectedSeq += 1;

            }
            if (DEBUG_RTPJITTER)
            {
                printf("Voip Jitter: we got the packet of seq no = %u\n",
                                                    seqNo);
            }
            //return from function
            return voiceOver;
        }
        else
        {
            //goto next location
            temp = temp->next;
        }
    }
    //Expected seq number is not found
    if (DEBUG_RTPJITTER)
    {
       printf("No packet sent to App\n");
    }
    return voiceOver;
}

//-------------------------------------------------------------------------
// NAME                       : RtpIsVoiceOverMessage()
// PURPOSE                    : Whether this is Voice over message?
// RETURN VALUE               : Boolean
// Parameters                 :
//             node           : Node
//     packetData             : Packet data in jitter buffer
// sequenceNoOfFirstPacket    : expected seq number
// ASSUMPTIONS      : None
//-------------------------------------------------------------------------

BOOL RtpIsVoiceOverMessage(Node* node, ListItem* packetData)
{
    BOOL voiceOver = FALSE;
    Message* msg = (Message*) packetData->data;
    char* packet = (char*) MESSAGE_ReturnPacket(msg);
    RtpPacket* rtpHeader = (RtpPacket*) packet;

    node = node;
    switch(RtpPacketGetPt(rtpHeader->rtpPkt))
    {
        case  VOIP_PAYLOAD_TYPE:
        {
            VoipData voipData;

            if (!(RtpPacketGetX(rtpHeader->rtpPkt)))
            {
                packet = packet + sizeof(RtpPacket)
                    + (RTP_CSRC_SIZE * RtpPacketGetCc(rtpHeader->rtpPkt));

                memcpy(&voipData, packet, sizeof(VoipData));
                if (voipData.type =='o')
                {
                    voiceOver = TRUE;
                }
            }
        }
    }
    // This is for VOIP any other multimedia application developed
    // should be patched
    return voiceOver;
}
//-------------------------------------------------------------------------
// NAME                       : RtpDropMaxDelayPacketAndItsFollower()
// PURPOSE                    : Drop First packet and also Drop packets
//                                from Jitter Buffer having sequencenumber
//                                less than FirstPacketSequenceNo.
// RETURN VALUE               :  Nothing
// Parameters                 :
//             node           : Node
//     jitterBuffer           : Jitter Buffer
// sequenceNoOfFirstPacket    : expected seq number
// ASSUMPTIONS      : None
//-------------------------------------------------------------------------

void RtpDropMaxDelayPacketAndItsFollower(
    Node* node,
    RtpJitterBuffer* jitterBuffer,
    unsigned short sequenceNoOfFirstPacket)
{
    // packet having less than sequenceNoOfFirstPacket is to be dropped
    ListItem* temp = jitterBuffer->listJitterNodes->first;
    ListItem* tempNext = NULL;
    unsigned short sequenceNoOfPacket = 0;

    //First Packet should be dropped because it's timestamp is already
    // excceds maxDelay.
    Int32 droppedPacketFromJitter = 0;
    while (temp != NULL)
    {
        sequenceNoOfPacket = RtpGetSequenceNumberofPacket(temp->data);
        tempNext = temp->next;

        //drop this element from buffer
        //Delete the message too
        //Take care about the sequence number because sequence number
        //is unsigned short so it's largest number is 65535,
        //then it goes to zero when increment by one. This zero is larger
        //than 65535. That's why we check the difference with 50000.
        if (((sequenceNoOfPacket <= sequenceNoOfFirstPacket)
               && ((sequenceNoOfFirstPacket - sequenceNoOfPacket) < 50000))
             || ((sequenceNoOfFirstPacket <= sequenceNoOfPacket)
            && ((sequenceNoOfPacket - sequenceNoOfFirstPacket) > 50000)))
        {
            ListGet(node,
                jitterBuffer->listJitterNodes,
                temp,
                TRUE,
                TRUE);
            // increment packetDropped by one to show the statistics.
            jitterBuffer->packetDropped += 1;
            droppedPacketFromJitter++;
            if (DEBUG_RTPJITTER)
            {
                printf("Voip Jitter: Node %d a packet dropped "
                    "of seq no = %u\n",
                            node->nodeId,sequenceNoOfPacket);
            }
        }


        temp = tempNext;
    }
    if (droppedPacketFromJitter > (int) jitterBuffer->maxConsecutivePacketDropped)
    {
        jitterBuffer->maxConsecutivePacketDropped = droppedPacketFromJitter;
    }

    //Set the new Sequence No
    jitterBuffer->expectedSeq = sequenceNoOfFirstPacket + 1;
}

//-------------------------------------------------------------------------
// NAME                       : RtpClearJitterBufferOnCloseSession()
// PURPOSE                    : Clear Jitter Buffer on Close Session
// RETURN VALUE               :  Nothing
// Parameters                 :
//             node           : Node
//           session          : A pointer of RtpSession of Remote address
// ASSUMPTIONS                : None
//-------------------------------------------------------------------------

void RtpClearJitterBufferOnCloseSession(Node* node, RtpSession* session)
{
    if (session == NULL)
    {
        return;
    }
    unsigned int droppedPacketFromJitter = 0;
    //check whether it is first packet
    RtpJitterBuffer* sessionJitterBuffer = session->jitterBuffer;

    if (sessionJitterBuffer == NULL)
    {
        return;
    }
    ListItem* temp = sessionJitterBuffer->listJitterNodes->first;
    ListItem* tempNext = NULL;

    while (temp != NULL)
    {
        tempNext = temp->next;

        //drop this element from buffer
        //Delete the message too
        if (DEBUG_RTPJITTER)
        {
            unsigned short sequenceNo
                    = RtpGetSequenceNumberofPacket(temp->data);
            printf("Voip Jitter: packet dropped of seq no = %u "
                    "Due to Close connection\n", sequenceNo);
        }

        ListGet(node,
            sessionJitterBuffer->listJitterNodes,
            temp,
            TRUE,
            TRUE);

        sessionJitterBuffer->packetDropped += 1;
        droppedPacketFromJitter++;
        temp = tempNext;
    }

    if (droppedPacketFromJitter >
                    sessionJitterBuffer->maxConsecutivePacketDropped)
    {
        sessionJitterBuffer->maxConsecutivePacketDropped =
                                            droppedPacketFromJitter;
    }
    if (sessionJitterBuffer->msgNominalDelay != NULL)
    {
        MESSAGE_CancelSelfMsg(node,
               sessionJitterBuffer->msgNominalDelay);
        sessionJitterBuffer->msgNominalDelay = NULL;
    }
    if (sessionJitterBuffer->msgTalkSpurtDelay != NULL)
    {
        MESSAGE_CancelSelfMsg(node,
                sessionJitterBuffer->msgTalkSpurtDelay);
        sessionJitterBuffer->msgTalkSpurtDelay = NULL;
    }
}

//--------------------------------------------------------------------------
// NAME         : RtpSetJitterBufferFirstPacketAsTrue()
// PURPOSE      : Set Jitter Buffer First Packet is true
// ASSUMPTION   : none
//      node    : Node
//  remoteAddr  : Remote address of this session
// RETURN VALUE : void
//--------------------------------------------------------------------------

void RtpSetJitterBufferFirstPacketAsTrue(Node* node,
                                         Address remoteAddr,
                                         unsigned short initiatorPort)
{
    RtpData* rtp = (RtpData*) node->appData.rtpData;
    if (rtp->jitterBufferEnable == FALSE)
    {
        return;
    }
    RtpJitterBuffer* sessionJitterBufferRemote = NULL;
    if (DEBUG_RTP)
    {
       printf("\n\n Calling 4 RtpGetSessionPtrByDestAddr\n");
    }
    RtpSession* sessionRemote
        = RtpGetSessionPtrByDestAddr(rtp, remoteAddr, initiatorPort, FALSE);
    if (sessionRemote == NULL)
    {
        return;
    }
    sessionJitterBufferRemote = sessionRemote->jitterBuffer;
    sessionJitterBufferRemote->firstPacket = TRUE;
    sessionJitterBufferRemote->expectedSeq = sessionRemote->rtpSeqNum;

}

//-------------------------------------------------------------------------
// NAME         : RtpCalculateAdaptiveJitterBufferMaxDelay()
// PURPOSE      : Calculate Jitter Buffer Maximum Delay dynamically
// ASSUMPTION   : none
//      node    : A pointer to Node
//      session : A pointer to RtpSession
//  arrivalTime : Arrival Time of the packet in Receiving Node
// rtpTimestamp : RTP Time Stamp of the packet in Sending Node
// RETURN VALUE : pointer to the RtpSession struct.
//-------------------------------------------------------------------------

void RtpCalculateAdaptiveJitterBufferMaxDelay(Node* node,
                                             RtpSession* session,
                                             clocktype arrivalTime,
                                             clocktype rtpTimestamp)
{
    RtpJitterBuffer* sessionJitterBuffer = session->jitterBuffer;
    double constA = 0.99;
     // When the actual delay is less than the estimated delay
    double constA1 = 0.9375;
     // When the actual delay is more than the estimated delay.
    double constA2 = 0.75 ;
    node = node;
    if (sessionJitterBuffer == NULL)
    {
        return;
    }
    clocktype maxDelayInJitter = sessionJitterBuffer->maxDelay;
    clocktype transitDelay = arrivalTime - rtpTimestamp;
    if (transitDelay < maxDelayInJitter)
    {
        constA = constA1;
    }
    else
    {
        constA = constA2;
    }

//                  If Si is the RTP timestamp from packet i, and Ri is
//                  the time of arrival in RTP timestamp units for
//                  packet i, then for two packets i and j, D may be
//                  expressed as D(i,j)=(Rj-Ri)-(Sj-Si)=(Rj-Sj)-(Ri-Si)
//                  The interarrival jitter is calculated continuously as
//                  each data packet i is received from source SSRC_n,
//                  using this difference D for that packet and the
//                  previous packet i-1 in order of arrival (not
//                  necessarily in sequence), according to the formula.
//                  J=J+(|D(i-1,i)|-J)/16 ["RFC 1889: RTP: A Transport
//                  Protocol for Real-Time Applications" January 1996
//                  by H. Schulzrinne, S. Casner, R. Frederick, and
//                          V. Jacobson]


    //D  = A* D (i-1) + (1-A)*Ni;
     sessionJitterBuffer->packetDelay
         = (clocktype) (constA * sessionJitterBuffer->packetDelay
                                    + (1 - constA) * transitDelay);

//Vi= A* V(i-1) + (1-A)* |Di - Ni |;
    sessionJitterBuffer->packetVariance
        = (clocktype) (constA * sessionJitterBuffer->packetVariance
                + (1- constA) * ABS(sessionJitterBuffer->packetDelay
                    - transitDelay));
}
//Tracing is not supported for RTP layer
#if 0
//-------------------------------------------------------------------------//
// FUNCTION NAME:RTPInitTrace
// PURPOSE      :Initialize trace from user configuration.
// ASSUMPTION   :None.
// RETURN VALUE :None.
//-------------------------------------------------------------------------//
static
void RTPInitTrace(Node* node, const NodeInput* nodeInput)
{
    char buf[MAX_STRING_LENGTH];
    BOOL retVal;
    BOOL traceAll = TRACE_IsTraceAll(node);
    BOOL trace = FALSE;
    static BOOL writeMap = TRUE;

    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "TRACE-RTP",
        &retVal,
        buf);

    if (retVal)
    {
        if (strcmp(buf, "YES") == 0)
        {
            trace = TRUE;
        }
        else if (strcmp(buf, "NO") == 0)
        {
            trace = FALSE;
        }
        else
        {
            ERROR_ReportError(
                "TRACE-RTP should be either \"YES\" or \"NO\".\n");
        }
    }
    else
    {
        if (traceAll)
        {
            trace = TRUE;
        }
    }

    if (trace)
    {
            TRACE_EnableTraceXML(node, TRACE_RTP,
                "RTP", RTP_PrintTraceXML, writeMap);
    }
    else
    {
            TRACE_DisableTraceXML(node, TRACE_RTP,
                "RTP", writeMap);
    }
    writeMap = FALSE;
}


// /**
// FUNCTION   :: RTP_PrintTraceXML
// LAYER      :: Application
// PURPOSE    :: Print out packet trace information in XML format
// PARAMETERS ::
// + node : Node* : Pointer to node, doing the packet trace
// + mntMsg  : Message* : Pointer to Message
// RETURN ::  void : NULL
// **/

void RTP_PrintTraceXML(Node* node, Message* msg)
{

    char buf[MAX_STRING_LENGTH];
    RtpPacket* rtpHeader = (RtpPacket *)MESSAGE_ReturnPacket(msg);

    sprintf(buf, "<rtp> %hu %hu %hu %hu %hu %hu %hu %hu %hu </rtp>",
        rtpHeader->v,
        rtpHeader->p,
        rtpHeader->x,
        rtpHeader->cc,
        rtpHeader->m,
        rtpHeader->pt,
        rtpHeader->seqNum,
        rtpHeader->timeStamp,
        rtpHeader->ssrc);

    TRACE_WriteToBufferXML(node, buf);
}


//-------------------------------------------------------------------------//
// FUNCTION NAME:RTCPInitTrace
// PURPOSE      :Initialize trace from user configuration.
// ASSUMPTION   :None.
// RETURN VALUE :None.
//-------------------------------------------------------------------------//
static
void RTCPInitTrace(Node* node, const NodeInput* nodeInput)
{
    char buf[MAX_STRING_LENGTH];
    BOOL retVal;
    BOOL traceAll = TRACE_IsTraceAll(node);
    BOOL trace = FALSE;
    static BOOL writeMap = TRUE;

    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "TRACE-RTCP",
        &retVal,
        buf);

    if (retVal)
    {
        if (strcmp(buf, "YES") == 0)
        {
            trace = TRUE;
        }
        else if (strcmp(buf, "NO") == 0)
        {
            trace = FALSE;
        }
        else
        {
            ERROR_ReportError(
                "TRACE-RTCP should be either \"YES\" or \"NO\".\n");
        }
    }
    else
    {
        if (traceAll)
        {
            trace = TRUE;
        }
    }

    if (trace)
    {
            TRACE_EnableTraceXML(node, TRACE_RTCP,
                "RTCP", RTCP_PrintTraceXML, writeMap);
    }
    else
    {
            TRACE_DisableTraceXML(node, TRACE_RTCP,
                "RTCP", writeMap);
    }
    writeMap = FALSE;
}

// /**
// FUNCTION   :: RTCP_PrintTraceXML
// LAYER      :: Application
// PURPOSE    :: Print out packet trace information in XML format
// PARAMETERS ::
// + node : Node* : Pointer to node, doing the packet trace
// + mntMsg  : Message* : Pointer to Message
// RETURN ::  void : NULL
// **/

void RTCP_PrintTraceXML(Node* node, Message* msg)
{
    char buf[MAX_STRING_LENGTH];
    RtcpPacket* rtcpHeader = (RtcpPacket *)MESSAGE_ReturnPacket(msg);

    switch (rtcpHeader->common.pt)
    {
    case RTCP_SR :
        sprintf(buf, "<rtcp> %hu %hu %hu %hu %hu %hu %hu %hu %hu %hu %hu"
            "%hu %hu %hu</rtcp>",
            rtcpHeader->common.pt,
            rtcpHeader->rtcpPacket.sr.sr.ntpFrac,
            rtcpHeader->rtcpPacket.sr.sr.ntpSec,
            rtcpHeader->rtcpPacket.sr.sr.rtptimeStamp,
            rtcpHeader->rtcpPacket.sr.sr.senderByteCount,
            rtcpHeader->rtcpPacket.sr.sr.senderPktCount,
            rtcpHeader->rtcpPacket.sr.sr.ssrc,
            rtcpHeader->rtcpPacket.sr.rr[0].dlsr,
            rtcpHeader->rtcpPacket.sr.rr[0].fracLost,
            rtcpHeader->rtcpPacket.sr.rr[0].jitter,
            rtcpHeader->rtcpPacket.sr.rr[0].lastSeqNum,
            rtcpHeader->rtcpPacket.sr.rr[0].lsr,
            rtcpHeader->rtcpPacket.sr.rr[0].ssrc,
            rtcpHeader->rtcpPacket.sr.rr[0].totLost);
        break;
    case RTCP_RR :
        sprintf(buf, "<rtcp> %hu %hu %hu %hu %hu %hu %hu %hu %hu</rtcp>",
            rtcpHeader->common.pt,
            rtcpHeader->rtcpPacket.rr.rr[0].dlsr,
            rtcpHeader->rtcpPacket.sr.rr[0].fracLost,
            rtcpHeader->rtcpPacket.rr.rr[0].jitter,
            rtcpHeader->rtcpPacket.rr.rr[0].lastSeqNum,
            rtcpHeader->rtcpPacket.rr.rr[0].lsr,
            rtcpHeader->rtcpPacket.rr.rr[0].ssrc,
            rtcpHeader->rtcpPacket.rr.rr[0].totLost,
            rtcpHeader->rtcpPacket.rr.ssrc);
        break;
    case RTCP_BYE :
        sprintf(buf, "<rtcp> %hu %hu</rtcp>",
            rtcpHeader->common.pt,
            rtcpHeader->rtcpPacket.bye.ssrc);
        break;
    }


    TRACE_WriteToBufferXML(node, buf);
}
#endif
