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

#ifndef __PROP_ITU_R_H__
# define __PROP_ITU_R_H__

#include "propagation.h"
#include "prop_data.h"

#define HORIZONTAL 0
#define VERTICAL 1

#define DEFAULT__HUMIDITY   50.0
#define DEFAULT__TEMPERATURE 25.0
#define DEFAULT__RAIN__INTENSITY 0.0

double PathlossItu_R_los(double  distance, // in meters
                         double  wavelength,  // in meters
                         double  txAntennaHeight,  // in meters
                         double  rxAntennaHeight, // in meters
                         Coordinates fromPosition,
                         Coordinates toPosition);

double PathlossITU_Rindoor(TerrainData* terrainData,
                           double       distance, // in meters
                           double       frequencyMhz,
                           Coordinates  fromPosition,
                           Coordinates  toPosition,
                           SelectionData* urbanStats);

double PathlossItu_R_VHFUHF(double  distanceKm,
                            double  frequencyMHz,
                            double  h_bs,    // in meters
                            double  h_ms,    // in meters
                            double  streetWidth, //in meters
                            double  avgRoofHeight, // in meters
                            BOOL isLOS);

double PathlossItu_R_UHF(double  distance, // in meters
                         double  wavelength,  // in meters
                         BOOL    isLOS,
                         double  locationPercentage); // percentage


double Prop_AtmosphericAttenuation(double distance,   // in km
                                   double Frequency,  // in GHz
                                   double humidity,
                                   double temperature);

double Prop_RainAttenuation(double Frequency,   //in GHz
                            double distance,    //in km
                            int polarization,
                            double rainintensity);

double PathlossITU_R_NLoS1 (
                        double distanceKm,
                        double frequencyMhz,
                        double h1,
                        double h2,
                        double streetWidth, //in meters
                        double avgRoofHeight,
                        double incidentAngle,
                        double buildingSeperation,
                        double buildingExtendDistance,
                        PropagationEnvironment  proEnvironment);

double PathlossItu_R_Nlos2LowBand(double  txStreetWidth, // in meters
                         double  rxStreetWidth, // in meters
                         double  wavelength,  // in meters
                         double  txStreetCrossingDistance,  // in meters
                         double  rxStreetCrossingDistance, // in meters
                         double  cornerAngle); // in degree

double PathlossItu_R_Nlos2HighBand(double  txStreetWidth, // in meters
                         double  rxStreetWidth, // in meters
                         double  wavelength,  // in meters
                         double  txStreetCrossingDistance,  // in meters
                         double  rxStreetCrossingDistance, // in meters
                         double  cornerAngle,            //in degrees
                         double  txAntennaHeight,  // in meters
                         double  rxAntennaHeight, // in meters
                         Coordinates fromPosition,
                         Coordinates toPosition);

#endif
