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

#include "prop_urban.h"
#include "prop_foliage.h"
#include "prop_itm.h"

#ifdef ADDON_DB
#include "db.h"
#include "dbapi.h"
#endif

#define DEBUG 0

#define FRESNEL_ZONE_CLEARANCE_RATIO 1.0;

static const std::string urbanModelNames[15] = {
    "FREESPACE",
    "TWORAY",
    "ITU_R_LOS",
    "ITU_R_NLOS1",
    "ITU_R_NLOS2",
    "ITU_R_UHF_LOS",
    "ITU_R_UHF_NLOS",
    "ITU_R_VHFUHF",
    "URBAN_COST_WI_LoS",
    "URBAN_COST_WI_NLoS",
    "URBAN_STREET_MICROCELL_LoS",
    "URBAN_STREET_MICROCELL_NLoS",
    "URBAN_STREET_M_TO_M",
    "COST231_INDOOR",
    "ITU_R_INDOOR"
};

static
void FindSWandNE(const int coordinateSystem,
                 const Coordinates p1,
                 const Coordinates p2,
                 Coordinates* sw,
                 Coordinates* ne) {
    if (coordinateSystem == CARTESIAN) {
        sw->cartesian.x = MIN(p1.cartesian.x, p2.cartesian.x);
        sw->cartesian.y = MIN(p1.cartesian.y, p2.cartesian.y);
        ne->cartesian.x = MAX(p1.cartesian.x, p2.cartesian.x);
        ne->cartesian.y = MAX(p1.cartesian.y, p2.cartesian.y);
    }
#ifdef MGRS_ADDON
    else if (coordinateSystem == MGRS)
    {
        sw->common.c1 = MIN(p1.common.c1, p2.common.c1);
        sw->common.c2 = MIN(p1.common.c2, p2.common.c2);
        ne->common.c1 = MAX(p1.common.c1, p2.common.c1);
        ne->common.c2 = MAX(p1.common.c2, p2.common.c2);
    }
#endif // MGRS_ADDON
    else { // coordinateSystem == LATLONALT
        sw->latlonalt.latitude = MIN(p1.latlonalt.latitude,
                                     p2.latlonalt.latitude);
        ne->latlonalt.latitude = MAX(p1.latlonalt.latitude,
                                     p2.latlonalt.latitude);
        // this isn't entirely right.  needs to be adjusted for crossing the
        // date line.
        sw->latlonalt.longitude = MIN(p1.latlonalt.longitude,
                                      p2.latlonalt.longitude);
        ne->latlonalt.longitude = MAX(p1.latlonalt.longitude,
                                      p2.latlonalt.longitude);
        // if ne - sw > 180, then swap them.
    }
}

static
bool GroundClearance(const double distance,
                     const double wavelength,
                     const double txAntennaHeight,
                     const double rxAntennaHeight)
{
    double refDistance = (4.0 * PI * txAntennaHeight * rxAntennaHeight) / wavelength;

    if (DEBUG) 
    {
        printf("in ground clearance, ref distance is %f, distance is %f\n",
               refDistance, distance);
    }

    return (distance < refDistance);
}

static
double PathlossCost231Indoor(TerrainData* terrainData,
                             double       distance, // in meters
                             double       wavelength,
                             Coordinates  fromPosition,
                             Coordinates  toPosition,
                             SelectionData* urbanStats,
                             bool         ignoreFreespaceComponent = false) {

    // COST231 Indoor model.  Good for 800MHz to 1900MHz.
    // L = Lfs + 37 + 3.4 kw1 + 6.9 kw2 + 18.3 n((n+2)/(n+1)-0.46)
    // L   : attenuation in dB
    // Lfs :  free space loss in dB
    // n   :  number of traversed floors (reinforced concrete,
    //        but not thicker than 30 cm)
    // kw1 :  number of light internal walls (e.g. plaster board), windows,
    //        etc.
    // kw2 :  number of concrete or brick internal walls

    int    thinWalls;
    int    thickWalls;
    double freespaceLoss = 0.0;
    double numFloors;
    double exponent;
    double loss;

    UrbanIndoorPathProperties* indoorProperties =
        terrainData->getIndoorPathProperties(&fromPosition,
                                   &toPosition);
    indoorProperties->countWalls(); // gets counts of thin and thick walls

    thickWalls = indoorProperties->getNumThickWalls();
    thinWalls  = indoorProperties->getNumThinWalls();
    numFloors  = (double) indoorProperties->getNumFloors();
    exponent   = ((numFloors + 2)/(numFloors + 1)) - 0.46;

    if (!ignoreFreespaceComponent) {
        freespaceLoss = PROP_PathlossFreeSpace(distance, wavelength);
    }

    loss = freespaceLoss + 37.0
        + (3.4 * thinWalls)
        + (6.9 * thickWalls)
        + (18.3 * pow(numFloors, exponent));

    if (DEBUG) 
    {
        printf("Indoor model: "
               "distance = %f, floors/thin/thick = %f,%d,%d, loss = %f\n",
               distance, numFloors,
               thinWalls, thickWalls,
               loss);
        fflush(stdout);
    }

    urbanStats->numWalls  += (thickWalls + thinWalls);
    urbanStats->numFloors += (int) numFloors;

    return loss;
}

// /**
// FUNCTION :: UrbanProp_Initialize
// LAYER :: Physical Layer.
// PURPOSE :: Initialize Urban Propagation Models
// PARAMETERS ::
// + propChannel: PropChannel* : Pointer to Propagation Channel data-struct
// + channelIndex: Int: Channel Index
// + nodeInput: NodeInput*: Pointer to Node Input
// RETURN :: void : NULL
//
// **/

void UrbanProp_Initialize(
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
        "PROPAGATION-URBAN-AUTOSELECT-ENVIRONMENT",
        channelIndex,
        (channelIndex == 0),
        &wasFound,
        propagationenvironment);

    if (wasFound) {
        if (strcmp(propagationenvironment, "METROPOLITAN") == 0) {
            propProfile->propagationEnvironment = METROPOLITAN;

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
            DEFAULT_URBAN_PROPAGATION_ENVIRONMENT;
    }

    if (DEBUG) 
    {
        printf("URBAN PROP Init Complete \n");
    }

    return;
}


