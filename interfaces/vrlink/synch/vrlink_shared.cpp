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

#include "vrlink_shared.h"
#include "vrlink.h"
#include <cerrno>



// /**
// FUNCTION :: ConvertGccToLatLonAlt
// PURPOSE :: Converts the GCC coordinates to latLonAlt coordinates.
// PARAMETERS  ::
// + gccCoord : DtVector : GCC coordinates to be converted.
// + latLonAltInDeg : DtVector& : Converted latLinAlt coordinates.
// RETURN :: void : NULL.
// **/
void ConvertGccToLatLonAlt(
    DtVector gccCoord,
    DtVector& latLonAltInDeg)
{
    DtGeodeticCoord radioLatLonAltInRad;

    radioLatLonAltInRad.setGeocentric(gccCoord);
    latLonAltInDeg.setX(DtRad2Deg(radioLatLonAltInRad.lat()));
    latLonAltInDeg.setY(DtRad2Deg(radioLatLonAltInRad.lon()));
    latLonAltInDeg.setZ(radioLatLonAltInRad.alt());
}

// /**
// FUNCTION :: ConvertLatLonAltToGcc
// PURPOSE :: Converts the latLonAlt coordinates to GCC coordinates.
// PARAMETERS  ::
// + latLonAlt : DtVector : LatLonAlt coordinates to be converted.
// + gccCoord : DtVector& : Converted GCC coordinates.
// RETURN :: void : NULL.
// **/
void ConvertLatLonAltToGcc(
    DtVector latLonAlt,
    DtVector& gccCoord)
{
    DtGeodeticCoord geod(DtDeg2Rad(latLonAlt.x()), DtDeg2Rad(latLonAlt.y()),
                                                              latLonAlt.z());

    gccCoord = geod.geocentric();
}



// /**
// FUNCTION :: VRLink
// PURPOSE :: Initializing function for VRLink.
// PARAMETERS ::
// **/
VRLink::VRLink()
{
    siteId = 1;
    applicationId = DEFAULT_APPLICATION_ID;

    cout.precision(6);
}

// /**
// FUNCTION :: ~VRLink
// PURPOSE :: Destructor of Class VRLink.
// PARAMETERS :: None.
// **/
VRLink::~VRLink()
{
    delete exConn;
}

// /**
//FUNCTION :: CreateFederation
//PURPOSE :: To make testfed join the federation. If the federation does not
//           exist, the exconn API creates a new federation first, and then
//           register testfed with it.
//PARAMETERS :: None
//RETURN :: void : NULL
// **/
void VRLink::CreateFederation()
{
}

// /**
// FUNCTION :: SetDebug
// PURPOSE :: Selector to set the debug setting.
// PARAMETERS ::
// + type : debug setting to use.
// RETURN :: void : NULL.
// **/
void VRLink::SetDebug(bool val)
{
    m_debug = val;
}

// /**
// FUNCTION :: SetType
// PURPOSE :: Selector to set the VRLink interface type.
// PARAMETERS ::
// + type : VRLinkInterfaceType : VRLink interface to set.
// RETURN :: void : NULL.
// **/
void VRLink::SetType(VRLinkInterfaceType type)
{
    vrLinkType = type;
}


// /**
// FUNCTION :: SetScenarioName
// PURPOSE :: Selector to set the VRLink scenario name.
// PARAMETERS ::
// + type : char [] : scenario name to set.
// RETURN :: void : NULL.
// **/
void VRLink::SetScenarioName(char scenarioName[])
{
    strcpy(m_scenarioName, scenarioName);
}

// /**
//FUNCTION :: GetType
//PURPOSE :: Selector to return the VRLink interface type.
//PARAMETERS :: None
//RETURN :: VRLinkInterfaceType : Configured VRLink interface.
// **/
VRLinkInterfaceType VRLink::GetType()
{
    return vrLinkType;
}

// /**
//FUNCTION :: VRLinkVerify
//PURPOSE :: Verifies the condition passed to be TRUE, and if found to be
//           FALSE, it asserts.
//PARAMETERS ::
// + condition : bool : Condition to be verified.
// + errorString : char* : Error message to be flashed in case the condition
//                         passed is found to be FALSE.
// + path : const char* : File from where the code asserts.
// + lineNumber : unsigned : Line number from where the code asserts.
//RETURN :: void : NULL.
// **/
void VRLinkVerify(
    bool condition,
    const char* errorString,
    const char* path,
    unsigned lineNumber)
{
    if (!condition)
    {
        VRLinkReportError(errorString, path, lineNumber);
    }
}

// /**
//FUNCTION :: VRLinkReportWarning
//PURPOSE :: Prints the warning message passed.
//PARAMETERS ::
// + warningString : const char* : Warning message to be printed.
// + path : const char* : File from where the code asserts.
// + lineNumber : unsigned : Line number from where the code asserts.
//RETURN :: void : NULL.
// **/
void VRLinkReportWarning(
    const char* warningString,
    const char* path,
    unsigned lineNumber)
{
    cout << "VRLink warning:";

    if (path != NULL)
    {
        cout << path << ":";

        if (lineNumber > 0)
        {
            cout << lineNumber << ":";
        }
    }

    cout << " " << warningString << endl;
}

// /**
//FUNCTION :: VRLinkReportError
//PURPOSE :: Prints the error message passed and exits code.
//PARAMETERS ::
// + errorString : const char* : Error message to be printed.
// + path : const char* : File from where the code asserts.
// + lineNumber : unsigned : Line number from where the code asserts.
//RETURN :: void : NULL.
// **/
void VRLinkReportError(
    const char* errorString,
    const char* path,
    unsigned lineNumber)
{
    cout << "VRLink error:";

    if (path != NULL)
    {
        cout << path << ":";

        if (lineNumber > 0)
        {
            cout << lineNumber << ":";
        }
    }

    cout << " " << errorString << endl;

    exit(EXIT_FAILURE);
}

// /**
// FUNCTION :: GetEntityIdKeyFromDtEntityId
// PURPOSE :: Builds entityIdentiferKey from vrlink DtEntityIdentifier
// PARAMETERS :: dtEntityId : DtEntityIdentifier* : pointer to DtEntity identifier
// RETURN :: EntityIdentifierKey* : pointer to a new Entity Identifer structure
// **/
EntityIdentifierKey* VRLink::GetEntityIdKeyFromDtEntityId(
    const DtEntityIdentifier* dtEntityId)
{
    EntityIdentifierKey* entityIdKey = new EntityIdentifierKey;

    entityIdKey->siteId = dtEntityId->site();
    entityIdKey->applicationId = dtEntityId->host();
    entityIdKey->entityNum = dtEntityId->entityNum();

    return entityIdKey;
}

// /**
// FUNCTION :: GetEntityIdKeyFromDtEntityId
// PURPOSE :: Builds entityIdentiferKey from vrlink DtEntityIdentifier
// PARAMETERS :: dtEntityId : DtEntityIdentifier* : pointer DtEntity identifier
// RETURN :: EntityIdentifierKey : Entity Identifer structure
// **/
EntityIdentifierKey VRLink::GetEntityIdKeyFromDtEntityId(
    const DtEntityIdentifier& dtEntityId)
{
    EntityIdentifierKey entityIdKey;

    entityIdKey.siteId = dtEntityId.site();
    entityIdKey.applicationId = dtEntityId.host();
    entityIdKey.entityNum = dtEntityId.entityNum();

    return entityIdKey;
}

// /**
// FUNCTION :: InitConnectionSetting
// PURPOSE :: Initializing function for exercise connection settings
// PARAMETERS ::
// + connectionVar1 : char * : connection setting variable 1
// + connectionVar2 : char * : connection setting variable 2
// + connectionVar3 : char * : connection setting variable 3
// + connectionVar4 : char * : connection setting variable 4
// **/
void VRLink::InitConnectionSetting(char* connectionVar1,
                            char* connectionVar2,
                            char* connectionVar3,
                            char* connectionVar4)
{

}


// /**
// FUNCTION :: GetId
// PURPOSE :: Selector for returning the global id of the entity.
// PARAMETERS :: None
// RETURN :: DtEntityIdentifier
// **/
DtEntityIdentifier VRLinkEntity::GetId()
{
    return id;
}

// /**
// FUNCTION :: GetIdAsString
// PURPOSE :: Selector for returning the global id of a entity as a string
// PARAMETERS :: None
// RETURN :: string
// **/
string VRLinkEntity::GetIdAsString()
{
    return id.string();
}

// /**
// FUNCTION :: GetForceType
// PURPOSE :: Selector for returning the force type of a entity
// PARAMETERS :: None
// RETURN :: DtForceType
// **/
DtForceType VRLinkEntity::GetForceType()
{
    return forceType;
}

// /**
// FUNCTION :: SetEntityIdentifier
// PURPOSE :: Set the DtEntityIdentifer of a specific entity
// PARAMETERS :: unsigned short : siteId
//               unsigned short : appId
//               unsigned short : entityNum
// RETURN :: DtForceType
// **/
void VRLinkEntity::SetEntityIdentifier(unsigned short siteId,
                             unsigned short appId,
                             unsigned short entityNum)
{
    id = DtEntityIdentifier(siteId, appId, entityNum);
}


// /**
// FUNCTION :: SetIsCopied
// PURPOSE :: Set the isCopied bool variable
// PARAMETERS :: bool : value
// RETURN :: NONE
// **/
void VRLinkEntity::SetIsCopied(bool value)
{
    isCopied = value;
}

