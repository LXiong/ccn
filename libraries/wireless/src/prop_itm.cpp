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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string.h>
#include "api.h"
#include "main.h"
#include "prop_itm.h"
#include "prop_itm_uarea.h"

// Longley-Rice / TIREM related parameters:
//
//                               Climate Refractivity
// Equatorial                    1       360
// Continental Subtropical       2       320
// Maritime Tropical             3       370
// Desert                        4       280
// Continental Temperate         5       301
// Maritime Temperate, Over Land 6       320
// Maritime Temperate, Over Sea  7       350
//
//                Dielectric Constant Ground Conductivity
//                (Permittivity)
// Average Ground 15                  0.005
// Poor Ground     4                  0.001
// Good Ground    25                  0.02
// Fresh Water    81                  0.01
// Salt Water     81                  5.0

// m_Nelev:  has the number of points in the terrain profile
// m_ArcInc: has the distance between points in meters
// m_dblElev[]: floating point values of the terrain
// m_dblTranH: has the transmitting antenna height in meters
// m_dblRecH: has the receiving antenna height in meters
// m_intHor: has value of 0 for Horizontal Polarization, or 1 for Vertical Polarization
// m_intRadioClimate: has the value of
//     1 for Equatorial
//     2 for Continental Subtropical
//     3 for Maritime Tropical
//     4 for Desert
//     5 for Continental Temperate
//     6 for Maritime Temperate, Over Land
//     7 for Maritime Temperate, Over Sea
// m_dblDielectric: has value of dielectric constant of ground
// m_dblCond: has value conductivity of ground
// m_dblFreq: has the frequency in MHz
// m_dblSurfRef: has the surface refractivity

double PathlossItm(
    int m_Nelev,
    double m_ArcInc,
    double m_dblElev[],
    double m_dblTranH,
    double m_dblRecH,
    int m_intHor,
    int m_intRadioClimate,
    double m_dblDielectric,
    double m_dblCond,
    double m_dblFreq,
    double m_dblSurfRef)
{
    prop_type prop;
    propv_type propv;
    propa_type propa;

    double zsys, np, eno, enso, q, loss;
    double ja, jb, eps, sgm, fs;
    int polarity, radioclimate;
    int i,jai,jbi;
    double dblelev[MAX_NUM_ELEVATION_SAMPLES];

    memset(&prop, 0, sizeof(prop_type));
    // setup the dblelev array for ITM Point-to-Point
    dblelev[0] = (double)m_Nelev - 1.0;
    dblelev[1] = (double)m_ArcInc;

    for (i = 0; i < m_Nelev; i++) {
        dblelev[i + 2] = m_dblElev[i];
    }
    prop.hg[0] = m_dblTranH;
    prop.hg[1] = m_dblRecH;
    polarity = (m_intHor==0) ? 0 : 1;
    propv.klim = m_intRadioClimate;
    radioclimate = propv.klim;
    prop.kwx = 0;
    propv.lvar = 5;
    prop.mdp = -1;
    eps = m_dblDielectric;
    sgm = m_dblCond;
    np = dblelev[0];
    eno = m_dblSurfRef;
    enso = 0.0;
    q = enso;
    zsys = 0.0;
    if (q <= 0) {
        ja = 3.0 + 0.1 * dblelev[0];
        jb = np - ja + 6.0;
       jai = (int)RoundToInt(ja);
       jbi = (int)RoundToInt(jb);

       for (i = jai; i <= jbi; i++) {
            zsys += dblelev[i - 1];
        }
        zsys /= (jb - ja + 1);
        q = eno;
    }

    propv.mdvar = 12;
    qlrps(m_dblFreq, zsys, q, polarity, eps, sgm, prop);
    qlrpfl(dblelev, radioclimate, propv.mdvar, prop, propa, propv);
    fs = 32.45 + 20.0 * log10(m_dblFreq) + 20.0 * log10(prop.dist/1000.0);
    q = prop.dist - propa.dla;
    q = FORTRAN_DIM(q, 0.5 * dblelev[1]) - FORTRAN_DIM(-q, 0.5 * dblelev[1]);

    loss = fs + prop.aref;

    if (loss < 0.0)
        loss = 0.0;

    return loss;
}

void ItmInitialize(
    PropChannel *propChannel,
    int channelIndex,
    const NodeInput *nodeInput)
{
    PropProfile* propProfile = propChannel->profile;
    BOOL wasFound;
    double elevationSamplingDistance;
    int climate;
    double refractivity;
    double conductivity;
    double permittivity;
    double humidity;
    char polarization[MAX_STRING_LENGTH];


    IO_ReadDoubleInstance(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "PROPAGATION-SAMPLING-DISTANCE",
        channelIndex,
        (channelIndex == 0),
        &wasFound,
        &elevationSamplingDistance);

    if (wasFound) {
        propProfile->elevationSamplingDistance =
            (float)elevationSamplingDistance;
    }
    else {
        propProfile->elevationSamplingDistance =
            DEFAULT_SAMPLING_DISTANCE;
    }

    IO_ReadDoubleInstance(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "PROPAGATION-REFRACTIVITY",
        channelIndex,
        (channelIndex == 0),
        &wasFound,
        &refractivity);

    if (wasFound) {
        propProfile->refractivity = refractivity;
    }
    else {
        propProfile->refractivity = DEFAULT_REFRACTIVITY;
    }

    IO_ReadDoubleInstance(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "PROPAGATION-CONDUCTIVITY",
        channelIndex,
        (channelIndex == 0),
        &wasFound,
        &conductivity);

    if (wasFound) {
        propProfile->conductivity = conductivity;
    }
    else {
        propProfile->conductivity = DEFAULT_CONDUCTIVITY;
    }

    IO_ReadDoubleInstance(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "PROPAGATION-PERMITTIVITY",
        channelIndex,
        (channelIndex == 0),
        &wasFound,
        &permittivity);

    if (wasFound) {
        propProfile->permittivity = permittivity;
    }
    else {
        propProfile->permittivity = DEFAULT_PERMITTIVITY;
    }

    IO_ReadDoubleInstance(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "PROPAGATION-HUMIDITY",
        channelIndex,
        (channelIndex == 0),
        &wasFound,
        &humidity);

    if (wasFound) {
        propProfile->humidity = humidity;
    }
    else {
        propProfile->humidity = DEFAULT_HUMIDITY;
    }

    IO_ReadIntInstance(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "PROPAGATION-CLIMATE",
        channelIndex,
        (channelIndex == 0),
        &wasFound,
        &climate);

    if (wasFound) {
        assert(climate >= 1 && climate <= 7);
        propProfile->climate = climate;
    }
    else {
        propProfile->climate = DEFAULT_CLIMATE_NUM;
    }

    //
    // This must be an antenna property..
    //
    IO_ReadStringInstance(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "ANTENNA-POLARIZATION",
        channelIndex,
        (channelIndex == 0),
        &wasFound,
        polarization);

    memset(propProfile->polarizationString, 0, 5);

    if (wasFound) {
        if (strcmp(polarization, "HORIZONTAL") == 0) {
            propProfile->polarization = HORIZONTAL;
            strncpy(propProfile->polarizationString, "H   ", 4);
        }
        else if (strcmp(polarization, "VERTICAL") == 0) {
            propProfile->polarization = VERTICAL;
            strncpy(propProfile->polarizationString, "V   ", 4);
        }
        else {
            ERROR_ReportError("Unrecognized polarization\n");
        }
    }
    else {
        propProfile->polarization = VERTICAL;
        strncpy(propProfile->polarizationString, "V   ", 4);
    }

    return;
}