// /**
// FUNCTION :: Pathloss_UrbanOutdoor
// LAYER :: Physical Layer.
// PURPOSE :: Calculate Urban Propagation Pathloss
//            by calling environment specific model
// PARAMETERS ::
// + node: Node* : Pointer to Node
// + distance: double: distance between transmitter and receiver
// + wavelength: double: wavelength
// + txPosition: Coordinates: Transmitter position
// + rxPosition: Coordinates: Receiver position
// + txAntennaHeight: float: Transmitter Antenna Height
// + rxAntennaHeight: float: Receiver Antenna Height
//
// RETURN :: double : Pathloss value in dB
//
// **/
double Pathloss_UrbanOutdoor(const Node *node,
                             const double distance,
                             const double wavelength,
                             Coordinates txPosition,
                             Coordinates rxPosition,
                             const float txAntennaHeight,
                             const float rxAntennaHeight,
                             SelectionData* urbanStats)
{
    TerrainData* terrainData = NODE_GetTerrainPtr(const_cast<Node*>(node));

    double frequencyMhz;
    double distanceKm;

    double txHeightAboveGround;
    double rxHeightAboveGround;
    double maxHeightAboveGround;
    double minHeightAboveGround;
    double maxAntennaHeight;
    double minAntennaHeight;

    double pathloss_dB = 0.0;

    double maxFresnelZone1Width;

    bool isFreeSpace = false;
    bool isLoS       = false;
    bool isCanyon1   = false;
    bool isCanyon2   = false;

    // The adjusted coordinates include the antenna height.
    Coordinates adjustedTxPosition = txPosition;
    Coordinates adjustedRxPosition = rxPosition;

    // These are used to calculate the ground level below each node.
    Coordinates txGroundPosition = txPosition;
    Coordinates rxGroundPosition = rxPosition;

    UrbanModelType urbanModelType;
    UrbanPathProperties* pathProperties;
    TerrainRegion* terrainRegion = NULL;

    TERRAIN_SetToGroundLevel(terrainData, &txGroundPosition);
    TERRAIN_SetToGroundLevel(terrainData, &rxGroundPosition);

    txHeightAboveGround = MAX(0.0, (txAntennaHeight +
                                    txPosition.latlonalt.altitude -
                                    txGroundPosition.latlonalt.altitude));
    rxHeightAboveGround = MAX(0.0, (rxAntennaHeight +
                                    rxPosition.latlonalt.altitude -
                                    rxGroundPosition.latlonalt.altitude));

    maxAntennaHeight     = MAX(txAntennaHeight, rxAntennaHeight);
    minAntennaHeight     = MIN(txAntennaHeight, rxAntennaHeight);
    maxHeightAboveGround = MAX(txHeightAboveGround, rxHeightAboveGround);
    minHeightAboveGround = MIN(txHeightAboveGround, rxHeightAboveGround);

    frequencyMhz = 1.0e-6 * SPEED_OF_LIGHT / wavelength;
    distanceKm = distance * 1.0e-3;
    maxFresnelZone1Width
        = 0.5 * sqrt(wavelength * distance) * FRESNEL_ZONE_CLEARANCE_RATIO;

    if (DEBUG) 
    {
        printf("antenna heights (tx, rx) = %f, %f\n",
               txAntennaHeight, rxAntennaHeight);
        printf("antenna heights above ground (tx, rx) = %f, %f\n",
               txHeightAboveGround, rxHeightAboveGround);
    }
    if (DEBUG) 
    {
        printf("dist = %f, freq = %f, fresnelWidth = %f\n",
               distance, frequencyMhz, maxFresnelZone1Width);
    }

    // Add antenna height before getting path properties.
    adjustedTxPosition.common.c3 += txAntennaHeight;
    adjustedRxPosition.common.c3 += rxAntennaHeight;

    pathProperties = terrainData->getUrbanPathProperties(
        &adjustedTxPosition,
        &adjustedRxPosition,
        maxFresnelZone1Width);

    assert(pathProperties != NULL);

    if (DEBUG) 
    {
        pathProperties->print();
    }

    // Free space if no obstructions and Two-ray == free-space
    isFreeSpace = ((pathProperties->getNumObstructions() == 0) &&
                   GroundClearance(distance, wavelength,
                                   txHeightAboveGround,
                                   rxHeightAboveGround));

    if (!isFreeSpace) {
        // LoS might still be true if the direct line is unobstructed.
        UrbanPathProperties* simplePathProperties;
        simplePathProperties = terrainData->getUrbanPathProperties(
            &adjustedTxPosition,
            &adjustedRxPosition,
            0.0);
        isLoS = (simplePathProperties->getNumObstructions() == 0);
        delete simplePathProperties;

        isCanyon1 = terrainData->getIsCanyon(&txPosition, txHeightAboveGround);
        isCanyon2 = terrainData->getIsCanyon(&rxPosition, rxHeightAboveGround);
    }

    if (DEBUG) 
    {
        printf("isFreeSpace, isLoS, isCanyon1, isCanyon2="
               "%d,%d,%d,%d\n",
               isFreeSpace, isLoS,
               isCanyon1, isCanyon2);
        fflush(stdout);
    }

    //
    // Model Selection Logic
    //
    if (isFreeSpace) {
        urbanModelType = FREESPACE;
    }
    else // it's not freespace
    {
        if ((frequencyMhz >= 30.0) && (frequencyMhz < 300.0))
        {
            urbanModelType = ITU_R_VHFUHF;
        }
        else if ((frequencyMhz >= 300.0) && (frequencyMhz < 800.0))
        {
            if (isLoS)
            {
                urbanModelType = ITU_R_UHF_LOS;
            }
            else // LoS is false
            {
                urbanModelType = ITU_R_UHF_NLOS;
            } // isLoS
        }
        else if ((frequencyMhz >= 800.0) && (frequencyMhz < 2000.0))
        {
        if (isLoS)
        {
            if (isCanyon1 && isCanyon2)
            {
                urbanModelType = URBAN_STREET_MICROCELL_LoS;
            }
            else if (isCanyon1 || isCanyon2)
            {
                urbanModelType = URBAN_COST_WI_LoS;
            }
            else
            {
                urbanModelType = TWORAY;
            }
        }
        else // LoS is false
        {
            if (isCanyon1 && isCanyon2)
            {
                // Both nodes are in Urban Canyon
                    if (pathProperties->getNumBuildings() == 1)
                {
                    urbanModelType = URBAN_STREET_MICROCELL_NLoS;
                }
                else {
                    urbanModelType = URBAN_STREET_M_TO_M;
                }
            }
            else if (isCanyon1 || isCanyon2)
            {
                // Only one of the nodes is in Urban Canyon
                urbanModelType = URBAN_COST_WI_NLoS;
                }
                else
                {
                    // URBAN_COST_WI_NLoS model is used when neither of
                    // the node is in canyon and line of sight is false. 
                    urbanModelType = URBAN_COST_WI_NLoS;
                }
            } // isLoS
        } // is 800-2000MHz
        else
        { // frequency is < 30MHz or > 2000MHz
            if (isLoS)
            {
                if (isCanyon1 || isCanyon2)
                {
                    urbanModelType = ITU_R_LOS;
                    }
                else
                {
                    urbanModelType = TWORAY;
                    }
                }
            else // LoS is false
            {
                if (isCanyon1 || isCanyon2)
                {
                    if (pathProperties->getDoStreetsCross(
                            &txPosition,
                            &rxPosition))
                    {
                        urbanModelType =  ITU_R_NLOS2;
                    }
                    else
                    {
                        urbanModelType = ITU_R_NLOS1;
                }
            }
            else
            {
                // URBAN_COST_WI_NLoS model is used when neither of
                // the node is in canyon and line of sight is false.
                urbanModelType = URBAN_COST_WI_NLoS;
            }
        } // isLoS
        } // not in 30-2000MHz
    } // isFreeSpace

    urbanStats->freeSpace  = isFreeSpace;
    urbanStats->LoS        = isLoS;
    urbanStats->txInCanyon = isCanyon1;
    urbanStats->rxInCanyon = isCanyon2;
    urbanStats->modelSelected += urbanModelNames[(int)urbanModelType];

    if (DEBUG)
    {
        UrbanProp_PrintModelInfo(isFreeSpace,
                                 isLoS,
                                 isCanyon1,
                                 isCanyon2,
                                 urbanModelType);
    }

    PropagationEnvironment proEnvironment;

    if (pathProperties->getAvgBuildingHeight() > 20.0)
    {
        proEnvironment = METROPOLITAN;
    }
    else
    {
        proEnvironment = URBAN;
    }

    // Call appropriate models

    switch (urbanModelType)
    {
        case FREESPACE: {
            pathloss_dB = PROP_PathlossFreeSpace(distance, wavelength);
            break;
        }

        case TWORAY: {
            pathloss_dB = PROP_PathlossTwoRay(
                              distance,
                              wavelength,
                (float)maxHeightAboveGround,
                (float)minHeightAboveGround);
            break;
        }

        case URBAN_COST_WI_LoS: {
            pathloss_dB = COST231_WI_LoS(distanceKm, frequencyMhz);

            break;
        }

        case URBAN_COST_WI_NLoS: {

            pathloss_dB = COST231_WI_NLoS(
                pathProperties->getNumBuildings(),
                                          distanceKm,
                                          frequencyMhz,
                maxHeightAboveGround,
                minHeightAboveGround,
                pathProperties->getRelativeOrientation(),
                pathProperties->getAvgBuildingHeight(),
                pathProperties->getAvgStreetWidth(),
                pathProperties->getAvgBuildingSeparation(),
                proEnvironment);

            break;
        }

        case URBAN_STREET_MICROCELL_LoS: {

            pathloss_dB = Pathloss_StreetMicrocell_LoS(
                maxHeightAboveGround,
                minHeightAboveGround,
                                                       wavelength,
                                                       distance);
            break;
        }

        case URBAN_STREET_MICROCELL_NLoS: {

            pathloss_dB = Pathloss_StreetMicrocell_NLoS(
                maxAntennaHeight,
                minAntennaHeight,
                                                        wavelength,
                pathProperties->getSrcDistanceToBuilding(),
                pathProperties->getDestDistanceToBuilding(),
                pathProperties->getDistanceThroughBuilding(0));

            break;
        }

        case URBAN_STREET_M_TO_M: {

            pathloss_dB = Pathloss_Street_M_to_M(
                pathProperties->getNumBuildings(),
                maxAntennaHeight,
                minAntennaHeight,
                pathProperties->getAvgBuildingHeight(),
                pathProperties->getAvgStreetWidth(),
                                                 distance,
                                                 wavelength);
            break;
        }

        case ITU_R_VHFUHF: { // applies to frequency range 30 - 300 MHz

            double distanceKm = distance / 1000.0;

            pathloss_dB =
                PathlossItu_R_VHFUHF(distanceKm,
                                     frequencyMhz,
                                     maxAntennaHeight,    // in meters
                                     minAntennaHeight,    // in meters
                                     pathProperties->getAvgStreetWidth(),
                                     pathProperties->getAvgBuildingHeight(),
                                     isLoS);
            break;
        }

        case ITU_R_UHF_LOS: { // frquency range 300 - 800 MHz

            Coordinates sw, ne;
            FindSWandNE(terrainData->getCoordinateSystem(),
                        txPosition, rxPosition, &sw, &ne);
            terrainRegion = new TerrainRegion(terrainData->getCoordinateSystem(),
                                              &sw, &ne);
            UrbanTerrainData* urbanData = terrainData->m_urbanData;
            urbanData->populateRegionData(terrainRegion, TerrainRegion::SIMPLE);

            double locationPercentage = terrainRegion->getBuildingCoverage();

            pathloss_dB =
                PathlossItu_R_UHF(distance, // in meters
                                  wavelength,  // in meters
                                  isLoS, //
                                  locationPercentage); // percentage
            break;
        }

        case ITU_R_UHF_NLOS: { // frquency range 300 - 800 MHz

            Coordinates sw, ne;
            FindSWandNE(terrainData->getCoordinateSystem(),
                        txPosition, rxPosition, &sw, &ne);
            terrainRegion = new TerrainRegion(terrainData->getCoordinateSystem(),
                                              &sw, &ne);
            UrbanTerrainData* urbanData = terrainData->m_urbanData;
            urbanData->populateRegionData(terrainRegion, TerrainRegion::SIMPLE);
            double locationPercentage = terrainRegion->getBuildingCoverage();

            pathloss_dB =
                PathlossItu_R_UHF(distance, // in meters
                                  wavelength,  // in meters
                                  isLoS, //
                                  locationPercentage); // percentage
            break;
        }

        case ITU_R_LOS: { // other frequency range
            pathloss_dB =  PathlossItu_R_los(distance, // in meters
                                             wavelength,  // in meters
                                             maxAntennaHeight,  // in meters
                                             minAntennaHeight, // in meters
                                             txPosition,
                                             rxPosition);
            break;
        }

        case ITU_R_NLOS1: {

            double streetWidth = pathProperties->getAvgStreetWidth();
            double avgBuildingSeparation =
                pathProperties->getAvgBuildingSeparation();
            double buildingExtendDistance =
                avgBuildingSeparation * pathProperties->getNumBuildings();
            double incidentangle = pathProperties->getRelativeOrientation();

            pathloss_dB =
                PathlossITU_R_NLoS1 (
                    distance / 1000.0, // in km
                    frequencyMhz,
                    maxAntennaHeight,
                    minAntennaHeight,
                    streetWidth, //in meters
                    pathProperties->getAvgBuildingHeight(),
                    incidentangle,
                    avgBuildingSeparation,
                    buildingExtendDistance,
                    proEnvironment);
            break;
        }

        case ITU_R_NLOS2: {

            double streetWidth = pathProperties->getAvgStreetWidth();
            double txStreetCrossingDistance =
                pathProperties->getSrcDistanceToIntersection();
            double rxStreetCrossingDistance =
                pathProperties->getDestDistanceToIntersection();
            double cornerAngle = pathProperties->getIntersectionAngle();

            pathloss_dB =
                PathlossItu_R_Nlos2HighBand(streetWidth, // in meters
                                            streetWidth, // in meters
                                            wavelength,  // in meters
                                            txStreetCrossingDistance,  // in meters
                                            rxStreetCrossingDistance, // in meters
                                            cornerAngle,
                                            maxAntennaHeight,  // in meters
                                            minAntennaHeight,
                                            txPosition,
                                            rxPosition);
            break;
        }

        default: {
            ERROR_ReportError("Invalid Urban Propagation model type "
                              "found \n");
            break;
        }

    }  //end switch statement

    if (DEBUG) 
    {
        printf("Outdoor PathLoss=%f\n", pathloss_dB);
    }

    delete pathProperties;

    return pathloss_dB;
}