// /**
// FUNCTION :: GetIsCopied
// PURPOSE :: Get the value of isCopied bool variable
// PARAMETERS ::
// RETURN :: bool
// **/
bool VRLinkEntity::GetIsCopied()
{
    return isCopied;
}

// /**
// FUNCTION :: ConvertReflOrientationToQualNetOrientation
// PURPOSE :: Converts a DIS / RPR-FOM 1.0 orientation into QualNet azimuth
//            and elevation angles. When an entity is located exactly at the
//            north pole, this function will return an azimuth of 0 (facing
//            north) and the entity is pointing towards 180 degrees
//            longitude. When it is located exactly at the south pole, this
//            function will return an azimuth of 0 (facing north) and the
//            entity is pointing towards 0 degrees longitude. This function
//            has not been optimized at all, so the calculations are more
//            readily understood, e.g., the phi angle doesn't affect the
//            results; some vector components end up being multipled by 0,
//            etc.
// PARAMETERS ::
// + orientation : const DtTaitBryan& : Received entity orientation.
// RETURN :: void : NULL.
// **/
void VRLinkEntity::ConvertReflOrientationToQualNetOrientation(
    const DtTaitBryan& orientation)
{
    VRLinkVerify(
        ((latLonAlt.x() >= -90.0) && (latLonAlt.x() <= 90.0)),
        "LatLonAlt coordinates out of bound of specified terrain",
        __FILE__, __LINE__);

    VRLinkVerify(
        ((latLonAlt.y() >= -180.0) && (latLonAlt.y() <= 180.0)),
        "LatLonAlt coordinates out of bound of specified terrain",
        __FILE__, __LINE__);

    // Introduction: --------------------------------------------------------

    // There are two coordinate systems considered:
    //
    // (1) the GCC coordinate system, and
    // (2) the entity coordinate system.
    //
    // Both are right-handed coordinate systems.
    //
    // The GCC coordinate system has well-defined axes.
    // In the entity coordinate system, the x-axis points in the direction
    // the entity is facing (the entity orientation).
    //
    // psi, theta, and phi are the angles by which one rotates the GCC axes
    // so that they match in direction with the entity axes.
    //
    // Start with the GCC axes, and rotate them in the following order:
    //
    // (1) psi, a right-handed rotation about the z-axis
    // (2) theta, a right-handed rotation about the new y-axis
    // (3) phi, a right-handed rotation about the new x-axis

    double psiRadians = orientation.psi();
    double thetaRadians = orientation.theta();
    double phiRadians = orientation.phi();

    // Convert latitude and longitude into a unit vector.
    // If one imagines the earth as a unit sphere, the vector will point
    // to the location of the entity on the surface of the sphere.

    double latRadians = DtDeg2Rad(latLonAlt.x());
    double lonRadians = DtDeg2Rad(latLonAlt.y());

    double entityLocationX = cos(lonRadians) * cos(latRadians);
    double entityLocationY = sin(lonRadians) * cos(latRadians);
    double entityLocationZ = sin(latRadians);

    // Discussion of basic theory: ------------------------------------------

    // Start with a point b in the initial coordinate system. b is
    // represented as a vector.
    //
    // Rotate the axes of the initial coordinate system by angle theta to
    // obtain a new coordinate system.
    //
    // Representation of point b in the new coordinate system is given by:
    //
    // c = ab
    //
    // where a is a rotation matrix:
    //
    // a = [ cos(theta)        sin(theta)
    //       -sin(theta)       cos(theta) ]
    //
    // and c is the same point, but in the new coordinate system.
    //
    // Note that the coordinate system changes; the point itself does not
    // move.
    // Also, matrix a is for rotating the axes; the matrix is different when
    // rotating the vector while not changing the axes.
    //
    // Applying this discussion to three dimensions, and using psi, theta,
    // and phi as described previously, three rotation matrices can be
    // created:
    //
    // Rx =
    // [ 1           0           0
    //   0           cos(phi)    sin(phi)
    //   0           -sin(phi)   cos(phi) ]
    //
    // Ry =
    // [ cos(theta)  0           -sin(theta)
    //   0           1           0
    //   sin(theta)  0            cos(theta) ]
    //
    // Rz =
    // [ cos(psi)    sin(psi)    0
    //   -sin(psi)   cos(psi)    0
    //   0           0           1 ]
    //
    // where
    //
    // c = ab
    // a = (Rx)(Ry)(Rz)
    //
    // b is the point as represented in the GCC coordinate system;
    // c is the point as represented in the entity coordinate system.
    //
    // Note that matrix multiplication is not commutative, so the order of
    // the factors in a is important (rotate by Rz first, so it's on the
    // right side).

    // Determine elevation angle: -------------------------------------------

    // In the computation of the elevation angle below, the change is in the
    // opposite direction, from the entity coordinate system to the GCC
    // system.
    // So, for:
    //
    // c = ab
    //
    // Vector b represents the entity orientation as expressed in the entity
    // coordinate system.
    // Vector c represents the entity orientation as expressed in the GCC
    // coordinate system.
    //
    // It turns out that:
    //
    // a = (Rz)'(Ry)'(Rx)'
    //
    // where Rx, Ry, and Rz are given earlier, and the ' symbol indicates the
    // transpose of each matrix.
    //
    // The ordering of the matrices is reversed, since one is going from the
    // entity coordinate system to the GCC system.  The negative of psi,
    // theta, and phi angles are used, and the transposed matrices end up
    // being the correct ones.

    double a11 = cos(psiRadians) * cos(thetaRadians);
    double a12 = -sin(psiRadians) * cos(phiRadians)
                 + cos(psiRadians) * sin(thetaRadians) * sin(phiRadians);
    double a13 = -sin(psiRadians) * -sin(phiRadians)
                 + cos(psiRadians) * sin(thetaRadians) * cos(phiRadians);

    double a21 = sin(psiRadians) * cos(thetaRadians);
    double a22 = cos(psiRadians) * cos(phiRadians)
                 + sin(psiRadians) * sin(thetaRadians) * sin(phiRadians);
    double a23 = cos(psiRadians) * -sin(phiRadians)
                 + sin(psiRadians) * sin(thetaRadians) * cos(phiRadians);

    double a31 = -sin(thetaRadians);
    double a32 = cos(thetaRadians) * sin(phiRadians);
    double a33 = cos(thetaRadians) * cos(phiRadians);

    // Vector b is chosen such that it is the unit vector pointing along the
    // positive x-axis of the entity coordinate system. I.e., vector b points
    // in the same direction the entity is facing.

    double b1 = 1.0;
    double b2 = 0.0;
    double b3 = 0.0;

    // The values below are the components of vector c, which represent the
    // entity orientation in the GCC coordinate system.

    double entityOrientationX = a11 * b1 + a12 * b2 + a13 * b3;
    double entityOrientationY = a21 * b1 + a22 * b2 + a23 * b3;
    double entityOrientationZ = a31 * b1 + a32 * b2 + a33 * b3;

    // One now has two vectors:
    //
    // (1) an entity-position vector, and
    // (2) an entity-orientation vector.
    //
    // Note that the position vector is normal to the sphere at the point
    // where the entity is located on the sphere.
    //
    // One can determine the angle between the two vectors using dot product
    // formulas.  The computed angle is the deflection from the normal.

    double dotProduct
        = entityLocationX * entityOrientationX
          + entityLocationY * entityOrientationY
          + entityLocationZ * entityOrientationZ;

    double entityLocationMagnitude
        = sqrt(pow(entityLocationX, 2)
               + pow(entityLocationY, 2)
               + pow(entityLocationZ, 2));

    double entityOrientationMagnitude
        = sqrt(pow(entityOrientationX, 2)
               + pow(entityOrientationY, 2)
               + pow(entityOrientationZ, 2));

    double deflectionFromNormalToSphereRadians
        = acos(dotProduct / (entityLocationMagnitude
                             * entityOrientationMagnitude));

    // The elevation angle is 90 degrees minus the angle for deflection from
    // normal.  (Note that the elevation angle can range from -90 to +90
    // degrees.)

    double elevationRadians
        = (M_PI / 2.0) - deflectionFromNormalToSphereRadians;

    elevation = (short) Rint(DtRad2Deg(elevationRadians));

    VRLinkVerify(
        ((elevation >= -90) && (elevation <= 90)),
        "Elevation value not valid",
        __FILE__, __LINE__);

    // Determine azimuth angle: ---------------------------------------------

    // To determine the azimuth angle, for:
    //
    // c = ab
    //
    // b is the entity orientation as represented in the GCC coordinate
    // system.
    //
    // c is the entity orientation as expressed in a new coordinate system.
    // This is a coordinate system where the yz plane is tangent to the
    // sphere at the point on the sphere where the entity is located.
    // The z-axis points towards true north; the y-axis points towards east;
    // the x-axis points in the direction of the normal to the sphere.
    //
    // The rotation matrix turns is then:
    //
    // a = (Ry)(Rz)
    //
    // where longitude is used for Rz and the negative of latitude is used
    // for Ry (since right-handed rotations of the y-axis are positive).

    a11 = cos(-latRadians) * cos(lonRadians);
    a12 = cos(-latRadians) * sin(lonRadians);
    a13 = -sin(-latRadians);

    a21 = -sin(lonRadians);
    a22 = cos(lonRadians);
    a23 = 0;

    a31 = sin(-latRadians) * cos(lonRadians);
    a32 = sin(-latRadians) * sin(lonRadians);
    a33 = cos(-latRadians);

    b1 = entityOrientationX;
    b2 = entityOrientationY;
    b3 = entityOrientationZ;

    // Variable unused.
    //double c1 = a11 * b1 + a12 * b2 + a13 * b3;

    double c2 = a21 * b1 + a22 * b2 + a23 * b3;
    double c3 = a31 * b1 + a32 * b2 + a33 * b3;

    // To determine azimuth, project c against the yz plane(the plane tangent
    // to the sphere at the entity location), creating a new vector without
    // an x component.
    //
    // Determine the angle between this vector and the unit vector pointing
    // north, using dot-product formulas.

    double vectorInTangentPlaneX = 0;
    double vectorInTangentPlaneY = c2;
    double vectorInTangentPlaneZ = c3;

    double northVectorInTangentPlaneX = 0;
    double northVectorInTangentPlaneY = 0;
    double northVectorInTangentPlaneZ = 1;

    dotProduct
        = vectorInTangentPlaneX * northVectorInTangentPlaneX
          + vectorInTangentPlaneY * northVectorInTangentPlaneY
          + vectorInTangentPlaneZ * northVectorInTangentPlaneZ;

    double vectorInTangentPlaneMagnitude
        = sqrt(pow(vectorInTangentPlaneX, 2)
               + pow(vectorInTangentPlaneY, 2)
               + pow(vectorInTangentPlaneZ, 2));

    double northVectorInTangentPlaneMagnitude
        = sqrt(pow(northVectorInTangentPlaneX, 2)
               + pow(northVectorInTangentPlaneY, 2)
               + pow(northVectorInTangentPlaneZ, 2));

    double azimuthRadians
        = acos(dotProduct / (vectorInTangentPlaneMagnitude
                             * northVectorInTangentPlaneMagnitude));

    // Handle azimuth values between 180 and 360 degrees.

    if (vectorInTangentPlaneY < 0.0)
    {
        azimuthRadians = (2.0 * M_PI) - azimuthRadians;
    }

    azimuth = (short) Rint(DtRad2Deg(azimuthRadians));

    if (azimuth == 360)
    {
        azimuth = 0;
    }

    VRLinkVerify(
        ((azimuth >= 0) && (azimuth <= 359)),
        "Azimuth value not valid",
        __FILE__, __LINE__);
}





