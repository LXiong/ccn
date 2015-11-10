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

#ifndef MAC_LINK_SATELLITE_H
#define MAC_LINK_SATELLITE_H

#include "mac_link.h"

#define HORIZONTAL 0
#define VERTICAL 1

#define DEFAULT_LINK_HUMIDITY   50.0
#define DEFAULT_TEMPERATURE 25.0
#define DEFAULT_RAIN_INTENSITY 0.0

#define DEFAULT_SATELLITE_ANTENNA_DISH_DIAMETER 1.0
#define DEFAULT_ANTENNA_CABLE_LOSS 1.5
#define DEFAULT_SATELLITE_TX_POWER 50.0

#define DEFAULT_SATELLITE_NOISE_FACTOR     1.2
#define DEFAUL_RAIN_EQUIVALENT_HEIGHT 4500.0
#define DEFAULT_ANTENNA_EFFICIENCY 0.85
#define DEFAULT_NOISE_TEMPERATURE_TZERO 290.0
#define DEFAULT_SNR_THRESHOLD   10.0
#define DEFAULT_DATA_RATE 10000000
#define DEFAULT_BANDWIDTH 13000000
#define DEFAULT_CONNECTION_LOSS 0.1
#define DEFAULT_MISMATCH_LOSS 0.2
#define DEFAULT_ANTENNA_TEMPERATURE 30.0
#define DEFAULT_MAX_VELOCITY  1.0

struct SatelliteLinkParameters {
    WirelessLinkSiteParameters txPara;
    WirelessLinkSiteParameters rxPara;

    double  humidity;
    double  temperature;
    double  rainIntensity;
    double  longitudes;
    double  latitudes;
    double  terrainAltitude;
    double  rxThreshold;
    double  dataRate;
    RandomSeed seed;

    FadingModel fadingModel;
    double  numGaussianComponents;
    double*  gaussianComponent1;
    double*  gaussianComponent2;
    double  kFactor;
    int startingPoint;
    double fadingStretchingFactor;
};


struct SatelliteLinkInfo {
    clocktype propDelay;
    double distance;
    WirelessLinkSiteParameters sourceSiteParameters;
    Coordinates sourcecoordinates;
};

// ITU-R Recommendation P.838-1
static double rainAttenuationParameters[26][5] =
     {{1.0, 0.0000387, 0.0000352, 0.912, 0.880},
      {2.0, 0.000154, 0.000138, 0.963, 0.923},
      {4.0, 0.000650, 0.000591, 1.121, 1.075},
      {6.0, 0.00175, 0.00155, 1.308, 1.265},
      {7.0, 0.00301, 0.00265, 1.332, 1.312},
      {8.0, 0.00454, 0.00395, 1.327, 1.310},
      {10.0, 0.0101, 0.00887, 1.276, 1.264},
      {12.0, 0.0188, 0.0168, 1.217, 1.200},
      {15.0, 0.0367, 0.0335, 1.154, 1.128},
      {20.0, 0.0751, 0.0691, 1.099, 1.065},
      {25.0, 0.124, 0.113, 1.061, 1.030},
      {30.0, 0.187, 0.167, 1.021, 1.000},
      {35.0, 0.263, 0.233, 0.979, 0.963},
      {40.0, 0.350, 0.310, 0.939, 0.929},
      {45.0, 0.442, 0.393, 0.903, 0.897},
      {50.0, 0.536, 0.479, 0.873, 0.868},
      {60.0, 0.707, 0.642, 0.826, 0.824},
      {70.0, 0.851, 0.784, 0.793, 0.793},
      {80.0, 0.975, 0.906, 0.769, 0.769},
      {90.0, 1.06, 0.999, 0.753, 0.754},
      {100.0, 1.12, 1.06, 0.734, 0.744},
      {120.0, 1.18, 1.13, 0.731, 0.732},
      {150.0, 1.31, 1.27, 0.710, 0.711},
      {200.0, 1.45, 1.42, 0.689, 0.690},
      {300.0, 1.36, 1.35, 0.688, 0.689},
      {400.0, 1.32, 1.31, 0.683, 0.684}};


void SatelliteLinkInitialize(Node *node,
                            LinkData *link,
                            Address* address,
                            const NodeInput *nodeInput);

void SatelliteLinkMessageArrivedFromLink(Node *node,
                                        int interfaceIndex,
                                        Message *msg);

#endif /* LINK_SATELLITE_H */
