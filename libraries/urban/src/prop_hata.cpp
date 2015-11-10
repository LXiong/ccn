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

#include "prop_hata.h"

// /**
// FUNCTION :: Okumura_HataInitialize
// LAYER :: Physical Layer.
// PURPOSE :: Initialize Okumura_Hata Propagation Models
// PARAMETERS ::
// + propChannel: PropChannel* : Pointer to Propagation Channel data-struct
// + channelIndex: Int: Channel Index
// + nodeInput: NodeInput*: Pointer to Node Input
// RETURN :: void : NULL
//
// **/
void Okumura_HataInitialize(
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
        "PROPAGATION-OKUMURA-HATA-ENVIRONMENT",
        channelIndex,
        (channelIndex == 0),
        &wasFound,
        propagationenvironment);

    if (wasFound) {
        if (strcmp(propagationenvironment, "OPEN-RURAL") == 0) {
            propProfile->propagationEnvironment = OPEN_RURAL;

        }
        else if (strcmp(propagationenvironment, "QUASI-OPEN-RURAL") == 0) {
            propProfile->propagationEnvironment = QUASI_OPEN_RURAL;

        }
        else if (strcmp(propagationenvironment, "SUBURBAN") == 0) {
            propProfile->propagationEnvironment = SUBURBAN;

        }
        else if (strcmp(propagationenvironment, "URBAN") == 0) {
            propProfile->propagationEnvironment = URBAN;

        }
        else if (strcmp(propagationenvironment, "METROPOLITAN") == 0) {
            propProfile->propagationEnvironment = METROPOLITAN;

        }
        else {
            ERROR_ReportError("Unrecognized propagation environment\n");
        }
    }
    else {
        propProfile->propagationEnvironment =
            DEFAULT_PROPAGATION_OKUMURA_HATA_ENVIRONMENT;

    }

    return;
}


// /**
// FUNCTION :: PathlossHata
// LAYER :: Physical Layer.
// PURPOSE :: Calculate Propagation Loss for Okumura-Hata model
// PARAMETERS ::
// + distance: double : Tx-Rx distance
// + wavelength: double : Wavelength
// + txAntennaHeight: float : Transmitter Antenna height
// + rxAntennaHeight: float : Receiver Antenna height
// + propProfile: PropProfile * : Propagation profile
// RETURN :: double : pathloss
//
// **/
double PathlossHata(double distance,
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
    double k;

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

        if (h1 == 0.0) {
            h1 = VERY_SMALL_ANTENNA_HEIGHT;
        }
        if (h2 == 0.0) {
            h2 = VERY_SMALL_ANTENNA_HEIGHT;
        }

        frequencyMhz = 1.0e-6 * SPEED_OF_LIGHT / waveLength;
        distanceKm = distance * 1.0e-3;

        char errorStr[MAX_STRING_LENGTH];

        if (!((frequencyMhz > 150.0) && (frequencyMhz < 1000.0)))
        {
            sprintf(errorStr, "Frequency = %f; Not in recommended"
                    "range [150:1000]M.Hz\n", frequencyMhz);
            ERROR_ReportWarning(errorStr);
        }

        k = 0.0;
        a = (1.1 * log10(frequencyMhz) - 0.7) * h2
            - (1.56 * log10(frequencyMhz) - 0.8);

        if (propProfile->propagationEnvironment == OPEN_RURAL) {

            k = 4.78 * pow (log10(frequencyMhz),2.0)
                - 18.33 * log10(frequencyMhz) + 40.94 ;
        }
        else if (propProfile->propagationEnvironment == QUASI_OPEN_RURAL) {

            k = 4.78 * pow (log10(frequencyMhz),2.0)
                - 18.33 * log10(frequencyMhz) + 35.94 ;
        }
        else if (propProfile->propagationEnvironment == SUBURBAN) {

            k = 2.0 * pow (log10(frequencyMhz/28.0),2.0) + 5.4;
        }
        else if (propProfile->propagationEnvironment == METROPOLITAN){
            // building heights greater than 15 m

            if (frequencyMhz >= 400.0){

                a = 3.2 * pow(log10(11.75 * h2),2.0) - 4.97;
            }
            else {

                a =8.29 * pow(log10(1.54 * h2),2.0) - 1.1;
            }
        }

        pathloss_dB =
            69.55 + 26.16 * log10(frequencyMhz) - 13.83 * log10(h1) -
            a + (44.9 -6.55 * log10(h1)) * log10(distanceKm) - k;
    }

    return pathloss_dB;
}
