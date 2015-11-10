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

#include <math.h>
#include <iostream>

#include "prop_urban.h"

#ifdef OOS_INTERFACE
#include "oos_interface.h"
#endif /* OOS_INTERFACE */

#define DEBUG 0


// field strength for 100 MHz
static double fieldStrength_100MHz[36][6] = {
 89.975, 92.181, 94.635, 97.384, 100.318, 103.120,
 80.275, 83.090, 86.001, 89.207, 92.674, 96.119,
 74.166, 77.529, 80.823, 84.350, 88.142, 91.968,
 69.518, 73.354, 77.014, 80.831, 84.885, 88.993,
 65.699, 69.920, 73.924, 78.021, 82.313, 86.660,
 62.435, 66.957, 71.272, 75.640, 80.163, 84.727,
 59.580, 64.332, 68.916, 73.542, 78.291, 83.063,
 57.041, 61.967, 66.778, 71.642, 76.612, 81.589,
 54.755, 59.813, 64.812, 69.890, 75.073, 80.252,
 52.679, 57.837, 62.989, 68.254, 73.638, 79.017,
 50.778, 56.012, 61.288, 66.715, 72.284, 77.859,
 49.025, 54.318, 59.694, 65.259, 70.995, 76.759,
 47.400, 52.738, 58.195, 63.876, 69.762, 75.706,
 45.887, 51.259, 56.781, 62.559, 68.577, 74.689,
 44.471, 49.870, 55.444, 61.303, 67.436, 73.703,
 43.142, 48.561, 54.177, 60.103, 66.335, 72.743,
 41.890, 47.324, 52.974, 58.955, 65.272, 71.807,
 40.707, 46.152, 51.829, 57.855, 64.245, 70.894,
 39.587, 45.039, 50.737, 56.801, 63.251, 70.001,
 38.523, 43.980, 49.695, 55.788, 62.290, 69.128,
 33.906, 39.353, 45.096, 51.267, 57.923, 65.057,
 30.181, 35.575, 41.290, 47.459, 54.161, 61.436,
 27.102, 32.409, 38.052, 44.171, 50.859, 58.194,
 24.517, 29.703, 35.239, 41.267, 47.901, 55.251,
 22.324, 27.356, 32.748, 38.652, 45.198, 52.532,
 20.445, 25.291, 30.508, 36.256, 42.685, 49.978,
 18.824, 23.455, 28.467, 34.030, 40.314, 47.543,
 17.413, 21.807, 26.590, 31.942, 38.055, 45.196,
 16.177, 20.316, 24.851, 29.971, 35.890, 42.919,
 15.084, 18.957, 23.231, 28.106, 33.812, 40.704,
 14.110, 17.712, 21.721, 26.340, 31.819, 38.550,
 13.233, 16.567, 20.309, 24.670, 29.912, 36.461,
12.436, 15.508, 18.990, 23.094, 28.095, 34.443,
11.704, 14.526, 17.756, 21.612, 26.370, 32.504,
11.024, 13.611, 16.603, 20.219, 24.738, 30.649,
10.388, 12.755, 15.524, 18.912, 23.201, 28.883};


// field strength for 600 MHz
static double fieldStrength_600MHz[36][6] = {
92.681, 94.868, 97.072, 99.699, 102.345, 104.591,
81.108, 84.291, 87.092, 90.356, 93.803, 97.071,
73.480, 77.690, 81.046, 84.741, 88.624, 92.462,
67.693, 72.675, 76.575, 80.667, 84.877, 89.107,
63.064, 68.556, 72.942, 77.421, 81.920, 86.457,
59.229, 65.047, 69.834, 74.687, 79.459, 84.256,
55.965, 61.992, 67.096, 72.296, 77.333, 82.365,
53.130, 59.293, 64.640, 70.152, 75.447, 80.700,
50.628, 56.879, 62.410, 68.195, 73.739, 79.204,
48.393, 54.701, 60.370, 66.387, 72.167, 77.839,
46.377, 52.719, 58.489, 64.702, 70.703, 76.576,
44.542, 50.904, 56.748, 63.122, 69.327, 75.396,
42.862, 49.230, 55.127, 61.633, 68.022, 74.282,
41.315, 47.680, 53.613, 60.224, 66.780, 73.223,
39.883, 46.238, 52.192, 58.888, 65.590, 72.209,
38.553, 44.890, 50.856, 57.617, 64.447, 71.233,
37.312, 43.626, 49.594, 56.404, 63.345, 70.289,
36.151, 42.437, 48.399, 55.244, 62.280, 69.373,
35.062, 41.315, 47.265, 54.133, 61.250, 68.480,
34.038, 40.254, 46.185, 53.066, 60.250, 67.607,
29.704, 35.679, 41.448, 48.276, 55.634, 63.479,
26.339, 31.999, 37.521, 44.162, 51.501, 59.617,
23.638, 28.930, 34.148, 40.517, 47.713, 55.935,
21.411, 26.304, 31.182, 37.224, 44.194, 52.395,
19.531, 24.013, 28.535, 34.219, 40.906, 48.992,
17.910, 21.986, 26.151, 31.464, 37.834, 45.734,
16.485, 20.173, 23.991, 28.936, 34.972, 42.632,
15.211, 18.536, 22.027, 26.616, 32.314, 39.698,
14.051, 17.044, 20.233, 24.486, 29.852, 36.938,
12.982, 15.675, 18.588, 22.530, 27.578, 34.354,
11.982, 14.407, 17.071, 20.730, 25.477, 31.941,
11.037, 13.223, 15.666, 19.068, 23.536, 29.694,
10.136, 12.111, 14.357, 17.527, 21.739, 27.602,
9.269, 11.059, 13.129, 16.093, 20.070, 25.654,
8.429, 10.056, 11.972, 14.751, 18.515, 23.837,
7.612, 9.095, 10.874, 13.489, 17.061, 22.138};


// field strength for 2000 MHz
static double fieldStrength_2000MHz[36][6] = {
94.233, 96.509, 98.662, 101.148, 103.509, 105.319,
82.427, 85.910, 88.758, 91.971, 95.244, 98.116,
74.501, 79.135, 82.671, 86.395, 90.171, 93.677,
68.368, 73.847, 78.078, 82.308, 86.474, 90.429,
63.385, 69.412, 74.253, 79.006, 83.536, 87.851,
59.209, 65.580, 70.908, 76.172, 81.068, 85.701,
55.628, 62.216, 67.909, 73.643, 78.912, 83.845,
52.499, 59.227, 65.186, 71.329, 76.970, 82.198,
49.725, 56.544, 62.696, 69.181, 75.181, 80.707,
47.236, 54.116, 60.406, 67.169, 73.507, 79.331,
44.981, 51.901, 58.291, 65.277, 71.922, 78.043,
42.922, 49.867, 56.329, 63.490, 70.408, 76.822,
41.029, 47.989, 54.502, 61.800, 68.956, 75.653,
39.279, 46.245, 52.794, 60.198, 67.558, 74.524,
37.653, 44.619, 51.191, 58.677, 66.210, 73.429,
36.137, 43.096, 49.682, 57.231, 64.909, 72.361,
34.717, 41.665, 48.257, 55.853, 63.652, 71.316,
33.384, 40.316, 46.908, 54.537, 62.437, 70.293,
32.129, 39.041, 45.627, 53.278, 61.260, 69.289,
30.945, 37.832, 44.407, 52.072, 60.121, 68.303,
25.889, 32.596, 39.051, 46.684, 54.906, 63.628,
21.921, 28.355, 34.599, 42.081, 50.306, 59.317,
18.729, 24.802, 30.756, 37.994, 46.119, 55.280,
16.114, 21.754, 27.352, 34.269, 42.210, 51.427,
13.939, 19.099, 24.292, 30.826, 38.510, 47.703,
12.108, 16.766, 21.526, 27.634, 34.999, 44.085,
10.548, 14.707, 19.028, 24.685, 31.681, 40.581,
9.201, 12.884, 16.779, 21.982, 28.576, 37.214,
8.025, 11.268, 14.761, 19.525, 25.699, 34.012,
6.984, 9.831, 12.957, 17.305, 23.060, 31.001,
6.050, 8.546, 11.343, 15.308, 20.658, 28.196,
5.200, 7.390, 9.896, 13.516, 18.482, 25.607,
4.416, 6.342, 8.594, 11.905, 16.516, 23.230,
3.683, 5.381, 7.413, 10.452, 14.740, 21.057,
2.990, 4.493, 6.334, 9.135, 13.131, 19.074,
2.327, 3.662, 5.339, 7.933, 11.669, 17.262};


// distance vector
static double distanceV[36] = {
 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0,
 10.0, 11.0, 12.0, 12.0, 14.0, 15.0, 16.0, 17.0, 18.0,
 19.0, 20.0, 25.0, 30.0, 35.0, 40.0, 45.0, 50.0, 55.0,
 60.0, 65.0, 70.0, 75.0, 80.0, 85.0, 90.0, 95.0, 100.0};


// height vector
static double  heightsV[6] = {10.0, 20.0, 37.5, 75.0, 150.0, 300.0};


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


// complex permittivity for concrete
static double concrete[3][3] =
    { 1.0, 7.0, -0.85,
      57.5, 6.5,  -0.43,
      95.9, 6.2,  -0.34};

// complex permittivity for synthetic resin
static double syntheticResin[3][3] =
    { 57.5, 3.91, -0.33,
      78.5, 3.64, -0.37,
      95.9, 3.16, -0.39};

// complex permittivity for plasterboard
static double plasterboard[4][3] =
    { 57.5, 2.25, -0.03,
      70.0, 2.43, -0.04,
      78.5, 2.37, -0.10,
      95.9, 2.25, -0.06};

// complex permittivity for rock wool
static double rockWool[4][3] =
    { 1.0, 1.2, -0.01,
      57.5, 1.59, -0.01,
      78.5, 1.56, -0.02,
      95.9, 1.56, -0.04};

// complex permittivity for metal
static double metal[3][3] =
    { 1.0, 7.0, -100.85,
      50.5, 5.5,  -100.43,
      99.9, 3.2,  -100.34};

