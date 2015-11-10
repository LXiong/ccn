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
#include <string.h>
#include <stdlib.h>

#include <math.h>
#include <limits.h>

#include "api.h"
#include "partition.h"
#include "antenna.h"
#include "random.h"
#include "node.h"
#include "mac_link.h"
#include "mac_link_satellite.h"
#include "network_ip.h"

#define DEBUG 0


static
double SatelliteLinkGetElevationAngle(
        Node* node,
        const Coordinates* fromPosition,
        const Coordinates* toPosition)
{
    int coordinateSystemType =
            NODE_GetTerrainPtr(node)->getCoordinateSystem();
    double elevationAngle = 0.0;

    if (coordinateSystemType == LATLONALT)
    {
       const Coordinates* satPosition;
       const Coordinates* earthStationPosition;

       if (toPosition->latlonalt.altitude >
          fromPosition->latlonalt.altitude)
       {
            satPosition = toPosition;
            earthStationPosition = fromPosition;
       }
       else
       {
            satPosition = fromPosition;
            earthStationPosition = toPosition;
       }

        CoordinateType diffLongitude =
                earthStationPosition->latlonalt.longitude
                - satPosition->latlonalt.longitude;

        double cos_diffLongitude = cos (diffLongitude * IN_RADIAN);

        double cos_EsLat =
                    cos ((earthStationPosition->latlonalt.latitude -
                    satPosition->latlonalt.latitude) * IN_RADIAN);

        double satHeight = satPosition->latlonalt.altitude;

        elevationAngle =
            (atan ((cos_diffLongitude * cos_EsLat -
            (EARTH_RADIUS/(satHeight + EARTH_RADIUS)))
             / (sqrt (1.0 - cos_diffLongitude * cos_diffLongitude
             * cos_EsLat * cos_EsLat)))/IN_RADIAN);
    }

    return elevationAngle;

}

static
double SatelliteLinkAntennaParabolicMaxGain(double diameterMeter,
                                           double frequencyHz,
                                           double efficiency,
                                           double propSpeed)
{
    double diameterinWavelength;
    double gainPeak;
    double wavelength;

    ERROR_Assert(frequencyHz > 0.0,
                 "Satellite point to point link:\n"
                 "      Please veirfy the value for frequency\n");

    ERROR_Assert(diameterMeter > 0.01,
                 "Satellite point to point link:\n"
                 "      Please verify the value for antenna diameter\n");

    ERROR_Assert(efficiency > 0.0,
                 "Satellite point to point link:\n"
                 "      Please verify the value for antenna efficiency\n");

    wavelength = propSpeed / frequencyHz;
    diameterinWavelength = diameterMeter / wavelength;

    gainPeak = 7.7 + 20.0 * log10(diameterinWavelength) + IN_DB(efficiency);

   // ITU-R Recommendation F.699-5

    return gainPeak;
}

static
double FourtimesDifference(double parameter0,
                           double parameter1,
                           double parameter2,
                           double parameter3,
                           double parameter4)
{
    double fourtimes;

    fourtimes = (parameter0 - parameter1)* (parameter0 - parameter2)
                * (parameter0 - parameter3) * (parameter0 - parameter4);

    return fourtimes;

}

static
double ExponentMultiple(double base1,
                        double exponent1,
                        double base2,
                        double exponent2,
                        double coefficient)
{

    double multipleexponent;

    multipleexponent = pow(base1, exponent1)* pow(base2, exponent2)
                       *exp(coefficient * (1.0 - base2));

    return multipleexponent;

}


/*
 Given the frequency in GHz, the atmospheric pressure in hPa, and the
 temperature in Centigrade, represents the oxygen attenuation coefficient
 in dB/km according to ITU-R Recommendation P.676-4.
*/
static
double SatelliteLinkOxygenAttenuationCoefficient(double atmosphericPressure,
                                                double temperature,
                                                double frequency)
{
    double relatedpressure;
    double relatedtemperature;
    double frequencyexponents;
    double axis1;
    double axis2;
    double derivedd;
    double derivedc;
    double deriveda;
    double derivedb;
    double derivedxila1;
    double derivedxila2;
    double derived54;
    double derived54prime;
    double derived57;
    double derived60;
    double derived63;
    double derived66;
    double derived66prime;
    double oxygenattenuationcoefficient = 0.0;
    double kasai;
    double kasai1;
    double kasai2;
    double kasai3;
    double kasai4;
    double kasai5;
    double factor1;
    double factor2;
    double factor3;
    double factor4;
    double factor5;
    double factor6;
    double factor7;

    // Relative pressure estimation
    relatedpressure = atmosphericPressure/1031.0;

    // Relative temperature estimation
    relatedtemperature = 288.0/(273.0 + temperature);

    // Frequency exponents estimation
    if (frequency <= 60.0) {

        frequencyexponents = 0.0;

    }else{

        frequencyexponents = -15.0;
    }

    // Related parameters estimation
    axis1 = 6.9575 * ExponentMultiple(relatedpressure, -0.3461,
                                      relatedtemperature, 0.2535,
                                      1.3766)-1.0;

    axis2 = 42.1309 * ExponentMultiple(relatedpressure, -0.3068,
                                       relatedtemperature, 1.2023,
                                       2.5147)-1.0;

    derivedc = log(axis2/axis1)/log(3.5);

    derivedd = pow(4.0, derivedc)/axis1;

    derivedxila1 = 6.7665 * ExponentMultiple(relatedpressure, -0.5050,
                                             relatedtemperature, 0.5106,
                                             1.5663)-1.0;

    derivedxila2 = 27.8843 * ExponentMultiple(relatedpressure, -0.4908,
                                              relatedtemperature, -0.8491,
                                              0.5496)-1.0;

    deriveda = log(derivedxila2/derivedxila1)/log(3.5);

    derivedb = pow(4.0, deriveda)/derivedxila1;

    derived54prime = 2.128 * ExponentMultiple(relatedpressure, 1.4954,
                                              relatedtemperature, -1.6032,
                                              -2.5280);

    derived54 = 2.136 * ExponentMultiple(relatedpressure, 1.4975,
                                         relatedtemperature, -1.5852,
                                         -2.5196);

    derived57 = 9.984 * ExponentMultiple(relatedpressure, 0.9313,
                                         relatedtemperature, 2.6732,
                                         0.9563);

    derived60 = 15.42 * ExponentMultiple(relatedpressure, 0.8595,
                                         relatedtemperature, 3.6178,
                                         1.1521);

    derived63 = 10.63 * ExponentMultiple(relatedpressure, 0.9298,
                                         relatedtemperature, 2.3284,
                                         0.6287);

    derived66 = 1.944 * ExponentMultiple(relatedpressure, 1.6673,
                                         relatedtemperature, -3.3583,
                                         -4.1612);

    derived66prime = 1.935 * ExponentMultiple(relatedpressure, 1.6657,
                                              relatedtemperature, -3.3714,
                                              -4.1643);

    // Oxygen attenuation coefficient estimation for different frequencies
    if (frequency <= 54.0) {

        factor1 = 7.34 * pow(relatedpressure, 2.0) *
                         pow(relatedtemperature, 3.0)/
                         (pow(frequency, 2.0) + 0.36 *
                         pow(relatedpressure, 2.0) *
                         pow(relatedtemperature, 2.0));

        factor2 = 0.3429 * derivedb * derived54prime/
                  (pow((54 - frequency), deriveda) + derivedb);

        oxygenattenuationcoefficient = (factor1 + factor2) *
                        pow(frequency, 2.0) * pow(10.0, -3.0);

    }
    else if (frequency < 66.0 ) {

        kasai1 = pow(54.0,-frequencyexponents) * log(derived54) *
                 FourtimesDifference(frequency, 57.0, 60.0, 63.0, 66.0)
                 /1944.0;

        kasai2 = pow(57.0,-frequencyexponents) * log(derived57) *
                 FourtimesDifference(frequency, 54.0, 60.0, 63.0, 66.0)
                 /486.0;

        kasai3 = pow(60.0,-frequencyexponents) * log(derived60) *
                 FourtimesDifference(frequency, 54.0, 57.0, 63.0, 66.0)
                 /324.0;

        kasai4 = pow(63.0,-frequencyexponents) * log(derived63) *
                 FourtimesDifference(frequency, 54.0, 57.0, 60.0, 66.0)
                 /486.0;

        kasai5 = pow(66.0,-frequencyexponents) * log(derived66) *
                 FourtimesDifference(frequency, 54.0, 57.0, 60.0, 63.0)
                 /1944.0;

        kasai = kasai1 - kasai2 + kasai3 - kasai4 +kasai5;


        oxygenattenuationcoefficient =
                           exp(kasai * pow(frequency, frequencyexponents));
    }
    else if (frequency < 120.0 ) {

        factor3 = 0.2296 * derivedd * derived66prime/
                  (pow((frequency - 66.0), derivedc) + derivedd);

        factor4 = 0.286 * pow(relatedpressure, 2.0) *
                          pow(relatedtemperature, 3.8)/
                          (pow((frequency - 118.75), 2.0) + 2.97 *
                          pow(relatedpressure, 2.0) *
                          pow(relatedtemperature, 1.6));

        oxygenattenuationcoefficient =
                   (factor3 + factor4) * pow(frequency, 2.0) *
                   pow(10.0, -3.0);
    }
    else if (frequency < 350.0) {

        factor5 = 3.02 * pow(10.0, -4.0) * pow(relatedpressure, 2.0) *
                         pow(relatedtemperature, 3.5);

        factor6 = 1.5827 * pow(relatedpressure, 2.0) *
                           pow(relatedtemperature, 3.0)/
                           pow((frequency - 66.0), 2.0);

        factor7 = 0.286 * pow(relatedpressure, 2.0) *
                          pow(relatedtemperature, 3.8)/
                          (pow((frequency - 118.75), 2.0) + 2.97 *
                          pow(relatedpressure, 2.0)*
                          pow(relatedtemperature, 1.6));

        oxygenattenuationcoefficient = (factor5 + factor6 + factor7)*
                                       pow(frequency, 2.0) * pow(10.0, -3.0);
    }
    else {

        ERROR_ReportError(" This frequency does not support.\n");

    }

    return oxygenattenuationcoefficient;

}