// /**
// FUNCTION :: GetName
// PURPOSE :: Selector for name of a network
// PARAMETERS ::
// RETURN :: string
// **/
string VRLinkNetwork::GetName()
{
    return name;
}



// /**
// FUNCTION :: VRLinkAggregate
// PURPOSE :: Initializing function for VR-Link aggregate.
// PARAMETERS :: None.
// **/
VRLinkAggregate::VRLinkAggregate()
{
#ifdef DtDIS
    id = DtEntityIdentifier(0, 0, 0);
#else
    id = "";
#endif
    xyz.zero();
    latLonAlt.zero();
    isCopied = false;
}


// /**
// FUNCTION :: SetEntityId
// PURPOSE :: Selector for entityId of a aggregate entity
// PARAMETERS :entId:DtEntityIdentifier
// RETURN :: Null
// **/
void VRLinkAggregate::SetEntityId(DtEntityIdentifier entId)
{
    id = entId;
}

// /**
// FUNCTION :: GetEntityId
// PURPOSE :: Retrieve entityId of a aggregate entity
// PARAMETERS ::
// RETURN :: DtEntityIdentifier
// **/
DtEntityIdentifier VRLinkAggregate::GetEntityId()
{
    return id;
}



// /**
// FUNCTION :: CbReflectAttributeValues
// PURPOSE :: Callback used to reflect the attribute updates received from an
//            outside federate.
// PARAMETERS ::
// + reflRadio : DtReflectedRadioTransmitter* : Reflected radio transmitter.
// + usr : void* : User data.
// RETURN :: void : NULL.
// **/
void VRLinkAggregate::CbReflectAttributeValues(
    DtReflectedAggregate* reflAgge,
    void* usr)
{
    VRLinkVerify(
        reflAgge,
        "NULL reflected aggregate tx data received",
        __FILE__, __LINE__);

    VRLink* vrLink = NULL;
    VRLinkAggregate* agge = NULL;

    VRLinkAggregateCbUsrData* usrData = (VRLinkAggregateCbUsrData*)usr;

    if (usrData)
    {
        usrData->GetData(&vrLink, &agge);
    }

    if ((agge) && (vrLink))
    {
        agge->ReflectAttributes(reflAgge);
    }
}



// /**
// FUNCTION :: ReflectAttributes
// PURPOSE :: Updates the aggregate attributes received from outside federate.
// PARAMETERS ::
// + agge : DtReflectedAggregate* : Reflected aggregate.
// RETURN :: void : NULL.
// **/
void VRLinkAggregate::ReflectAttributes(DtReflectedAggregate* agge)
{
    DtAggregateStateRepository* asr = agge->asr();

    id = asr->entityId();
    xyz = asr->location();

    ConvertGccToLatLonAlt(xyz,
                          latLonAlt);

    myForceID = asr->forceId();
    myAggregateState = asr->aggregateState();
    myFormation = asr->formation();
    myMarkingCharSet = asr->markingCharacterSet();
    myMarkingText = asr->markingText();
    myDimensions = asr->dimensions();

    DtGlobalObjectDesignatorList* entitiesList = asr->entities();
    int numObjs = entitiesList->numObjects();

    std::vector<std::string>::iterator it;
    bool isFound = false;
    for (int i = 0; i < numObjs; i++)
    {
        isFound = false;
        for (it = myEntities.begin(); it != myEntities.end(); it++)
        {
            if (*it == entitiesList->object(i).string())
            {
                isFound = true;
                break;
            }
        }

        if (isFound == false)
        {
            myEntities.push_back(entitiesList->object(i).string());
        }
    }

    DtGlobalObjectDesignatorList* subAggList = asr->subAggregates();
    int numSubAggs = subAggList->numObjects();
    isFound = false;
    for (int i = 0; i < numSubAggs; i++)
    {
        isFound = false;
        for (it = mySubAggs.begin(); it != mySubAggs.end(); it++)
        {
            if (*it == subAggList->object(i).string())
            {
                isFound = true;
                break;
            }
        }

        if (isFound == false)
        {
            mySubAggs.push_back(subAggList->object(i).string());
        }
    }
}


// /**
// FUNCTION :: SetIsCopied
// PURPOSE :: Set the isCopied bool variable
// PARAMETERS :: bool : value
// RETURN :: NONE
// **/
void VRLinkAggregate::SetIsCopied(bool value)
{
    isCopied = value;
}

// /**
// FUNCTION :: GetIsCopied
// PURPOSE :: Get the value of isCopied bool variable
// PARAMETERS ::
// RETURN :: bool
// **/
bool VRLinkAggregate::GetIsCopied()
{
    return isCopied;
}


// /**
// FUNCTION :: GetMyMarkingText
// PURPOSE :: Retriever for marking data of a aggregate
// PARAMETERS ::
// RETURN :: DtString
// **/
DtString VRLinkAggregate::GetMyMarkingText()
{
    return myMarkingText;
}

// /**
// FUNCTION :: GetXYZ
// PURPOSE :: Retriever for xyz location of a aggregate
// PARAMETERS ::
// RETURN :: DtVector
// **/
DtVector VRLinkAggregate::GetXYZ()
{
    return xyz;
}

// /**
// FUNCTION :: GetLatLonAlt
// PURPOSE :: Retriever for latLonAlt location of a aggregate
// PARAMETERS ::
// RETURN :: DtVector
// **/
DtVector VRLinkAggregate::GetLatLonAlt()
{
    return latLonAlt;
}



// /**
// FUNCTION :: GetMyDimensions
// PURPOSE :: Retriever for dimension of a aggregate
// PARAMETERS ::
// RETURN :: DtVector
// **/
DtVector VRLinkAggregate::GetMyDimensions()
{
    return myDimensions;
}


// /**
// FUNCTION :: GetMyEntitiesList
// PURPOSE :: Retriever for myEntities list
// PARAMETERS ::
// RETURN :: std::vector<std::string>
// **/
std::vector<std::string> VRLinkAggregate::GetMyEntitiesList()
{
    return myEntities;
}

// /**
// FUNCTION :: GetMySubAggsList
// PURPOSE :: Retriever for mySubAggs list
// PARAMETERS ::
// RETURN :: std::vector<std::string>
// **/
std::vector<std::string> VRLinkAggregate::GetMySubAggsList()
{
    return mySubAggs;
}





// /**
// FUNCTION :: GetMarkingData
// PURPOSE :: Selector for marking data of a radio
// PARAMETERS ::
// RETURN :: string
// **/
string VRLinkRadio::GetMarkingData()
{
    return markingData;
}

// /**
// FUNCTION :: GetEntityId
// PURPOSE :: Selector for getting entity Id
// PARAMETERS ::
// RETURN :: DtEntityIdentifier
// **/
DtEntityIdentifier VRLinkRadio::GetEntityId()
{
    return entityId;
}


// /**
// FUNCTION :: GetRadioSystemType
// PURPOSE :: Selector for getting RadioSystemType
// PARAMETERS ::
// RETURN :: RadioSystemType
// **/
RadioSystemType VRLinkRadio::GetRadioSystemType()
{
    return radioSystemType;
}



// /**
// FUNCTION :: GetRadioIndex
// PURPOSE :: Selector for index of a radio
// PARAMETERS ::
// RETURN :: string
// **/
unsigned short VRLinkRadio::GetRadioIndex()
{
    return radioIndex;
}