// loss coefficients
static double lossCoefficients[9][3] =
{0.9, 33.0, 20.0,
 1.2, 32.0, 22.0,
 1.3, 32.0, 22.0,
 1.8, 30.0, 22.0,
 2.0, 30.0, 22.0,
 4.0, 28.0, 22.0,
 5.2, 31.0, 22.0,
 60.0, 22.0, 17.0,
 70.0, 22.0, 17.0};


static
double fourtimesdifference(double parameter0,
                           double parameter1,
                           double parameter2,
                           double parameter3,
                           double parameter4)
{
    double fourtimes;

    fourtimes =
            (parameter0 - parameter1) *
            (parameter0 - parameter2) *
            (parameter0 - parameter3) *
            (parameter0 - parameter4);

    return fourtimes;

}


static
double exponentmultiple(double base1,
                        double exponent1,
                        double base2,
                        double exponent2,
                        double coefficient)
{

    double multipleexponent;

    multipleexponent =
            pow(base1, exponent1) *
            pow(base2, exponent2) *
            exp(coefficient * (1.0 - base2));

    return multipleexponent;

}


static
double Prop_RainAttenuationCoefficient(double rainintensity,
                                       double kfactor,
                                       double alphafactor)
{

    double rainAttenuationCoefficient;

    rainAttenuationCoefficient =
            kfactor * pow(rainintensity, alphafactor);

    return rainAttenuationCoefficient;
}


/*
 Given the frequency in GHz, the atmospheric pressure in hPa, and the
 temperature in Centigrade, represents the oxygen attenuation coefficient
 in dB/km according to ITU-R Recommendation P.676-4.
*/
static
double Prop_OxygenAttenuationCoefficient(double atmosphericPressure,
                                         double temperature,
                                         double frequency)
{
    double relatedpressure;
    double relatedtemperature;
    double frequencyexponents = 0.0;
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

    relatedpressure = atmosphericPressure / 1013.0;
    relatedtemperature = 288.0 / (273.0 + temperature);

    if (frequency > 60.0)
    {
        frequencyexponents = -15.0;
    }

    axis1 =
        6.9575 * exponentmultiple(relatedpressure,
                                  -0.3461,
                                  relatedtemperature,
                                  0.2535,
                                  1.3766) - 1.0;

    axis2 =
        42.1309 * exponentmultiple(relatedpressure,
                                   -0.3068,
                                   relatedtemperature,
                                   1.2023,
                                   2.5147) - 1.0;

    derivedc = log(axis2 / axis1) / log(3.5);

    derivedd = pow(4.0, derivedc) / axis1;

    derivedxila1 =
        6.7665 * exponentmultiple(relatedpressure,
                                  -0.5050,
                                  relatedtemperature,
                                  0.5106,
                                  1.5663) - 1.0;

    derivedxila2 =
        27.8843 * exponentmultiple(relatedpressure,
                                   -0.4908,
                                   relatedtemperature,
                                   -0.8491,
                                   0.5496) - 1.0;

    deriveda = log(derivedxila2 / derivedxila1) / log(3.5);

    derivedb = pow(4.0, deriveda) / derivedxila1;

    derived54prime =
        2.128 * exponentmultiple(relatedpressure,
                                 1.4954,
                                 relatedtemperature,
                                 -1.6032,
                                 -2.5280);

    derived54 =
        2.136 * exponentmultiple(relatedpressure,
                                 1.4975,
                                 relatedtemperature,
                                 -1.5852,
                                 -2.5196);

    derived57 =
        9.984 * exponentmultiple(relatedpressure,
                                 0.9313,
                                 relatedtemperature,
                                 2.6732,
                                 0.8563);

    derived60 =
        15.42 * exponentmultiple(relatedpressure,
                                 0.8595,
                                 relatedtemperature,
                                 3.6178,
                                 1.1521);

    derived63 =
        10.63 * exponentmultiple(relatedpressure,
                                 0.9298,
                                 relatedtemperature,
                                 2.3284,
                                 0.6287);

    derived66 =
        1.944 * exponentmultiple(relatedpressure,
                                 1.6673,
                                 relatedtemperature,
                                 -3.3583,
                                 -4.1612);

    derived66prime =
        1.935 * exponentmultiple(relatedpressure,
                                 1.6657,
                                 relatedtemperature,
                                 -3.3714,
                                 -4.1643);

    double squarepressure = pow(relatedpressure, 2.0);

    if (frequency <= 54.0)
    {

        factor1 =
                7.34 * squarepressure * pow(relatedtemperature, 3.0) /
                (pow(frequency, 2.0) + 0.36 * squarepressure *
                pow(relatedtemperature, 2.0));

        factor2 =
                0.3429 * derivedb * derived54prime /
                (pow((54 - frequency), deriveda) + derivedb);

        oxygenattenuationcoefficient = (factor1 + factor2) *
                        pow(frequency, 2.0) * pow(10.0, -3.0);
    }
    else if (frequency < 66.0 )
    {

        kasai1 =
            pow(54.0, -frequencyexponents) * log(derived54) *
            fourtimesdifference(frequency, 57.0, 60.0, 63.0, 66.0)
            / 1944.0;

        kasai2 =
            pow(57.0, -frequencyexponents) * log(derived57) *
            fourtimesdifference(frequency, 54.0, 60.0, 63.0, 66.0)
            / 486.0;

        kasai3 =
            pow(60.0, -frequencyexponents) * log(derived60) *
            fourtimesdifference(frequency, 54.0, 57.0, 63.0, 66.0)
            / 324.0;

        kasai4 =
            pow(63.0, -frequencyexponents) * log(derived63) *
            fourtimesdifference(frequency, 54.0, 57.0, 60.0, 66.0)
            / 486.0;

        kasai5 =
            pow(66.0, -frequencyexponents) * log(derived66) *
            fourtimesdifference(frequency, 54.0, 57.0, 60.0, 63.0)
            / 1944.0;

        kasai = kasai1 - kasai2 + kasai3 - kasai4 +kasai5;


        oxygenattenuationcoefficient =
                   exp(kasai * pow(frequency, frequencyexponents));
    }
    else if (frequency < 120.0 ){

        factor3 = 0.2296 * derivedd * derived66prime /
                  (pow((frequency - 66.0), derivedc) + derivedd);

        factor4 = 0.286 * squarepressure *
                  pow(relatedtemperature, 3.8)/
                  (pow((frequency - 118.75), 2.0) +
                  2.97 * squarepressure *
                  pow(relatedtemperature, 1.6));

        oxygenattenuationcoefficient =
                  (factor3 + factor4) * pow(frequency, 2.0) *
                  pow(10.0, -3.0);
    }
    else if (frequency < 350.0)
    {

        factor5 =
            3.02 * pow(10.0, -4.0) * squarepressure *
            pow(relatedtemperature, 3.5);

        factor6 =
            1.5827 * squarepressure * pow(relatedtemperature, 3.0)/
            pow((frequency - 66.0), 2.0);

        factor7 =
            0.286 * squarepressure * pow(relatedtemperature, 3.8)/
            (pow((frequency - 118.75), 2.0) + 2.97 *
            squarepressure * pow(relatedtemperature, 1.6));

        oxygenattenuationcoefficient =
            (factor5 + factor6 + factor7) *
            pow(frequency, 2.0) * pow(10.0, -3.0);
    }
    else
    {
        char errorStr[MAX_STRING_LENGTH];

        sprintf(errorStr, "Frequency = %f: Not in "
                "recommended range [1:350] GHz\n", frequency);

        ERROR_ReportWarning(errorStr);
    }

    return oxygenattenuationcoefficient;

}


/*
 Given the frequency in GHz, the atmospheric pressure in hPa, the
 temperature in Centigrade, and the water vapor content in g/m3,
 represents the water vapor attenuation coefficient in dB/km
 according to ITU-R Recommendation P.676-4.
*/
static
double Prop_WaterVaporAttenuationCoefficient(
                                    double atmosphericPressure,
                                    double temperature,
                                    double watervapordensity,
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
    double watervaporattenuationcoefficient;

    relatedpressure = atmosphericPressure / 1013.0;
    relatedtemperature = 288.0 / (273.0 + temperature);

    axis1 =
        6.9575 * exponentmultiple(relatedpressure,
                                  -0.3461,
                                  relatedtemperature,
                                  0.2535,
                                  1.3766) - 1;

    axis2 =
        42.1309 * exponentmultiple(relatedpressure,
                                   -0.3068,
                                   relatedtemperature,
                                   1.2023,
                                   2.5147) - 1;

    derivedc = log(axis2 / axis1) / log(3.5);

    derivedxila1 =
        6.7665 * exponentmultiple(relatedpressure,
                                  -0.5050,
                                  relatedtemperature,
                                  0.5106,
                                  1.5663) - 1;

    derivedxila2 =
        27.8843 * exponentmultiple(relatedpressure,
                                   -0.4908,
                                   relatedtemperature,
                                   -0.8491,
                                   0.5496) - 1;

    deriveda = log(derivedxila2 / derivedxila1) / log(3.5);


     weight1 =
            0.9544 * relatedpressure * pow(relatedtemperature, 0.69)
            + 0.0061 * watervapordensity;

    weight2 =
            0.9500 * relatedpressure * pow(relatedtemperature, 0.64)
            + 0.0067 * watervapordensity;

    weight3 =
            0.9561 * relatedpressure * pow(relatedtemperature, 0.67)
            + 0.0059 * watervapordensity;

    weight4 =
            0.9543 * relatedpressure * pow(relatedtemperature, 0.68)
            + 0.0061 * watervapordensity;

    weight5 =
            0.9550 * relatedpressure * pow(relatedtemperature, 0.68)
            + 0.0060 * watervapordensity;

    g22 = 1.0 + pow((frequency - 22.235), 2.0) /
          pow((frequency + 22.235), 2.0);

    g557 = 1.0 + pow((frequency-557.0), 2.0) /
           pow((frequency+557.0), 2.0);

    g752 = 1.0 + pow((frequency - 752.0), 2.0) /
           pow((frequency + 752.0), 2.0);

    item1 =
            3.13 * pow(10.0, -2.0) * relatedpressure *
            relatedtemperature + 1.76 *
            pow(10.0,-3.0) * watervapordensity *
            pow(relatedtemperature, 8.5);

   double temp = 1.0 - relatedtemperature;

    item2 =
        3.84 * weight1 * g22 * exp(2.23 * temp) /
        (pow((frequency - 22.235), 2.0) + 9.42 * pow(weight1, 2.0));

    item3 =
        10.48 * weight2 * exp(0.7 * temp) /
        (pow((frequency - 183.31), 2.0) + 9.48 * pow(weight2, 2.0));

    item4 =
        0.078 * weight3 * exp(6.4385 * temp) /
        (pow((frequency-321.226), 2.0) + 6.29 * pow(weight3, 2.0));

    item5 =
        3.76 * weight4 * exp( 1.6 * temp) /
        (pow((frequency - 325.153), 2.0) + 9.22 * pow(weight4, 2.0));

    item6 =
        26.36 * weight5 * exp(1.09 * temp) /
        pow((frequency-380.0), 2.0);

    item7 =
        17.87 * weight5 * exp(1.46 * temp) /
        pow((frequency - 448.0), 2.0);

    item8 =
        frequency * relatedtemperature * deriveda * derivedc *
        0.8837 * weight5 * g557 * exp(0.17 * temp) /
        pow((frequency - 557.0), 2.0);

    item9 =
        302.6 * weight5 * g752 * exp(0.41 * temp) /
        pow((frequency - 752.0), 2.0);

    sumitems = item2 + item3 + item4 + item5 +
               item6 + item7 + item8 + item9;

    watervaporattenuationcoefficient =
            (item1 + pow(relatedtemperature, 2.5) * sumitems) *
            pow(frequency, 2.0) * watervapordensity * pow(10.0, -4.0);

    return watervaporattenuationcoefficient;

}


