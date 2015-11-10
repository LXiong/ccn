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

#include "prop_cost_hata.h"

// /**
// FUNCTION :: COST231_HataInitialize
// LAYER :: Physical Layer.
// PURPOSE :: Initialize COST 231 Hata Propagation Models
// PARAMETERS ::
// + propChannel: PropChannel* : Pointer to Propagation Channel data-struct
// + channelIndex: Int: Channel Index
// + nodeInput: NodeInput*: Pointer to Node Input
// RETURN :: void : NULL
//
// **/
void COST231_HataInitialize(
    PropChannel *propChannel,
    int channelIndex,
    const NodeInput *nodeInput)
{
    BOOL wasFound;
    char propagationenvironment[MAX_STRING_LENGTH];

    PropProfile* propProfile = propChannel->profile;

    IO_ReadStringInstance(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "PROPAGATION-COST231-HATA-ENVIRONMENT",
        channelIndex,
        (channelIndex == 0),
        &wasFound,
        propagationenvironment);

    if (wasFound) {
        if (strcmp(propagationenvironment, "SUBURBAN") == 0) {
            propProfile->propagationEnvironment = SUBURBAN;

        }
        else if (strcmp(propagationenvironment, "URBAN") == 0) {
            propProfile->propagationEnvironment = URBAN;

        }
        else {
            ERROR_ReportError("Unrecognized propagation environment\n");
        }
    }
    else {
        propProfile->propagationEnvironment =
            DEFAULT_PROPAGATION_COST231_HATA_ENVIRONMENT;

    }

    return;
}


// /**
// FUNCTION :: PathlossCOST231Hata
// LAYER :: Physical Layer.
// PURPOSE :: Calculate COST231Hata Propagation Loss.
// PARAMETERS ::
// + distance: double : Tx-Rx distance
// + wavelength: double : Wavelength
// + txAntennaHeight: float : Transmitter antenna height
// + rxAntennaHeight: float : Receiver antenna height
// + propProfile: PropProfile * : propagation profile
// RETURN :: double : pathloss
//
// **/
double PathlossCOST231Hata(double distance,
                           double waveLength,
                           float txAntennaHeight,
                           float rxAntennaHeight,
                           PropProfile *propProfile)
{

    double frequencyMhz;
    double distanceKm;
    double h1;
    double h2;
    double pathloss_dB;
    double a;
    double k = 0.0;

    if (distance == 0.0)
    {
        pathloss_dB = 0.0;
    }
    else {

        if (txAntennaHeight >= rxAntennaHeight)
        {
            h1 = txAntennaHeight;
            h2 = rxAntennaHeight;
        }
        else {
            h1 = rxAntennaHeight;
            h2 = txAntennaHeight;
        }

        frequencyMhz = 1.0e-6 * SPEED_OF_LIGHT / waveLength;
        distanceKm = distance * 1.0e-3;

        char errorStr[MAX_STRING_LENGTH];

        if (!((frequencyMhz >= 1500.0) && (frequencyMhz <= 2000.0)))
        {
            sprintf(errorStr, "Frequency = %f; Not in recommended"
                    "range [1500:2000]M.Hz\n", frequencyMhz);
            ERROR_ReportWarning(errorStr);
        }

        a = (1.1 * log10(frequencyMhz) - 0.7) * h2
            - (1.56 * log10(frequencyMhz) - 0.8);

        if (propProfile->propagationEnvironment == SUBURBAN) {

            k = 0.0 ;
        }
        else if (propProfile->propagationEnvironment == URBAN) {

            k = 3.0;
        }

        pathloss_dB = 46.33 + 33.9 * log10(frequencyMhz) - 13.82 * log10(h1) -
                      a + (44.9 - 6.55 * log10(h1)) * log10(distanceKm) + k;

    }

    return pathloss_dB;
}