// /**
// FUNCTION :: Pathloss_Foliage
// LAYER :: Physical Layer.
// PURPOSE :: Calculate Foliage Propagation Pathloss
//            by calling environment specific model
// PARAMETERS ::
// + node: Node* : Pointer to Node
// + distance: double : Distance between the transmitter and receiver
// + wavelength: double : wavelength
// + txAntennaHeight: float : Transmitter Antenna Height
// + rxAntennaHeight: float : Receiver Antenna Height
// + propProfile: PropProfile * : Pointer to propagation profile
// RETURN :: double : Pathloss value in dB
//
// **/
double Pathloss_Foliage(const Node *node,
                        const double distance,
                        const double wavelength,
                        Coordinates txPosition,
                        Coordinates rxPosition,
                        const float txAntennaHeight,
                        const float rxAntennaHeight,
                        PropProfile *propProfile)
{
    // Obtain foliage path loss as follows:
    // Multiply each segment length by its density, then pass it into the
    // foliage path loss model, giving the path loss for one segment.  Repeat
    // and sum for all segments, giving the total foliage path loss.

    TerrainData* terrainData = NODE_GetTerrainPtr(const_cast<Node*>(node));

    if (terrainData->m_urbanData == NULL) {
        // We can return 0.0 path loss, because we do not consider free
        // space loss in this function.  Really, this dependency should
        // have been checked during initialization.
        return 0.0;
    }

    // TODO:  Can optimize by skipping Fresnel checks for air-to-air
    // transmissions (src and dst above maximum foliage height + Fresnel
    // radius).

    double h1;
    double h2;
    double pathloss_dB = 0.0;
    double frequencyMhz = 1.0e-6 * SPEED_OF_LIGHT / wavelength;
    double maxFresnelZone1Width
        = 0.5 * sqrt(wavelength * distance) * FRESNEL_ZONE_CLEARANCE_RATIO;

    Coordinates adjustedTxPosition = txPosition;
    Coordinates adjustedRxPosition = rxPosition;

    UrbanPathProperties* pathProperties;

    if (txAntennaHeight >= rxAntennaHeight)
    {
        h1 = txAntennaHeight;
        h1 += MAX(0.0, txPosition.latlonalt.altitude);

        h2 = rxAntennaHeight;
        h2 += MAX(0.0, rxPosition.latlonalt.altitude);
    } else {
        h1 = rxAntennaHeight;
        h1 += MAX(0.0, rxPosition.latlonalt.altitude);

        h2 = txAntennaHeight;
        h2 += MAX(0.0, txPosition.latlonalt.altitude);
    }

    // Add antenna height before getting path properties.
    adjustedTxPosition.common.c3 += txAntennaHeight;
    adjustedRxPosition.common.c3 += rxAntennaHeight;

    pathProperties = terrainData->getUrbanPathProperties(
        &adjustedTxPosition,
        &adjustedRxPosition,
        maxFresnelZone1Width);

    int numFoliage = pathProperties->getNumFoliage();
    if (numFoliage == 0) {
        delete pathProperties;
        return 0.0;
    }

    if (numFoliage > 0)
    {
        int i;
        for (i = 0; i < numFoliage; i++)
        {
            pathloss_dB +=
                PROP_Foliage_Attenuation_Exponential_Decay_Model(
                    pathProperties->getDistanceThroughFoliage(i) *
                    pathProperties->getFoliageDensity(i),
                    frequencyMhz,
                    pathProperties->getFoliatedState(i));
        }
    }

    delete pathProperties;

    if (DEBUG) 
    {
        printf("Foliage: pathloss_dB = %.3f\n", pathloss_dB);
    }

    return pathloss_dB;
    }

