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

#include "prop_cost_wi.h"

#define DEBUG 0

// /**
// FUNCTION :: COST231_WIInitialize
// LAYER :: Physical Layer.
// PURPOSE :: Initialize COST 231 Walfish-Ikegami Propagation Models
// PARAMETERS ::
// + propChannel: PropChannel* : Pointer to Propagation Channel data-struct
// + channelIndex: Int: Channel Index
// + nodeInput: NodeInput*: Pointer to Node Input
// RETURN :: void : NULL
//
// **/
void COST231_WIInitialize( PropChannel *propChannel,
                           int channelIndex,
                           const NodeInput *nodeInput)
{
    double roofHeight;
    double streetWidth;
    double buildingSeparation;
    BOOL wasFound;
    char propagationEnvironment[MAX_STRING_LENGTH];

    PropProfile* propProfile = propChannel->profile;


    IO_ReadStringInstance(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "PROPAGATION-COST231-WALFISH-IKEGAMI-ENVIRONMENT",
        channelIndex,
        (channelIndex == 0),
        &wasFound,
        propagationEnvironment);

    if (wasFound) {
        if (strcmp(propagationEnvironment, "METROPOLITAN") == 0) {
            propProfile->propagationEnvironment = METROPOLITAN;

        }
        else if (strcmp(propagationEnvironment, "URBAN") == 0) {
            propProfile->propagationEnvironment = URBAN;

        }
        else {
            ERROR_ReportError("Unrecognized propagation environment\n.");
        }
    }
    else {
        propProfile->propagationEnvironment =
            DEFAULT_PROPAGATION_COST231_WI_ENVIRONMENT;
    }

    IO_ReadDoubleInstance(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "PROPAGATION-ROOF-HEIGHT",
        channelIndex,
        (channelIndex == 0),
        &wasFound,
        &roofHeight);

    if (wasFound) {

        if (roofHeight < 0.0) {
            ERROR_ReportError("PROPAGATION-ROOF-HEIGHT should be"
                              " larger than 0.\n");
        }

        UrbanProp_SetAvgHRoof(propProfile, roofHeight);
    }

    else {
        UrbanProp_SetAvgHRoof(propProfile, DEFAULT_ROOF_HEIGHT);
    }

    IO_ReadDoubleInstance(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "PROPAGATION-STREET-WIDTH",
        channelIndex,
        (channelIndex == 0),
        &wasFound,
        &streetWidth);

    if (wasFound) {

        if (streetWidth < 0.0) {
            ERROR_ReportError("PROPAGATION-STREET-WIDTH must be"
                              " larger than 0.0\n");
        }

        UrbanProp_SetStreetWidth(propProfile, streetWidth );
    }

    else {
        UrbanProp_SetStreetWidth(propProfile,  DEFAULT_STREET_WIDTH);
    }

    IO_ReadDoubleInstance(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "PROPAGATION-BUILDING-SEPARATION",
        channelIndex,
        (channelIndex == 0),
        &wasFound,
        &buildingSeparation);

    if (wasFound) {

        if (buildingSeparation < 0.0) {

            ERROR_ReportError("PROPAGATION-BUILDING-SEPARATION "
                              "should be larger than 0.0\n");
        }

        UrbanProp_SetBuildingSep(propProfile, buildingSeparation);
    }
    else {
        UrbanProp_SetBuildingSep(propProfile, DEFAULT_BUILDING_SEPARATION);
    }

    if (DEBUG) {
        printf("COST-WI INit Complete \n");
    }

    return;
}