/*
 Given the frequency in GHz, the atmospheric pressure in hPa, the temperature
 in Centigrade, and the water vapor content in g/m3, represents the water vapor
 attenuation coefficient in dB/km according to ITU-R Recommendation P.676-4.
*/
static
double SatelliteLinkWaterVaporAttenuationCoefficient(
                                    double atmosphericPressure,
                                    double temperature,
                                    double waterVaporDensity,
                                    double frequency)
{
    double relatedpressure;
    double relatedtemperature;
    double axis1;
    double axis2;
    double deriveda;
    double derivedc;
    double derivedxila1;
    double derivedxila2;
    double weight1;
    double weight2;
    double weight3;
    double weight4;
    double weight5;
    double item1;
    double item2;
    double item3;
    double item4;
    double item5;
    double item6;
    double item7;
    double item8;
    double item9;
    double g22;
    double g557;
    double g752;
    double sumitems;
    double waterVaporAttenuationCoefficient;

    // Relative pressure estimation
    relatedpressure = atmosphericPressure/1031.0;

    // Relative temperature estimation
    relatedtemperature = 288.0/(273.0 + temperature);

    // Related parameters estimation
    axis1 = 6.9575 * ExponentMultiple(relatedpressure, -0.3461,
                                      relatedtemperature, 0.2535,
                                      1.3766)-1;

    axis2 = 42.1309 * ExponentMultiple(relatedpressure, -0.3068,
                                       relatedtemperature, 1.2023,
                                       2.5147)-1;

    derivedc = log(axis2/axis1)/log(3.5);

    derivedxila1 = 6.7665 * ExponentMultiple(relatedpressure, -0.5050,
                                             relatedtemperature, 0.5106,
                                             1.5663)-1;

    derivedxila2 = 27.8843 * ExponentMultiple(relatedpressure, -0.4908,
                                              relatedtemperature, -0.8491,
                                              0.5496)-1;

    deriveda = log(derivedxila2/derivedxila1)/log(3.5);


    weight1 = 0.9544 * relatedpressure * pow(relatedtemperature, 0.69)
               + 0.0061 * waterVaporDensity;

    weight2 = 0.9500 * relatedpressure * pow(relatedtemperature, 0.64)
              + 0.0067 * waterVaporDensity;

    weight3 = 0.9561 * relatedpressure * pow(relatedtemperature, 0.67)
              + 0.0059 * waterVaporDensity;

    weight4 = 0.9543 * relatedpressure * pow(relatedtemperature, 0.68)
              + 0.0061 * waterVaporDensity;

    weight5 = 0.9550 * relatedpressure * pow(relatedtemperature, 0.68)
              + 0.0060 * waterVaporDensity;

    g22 = 1.0 + pow((frequency - 22.235), 2.0)/pow((frequency + 22.235), 2.0);

    g557 = 1.0 + pow((frequency-557.0), 2.0)/pow((frequency+557.0), 2.0);

    g752 = 1.0 + pow((frequency - 752.0), 2.0)/pow((frequency + 752.0), 2.0);

    item1 = 3.13 * pow(10.0, -2.0) * relatedpressure *  relatedtemperature +
            1.76 * pow(10.0,-3.0) * waterVaporDensity *
            pow(relatedtemperature, 8.5);

    item2 = 3.84 * weight1 * g22 * exp(2.23 * (1.0 - relatedtemperature))/
            (pow((frequency - 22.235), 2.0) + 9.42 * pow(weight1, 2.0));

    item3 = 10.48 * weight2 * exp(0.7 * (1.0 - relatedtemperature))/
            (pow((frequency - 183.31), 2.0) + 9.48 * pow(weight2, 2.0));

    item4 = 0.078 * weight3 * exp(6.4385 * (1.0 - relatedtemperature))/
            (pow((frequency-321.226), 2.0) + 6.29 * pow(weight3, 2.0));

    item5 = 3.76 * weight4 * exp( 1.6 * (1.0 - relatedtemperature))/
            (pow((frequency - 325.153), 2.0) + 9.22 * pow(weight4, 2.0));

    item6 = 26.36 * weight5 * exp(1.09 * (1.0 - relatedtemperature))/
            pow((frequency-380.0), 2.0);

    item7 = 17.87 * weight5 * exp(1.46 * (1.0 - relatedtemperature))/
            pow((frequency - 448.0), 2.0);

    item8 = frequency * relatedtemperature * deriveda * derivedc * 0.8837
            * weight5 * g557 * exp(0.17 * (1.0 - relatedtemperature))/
            pow((frequency - 557.0), 2.0);

    item9 = 302.6 * weight5 * g752 * exp(0.41*(1-relatedtemperature))/
            pow((frequency - 752.0), 2.0);

    sumitems = item2 + item3 + item4 + item5 + item6 + item7 + item8 + item9;

    // Water vapor attenuation coefficient estimation
    waterVaporAttenuationCoefficient =
            (item1 + pow(relatedtemperature, 2.5) * sumitems) *
             pow(frequency, 2.0) * waterVaporDensity * pow(10.0, -4.0);

    return waterVaporAttenuationCoefficient;

}

