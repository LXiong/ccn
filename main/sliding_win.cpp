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

#include <stdlib.h>

#include "api.h"
#include "sliding_win.h"
#include "qualnet_error.h"

#define SIM2SEC(x) (((double)(x)) / SECOND)

void MsTmWinInit(MsTmWin* pWin, clocktype sSize, int nSlot, double weight,
                 clocktype theTime)
{
    pWin->pSlot     = NULL;
    pWin->sSize     = 0;
    pWin->nSlot     = 0;
    pWin->weight    = 0.0;
    pWin->total     = 0;
    pWin->tmBase    = 0;
    pWin->tmStart   = 0;

    MsTmWinReset(pWin, sSize, nSlot, weight, theTime);
}

void MsTmWinClear(MsTmWin* pWin)
{
    if (pWin->pSlot != NULL)
        MEM_free(pWin->pSlot);
    pWin->pSlot     = NULL;
    pWin->sSize     = 0;
    pWin->nSlot     = 0;
    pWin->weight    = 0.0;
    pWin->total     = 0;
    pWin->tmBase    = 0;
    pWin->tmStart   = 0;
}

void MsTmWinReset(MsTmWin* pWin, clocktype sSize, int nSlot, double weight,
                  clocktype theTime)
{
    register int    idx;
    clocktype       time = theTime;

    MsTmWinClear(pWin);

    assert((pWin->pSlot = (double*)MEM_malloc(sizeof (double) * nSlot))
        != NULL);
    pWin->sSize = sSize;
    pWin->nSlot = nSlot;
    pWin->weight = weight;
    pWin->total = 0.0;
    pWin->tmBase = pWin->tmStart = time;
    // Clear the slots
    for (idx = 0; idx < pWin->nSlot; idx++)
        *(pWin->pSlot + idx) = 0.0;
}

void MsTmWinNewData(MsTmWin* pWin, double data, clocktype theTime)
{
    register int    tmp;
    int             idx;
    int             shift;
    clocktype       time = theTime;

    // check if the window is created
    if (pWin->pSlot == NULL // not yet
        || time < pWin->tmBase) // invalid
        return;
    // compute idx
    idx = (int) ((time - pWin->tmBase) / pWin->sSize);
    shift = (idx >= pWin->nSlot) ? idx - pWin->nSlot + 1 : 0;
    if (shift) {
        for (tmp = 0; tmp < (int)(pWin->nSlot) - shift; tmp++)
            *(pWin->pSlot + tmp) = *(pWin->pSlot + tmp + shift);
        for (; tmp < pWin->nSlot; tmp++)
            *(pWin->pSlot + tmp) = 0.0;
        idx -= shift;
        pWin->tmBase += pWin->sSize * shift;
    }
    pWin->pSlot[idx] += data;
    pWin->total += data;
}

clocktype MsTmWinWinSize(MsTmWin* pWin, clocktype theTime)
{
    register int    idx;
    clocktype       time = theTime;

    // check the current time
    if (time < pWin->tmBase || pWin->pSlot == NULL)
        return (clocktype)(0);
    // compute idx
    idx = (int) ((time - pWin->tmBase) / pWin->sSize);
    if (idx >= pWin->nSlot)
        return (clocktype)(pWin->nSlot * pWin->sSize);
    else
        return (clocktype)((idx + 1) * pWin->sSize);
        // return (clocktype)(idx * pWin->sSize);
}

double MsTmWinSum(MsTmWin* pWin, clocktype theTime)
{
    register int    tmp;
    int             idx;
    double          sum;
    int             shift;
    clocktype       time = theTime;

    // check the current time
    if (time < pWin->tmBase || pWin->pSlot == NULL)
        return 0;
    // compute idx
    idx = (int) ((time - pWin->tmBase) / pWin->sSize);
    shift = (idx >= pWin->nSlot) ? idx - pWin->nSlot + 1 : 0;
    idx -= shift;
    for (sum = 0, tmp = shift; tmp <= idx; tmp++)
        sum += *(pWin->pSlot + tmp);
    return sum;
}

double MsTmWinAvg(MsTmWin* pWin, clocktype theTime)
{
    register int    tmp;
    int             idx;
    double          avg;
    double          weight;
    int             shift;
    clocktype       time = theTime;

    // check the current time
    if (time < pWin->tmBase || pWin->pSlot == NULL)
        return 0;
    // compute idx
    idx = (int) ((time - pWin->tmBase) / pWin->sSize);
    shift = (idx >= pWin->nSlot) ? idx - pWin->nSlot + 1 : 0;
    idx -= shift;
    for (avg = 0.0, tmp = shift; tmp <= idx; tmp++) {
        //
        //         1     2n - N + 1
        // W(n) = --- + ------------ Wf,   -1 < Wf < 1
        //         N      N(N - 1)
        //
        weight = (double)(1.0 / pWin->nSlot)
            + (pWin->weight / (double)(pWin->nSlot * (pWin->nSlot - 1.0)))
            * (double)(2.0 * (pWin->nSlot - 1.0 - idx + tmp - shift)
            - pWin->nSlot + 1.0);
        avg += (*(pWin->pSlot + tmp) / SIM2SEC(pWin->sSize)) * weight;
    }
    return avg;
}

double MsTmWinTotalSum(MsTmWin* pWin, clocktype theTime)
{
    return pWin->total;
}

double MsTmWinTotalAvg(MsTmWin* pWin, clocktype theTime)
{
    clocktype   time = theTime;
    clocktype   elapse = time - pWin->tmStart;
    return (elapse == (clocktype)(0)) ? 0 : pWin->total / SIM2SEC(elapse);
}