// /**
// FUNCTION :: PathlossCOST231_WI
// LAYER :: Physical Layer.
// PURPOSE :: Calculate Propagation Loss due to COST Walfish Ikegami model
// PARAMETERS ::
// + node: Node* : Pointer to Node structure
// + distance: double : Tx-Rx distance
// + wavelength: double : Wavelength
// + txAntennaHeight: float : Transmitter Antenna height
// + rxAntennaHeight: float : Receiver Antenna height
// + propProfile: PropProfile * : Propagation profile
// RETURN :: double : pathloss in dB
//
// **/
double PathlossCOST231_WI(Node *node,
                          double distance,
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

    if (DEBUG) {
        printf("COST-WI Compute Pathloss:  PathlossCOST231_WI\n");
    }

    if ( txAntennaHeight >= rxAntennaHeight )
    {
        h1 = (double)txAntennaHeight;
        h2 = (double)rxAntennaHeight;
    }
    else {
        h1 = (double)rxAntennaHeight;
        h2 = (double)txAntennaHeight;
    }

    frequencyMhz = 1.0e-6 * SPEED_OF_LIGHT / waveLength;
    distanceKm = distance * 1.0e-3;

    if (!((frequencyMhz > 800.0) && (frequencyMhz <2000.0))) {

        ERROR_ReportWarning("WARNING: Frequency not in "
                            "recommended range [800:2000] Mhz"
                            " for use in COST-Walfish Ikegami Model \n");
    }


// Orientation always defaulted to 90 degrees: Ref: Principles of Mobile
// Communication by G.L.Stuber

    double  orientation = 90.0;
    //UrbanProp_SetOrientation(propProfile, Orientation);

// Rooftop to Street loss + Multi screen diffraction loss if
// NumOfBuildingsInPath > 1; ONLY Rooftop to Street loss otherwise
    int NumOfBuildingsInPath = 2;

    if (distanceKm == 0.0)
    {

        pathloss_dB = 0.0;
    }
    else {

        double aveRoofHeight;
        double streetWidth;
        double buildingSeparation ;
        PropagationEnvironment environment;

        aveRoofHeight = UrbanProp_GetAvgHRoof(propProfile);
        streetWidth = UrbanProp_GetStreetWidth(propProfile);
        buildingSeparation = UrbanProp_GetBuildingSep(propProfile);
        environment = propProfile->propagationEnvironment;

        pathloss_dB = COST231_WI_NLoS (NumOfBuildingsInPath,
                                       distanceKm,
                                       frequencyMhz,
                                       h1,
                                       h2,
                                       orientation,
                                       aveRoofHeight,
                                       streetWidth,
                                       buildingSeparation,
                                       environment);
    }

    if (DEBUG) {
        printf("COST_WI pathloss = %f \n", pathloss_dB);
    }

    return pathloss_dB;
}


/* Loss in line-of-sight conditions */
double COST231_WI_LoS(double distanceKm,
                      double frequencyMhz)
{
    assert(frequencyMhz > 0);

    double pathloss_dB = 42.6
                         + 26.0 * log10(distanceKm)
                         + 20.0 * log10(frequencyMhz);

    return pathloss_dB;
}

/* Rooftop to street propagation loss */
double COST231_WI_RooftoStreetLoss( double frequencyMhz,
                                    double incidentAngle,
                                    double h_ms,
                                    double roofH,
                                    double streetW)
{
    double streetDiffractionLoss;
    double lcri = 0.0;

    assert((incidentAngle >= 0.0 ) && (incidentAngle <= 90.0));

    if ((incidentAngle >= 0.0 )&& (incidentAngle < 35.0))
    {

        lcri = -10.0 + 0.354 * incidentAngle;
    }
    else if ((incidentAngle >= 35.0 )&& (incidentAngle < 55.0))
    {

        lcri = 2.5 + 0.075 * (incidentAngle - 35.0);
    }
    else if ((incidentAngle >= 55.0 )&& (incidentAngle <= 90.0))
    {

        lcri = 4.0 - 0.114 * (incidentAngle - 55.0);
    }

    if (DEBUG) {
        printf("R-to-S: Roof Height, Angle, lcri = %f, %f, %f\n",
               roofH, incidentAngle, lcri);
    }

    double val2 = roofH - h_ms;

    streetDiffractionLoss = lcri - 16.9 ;

    if(streetW > 0.0)
    {
        streetDiffractionLoss += (- 10.0 * log10(streetW));
    }

    if(frequencyMhz > 0.0)
    {
        streetDiffractionLoss += 10.0 * log10(frequencyMhz);
    }

    if(frequencyMhz > 0.0)
    {
        streetDiffractionLoss += 20.0 * log10(val2);
    }

    return streetDiffractionLoss;
}