/* to represent Atmospheric Attenuation only humidity and temperature are required
 pressure and density are derived from them */
static
double SatelliteLinkAtmosphericAttenuation(double distanceOxygen,    // in Km
                                          double distanceWaterVapor, // in Km
                                          double frequency,  // in GHz
                                          double humidity,
                                          double temperature)
{

    double saturationPressure;
    double waterVaporPressure;
    double waterVaporDensity;
    double attenuationCoefficientWaterVapor;
    double attenuationCoefficientOxygen;
    double atmosphereattenuation;

    // Saturation pressure estimation for given temperature.
    //ITU-R Recommendation P.453-7
    saturationPressure = 6.1121 * exp(17.502 * temperature /
                        ( 240.97 + temperature));

    // Water vapor pressure estimation for given humidity
    waterVaporPressure = (humidity)*saturationPressure/100.0;

    // Water vapor density estimation for given temperature.
    //ITU-R Recommendation P.453-6
    waterVaporDensity = 216.7 * waterVaporPressure / (temperature + 273.3);

    //  Oxygen attenuation coefficient estimation for given  water vapor
    //  pressure, temperature and frequency.
    attenuationCoefficientOxygen =
             SatelliteLinkOxygenAttenuationCoefficient(waterVaporPressure,
                                                      temperature,
                                                      frequency);

    // Water vapor attenuation coefficient estimation for given pressure,
    // temperature, water vapor density and frequency.
    attenuationCoefficientWaterVapor =
            SatelliteLinkWaterVaporAttenuationCoefficient(waterVaporPressure,
                                                         temperature,
                                                         waterVaporDensity,
                                                         frequency);

    // Atmosphere attenuation estimation for given distance, oxygen
    // attenuation coefficient and water vapor attenuation coefficient.
    atmosphereattenuation =
            distanceOxygen * attenuationCoefficientOxygen +
            distanceWaterVapor * attenuationCoefficientWaterVapor;

    return atmosphereattenuation;
}


// Estimate the rain attenuation coefficient for given rain intensity and
// other parameters
static
double SatelliteLinkRainAttenuationCoefficient(double rainintensity,
                                              double kfactor,
                                              double alphafactor)
{

    double rainAttenuationCoefficient;

    rainAttenuationCoefficient = kfactor * pow(rainintensity, alphafactor);

    return rainAttenuationCoefficient;
}


/*
  Calculate the rain attenuation according to ITU-R Recommendation P.530-8,
  herein frequency is in GHz, distance is in km, and rain intensity is in mm/h.
*/
static
double SatelliteLinkRainAttenuation(double frequency,   //in GHz
                                   double distance,    //in km
                                   int polarization,
                                   double rainintensity)
{

    int i, frequencyindex = 0;
    double rainattenuationcoefficient_k = 0.0;
    double rainattenuationcoefficient_alpha = 0.0;
    double rainattenuationperunitlength;
    double referencedistance;
    double effectiveoathlength;
    double rainattenuation;

    ERROR_Assert(frequency >= 1.0,
                 "Satellite point to point link:\n"
                 "      Please verify the value for frequency\n");

    for (i = 0; i < 26; i++){
        if (frequency > rainAttenuationParameters[i][0])
        {
           frequencyindex = i;

        }
    }

    //Get the attenuation coefficient for given frequency and polarization

    if (polarization == HORIZONTAL){

        rainattenuationcoefficient_k =
            ((rainAttenuationParameters[frequencyindex+1][1] -
            rainAttenuationParameters[frequencyindex][1])/
            (rainAttenuationParameters[frequencyindex+1][0]-
            rainAttenuationParameters[frequencyindex][0]))*
            (frequency - rainAttenuationParameters[frequencyindex][0])
            + rainAttenuationParameters[frequencyindex][1];

        rainattenuationcoefficient_alpha =
            ((rainAttenuationParameters[frequencyindex+1][3] -
            rainAttenuationParameters[frequencyindex][3])/
            (rainAttenuationParameters[frequencyindex+1][0]-
            rainAttenuationParameters[frequencyindex][0]))*
            (frequency - rainAttenuationParameters[frequencyindex][0])
            + rainAttenuationParameters[frequencyindex][3];
    }
    else if (polarization == VERTICAL){

        rainattenuationcoefficient_k =
            ((rainAttenuationParameters[frequencyindex+1][2] -
            rainAttenuationParameters[frequencyindex][2])/
            (rainAttenuationParameters[frequencyindex+1][0]-
            rainAttenuationParameters[frequencyindex][0]))*
            (frequency - rainAttenuationParameters[frequencyindex][0])
            + rainAttenuationParameters[frequencyindex][2];

        rainattenuationcoefficient_alpha =
            ((rainAttenuationParameters[frequencyindex+1][4] -
            rainAttenuationParameters[frequencyindex][4])/
            (rainAttenuationParameters[frequencyindex+1][0]-
            rainAttenuationParameters[frequencyindex][0]))*
            (frequency - rainAttenuationParameters[frequencyindex][0])
            + rainAttenuationParameters[frequencyindex][4];

    }

    // Get the attenuation for per unit length for given attenuation
    // coefficient and rain intensity.
    rainattenuationperunitlength =
        SatelliteLinkRainAttenuationCoefficient(
                                    rainintensity,
                                    rainattenuationcoefficient_k,
                                    rainattenuationcoefficient_alpha);

    if (rainintensity <= 100){

        referencedistance = 35.0 * exp(-0.015 * rainintensity);
    }else{

        referencedistance = 100.0;
    }

    effectiveoathlength = distance/(1.0 + distance / referencedistance);

    // rain attenuation estimation
    rainattenuation = rainattenuationperunitlength * effectiveoathlength;

    return rainattenuation;
}


static //inline//
double PathlossFreeSpace(double distance,
                         double waveLength)
{
    double pathloss_dB = 0.0;
    double valueForLog = 4.0 * PI * distance / waveLength;

    if (valueForLog > 1.0) {
        pathloss_dB = 20.0 * log10(valueForLog);
    }

    return pathloss_dB;
}


static
double subValueCalculation(double base,
                       double exponent,
                       double coefficient)
{
    double subValue;
    subValue = 1.0 + coefficient * pow (base, exponent);

    return subValue;
}


// Calculate the oxygen equivalent height according to ITU-R Recommendation P.676
static
double GetOxygenEquivalentHeight(double atmosphericPressure,
                double frequency)  // frequency in GHz
{
    double relativePressure;
    double t1;
    double t1FirestPart;
    double t1SecondPart;
    double t2;
    double t2FirstPart;
    double t2SecondPart;
    double t3;
    double t3FirstPart;
    double t3SecondPart;
    double t3ThirdPart;
    double equivalentHeight;
    double equivalentHeightFirstPart;

    //relative pressure estimation
    relativePressure = atmosphericPressure / 1031.0;

    t1FirestPart = 4.64 / subValueCalculation(relativePressure, -2.3, 0.066);

    t1SecondPart =
            (frequency - 59.7) / (2.87 + 12.4 * exp (-7.9 * relativePressure));

    t1 = t1FirestPart * exp ( - pow(t1SecondPart, 2));

    t2FirstPart = 0.14 * exp(2.12 * relativePressure);

    t2SecondPart =
            pow((frequency - 118.75), 2) + 0.031 * exp(2.2 * relativePressure);

    t2 = t2FirstPart / t2SecondPart;

    t3FirstPart =
            frequency * 0.0114 /
            subValueCalculation(relativePressure, -2.6, 0.14);

    t3SecondPart = -0.0247 + 0.0001 * frequency + 1.61e-6 * pow(frequency, 2);

    t3ThirdPart =
            1.0 - 0.0169 * frequency + 4.1e-5 *
            pow(frequency, 2) + 3.2e-7 * pow(frequency, 3);

    t3 = t3FirstPart * t3SecondPart / t3ThirdPart;

    equivalentHeightFirstPart =
                6.1 / subValueCalculation(relativePressure, -1.1, 0.17);

    //equivalent oxygen height estimation
    equivalentHeight =  equivalentHeightFirstPart * ( 1.0 + t1 + t2 + t3);

    double upperLimit = subValueCalculation(relativePressure, 0.3, 10.7) - 1.0;

    if (equivalentHeight > upperLimit)
    {
        if (frequency < 70.0)
        {
            equivalentHeight = upperLimit;
        }
    }

    return equivalentHeight;
}