// wavelength in meter
// distance in meter
// pathloss in dB
static double calculation_pathlossExponent(double wavelength,
                                           double distance,
                                           double pathloss_dB)
{
    double shiftconstant;
    double lossExponent = 2.0;

    if ((distance > 0.0) && (pathloss_dB > 0.0))
    {
        shiftconstant = 2.0 * IN_DB(4.0 * PI / wavelength);
        lossExponent = (pathloss_dB - shiftconstant) / IN_DB(distance);

        if (lossExponent < 1.0)
        {
            lossExponent = pathloss_dB / IN_DB(distance);

        }
    }

    return lossExponent;
}

double Pathloss_UrbanProp(const Node *node,
                          const NodeId txNodeId,
                          const NodeId rxNodeId,
                          const double wavelength,
                          const float txAntennaHeight,
                          const float rxAntennaHeight,
                          PropProfile *propProfile,
                          const PropPathProfile *pathProfile)
{
    // first check indoor, because if either is indoor we have to call
    // the indoor models.  There won't be line of sight unless they're in
    // the same room, and even though it won't be free space.

    // need to add indoor parameters to pathProfile.
    // also might need to add to CES to forward platform updates to all
    // partitions so that as nodes move the building handle is already
    // known.  Definite question about which is more efficient.

    SelectionData urbanStats;

    double pathloss_dB = 0.0;
    double distance    = pathProfile->distance;
    double frequencyMhz = 1.0e-6 * SPEED_OF_LIGHT / wavelength;

    TerrainData* terrainData = NODE_GetTerrainPtr(const_cast<Node*>(node));
    Coordinates fromPosition, toPosition;
    memcpy(&fromPosition, &pathProfile->fromPosition, sizeof(Coordinates));
    memcpy(&toPosition, &pathProfile->toPosition, sizeof(Coordinates));

    if (DEBUG) 
    {
        printf("URBAN-AUTOSELECT from (%f,%f,%2.3f) to (%f,%f,%2.3f)\n",
               fromPosition.latlonalt.latitude,
               fromPosition.latlonalt.longitude,
               fromPosition.latlonalt.altitude,
               toPosition.latlonalt.latitude,
               toPosition.latlonalt.longitude,
               toPosition.latlonalt.altitude);
    }
    // Check whether the nodes are indoors.
    bool fromIndoors = terrainData->isPositionIndoors(&fromPosition);
    bool toIndoors   = terrainData->isPositionIndoors(&toPosition);

    urbanStats.txNodeId  = txNodeId;
    urbanStats.rxNodeId  = rxNodeId;
    urbanStats.txPosition = fromPosition;
    urbanStats.rxPosition = toPosition;
    urbanStats.txIndoors = fromIndoors;
    urbanStats.rxIndoors = toIndoors;
    urbanStats.frequency = frequencyMhz;
    urbanStats.distance  = distance;
    urbanStats.modelSelected = "";
    
    if (fromIndoors || toIndoors) {
        // if either is indoors we need to apply an indoor model in some
        // form.
        urbanStats.modelSelected += "INDOOR/";
        if (fromIndoors && toIndoors) {
            // get and compare building handle
            if (terrainData->arePositionsInSameBuilding(&fromPosition,
                                                   &toPosition))
            {
                urbanStats.modelSelected += "INDOOR";
                if (DEBUG) 
                {
                    printf("INDOOR: same building\n");
                }

                if ((frequencyMhz > 800.0) && (frequencyMhz < 1900.0))
                {
                pathloss_dB =
                    PathlossCost231Indoor(terrainData,
                                          distance,
                                          wavelength,
                                          fromPosition,
                                              toPosition,
                                              &urbanStats);
                }
                else
                {

                    pathloss_dB = 
                        PathlossITU_Rindoor(terrainData,
                                            distance,
                                            frequencyMhz,
                                            fromPosition,
                                            toPosition,
                                            &urbanStats);

                }

                urbanStats.insideDistance1 = distance;
                urbanStats.insidePathloss1 = pathloss_dB;
            }
            else { // both in buildings, but different buildings
                // call line of sight function to count walls
                // here we assume there are no intervenening floors because
                // vertical distance between buildings will be covered in
                // the space between buildings.  (Probably need to adjust
                // that for buildings that are adjacent.)
                if (DEBUG) 
                {
                    printf("INDOOR: different buildings\n");
                }

                // apply COST 231 function piecewise
                // we have three segments, in each building, and between
                // buildings
                Coordinates edgeOfBuilding1;
                Coordinates edgeOfBuilding2;
                double      insideDistance1;
                double      insideDistance2;
                double      outsideDistance;
                double      indoorLoss1;
                double      indoorLoss2;
                double      outdoorLoss;
                double      foliageLoss;
                double      outdoorLossExponent;
                double      indoorLossExponent1;
                double      indoorLossExponent2;

                double totalFreespaceLoss = PROP_PathlossFreeSpace(distance,
                                            wavelength);
                terrainData->findEdgeOfBuilding(&fromPosition,
                                           &toPosition,
                                           &edgeOfBuilding1);
                terrainData->findEdgeOfBuilding(&toPosition,
                                           &fromPosition,
                                           &edgeOfBuilding2);
                COORD_CalcDistance(terrainData->getCoordinateSystem(),
                                   &fromPosition,
                                   &edgeOfBuilding1,
                                   &insideDistance1);
                COORD_CalcDistance(terrainData->getCoordinateSystem(),
                                   &edgeOfBuilding1,
                                   &edgeOfBuilding2,
                                   &outsideDistance);
                COORD_CalcDistance(terrainData->getCoordinateSystem(),
                                   &edgeOfBuilding2,
                                   &toPosition,
                                   &insideDistance2);

                urbanStats.outsideDistance = outsideDistance;
                urbanStats.insideDistance1 = insideDistance1;
                urbanStats.insideDistance2 = insideDistance2;

                if ((frequencyMhz > 800.0) && (frequencyMhz < 1900.0))
                {

                indoorLoss1 =
                    PathlossCost231Indoor(terrainData,
                                          insideDistance1,
                                          wavelength,
                                          fromPosition,
                                          edgeOfBuilding1,
                                              &urbanStats,
                                          true);
                indoorLoss2 =
                    PathlossCost231Indoor(terrainData,
                                          insideDistance2,
                                          wavelength,
                                          edgeOfBuilding2,
                                          toPosition,
                                              &urbanStats,
                                          true);
                }
                else
                {
                    indoorLoss1 = PathlossITU_Rindoor(terrainData,
                                        insideDistance1,
                                        frequencyMhz,
                                        fromPosition,
                                        edgeOfBuilding1,
                                        &urbanStats);

                    indoorLoss2 = PathlossITU_Rindoor(terrainData,
                                        insideDistance2,
                                        frequencyMhz,
                                        edgeOfBuilding2,
                                        toPosition,
                                        &urbanStats);

                }

                outdoorLoss =
                    Pathloss_UrbanOutdoor(node,
                                          outsideDistance,
                                          wavelength,
                                          edgeOfBuilding1,
                                          edgeOfBuilding2,
                                          txAntennaHeight,
                                          rxAntennaHeight,
                                          &urbanStats);

                outdoorLossExponent =
                    calculation_pathlossExponent(
                                        wavelength,
                                        outsideDistance,
                                        outdoorLoss);

                indoorLossExponent1 =
                    calculation_pathlossExponent(
                                        wavelength,
                                        insideDistance1,
                                        indoorLoss1);

                indoorLossExponent2 =
                    calculation_pathlossExponent(
                                        wavelength,
                                        insideDistance2,
                                        indoorLoss2);
                pathloss_dB =
                    2.0 * IN_DB(4.0 * PI / wavelength) +
                    outdoorLossExponent * IN_DB(distance) +
                    (indoorLossExponent1 - outdoorLossExponent) *
                    IN_DB(1+ distance / insideDistance1) +
                    (indoorLossExponent2 - indoorLossExponent1) *
                    IN_DB(1+ distance / insideDistance2);

                if (pathloss_dB >= ( outdoorLoss + indoorLoss1 + indoorLoss2))
                {
                    pathloss_dB = outdoorLoss + indoorLoss1 + indoorLoss2;
                }

                if ((pathloss_dB < outdoorLoss) ||
                   (pathloss_dB < indoorLoss1) ||
                   (pathloss_dB < indoorLoss2))
                {

                    pathloss_dB = MAX(MAX(outdoorLoss, indoorLoss1),indoorLoss2);
                }

                foliageLoss =
                    Pathloss_Foliage(node,
                                     outsideDistance,
                                     wavelength,
                                     edgeOfBuilding1,
                                     edgeOfBuilding2,
                                     txAntennaHeight,
                                     rxAntennaHeight,
                                     propProfile);

               if (DEBUG) 
               {
                    printf("INDOOR: Free/In/Out/In/Total/pathloss %f, %f,"
                           " %f, %f, %f, %f\n",
                           totalFreespaceLoss, indoorLoss1, outdoorLoss,
                           indoorLoss2, indoorLoss1 + indoorLoss2 +
                           outdoorLoss, pathloss_dB);
                }

                pathloss_dB = pathloss_dB + foliageLoss;

                urbanStats.outsidePathloss = outdoorLoss;
                urbanStats.insidePathloss1 = indoorLoss1;
                urbanStats.insidePathloss2 = indoorLoss2;
            }
        }
        else { // one indoor one outdoor
            // call line of sight function
            // again don't count floors
            // here we may have to combine the indoor model with an outdoor
            // model.  Would be useful to find the edge of the current building
            // so we can use that as the starting point for the outdoor model
            // and build a piece-wise loss.
            if (DEBUG) 
            {
                printf("INDOOR/OUTDOOR\n");
            }

            urbanStats.modelSelected += "OUTDOOR"; 

            // apply COST 231 function piecewise
            // two segments, one inside and one outside.
            Coordinates outsidePosition;
            Coordinates insidePosition;
            Coordinates edgeOfBuilding;
            double      insideDistance;
            double      outsideDistance;
            double      indoorLoss;
            double      outdoorLoss;
            double      foliageLoss;
            double outdoorLossExponent;
            double totalFreespaceLoss = PROP_PathlossFreeSpace(distance,
                                        wavelength);

            if (fromIndoors) {
                // fromPosition is inside
                insidePosition = fromPosition;
                outsidePosition = toPosition;
            }
            else
            {
                // toPosition is inside
                insidePosition = toPosition;
                outsidePosition = fromPosition;
            }
            terrainData->findEdgeOfBuilding(&insidePosition,
                                       &outsidePosition,
                                       &edgeOfBuilding);
            COORD_CalcDistance(terrainData->getCoordinateSystem(),
                               &insidePosition,
                               &edgeOfBuilding,
                               &insideDistance);
            COORD_CalcDistance(terrainData->getCoordinateSystem(),
                               &outsidePosition,
                               &edgeOfBuilding,
                               &outsideDistance);

            urbanStats.outsideDistance = outsideDistance;
            urbanStats.insideDistance1 = insideDistance;

            if ((frequencyMhz > 800.0) && (frequencyMhz < 1900.0))
            {
            indoorLoss =
                PathlossCost231Indoor(terrainData,
                                      insideDistance,
                                      wavelength,
                                      insidePosition,
                                      edgeOfBuilding,
                                          &urbanStats,
                                      true);
            }
            else
            {
                indoorLoss = PathlossITU_Rindoor(terrainData,
                                                 insideDistance,
                                                 frequencyMhz,
                                                 insidePosition,
                                                 edgeOfBuilding,
                                                 &urbanStats);
            }

            outdoorLoss =
                Pathloss_UrbanOutdoor(node,
                                      outsideDistance,
                                      wavelength,
                                      edgeOfBuilding,
                                      outsidePosition,
                                      txAntennaHeight,
                                      rxAntennaHeight,
                                      &urbanStats);

            outdoorLossExponent =
                    calculation_pathlossExponent(
                                        wavelength,
                                        outsideDistance,
                                        outdoorLoss);

            pathloss_dB =
                indoorLoss + outdoorLossExponent *
                IN_DB(distance / insideDistance);


            if (pathloss_dB >= ( outdoorLoss + indoorLoss))
            {
                pathloss_dB = outdoorLoss + indoorLoss;
            }

            if ((pathloss_dB < outdoorLoss) || (pathloss_dB < indoorLoss))
            {

                pathloss_dB = MAX(outdoorLoss, indoorLoss);
            }

            foliageLoss =
                Pathloss_Foliage(node,
                                 outsideDistance,
                                 wavelength,
                                 edgeOfBuilding,
                                 outsidePosition,
                                 txAntennaHeight,
                                 rxAntennaHeight,
                                 propProfile);

            if (DEBUG) 
            {
                printf("INDOOR/OUTDOOR: Free/In/Out/total/pathloss"
                       " %f, %f, %f %f %f\n",
                       totalFreespaceLoss, indoorLoss,
                       outdoorLoss, indoorLoss + outdoorLoss, pathloss_dB);
            }

            pathloss_dB = pathloss_dB + foliageLoss;
            urbanStats.outsidePathloss = outdoorLoss;
            urbanStats.insidePathloss1 = indoorLoss;
        }
    }
    else // both outdoors
    {
        if (DEBUG)
        {
            printf("OUTDOOR: both outdoors\n");
        }

        double outdoorLoss;
        double foliageLoss;

        outdoorLoss = Pathloss_UrbanOutdoor(node,
                                            distance,
                                            wavelength,
                                            fromPosition,
                                            toPosition,
                                            txAntennaHeight,
                                            rxAntennaHeight,
                                            &urbanStats);

        foliageLoss = Pathloss_Foliage(node,
                                       distance,
                                       wavelength,
                                       fromPosition,
                                       toPosition,
                                       txAntennaHeight,
                                       rxAntennaHeight,
                                       propProfile);

        pathloss_dB = outdoorLoss + foliageLoss;

        urbanStats.outsidePathloss = outdoorLoss;
        urbanStats.outsideDistance = distance;
    }

    urbanStats.pathloss = pathloss_dB;

    if ((urbanStats.modelSelected != "FREESPACE") &&
         (urbanStats.modelSelected != "TWORAY") ) {

        urbanStats.freeSpacePathloss = PROP_PathlossFreeSpace(
                                            distance, 
                                            wavelength);
        urbanStats.twoRayPathloss = PROP_PathlossTwoRay(
                                         distance, 
                                         wavelength,
                                         txAntennaHeight,
                                         rxAntennaHeight);

         int numSamples;
         double elevationArray[MAX_NUM_ELEVATION_SAMPLES];
         double txPlatformHeight = 0.0,  rxPlatformHeight = 0.0;

         if (pathProfile->distance == 0.0) {
             urbanStats.itmPathloss = 0.0;

         } else {
             numSamples =
                 TERRAIN_GetElevationArray(
                     terrainData,
                     &(pathProfile->fromPosition),
                     &(pathProfile->toPosition),
                     pathProfile->distance,
                     100.0,
                     elevationArray);

             if (((terrainData->getCoordinateSystem() == CARTESIAN) &&
                  (pathProfile->fromPosition.cartesian.y >=
                      pathProfile->toPosition.cartesian.y)) ||
#ifdef MGRS_ADDON
                  ((terrainData->getCoordinateSystem() == MGRS) &&
                  (pathProfile->fromPosition.common.c2 >=
                    pathProfile->toPosition.common.c2)) ||
#endif // MGRS_ADDON
                  // LATLONALT
                  (pathProfile->fromPosition.latlonalt.latitude >=
                      pathProfile->toPosition.latlonalt.latitude))
             {
                 // the MAX adjusts for nodes that for some reason are
                    // below ground
                 txPlatformHeight =
                     MAX(0.0, pathProfile->fromPosition.common.c3 -
                              elevationArray[0]) + txAntennaHeight;
                 rxPlatformHeight = 
                     MAX(0.0, pathProfile->toPosition.common.c3 -
                              elevationArray[numSamples]) + rxAntennaHeight;
                     // yes, that should be numSamples, not numSamples - 1
            } else {
                 txPlatformHeight = 
                     MAX(0.0, pathProfile->fromPosition.common.c3 -
                              elevationArray[numSamples]) + txAntennaHeight;
                 rxPlatformHeight = 
                     MAX(0.0, pathProfile->toPosition.common.c3 -
                              elevationArray[0]) + rxAntennaHeight;
            }

            urbanStats.itmPathloss =
                PathlossItm(numSamples + 1,
                            pathProfile->distance / (double)numSamples,
                            elevationArray,
                            txPlatformHeight,
                            rxPlatformHeight,
                            VERTICAL,
                            1,
                            15,
                            0.005,
                            propProfile->frequency / 1.0e6,
                            360);


            if (DEBUG) 
            {
                printf("from (%3.4f, %3.4f) to (%3.4f, %3.4f) heights %4.2f, %4.2f: ",
                       pathProfile->fromPosition.latlonalt.latitude,
                       pathProfile->fromPosition.latlonalt.longitude,
                       pathProfile->toPosition.latlonalt.latitude,
                       pathProfile->toPosition.latlonalt.longitude,
                       txPlatformHeight, rxPlatformHeight);
                printf("Urban: %3.4f, Free Space: %3.4f, Diff: %2.4f\n",
                       urbanStats.pathloss,
                       PROP_PathlossFreeSpace(pathProfile->distance,
                                              wavelength),
                       urbanStats.pathloss -
                       PROP_PathlossFreeSpace(pathProfile->distance,
                                              wavelength));
                fflush(stdout);
            }
        }
    }

#ifdef ADDON_DB

    StatsDb* db = node->partitionData->statsDb;

    if (db != NULL && db->statsTable->createUrbanPropTable) {
        STATSDB_HandleUrbanPropInsertion(node, &urbanStats);
    }

#endif
    if (DEBUG) 
    {
        printf("Total pathloss from urban autoselect is %f \n\n",
               pathloss_dB);
    }

    return pathloss_dB;
}

