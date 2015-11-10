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

#ifndef __LINKMICROWAVE_H__
# define __LINKMICROWAVE_H__

#include "mac_link.h"

#define Constants_RadiusEarth 8500000.0;

#define HORIZONTAL 0
#define VERTICAL 1

#define DEFAULT_DIELECTRIC_PERMITTIVITY_VACUUM 1.0
#define DEFAULT_LINK_HUMIDITY   50.0
#define DEFAULT_TEMPERATURE 25.0
#define DEFAULT_RAIN_INTENSITY 0.0
#define DEFAULT_PERCENTAGE_TIME 15.0  //ITU-R Recommendation P.453.6

#define DEFAULT_ANTENNA_HEIGHT 30.0
#define DEFAULT_ANTENNA_DISH_DIAMETER 0.8
#define DEFAULT_ANTENNA_CABLE_LOSS 1.5
#define DEFAULT_TX_POWER 30.0
#define DEFAULT_RX_SENSITIVITY -80.0
#define DEFAULT_NOISE_TEMPERATURE 290.0
#define DEFAULT_NOISE_FACTOR     4.0


typedef enum
{
    PLAINS = 0,
    HILLS = 1,
    MOUNTAINS = 2
} TerrainType;


struct WirelessLinkParameters {
    WirelessLinkSiteParameters Tx;
    WirelessLinkSiteParameters Rx;
    TerrainType terraintype;
    double  TxPower;
    double  RxSensitivity;
    int     climate;   //itm
    double  humidity;  //itm
    double  temperature;
    double  rain_intensity;
    double  longitudes;
    double  latitudes;
    double  terrainaltitude;
    double  percentage_refractivity_gradient_less;
    double  permittivity;  //itm
    double  conductivity;  //itm
    double  refractivity;  //itm
    double  elevationSamplingDistance;  //itm
    double rxpower;
    RandomSeed seed;
    double  noise_temperature;
    double  noisepower;
    double  noise_factor;
};

// ITU-R Recommendation P.838-1
static double rainattenuationparameters[26][5] =
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


double WirelessLinkAntennaParabolicMaxGain(double DiameterinMeter,
                                           double FrequencyinHz);


double WirelessLinkMaxDistance(double TxAntennaHeight,
                               double RxAntennaHeight);


double fourtimesdifference(double parameter0,
                           double parameter1,
                           double parameter2,
                           double parameter3,
                           double parameter4);

double exponentmultiple(double base1,
                        double exponent1,
                        double base2,
                        double exponent2,
                        double coefficient);


double WirelessLinkOxygenAttenuationCoefficient(
                                    double AtmosphericPressure,
                                    double Temperature,
                                    double Frequency);

double WirelessLinkWaterVaporAttenuationCoefficient(
                                    double AtmosphericPressure,
                                    double Temperature,
                                    double watervapordensity,
                                    double Frequency);

double WirelessLinkAtmosphericAttenuation(double distance,
                                          double Frequency,
                                          double humidity,
                                          double temperature);

double WirelessLinkRainAttenuationCoefficient(double rainintensity,
                                              double kfactor,
                                              double alphafactor);


double WirelessLinkRainAttenuation(double Frequency,
                                   double distance,
                                   int polarization,
                                   double rainintensity);

double WirelessDiffractionAttenuationSphericalEarth(
                                    double electricconductivity,
                                    double dielectricpermittivity,
                                    double distance,
                                    double Txantennaheight,
                                    double Rxantennaheight,
                                    double frequency,
                                    int polarization);

double WirelessLinkFadingProbability(double distance,  //in km
                         double frequency,            //in GHz
                         double Txantennaheight,      //in meters
                         double Rxantennaheight,      // in meters
                         double receivedpower,        // in nondB
                         double receiversensitivity,  //in nondB
                         char terraintype,
                         double terrainaltitude,      // in meters
                         double probabilityexceedrefractivitygradient,
                         double latitudes,
                         double longitudes);


double calculatefadingprobability(double multipathfactor,
                                  double depthfading,     // in dB
                                  double marginfading);  // in dB


void WirelessLinkInitialize(Node *node,
                            LinkData *link,
                            Address* address,
                            const NodeInput *nodeInput);

double WirelessLinkCalculatePathloss(Node* node,
                                     LinkData* link,
                                     Message* msg);

BOOL WirelessLinkCheckPacketError(Node* node,
                                  LinkData* link,
                                  Message* msg);

void WirelessLinkMessageArrivedFromLink(Node *node,
                                        int interfaceIndex,
                                        Message* msg);

//---------------------------------------------------------------------------
// FUNCTION             : WirelessLinkCheckIsDestNodeReachable
// PURPOSE             :: Check whether destination is reachable or not
// PARAMETERS          ::
// +node                : Node* node    : Pointer to Node
// +link                : LinkData* link : Pointer to LinkData
// +msg                 : Message* msg  : Pointer to Message Structure
// RETURN               : BOOL
//---------------------------------------------------------------------------

BOOL WirelessLinkCheckIsDestNodeReachable(Node* node,
                                          LinkData* link,
                                          Message* msg);


#endif /* LINKMICROWAVE_H */