static
double subCalculation(double coefficient,
                      double parameter1,
                      double parameter2,
                      double parameter3,
                      double coefficient0)
{
    double subValue;
    double numerator;
    double denominator;

    numerator = coefficient * parameter1;
    denominator = pow((parameter2 - parameter3), 2) + coefficient0 * parameter1;
    subValue = numerator / denominator;

    return subValue;
}


// Calculate the equivalent height for water vapour according
// to ITU-R Recommendation P.676

static
double GetWaterVapourEquivalentHeight(double atmosphericPressure,
                double frequency) // frequency in GHz
{

    double relativePressure;
    double t1;
    double t2;
    double t3;
    double rouW;
    double firstPart;
    double secondPart;
    double thirdPart;
    double equivalentHeight;

    // relative pressure estimation
    relativePressure = atmosphericPressure/1031.0;
    rouW = 1.013 / ( 1.0 + exp (-8.6 * (relativePressure - 0.57)));

    firstPart = subCalculation(1.39,
                          rouW,
                          frequency,
                          22.235,
                          2.56);

    secondPart = subCalculation(3.37,
                          rouW,
                          frequency,
                          183.31,
                          4.69);

    thirdPart = subCalculation(1.58,
                          rouW,
                          frequency,
                          325.1,
                          2.89);

    //the water vapour equivalent height estimation
    equivalentHeight = 1.66 * ( 1 + firstPart + secondPart + thirdPart);

    return equivalentHeight;
}

/*
Calculate the pathloss for the link, free space is used to estimate the
propagation attenuation, up on the propagation attenuation, additional
attenuation due to rain and atmospheric are added to obtain the total pathloss.
*/
static
double SatelliteLinkcalculatepathloss(Node* node,
                                     LinkData* link,
                                     Message *msg)
{

    Coordinates nodePosition;
    Coordinates sourcePosition;
    double distance;
    double pathloss_dB;
    double diffractionloss;
    double rainattenuation;
    double atmosphericattenuation;
    double pathlosstotal;

    PartitionData* partition = node->partitionData;

    SatelliteLinkParameters *linkvar =
                       (SatelliteLinkParameters*)link->linkVar;

    SatelliteLinkInfo* linkInfo = (SatelliteLinkInfo*) MESSAGE_ReturnInfo(msg);

    ERROR_Assert(partition->terrainData->getCoordinateSystem() == LATLONALT,
                 "Satellite point to point link:\n"
                 "      Please select the LATLONALT coordinate system\n");

    MOBILITY_ReturnCoordinates(node, &nodePosition);

    WirelessLinkSiteParameters* sourcesiteparameters =
                            &(linkInfo->sourceSiteParameters);
    WirelessLinkSiteParameters* destsiteparameters = &(linkvar->rxPara);

    sourcePosition = linkInfo->sourcecoordinates;

    COORD_CalcDistance(
        partition->terrainData->getCoordinateSystem(),
        &sourcePosition, &nodePosition, &distance);

    ERROR_Assert(distance > 0.0,
                 "Satellite point to point link:\n"
                 "      Please increase the distance for the link\n");

    linkInfo->distance = distance;

    // the elevation angle for the ground station
    double elevationAngle =
                    SatelliteLinkGetElevationAngle(
                                node,
                                &sourcePosition,
                                &nodePosition);

    //Satellite is not reachable
    if (elevationAngle < 3.0){

        ERROR_ReportError("Satellite point to point link:\n"
            "     The satellite is not reachable from the ground station\n");
    }

    double wavelength = link->propSpeed / sourcesiteparameters->Frequency;

    pathloss_dB = PathlossFreeSpace(distance, wavelength);

    double saturationPressure = 6.1121 * exp(17.502 * linkvar->temperature /
                        ( 240.97 + linkvar->temperature));

    double atmosphericPressure =
        (linkvar->humidity) * saturationPressure / 100.0;

    // using the rough value 4.5 km for rain height, refer to ITU-R
    // Recommendation P.839 for the rain height value at different position
    double rainHeight = DEFAUL_RAIN_EQUIVALENT_HEIGHT;

    double slantPathLength = rainHeight / sin(elevationAngle);

    // determine the equivalent heights for oxygen
    double oxygenEquivalentHeight =
                GetOxygenEquivalentHeight(atmosphericPressure,
                            sourcesiteparameters->Frequency/1.0e9);

    // determine the equivalent heights for water vapor
    double waterVapourEquivalentHeight =
                GetWaterVapourEquivalentHeight(atmosphericPressure,
                            sourcesiteparameters->Frequency/1.0e9);

    // slant path length for the oxygen
    oxygenEquivalentHeight /= sin(elevationAngle);

    // slant path length for the water vapour
    waterVapourEquivalentHeight /= sin(elevationAngle);

    // Determine the specific attenuation value for rain
    rainattenuation = 0.0;

    if (sourcesiteparameters->Frequency > 1.0e9)
    {
        rainattenuation = SatelliteLinkRainAttenuation(
                                    sourcesiteparameters->Frequency/1.0e9,
                                    slantPathLength/1000.0,
                                    sourcesiteparameters->polarization,
                                    linkvar->rainIntensity);
    }

    // Determine the total slant path gaseous attenuation through
    // the atmosphere.
    atmosphericattenuation = SatelliteLinkAtmosphericAttenuation(
                                    oxygenEquivalentHeight/1.0e3,
                                    waterVapourEquivalentHeight/1.0e3,
                                    sourcesiteparameters->Frequency/1.0e9,
                                    linkvar->humidity,
                                    linkvar->temperature);

    // Determine the total slant path attenuation for the link
    pathlosstotal = pathloss_dB + rainattenuation + atmosphericattenuation;

    if (DEBUG) {
        printf("dest node  %d  dis %f  loss %f \n",
               node->nodeId, distance, pathlosstotal);
    }

    return pathlosstotal;

}




static
double SatelliteLinkCalculateFading(
            Node* node,
            SatelliteLinkParameters *linkInfo)
{
    double fading_dB = 0.0;

    if (linkInfo->fadingModel == RICEAN) {
        int arrayIndex;
        double arrayIndexInDouble;
        double value1, value2;

        const double kFactor = linkInfo->kFactor;
        const int numGaussianComponents = linkInfo->numGaussianComponents;
        const int startingPoint = linkInfo->startingPoint;
        clocktype currentTime;


        currentTime = node->getNodeTime();

        arrayIndexInDouble =
            linkInfo->fadingStretchingFactor * (double) currentTime;

        arrayIndexInDouble -=
            (double)numGaussianComponents *
            floor(arrayIndexInDouble / (double)numGaussianComponents);

        arrayIndex =
            (RoundToInt(arrayIndexInDouble) + startingPoint) %
            numGaussianComponents;

        value1 = linkInfo->gaussianComponent1[arrayIndex] +
            sqrt(2.0 * kFactor);
        value2 = linkInfo->gaussianComponent2[arrayIndex];

        fading_dB =
            (double)IN_DB((value1 * value1 + value2 * value2) / (2.0 * (kFactor + 1)));
    }

    return fading_dB;
}