/****** "Other than COST231_WI models" for Urban Propagation *******/

/* Computation of constants a, b: Equations used in the following function
are obtained by performing simple non-linear regression to the data in Stuber
p. 103. The data originally appeared in "Short distance attenuation measurements
at 900 MHz and 1.8 gHz using low antenna heights for microcells", P.Harley,
IEEE JSAC, vol.7, pp.5-11, Jan 1989. The data gives values of a, b  for
four different heights. The constants are obtained by fitting a function
of the form f(x) = C1 + C2*x + C3*x**2 to the data and minimizing the mean
squared error between the data and the function. */

// /**
// FUNCTION :: Pathloss_StreetMicrocell_LoS
// LAYER :: Physical Layer.
// PURPOSE :: Propagation pathloss for LoS, Street Microcell model
// PARAMETERS ::
// + h1: double : Height of node first node
// + h2: double : Height of node second node
// + wavelength: double : wavelength
// + distance: double : distance between nodes or from wall corner
//          (when called from Pathloss_StreetMicrocell_NLoS())
// RETURN :: double : Pathloss value
//
// **/

double Pathloss_StreetMicrocell_LoS(
    double h1,
    double h2,
    double wavelength,
    double distance)
{
    double pathloss_dB;
    double g;
    double a;
    double b;
    double arg;
    double offset;

    if (h1 == 0.0) {
        h1 = VERY_SMALL_ANTENNA_HEIGHT;
    }
    if (h2 == 0.0) {
        h2 = VERY_SMALL_ANTENNA_HEIGHT;
    }

    if (h2 > h1) {

        h1 = h2;
    }

    if (distance == 0.0) {

        pathloss_dB = 0.0;
    }
    else {

        if (h1 > 15.0){

            double frequencyMhz = 1.0e-6 * SPEED_OF_LIGHT / wavelength;
            double distanceKm = distance * 1.0e-3;

            pathloss_dB =
                32.4 + 20.0 * log10(distanceKm) +
                20.0 * log10(frequencyMhz);
        }
        else {

            a = 2.1647 - 0.2885 * h1 + 0.0204 * pow(h1, 2)
                - 0.0007 * pow(h1, 3);

            b = -1.3484 + 0.3736 * h1 - 0.0325 * pow(h1, 2)
                + 0.0012 * pow(h1, 3);

            g = 101.1643 + 16.574 * h1 - 1.6686 * pow(h1, 2)
                + 0.0502 * pow(h1, 3);

            offset = 111.1964 - 3.1881 * h1 - 0.0279 *
                pow(h1, 2) - 0.0005 * pow(h1, 3);

            arg = pow(distance, a);
            arg = arg * pow((1.0 + distance/g), b);

            if (DEBUG) 
            {
                printf("h1,distance,a,b,g,offset,arg=%f,%f,%f,%f,%f,%f,%f\n",
                       h1, distance, a, b, g, offset, arg);
            }
            pathloss_dB = 20.0 * log10(arg) - offset + 140.0;

        }
    }

    return pathloss_dB;
}