// /**
// FUNCTION :: SetRadioIndex
// PURPOSE :: Selector for assigning index of a radio
// PARAMETERS :unsigned short: index
// RETURN :: string
// **/
void VRLinkRadio::SetRadioIndex(unsigned short index)
{
    radioIndex = index;
}



// /**
// FUNCTION :: SetTransmitState
// PURPOSE :: Set the current transmit state of a radio
// PARAMETERS :: DtRadioTransmitState : val
// RETURN :: void : NULL
// **/
void VRLinkRadio::SetTransmitState(DtRadioTransmitState val)
{
    txOperationalStatus = val;
}

// /**
// FUNCTION :: SetEntityPtr
// PURPOSE :: Set the parent entity pointer of a radio
// PARAMETERS :: VRLinkEntity* : ptr
// RETURN :: void : NULL
// **/
void VRLinkRadio::SetEntityPtr(VRLinkEntity* ptr)
{
    entityPtr = ptr;
}

// /**
// FUNCTION :: GetRelativePosition
// PURPOSE :: Selector for relative position of a radio to the parent entity
// PARAMETERS ::
// RETURN :: DtVector
// **/
DtVector VRLinkRadio::GetRelativePosition()
{
    return relativePosition;
}

// /**
// FUNCTION :: GetAbsoluteLatLonAltPositionForNode
// PURPOSE :: Selector for absolute position of a radio 
//            in lat, lon, alt
// PARAMETERS ::
// RETURN :: DtVector
// **/
DtVector VRLinkRadio::GetAbsoluteLatLonAltPosition()
{
    return absolutePosInLatLonAlt;
}

// /**
// FUNCTION :: GetRadioKey
// PURPOSE :: Selector for key of a radio
// PARAMETERS ::
// RETURN :: string
// **/
string VRLinkRadio::GetRadioKey()
{
    char tmp[512];
    sprintf(tmp, "%s:%d", markingData.c_str(), radioIndex);
    return tmp;
}




// /**
// FUNCTION :: SetRadioIdKey
// PURPOSE :: Selector for setting RadioIdkey of a radio
// PARAMETERS :RadioIdKey: key
// RETURN :: none
// **/
void VRLinkRadio::SetRadioIdKey(RadioIdKey key)
{
    radioKey = key;
}




// /**
// FUNCTION :: GetRadioIdKey
// PURPOSE :: Selector for getting RadioIdkey of a radio
// PARAMETERS ::
// RETURN :: RadioIdKey
// **/
RadioIdKey VRLinkRadio::GetRadioIdKey()
{
    return radioKey;
}



// /**
// FUNCTION :: GetNodeId
// PURPOSE :: Selector to get the qualnet node id of a radio
// PARAMETERS :: None.
// RETURN : unsigned
// **/
unsigned VRLinkRadio::GetNodeId()
{
    return nodeId;
}

// /**
// FUNCTION :: SetRelativePosition
// PURPOSE :: Used to set relative position in XYZ (GCC) and the absolute position latlonalt
// PARAMETERS :: position : DtVector : relative position in GCC.
// RETURN : none
// **/
void VRLinkRadio::SetRelativePosition(DtVector position)
{
    relativePosition = position;

    if (relativePosition == DtVector(0.0,0.0,0.0))
    {
        //If the relative position is zero, then the entity position + relative position
        //will be the same, so no need to covert the entity position when it already has been converted
        //Skip the heavy math, and just use the lat lon representation via the entity pointer
        absolutePosInLatLonAlt = entityPtr->GetLatLon();
    }
    else
    {
        RecalculateAbsolutePosition(entityPtr->GetXYZ());
    }
}

// /**
// FUNCTION :: RecalculateAbsolutePosition
// PURPOSE :: Used to calculate the absolute position of the radio in latlonalt
// PARAMETERS :: entityPosition : DtVector : position of entity in GCC.
// RETURN : none
// **/
void VRLinkRadio::RecalculateAbsolutePosition(DtVector entityPosition)
{
    //combine entity gcc position and the relative gcc position of the 
    //radio, then convert the sum to lat lon alt
    ConvertGccToLatLonAlt(entityPosition + relativePosition, absolutePosInLatLonAlt);
}

// /**
// FUNCTION :: SetRadioSystemType
// PURPOSE :: Used to set radioSyatemType variable
// PARAMETERS :: radioEntType : DtRadioEntityType.
// RETURN : unsigned
// **/
void VRLinkRadio::SetRadioSystemType(DtRadioEntityType radioEntType)
{
    radioSystemType.domain = radioEntType.domain();
    radioSystemType.entityKind = radioEntType.kind();
    radioSystemType.countryCode = radioEntType.country();
    radioSystemType.category = radioEntType.category ();
    radioSystemType.nomenclatureVersion = radioEntType.nomenclatureVersion();
    radioSystemType.nomenclature = radioEntType.nomenclature();
}

// /**
// FUNCTION :: SetEntityId
// PURPOSE :: Used to set entityId variable
// PARAMETERS :: entId : DtEntityIdentifier.
// RETURN : unsigned
// **/
void VRLinkRadio::SetEntityId(DtEntityIdentifier entId)
{
    entityId = entId;
}

// /**
// FUNCTION :: SetIsCopied
// PURPOSE :: Set the isCopied bool variable
// PARAMETERS :: bool : value
// RETURN :: NONE
// **/
void VRLinkRadio::SetIsCopied(bool value)
{
    isCopied = value;
}

// /**
// FUNCTION :: GetIsCopied
// PURPOSE :: Get the value of isCopied bool variable
// PARAMETERS ::
// RETURN :: bool
// **/
bool VRLinkRadio::GetIsCopied()
{
    return isCopied;
}

// /**
// FUNCTION :: VRLinkEntity
// PURPOSE :: Initializing function for VR-Link entity.
// PARAMETERS :: None.
// **/
VRLinkEntity::VRLinkEntity()
{
#ifdef DtDIS
    id = DtEntityIdentifier(0, 0, 0);
#else
    id = "";
#endif
    numRadioPtrs = 0;

    orientation = DtTaitBryan(0.0, 0.0, 0.0);
    damageState = DtDamageNone;
    xyz.zero();
    latLonAlt.zero();
    isCopied = false;
}



// /**
// FUNCTION :: GetEntityType
// PURPOSE :: Selector to get the entity type of an entity
// PARAMETERS :: None.
// RETURN : DtEntityType
// **/
DtEntityType VRLinkEntity::GetEntityType()
{
    return type;
}

// /**
// FUNCTION :: GetXYZ
// PURPOSE :: Selector to get the xyz (gcc) world position of an entity
// PARAMETERS :: None.
// RETURN : DtVector
// **/
DtVector VRLinkEntity::GetXYZ()
{
    return xyz;
}

// /**
// FUNCTION :: GetLatLon
// PURPOSE :: Selector to get the lat/lon world position of an entity
// PARAMETERS :: None.
// RETURN : DtVector
// **/
DtVector VRLinkEntity::GetLatLon()
{
    return latLonAlt;
}


// /**
// FUNCTION :: SetLatLon
// PURPOSE :: Set the lat/lon world position of an entity
// PARAMETERS :: DtVector : val : new lat lon postition value
// RETURN : void : NULL
// **/
void VRLinkEntity::SetLatLon(DtVector val)
{
    latLonAlt = val;
}


// /**
// FUNCTION :: SetOrientation
// PURPOSE :: Set the orientation of an entity
// PARAMETERS :: DtTaitBryan : val : new orientation value
// RETURN : void : NULL
// **/
void VRLinkEntity::SetOrientation(DtTaitBryan val)
{
    orientation = val;
}

// /**
// FUNCTION :: SetLatLon
// PURPOSE :: Set the xyz (gcc) world position of an entity
// PARAMETERS :: DtVector : val : new xyz (gcc) postition value
// RETURN : void : NULL
// **/
void VRLinkEntity::SetXYZ(DtVector val)
{
    xyz = val;
}

// /**
// FUNCTION :: GetOrientation
// PURPOSE :: Selector to get the orientation of an entity
// PARAMETERS :: None.
// RETURN : DtTaitBryan
// **/
DtTaitBryan VRLinkEntity::GetOrientation()
{
    return orientation;
}


// /**
// FUNCTION :: GetAzimuth
// PURPOSE :: Selector to get the azimuth of an entity
// PARAMETERS :: None.
// RETURN : short
// **/
short VRLinkEntity::GetAzimuth()
{
    return azimuth;
}

// /**
// FUNCTION :: GetElevation
// PURPOSE :: Selector to get the elevation of an entity
// PARAMETERS :: None.
// RETURN : short
// **/
short VRLinkEntity::GetElevation()
{
    return elevation;
}




// /**
// FUNCTION :: GetDamageState
// PURPOSE :: Selector to get the damage state of an entity
// PARAMETERS :: None.
// RETURN : DtDamageState
// **/
DtDamageState VRLinkEntity::GetDamageState()
{
    return damageState;
}

// /**
// FUNCTION :: SetDamageState
// PURPOSE :: Set the damage state of an entity
// PARAMETERS :: DtDamageState : state : new damage value
// RETURN : void : NULL
// **/
void VRLinkEntity::SetDamageState(DtDamageState state)
{
    damageState = state;
}