static
BOOL SatelliteLinkCheckPacketError(Node* node,
                                  LinkData* link,
                                  Message *msg)
{
    double rxpower;

    //Get the satellite link info
    SatelliteLinkInfo* wli = (SatelliteLinkInfo*) MESSAGE_ReturnInfo(msg);

    WirelessLinkSiteParameters* sourcesiteparameter =
                                   &(wli->sourceSiteParameters);

    // Get the satellite link parameters
    SatelliteLinkParameters *linkvar =
                   (SatelliteLinkParameters*)link->linkVar;

    WirelessLinkSiteParameters* destsiteparameter = &(linkvar->rxPara);

    // free space pathloss
    double pathloss = SatelliteLinkcalculatepathloss(node, link, msg);

    // Estimate the signal power
    if ((sourcesiteparameter->Frequency == destsiteparameter->Frequency) &&
        (sourcesiteparameter->polarization == destsiteparameter->polarization))
    {
        double fadingdB = SatelliteLinkCalculateFading(
                    node,
                    linkvar);

        rxpower =
                sourcesiteparameter->TxPower_dBm
                + sourcesiteparameter->AntennaGain
                + destsiteparameter->AntennaGain
                - sourcesiteparameter->totalLossdB
                - destsiteparameter->totalLossdB
                - pathloss - fadingdB;

    }
    else{

        rxpower = IN_DB(destsiteparameter->noise_mW);
    }

    // Estimate the instant SINR for the packet
    double sinr = NON_DB(rxpower) / destsiteparameter->noise_mW;

    if (DEBUG)
    {
        printf("dest node  %d  sinr %f  sinr threshold %f \n",
               node->nodeId, sinr, linkvar->rxThreshold);
    }

    //Packet reception quality estimation
    if (sinr < linkvar->rxThreshold) {
        return TRUE;
    }

    return FALSE;

}


void SatelliteLinkMessageArrivedFromLink(
    Node *node,
    int interfaceIndex,
    Message *msg)
{

    Mac802Address sourceAddr;
    Mac802Address destAddr;
    BOOL isVlanTag = FALSE;
    MacHeaderVlanTag tagInfo;

    LinkData *link = (LinkData*)node->macData[interfaceIndex]->macVar;

    LinkFrameHeader *header = (LinkFrameHeader *) MESSAGE_ReturnPacket(msg);

    SatelliteLinkInfo* wli = (SatelliteLinkInfo*) MESSAGE_ReturnInfo(msg);

    MAC_CopyMac802Address(&sourceAddr,&header->sourceAddr);
    MAC_CopyMac802Address(&destAddr,&header->destAddr);

    isVlanTag = header->vlanTag;

    link->myMacData->propDelay = wli->propDelay;

    ERROR_Assert(link->phyType == SATELLITE,
                 "Satellite point to point link:\n"
                 "      Please check the link type\n");

    // receiving end of wireless link can calculate/store propDelay

    BOOL packeterror = SatelliteLinkCheckPacketError(node, link, msg);

    if (packeterror == FALSE)
    {

        MESSAGE_RemoveHeader(node, msg, sizeof(LinkFrameHeader), TRACE_LINK);
        memset(&tagInfo, 0, sizeof(MacHeaderVlanTag));

        if (isVlanTag)
        {
            // Vlan tag present in header.
            // Collect vlan information and remove header
            MacHeaderVlanTag *tagHeader = (MacHeaderVlanTag *)
                                      MESSAGE_ReturnPacket(msg);

            memcpy(&tagInfo, tagHeader, sizeof(MacHeaderVlanTag));

            MESSAGE_RemoveHeader(
                node, msg, sizeof(MacHeaderVlanTag), TRACE_VLAN);
        }


        MacHWAddress destHWAddr;
        Convert802AddressToVariableHWAddress(node,&destHWAddr,&destAddr);
        BOOL isMyAddr = FALSE;

        if (NetworkIpIsUnnumberedInterface(node, interfaceIndex))
        {
            isMyAddr = (MAC_IsBroadcastHWAddress(&destHWAddr) ||
                            MAC_IsMyAddress(node, &destHWAddr));
        }
        else
        {
           isMyAddr = MAC_IsMyHWAddress(node, interfaceIndex, &destHWAddr);
        }

        if (isMyAddr)
        {
            // Either broadcast frame or frame for this station
            MacHWAddress srcHWAddr;
            Convert802AddressToVariableHWAddress(node,&srcHWAddr,&sourceAddr);
            MAC_HandOffSuccessfullyReceivedPacket(
                node, interfaceIndex, msg, &srcHWAddr);


            link->stats.packetsReceived++;
        }
        else
        {
            MESSAGE_Free(node, msg);
        }
    }
    else
    {
#ifdef ADDON_DB
        MESSAGE_RemoveHeader(node, msg, sizeof(LinkFrameHeader), TRACE_LINK);
        memset(&tagInfo, 0, sizeof(MacHeaderVlanTag));

        if (isVlanTag)
        {
            // Vlan tag present in header.
            // Collect vlan information and remove header
            MacHeaderVlanTag *tagHeader = (MacHeaderVlanTag *)
                    MESSAGE_ReturnPacket(msg);

            memcpy(&tagInfo, tagHeader, sizeof(MacHeaderVlanTag));

            MESSAGE_RemoveHeader(
                                 node, msg, sizeof(MacHeaderVlanTag),
                                 TRACE_VLAN);
        }

        MacHWAddress destHWAddr;
        Convert802AddressToVariableHWAddress(node,&destHWAddr,&destAddr);
        BOOL isMyAddr = FALSE;

        if (NetworkIpIsUnnumberedInterface(node, interfaceIndex))
        {
            isMyAddr = (MAC_IsBroadcastHWAddress(&destHWAddr) ||
                    MAC_IsMyAddress(node, &destHWAddr));
        }
        else
        {
            isMyAddr = MAC_IsMyHWAddress(node, interfaceIndex, &destHWAddr);
        }

        if (isMyAddr)
        {
            HandleMacDBEvents(
                              node,
                              msg,
                              node->macData[interfaceIndex]->phyNumber,
                              interfaceIndex,
                              MAC_Drop,
                              node->macData[interfaceIndex]->macProtocol,
                              TRUE,
                              "Packet With Error");
        }

#endif
        MESSAGE_Free(node, msg);
    }
}