// /**
// FUNCTION :: Pathloss_StreetMicrocell_NLoS
// LAYER :: Physical Layer.
// PURPOSE :: Propagation pathloss for NLoS, Street Microcell model
// PARAMETERS ::
// + h1: double : Height of node first node
// + h2: double : Height of node second node
// + wavelength: double : wavelength
// + txDistanceToBuilding: double : Transmitter distance from building corner
// + rxDistanceToBuilding: double : Receiver distance from building corner
// + distanceThruBuilding: double : Distance through building
// RETURN :: double : Pathloss value
//
// **/

double Pathloss_StreetMicrocell_NLoS(double h1,
                                     double h2,
                                     double wavelength,
                                     double txDistanceToBuilding,
                                     double rxDistanceToBuilding,
                                     double distanceThruBuilding)
{
    double pathloss_dB = 0.0;

    if (DEBUG) 
    {
        printf("StreetMicrocell_NLoS: %f, %f, %f, %f, %f, %f\n",
               h1, h2, wavelength, txDistanceToBuilding,
               rxDistanceToBuilding, distanceThruBuilding);
    }

    // The "if" statement is to include cases where the tx/rx are
    // located on top of a building.
    if (txDistanceToBuilding > 0.0) {
        pathloss_dB = Pathloss_StreetMicrocell_LoS(h1,h2,
                                                   wavelength,
                                                   txDistanceToBuilding);

    }

    double d = distanceThruBuilding + rxDistanceToBuilding;

    if (d > 0.0) {
        pathloss_dB += Pathloss_StreetMicrocell_LoS(h1,h2, wavelength, d);

    }
    if (DEBUG) 
    {
        printf("StreetMicrocell_NLoS: %f, %f, %f, %f, %f\n",
               Pathloss_StreetMicrocell_LoS(h1,h2, wavelength, d), h1, h2,
               distanceThruBuilding,
               rxDistanceToBuilding);
    }
    //pathloss due to building penetration neglected.

    return pathloss_dB;
}