// /**
// FUNCTION :: GetMarkingData
// PURPOSE :: Selector for returning the entity marking data.
// PARAMETERS :: None
// RETURN :: string
// **/
string VRLinkEntity::GetMarkingData()
{
    return markingData;
}
// /**
// FUNCTION :: GetMarkingData
// PURPOSE :: Selector for returning the entity marking data.
// PARAMETERS :: None
// RETURN :: string
// **/
string VRLinkEntity::GetEntityIdString()
{
    char tmp[64];
    sprintf(tmp, "%hu.%hu.%hu", id.site(), id.host(), id.entityNum());
    return string(tmp);
}


// /**
// FUNCTION :: GetMyName
// PURPOSE :: Selector for returning the myName for HLA13 and HLA1516.
// PARAMETERS :: None
// RETURN :: string
// **/
string VRLinkEntity::GetMyName()
{
    return myName;
}


// /**
// FUNCTION :: CbReflectAttributeValues
// PURPOSE :: Callback for reflecting the attribute updates.
// PARAMETERS ::
// + ent : DtReflectedEntity* : Entity of which the attributes are to be
//                              updated.
// + usr : void* : User data.
// RETURN :: void : NULL
// **/
void VRLinkEntity::CbReflectAttributeValues(
    DtReflectedEntity* ent,
    void* usr)
{
     VRLinkVerify(
         ent,
         "NULL reflected entity data received",
         __FILE__, __LINE__);

    VRLink* vrLink = NULL;
    VRLinkEntity* entity = NULL;

    VRLinkEntityCbUsrData* usrData = (VRLinkEntityCbUsrData*)usr;

    if (usrData)
    {
        usrData->GetData(&vrLink, &entity);
    }

    if ((entity) && (vrLink))
    {
        entity->ReflectAttributes(ent, vrLink);
        entity->SetIsCopied(false);
    }
}



// /**
// FUNCTION :: ReflectAttributes
// PURPOSE :: Reflects new entity attributes (called by
//            CbReflectAttributeValues callback).
// PARAMETERS ::
// + ent : DtReflectedEntity* : Entity of which the attributes are to be
//                              updated.
// + vrLink : VRLink* : VRLink pointer.
// RETURN :: void : NULL.
// **/
void VRLinkEntity::ReflectAttributes(DtReflectedEntity* ent, VRLink* vrLink)
{
    DtEntityStateRepository* esr = ent->esr();

    id = esr->entityId();
    ReflectDamageState(esr->damageState(), vrLink);

    ReflectWorldLocation(
        esr->location(),
        vrLink);

    ReflectOrientation(esr->orientation());

    ReflectForceId(esr->forceId());

    type = esr->entityType();
    markingData = esr->markingText();

#if defined (DtHLA13) || defined (DtHLA_1516)
    myName = ent->name().c_str();
#endif
}

// /**
// FUNCTION :: ReflectDamageState
// PURPOSE :: To update the entity damage state based on the value received
//            from an outside federate.
// PARAMETERS ::
// + reflDamageState : DtDamageState : Received damage state.
// + vrLink : VRLink* : VRLink pointer.
// RETURN :: void : NULL.
// **/
void VRLinkEntity::ReflectDamageState(
    DtDamageState reflDamageState,
    VRLink* vrLink)
{
    VRLinkVerify(reflDamageState >= DtDamageNone
              && reflDamageState <= DtDamageDestroyed,
              "Unexpected attribute value",
              __FILE__, __LINE__);

    VRLinkVerify(
        (damageState >= DtDamageNone) && (damageState <= DtDamageDestroyed),
        "Invalid damage state received",
         __FILE__, __LINE__);

    damageState = reflDamageState;
}




// /**
// FUNCTION :: ReflectWorldLocation
// PURPOSE :: To update the entity location based on the value received from
//            an outside federate.
// PARAMETERS ::
// + reflWorldLocation : const DtVector& : Received entity location (GCC).
// + vrLink : VRLink* : VRLink pointer.
// RETURN :: void : NULL.
// **/
void VRLinkEntity::ReflectWorldLocation(
    const DtVector& reflWorldLocation,
    VRLink* vrLink)
{
    DtVector reflLatLonAltInDeg;


    if (xyz == reflWorldLocation)
    {
        return;
    }

    ConvertGccToLatLonAlt(reflWorldLocation, reflLatLonAltInDeg);

    VRLinkVerify(
        ((reflLatLonAltInDeg.x() >= -90.0) &&
         (reflLatLonAltInDeg.x() <= 90.0)),
        "Reflected coordinates out of bound of specified terrain",
        __FILE__, __LINE__);

     VRLinkVerify(
         ((reflLatLonAltInDeg.y() >= -180.0) &&
          (reflLatLonAltInDeg.y() <= 180.0)),
         "Reflected coordinates out of bound of specified terrain",
         __FILE__, __LINE__);


    xyz = reflWorldLocation;
    latLonAlt = reflLatLonAltInDeg;

    //Update any absolute positions of child radio transmitters
    for (map <RadioIdKey, VRLinkRadio*>::iterator it = vrLink->GetVRLinkRadios()->begin(); 
        it != vrLink->GetVRLinkRadios()->end(); it++)
    {
        if (it->second->GetEntityPtr() != NULL)
        {            
            if (it->second->GetEntityId() == id)
            {
                it->second->RecalculateAbsolutePosition(reflWorldLocation);
            }
        }
    }   
}


// /**
// FUNCTION :: ReflectOrientation
// PURPOSE :: To update the entity orientation based on the value received
//            from an outside federate.
// PARAMETERS ::
// + reflOrientation : const DtTaitBryan& : Received entity orientation.
// RETURN :: void : NULL.
// **/
void VRLinkEntity::ReflectOrientation(
    const DtTaitBryan& reflOrientation)
{
    if (orientation == reflOrientation)
    {
        return;
    }

    ConvertReflOrientationToQualNetOrientation(reflOrientation);

    orientation = reflOrientation;
}

// /**
// FUNCTION :: ReflectForceId
// PURPOSE :: To update the entity force id based on the value received
//            from an outside federate.
// PARAMETERS reflForceId
// + reflForceId : const DtForceType& : Received entity force id.
// RETURN :: void : NULL.
// **/
void VRLinkEntity::ReflectForceId(
   const DtForceType & reflForceId)
{
    forceType = reflForceId;
}



// /**
// FUNCTION :: ~VRLinkEntity
// PURPOSE :: Destructor of Class VRLinkEntity.
// PARAMETERS :: None.
// **/
VRLinkEntity::~VRLinkEntity()
{

}

// /**
// FUNCTION :: VRLinkRadio
// PURPOSE :: Initializing function for VRLink radio.
// PARAMETERS :: None.
// **/
VRLinkRadio::VRLinkRadio()
{
    entityPtr = NULL;
    networkPtr = NULL;
    isCopied = false;
    txOperationalStatus = DtRadioOnNotTransmitting;
    radioIndex = 0;
}

// /**
// FUNCTION :: GetNetworkPtr
// PURPOSE :: Selector to get a pointer to the network which a radio is associate with
// PARAMETERS :: None.
// RETURN : VRLinkNetwork*
// **/
VRLinkNetwork* VRLinkRadio::GetNetworkPtr()
{
    return networkPtr;
}

// /**
// FUNCTION :: GetTransmitState
// PURPOSE :: Selector for returning the transmit state of a radio
// PARAMETERS :: None
// RETURN :: DtRadioTransmitState
// **/
DtRadioTransmitState VRLinkRadio::GetTransmitState()
{
    return txOperationalStatus;
}

// /**
// FUNCTION :: GetEntityPtr
// PURPOSE :: Selector to get the parent entity of a radio
// PARAMETERS :: None.
// RETURN : VRLinkEntity*
// **/
VRLinkEntity* VRLinkRadio::GetEntityPtr()
{
    return entityPtr;
}

// /**
// FUNCTION :: SetDamageState
// PURPOSE :: Set parent network pointer for a radio
// PARAMETERS :: VRLinkNetwork* : ptr : network you wish associate the radio with
// RETURN : void : NULL
// **/
void VRLinkRadio::SetNetworkPtr(VRLinkNetwork* ptr)
{
    networkPtr = ptr;
}

// /**
// FUNCTION :: ~VRLinkRadio
// PURPOSE :: Destructor of Class VRLinkRadio.
// PARAMETERS :: None.
// **/
VRLinkRadio::~VRLinkRadio()
{
    entityPtr = NULL;
    networkPtr = NULL;
}



// /**
// FUNCTION :: GetExConn
// PURPOSE :: Selector to get the vrlink exercise connection
// PARAMETERS :: None.
// RETURN : DtExerciseConn*
// **/
DtExerciseConn* VRLink::GetExConn()
{
    return exConn;
}



double Rint(double a)
{
#ifdef _WIN32
    // Always round up when the fractional value is exactly 0.5.
    // (Usually such functions round up only half the time.)

    return floor(a + 0.5);
#else /* _WIN32 */
    return rint(a);
#endif /* _WIN32 */
}


// /**
// FUNCTION :: RegisterProtocolSpecificCallbacks
// PURPOSE :: Registers HLA/DIS specific callbacks.
// PARAMETERS :: None.
// RETURN :: void : NULL
// **/
void VRLink::RegisterProtocolSpecificCallbacks()
{
}


// /**
// FUNCTION :: RadioCallbackCriteria
// PURPOSE :: Specifies the callback calling criteria for addition of
//            radios.
// PARAMETERS ::
// + obj : DtReflectedObject* : Reflected object.
// + usr : void* : User pointer.
// RETURN :: bool : Returns TRUE if criteria satisfied, FALSE otherwise.
// **/
bool VRLink::RadioCallbackCriteria(DtReflectedObject* obj, void* usr)
{
    DtReflectedRadioTransmitter* radTx = (DtReflectedRadioTransmitter*) obj;

    if ((radTx->tsr()->radioId() < 0) ||
                   (radTx->tsr()->entityId() == DtEntityIdentifier(0, 0, 0)))
    {
        return false;
    }
    return true;
}