static
void SatelliteLinkInitializeFadingModel(
    Node *node,
    SatelliteLinkParameters *linkInfo,
    Address* address,
    const NodeInput *nodeInput)
{
    char buf[MAX_STRING_LENGTH];
    double kFactor;
    double maxVelocity;
    BOOL wasFound;
    BOOL fadingOnChannel = FALSE;
    double dopplerFrequency = 0.0;

    IO_ReadString(
         node->nodeId,
         address,
         nodeInput,
         "LINK-SATELLITE-PROPAGATION-FADING-MODEL",
         &wasFound,
         buf);

     if (wasFound) {
        if (strcmp(buf, "NONE") == 0) {
            linkInfo->fadingModel = NONE;
        }
        else if (strcmp(buf, "RAYLEIGH") == 0) {
            //
            // When Rayleigh fading is specified, Ricean with K = 0 is
            // actually set.
            //
            linkInfo->fadingModel = RICEAN;
            linkInfo->kFactor = 0.0;
        }
        else if (strcmp(buf, "RICEAN") == 0) {
            linkInfo->fadingModel = RICEAN;

            //
            // Set K factor
            //
            IO_ReadDouble(
                   node->nodeId,
                   address,
                   nodeInput,
                   "LINK-SATELLITE-PROPAGATION-RICEAN-K-FACTOR",
                   &wasFound,
                   &kFactor);

            if (wasFound) {
                linkInfo->kFactor = kFactor;
            }
            else {
                ERROR_ReportError(
                    "Error: LINK-SATELLITE-PROPAGATION-RICEAN-K-FACTOR"
                    " is required for the specified fading model");
            }
        }
        else if (strcmp(buf, "LUZE") == 0) {

            linkInfo->fadingModel = RICEAN;
            linkInfo->kFactor = 0.0;
        }
        else {
            char errorMessage[MAX_STRING_LENGTH];
            sprintf(errorMessage,
                    "Error: unknown LINK-SATELLITE-PROPAGATION-FADING-MODEL '%s'.\n",
                    buf);
            ERROR_ReportError(errorMessage);
        }
    }
    else {
        linkInfo->fadingModel = NONE;
    }

    if (linkInfo->fadingModel == RICEAN) {

        fadingOnChannel = TRUE;

        IO_ReadDouble(
             node->nodeId,
             address,
             nodeInput,
             "LINK-SATELLITE-PROPAGATION-FADING-MAX-VELOCITY",
             &wasFound,
             &maxVelocity);

        if (!wasFound) {
            maxVelocity = DEFAULT_MAX_VELOCITY;
        }

        ERROR_Assert(maxVelocity > 0.0,
                 "Satellite point to point link:\n"
                 "      Please veirfy the value for parameter "
                 " LINK-SATELLITE-PROPAGATION-FADING-MAX-VELOCITY\n");

        double wavelength = linkInfo->txPara.propSpeed / linkInfo->txPara.Frequency;

        dopplerFrequency = maxVelocity / wavelength;
    }


    if (fadingOnChannel == TRUE) {
        int i;
        char Token[MAX_STRING_LENGTH];
        char *StrPtr;
        int startLine = 0;
        int numItems;

        NodeInput fadingInput;
        double baseDopplerFrequency = 0.0;
        int numGaussianComponents = 0;
        Int32 samplingRate = 0;

        IO_ReadCachedFile(
            node->nodeId,
            address,
            nodeInput,
            "LINK-SATELLITE-PROPAGATION-FADING-GAUSSIAN-COMPONENTS-FILE",
            &wasFound,
            &fadingInput);

        if (!wasFound) {
            ERROR_ReportError("LINK-SATELLITE-PROPAGATION-FADING-GAUSSIAN-COMPONENTS-FILE is missing");
        }

        for (i = 0; i < 3; i++) {
            IO_GetToken(Token, fadingInput.inputStrings[i], &StrPtr);

            if (strcmp(Token, "NUMBER-OF-GAUSSIAN-COMPONENTS") == 0) {
                IO_GetToken(Token, StrPtr, &StrPtr);
                numGaussianComponents = (int)atoi(Token);
            }
            else if (strcmp(Token, "SAMPLING-RATE") == 0) {
                IO_GetToken(Token, StrPtr, &StrPtr);
                samplingRate = (int)atoi(Token);
            }
            else if (strcmp(Token, "BASE-DOPPLER-FREQUENCY") == 0) {
                IO_GetToken(Token, StrPtr, &StrPtr);
                baseDopplerFrequency = (double)atof(Token);
            }
            else {
                char errorMessage[MAX_STRING_LENGTH];

                sprintf(errorMessage,
                        "Unknown variable '%s'\n"
                        "LINK-SATELLITE-PROPAGATION-FADING-GAUSSIAN-COMPONENTS-FILE "
                        "expects the following three variables "
                        "at the beginning of file:\n"
                        "    NUMBER-OF-GAUSSIAN-COMPONENTS\n"
                        "    SAMPLING-RATE\n"
                        "    BASE-DOPPLER-FREQUENCY",
                        Token);

                ERROR_ReportError(errorMessage);
            }
        }

        linkInfo->numGaussianComponents = numGaussianComponents;

        linkInfo->fadingStretchingFactor =
            (double)(samplingRate) *
            dopplerFrequency /
            baseDopplerFrequency /
            (double)SECOND;

        linkInfo->gaussianComponent1 =
            (double *)MEM_malloc(numGaussianComponents * sizeof(double));
        linkInfo->gaussianComponent2 =
            (double *)MEM_malloc(numGaussianComponents * sizeof(double));

        startLine += 3;
        numItems = 0;

        for (i = startLine; i < fadingInput.numLines; i++) {
            IO_GetToken(Token, fadingInput.inputStrings[i], &StrPtr);

            linkInfo->gaussianComponent1[numItems] = (double)atof(Token);

            IO_GetToken(Token, StrPtr, &StrPtr);

            linkInfo->gaussianComponent2[numItems] = (double)atof(Token);

            numItems++;
        }

        assert(numItems == numGaussianComponents);

        linkInfo->startingPoint =
                RANDOM_nrand(linkInfo->seed) % numGaussianComponents;
    }
    else {

        linkInfo->fadingStretchingFactor = 0.0;
        linkInfo->numGaussianComponents = 0;
        linkInfo->gaussianComponent1 = NULL;
        linkInfo->gaussianComponent2 = NULL;
    }

}