// /**
// FUNCTION :: Pathloss_Street_M_to_M
// LAYER :: Physical Layer.
// PURPOSE :: Propagation pathloss for mobile-to-mobile in urban canyon
// PARAMETERS ::
// + numBuildingsInPath: int : number of buildings in propagation path
// + h1: double : Height of node first node
// + h2: double : Height of node second node
// + buildingHeight: double : Average building height in propagation path
// + streetWidth: double : Average street-width height in propagation path
// + txRxDistance: double : distance between nodes
// + wavelength: double: wavelength
// RETURN :: double : Pathloss value
//
// **/
double Pathloss_Street_M_to_M(int numBuildingsInPath,
                              double h1,
                              double h2,
                              double buildingHeight,
                              double streetWidth,
                              double txRxDistance,
                              double wavelength)
{
    double pathloss_dB;
    double arg1;
    double arg2;
    double arg3;
    double rho;
    double A;

    if (txRxDistance == 0.0)
    {
        pathloss_dB = 0.0;
    }
    else {

    //Assumption: Mobiles located at the center of streets
    rho = wavelength * streetWidth/2.0;
        A = 2.0 * PI_SQUARE;

        arg1 = 2.0 * IN_DB((4.0 * PI * txRxDistance) / wavelength);
        arg2 = pow((double)(buildingHeight - h1), 2.0);

        if ((buildingHeight - h1) > 0.0)
        {
            arg2 = IN_DB((A * arg2) / rho);

            if (arg2 < 0.0)
            {
                arg2 = 0.0;
            }
        }
        else if ((buildingHeight - h1) < 0.0)
        {
            arg2 = -IN_DB((A * arg2) / rho);

            if (arg2 > 0.0)
            {
                arg2 = 0.0;
            }
        }
        else
        {
            arg2 = 0.0;
        }

        arg3 = pow((double)(buildingHeight - h2), 2.0);

        if ((buildingHeight - h2) > 0.0)
        {
            arg3 = IN_DB((A * arg3) / rho);

            if (arg3 < 0.0)
            {
                arg3 = 0.0;
            }

        }
        else if ((buildingHeight - h2) < 0.0)
        {
            arg3 = -IN_DB((A * arg3) / rho);

            if (arg3 > 0.0)
            {
                arg3 = 0.0;
            }

        }
        else
        {
            arg3 = 0.0;
        }

        pathloss_dB = arg1 + arg2 + arg3;

        if ((numBuildingsInPath - 1) > 0)
        {
            pathloss_dB += 20.0*log10((double)numBuildingsInPath - 1.0);

        }

        if (pathloss_dB < arg1)
        {
            pathloss_dB = arg1;
        }

        if (DEBUG) 
        {
            printf("h1, h2= %f, %f, building height = %f,"
               " Street Width = %f\n",
               h1, h2, buildingHeight, streetWidth);

            printf("pathloss components = %f, %f, %f, %f \n",
                    arg1, arg2, arg3,
                    20.0*log10((double)numBuildingsInPath - 1.));
        }
    }

    // Same Model: Another approximation
    /*
    double val2 =
    ((double)numBuildingsInPath - 1.)/(double)numBuildingsInPath;

    val2 = 20.*log10(16.*pow(PI, 3)) + 20.*log10(val2)
         + 40.*log10((buildingHeight- h1)/wavelength);

    if (DEBUG)
    printf("PLoss=%f, %f\n", val2, pathloss_dB);
    */

    return pathloss_dB;
}