/* Multi-screen propagation loss */
double COST231_WI_MultiScreenLoss( double distanceKm,
                                   double frequencyMhz,
                                   double h_bs,
                                   double AvgHRoof,
                                   double buildingseparation,
                                   PropagationEnvironment env)
{
    double multiscreenDiffractionLoss;
    double Lbsh;
    double ka;
    double kd;
    double kf = 0.0;
    double arg;

    if (h_bs > AvgHRoof)
    {
        arg = 1.0 + h_bs - AvgHRoof;
        Lbsh = -18.0 * log10(arg);
        ka = 54.0;
        kd = 18.0;
    }
    else {
        Lbsh = 0.0;
        kd = 18.0 - 15.0 * (h_bs - AvgHRoof) /AvgHRoof;

        if (distanceKm < 0.5) {

            ka = 54.0 - 0.8 * (h_bs - AvgHRoof) *
                 distanceKm / 0.5;
        }
        else {

            ka = 54.0 - 0.8 * (h_bs - AvgHRoof);
        }
    }

    if (env == URBAN) {

        kf = -4.0 + 0.7 * (frequencyMhz/925.0 - 1.0);
    }
    else if (env == METROPOLITAN) {

        kf = -4.0 + 1.5 * (frequencyMhz/925.0 - 1.0);
    }

    multiscreenDiffractionLoss = Lbsh + ka;

    if(distanceKm > 0.0)
    {
       multiscreenDiffractionLoss += kd * log10(distanceKm);
    }

    if(frequencyMhz > 0.0)
    {
       multiscreenDiffractionLoss += kf * log10(frequencyMhz);
    }

    if(buildingseparation > 0.0)
    {
       multiscreenDiffractionLoss += (- 9.0 * log10(buildingseparation));
    }

    return multiscreenDiffractionLoss;
}


/* Loss under non-line-of-sight conditions */
double COST231_WI_NLoS (int NumOfBuildingsInPath,
                        double distanceKm,
                        double frequencyMhz,
                        double h1,
                        double h2,
                        double orientation,
                        double roofHeight,
                        double streetWidth,
                        double buildingSeparation,
                        PropagationEnvironment environment)
{
    double freeSpaceLoss = 0.0;
    double streetDiffractionLoss = 0.0;
    double pathloss_dB = 0.0;
    double multiscreenDiffractionLoss = 0.0;

    if (NumOfBuildingsInPath < 1) {

        pathloss_dB = COST231_WI_LoS(distanceKm, frequencyMhz);
    }
    else {

        if (DEBUG) {
            printf("COST231_WI_NLoS: \n"
                   "  Distance in Km = %f\n"
                   "  Frequency in MHz = %f\n"
                   "  Bs antenna height = %f\n"
                   "  Ms antenna height = %f\n"
                   "  Incident angle = %f\n",
                   distanceKm, frequencyMhz, h1, h2, orientation);
        }

        streetDiffractionLoss = COST231_WI_RooftoStreetLoss(
                                    frequencyMhz,
                                    orientation,
                                    h2,
                                    roofHeight,
                                    streetWidth);

        if (DEBUG) {
            printf("COST231_WI_NLoS: NumOfBuildingsInPath = %d\n",
                   NumOfBuildingsInPath);
        }

        multiscreenDiffractionLoss = COST231_WI_MultiScreenLoss(
                                         distanceKm,
                                         frequencyMhz,
                                         h1,
                                         roofHeight,
                                         buildingSeparation,
                                         environment);

        freeSpaceLoss =
            32.4 + 20.0 * log10(distanceKm) + 20.0 * log10(frequencyMhz);

        if ((streetDiffractionLoss + multiscreenDiffractionLoss) > 0.0) {

            pathloss_dB = freeSpaceLoss + streetDiffractionLoss
                          + multiscreenDiffractionLoss;
        }
        else {
            pathloss_dB = freeSpaceLoss;
        }

        if (DEBUG) {
            printf("COST_WI freespace, roof-to-street, multiscr loss ="
                   " %f, %f, %f \n", freeSpaceLoss,
                   streetDiffractionLoss, multiscreenDiffractionLoss);
        }
    }

    return pathloss_dB;

}