// /**
// FUNCTION :: EntityCallbackCriteria
// PURPOSE :: Specifies the callback calling criteria for addition of
//            entities.
// PARAMETERS ::
// + obj : DtReflectedObject* : Reflected object.
// + usr : void* : User pointer.
// RETURN :: bool : Returns TRUE if criteria satisfied, FALSE otherwise.
// **/
bool VRLink::EntityCallbackCriteria(DtReflectedObject* obj, void* usr)
{

    DtReflectedEntity* entity = (DtReflectedEntity*) obj;

    if (!(entity->esr()->markingText()) ||
                   (entity->esr()->entityId() == DtEntityIdentifier(0,0,0)))
    {
        return false;
    }
    return true;
}

// /**
// FUNCTION :: CbEntityAdded
// PURPOSE :: Callback for adding a new entity.
// PARAMETERS ::
// + ent : DtReflectedEntity* : Entity to be added.
// + usr : void* : User data.
// RETURN :: void : NULL
// **/
void VRLink::CbEntityAdded(DtReflectedEntity* ent, void* usr)
{
    VRLinkVerify(
        ent,
        "NULL reflected entity data received",
        __FILE__, __LINE__);

    VRLink* vrLinkPtr = (VRLink*)usr;

    if (vrLinkPtr)
    {
        vrLinkPtr->AddReflectedEntity(ent);
    }
}

// /**
// FUNCTION :: CbEntityRemoved
// PURPOSE :: Callback for removing an entity.
// PARAMETERS ::
// + ent : DtReflectedEntity* : Entity to be removed.
// + usr : void* : User data.
// RETURN :: void : NULL
// **/
void VRLink::CbEntityRemoved(DtReflectedEntity* ent, void *usr)
{
    VRLinkVerify(
        ent,
        "NULL reflected entity data received",
        __FILE__, __LINE__);

    VRLink* vrLinkPtr = (VRLink*)usr;

    vrLinkPtr->RemoveReflectedEntity(ent);
}

// /**
// FUNCTION :: AddReflectedEntity
// PURPOSE :: Adds a new entity (called by CbEntityAdded callback).
// PARAMETERS ::
// + ent : DtReflectedEntity* : Entity to be added.
// RETURN :: void : NULL
// **/
void VRLink::AddReflectedEntity(DtReflectedEntity* ent)
{
    hash_map <string, VRLinkEntity*>::iterator entitiesIter;
    hash_map <string, VRLinkEntity*>::iterator reflectedEntitiesIter;

    VRLinkEntity* entityPtr = NULL;
    DtEntityStateRepository* esr = ent->esr();
    string markingDataStr = esr->markingText();
    DtEntityIdentifier entId = esr->entityId();
    reflectedEntitiesIter = reflectedEntities.find(entId.string());

    if (reflectedEntitiesIter == reflectedEntities.end())
    {
        entityPtr = new VRLinkEntity;

        entityPtr->SetEntityIdentifier(entId.site(),
                                       entId.host(),
                                       entId.entityNum());
        entityPtr->SetIsCopied(false);
        entityPtr->ReflectAttributes(ent, this);

        reflectedEntities[entId.string()] = entityPtr;

        EntityIdentifierKey* entityIdKey =
                                        GetEntityIdKeyFromDtEntityId(&entId);
        entityIdToEntityData[*entityIdKey] = entityPtr;

        VRLinkEntityCbUsrData* usrData = new VRLinkEntityCbUsrData(
                                                 this,
                                                 entityPtr);

        try
        {
            ent->addPostUpdateCallback(
                     &(VRLinkEntity::CbReflectAttributeValues),
                     usrData);
        }
        DtCATCH_AND_WARN(cout);

        //try and "rediscover" any already reflected radio that were tossed out because the 
        //entity was not discovered yet
        if (refRadiosWithoutEntities.find(entId) != refRadiosWithoutEntities.end())
        {
            for (list<DtReflectedRadioTransmitter*>::iterator it = refRadiosWithoutEntities[entId].begin();
                it != refRadiosWithoutEntities[entId].end();it++)
            {
                AddReflectedRadio(*it);
            }
            refRadiosWithoutEntities.erase(entId);
        }
    }
    else
    {
        char errorString[MAX_STRING_LENGTH];

        sprintf(
            errorString,
            "Two or more PhysicalEntity objects with duplicate EntityId =" \
            " %s", entId.string());

        VRLinkReportError(errorString, __FILE__, __LINE__);
    }
}

// /**
// FUNCTION :: RemoveReflectedEntity
// PURPOSE :: Removes an entity (called by CbEntityRemoved callback).
// PARAMETERS ::
// + ent : DtReflectedEntity* : Reflected entity pointer.
// RETURN :: void : NULL
// **/
void VRLink::RemoveReflectedEntity(DtReflectedEntity* ent)
{
    hash_map <string, VRLinkEntity*>::iterator reflEntitiesIter;

    string entityId = ent->esr()->entityId().string();

    reflEntitiesIter = reflectedEntities.find(entityId);

    if (reflEntitiesIter != reflectedEntities.end())
    {
        VRLinkEntity* entity = reflEntitiesIter->second;

         VRLinkVerify(
                entity,
                "Reflected entity not found while removing",
                __FILE__, __LINE__);

        if (m_debug)
        {
            cout << "VRLINK: Removing entity ("
                 << entity->GetEntityIdString() << ") "
                 << entity->GetMarkingData() << endl;
        }

        delete entity;

        reflectedEntities.erase(entityId);

        ent->removePostUpdateCallback(
                 &(VRLinkEntity::CbReflectAttributeValues),
                 this);
    }
    else
    {
        VRLinkReportWarning("Entity not found in reflectedEntitiesHash" \
                            " while removal",
                          __FILE__, __LINE__);
    }
}

// /**
// FUNCTION :: AddReflectedRadio
// PURPOSE :: Adds a new radio (called by CbRadioAdded callback).
// PARAMETERS ::
// + radioTx : DtReflectedRadioTransmitter* : Radio to be added.
// RETURN :: void : NULL
// **/
void VRLink::AddReflectedRadio(DtReflectedRadioTransmitter* radioTx)
{
    map <RadioIdKey, VRLinkRadio*>::iterator reflRadiosIter;
    hash_map <string, VRLinkEntity*>::iterator reflEntitiesIter;

    VRLinkRadio* radioPtr = NULL;
    DtRadioTransmitterRepository* tsr = radioTx->tsr();

    RadioIdKey radioKey = GetRadioIdKey(tsr);

    reflRadiosIter = reflRadioIdToRadioMap.find(radioKey);

    if (reflRadiosIter == reflRadioIdToRadioMap.end())
    {
        const char* entIdStr = tsr->entityId().string();
        reflEntitiesIter = reflectedEntities.find(entIdStr);

        if (reflEntitiesIter != reflectedEntities.end())
        {
            radioPtr = new VRLinkRadio;

            radioPtr->SetEntityPtr(reflEntitiesIter->second);
            radioPtr->SetRadioIdKey(radioKey);

            reflRadioIdToRadioMap[radioKey] = radioPtr;

            EntityIdentifierKey* entityIdKey =
                          GetEntityIdKeyFromDtEntityId(&tsr->entityId());

            EntityIdKeyRadioIndex entityRadioIndex(*entityIdKey,
                                                  radioPtr->GetRadioIndex());
            radioIdToRadioData[entityRadioIndex] = radioPtr;

            radioPtr->ReflectAttributes(radioTx, this);
        }
        else
        {
            DtEntityIdentifier entId = radioTx->tsr()->entityId();
            if (refRadiosWithoutEntities.find(entId) != refRadiosWithoutEntities.end())
            {
                refRadiosWithoutEntities[entId].push_back(radioTx);                
            }
            else
            {
                refRadiosWithoutEntities[entId] = list<DtReflectedRadioTransmitter*>();
                refRadiosWithoutEntities[entId].push_back(radioTx);
            }
        }

        VRLinkRadioCbUsrData* usrData = new VRLinkRadioCbUsrData(
                                                this,
                                                radioPtr);

        try
        {
            radioTx->addPostUpdateCallback(
                         &(VRLinkRadio::CbReflectAttributeValues),
                         usrData);
        }
        DtCATCH_AND_WARN(cout);
    }
    else
    {
        VRLinkRadio* radio = reflRadiosIter->second;
        char errorString[MAX_STRING_LENGTH + 300];

        sprintf(
            errorString,
            " Duplicate RadioTransmitter object received:"
            " EntityID = %s, MarkingData = %s, RadioIndex = %u."
            " (Mapped RadioTransmitter object's host-entity EntityID to"
            " PhysicalEntity object, retrieved MarkingData for that"
            " PhysicalEntity object, but have already discovered a"
            " RadioTransmitter with that MarkingData and RadioIndex)",
            radio->GetEntityPtr()->GetEntityIdString().c_str(),
            radio->GetEntityPtr()->GetMarkingData().c_str(),
            radio->GetRadioIndex());

        VRLinkReportError(errorString);
    }
}