/* to represent Atmospheric Attenuation only humidity and
 temperature are required pressure and density
  are derived from them */
double Prop_AtmosphericAttenuation(double distance,   // in km
                                   double Frequency,  // in GHz
                                   double humidity,
                                   double temperature)
{

    double saturationPressure;
    double waterVaporPressure;
    double waterVaporDensity;
    double attenuationCoefficientWaterVapor;
    double attenuationCoefficientOxygen;
    double atmosphereattenuation;

    //ITU-R Recommendation P.453-7
    saturationPressure =
            6.1121 * exp(17.502 * temperature /
            ( 240.97 + temperature));

    waterVaporPressure = humidity * saturationPressure / 100.0;

    //ITU-R Recommendation P.453-6
    waterVaporDensity =
            216.7 * waterVaporPressure / (temperature + 273.3);

    attenuationCoefficientOxygen =
             Prop_OxygenAttenuationCoefficient(waterVaporPressure,
                                               temperature,
                                               Frequency);

    attenuationCoefficientWaterVapor =
            Prop_WaterVaporAttenuationCoefficient(waterVaporPressure,
                                                  temperature,
                                                  waterVaporDensity,
                                                  Frequency);

    atmosphereattenuation =
            distance * (attenuationCoefficientOxygen +
            attenuationCoefficientWaterVapor);

    return atmosphereattenuation;

}