void SatelliteLinkInitialize(
    Node *node,
    LinkData *link,
    Address* address,
    const NodeInput *nodeInput)
{

    BOOL    wasFound;
    double  txAntennaEfficiency;
    double  rxAntennaEfficiency;
    double  rxThreshold;
    double  noiseFactor;
    double  dataRate;
    double  humidity;
    double  rain_intensity;
    double  bandwidth;
    double  temperature;
    char    polarization[MAX_STRING_LENGTH];
    double  antennadishdiameter;    //in meter
    double  txConnectionLoss;       // in dB
    double  rxConnectionLoss;       // in dB
    double  txMismatchLoss;         // in dB
    double  rxMismatchLoss;         // in dB
    double  txCableLoss;     // in dB
    double  rxCableLoss;     // in dB
    double  rxTotalLossdB;       // in dB
    double  txTotalLossdB;
    double  totalLoss;
    double  noise_mW;
    double  systemNoiseTemperature;
    double  frequency;
    double  antennatemperature;
    double  txPower;
    double  rxSensitivity;
    char    nodeType[MAX_STRING_LENGTH];

    SatelliteLinkParameters *Linkinfo =
         (SatelliteLinkParameters *)MEM_malloc(sizeof( SatelliteLinkParameters));

    link->linkVar = Linkinfo;

    IO_ReadString(
        node->nodeId,
        address,
        nodeInput,
        "LINK-SATELLITE-NODE-TYPE",
        &wasFound,
        nodeType);

    if (wasFound) {
        if (strcmp(nodeType, "SATELLITE") == 0) {
            Linkinfo->txPara.nodetype = SATELLITE_NODE;
            Linkinfo->rxPara.nodetype = SATELLITE_NODE;
        }
        else if (strcmp(nodeType, "GROUND") == 0) {
            Linkinfo->txPara.nodetype = GROUND_STATION;
            Linkinfo->rxPara.nodetype = GROUND_STATION;
        }
        else {
            ERROR_ReportError(" Satellite point to point link:\n"
                 "      Unrecognized node type\n");

        }
    }
    else {

        ERROR_ReportError("Satellite point to point link:\n"
            "      Please specify the node type\n");
    }

    IO_ReadDouble(
         node->nodeId,
         address,
         nodeInput,
         "LINK-SATELLITE-PROPAGATION-HUMIDITY",
         &wasFound,
         &humidity);


    if (wasFound) {
        Linkinfo->humidity = humidity;
    }
    else {
        Linkinfo->humidity = DEFAULT_LINK_HUMIDITY;
    }


    IO_ReadDouble(
         node->nodeId,
         address,
         nodeInput,
         "LINK-SATELLITE-PROPAGATION-TEMPERATURE",
         &wasFound,
         &temperature);

    if (wasFound) {
        Linkinfo->temperature = temperature;
    }
    else {
        Linkinfo->temperature = DEFAULT_TEMPERATURE;
    }

    // refer to ITU-R Recommendation P.837-1 to have rain intensity value
    // for different zones

    IO_ReadDouble(
         node->nodeId,
         address,
         nodeInput,
         "LINK-SATELLITE-PROPAGATION-RAIN-INTENSITY",
         &wasFound,
         &rain_intensity);


    if (wasFound) {
       Linkinfo->rainIntensity = rain_intensity;
    }
    else {
       Linkinfo->rainIntensity = DEFAULT_RAIN_INTENSITY;
    }

    Linkinfo->txPara.propSpeed = link->propSpeed;


    IO_ReadString(
        node->nodeId,
        address,
        nodeInput,
        "LINK-SATELLITE-TX-ANTENNA-POLARIZATION",
        &wasFound,
        polarization);

    if (wasFound) {
        if (strcmp(polarization, "HORIZONTAL") == 0) {
            Linkinfo->txPara.polarization = HORIZONTAL;
        }
        else if (strcmp(polarization, "VERTICAL") == 0) {
            Linkinfo->txPara.polarization = VERTICAL;
        }
        else {
            ERROR_ReportError("Satellite point to point link:\n"
                    "      Unrecognized polarization\n");
        }
    }
    else {
        Linkinfo->txPara.polarization = VERTICAL;
    }


    IO_ReadString(
        node->nodeId,
        address,
        nodeInput,
        "LINK-SATELLITE-RX-ANTENNA-POLARIZATION",
        &wasFound,
        polarization);

    if (wasFound) {
        if (strcmp(polarization, "HORIZONTAL") == 0) {
            Linkinfo->rxPara.polarization = HORIZONTAL;
        }
        else if (strcmp(polarization, "VERTICAL") == 0) {
            Linkinfo->rxPara.polarization = VERTICAL;
        }
        else {
            ERROR_ReportError("Satellite point to point link:\n"
                    "      Unrecognized polarization\n");
        }
    }
    else {
        Linkinfo->rxPara.polarization = VERTICAL;
    }

    IO_ReadDouble(
         node->nodeId,
         address,
         nodeInput,
         "LINK-SATELLITE-TX-ANTENNA-DISH-DIAMETER",
         &wasFound,
         &antennadishdiameter);


    if (wasFound) {

        Linkinfo->txPara.AntennaDishDiameter = antennadishdiameter;
    }
    else {
        Linkinfo->txPara.AntennaDishDiameter =
                                  DEFAULT_SATELLITE_ANTENNA_DISH_DIAMETER;

    }

    if (Linkinfo->txPara.AntennaDishDiameter < 0.1)
    {
        ERROR_ReportError("Satellite point to point link:\n"
             "      Please increase the value for parameter"
             " LINK-SATELLITE-TX-ANTENNA-DISH-DIAMETER\n");
    }

    IO_ReadDouble(
         node->nodeId,
         address,
         nodeInput,
         "LINK-SATELLITE-TX-ANTENNA-EFFICIENCY",
         &wasFound,
         &txAntennaEfficiency);


    if (!wasFound) {

        txAntennaEfficiency = DEFAULT_ANTENNA_EFFICIENCY;

    }

    if (txAntennaEfficiency < 0.1 ||
        txAntennaEfficiency > 1.0 )
    {
        ERROR_ReportError("Satellite point to point link:\n"
             "      Please verify the value for parameter"
             " LINK-SATELLITE-TX-ANTENNA-EFFICIENCY\n");
    }

    IO_ReadDouble(
         node->nodeId,
         address,
         nodeInput,
         "LINK-SATELLITE-RX-ANTENNA-DISH-DIAMETER",
         &wasFound,
         &antennadishdiameter);


    if (wasFound) {

        Linkinfo->rxPara.AntennaDishDiameter = antennadishdiameter;
    }
    else {
        Linkinfo->rxPara.AntennaDishDiameter =
                                 DEFAULT_SATELLITE_ANTENNA_DISH_DIAMETER;

    }

    if (Linkinfo->rxPara.AntennaDishDiameter < 0.1)
    {
        ERROR_ReportError("Satellite point to point link:\n"
             "      Please increase the value for parameter"
             " LINK-SATELLITE-RX-ANTENNA-DISH-DIAMETER\n");
    }

    IO_ReadDouble(
         node->nodeId,
         address,
         nodeInput,
         "LINK-SATELLITE-RX-ANTENNA-EFFICIENCY",
         &wasFound,
         &rxAntennaEfficiency);


    if (!wasFound) {

        rxAntennaEfficiency = DEFAULT_ANTENNA_EFFICIENCY;

    }

    if (rxAntennaEfficiency < 0.1 ||
        rxAntennaEfficiency > 1.0)
    {
        ERROR_ReportError("Satellite point to point link:\n"
             "      Please verify the value for parameter"
             " LINK-SATELLITE-RX-ANTENNA-EFFICIENCY\n");
    }

    IO_ReadDouble(
         node->nodeId,
         address,
         nodeInput,
         "LINK-SATELLITE-DOWNLINK-FREQUENCY",
         &wasFound,
         &frequency);

    if (wasFound) {

        if (Linkinfo->txPara.nodetype == GROUND_STATION)
        {
            Linkinfo->rxPara.Frequency = frequency;
        }
        else
        {
            Linkinfo->txPara.Frequency = frequency;
        }
    }
    else {

        ERROR_ReportError("Satellite point to point link:\n"
                "      Please specify value for parameter"
                " LINK-SATELLITE-DOWNLINK-FREQUENCY\n");
    }


    IO_ReadDouble(
         node->nodeId,
         address,
         nodeInput,
         "LINK-SATELLITE-UPLINK-FREQUENCY",
         &wasFound,
         &frequency);

    if (wasFound) {

        if (Linkinfo->txPara.nodetype == GROUND_STATION)
        {
            Linkinfo->txPara.Frequency = frequency;
        }
        else
        {
            Linkinfo->rxPara.Frequency = frequency;
        }
    }
    else {

        ERROR_ReportError("Satellite point to point link:\n"
                   "      Please specify value for parameter"
                " LINK-SATELLITE-UPLINK-FREQUENCY\n");
    }

    if (Linkinfo->txPara.Frequency <= 0.0 ||
        Linkinfo->rxPara.Frequency <= 0.0)
    {
        ERROR_ReportError("Satellite point to point link:\n"
             "      Please verify the value for parameter"
             " LINK-SATELLITE-UPLINK-FREQUENCY or"
             " LINK-SATELLITE-DOWNLINK-FREQUENCY\n");
    }

    // Tx power in dBm
    IO_ReadDouble(
         node->nodeId,
         address,
         nodeInput,
         "LINK-SATELLITE-TX-POWER",
         &wasFound,
         &txPower);

    if (wasFound) {

        Linkinfo->txPara.TxPower_dBm = txPower;
    }
    else {

        Linkinfo->txPara.TxPower_dBm = DEFAULT_SATELLITE_TX_POWER;
    }


    IO_ReadDouble(
         node->nodeId,
         address,
         nodeInput,
         "LINK-SATELLITE-NOISE-FACTOR",
         &wasFound,
         &noiseFactor);

    if (!wasFound) {
        noiseFactor = DEFAULT_SATELLITE_NOISE_FACTOR;

    }

    if (noiseFactor <= 1.0)
    {
        ERROR_ReportError(" Satellite point to point link:\n"
             "      Please increase the value for parameter"
             " LINK-SATELLITE-NOISE-FACTOR\n");
    }

    double rxNoiseTemperature =
            (noiseFactor - 1.0) * DEFAULT_NOISE_TEMPERATURE_TZERO;

    IO_ReadDouble(
         node->nodeId,
         address,
         nodeInput,
         "LINK-SATELLITE-RX-SNR-THRESHOLD",
         &wasFound,
         &rxThreshold);

    if (wasFound) {

        Linkinfo->rxThreshold = rxThreshold;
    }
    else {
        Linkinfo->rxThreshold = DEFAULT_SNR_THRESHOLD;

    }

    if (Linkinfo->rxThreshold <= 1.0)
    {
        ERROR_ReportError(" Satellite point to point link:\n"
             "      Please increase the value for parameter"
             " LINK-SATELLITE-RX-SNR-THRESHOLD\n");
    }

    IO_ReadDouble(
         node->nodeId,
         address,
         nodeInput,
         "LINK-SATELLITE-DATA-RATE",
         &wasFound,
         &dataRate);

    if (wasFound) {

        Linkinfo->dataRate = dataRate;
    }
    else {
        Linkinfo->dataRate = DEFAULT_DATA_RATE;

    }

    if (Linkinfo->dataRate <= 0.0)
    {
        ERROR_ReportError(" Satellite point to point link:\n"
             "      Please verify the value for parameter"
             " LINK-SATELLITE-DATA-RATE\n");
    }

    IO_ReadDouble(
         node->nodeId,
         address,
         nodeInput,
         "LINK-SATELLITE-BANDWIDTH",
         &wasFound,
         &bandwidth);

    if (!wasFound) {

        bandwidth = DEFAULT_BANDWIDTH;

    }

    if (bandwidth <= 0.0 ||
        bandwidth >= Linkinfo->txPara.Frequency ||
        bandwidth >= Linkinfo->rxPara.Frequency)
    {
        ERROR_ReportError(" Satellite point to point link:\n"
             "      Please verify the value for parameter"
             " LINK-SATELLITE-BANDWIDTH\n");
    }

    IO_ReadDouble(
         node->nodeId,
         address,
         nodeInput,
         "LINK-SATELLITE-TX-CONNECTION-LOSS",
         &wasFound,
         &txConnectionLoss);

    if (!wasFound) {

        txConnectionLoss = DEFAULT_CONNECTION_LOSS;

    }

    if (txConnectionLoss < 0.0)
    {
        ERROR_ReportError(" Satellite point to point link:\n"
             "      Please verify the value for parameter"
             " LINK-SATELLITE-TX-CONNECTION-LOSS\n");
    }

    IO_ReadDouble(
         node->nodeId,
         address,
         nodeInput,
         "LINK-SATELLITE-TX-MISMATCH-LOSS",
         &wasFound,
         &txMismatchLoss);

    if (!wasFound) {

        txMismatchLoss = DEFAULT_MISMATCH_LOSS;

    }

    if (txMismatchLoss < 0.0)
    {
        ERROR_ReportError(" Satellite point to point link:\n"
             "      Please verify the value for parameter"
             " LINK-SATELLITE-TX-MISMATCH-LOSS\n");
    }

    IO_ReadDouble(
         node->nodeId,
         address,
         nodeInput,
         "LINK-SATELLITE-TX-ANTENNA-CABLE-LOSS",
         &wasFound,
         &txCableLoss);


    if (!wasFound) {

        txCableLoss = DEFAULT_ANTENNA_CABLE_LOSS;

    }

    if (txCableLoss < 0.0)
    {
        ERROR_ReportError(" Satellite point to point link:\n"
             "      Please verify the value for parameter"
             " LINK-SATELLITE-TX-ANTENNA-CABLE-LOSS\n");
    }

    txTotalLossdB = txConnectionLoss + txMismatchLoss + txCableLoss;

    Linkinfo->txPara.totalLossdB = txTotalLossdB;

    IO_ReadDouble(
         node->nodeId,
         address,
         nodeInput,
         "LINK-SATELLITE-RX-CONNECTION-LOSS",
         &wasFound,
         &rxConnectionLoss);

    if (!wasFound) {

        rxConnectionLoss = DEFAULT_CONNECTION_LOSS;

    }

    if (rxConnectionLoss < 0.0)
    {
        ERROR_ReportError(" Satellite point to point link:\n"
             "      Please verify the value for parameter"
             " LINK-SATELLITE-RX-CONNECTION-LOSS\n");
    }

    IO_ReadDouble(
         node->nodeId,
         address,
         nodeInput,
         "LINK-SATELLITE-RX-MISMATCH-LOSS",
         &wasFound,
         &rxMismatchLoss);

    if (!wasFound) {

        rxMismatchLoss = DEFAULT_MISMATCH_LOSS;

    }

    if (rxMismatchLoss < 0.0)
    {
        ERROR_ReportError(" Satellite point to point link:\n"
             "      Please verify the value for parameter"
             " LINK-SATELLITE-RX-MISMATCH-LOSS\n");
    }

    IO_ReadDouble(
        node->nodeId,
        address,
        nodeInput,
        "LINK-SATELLITE-RX-ANTENNA-CABLE-LOSS",
        &wasFound,
        &rxCableLoss);


    if (!wasFound) {

        rxCableLoss = DEFAULT_ANTENNA_CABLE_LOSS;

    }

    if (rxCableLoss < 0.0)
    {
        ERROR_ReportError(" Satellite point to point link:\n"
             "      Please verify the value for parameter"
             " LINK-SATELLITE-RX-ANTENNA-CABLE-LOSS\n");
    }

    rxTotalLossdB = rxMismatchLoss + rxConnectionLoss + rxCableLoss;

    Linkinfo->rxPara.totalLossdB = rxTotalLossdB;

    //the antenna temperature may include the contribution from sky noise
    //ambient noise and ground noise

    IO_ReadDouble(
         node->nodeId,
         address,
         nodeInput,
         "LINK-SATELLITE-ANTENNA-TEMPERATURE",
         &wasFound,
         &antennatemperature);

    if (!wasFound) {

        antennatemperature = DEFAULT_ANTENNA_TEMPERATURE;

    }

    if (antennatemperature <= 0.0)
    {
        ERROR_ReportError(" Satellite point to point link:\n"
             "      Please verify the value for parameter"
             " LINK-SATELLITE-ANTENNA-TEMPERATURE\n");
    }

    totalLoss = NON_DB(rxTotalLossdB);

    if (totalLoss < 1.0)
    {
        totalLoss = 1.0;
    }

    //the equivalent noise temperature at receiver input
    systemNoiseTemperature =
        antennatemperature * totalLoss + DEFAULT_NOISE_TEMPERATURE_TZERO *
        (totalLoss - 1.0) + rxNoiseTemperature;

    // noise power
    noise_mW
        = BOLTZMANN_CONSTANT * systemNoiseTemperature * bandwidth * 1000.0;

    Linkinfo->rxPara.noise_mW = noise_mW;

    Linkinfo->txPara.AntennaGain = SatelliteLinkAntennaParabolicMaxGain(
                                        Linkinfo->txPara.AntennaDishDiameter,
                                        Linkinfo->txPara.Frequency,
                                        txAntennaEfficiency,
                                        link->propSpeed);

    Linkinfo->rxPara.AntennaGain = SatelliteLinkAntennaParabolicMaxGain(
                                        Linkinfo->rxPara.AntennaDishDiameter,
                                        Linkinfo->rxPara.Frequency,
                                        rxAntennaEfficiency,
                                        link->propSpeed);

    RANDOM_SetSeed(Linkinfo->seed,
                   node->globalSeed,
                   node->nodeId,
                   SATELLITE);

    SatelliteLinkInitializeFadingModel(
            node,
            Linkinfo,
            address,
            nodeInput);

    return;
}