// /**
// FUNCTION :: RemoveReflectedRadio
// PURPOSE :: Removes a radio (called by CbRadioRemoved callback).
// PARAMETERS ::
// + radioTx : DtReflectedRadioTransmitter* : Tx radio to be removed.
// RETURN :: void : NULL
// **/
void VRLink::RemoveReflectedRadio(DtReflectedRadioTransmitter* radioTx)
{
    DtRadioTransmitterRepository* tsr = radioTx->tsr();
    map <RadioIdKey, VRLinkRadio*>::iterator reflRadiosIter;
    unsigned int radioId = tsr->radioId();

    RadioIdKey radioKey = GetRadioIdKey(tsr);

    reflRadiosIter = reflRadioIdToRadioMap.find(radioKey);

    if (reflRadiosIter != reflRadioIdToRadioMap.end())
    {
        VRLinkRadio* radio = reflRadiosIter->second;

        if (m_debug)
        {
            cout << "VRLINK: Removing node "
                << " (" << radio->GetEntityPtr()->GetEntityIdString()<< ", "
                << radio->GetRadioIndex() << ") "<< radio->GetMarkingData()
                << endl;
        }

        if (radio->GetTransmitState() != DtOff)
        {
            DtRadioTransmitState val = DtOff;
            radio->SetTransmitState(val);
        }

        delete radio;

        reflRadioIdToRadioMap.erase(radioKey);

        radioTx->removePostUpdateCallback(
                 &(VRLinkRadio::CbReflectAttributeValues),
                 this);

    }
    else
    {
        VRLinkReportWarning("Radio not found in reflRadioIdToRadioHash" \
                            " while removal",
                          __FILE__, __LINE__);
    }
}



// /**
// FUNCTION :: GetNextReflectedEntity
// PURPOSE :: Get the next reflected entity object in the reflectedEntityList
// PARAMETERS ::
// RETURN :: VRLinkEntity* : Return NULL if no more entity left for traversal
// **/
VRLinkEntity* VRLink::GetNextReflectedEntity()
{
    VRLinkEntity* refEntity = NULL;
    hash_map <string, VRLinkEntity*>::iterator reflectedEntitiesIter;

    for (reflectedEntitiesIter = reflectedEntities.begin();
         reflectedEntitiesIter != reflectedEntities.end();
         reflectedEntitiesIter++)
    {
        refEntity = reflectedEntitiesIter->second;
        if (refEntity->GetIsCopied())
        {
            // This reflected entity is already entertained
            refEntity = NULL;
            continue;
        }
        else
        {
            refEntity->SetIsCopied(true);
            break;
        }

    }
    return refEntity;
}


// /**
// FUNCTION :: GetNextReflectedRadio
// PURPOSE :: Get the next reflected radio object in the reflected RadioList
// PARAMETERS ::
// RETURN :: VRLinkRadio* : Return NULL if no more radio left for traversal
// **/
VRLinkRadio* VRLink::GetNextReflectedRadio()
{
    VRLinkRadio* refRadio = NULL;
    map <RadioIdKey, VRLinkRadio*>::iterator reflRadiosIter;

    for (reflRadiosIter = reflRadioIdToRadioMap.begin();
         reflRadiosIter != reflRadioIdToRadioMap.end();
         reflRadiosIter++)
    {
        refRadio = reflRadiosIter->second;
        if (refRadio->GetIsCopied())
        {
            // This reflected radio is already entertained
            refRadio = NULL;
            continue;
        }
        else
        {
            refRadio->SetIsCopied(true);
            break;
        }
    }

    return refRadio;
}


// /**
// FUNCTION :: GetNextReflectedAggregate
// PURPOSE :: Get the next reflected aggregate object in the reflected
//            Aggregate List
// PARAMETERS ::
// RETURN :: VRLinkAggregate* : Return NULL if no more left for traversal
// **/
VRLinkAggregate* VRLink::GetNextReflectedAggregate()
{
    VRLinkAggregate* refAggr = NULL;
    std::vector<VRLinkAggregate*>::iterator it;

    for (it = refAggregateEntities.begin();
         it != refAggregateEntities.end();
         it++)
    {
        refAggr = *it;
        if (refAggr->GetIsCopied())
        {
            // This reflected aggregate is already entertained
            refAggr = NULL;
            continue;
        }
        else
        {
            refAggr->SetIsCopied(true);
            break;
        }
    }
    return refAggr;
}



// /**
// FUNCTION :: GetRadioIdKey
// PURPOSE :: Get the RadioIdKey for radio transmitter repository.
// PARAMETERS ::
// + dataIxnInfo : DtRadioTransmitterRepository* : Pointer to radio
//                  transmitter repository
// RETURN :: RadioIdKey : EntityIdentifierKey and int pair
// **/
RadioIdKey VRLink::GetRadioIdKey(DtRadioTransmitterRepository* radioTsr)
{
    return GetRadioIdKey(radioTsr);
}



// /**
// FUNCTION :: CbReflectAttributeValues
// PURPOSE :: Callback used to reflect the attribute updates received from an
//            outside federate.
// PARAMETERS ::
// + reflRadio : DtReflectedRadioTransmitter* : Reflected radio transmitter.
// + usr : void* : User data.
// RETURN :: void : NULL.
// **/
void VRLinkRadio::CbReflectAttributeValues(
    DtReflectedRadioTransmitter* reflRadio,
    void* usr)
{
    VRLinkVerify(
        reflRadio,
        "NULL reflected radio tx data received",
        __FILE__, __LINE__);

    VRLink* vrLink = NULL;
    VRLinkRadio* radio = NULL;

    VRLinkRadioCbUsrData* usrData = (VRLinkRadioCbUsrData*)usr;

    if (usrData)
    {
        usrData->GetData(&vrLink, &radio);
    }

    if ((radio) && (vrLink))
    {
        radio->ReflectAttributes(reflRadio, vrLink);
        radio->SetIsCopied(false);
    }
}


// /**
// FUNCTION :: ReflectAttributes
// PURPOSE :: Updates the radio attributes received from an outside federate.
// PARAMETERS ::
// + radioTx : DtReflectedRadioTransmitter* : Reflected radio transmitter.
// + vrLink : VRLink* : VRLink pointer.
// RETURN :: void : NULL.
// **/
void VRLinkRadio::ReflectAttributes(
    DtReflectedRadioTransmitter* radioTx,
    VRLink* vrLink)
{
    DtRadioTransmitterRepository* tsr = radioTx->tsr();

    SetRadioIndex((unsigned short)tsr->radioId());

    DtRadioEntityType radioEntType = tsr->radioEntityType();
    SetRadioSystemType(radioEntType);

    SetRelativePosition(tsr->relativePosition());
    ReflectOperationalStatus(tsr->transmitState(), vrLink);

    SetEntityId(tsr->entityId());
}

// /**
// FUNCTION :: ReflectOperationalStatus
// PURPOSE :: Updates the radio's operational status received from an outside
//            federate.
// PARAMETERS ::
// + reflTxOperationalStatus : const DtRadioTransmitState& : Reflected
//                                                        operational status.
// + vrLink : VRLink* : VRLink pointer.
// RETURN :: void : NULL.
// **/
void VRLinkRadio::ReflectOperationalStatus(
    const DtRadioTransmitState& reflTxOperationalStatus,
    VRLink* vrLink)
{
    if (reflTxOperationalStatus == txOperationalStatus)
    {
        return;
    }

    VRLinkVerify(
        reflTxOperationalStatus >= DtOff
        && reflTxOperationalStatus <= DtOn,
        "Unexpected attribute value",
        __FILE__, __LINE__);

    VRLinkVerify(
        (txOperationalStatus <= DtOn),
        "invalid current radio tx state",
        __FILE__, __LINE__);


    txOperationalStatus = reflTxOperationalStatus;
}

// /**
// FUNCTION :: CbRadioAdded
// PURPOSE :: Callback for adding a new radio.
// PARAMETERS ::
// + radioTx : DtReflectedRadioTransmitter* : Radio to be added.
// + usr : void* : User data.
// RETURN :: void : NULL
// **/
void VRLink::CbRadioAdded(DtReflectedRadioTransmitter* radioTx, void* usr)
{
    VRLinkVerify(
        radioTx,
        "NULL reflected radio tx data received",
        __FILE__, __LINE__);

    VRLink* vrLinkPtr = (VRLink*)usr;

    vrLinkPtr->AddReflectedRadio(radioTx);
}

// /**
// FUNCTION :: CbRadioRemoved
// PURPOSE :: Callback for removing a radio.
// PARAMETERS ::
// + radioTx : DtReflectedRadioTransmitter* : Radio to be added.
// + usr : void* : User data.
// RETURN :: void : NULL
// **/
void VRLink::CbRadioRemoved(DtReflectedRadioTransmitter* radioTx, void* usr)
{
    VRLinkVerify(
        radioTx,
        "NULL reflected radio tx data received",
        __FILE__, __LINE__);

    VRLink* vrLinkPtr = (VRLink*)usr;

    vrLinkPtr->RemoveReflectedRadio(radioTx);
}




// /**
// FUNCTION :: CbAggregateAdded
// PURPOSE :: Callback for adding a new aggregate entity.
// PARAMETERS ::
// + agge : DtReflectedAggregate* : Aggregate to be added.
// + usr : void* : User data.
// RETURN :: void : NULL
// **/
void VRLink::CbAggregateAdded(DtReflectedAggregate* agge, void* usr)
{
    VRLinkVerify(
        agge,
        "NULL reflected aggregate entity data received",
        __FILE__, __LINE__);

    VRLink* vrLinkPtr = (VRLink*)usr;

    vrLinkPtr->AddReflectedAggregate(agge);
}