/*
  Calculate the rain attenuation according to ITU-R Recommendation P.530-8,
  herein frequency is in GHz, distance is in km, and rain intensity is in mm/h.
*/
double Prop_RainAttenuation(double Frequency,   //in GHz
                            double distance,    //in km
                            int polarization,
                            double rainintensity)
{

    int i = 0;
    int frequencyindex = 0;
    double rainattenuationcoefficient_k = 0.0;
    double rainattenuationcoefficient_alpha = 0.0;
    double rainattenuationperunitlength;
    double referencedistance = 1.0;
    double effectiveoathlength;
    double rainattenuation = 0.0;

    if(Frequency > 1.0)
    {
        while(Frequency > rainattenuationparameters[i][0])
        {
            i++;
        }

        frequencyindex = i - 1;

        if (polarization == HORIZONTAL)
        {
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
        else if (polarization == VERTICAL)
        {

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
                    Prop_RainAttenuationCoefficient(
                            rainintensity,
                            rainattenuationcoefficient_k,
                            rainattenuationcoefficient_alpha);

        if (rainintensity <= 100)
        {
            referencedistance = 35.0 * exp(-0.015 * rainintensity);
        }
        else
        {
            referencedistance = 100.0;
        }

        effectiveoathlength =
                distance/(1.0 + distance / referencedistance);

        rainattenuation =
                rainattenuationperunitlength * effectiveoathlength;
    }

    return rainattenuation;
}


// pathloss for LOS according to Recommendation ITU-R P.1411
double PathlossItu_R_los(double  distance, // in meters
                         double  wavelength,  // in meters
                         double  txAntennaHeight,  // in meters
                         double  rxAntennaHeight, // in meters
                         Coordinates fromPosition,
                         Coordinates toPosition)
{

    double breakpointdistance;
    double referenceLoss;
    double pathloss = 0.0;
    double effiectiveRoadHeight = 0.0;
    double temp;
    double rainAttenuation = 0.0;
    double atmosphereAttenuation = 0.0;

    if(txAntennaHeight < rxAntennaHeight)
    {
        temp = txAntennaHeight;
        txAntennaHeight = rxAntennaHeight;
        rxAntennaHeight = temp;
    }

    if (wavelength >= 0.03) // less than 10 GHz
    {
        // SHF 3GHz
        if (wavelength <= 0.1) {
            effiectiveRoadHeight = 0.43;
        }

        if ((txAntennaHeight > effiectiveRoadHeight) &&
            (rxAntennaHeight > effiectiveRoadHeight))
        {
            if (distance > 0.0)
            {
                double h1RoadDiff = txAntennaHeight - effiectiveRoadHeight;
                double h2RoadDiff = rxAntennaHeight - effiectiveRoadHeight;

                breakpointdistance =
                            4.0 * h1RoadDiff * h2RoadDiff / wavelength;

                referenceLoss = 10.0 + 2.0 * IN_DB(
                            8.0 * PI * h1RoadDiff * h2RoadDiff /
                            (wavelength * wavelength));

                if (distance <= breakpointdistance)
                {
                    pathloss =
                        referenceLoss + 2.0 *
                        IN_DB(distance / breakpointdistance);
                }
                else
                {
                    pathloss =
                        referenceLoss + 4.0 *
                        IN_DB(distance / breakpointdistance);
                }
            }
        }
        else
        {
            if (distance > 0.0)
            {
                breakpointdistance = 20.0;

                referenceLoss = 10.0 + 2.0 * IN_DB(
                        (2.0 * PI * breakpointdistance / wavelength));

                if (distance <= breakpointdistance)
                {
                    pathloss =
                        referenceLoss + 2.0 *
                        IN_DB(distance / breakpointdistance);
                }
                else
                {
                    pathloss =
                        referenceLoss + 3.0 *
                        IN_DB(distance / breakpointdistance);
                }
            }
        }
    }
    else
    {// larger than 10GHz

        if((distance > 0.0) && (wavelength > 0.0))
        {
            pathloss =  2.2 * IN_DB(4.0 * PI * distance / wavelength);

            double distanceKm = distance / 1000.0;
            double frequencyGHz = (SPEED_OF_LIGHT / wavelength) / 1.0e9;
            int    polarization = HORIZONTAL;
            double rainIntensity = DEFAULT__RAIN__INTENSITY;
            double humidity = DEFAULT__HUMIDITY;
            double temperature = DEFAULT__TEMPERATURE;

            // Rain attenuation according to Recommendation ITU-R P.530
            rainAttenuation = Prop_RainAttenuation(
                                         frequencyGHz,
                                         distanceKm,
                                         polarization,
                                         rainIntensity);

            // Gaseous attenuation according to Recommendation ITU-R P.676
            atmosphereAttenuation =
                        Prop_AtmosphericAttenuation(
                                   distanceKm,
                                   frequencyGHz,
                                   humidity,
                                   temperature);

            pathloss =
                    pathloss + rainAttenuation + atmosphereAttenuation;
        }
    }

    if (DEBUG) {
        printf("\n ITU-R LOS:\n"
               "    Distance (m) = %f\n"
               "    Tx antenna height (m) = %f\n"
               "    Rx antenna height (m) = %f\n"
               "    Wavelength (m) = %f\n"
               "    Rain attenuation  = %f\n"
               "    Atmosphere attenuation = %f\n"
               "    Total pathloss = %f\n",
               distance, txAntennaHeight, rxAntennaHeight,
               wavelength, rainAttenuation,
               atmosphereAttenuation, pathloss);

        fflush(stdout);
    }

    return pathloss;
}


/* Rooftop to street propagation loss */
static
double ITU_R_RooftoStreetDiffractionLoss(
                                    double frequencyMhz,
                                    double incidentAngle,
                                    double h_ms,
                                    double avgRoofHeight,
                                    double streetWidth)
{
    double streetDiffractionLoss;
    double lori = 0.0;
    double heightDifference;
    int incidentangle;
    double inAngle;

    heightDifference =  avgRoofHeight - h_ms;

    incidentangle = RoundToInt(incidentAngle);

    if (incidentangle < 0) {
        while (incidentangle < 0) {

            incidentangle += 90;
        }
    }

    if (incidentangle >= 90) {

        incidentangle %= 90;
    }

    inAngle = incidentangle + (incidentAngle - RoundToInt(incidentAngle));
    if (inAngle < 0.0)
    {
        inAngle += 90.0;
    }

    //printf("%f, %f, %d\n", inAngle, incidentAngle, incidentangle);
    assert((inAngle >= 0.0 ) && (inAngle <= 90.0));

    if ((inAngle >= 0.0 )&& (inAngle < 35.0))
    {

        lori = -10.0 + 0.354 * inAngle;
    }
    else if ((inAngle >= 35.0 )&& (inAngle < 55.0))
    {

        lori = 2.5 + 0.075 * (inAngle - 35.0);
    }
    else if ((inAngle >= 55.0 )&& (inAngle <= 90.0))
    {

        lori = 4.0 - 0.114 * (inAngle - 55.0);
    }

    streetDiffractionLoss = -8.2 + lori;

    if(streetWidth > 0.1)
    {
        streetDiffractionLoss += (- IN_DB(streetWidth));
    }

    if ( frequencyMhz > 0.0)
    {
        streetDiffractionLoss += IN_DB(frequencyMhz);
    }

    if (heightDifference > 0.0)
    {
        streetDiffractionLoss += 2.0 * IN_DB(heightDifference);
    }


    if (DEBUG) {
        printf("\n Roof to Street:\n"
               "    Roof Height = %f\n"
               "    Incident Angle = %f\n"
               "    Street width = %f\n"
               "    Frequency ( MHz) = %f\n"
               "    Mbile antenna height  = %f\n"
               "    Street orientation correction factor = %f\n"
               "    Diffraction loss = %f\n",
               avgRoofHeight, incidentAngle, streetWidth,
               frequencyMhz, h_ms, lori, streetDiffractionLoss);

        fflush(stdout);
    }

    return streetDiffractionLoss;
}


static
double ITU_R_L1msd(double distance, // in meters
                   double buildingExtendDistance,  // in meters
                   double frequencyMHz,
                   double h_bs,         // in meters
                   double avgRoofHeight,   // in meters
                   double buildingSeperation,  // in meters
                   PropagationEnvironment  proEnvironment)
 {

    double l1msd;
    double lbsh;
    double ka;
    double kd;
    double kf;
    double heightDifference;

    lbsh = 0.0;
    kd = 18.0;
    kf = -8.0;
    ka = 71.4;

    heightDifference = h_bs - avgRoofHeight;

    if (heightDifference > 0.0)
    {
        lbsh = -18.0 * log10(1 + heightDifference);

        if (frequencyMHz <= 2000.0)
        {
            ka = 54.0;
        }

    }
    else
    {
        kd = 18.0 - 15.0 * heightDifference / avgRoofHeight;

        if (frequencyMHz <= 2000.0)
        {
            ka = 54.0 - 0.8 * heightDifference;

            if (distance < 500.0)
            {
                ka = 54.0 - 1.6 * heightDifference * distance / 1000.0;
            }
        }
        else
        {
            ka = 73.0 - 0.8 * heightDifference;

            if (distance < 500.0)
            {
                ka = 73.0 - 1.6 * heightDifference * distance / 1000.0;
            }
        }
    }

    if (frequencyMHz <= 2000.0)
    {
        kf = -4.0 + 0.7 * (frequencyMHz / 925.0 - 1.0);

        if (proEnvironment == METROPOLITAN)
        {
            kf = -4.0 + 1.5 * (frequencyMHz / 925.0 - 1.0);
        }
    }

    l1msd = lbsh + ka;

    if (distance > 0.0)
    {
        l1msd += kd * log10 (distance / 1000.0);
    }

    if (frequencyMHz > 0.0)
    {
        l1msd +=  kf * log10( frequencyMHz);
    }

    if (buildingSeperation > 0.0)
    {
        l1msd += (- 9.0 * log10(buildingSeperation));
    }

    return l1msd;

 }


static
double ITU_R_L2msd(double distance,                // in meters
                    double buildingExtendDistance, //in meters
                    double frequencyMHz,
                    double h_bs,                   // in meters
                    double avgRoofHeight,          // in meters
                    double buildingSeperation)     // in meters
{
    double l2msd;
    double Qm = 1.0;
    double theta;
    double rho;
    double heightDifference;
    double deatahu;
    double deatahl;
    double temp1 = 0.0;
    double temp2 = 0.0;
    double temp3 = 0.0;
    double temp4 = 0.0;
    double waveLength;

    heightDifference = h_bs - avgRoofHeight;
    waveLength = 1.0e-6 * SPEED_OF_LIGHT / frequencyMHz;

    if ((buildingSeperation > 0.0) && (waveLength > 0.0))
    {
        temp1 = log(sqrt(buildingSeperation / waveLength));
    }

    if(distance > 0.0)
    {
        temp2 = log(distance) / 9.0;
    }

    if (buildingSeperation > 0.0)
    {
        temp3 = (10.0 / 9.0) * log(buildingSeperation/2.35);
    }

    deatahu = pow(10.0, (-temp1 - temp2 + temp3));

    temp4 =
        (0.00023 * buildingSeperation * buildingSeperation -
        0.1827 * buildingSeperation - 9.4978) /
        (pow(log(frequencyMHz), 2.938));

    deatahl = 0.000781 * buildingSeperation + 0.06932 + temp4;

    theta = atan(heightDifference / buildingSeperation);

    rho = sqrt(heightDifference * heightDifference +
          buildingSeperation * buildingSeperation);


    if (heightDifference > deatahu)
    {

        Qm = 2.35 * pow((heightDifference / distance) *
            sqrt(buildingSeperation / waveLength), 0.9);
    }
    else if ((heightDifference <= deatahu) &&
            (heightDifference >= deatahl))
    {

        Qm = buildingSeperation / distance;
    }
    else if (heightDifference < deatahl)
    {

        Qm = (buildingSeperation / (2.0 * PI * distance)) *
             sqrt(waveLength / rho) *
             ( 1 / theta - 1 / (2 * PI + theta));
    }

    l2msd = -IN_DB(Qm * Qm);

    return l2msd;
}



/* Multi-screen propagation loss */
static
double ITU_R_MultiScreenLoss(double distance,  // in meters
                                   double buildingExtendDistance, // in meters
                                   double frequencyMHz,
                                   double h_bs,   // in meters
                                   double h_ms,   // in meters
                                   double avgRoofHeight,  // in meters
                                   double buildingSeperation,  // in meters
                                   PropagationEnvironment  proEnvironment)
{
    double settledFieldDistance;
    double heightDifference;
    double waveLength;
    double dbp = 1.0;
    double llow;
    double lupp;
    double lmid;
    double xiks;
    double dhbp;
    double luppmid;
    double lmidlow;
    double lmsd;
    double l1msd;
    double temp;
    double l2msd;

    waveLength = 1.0e-6 * SPEED_OF_LIGHT / frequencyMHz;
    heightDifference = h_bs - avgRoofHeight;

    if ((buildingExtendDistance > 0.0) && (waveLength > 0.0))
    {
        dbp = fabs(heightDifference) *
              sqrt(buildingExtendDistance / waveLength);
    }

    llow = ITU_R_L2msd(dbp,
                       buildingExtendDistance,
                       frequencyMHz,
                       h_bs,
                       avgRoofHeight,
                       buildingSeperation);

    lupp = ITU_R_L1msd(dbp,
                       buildingExtendDistance,
                       frequencyMHz,
                       h_bs,
                       avgRoofHeight,
                       buildingSeperation,
                       proEnvironment);


    lmid = (llow + lupp) / 2.0;

    xiks = (lupp - llow) * 0.0417;
    dhbp = lupp - llow;

    luppmid = lupp - lmid;
    lmidlow = lmid -llow;

    settledFieldDistance =
            (waveLength * distance * distance ) /
            (heightDifference * heightDifference);

    l2msd = ITU_R_L2msd(distance,
                       buildingExtendDistance,
                       frequencyMHz,
                       h_bs,
                       avgRoofHeight,
                       buildingSeperation);

    l1msd = ITU_R_L1msd(distance,
                       buildingExtendDistance,
                       frequencyMHz,
                       h_bs,
                       avgRoofHeight,
                       buildingSeperation,
                       proEnvironment);

    temp = log(distance) - log(dbp);

    lmsd = l2msd;

    if (buildingExtendDistance > settledFieldDistance)
    {
        if(dhbp > 0.0)
        {
            lmsd = -tanh(temp / 0.1) * (l1msd - lmid) + lmid;
        }
        else if(dhbp < 0.0)
        {
            lmsd = l1msd - tanh(temp / xiks) * luppmid - luppmid;
        }
    }
    else
    {
        if(dhbp > 0.0)
        {
            lmsd = tanh(temp / 0.1) * (l2msd - lmid) + lmid;
        }
        else if(dhbp < 0.0)
        {
            lmsd =l2msd + tanh(temp / xiks) * lmidlow + lmidlow;
        }
    }

    if (DEBUG) {
        printf("\n Multiscreen Loss:\n"
               "    Distance = %f\n"
               "    Roof Height = %f\n"
               "    Building Extend Distance = %f\n"
               "    Building Seperation = %f\n"
               "    Frequency ( MHz) = %f\n"
               "    BS antenna height = %f\n"
               "    MS antenna height  = %f\n"
               "    Multiscreen Diffraction Loss = %f\n",
               distance, avgRoofHeight, buildingExtendDistance,
               buildingSeperation, frequencyMHz, h_bs, h_ms, lmsd);

        fflush(stdout);
    }

    return lmsd;
}


/* Loss under non-line-of-sight conditions */
double PathlossITU_R_NLoS1 (
                        double distanceKm,
                        double frequencyMhz,
                        double h1,   // BS antenna height in meters
                        double h2,   // MS antenna height in meters
                        double streetwidth, //in meters
                        double avgRoofheight,  // in meters
                        double incidentangle,  // in degrees
                        double buildingseperation, // in meters
                        double buildingextendDistance,  // in meters
                        PropagationEnvironment  proEnvironment)
{
    double freeSpaceLoss = 0.0;
    double streetDiffractionLoss = 0.0;
    double multiscreenDiffractionLoss = 0.0;
    double pathloss_dB = 0.0;
    double temp;

    if(h1 < h2)
    {
        temp = h1;
        h1 = h2;
        h2 = temp;
    }

    if (DEBUG) {
        printf("\n ITU_R_NLoS1: \n"
               "    Distance (Km) = %f\n"
               "    Frequency (MHz) = %f\n"
               "    Bs antenna height (m) = %f\n"
               "    Ms antenna height (m) = %f\n"
               "    Incident angle = %f\n"
               "    Street width (m) = %f\n",
               distanceKm, frequencyMhz, h1, h2,
               incidentangle, streetwidth);

        fflush(stdout);
    }

    if (distanceKm > 0.0)
    {
        streetDiffractionLoss = ITU_R_RooftoStreetDiffractionLoss(
                                    frequencyMhz,
                                    incidentangle,
                                    h2,
                                    avgRoofheight,
                                    streetwidth);

        multiscreenDiffractionLoss =
                ITU_R_MultiScreenLoss(distanceKm * 1000.0,
                                      buildingextendDistance,
                                      frequencyMhz,
                                      h1,
                                      h2,
                                      avgRoofheight,
                                      buildingseperation,
                                      proEnvironment);

        freeSpaceLoss =
            32.4 + 2.0 * IN_DB(distanceKm) + 2.0 * IN_DB(frequencyMhz);

        if ((streetDiffractionLoss + multiscreenDiffractionLoss) > 0.0) {

            pathloss_dB = freeSpaceLoss + streetDiffractionLoss
                          + multiscreenDiffractionLoss;
        }
        else {
            pathloss_dB = freeSpaceLoss;
        }

        if (DEBUG) {
            printf("\n ITU_R_NLoS1:\n"
                   "    Freespace loss (dB) = %f\n"
                   "    Roof-to-street loss (dB) = %f\n"
                   "    Multiscr loss (dB) = %f\n ",
                   freeSpaceLoss, streetDiffractionLoss,
                   multiscreenDiffractionLoss);

            fflush(stdout);

        }
    }

    return pathloss_dB;
}


// 800 to 2000 MHz
double PathlossItu_R_Nlos2LowBand(double  txStreetWidth, // in meters
                         double  rxStreetWidth, // in meters
                         double  wavelength,  // in meters
                         double  txStreetCrossingDistance,  // in meters
                         double  rxStreetCrossingDistance, // in meters
                         double  cornerAngle) // in degree
{
    double pathloss = 0.0;
    double reflectionLoss;
    double diffractionLoss;
    double cornerAngleFactor;
    double cornerfactor;
    double distanceSum;
    double distanceMultiply;
    double frequencyFactor;
    double txDistanceToStreetWidth;
    double rxDistanceToStreetWidth;
    int corAngle;
    double cornerangle;

    if ((txStreetCrossingDistance + rxStreetCrossingDistance) > 0.0)
    {
        corAngle = RoundToInt(cornerAngle);

        if (corAngle < 0.0) {
            while (corAngle < 0.0) {

                corAngle += ANGLE_RESOLUTION;
            }
        }

        if (corAngle >= ANGLE_RESOLUTION) {

            corAngle %= ANGLE_RESOLUTION;
        }

        corAngle %= (ANGLE_RESOLUTION / 2);

        cornerangle = corAngle + (cornerAngle - RoundToInt(cornerAngle));

        if(cornerangle <= 0.0) {

            cornerangle = 0.6;
        }

        distanceSum = txStreetCrossingDistance + rxStreetCrossingDistance;
        distanceMultiply = txStreetCrossingDistance * rxStreetCrossingDistance;
        frequencyFactor = 4.0 * PI / wavelength;
        txDistanceToStreetWidth = txStreetCrossingDistance / txStreetWidth;
        rxDistanceToStreetWidth = rxStreetCrossingDistance / rxStreetWidth;

        cornerAngleFactor = 3.86 / (pow(cornerAngle * IN_RADIAN, 3.5));

        cornerfactor =
            (40.0 / (2.0 * PI)) * (atan(txDistanceToStreetWidth)
            + atan(rxDistanceToStreetWidth) - (PI / 2.0));

        diffractionLoss =
            IN_DB(distanceMultiply * distanceSum) + 2.0 *
            cornerfactor - 0.1 * ( 90.0 - cornerAngle / IN_RADIAN) +
            2.0 * IN_DB(frequencyFactor);

        reflectionLoss =
            2.0 * IN_DB(distanceSum) + cornerAngleFactor  *
             txDistanceToStreetWidth * rxDistanceToStreetWidth +
            2.0 * IN_DB(frequencyFactor);

        pathloss = -IN_DB(NON_DB(-diffractionLoss) + NON_DB(-reflectionLoss));
    }

    return pathloss;

}


// 2 to 16 GHz
double PathlossItu_R_Nlos2HighBand(double  txStreetWidth, // in meters
                         double  rxStreetWidth, // in meters
                         double  wavelength,  // in meters
                         double  txStreetCrossingDistance,  // in meters
                         double  rxStreetCrossingDistance, // in meters
                         double  cornerAngle,
                         double  txAntennaHeight,  // in meters
                         double  rxAntennaHeight, // in meters
                         Coordinates fromPosition,
                         Coordinates toPosition)
{

    double pathloss = 0.0;
    double cornerloss;
    double cornerRegion;
    double distanceCorner;
    double latt;
    double distanceSum;
    double losLoss;
    double beta;

    distanceCorner = 30.0;
    cornerloss = 20.0;

    beta = 6.0;
    latt = 0;

    distanceSum = txStreetCrossingDistance + rxStreetCrossingDistance;

    if (distanceSum > 0.0)
    {
        cornerRegion = txStreetWidth / 2.0 + 1.0 + distanceCorner;

        if (rxStreetCrossingDistance <= cornerRegion)
        {

            cornerloss =
                (cornerloss / (1.0 - log10(1.0 + distanceCorner))) *
                (1.0 - log10(rxStreetCrossingDistance - txStreetWidth / 2.0));
        }

        if (rxStreetCrossingDistance > cornerRegion)
        {

            latt =
                10.0 * beta * log10(distanceSum /
                (txStreetCrossingDistance + cornerRegion - 1.0));
        }

        losLoss = PathlossItu_R_los(distanceSum, // in meters
                                    wavelength,  // in meters
                                    txAntennaHeight,  // in meters
                                    rxAntennaHeight, // in meters
                                    fromPosition,
                                    toPosition);

        pathloss = losLoss + cornerloss + latt;
    }

    return pathloss;
}



double PathlossItu_R_UHFlos(double  distance, // in meters
                            double  wavelength,  // in meters
                            double  locationPercentage)  // percentage
{

    double losLoss = 0.0;
    double losLocationCorrection = 0.0;
    double lossLos = 0.0;
    double distanceKm = 0.0;
    double frequencyMHz = 0.0;

    if ((distance > 0.0) && (wavelength > 0.0))
    {
        distanceKm = distance / 1000.0;
        frequencyMHz = (SPEED_OF_LIGHT / wavelength) / 1.0e6;

        losLoss =
            32.45 + (2.0 * IN_DB(frequencyMHz) + 2.0 * IN_DB(distanceKm));

        if ((locationPercentage > 0.0) && (locationPercentage < 100.0)) {

            losLocationCorrection =
                1.5624 * 7.0 * (sqrt(-2.0 * log(1.0 - locationPercentage
                / 100.0)) - 1.1774);
        }

        lossLos = losLoss + losLocationCorrection;
    }

    return lossLos;
}

// Implementation of location Variability Function according
// to ITU-R Recommendation P.1546.3
static
double locationVariabilityFunction(double variabilityFactor)
{
    double kase;
    double t;
    double q;
    double temp;
    double c[3] = {2.515517, 0.802853, 0.010328};
    double d[3] = {1.432788, 0.189269, 0.001308};

    temp = variabilityFactor;

    if (variabilityFactor > 0.5)
    {
        temp = 1.0 - variabilityFactor;
    }

    t = sqrt(fabs(-2.0 * log(temp)));

    kase =
        ((c[2] * t + c[1]) * t + c[0]) /
        (((d[2] * t + d[1]) * t + d[0]) * t +1.0);

    q = t - kase;

    if (variabilityFactor > 0.5)
    {
        q = -( t - kase);
    }

    return q;

}


double PathlossItu_R_UHFnlos(
            double  distance, // in meters
            double  wavelength,  // in meters
            double  locationPercentage) // percentage [ 0 - 100]
{

    double nlosLoss = 0.0;
    double nlosLocationCorrection = 0.0;
    double distanceKm;
    double frequencyMhz;
    double lossNlos = 0.0;

    distanceKm = distance / 1000.0;
    frequencyMhz = (SPEED_OF_LIGHT / wavelength) / 1.0e6;

    if((frequencyMhz > 0.0) && (distanceKm > 0.0))
    {
        nlosLoss =
                9.5 + 4.5 * IN_DB(frequencyMhz) +
                4.0 * IN_DB(distanceKm) + 6.8;

        if ((locationPercentage > 0.0) && (locationPercentage < 100.0))
        {
            nlosLocationCorrection =
                    7.0 * locationVariabilityFunction(
                    locationPercentage / 100.0);
        }

        lossNlos = nlosLoss + nlosLocationCorrection;
    }

    return lossNlos;

}


static
double losDistance( double locationPercentage) // percentage [ 0 100]
{
    double distance = 2.9;
    double percentage;

    if ((locationPercentage > 0.0) && (locationPercentage < 100.0))
    {
        percentage = locationPercentage / 100.0;
        distance = 72.9 - 70.0 * (percentage);

        if (locationPercentage < 45.0)
        {
            distance =
                    212.0 * (log10(percentage) * log10(percentage)) -
                    64.0 * (log10(percentage));

        }
    }

    return distance;
}


// frquency range 300 - 3000 MHz
// low height terminals where both Tx Rx antenna heights
// are near street level well below roof-top height.

double PathlossItu_R_UHF(double  distance, // in meters
                         double  wavelength,  // in meters
                         BOOL    isLOS,
                         double  locationPercentage) // percentage [ 0 100]
{
    double losdistance;
    double pathloss;
    double transitionRegion;
    double pathlossLos;
    double pathlossNlos;

    pathloss = 0.0;
    transitionRegion = 20.0;

    if ((distance > 0.0) && (wavelength > 0.0))
    {
        if (isLOS)
        {
            pathloss =
                PathlossItu_R_UHFlos(distance,
                                     wavelength,
                                     locationPercentage);
        }
        else {

            losdistance = losDistance(locationPercentage);

            pathloss =
                PathlossItu_R_UHFnlos(distance,
                                      wavelength,
                                      locationPercentage);

            if ((distance < (losdistance + transitionRegion)) &&
                (distance > losdistance))
            {
                pathlossLos =
                    PathlossItu_R_UHFlos(losdistance,
                                         wavelength,
                                         locationPercentage);

                pathlossNlos =
                    PathlossItu_R_UHFnlos(losdistance + transitionRegion,
                                          wavelength,
                                          locationPercentage);

                pathloss =
                    pathlossLos + (pathlossNlos - pathlossLos) *
                    (distance - losdistance) / transitionRegion;
            }
        }
    }

    return pathloss;
}


static
double calculationE1(double distance,   // in Km
                     double estrength[36][6],
                     int hIndex,
                     int lIndex,
                     double h1,
                     double lowH,
                     double highH)
{
    double efield = 0.0;
    double e01;
    double e1;
    double e1low;
    double e1high;

    if ( distance <= 0.01)
    {
        efield = 106.9 - 2.0 * IN_DB(0.01);
    }
    else if ((distance <= 0.1) && (distance > 0.01))
    {
        efield = 106.9 - 2.0 * IN_DB(distance);
    }
    else
    {
        e01 = 106.9 - 2.0 * IN_DB(0.1);
        e1 = estrength[0][lIndex];

        if (hIndex != -1)
        {
            e1low = estrength[0][lIndex];
            e1high = estrength[0][hIndex];

            e1 = e1low + (e1high - e1low) *
                 log10(h1 / lowH) / log10(highH / lowH);

        }

        efield = e01 + (e1 - e01) * log10(distance / 0.1);
    }

    return efield;


}


static
double calculationCh1( double height,
                       double frequencyMHz)
{

    double kv;
    double thetaeff2;
    double v;
    double j;
    double ch1;
    double temp;

    kv = 6.0;

    if ((frequencyMHz < 900.0) && (frequencyMHz >= 300.0))
    {
        kv = 3.31;
    }
    else if (frequencyMHz < 300.0)
    {
        kv = 1.35;
    }

    thetaeff2 = atan(-height / 9000.0) * 180.0 / PI;
    v = kv * thetaeff2;

    temp = v - 0.1;
    j = 6.9 + 2.0 * IN_DB(sqrt(temp * temp +1) + temp);

    ch1 = 6.03 - j;

    return ch1;
}


static
double correctionMSantennaHeight(double distance,  // in Km
                                 double ms_h,     // in meters
                                 double bs_h,     // in meters
                                 double frequencyMhz,
                                 double r) // in meters
{
    double knu;
    double kh2;
    double thetaclut;
    double hdiff;
    double v;
    double jv;
    double temp;
    double correction = 0.0;
    double mr;

    mr =
        (1000.0 * distance * r - 15.0 * bs_h) /
        (1000.0 * distance - 15.0);

    if ( bs_h < 6.5 * distance + r)
    {
        mr = r;
    }

    if ( mr < 1.0)
    {
        mr = 1.0;
    }

    knu = 0.0108;
    kh2 = 3.2;

    if (frequencyMhz > 0.0)
    {
        knu = 0.0108 * sqrt(frequencyMhz);
        kh2 = 3.2 + 6.2 * log10(frequencyMhz);
    }

    if ( ms_h >= mr)
    {
        correction = kh2 * log10(ms_h / mr);
    }
    else
    {
        hdiff = mr - ms_h;
        thetaclut = (atan(hdiff / 27.0)) * 180.0 / PI;
        v = knu * sqrt(hdiff * thetaclut);

        temp = v - 0.1;
        jv = 6.9 + 2.0 * IN_DB(sqrt(temp * temp +1) + temp);

        correction = 6.03 - jv;
    }

    if (mr < 10.0)
    {
        correction -= kh2 * log10(10.0 / mr);
    }

   if(DEBUG)
    {
        printf("\n VHF UHF MS Antenna Height Correction:\n"
               "    Frequency (MHz) = %f\n"
               "    Bs antenna height (m) = %f\n"
               "    Ms antenna height (m) = %f\n"
               "    Distance (Km) = %f\n"
               "    Average Roof Height (m) = %f\n"
               "    Pathloss Correction = %f\n",
               frequencyMhz, bs_h, ms_h, distance,
               r, correction);

        fflush(stdout);

    }

    return correction;
}


static
double clearanceCorrection(double streetWidth, // in meters
                           double aveRoofH,    // in meters
                           double h_ms,       // in meters
                           double frequencyMhz)
{
    double diffH;
    double clearanceAngle;
    double v;
    double vm;
    double temp;
    double temp1;
    double jv;
    double jvm;
    double correction;


    diffH = aveRoofH - h_ms;
    clearanceAngle = atan(diffH / (streetWidth / 2.0)) * 180.0 / PI;

    if (clearanceAngle > 40.0)
    {
        clearanceAngle = 40.0;
    }

    if (clearanceAngle < 0.55)
    {
        clearanceAngle = 0.55;
    }

    v = 0.065;
    vm = 0.036;

    if (frequencyMhz > 0.0)
    {
        v = 0.065 * clearanceAngle * sqrt(frequencyMhz);
        vm = 0.036 * sqrt(frequencyMhz);
    }

    temp = v - 0.1;
    jv = 6.9 + 2.0 * IN_DB(sqrt(temp * temp +1) + temp);

    temp1 = vm - 0.1;
    jvm = 6.9 + 2.0 * IN_DB(sqrt(temp1 * temp1 +1) + temp1);

    correction = jvm - jv;

   if(DEBUG)
    {
        printf("\n VHF UHF Clearance Correction:\n"
               "    Frequency (MHz) = %f\n"
               "    Ms antenna height (m) = %f\n"
               "    Average Roof Height (m) = %f\n"
               "    Street Width (m) = %f\n"
               "    Pathloss correction = %f\n",
               frequencyMhz, h_ms, aveRoofH,
               streetWidth, correction);

        fflush(stdout);

    }

    return correction;
}


static
double distanceCorrection(double dis_Km,  // in Km
                          double avgRoofH,  // in meters
                          double height,   // in meters
                          double freMhz)
{
    double losecorrection;
    double freFactor = 0.0;
    double disFactor = 0.0;
    double hfactor = 1.0;

    if(freMhz > 0.0)
    {
        freFactor = log10(freMhz);
    }

    if(dis_Km > 0.02)
    {
        disFactor = 1 - 0.85 *log10(dis_Km);
    }

    if((height - avgRoofH) > -1.0)
    {
        hfactor = 1 - 0.46 * log10(1 + height - avgRoofH);

    }

    losecorrection =
        -3.3 * freFactor * disFactor * hfactor;

   if(DEBUG)
    {
        printf("\n VHF UHF Distance Correction:\n"
               "    Frequency (MHz) = %f\n"
               "    Average Roof Height (m) = %f\n"
               "    Antenna Height (m) = %f\n"
               "    Distance (Km) = %f\n"
               "    Pathloss Correction = %f\n",
               freMhz, avgRoofH, height, dis_Km, losecorrection);

        fflush(stdout);

    }

    return losecorrection;
}


static
double calculationLoss( int highHindex,
                        int lowHindex,
                        int highDindex,
                        int lowDindex,
                        double distance_Km,
                        double h1,
                        double h2,
                        double frequency_MHz,
                        double fieldStrength[36][6],
                        double avgRoofHeight)
{
    double efield = 0.0;
    double elow;
    double ehigh;
    double dlow;
    double dhigh;
    double hlow;
    double hhigh;
    double eDlow;
    double eDhigh;
    double temp;
    double disCorrection = 0.0;

    if(h2 > h1)
    {
        temp = h1;
        h1 = h2;
        h2 = temp;
    }

    dlow = distanceV[lowDindex];
    dhigh = distanceV[lowDindex];

    if (highDindex != -1)
    {
        dhigh = distanceV[highDindex];
    }

    hlow = heightsV[lowHindex];
    hhigh = heightsV[lowHindex];

    if (highHindex != -1)
    {
        hhigh = heightsV[highHindex];
    }

    if(highHindex == -1)
    {// antenna height at specified value
        if (highDindex == -1)
        {// distance at specified value
            efield = fieldStrength[lowDindex][lowHindex];

        }
        else
        { // distance at other value
            if(distance_Km < 1.0)
            {
                efield = calculationE1(distance_Km,
                                       fieldStrength,
                                       highHindex,
                                       lowHindex,
                                       h1,
                                       hlow,
                                       hhigh);
            }
            else
            {
                //low distance for the antenna height
                elow = fieldStrength[lowDindex][lowHindex];

                // high distance for the antenna height
                ehigh = fieldStrength[highDindex][lowHindex];
                //Interpolation

                efield =
                    elow + (ehigh - elow) * log10(distance_Km / dlow) /
                    log10(dhigh / dlow);

            }
        }
    }
    else
    { // antenna height at other value
        if (h1 > 10.0)
        {
            if (highDindex == -1)
            {  // distance at specified value

                //low distance for low height antenna
                elow = fieldStrength[lowDindex][lowHindex];

                //low distance for high height antenna
                ehigh = fieldStrength[lowDindex][highHindex];

                //Interpolation
                efield = elow + (ehigh - elow) * log10(h1 / hlow) /
                         log10(hhigh / hlow);


            }
            else
            { // distance at other value
                if(distance_Km < 1.0)
                {
                    efield = calculationE1(distance_Km,
                                           fieldStrength,
                                           highHindex,
                                           lowHindex,
                                           h1,
                                           hlow,
                                           hhigh);

                }
                else
                {
                    //low distance for low height antenna
                    elow = fieldStrength[lowDindex][lowHindex];

                    //low distance for high height antenna
                    ehigh = fieldStrength[lowDindex][highHindex];

                    //Interpolation for antenna heights
                    eDlow = elow + (ehigh - elow) * log10(h1 / hlow) /
                            log10(hhigh / hlow);

                    //high distance for low height antenna
                    double edlow = fieldStrength[highDindex][lowHindex];

                    //high distance for high height antenna
                    double edhigh = fieldStrength[highDindex][highHindex];

                    //Interpolation for antenna height
                    eDhigh = edlow + (edhigh - edlow) * log10(h1 / hlow) /
                             log10(hhigh / hlow);

                    //Interpolation for distance
                    efield = eDlow + (eDhigh - eDlow) *
                             log10(distance_Km / dlow) /
                             log10(dhigh / dlow);

                }

            }
        }
        else
        { // antenna height is less than 10.0
            if(distance_Km < 1.0)
            {
                efield = calculationE1(distance_Km,
                                       fieldStrength,
                                       highHindex,
                                       lowHindex,
                                       h1,
                                       hlow,
                                       hhigh);
            }
            else
            {
                double elow10 = fieldStrength[lowDindex][0];
                double ehigh10 = fieldStrength[lowDindex][0];
                double elow20 = fieldStrength[lowDindex][1];
                double ehigh20 = fieldStrength[lowDindex][1];

                if (highDindex != -1)
                {
                    ehigh10 = fieldStrength[highDindex][0];
                    ehigh20 = fieldStrength[highDindex][1];
                }

                double E10 = elow10;
                double E20 = elow20;

                if (highDindex != -1)
                {
                    E10 = elow10 + (ehigh10 - elow10) *
                          log10(distance_Km / dlow) / log10(dhigh / dlow);

                    E20 = elow20 + (ehigh20 - elow20) *
                          log10(distance_Km / dlow) / log10(dhigh / dlow);
                }

                double C1020 = E10 - E20;

                double Ch1neg10 =
                            calculationCh1(-10.0, frequency_MHz);

                double Ezero = E10;

                if((C1020 < 0.0) && (Ch1neg10 < 0.0))
                {
                    Ezero = E10 + 0.5 * (C1020 + Ch1neg10);
                }

                efield = Ezero + 0.1 * h1 * (E10 - Ezero);
            }
        }

    }

    if ((distance_Km < 15.0) && ((h1 - 20.0) < 150.0 ))
    {
        disCorrection =
                distanceCorrection(distance_Km,
                                   avgRoofHeight,
                                   h1,
                                   frequency_MHz);
    }

    efield += disCorrection;

    return efield;
}


// frquency range 30 - 300 MHz
double PathlossItu_R_VHFUHF(double  distanceKm,
                            double  frequencyMHz,
                            double  h_bs,    // in meters
                            double  h_ms,    // in meters
                            double  streetWidth, //in meters
                            double avgRoofHeight, // in meters
                            BOOL isLOS)
{

    /*if(DEBUG)
        std::cout << "Entering PathlossItu_R_VHFUHF..." 
                  << std::endl;*/

    double temp;
    int i;
    int j;
    double (*lowStrength)[36][6] = NULL;
    double (*highStrength)[36][6] = NULL;
    double E = 0.0;
    double loss = 0.0;
    double maxFieldStrength;
    int lowDistIndex;
    int highDistIndex;
    int lowHeightIndex;
    int highHeightIndex;
    double nLowFre = 0.0;
    double nHighFre = 0.0;

    if ((distanceKm > 0.0) && (frequencyMHz > 0.0))
    {
        if (h_ms > h_bs)
        {
            temp = h_bs;
            h_bs = h_ms;
            h_ms = temp;
        }

        lowHeightIndex = 5;
        highHeightIndex = 6;

        if(h_bs < 300.0)
        {
            i = 0;

            while (h_bs > heightsV[i])
            {
                i++;
            }

            if (i == 0)
            {
                i = 1;
            }

            lowHeightIndex = i - 1;
            highHeightIndex = i;

            if(heightsV[i-1] == h_bs)
            {
                highHeightIndex = -1;
            }
        }

        lowDistIndex = 35;
        highDistIndex = 36;

        if (distanceKm < 100.0)
        {
            j = 0;

            while (distanceKm > distanceV[j])
            {
                j++;
            }

            if (j == 0)
            {
                j = 1;
            }

            lowDistIndex = j - 1 ;
            highDistIndex = j;

            if (distanceKm == distanceV[j - 1])
            {
                highDistIndex = -1;
            }

        }

        if (frequencyMHz < 600.0)
        {
            lowStrength = &fieldStrength_100MHz;
            nLowFre = 100.0;
            nHighFre = 600.0;

            if (frequencyMHz != 100.0)
            {
                highStrength = &fieldStrength_600MHz;

            }

        }
        else if (frequencyMHz > 600.0)
        {
            lowStrength = &fieldStrength_600MHz;
            highStrength = &fieldStrength_2000MHz;
            nLowFre = 600.0;
            nHighFre = 2000.0;

            if (frequencyMHz == 2000.0)
            {
                lowStrength = &fieldStrength_2000MHz;
                highStrength = NULL;
            }
        }
        else if (frequencyMHz == 600.0)
        {
            lowStrength = &fieldStrength_600MHz;
        }

        if (!highStrength)
        {//frequency equals to 100 600 2000 MHz
            E = calculationLoss( highHeightIndex,
                                 lowHeightIndex,
                                 highDistIndex,
                                 lowDistIndex,
                                 distanceKm,
                                 h_bs,
                                 h_ms,
                                 frequencyMHz,
                                 *lowStrength,
                                 avgRoofHeight);
        }
        else
        { // frequency in other value

            double eLow = calculationLoss(highHeightIndex,
                                    lowHeightIndex,
                                    highDistIndex,
                                    lowDistIndex,
                                    distanceKm,
                                    h_bs,
                                    h_ms,
                                    frequencyMHz,
                                    *lowStrength,
                                    avgRoofHeight);

            double eHigh = calculationLoss(highHeightIndex,
                                    lowHeightIndex,
                                    highDistIndex,
                                    lowDistIndex,
                                    distanceKm,
                                    h_bs,
                                    h_ms,
                                    frequencyMHz,
                                    *highStrength,
                                    avgRoofHeight);

            E = eLow + (eHigh - eLow) *
                log10(frequencyMHz / nLowFre) / log10(nHighFre / nLowFre);

        }

        maxFieldStrength = 106.9 - 2.0 * IN_DB(distanceKm);

        double clearanceC = 0.0;

        if(!isLOS)
        {
            if (avgRoofHeight > h_ms)
            {
                clearanceC =
                    clearanceCorrection(streetWidth,
                                        avgRoofHeight,
                                        h_ms,
                                        frequencyMHz);
            }
        }

        double msHeightCorrection =
                    correctionMSantennaHeight(distanceKm,  // in Km
                                              h_ms,     // in meter
                                              h_bs,     // in meter
                                              frequencyMHz,
                                              avgRoofHeight);

        E = E + clearanceC + msHeightCorrection;

        if ( E > maxFieldStrength)
        {
            E = maxFieldStrength;
        }

        loss = 139.3 - E + 2.0 * IN_DB(frequencyMHz);

        if(DEBUG)
        {
            printf("\n VHF UHF pathloss:\n"
                   "    Frequency (MHz) = %f\n"
                   "    Bs antenna height (m) = %f\n"
                   "    Ms antenna height (m) = %f\n"
                   "    Distance (Km) = %f\n"
                   "    High height index = %d\n"
                   "    Low height index = %d\n"
                   "    High distance index = %d\n"
                   "    Low distance index = %d\n"
                   "    Street width = %f\n"
                   "    Average roof height = %f\n"
                   "    Pathloss = %f\n",
                   frequencyMHz, h_bs, h_ms, distanceKm,
                   highHeightIndex, lowHeightIndex,
                   highDistIndex, lowDistIndex, streetWidth,
                   avgRoofHeight, loss);

            fflush(stdout);

        }
    }

    return loss;
}


// Calculation the penetration loss according to the complex permittivity,
// frequency and thickness of the wall or floor. Herein, complex
// permittivity is function of construction material and frequency.

static
double calculationPenetrationLossFactor(double realPermittivity,
                                        double imagPermittivity,
                                        double wavelength,
                                        double wallThickness)
{
    double modulus;
    double angle;
    double sqrtmodulus;
    double sqrtangle;
    double realsqrtPerm;
    double imagsqrtPerm;
    double realsquarePerm;
    double imagsquarePerm;
    double partNumerator;
    double partDenominator;
    double realsquareRn;
    double imagsquareRn;
    double modulusRn;
    double angleRn;
    double delta;
    double realDelta;
    double imagDelta;
    double modulusT;
    double realDenominatorT;
    double imagDenominatorT;
    double modulusNumeratorPart;
    double angleNumeratorPart;
    double realNumeratorT;
    double imagNumeratorT;
    double realT;
    double imagT;
    double Tsquare;
    double penetrationLoss;

    modulus = sqrt(realPermittivity * realPermittivity  +
                   imagPermittivity * imagPermittivity);

    angle = atan(imagPermittivity / realPermittivity);

    sqrtmodulus = sqrt(modulus);
    sqrtangle = angle / 2.0;

    realsqrtPerm = sqrtmodulus * cos(sqrtangle);
    imagsqrtPerm = sqrtmodulus * sin(sqrtangle);

    realsquarePerm = realsqrtPerm * realsqrtPerm;
    imagsquarePerm = imagsqrtPerm * imagsqrtPerm;

    partNumerator = 1 - realsquarePerm - imagsquarePerm;
    partDenominator =
                (1.0 + realsqrtPerm) *
                (1.0 + realsqrtPerm) + imagsquarePerm;

    realsquareRn =
            (partNumerator * partNumerator - 4.0 * imagsquarePerm) /
            (partDenominator * partDenominator);

    imagsquareRn =
            4.0 * imagsqrtPerm * partNumerator /
            (partDenominator * partDenominator);

    modulusRn =
            sqrt(realsquareRn * realsquareRn +
                 imagsquareRn * imagsquareRn);

    angleRn = atan (imagsquareRn / realsquareRn);

    delta = 2.0 * 3.141 * wallThickness * sqrtmodulus / wavelength;

    realDelta = delta * cos(sqrtangle);
    imagDelta = delta * sin(sqrtangle);

    modulusT = modulusRn * exp(2.0 * imagDelta);

    realDenominatorT =
        1.0 - modulusT * cos(2.0 * realDelta + angleRn);
    imagDenominatorT = modulusT * sin(2.0 * realDelta + angleRn);

    modulusNumeratorPart =
            sqrt((1.0 - realsquareRn) * (1.0 - realsquareRn) +
            imagsquareRn * imagsquareRn);
    angleNumeratorPart = atan(imagsquareRn / (1.0 - realsquareRn));

    realNumeratorT =
        modulusNumeratorPart * exp(imagDelta) *
        cos(realDelta + angleNumeratorPart);

    imagNumeratorT =
        modulusNumeratorPart * exp(imagDelta) *
        sin(realDelta + angleNumeratorPart);


    realT =
        (realNumeratorT * realDenominatorT +
        imagNumeratorT * imagDenominatorT) /
        (realDenominatorT * realDenominatorT +
        imagDenominatorT * imagDenominatorT);

    imagT =
        (imagNumeratorT * realDenominatorT -
        realNumeratorT * imagDenominatorT) /
        (realDenominatorT * realDenominatorT +
        imagDenominatorT * imagDenominatorT);

    Tsquare = realT * realT + imagT * imagT;

    penetrationLoss  = - 10*log10(Tsquare);

    if (DEBUG) {

        printf("\n Indoor Penetration Loss:\n"
               "    Permittivity (real) = %f\n"
               "    Permittivity (imag) = %f\n"
               "    Wavelength (m) = %f\n"
               "    Wall Thickness (m) = %f\n"
               "    Penetration Loss = %f\n",
               realPermittivity, imagPermittivity,
               wavelength, wallThickness,
               penetrationLoss);

        fflush(stdout);
    }

    return penetrationLoss;
}


static
double calculationPermittivity(double frequencyGhz,
                               double *realPermit,
                               double permittivityV[][3])
{
    double imagPermit;
    int k;
    k = 0;

    if (frequencyGhz > 95.9)
    {
        frequencyGhz = 95.9;
    }

    while (frequencyGhz > permittivityV[k][0])
    {
        k++;
    }

    if( k == 0)
    {
        k = 1;
    }

    *realPermit =
        permittivityV[k-1][1] + (permittivityV[k][1] -
        permittivityV[k-1][1]) * (frequencyGhz - permittivityV[k-1][0]) /
        (permittivityV[k][0] - permittivityV[k-1][0]);

    imagPermit =
        permittivityV[k-1][2] + (permittivityV[k][2] -
        permittivityV[k-1][2]) * (frequencyGhz - permittivityV[k-1][0]) /
        (permittivityV[k][0] - permittivityV[k-1][0]);

    if (imagPermit > 0.0)
    {
        imagPermit = (-imagPermit);
    }

    if (DEBUG) {

        printf("\n Complex Permittivity:\n"
               "    real %f\n"
               "    imag %f\n", *realPermit, imagPermit);

        fflush(stdout);

    }

    return imagPermit;
}


static
double getGlassPermittivity(double frequencyGhz,
                            double *realpermit)
{
    double x;
    double nci;
    double temp;
    double realPermitt;
    double imagPermitt;

    x = 0.0;

    if (frequencyGhz > 0.0)
    {
        x = log10(frequencyGhz);
    }

    temp =
        -1.773 + 0.153 * x - 0.027 * pow(x, 2.0) -
        0.011 * pow(x, 3.0) + 0.014 * pow(x, 4.0);

    nci = pow(10.0, temp);

    realPermitt = 2.6 * 2.6 + nci * nci;

    if(realPermitt > 7.0)
    {
        realPermitt = 7.0;
    }

    if(realPermitt < 2.0)
    {
        realPermitt = 2.0;
    }

    imagPermitt = -2.0 * 2.6 * nci;

    if (imagPermitt > -0.01)
    {
        imagPermitt = -0.01;

    }

    if (imagPermitt < -0.4)
    {

        imagPermitt = -0.4;
    }

    *realpermit = realPermitt;

    return imagPermitt;

}


static
double calculationPermitt(double   frequencyM, // in MHz
                          double   *permittivityReal,
                          TERRAIN::ConstructionMaterials meterialConst)
{
    double realP = 0.0;
    double imagP = 0.0;
    double frequencyGhz;
    double (*strength)[3][3] = NULL;
    double (*estrength)[4][3] = NULL;

    frequencyGhz = frequencyM / 1.0e3;

    if (meterialConst == TERRAIN::CONCRETE)
    {
        strength = &concrete;
        imagP = calculationPermittivity(frequencyGhz,
                                        &realP,
                                        *strength);
        *permittivityReal = realP;

    }
    else if (meterialConst == TERRAIN::LIGHTWEIGHTCONCRETE)
    {
        *permittivityReal = 2.0;
        imagP = -0.5;
    }
    else if (meterialConst == TERRAIN::SYNTHETICRESIN)
    {
        strength = &syntheticResin;
        imagP = calculationPermittivity(frequencyGhz,
                                        &realP,
                                        *strength);
        *permittivityReal = realP;
    }
    else if (meterialConst == TERRAIN::PLASTERBOARD)
    {
        estrength = &plasterboard;

        imagP = calculationPermittivity(frequencyGhz,
                                        &realP,
                                        *estrength);
        *permittivityReal = realP;
    }
    else if (meterialConst == TERRAIN::ROCKWOOL)
    {
        estrength = &rockWool;

        imagP = calculationPermittivity(frequencyGhz,
                                        &realP,
                                        *estrength);
        *permittivityReal = realP;
    }
    else if (meterialConst == TERRAIN::GLASS)
    {
        imagP = getGlassPermittivity(frequencyGhz, 
                                     &realP);
 
        *permittivityReal = realP;
    }
    else if (meterialConst == TERRAIN::FIBERGLASS)
    {
        *permittivityReal = 1.2;
        imagP = -0.1;
    }
    else if (meterialConst == TERRAIN::WOOD)
    {
        estrength = &rockWool;

        imagP = calculationPermittivity(frequencyGhz,
                                        &realP,
                                        *estrength);
        *permittivityReal = realP;
    }
    else if (meterialConst == TERRAIN::METAL)
    {
        strength = &metal;
        imagP = calculationPermittivity(frequencyGhz,
                                        &realP,
                                        *strength);
        *permittivityReal = realP;
    }
    else if (meterialConst == TERRAIN::NA)  //Same as PLASTERBOARD
    {
        estrength = &plasterboard;

        imagP = calculationPermittivity(frequencyGhz,
                                        &realP,
                                        *estrength);
        *permittivityReal = realP;
    }
    else
    {
        ERROR_ReportError(" The supported construction materials are:\n"
                          "     CONCRETE,\n"
                          "     LIGHTWEIGHT CONCRETE\n"
                          "     SYNTHETICRESIN\n"
                          "     PLASTERBOARD\n"
                          "     ROCKWOOL\n"
                          "     GLASS\n"
                          "     FIBERGLASS\n"
                          "     WOOD\n"
                          "     METAL\n");
    }

    if (DEBUG) {

        printf("\n Complex Permittivity:\n"
               "    Real = %f\n"
               "    imag  = %f\n", *permittivityReal, imagP);

        fflush(stdout);
    }

    return imagP;
}



double PathlossITU_Rindoor(TerrainData* terrainData,
                           double       distance, // in meters
                           double       frequencyMhz,
                           Coordinates  fromPosition,
                           Coordinates  toPosition,
                           SelectionData* urbanStats)
{

    double realpermitt;
    double imagpermitt;
    double realPermittivityFloor;
    double wavelength;
    double penetrationLossThin;
    double penetrationLossThick;
    double penetrationLossFloor;
    double loss;
    double freqLoss;
    double lossDis;
    double imagpermittFloor;
    int k;
    double coefficientLoss;
    double thickImagpermitt;
    double thickRealpermitt;

    UrbanIndoorPathProperties* indoorProperties =
        terrainData->getIndoorPathProperties(&fromPosition,
                                             &toPosition);
    indoorProperties->countWalls();

    if (DEBUG) {
        printf("in ITU_R Indoor, distance=%f\n", distance);
        indoorProperties->print();
    }

    wavelength = SPEED_OF_LIGHT /(frequencyMhz * 1.0e6);
    loss = 0.0;
    freqLoss = 0.0;
    k = 0;

    imagpermitt =
        calculationPermitt(frequencyMhz,
                           &realpermitt,
                           indoorProperties->getMostCommonThinWallMaterial());

    penetrationLossThin =
            calculationPenetrationLossFactor(
                                 realpermitt,
                                 imagpermitt,
                                 wavelength,
                                 indoorProperties->getAvgThinWallThickness());

    thickImagpermitt =
        calculationPermitt(frequencyMhz,
                           &thickRealpermitt,
                           indoorProperties->getMostCommonThickWallMaterial());

    penetrationLossThick =
            calculationPenetrationLossFactor(
                                 thickRealpermitt,
                                 thickImagpermitt,
                                 wavelength,
                                 indoorProperties->getAvgThickWallThickness());

    imagpermittFloor =
        calculationPermitt(frequencyMhz,
                           &realPermittivityFloor,
                           indoorProperties->getMostCommonFloorMaterial());

    penetrationLossFloor =
            calculationPenetrationLossFactor(
                                 realPermittivityFloor,
                                 imagpermittFloor,
                                 wavelength,
                                 indoorProperties->getAvgFloorThickness());

    if (DEBUG) {
        printf("ITU_R indoor values %f, %f, %f, %f, %f, %f\n",
               imagpermitt,
               penetrationLossThin,
               thickImagpermitt,
               penetrationLossThick,
               imagpermittFloor,
               penetrationLossFloor);
              
    }

    double frequencyGhz = frequencyMhz / 1.0e3;

    while (frequencyGhz > lossCoefficients[k][0] )
    {
        k++;
    }

    if( k == 0)
    {
        k = 1;
    }

    coefficientLoss =
        lossCoefficients[k-1][1] + (lossCoefficients[k][1] -
        lossCoefficients[k-1][1]) * (frequencyGhz -
        lossCoefficients[k-1][0]) /
        (lossCoefficients[k][0] - lossCoefficients[k-1][0]);

    if(frequencyMhz > 0.0)
    {
        freqLoss = 2.0 * IN_DB(frequencyMhz);
    }

    if(distance > 0.0)
    {
        lossDis = coefficientLoss * log10(distance);

        if(lossDis < 0.0)
        {
          lossDis = 0.0;
        }

        loss =
            freqLoss + lossDis +
            penetrationLossThin * indoorProperties->getNumThinWalls() +
            penetrationLossThick * indoorProperties->getNumThickWalls() +
            penetrationLossFloor * indoorProperties->getNumFloors() - 28.0;
    }

    if (loss < 0.0)
    {
        loss = 0.0;
    }

    if (DEBUG) {
        printf("\n Indoor model:\n"
               "    Distance = %f,\n"
               "    Floors/thin/thick = %d,%d,%d,\n"
               "    Wall permittivity (real) = %f,\n"
               "    Wall permittivity (imaginary) = %f,\n"
               "    Floor permittivity (real) = %f,\n"
               "    Floor permittivity (imaginary) = %f,\n"
               "    Thin wall loss = %f,\n"
               "    Thick wall loss = %f,\n"
               "    Floor loss = %f,\n"
               "    Indoor loss = %f\n",
               distance, indoorProperties->getNumFloors(),
               indoorProperties->getNumThinWalls(),
               indoorProperties->getNumThickWalls(),
               realpermitt, imagpermitt, realPermittivityFloor,
               imagpermittFloor, penetrationLossThin,
               penetrationLossThick, penetrationLossFloor,
               loss);

        fflush(stdout);
    }

    urbanStats->numWalls  += 
        (indoorProperties->getNumThinWalls() + 
         indoorProperties->getNumThickWalls());
    urbanStats->numFloors += 
         (int)indoorProperties->getNumFloors();

    delete indoorProperties;

    return loss;
}