// /**
// FUNCTION :: UrbanProp_PrintModelInfo
// LAYER :: Physical Layer.
// PURPOSE :: Prints model information for debugging purposes
// PARAMETERS ::
// + isFreeSpace: BOOL : Flag to indicate Freespace
// + isLoS: BOOL : Flag to indicate LoS
// + isCanyon1: BOOL : Flag to indicate Node 1 in Canyon
// + isCanyon2: BOOL : Flag to indicate Node 2 in Canyon
// + urbanModelType: UrbanModelType (Enum): Urban Model Type
// RETURN :: void : void
//
// **/
void UrbanProp_PrintModelInfo(BOOL isFreeSpace,
                              BOOL isLoS,
                              BOOL isCanyon1,
                              BOOL isCanyon2,
                              UrbanModelType urbanModelType)
{
    const char* ModelName = NULL;

    switch (urbanModelType)
    {
        case FREESPACE:
            ModelName="FREESPACE";
            break;

        case TWORAY:
            ModelName="TWORAY";
            break;

        case ITU_R_LOS:
            ModelName="ITU_R_LOS";
            break;

        case ITU_R_NLOS1:
            ModelName="ITU_R_NLOS1";
            break;

        case ITU_R_NLOS2:
            ModelName="ITU_R_NLOS2";
            break;

        case ITU_R_UHF_LOS:
            ModelName="ITU_R_UHF_LOS";
            break;

        case ITU_R_UHF_NLOS:
            ModelName="ITU_R_UHF_NLOS";
            break;

        case ITU_R_VHFUHF:
            ModelName="ITU_R_VHFUHF";
            break;

        case URBAN_COST_WI_LoS:
            ModelName="URBAN_COST_WI_LoS";
            break;

        case URBAN_COST_WI_NLoS:
            ModelName="URBAN_COST_WI_NLoS";
            break;

        case URBAN_STREET_MICROCELL_LoS:
            ModelName="URBAN_STREET_MICROCELL_LoS";
            break;

        case URBAN_STREET_MICROCELL_NLoS:
            ModelName="URBAN_STREET_MICROCELL_NLoS";
            break;

        case URBAN_STREET_M_TO_M:
            ModelName="URBAN_STREET_M_TO_M";
            break;
    }
    if (DEBUG)
    {
        if (ModelName != NULL)
        {
            printf("Outdoor model: FreeSpace, LoS, Canyon1, Canyon2 ="
                   "%d, %d, %d, %d, Model: %s\n",
                   isFreeSpace, isLoS, isCanyon1, isCanyon2,
                   ModelName);
        } else 
        {
            printf("No Model Chossen. ERROR!\n\n");
        }
    }
}


// /**
// FUNCTION :: StreetMicrocell_Initialize
// LAYER :: Physical Layer.
// PURPOSE :: Initialize Street Microcell model
// PARAMETERS ::
// + propChannel: PropChannel * : Pointer to propagation channel
// + channelIndex: int : Channel Index
// + nodeInput:  NodeInput * : Node input
// RETURN :: void : void value
//
// **/
void StreetMicrocell_Initialize(PropChannel *propChannel,
                                int channelIndex,
                                const NodeInput *nodeInput)
{
    BOOL wasFound;
    char losIndicator[MAX_STRING_LENGTH];

    PropProfile* propProfile = propChannel->profile;

    IO_ReadStringInstance(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "PROPAGATION-STREET-MICROCELL-ENVIRONMENT",
        channelIndex,
        (channelIndex == 0),
        &wasFound,
        losIndicator);

    if (wasFound) {
        if (strcmp(losIndicator, "LOS") == 0) {
            propProfile->losIndicator = LOS;

        }
        else if (strcmp(losIndicator, "NLOS") == 0) {
            propProfile->losIndicator = NLOS;
        }
        else {
            ERROR_ReportError("Unrecognized propagation environment\n");
        }
    }
    else {
        propProfile->losIndicator = LOS ;
    }

    if (DEBUG)
    {
        printf("Street-Microcell-LoS Init Complete \n");
    }

    return;
}


// /**
// FUNCTION :: Street_M_to_M_Initialize
// LAYER :: Physical Layer.
// PURPOSE :: Initialize Street mobile-to-mobile model
// PARAMETERS ::
// + propChannel: PropChannel * : Pointer to propagation channel
// + channelIndex: int : Channel Index
// + nodeInput:  NodeInput * : Node input
// RETURN :: void : void value
//
// **/
void Street_M_to_M_Initialize(PropChannel *propChannel,
                              int channelIndex,
                              const NodeInput *nodeInput)
{
    BOOL wasFound;
    int num_buildings;

    PropProfile* propProfile = propChannel->profile;

    IO_ReadIntInstance(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "NUM-OF-BUILDINGS-IN-PATH",
        channelIndex,
        TRUE,
        &wasFound,
        &num_buildings);

    if (wasFound) {
        propProfile->Num_builings_in_path = num_buildings;
    }
    else {
        propProfile->Num_builings_in_path =
            DEFAULT_NUM_OF_BUILDINGS_IN_PATH;
    }

    double val;

    IO_ReadDoubleInstance(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "PROPAGATION-STREET-WIDTH",
        channelIndex,
        TRUE,
        &wasFound,
        &val);

    if (wasFound) {
        propProfile->streetWidth = val;
    }
    else {
        propProfile->streetWidth = DEFAULT_STREET_WIDTH;
    }

    IO_ReadDoubleInstance(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "PROPAGATION-ROOF-HEIGHT",
        channelIndex,
        TRUE,
        &wasFound,
        &val);

    if (wasFound) {
        propProfile->roofHeight = val;
    }
    else {
        propProfile->roofHeight = DEFAULT_ROOF_HEIGHT;
    }

    if (DEBUG)
    {
        printf("Street-M-to-M Init Complete \n");
    }

    return;
}