// /**
// FUNCTION :: CbAggregateRemoved
// PURPOSE :: Callback for removing a aggregate entity.
// PARAMETERS ::
// + agge : DtReflectedAggregate* : Aggregate to be removed.
// + usr : void* : User data.
// RETURN :: void : NULL
// **/
void VRLink::CbAggregateRemoved(DtReflectedAggregate* agge, void* usr)
{
    VRLinkVerify(
        agge,
        "NULL reflected aggregate entity data received",
        __FILE__, __LINE__);

    VRLink* vrLinkPtr = (VRLink*)usr;

    vrLinkPtr->RemoveReflectedAggregate(agge);
}


// /**
// FUNCTION :: AddReflectedAggregate
// PURPOSE :: Adds a new aggregate (called by CbAggregateAdded callback).
// PARAMETERS ::
// + agge : DtReflectedAggregate* : Aggregate to be added.
// RETURN :: void : NULL
// **/
void VRLink::AddReflectedAggregate(DtReflectedAggregate* agge)
{
    VRLinkAggregate* aggPtr = NULL;
    bool isFound = false;
    DtAggregateStateRepository* asr = agge->asr();

    std::vector<VRLinkAggregate*>::iterator it;

    for (it = refAggregateEntities.begin();
         it != refAggregateEntities.end();
         it++)
    {
        VRLinkAggregate* agge = *it;
        if (agge->GetEntityId() == asr->entityId())
        {
            isFound = true;
        }
    }

    if (!isFound)
    {
        aggPtr = new VRLinkAggregate;

        aggPtr->ReflectAttributes(agge);
        refAggregateEntities.push_back(aggPtr);


        VRLinkAggregateCbUsrData* usrData = new VRLinkAggregateCbUsrData(
                                                 this,
                                                 aggPtr);

        try
        {
            agge->addPostUpdateCallback(
                     &(VRLinkAggregate::CbReflectAttributeValues),
                     usrData);
        }
        DtCATCH_AND_WARN(cout);

    }
}


// /**
// FUNCTION :: RemoveReflectedAggregate
// PURPOSE :: Removes a aggregate (called by CbAggregateRemoved callback).
// PARAMETERS ::
// + agge : DtReflectedAggregate*& : aggregate entity to be removed.
// RETURN :: void : NULL
// **/
void VRLink::RemoveReflectedAggregate(DtReflectedAggregate* agge)
{
    DtAggregateStateRepository* asr = agge->asr();

    VRLinkAggregate* aggPtr = NULL;
    bool isFound = false;
    std::vector<VRLinkAggregate*>::iterator it;

    for (it = refAggregateEntities.begin();
         it != refAggregateEntities.end();
         it++)
    {
        VRLinkAggregate* agge = *it;
        if (agge->GetEntityId() == asr->entityId())
        {
            isFound = true;
            break;
        }
    }

    if (isFound)
    {
        aggPtr = *it;
        *it = NULL;
        delete aggPtr;

        agge->removePostUpdateCallback(
                 &(VRLinkAggregate::CbReflectAttributeValues),
                 this);

    }
    else
    {
         VRLinkVerify(
                aggPtr,
                "Reflected aggregate entity not found while removing",
                __FILE__, __LINE__);
    }
}




// /**
// FUNCTION :: RegisterCallbacks
// PURPOSE :: Registers the VRLink callbacks required for communication with
//            other federates i.e. send/receive data.
// PARAMETERS :: None.
// RETURN :: void : NULL.
// **/
void VRLink::RegisterCallbacks()
{
    try
    {
        refEntityList = new DtReflectedEntityList(exConn);

        refEntityList->setDiscoveryCondition(
                           VRLink::EntityCallbackCriteria,
                           NULL);

        refEntityList->addEntityAdditionCallback(
                           VRLink::CbEntityAdded,
                           this);

        refEntityList->addEntityRemovalCallback(
                           VRLink::CbEntityRemoved,
                           this);

        refRadioTxList = new DtReflectedRadioTransmitterList(exConn);

        refRadioTxList->setDiscoveryCondition(
                            VRLink::RadioCallbackCriteria,
                            NULL);

        refRadioTxList->addRadioTransmitterAdditionCallback(
                            VRLink::CbRadioAdded,
                            this);

        refRadioTxList->addRadioTransmitterRemovalCallback(
                            VRLink::CbRadioRemoved,
                            this);


        refAggregateList = new DtReflectedAggregateList(exConn);


        refAggregateList->addAggregateAdditionCallback(
                            VRLink::CbAggregateAdded,
                            this);


        refAggregateList->addAggregateRemovalCallback(
                            VRLink::CbAggregateRemoved,
                            this);

        RegisterProtocolSpecificCallbacks();
    }
    DtCATCH_AND_WARN(cout);
}


// /**
// FUNCTION :: DeregisterCallbacks
// PURPOSE :: Deregisters the VRLink callbacks required for communication with
//            other federates i.e. send/receive data.
// PARAMETERS :: None.
// RETURN :: void : NULL.
// **/
void VRLink::DeregisterCallbacks()
{

    refEntityList->removeEntityAdditionCallback(
                        VRLink::CbEntityAdded,
                        this);

    refEntityList->removeEntityRemovalCallback(
                        VRLink::CbEntityRemoved,
                        this);

    refRadioTxList->removeRadioTransmitterAdditionCallback(
                        VRLink::CbRadioAdded,
                        this);

    refRadioTxList->removeRadioTransmitterRemovalCallback(
                        VRLink::CbRadioRemoved,
                        this);

    refAggregateList->removeAggregateAdditionCallback(
                        VRLink::CbAggregateAdded,
                        this);

    refAggregateList->removeAggregateRemovalCallback(
                        VRLink::CbAggregateRemoved,
                        this);
}




// /**
//FUNCTION :: Tick
//PURPOSE :: Called from when all publishers need to tick
//PARAMETERS :: None
//RETURN :: void : NULL
// **/
void VRLink::Tick(clocktype simTime)
{
    double tmpTime = double(simTime) / SECOND;
    exConn->clock()->setSimTime(DtTime(tmpTime));
}


// /**
//FUNCTION :: VRLinkEntityCbUsrData
//PURPOSE :: Initialization function for VRLinkEntityCbUsrData
//PARAMETERS ::
// + vrLinkPtr : VRLink* : VRLink pointer.
// + entityPtr : VRLinkEntity* : VRLink entity pointer.
// **/
VRLinkEntityCbUsrData::VRLinkEntityCbUsrData(
    VRLink* vrLinkPtr,
    VRLinkEntity* entityPtr)
{
    vrLink = vrLinkPtr;
    entity = entityPtr;
}

// /**
//FUNCTION :: GetData
//PURPOSE :: Selector to return the variables of class VRLinkEntityCbUsrData.
//PARAMETERS ::
// + vrLinkPtr : VRLink** : VRLink pointer
// + entityPtr : VRLinkEntity* : VRLink entity pointer.
//RETURN :: void : NULL.
// **/
void VRLinkEntityCbUsrData::GetData(
    VRLink** vrLinkPtr,
    VRLinkEntity** entityPtr)
{
    *vrLinkPtr = vrLink;
    *entityPtr = entity;
}

// /**
//FUNCTION :: VRLinkRadioCbUsrData
//PURPOSE :: Initialization function for VRLinkRadioCbUsrData
//PARAMETERS ::
// + vrLinkPtr : VRLink* : VRLink pointer.
// + radioPtr : VRLinkRadio* : VRLink radio pointer.
// **/
VRLinkRadioCbUsrData::VRLinkRadioCbUsrData(
    VRLink* vrLinkPtr,
    VRLinkRadio* radioPtr)
{
    vrLink = vrLinkPtr;
    radio = radioPtr;
}

// /**
//FUNCTION :: GetData
//PURPOSE :: Selector to return the variables of class VRLinkRadioCbUsrData.
//PARAMETERS ::
// + vrLinkPtr : VRLink** : VRLink pointer
// + radioPtr : VRLinkRadio* : VRLink radio pointer.
//RETURN :: void : NULL.
// **/
void VRLinkRadioCbUsrData::GetData(
    VRLink** vrLinkPtr,
    VRLinkRadio** radioPtr)
{
    *vrLinkPtr = vrLink;
    *radioPtr = radio;
}


// /**
//FUNCTION :: VRLinkAggregateCbUsrData
//PURPOSE :: Initialization function for VRLinkAggregateCbUsrData
//PARAMETERS ::
// + vrLinkPtr : VRLink* : VRLink pointer.
// + aggePtr : VRLinkAggregate* : VRLink aggregate pointer.
// **/
VRLinkAggregateCbUsrData::VRLinkAggregateCbUsrData(
    VRLink* vrLinkPtr,
    VRLinkAggregate* aggePtr)
{
    vrLink = vrLinkPtr;
    agge = aggePtr;
}

// /**
//FUNCTION :: GetData
//PURPOSE :: Selector to return the variables of class VRLinkAggregateCbUsrData.
//PARAMETERS ::
// + vrLinkPtr : VRLink** : VRLink pointer
// + aggePtr : VRLinkAggregate* : VRLink aggregate pointer.
//RETURN :: void : NULL.
// **/
void VRLinkAggregateCbUsrData::GetData(
    VRLink** vrLinkPtr,
    VRLinkAggregate** aggePtr)
{
    *vrLinkPtr = vrLink;
    *aggePtr = agge;
}

map <RadioIdKey, VRLinkRadio*>* VRLink::GetVRLinkRadios()
{
    return &reflRadioIdToRadioMap;
}
