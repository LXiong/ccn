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
#include "prop_itm.h"
#include "random.h"
#include "node.h"
#include "mac_link.h"
#include "mac_link_microwave.h"
#include "network_ip.h"

#define DEBUG 0

double WirelessLinkAntennaParabolicMaxGain(double DiameterinMeter,
                                           double FrequencyinHz,
                                           double propSpeed)
{
    double DiameterinWavelength;
    double GainPeak;
    double wavelength;

    assert(FrequencyinHz > 0.0);
    assert(DiameterinMeter > 0.01);

    wavelength = propSpeed / FrequencyinHz;
    DiameterinWavelength = DiameterinMeter / wavelength;

    GainPeak = 7.7 + 20.0 * log10(DiameterinWavelength);

   // ITU-R Recommendation F.699-5

    return GainPeak;
}


double WirelessLinkMaxDistance(double TxAntennaHeight,
                               double RxAntennaHeight)
{
    double distancefromTx;
    double distancefromRx;
    double maxdistance;
    double equivalentearthradius;

    // for standard atmoshere the equivalent earth radius is 8500 km instead
    // of its physical radius 6370 km

    equivalentearthradius = Constants_RadiusEarth;
    distancefromTx = sqrt(2.0 * TxAntennaHeight * equivalentearthradius);
    distancefromRx = sqrt(2.0 * RxAntennaHeight * equivalentearthradius);
    maxdistance = distancefromTx + distancefromRx;

    return maxdistance;

   //ITU-R Recommendation P.310-9
}

double fourtimesdifference(double parameter0,
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


double exponentmultiple(double base1,
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

double WirelessLinkOxygenAttenuationCoefficient(double AtmosphericPressure,
                                                double Temperature,
                                                double Frequency)
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


    relatedpressure = AtmosphericPressure/1031.0;
    relatedtemperature = 288.0/(273.0 + Temperature);

    if (Frequency <= 60.0) {

        frequencyexponents = 0.0;

    }else{

        frequencyexponents = -15.0;
    }

    axis1 = 6.9575 * exponentmultiple(relatedpressure, -0.3461,
                                      relatedtemperature, 0.2535,
                                      1.3766)-1.0;

    axis2 = 42.1309 * exponentmultiple(relatedpressure, -0.3068,
                                       relatedtemperature, 1.2023,
                                       2.5147)-1.0;

    derivedc = log(axis2/axis1)/log(3.5);

    derivedd = pow(4.0, derivedc)/axis1;

    derivedxila1 = 6.7665 * exponentmultiple(relatedpressure, -0.5050,
                                             relatedtemperature, 0.5106,
                                             1.5663)-1.0;

    derivedxila2 = 27.8843 * exponentmultiple(relatedpressure, -0.4908,
                                              relatedtemperature, -0.8491,
                                              0.5496)-1.0;

    deriveda = log(derivedxila2/derivedxila1)/log(3.5);

    derivedb = pow(4.0, deriveda)/derivedxila1;

    derived54prime = 2.128 * exponentmultiple(relatedpressure, 1.4954,
                                              relatedtemperature, -1.6032,
                                              -2.5280);

    derived54 = 2.136 * exponentmultiple(relatedpressure, 1.4975,
                                         relatedtemperature, -1.5852,
                                         -2.5196);

    derived57 = 9.984 * exponentmultiple(relatedpressure, 0.9313,
                                         relatedtemperature, 2.6732,
                                         0.9563);

    derived60 = 15.42 * exponentmultiple(relatedpressure, 0.8595,
                                         relatedtemperature, 3.6178,
                                         1.1521);

    derived63 = 10.63 * exponentmultiple(relatedpressure, 0.9298,
                                         relatedtemperature, 2.3284,
                                         0.6287);

    derived66 = 1.944 * exponentmultiple(relatedpressure, 1.6673,
                                         relatedtemperature, -3.3583,
                                         -4.1612);

    derived66prime = 1.935 * exponentmultiple(relatedpressure, 1.6657,
                                              relatedtemperature, -3.3714,
                                              -4.1643);


    if (Frequency <= 54.0){

        factor1 = 7.34 * pow(relatedpressure, 2.0) *
                         pow(relatedtemperature, 3.0)/
                         (pow(Frequency, 2.0) + 0.36 *
                         pow(relatedpressure, 2.0) *
                         pow(relatedtemperature, 2.0));

        factor2 = 0.3429 * derivedb * derived54prime/
                  (pow((54 - Frequency), deriveda) + derivedb);

        oxygenattenuationcoefficient = (factor1 + factor2) *
                        pow(Frequency, 2.0) * pow(10.0, -3.0);

    }
    else if (Frequency < 66.0 ){

        kasai1 = pow(54.0,-frequencyexponents) * log(derived54) *
                 fourtimesdifference(Frequency, 57.0, 60.0, 63.0, 66.0)
                 /1944.0;

        kasai2 = pow(57.0,-frequencyexponents) * log(derived57) *
                 fourtimesdifference(Frequency, 54.0, 60.0, 63.0, 66.0)
                 /486.0;

        kasai3 = pow(60.0,-frequencyexponents) * log(derived60) *
                 fourtimesdifference(Frequency, 54.0, 57.0, 63.0, 66.0)
                 /324.0;

        kasai4 = pow(63.0,-frequencyexponents) * log(derived63) *
                 fourtimesdifference(Frequency, 54.0, 57.0, 60.0, 66.0)
                 /486.0;

        kasai5 = pow(66.0,-frequencyexponents) * log(derived66) *
                 fourtimesdifference(Frequency, 54.0, 57.0, 60.0, 63.0)
                 /1944.0;

        kasai = kasai1 - kasai2 + kasai3 - kasai4 +kasai5;


        oxygenattenuationcoefficient =
                           exp(kasai * pow(Frequency, frequencyexponents));
    }

    else if (Frequency < 120.0 ){

        factor3 = 0.2296 * derivedd * derived66prime/
                  (pow((Frequency - 66.0), derivedc) + derivedd);

        factor4 = 0.286 * pow(relatedpressure, 2.0) *
                          pow(relatedtemperature, 3.8)/
                          (pow((Frequency - 118.75), 2.0) + 2.97 *
                          pow(relatedpressure, 2.0) *
                          pow(relatedtemperature, 1.6));

        oxygenattenuationcoefficient =
                   (factor3 + factor4) * pow(Frequency, 2.0) *
                   pow(10.0, -3.0);
    }

    else if (Frequency < 350.0){

        factor5 = 3.02 * pow(10.0, -4.0) * pow(relatedpressure, 2.0) *
                         pow(relatedtemperature, 3.5);

        factor6 = 1.5827 * pow(relatedpressure, 2.0) *
                           pow(relatedtemperature, 3.0)/
                           pow((Frequency - 66.0), 2.0);

        factor7 = 0.286 * pow(relatedpressure, 2.0) *
                          pow(relatedtemperature, 3.8)/
                          (pow((Frequency - 118.75), 2.0) + 2.97 *
                          pow(relatedpressure, 2.0)*
                          pow(relatedtemperature, 1.6));

        oxygenattenuationcoefficient = (factor5 + factor6 + factor7)*
                                       pow(Frequency, 2.0) * pow(10.0, -3.0);
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

double WirelessLinkWaterVaporAttenuationCoefficient(
                                    double AtmosphericPressure,
                                    double Temperature,
                                    double watervapordensity,
                                    double Frequency)
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
    double watervaporattenuationcoefficient;

    relatedpressure = AtmosphericPressure/1031.0;
    relatedtemperature = 288.0/(273.0 + Temperature);

    axis1 = 6.9575 * exponentmultiple(relatedpressure, -0.3461,
                                      relatedtemperature, 0.2535,
                                      1.3766)-1;

    axis2 = 42.1309 * exponentmultiple(relatedpressure, -0.3068,
                                       relatedtemperature, 1.2023,
                                       2.5147)-1;

    derivedc = log(axis2/axis1)/log(3.5);

    derivedxila1 = 6.7665 * exponentmultiple(relatedpressure, -0.5050,
                                             relatedtemperature, 0.5106,
                                             1.5663)-1;

    derivedxila2 = 27.8843 * exponentmultiple(relatedpressure, -0.4908,
                                              relatedtemperature, -0.8491,
                                              0.5496)-1;

    deriveda = log(derivedxila2/derivedxila1)/log(3.5);


     weight1 = 0.9544 * relatedpressure * pow(relatedtemperature, 0.69)
               + 0.0061 * watervapordensity;

    weight2 = 0.9500 * relatedpressure * pow(relatedtemperature, 0.64)
              + 0.0067 * watervapordensity;

    weight3 = 0.9561 * relatedpressure * pow(relatedtemperature, 0.67)
              + 0.0059 * watervapordensity;

    weight4 = 0.9543 * relatedpressure * pow(relatedtemperature, 0.68)
              + 0.0061 * watervapordensity;

    weight5 = 0.9550 * relatedpressure * pow(relatedtemperature, 0.68)
              + 0.0060 * watervapordensity;

    g22 = 1.0 + pow((Frequency - 22.235), 2.0)/pow((Frequency + 22.235), 2.0);

    g557 = 1.0 + pow((Frequency-557.0), 2.0)/pow((Frequency+557.0), 2.0);

    g752 = 1.0 + pow((Frequency - 752.0), 2.0)/pow((Frequency + 752.0), 2.0);

    item1 = 3.13 * pow(10.0, -2.0) * relatedpressure *  relatedtemperature +
            1.76 * pow(10.0,-3.0) * watervapordensity *
            pow(relatedtemperature, 8.5);

    item2 = 3.84 * weight1 * g22 * exp(2.23 * (1.0 - relatedtemperature))/
            (pow((Frequency - 22.235), 2.0) + 9.42 * pow(weight1, 2.0));

    item3 = 10.48 * weight2 * exp(0.7 * (1.0 - relatedtemperature))/
            (pow((Frequency - 183.31), 2.0) + 9.48 * pow(weight2, 2.0));

    item4 = 0.078 * weight3 * exp(6.4385 * (1.0 - relatedtemperature))/
            (pow((Frequency-321.226), 2.0) + 6.29 * pow(weight3, 2.0));

    item5 = 3.76 * weight4 * exp( 1.6 * (1.0 - relatedtemperature))/
            (pow((Frequency - 325.153), 2.0) + 9.22 * pow(weight4, 2.0));

    item6 = 26.36 * weight5 * exp(1.09 * (1.0 - relatedtemperature))/
            pow((Frequency-380.0), 2.0);

    item7 = 17.87 * weight5 * exp(1.46 * (1.0 - relatedtemperature))/
            pow((Frequency - 448.0), 2.0);

    item8 = Frequency * relatedtemperature * deriveda * derivedc * 0.8837
            * weight5 * g557 * exp(0.17 * (1.0 - relatedtemperature))/
            pow((Frequency - 557.0), 2.0);

    item9 = 302.6 * weight5 * g752 * exp(0.41*(1-relatedtemperature))/
            pow((Frequency - 752.0), 2.0);

    sumitems = item2 + item3 + item4 + item5 + item6 + item7 + item8 + item9;

    watervaporattenuationcoefficient =
            (item1 + pow(relatedtemperature, 2.5) * sumitems) *
             pow(Frequency, 2.0) * watervapordensity * pow(10.0, -4.0);

    return watervaporattenuationcoefficient;

}

/* to represent Atmospheric Attenuation only humidity and temperature are required
 pressure and density are derived from them */

double WirelessLinkAtmosphericAttenuation(double distance,   // in km
                                          double Frequency,  // in GHz
                                          double humidity,
                                          double temperature)
{

    double SaturationPressure;
    double WaterVaporPressure;
    double WaterVaporDensity;
    double AttenuationCoefficientWaterVapor;
    double AttenuationCoefficientOxygen;
    double atmosphereattenuation;

    //ITU-R Recommendation P.453-7
    SaturationPressure = 6.1121 * exp(17.502 * temperature /
                        ( 240.97 + temperature));

    WaterVaporPressure = (humidity)*SaturationPressure/100.0;

    //ITU-R Recommendation P.453-6
    WaterVaporDensity = 216.7 * WaterVaporPressure / (temperature + 273.3);

    AttenuationCoefficientOxygen =
             WirelessLinkOxygenAttenuationCoefficient(WaterVaporPressure,
                                                      temperature,
                                                      Frequency);

    AttenuationCoefficientWaterVapor =
            WirelessLinkWaterVaporAttenuationCoefficient(WaterVaporPressure,
                                                         temperature,
                                                         WaterVaporDensity,
                                                         Frequency);

    atmosphereattenuation = distance*(AttenuationCoefficientOxygen +
                            AttenuationCoefficientWaterVapor);

    return atmosphereattenuation;

}



double WirelessLinkRainAttenuationCoefficient(double rainintensity,
                                              double kfactor,
                                              double alphafactor)
{

    double RainAttenuationCoefficient;

    RainAttenuationCoefficient = kfactor*pow(rainintensity,alphafactor);

    return RainAttenuationCoefficient;
}

/*
  Calculate the rain attenuation according to ITU-R Recommendation P.530-8,
  herein frequency is in GHz, distance is in km, and rain intensity is in mm/h.
*/

double WirelessLinkRainAttenuation(double Frequency,   //in GHz
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

    assert(Frequency >= 1.0);

    for (i = 0;i < 26;i++){
        if (Frequency > rainattenuationparameters[i][0]){
           frequencyindex = i;

        }
    }

    if (polarization == HORIZONTAL){

        rainattenuationcoefficient_k =
            ((rainattenuationparameters[frequencyindex+1][1] -
            rainattenuationparameters[frequencyindex][1])/
            (rainattenuationparameters[frequencyindex+1][0]-
            rainattenuationparameters[frequencyindex][0]))*
            (Frequency - rainattenuationparameters[frequencyindex][0])
            + rainattenuationparameters[frequencyindex][1];

        rainattenuationcoefficient_alpha =
            ((rainattenuationparameters[frequencyindex+1][3] -
            rainattenuationparameters[frequencyindex][3])/
            (rainattenuationparameters[frequencyindex+1][0]-
            rainattenuationparameters[frequencyindex][0]))*
            (Frequency - rainattenuationparameters[frequencyindex][0])
            + rainattenuationparameters[frequencyindex][3];
    }
    else if (polarization == VERTICAL){

        rainattenuationcoefficient_k =
            ((rainattenuationparameters[frequencyindex+1][2] -
            rainattenuationparameters[frequencyindex][2])/
            (rainattenuationparameters[frequencyindex+1][0]-
            rainattenuationparameters[frequencyindex][0]))*
            (Frequency - rainattenuationparameters[frequencyindex][0])
            + rainattenuationparameters[frequencyindex][2];

        rainattenuationcoefficient_alpha =
            ((rainattenuationparameters[frequencyindex+1][4] -
            rainattenuationparameters[frequencyindex][4])/
            (rainattenuationparameters[frequencyindex+1][0]-
            rainattenuationparameters[frequencyindex][0]))*
            (Frequency - rainattenuationparameters[frequencyindex][0])
            + rainattenuationparameters[frequencyindex][4];

    }

    rainattenuationperunitlength =
        WirelessLinkRainAttenuationCoefficient(
                                    rainintensity,
                                    rainattenuationcoefficient_k,
                                    rainattenuationcoefficient_alpha);

    if (rainintensity <= 100){

        referencedistance = 35.0*exp(-0.015*rainintensity);
    }else{

        referencedistance = 100.0;
    }

    effectiveoathlength = distance/(1.0 + distance/referencedistance);

    rainattenuation = rainattenuationperunitlength*effectiveoathlength;

    return rainattenuation;
}


/*
  Calculate the diffraction attenuation over spherical earth according to
  ITU-R Recommendation P.526-6, herein distance, Txantennaheight and
  Rxantennaheight are all in meter, frequency is in Hz, and dielectric
  permittivity should be 15 for average ground and 81 for sea water.
*/

double WirelessDiffractionAttenuationSphericalEarth(
                                            double electricconductivity,
                                            double dielectricpermittivity,
                                            double distance,
                                            double Txantennaheight,
                                            double Rxantennaheight,
                                            double frequency,
                                            double propSpeed,
                                            int polarization)
{

    double relativedielectricpermittivity;
    double generalizedsurfaceadmitanceH;
    double generalizedsurfaceadmitance = 0.0;
    double relativephase;
    double normalizedpathlength;
    double normalizedTxantennaheight;
    double normalizedRxantennaheight;
    double beltaparameter;
    double wavelength;
    double item2;
    double item3;
    double item4;
    double itemf;
    double itemg1;
    double itemg2;
    double diffractionloss;

    double vacuumpermittivity;
    double effectiveearthradius = Constants_RadiusEarth;

    vacuumpermittivity = DEFAULT_DIELECTRIC_PERMITTIVITY_VACUUM;

    relativedielectricpermittivity =
                    dielectricpermittivity/vacuumpermittivity;
    // it should be 15 for average ground and 81 for sea water

    wavelength = propSpeed/frequency;

    relativephase = 2.0 * PI * effectiveearthradius/wavelength;

    item2 = pow(relativedielectricpermittivity-1, 2.0) +
            pow(60.0*wavelength*electricconductivity, 2.0);

    generalizedsurfaceadmitanceH =
                  pow(relativephase, -1.0/3.0)*
                  pow(item2, -1.0/4.0);

    if (polarization == HORIZONTAL){

        generalizedsurfaceadmitance = generalizedsurfaceadmitanceH;
    }
    else if (polarization == VERTICAL){

        generalizedsurfaceadmitance =
                   generalizedsurfaceadmitanceH *
                   sqrt(pow(relativedielectricpermittivity, 2.0)
                   + pow(60.0*wavelength*electricconductivity, 2.0));

    }

    beltaparameter = (1.0 + 1.6 * pow(generalizedsurfaceadmitance, 2.0)+
                      0.75 * pow(generalizedsurfaceadmitance, 4.0))/
                      (1.0 + 4.5 * pow(generalizedsurfaceadmitance, 2.0) +
                      1.35 * pow(generalizedsurfaceadmitance, 4.0));


    item3 = PI /(wavelength * pow(effectiveearthradius, 2.0));

    normalizedpathlength = beltaparameter * distance * pow(item3, 1.0/3.0);

    item4 = pow(PI, 2.0)/(pow(wavelength, 2.0) * effectiveearthradius);
    normalizedTxantennaheight = 2.0 * beltaparameter * Txantennaheight *
                                pow(item4, 1.0/3.0);

    normalizedRxantennaheight = 2.0 * beltaparameter * Rxantennaheight *
                                pow(item4, 1.0/3.0);

    itemf = 11.0 + 10.0 * log10(normalizedpathlength) - 17.6 *
            normalizedpathlength;


    if (normalizedTxantennaheight >= 2.0){

        itemg1 = 17.6 * sqrt(normalizedTxantennaheight - 1.1) -
                 5.0 * log10(normalizedTxantennaheight - 1.1) - 8.0;
    }
    else if (normalizedTxantennaheight > 10.0*generalizedsurfaceadmitance){

        itemg1 = 20.0 * log10(normalizedTxantennaheight +
                 0.1 * pow(normalizedTxantennaheight, 3.0));
    }
    else if (normalizedTxantennaheight > generalizedsurfaceadmitance/10.0){

        itemg1 = 2.0 + 20.0 * log10(generalizedsurfaceadmitance) + 9.0 *
               log10(normalizedTxantennaheight/generalizedsurfaceadmitance) *
               (log10(normalizedTxantennaheight/generalizedsurfaceadmitance) +
               1.0);
    }
    else {

        itemg1 = 2.0 +20.0*log10(generalizedsurfaceadmitance);
    }

    if (normalizedRxantennaheight >= 2.0){

        itemg2 = 17.6 * sqrt(normalizedRxantennaheight - 1.1) - 5.0 *
                 log10(normalizedRxantennaheight - 1.1) - 8.0;
    }
    else if (normalizedRxantennaheight > 10.0*generalizedsurfaceadmitance){

        itemg2 = 20.0 * log10(normalizedRxantennaheight + 0.1 *
                 pow(normalizedRxantennaheight, 3.0));
    }
    else if (normalizedRxantennaheight > generalizedsurfaceadmitance/10.0){

        itemg2 = 2.0 + 20.0 * log10(generalizedsurfaceadmitance) + 9.0 *
             log10(normalizedRxantennaheight/generalizedsurfaceadmitance) *
             (log10(normalizedRxantennaheight/generalizedsurfaceadmitance) +
             1.0);
    }
    else{

        itemg2 = 2.0 + 20.0 * log10(generalizedsurfaceadmitance);
    }

    diffractionloss = -(itemf +itemg1+itemg2);

    return diffractionloss;

}

//calculate the fading probability according to ITU-R Recommendation P.530-8

double WirelessLinkFadingProbability(double distance,  //in km
                         double frequency,         //in GHz
                         double Txantennaheight,   //in meters
                         double Rxantennaheight,   // in meters
                         double receivedpower,     // in nondB
                         double receiversensitivity,  //in nondB
                         char   terraintype,
                         double terrainaltitude,   // in meters
                         double probabilityexceedrefractivitygradient,
                         double latitudes,
                         double longitudes)
{

    double geoclimaticfactor;
    double constantone = 0.0;
    double constantlat;
    double constantlong;
    double constantinter;
    double pathslope;
    double fadingprobability;
    double probabilityfading;
    double multipathoccurrencefactor;
    double fadingdepth;
    double fadingmargin;

    if (terraintype == PLAINS)
    {
        if (terrainaltitude < 400.0)

            constantone = 0.0;

        else if (terrainaltitude< 700.0)

            constantone = 2.5;
        else

            constantone = 5.5;
    }
    else if (terraintype == HILLS)
    {
        if (terrainaltitude < 400.0)

            constantone = 3.5;

        else if (terrainaltitude < 700.0)

            constantone = 6.0;

        else
            constantone = 8.0;
    }
    else if (terraintype == MOUNTAINS)
    {

        constantone = 10.5;
    }


    if (latitudes > -53.0 && latitudes < 53.0)
    {
        constantlat = 0.0;
    }
    else if (latitudes > -60.0 && latitudes < 60.0){

        constantlat = -53.0 + fabs(latitudes);

    }else{

        constantlat = 7.0;
    }


    if ((longitudes > -30.0)  &&  (longitudes < 30.0) ){

        constantlong = 3.0;
    }//??  "longitudes Europe and Africa"
    else if ((longitudes > -130.0) && (longitudes < -60.0 )){

        constantlong = -3.0;
    }//??"longitudes America north central south"
    else{

        constantlong = 0.0;
    }

    constantinter = 0.1 * (constantone - constantlat - constantlong);

    geoclimaticfactor = 5.0 * pow(10.0, -7.0) * pow(10.0, -constantinter)*
                        pow(probabilityexceedrefractivitygradient, 1.5);

    pathslope = fabs(Rxantennaheight - Txantennaheight)/distance;

    multipathoccurrencefactor = geoclimaticfactor * pow(distance, 3.6)*
                   pow(frequency, 0.89) * pow(1.0 + pathslope, -1.4) *
                   pow(10.0, -2.0);

    fadingprobability = multipathoccurrencefactor*
                        (receiversensitivity/receivedpower);

    fadingdepth = 27.4 + 1.2 * log10(receiversensitivity);

    fadingmargin = 10.0 * log10(receivedpower/receiversensitivity);

    if (fadingmargin >= fadingdepth) {

        probabilityfading = fadingprobability;

    } else {

        probabilityfading = calculatefadingprobability(
                                  multipathoccurrencefactor,
                                  fadingdepth,
                                  fadingmargin);

    }

    return probabilityfading;

}


double calculatefadingprobability( double multipathfactor,
                                   double depthfading,  // in dB
                                   double marginfading)  // in dB

{

    double depthfadingprobability;
    double fadingprobability;
    double coefficientone;
    double coefficienttwo;
    double coefficientthree;
    double middleone;
    double middletwo;
    double middlethree;
    double middlefour;

    depthfadingprobability = multipathfactor * pow(10.0, -depthfading/10.0);

    coefficientone = (-20.0 * log10(-log(1-depthfadingprobability)))
                     /depthfading;

    middleone = (1.0 + 0.3 * pow(10.0, -depthfading/20.0)) *
                pow(10.0, -0.016*depthfading);

    middletwo = 4.3 * (pow(10.0, -depthfading/20.0) + depthfading/800.0);

    middlethree = (1.0 + 0.3 * pow(10.0, -marginfading/20.0)) *
                  pow(10.0, -0.016*marginfading);

    middlefour = 4.3 * (pow(10.0, -marginfading/20.0) + marginfading/800.0);

    coefficienttwo = (coefficientone - 2.0)/middleone - middletwo;

    coefficientthree = 2.0 + middlethree * (coefficienttwo + middlefour);

    fadingprobability =
             1.0 - exp(-pow(10.0,-coefficientthree*marginfading/20.0));

    return fadingprobability;

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
                                          Message* msg)
{
    Coordinates nodePosition;
    Coordinates sourcePosition;
    double distance = 0;
    PartitionData* partition = node->partitionData;
    WirelessLinkParameters* linkvar =
                       (WirelessLinkParameters*)link->linkVar;
    WirelessLinkInfo* linkInfo = (WirelessLinkInfo*)MESSAGE_ReturnInfo(msg);
    ERROR_AssertArgs(partition->terrainData->getCoordinateSystem() ==
        LATLONALT, "Coordinate system needs to be LATLONALT\n");

    MOBILITY_ReturnCoordinates(node, &nodePosition);

    linkvar->longitudes = nodePosition.latlonalt.longitude;
    linkvar->latitudes = nodePosition.latlonalt.latitude;
    linkvar->terrainaltitude = nodePosition.latlonalt.altitude;

    WirelessLinkSiteParameters* sourcesiteparameters =
                            &(linkInfo->sourcesiteparameters);
    WirelessLinkSiteParameters* destsiteparameters = &(linkvar->Rx);

    sourcePosition = linkInfo->sourcecoordinates;
    COORD_CalcDistance(partition->terrainData->getCoordinateSystem(),
                       &sourcePosition,
                       &nodePosition,
                       &distance);

    ERROR_AssertArgs(distance > 0.0, "Distance must be greater than zero");
    linkInfo->distance = distance;
    double maxdistance = WirelessLinkMaxDistance(
                                sourcesiteparameters->AntennaHeight
                                + sourcePosition.latlonalt.altitude,
                                destsiteparameters->AntennaHeight
                                + linkvar->terrainaltitude);
    if (distance > maxdistance)
    {
        ERROR_ReportWarningArgs("\n Destination node is out of reach."
            " Destination %d, Source %d, Source InterfaceIndex %d, Allowed "
            "Max distance %f, Calculated distance %f\n", node->nodeId,
            link->destNodeId, link->destInterfaceIndex, maxdistance,
            distance);

        return FALSE;
    }
    return TRUE;
}


/*

Calculate pathloss for the link, if terrain data is available ITM
propagation model is used to obtain the propagation loss, otherwise
free space and diffraction over spherical earth are considered, up on
propagation loss, additional attenuation due to rain and atmospheric
are added to obtain the total pathloss.

*/

double WirelessLinkCalculatePathloss(Node* node,
                                     LinkData* link,
                                     Message* msg)
{

    Coordinates nodePosition;
    Coordinates sourcePosition;
    double distance = 0;
    double pathloss_dB;
    double diffractionloss;
    double rainattenuation;
    double atmosphericattenuation;
    double pathlosstotal;

    PartitionData* partition = node->partitionData;
    WirelessLinkParameters* linkvar =
                       (WirelessLinkParameters*)link->linkVar;
    WirelessLinkInfo* linkInfo = (WirelessLinkInfo*)MESSAGE_ReturnInfo(msg);
    MOBILITY_ReturnCoordinates(node, &nodePosition);
    WirelessLinkSiteParameters* sourcesiteparameters =
                            &(linkInfo->sourcesiteparameters);
    WirelessLinkSiteParameters* destsiteparameters = &(linkvar->Rx);

    sourcePosition = linkInfo->sourcecoordinates;
    distance = linkInfo->distance;

    if (partition->terrainData->hasElevationData()) {
        int numSamples;
        double elevationArray[MAX_NUM_ELEVATION_SAMPLES];
        PropPathProfile pathProfile;

        pathProfile.fromPosition = sourcePosition;
        pathProfile.toPosition = nodePosition;
        pathProfile.distance = distance;

        numSamples = TERRAIN_GetElevationArray(
               partition->terrainData,
               &sourcePosition,
               &nodePosition,
               distance,
               (double)linkvar->elevationSamplingDistance,
               elevationArray);

        pathloss_dB = PathlossItm(
               numSamples + 1,
               linkInfo->distance / (double)numSamples,
               elevationArray,
               sourcesiteparameters->AntennaHeight,
               destsiteparameters->AntennaHeight,
               sourcesiteparameters->polarization,
               linkvar->climate,
               linkvar->permittivity,
               linkvar->conductivity,
               sourcesiteparameters->Frequency / 1.0e6,
               linkvar->refractivity);
    }else{

        double wavelength = link->propSpeed / sourcesiteparameters->Frequency;

        pathloss_dB = PathlossFreeSpace(distance, wavelength);

        diffractionloss = WirelessDiffractionAttenuationSphericalEarth(
                                    linkvar->conductivity,
                                    linkvar->permittivity,
                                    linkInfo->distance,
                                    sourcesiteparameters->AntennaHeight,
                                    destsiteparameters->AntennaHeight,
                                    sourcesiteparameters->Frequency,
                                    link->propSpeed,
                                    sourcesiteparameters->polarization);

        pathloss_dB = pathloss_dB + diffractionloss;


    }

    rainattenuation = WirelessLinkRainAttenuation(
                                    sourcesiteparameters->Frequency/1.0e9,
                                    linkInfo->distance/1000.0,
                                    sourcesiteparameters->polarization,
                                    linkvar->rain_intensity);

    atmosphericattenuation = WirelessLinkAtmosphericAttenuation(
                                    linkInfo->distance/1.0e3,
                                    sourcesiteparameters->Frequency/1.0e9,
                                    linkvar->humidity,
                                    linkvar->temperature);

    pathlosstotal = pathloss_dB + rainattenuation + atmosphericattenuation;

    if (DEBUG) {
        printf("dest node  %d  dis %f  loss %f \n",
               node->nodeId,distance,pathlosstotal);
    }

    return pathlosstotal;

}


BOOL WirelessLinkCheckPacketError(Node* node,
                                  LinkData* link,
                                  Message* msg)
{
    double FadingProbability;
    MacData*         thisMac = link->myMacData;
    double rxpower;
    double pathloss = 0;

    WirelessLinkInfo* wli = (WirelessLinkInfo*)MESSAGE_ReturnInfo(msg);
    WirelessLinkParameters *linkvar =
                   (WirelessLinkParameters*)link->linkVar;

    WirelessLinkSiteParameters* sourcesiteparameter =
                                   &(wli->sourcesiteparameters);
    WirelessLinkSiteParameters* destsiteparameter = &(linkvar->Rx);

    double noisepower = BOLTZMANN_CONSTANT * linkvar->noise_temperature
                          * linkvar->noise_factor * (double)thisMac->bandwidth
                          * 1000.0;

    if (WirelessLinkCheckIsDestNodeReachable(node, link, msg))
    {
        pathloss = WirelessLinkCalculatePathloss(node, link, msg);
    }
    else
    {
        return TRUE;
    }
    assert(linkvar->RxSensitivity > IN_DB(noisepower));

    if ((sourcesiteparameter->Frequency == destsiteparameter->Frequency)
        && (sourcesiteparameter->polarization ==
            destsiteparameter->polarization))
        {
            rxpower = sourcesiteparameter->TxPower_dBm
                        + sourcesiteparameter->AntennaGain
                        + destsiteparameter->AntennaGain
                        - sourcesiteparameter->AntennaCableLoss
                        - destsiteparameter->AntennaCableLoss
                        - pathloss;
    }
    else
    {
        rxpower = IN_DB(noisepower);
    }

    FadingProbability = WirelessLinkFadingProbability(
                         wli->distance/1000.0,          //in km
                         sourcesiteparameter->Frequency/1.0e9,   //in GHz
                         sourcesiteparameter->AntennaHeight,     //in meters
                         destsiteparameter->AntennaHeight,     // in meters
                         NON_DB(rxpower),          // in nondB
                         NON_DB(linkvar->RxSensitivity),  //in nondB
                         (char)linkvar->terraintype,
                         linkvar->terrainaltitude,   // in meters
                         linkvar->percentage_refractivity_gradient_less,
                         linkvar->latitudes,
                         linkvar->longitudes);
    if (DEBUG) {
        printf("dest node  %d  RxPower %f  Fading probalility %f \n",
               node->nodeId, rxpower, FadingProbability);
    }

    double rand = RANDOM_erand(linkvar->seed);

    assert((FadingProbability >= 0.0) && (FadingProbability <= 1.0));

    if (FadingProbability > rand) {
        return TRUE;
    }

    return FALSE;

}


void WirelessLinkMessageArrivedFromLink(
    Node *node,
    int interfaceIndex,
    Message* msg)
{

    Mac802Address sourceAddr;
    Mac802Address destAddr;
    BOOL isVlanTag = FALSE;
    MacHeaderVlanTag tagInfo;

    LinkData *link = (LinkData*)node->macData[interfaceIndex]->macVar;

    LinkFrameHeader *header = (LinkFrameHeader *) MESSAGE_ReturnPacket(msg);
#ifdef ADDON_DB
    HandleMacDBEvents(node,
                      msg,
                      node->macData[interfaceIndex]->phyNumber,
                      interfaceIndex,
                      MAC_ReceiveFromPhy,
                      node->macData[interfaceIndex]->macProtocol);
#endif
    WirelessLinkInfo* wli = (WirelessLinkInfo*) MESSAGE_ReturnInfo(msg);

    MAC_CopyMac802Address(&sourceAddr,&header->sourceAddr);
    MAC_CopyMac802Address(&destAddr,&header->destAddr);

    isVlanTag = header->vlanTag;

    link->myMacData->propDelay = wli->propDelay;

    assert(link->phyType == MICROWAVE);

    // receiving end of wireless link can calculate/store propDelay

    BOOL packeterror = WirelessLinkCheckPacketError(node, link, msg);

    if (packeterror == FALSE)
    {
        STAT_DestAddressType type;
        BOOL isMyAddr = FALSE;
        BOOL isAnyFrame = FALSE;

        MacLinkGetPacketProperty(node,
                                 msg,
                                 interfaceIndex,
                                 type,
                                 isMyAddr,
                                 isAnyFrame);

        MESSAGE_RemoveHeader(node, msg, sizeof(LinkFrameHeader), TRACE_LINK);
        Int32 controlSize = sizeof(LinkFrameHeader);
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
            controlSize += sizeof(MacHeaderVlanTag);
        }

        if (isMyAddr || isAnyFrame)
        {
            if (node->macData[interfaceIndex]->macStats)
            {
                node->macData[interfaceIndex]->stats->
                    AddFrameReceivedDataPoints(
                                           node,
                                           msg,
                                           type,
                                           controlSize,
                                           MESSAGE_ReturnPacketSize(msg),
                                           interfaceIndex);
            }

            MacHWAddress srcHWAddr;
            Convert802AddressToVariableHWAddress(node,&srcHWAddr,&sourceAddr);
#ifdef ADDON_DB
            HandleMacDBEvents(
                node,
                msg,
                node->macData[interfaceIndex]->phyNumber,
                interfaceIndex,
                MAC_SendToUpper,
                node->macData[interfaceIndex]->macProtocol);
#endif
            MAC_HandOffSuccessfullyReceivedPacket(
                node, interfaceIndex, msg, &srcHWAddr);

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
            isMyAddr = MAC_IsMyAddress(node, &destHWAddr);
        }
        else
        {
            MacHWAddress* myMacAddr = &node->macData[interfaceIndex]->
                                                                  macHWAddr;
            isMyAddr = MAC_IsIdenticalHWAddress(myMacAddr, &destHWAddr);
        }

        if (!isMyAddr)
        {
            isMyAddr = MAC_IsBroadcastHWAddress(&destHWAddr);
        }

        if (isMyAddr)
        {
            HandleMacDBEvents(node,
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



// ITM related parameters:
//
//                           Climate  Refractivity
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
// Average Ground      15                  0.005
// Poor Ground          4                  0.001
// Good Ground         25                  0.02
// Fresh Water         81                  0.01
// Salt Water          81                  5.0

// Climate: has the value of
//     1 for Equatorial
//     2 for Continental Subtropical
//     3 for Maritime Tropical
//     4 for Desert
//     5 for Continental Temperate
//     6 for Maritime Temperate, Over Land
//     7 for Maritime Temperate, Over Sea


void WirelessLinkInitialize(
    Node *node,
    LinkData *link,
    Address* address,
    const NodeInput *nodeInput)
{

    BOOL    wasFound;
    double  elevationSamplingDistance;
    int     climate;
    double  refractivity;
    double  conductivity;
    double  permittivity;
    double  humidity;
    double  rain_intensity;
    double  percentageoftime;
    double  temperature;
    char    polarization[MAX_STRING_LENGTH];
    double  antennadishdiameter;    //in meter
    double  antennaheight;          //in meter ??
    double  antennacableloss;       // in dB
    double  frequency;
    double  noisetemperature;
    double  noisefactor;
    double  TxPower;
    double  RxSensitivity;
    char    terraintype[MAX_STRING_LENGTH];
    Int32 interfaceIndex = MAPPING_GetInterfaceIndexFromInterfaceAddress(
                             node,
                             *address);

    WirelessLinkParameters *Linkinfo =
         (WirelessLinkParameters *)MEM_malloc(sizeof( WirelessLinkParameters));

    link->linkVar = Linkinfo;

    IO_ReadDouble(
         node,
         node->nodeId,
         interfaceIndex,
         nodeInput,
         "LINK-PROPAGATION-SAMPLING-DISTANCE",
         &wasFound,
         &elevationSamplingDistance);


    if (wasFound) {
        Linkinfo->elevationSamplingDistance = elevationSamplingDistance;
    }
    else {
        Linkinfo->elevationSamplingDistance = DEFAULT_SAMPLING_DISTANCE;
    }


    IO_ReadDouble(
         node,
         node->nodeId,
         interfaceIndex,
         nodeInput,
         "LINK-PROPAGATION-REFRACTIVITY",
         &wasFound,
         &refractivity);


    if (wasFound) {
        Linkinfo->refractivity = refractivity;
    }
    else {
        Linkinfo->refractivity = DEFAULT_REFRACTIVITY;
    }


    IO_ReadDouble(
         node,
         node->nodeId,
         interfaceIndex,
         nodeInput,
         "LINK-PROPAGATION-CONDUCTIVITY",
         &wasFound,
         &conductivity);


    if (wasFound) {
        Linkinfo->conductivity = conductivity;
    }
    else {
        Linkinfo->conductivity = DEFAULT_CONDUCTIVITY;
    }


    IO_ReadDouble(
         node,
         node->nodeId,
         interfaceIndex,
         nodeInput,
         "LINK-PROPAGATION-PERMITTIVITY",
         &wasFound,
         &permittivity);


    if (wasFound) {
        Linkinfo->permittivity = permittivity;
    }
    else {
        Linkinfo->permittivity = DEFAULT_PERMITTIVITY;
    }


    IO_ReadDouble(
         node,
         node->nodeId,
         interfaceIndex,
         nodeInput,
         "LINK-PROPAGATION-HUMIDITY",
         &wasFound,
         &humidity);


    if (wasFound) {
        Linkinfo->humidity = humidity;
    }
    else {
        Linkinfo->humidity = DEFAULT_LINK_HUMIDITY;
    }


    IO_ReadInt(
         node,
         node->nodeId,
         interfaceIndex,
         nodeInput,
         "LINK-PROPAGATION-CLIMATE",
         &wasFound,
         &climate);

    if (wasFound) {
        assert(climate >= 1 && climate <= 7);
        Linkinfo->climate = climate;
    }
    else {
        Linkinfo->climate = DEFAULT_CLIMATE_NUM;
    }


    IO_ReadDouble(
         node,
         node->nodeId,
         interfaceIndex,
         nodeInput,
         "LINK-PROPAGATION-TEMPERATURE",
         &wasFound,
         &temperature);

    if (wasFound) {
        Linkinfo->temperature = temperature;
    }
    else {
        Linkinfo->temperature = DEFAULT_TEMPERATURE;
    }

    // refer to ITU-R Recommendation P.837-1 to have rain intensity value

    IO_ReadDouble(
         node,
         node->nodeId,
         interfaceIndex,
         nodeInput,
         "LINK-PROPAGATION-RAIN-INTENSITY",
         &wasFound,
         &rain_intensity);


    if (wasFound) {
       Linkinfo->rain_intensity = rain_intensity;
    }
    else {
       Linkinfo->rain_intensity = DEFAULT_RAIN_INTENSITY;
    }

    Linkinfo->Tx.propSpeed = link->propSpeed;

    // refer to ITU-R Recommendation P.453-6 to have the following value

    IO_ReadDouble(
         node,
         node->nodeId,
         interfaceIndex,
         nodeInput,
         "LINK-PERCENTAGE-TIME-REFRACTIVITY-GRADIENT-LESS-STANDARD",
         &wasFound,
         &percentageoftime);


    if (wasFound) {
       Linkinfo->percentage_refractivity_gradient_less = percentageoftime;
    }
    else {
       Linkinfo->percentage_refractivity_gradient_less =
                                                  DEFAULT_PERCENTAGE_TIME;
    }


    IO_ReadString(
        node,
        node->nodeId,
        interfaceIndex,
        nodeInput,
        "LINK-TERRAIN-TYPE",
        &wasFound,
        terraintype);

    if (wasFound) {
        if (strcmp(terraintype, "PLAINS") == 0) {
            Linkinfo->terraintype = PLAINS;

        }
        else if (strcmp(terraintype, "HILLS") == 0) {
            Linkinfo->terraintype = HILLS;

        }
        else if (strcmp(terraintype, "MOUNTAINS") == 0) {
            Linkinfo->terraintype = MOUNTAINS;

        }else {
            ERROR_ReportError("Unrecognized terrain type\n");
        }
    }
    else {
        Linkinfo->terraintype = PLAINS;
    }


    //
    // This must be an antenna property..
    //
    IO_ReadString(
        node,
        node->nodeId,
        interfaceIndex,
        nodeInput,
        "LINK-TX-ANTENNA-POLARIZATION",
        &wasFound,
        polarization);

    if (wasFound) {
        if (strcmp(polarization, "HORIZONTAL") == 0) {
            Linkinfo->Tx.polarization = HORIZONTAL;
        }
        else if (strcmp(polarization, "VERTICAL") == 0) {
            Linkinfo->Tx.polarization = VERTICAL;
        }
        else {
            ERROR_ReportError("Unrecognized polarization\n");
        }
    }
    else {
        Linkinfo->Tx.polarization = VERTICAL;
    }


    IO_ReadString(
        node,
        node->nodeId,
        interfaceIndex,
        nodeInput,
        "LINK-RX-ANTENNA-POLARIZATION",
        &wasFound,
        polarization);


    if (wasFound) {
        if (strcmp(polarization, "HORIZONTAL") == 0) {
            Linkinfo->Rx.polarization = HORIZONTAL;
        }
        else if (strcmp(polarization, "VERTICAL") == 0) {
            Linkinfo->Rx.polarization = VERTICAL;
        }
        else {
            ERROR_ReportError("Unrecognized polarization\n");
        }
    }
    else {
        Linkinfo->Rx.polarization = VERTICAL;
    }


   IO_ReadDouble(
         node,
         node->nodeId,
         interfaceIndex,
         nodeInput,
         "LINK-TX-ANTENNA-HEIGHT",
         &wasFound,
         &antennaheight);


    if (wasFound) {
        assert(antennaheight > 0.0);
        Linkinfo->Tx.AntennaHeight = antennaheight;
    }
    else {
        Linkinfo->Tx.AntennaHeight = DEFAULT_ANTENNA_HEIGHT;

    }


    IO_ReadDouble(
         node,
         node->nodeId,
         interfaceIndex,
         nodeInput,
         "LINK-RX-ANTENNA-HEIGHT",
         &wasFound,
         &antennaheight);


    if (wasFound) {
        assert(antennaheight > 0.0);
        Linkinfo->Rx.AntennaHeight = antennaheight;
    }
    else {
        Linkinfo->Rx.AntennaHeight = DEFAULT_ANTENNA_HEIGHT;

    }


    IO_ReadDouble(
         node,
         node->nodeId,
         interfaceIndex,
         nodeInput,
         "LINK-TX-ANTENNA-DISH-DIAMETER",
         &wasFound,
         &antennadishdiameter);


    if (wasFound) {
        assert(antennadishdiameter > 0.01);
        Linkinfo->Tx.AntennaDishDiameter = antennadishdiameter;
    }
    else {
        Linkinfo->Tx.AntennaDishDiameter =
                                  DEFAULT_ANTENNA_DISH_DIAMETER;

    }


    IO_ReadDouble(
         node,
         node->nodeId,
         interfaceIndex,
         nodeInput,
         "LINK-RX-ANTENNA-DISH-DIAMETER",
         &wasFound,
         &antennadishdiameter);


    if (wasFound) {
        assert(antennadishdiameter > 0.01);
        Linkinfo->Rx.AntennaDishDiameter = antennadishdiameter;
    }
    else {
        Linkinfo->Rx.AntennaDishDiameter =
                                 DEFAULT_ANTENNA_DISH_DIAMETER;;

    }


    IO_ReadDouble(
         node,
         node->nodeId,
         interfaceIndex,
         nodeInput,
         "LINK-TX-ANTENNA-CABLE-LOSS",
         &wasFound,
         &antennacableloss);


    if (wasFound) {
        assert(antennacableloss >= 0.0);
        Linkinfo->Tx.AntennaCableLoss = antennacableloss;
    }
    else {
        Linkinfo->Tx.AntennaCableLoss =
                                  DEFAULT_ANTENNA_CABLE_LOSS;

    }


    IO_ReadDouble(
         node,
         node->nodeId,
         interfaceIndex,
         nodeInput,
         "LINK-RX-ANTENNA-CABLE-LOSS",
         &wasFound,
         &antennacableloss);


    if (wasFound) {
        assert(antennacableloss >= 0.0);
        Linkinfo->Rx.AntennaCableLoss = antennacableloss;
    }
    else {
        Linkinfo->Rx.AntennaCableLoss =
                                 DEFAULT_ANTENNA_CABLE_LOSS;

    }


    IO_ReadDouble(
         node,
         node->nodeId,
         interfaceIndex,
         nodeInput,
         "LINK-TX-FREQUENCY",
         &wasFound,
         &frequency);

    if (wasFound) {
        assert(frequency > 0.0);

        Linkinfo->Tx.Frequency = frequency;
    }
    else {
       ERROR_ReportError("Please specify transmitter frequency for the link\n");
    }


    IO_ReadDouble(
         node,
         node->nodeId,
         interfaceIndex,
         nodeInput,
         "LINK-RX-FREQUENCY",
         &wasFound,
         &frequency);

    if (wasFound) {
        assert(frequency > 0.0);
        Linkinfo->Rx.Frequency = frequency;
    }
    else {
        ERROR_ReportError("Please specify receiver frequency for the link\n");
    }


    IO_ReadDouble(
         node,
         node->nodeId,
         interfaceIndex,
         nodeInput,
         "LINK-TX-POWER",
         &wasFound,
         &TxPower);

    if (wasFound) {
        assert(TxPower >= 0.0);
        Linkinfo->Tx.TxPower_dBm = TxPower;
    }
    else {

        Linkinfo->Tx.TxPower_dBm = DEFAULT_TX_POWER;
    }


    IO_ReadDouble(
         node,
         node->nodeId,
         interfaceIndex,
         nodeInput,
         "LINK-RX-SENSITIVITY",
         &wasFound,
         &RxSensitivity);

    if (wasFound) {
        assert(RxSensitivity < 0.0);
        Linkinfo->RxSensitivity = RxSensitivity;
    }
    else {
        Linkinfo->RxSensitivity = DEFAULT_RX_SENSITIVITY;

    }


    IO_ReadDouble(
         node,
         node->nodeId,
         interfaceIndex,
         nodeInput,
         "LINK-NOISE-TEMPERATURE",
         &wasFound,
         &noisetemperature);

    if (wasFound) {

        Linkinfo->noise_temperature = noisetemperature;
    }
    else {
        Linkinfo->noise_temperature = DEFAULT_NOISE_TEMPERATURE;

    }


    IO_ReadDouble(
         node,
         node->nodeId,
         interfaceIndex,
         nodeInput,
         "LINK-NOISE-FACTOR",
         &wasFound,
         &noisefactor);

    if (wasFound) {

        Linkinfo->noise_factor = noisefactor;
    }
    else {
        Linkinfo->noise_factor = DEFAULT_NOISE_FACTOR;

    }

    Linkinfo->Tx.AntennaGain = WirelessLinkAntennaParabolicMaxGain(
                                        Linkinfo->Tx.AntennaDishDiameter,
                                        Linkinfo->Tx.Frequency,
                                        link->propSpeed);

    Linkinfo->Rx.AntennaGain = WirelessLinkAntennaParabolicMaxGain(
                                        Linkinfo->Rx.AntennaDishDiameter,
                                        Linkinfo->Rx.Frequency,
                                        link->propSpeed);

    RANDOM_SetSeed(Linkinfo->seed,
                   node->globalSeed,
                   node->nodeId,
                   MICROWAVE);

    return;
}

