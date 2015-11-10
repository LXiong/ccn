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
#include "testfed_shared.h"
#include "fileio.h"
#include <cerrno>

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
    for (hash_map <string, DtEntityPublisher*>::iterator entityIt = entityPublishers.begin();
        entityIt != entityPublishers.end();
        entityIt++)
    {
        delete entityIt->second;
        entityIt->second = NULL;
    }

    //publish radios
    for (hash_map <unsigned, DtRadioTransmitterPublisher*>::iterator radioIt = radioPublishers.begin();
        radioIt != radioPublishers.end();
        radioIt++)
    {
        delete radioIt->second;
        radioIt->second = NULL;
    }
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
// FUNCTION :: InitConnectionSetting
// PURPOSE :: Initializing function for exercise connection settings
// PARAMETERS ::
// + connectionVar1 : char [] : connection setting variable 1
// + connectionVar2 : char [] : connection setting variable 2
// + connectionVar3 : char [] : connection setting variable 3
// + connectionVar4 : char [] : connection setting variable 4
// **/
void VRLink::InitConnectionSetting(char connectionVar1[],
                            char connectionVar2[],
                            char connectionVar3[],
                            char connectionVar4[])
{

}

// /**
// FUNCTION :: ReadEntitiesFile
// PURPOSE :: To parse the .vrlink-entities file.
// PARAMETERS ::
// RETURN :: void : NULL
// **/
void VRLink::ReadEntitiesFile()
{
    VRLinkVerify(
        entities.empty(),
        "Data already present in .entities file",
        __FILE__, __LINE__);

    // Determine path of .vrlink-entities file.
    char path[256];
    strcpy(path, m_scenarioName);
    strcat (path, ".vrlink-entities");

    // Determine number of lines in file.

    unsigned numEntities = GetNumLinesInFile(path);

    if (numEntities == 0) {
        return;
    }

    // Open file.

    FILE* fpEntities = fopen(path, "r");
    VRLinkVerify(fpEntities != NULL, "Can't open for reading", path);

    // Read file.

    char line[g_lineBufSize];
    for (unsigned i = 0;
         i < numEntities;
         i++)
    {
        VRLinkVerify(fgets(line, g_lineBufSize, fpEntities) != NULL,
                 "Not enough lines",
                 path);

        VRLinkVerify(strlen(line) < g_lineBufSize - 1,
                 "Exceeds permitted line length",
                 path);
                
        VRLinkEntity* entity = new VRLinkEntity;


        // parse record from file, and retrieve the
        // corresponding senderId(s)
        entity->ParseRecordFromFile(line);

        entity->SetEntityIdentifier(siteId, 
                                    applicationId,
                                    i + 1);

        hash_map <string, VRLinkEntity*>::iterator 
            entitiesIter = entities.find(entity->GetMarkingData());

        if (entitiesIter == entities.end())
        {
            entities[entity->GetMarkingData()] = entity;                     
        }
        else
        {
            VRLinkVerify(entitiesIter == entities.end(),
                   "Entity with duplicate MarkingData",
                    line, i + 1);
        }
    }
    fclose(fpEntities);
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
// FUNCTION :: ReadNetworksFile
// PURPOSE :: To parse the .vrlink-networks file.
// PARAMETERS ::
// + partitionData : PartitionData* : Pointer to partition data.
// + nodeInput : NodeInput* : Pointer to cached file.
// RETURN :: void : NULL
// **/
void VRLink::ReadNetworksFile()
{
    VRLinkVerify(
        networks.empty(),
        "Data already present in .networks file",
        __FILE__, __LINE__);

    char path[256];
    strcpy(path, m_scenarioName);
    strcat (path, ".vrlink-networks");

    unsigned numNetworks = GetNumLinesInFile(path);

    if (numNetworks == 0) {
        return;
    }

    FILE* fpNetworks = fopen(path, "r");
    VRLinkVerify(fpNetworks != NULL, "Can't open for reading", path);

    // Read file.

    hash_map <string, VRLinkNetwork*>::iterator networksIter;

    char line[g_lineBufSize];        
    for (unsigned i = 0;
         i < numNetworks;
         i++)
    {

        VRLinkVerify(fgets(line, g_lineBufSize, fpNetworks) != NULL,
                 "Not enough lines",
                 path);

        VRLinkVerify(strlen(line) < g_lineBufSize - 1,
                 "Exceeds permitted line length",
                 path);

        
        VRLinkNetwork* newNetwork = new VRLinkNetwork();

        //parse record from file, and retrieve the
        //corresponding senderId(s)
        newNetwork->ParseRecordFromFile(
                        line,
                        radios);

        networksIter = networks.find(newNetwork->GetName());

        if (networksIter == networks.end())
        {
            networks[newNetwork->GetName()] = newNetwork;
        }
        else
        {
            VRLinkReportError("Duplicate entry for network",
                                __FILE__, __LINE__);
        }
    }
    fclose(fpNetworks);
}

// /**
// FUNCTION :: ParseRecordFromFile
// PURPOSE :: To parse a record from the networks file.
// PARAMETERS ::
// + nodeInputStr : char* : Pointer to the record to be parsed.
// RETURN :: void : NULL.
// **/
void VRLinkNetwork::ParseRecordFromFile(
    char* nodeInputStr,
    hash_map <unsigned, VRLinkRadio*>& radioList)
{
    assert(radioList.size() > 0);

    char* tempStr = nodeInputStr;
    char* p = NULL;
    char fieldStr[MAX_STRING_LENGTH] = "";

    hash_map <unsigned, VRLinkRadio*>::const_iterator radiosPtrIter;

    p = IO_GetDelimitedToken(fieldStr , tempStr, ";", &tempStr);
    IO_TrimLeft(fieldStr);
    IO_TrimRight(fieldStr);
    VRLinkVerify(p != NULL, "Can't read network name", nodeInputStr);
    name = fieldStr;

    p = IO_GetDelimitedToken(fieldStr , tempStr, ";", &tempStr);
    IO_TrimLeft(fieldStr);
    IO_TrimRight(fieldStr);
    VRLinkVerify(p != NULL, "Can't read network frequency", nodeInputStr);
    int retVal = atouint64(fieldStr, &frequency);
    VRLinkVerify(retVal == 1, "Can't parse network frequency", nodeInputStr);

    //parsing node list
    p = IO_GetDelimitedToken(fieldStr, tempStr, ";", &tempStr);
        VRLinkVerify(p != NULL, "Can't read list of nodeIds", fieldStr);

    numRadioPtrs = 0;

    char nodeIdToken[512];
    char* nextNodeIdToken;

    nextNodeIdToken = fieldStr;
    while (IO_GetDelimitedToken(nodeIdToken, nextNodeIdToken, ",",
                                &nextNodeIdToken))
    {
        VRLinkVerify(
            numRadioPtrs < g_vrlinkMaxMembersInNetwork,
            "Exceeded maximum number of nodes for network",
            fieldStr);

        char* endPtr = NULL;
        errno = 0;
        unsigned nodeId = (unsigned) strtoul(nodeIdToken, &endPtr, 10);
        VRLinkVerify(
            endPtr != nodeIdToken && errno == 0,
            "Couldn't parse nodeId",
            fieldStr);
                
        hash_map <unsigned, VRLinkRadio*>::iterator 
            radioIter = radioList.find(nodeId);

        VRLinkVerify(
            radioIter != radioList.end(),
            "Can't find nodeId in list of radios",
            fieldStr);
        
        VRLinkRadio* radio = radioIter->second;

        radioPtrs[numRadioPtrs++] = radio;

        VRLinkVerify(
            radio->GetNetworkPtr() == NULL,
            "nodeId present in more than one network",
            fieldStr);

        radio->SetNetworkPtr(this);
    }

    VRLinkVerify(numRadioPtrs > 0,
             "Network must have at least one node",
             fieldStr);
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
// FUNCTION :: ReadRadiosFile
// PURPOSE :: To parse the .vrlink-radios file.
// PARAMETERS ::
// RETURN :: void : NULL
// **/
void VRLink::ReadRadiosFile()
{
    VRLinkVerify(
        radios.empty(),
        "Data already present in .radios file",
        __FILE__, __LINE__);


     // Determine path of .hla-radios file.

    char path[256];
    strcpy(path, m_scenarioName);
    strcat (path, ".vrlink-radios");

    // Determine number of lines in file.

    unsigned numRadios = GetNumLinesInFile(path);

    if (numRadios == 0) {
        return;
    }

    // Open file.

    FILE* fpRadios = fopen(path, "r");
    VRLinkVerify(fpRadios != NULL, "Can't open for reading", path);

    // Read file.

    char line[g_lineBufSize];
    for (unsigned i = 0;
         i < numRadios;
         i++)
    {
        VRLinkVerify(fgets(line, g_lineBufSize, fpRadios) != NULL,
                 "Not enough lines",
                 path);

        VRLinkVerify(strlen(line) < g_lineBufSize - 1,
                 "Exceeds permitted line length",
                 path);

        
        VRLinkRadio* newRadio = new VRLinkRadio();

        //parse record from file, and retrieve the
        //corresponding senderId(s)
        newRadio->ParseRecordFromFile(line);

        hash_map <unsigned, VRLinkRadio*>::iterator 
            radioIter = radios.find(newRadio->GetNodeId());

        if (radioIter == radios.end())
        {
            radios[newRadio->GetNodeId()] = newRadio;
        }
        else
        {
            VRLinkVerify(radioIter == radios.end(),
                   "Radio with duplicate NodeId",
                    line, i + 1);
        }

        hash_map <string, VRLinkEntity*>::iterator 
            entitiesIter = entities.find(newRadio->GetMarkingData());

        if (entitiesIter == entities.end())
        {
            VRLinkVerify(entitiesIter == entities.end(),
                   "Can't find entity with MarkingData",
                    line, i + 1);
        }
        else
        {
            newRadio->SetEntityPtr(entitiesIter->second);
        }
        
        VRLinkEntity* entity = entitiesIter->second;
        RadioKey radioKey;
        strcpy(radioKey.markingData, entity->GetMarkingData().c_str());
        radioKey.radioIndex = newRadio->GetRadioIndex();

        map <RadioKey, VRLinkRadio*>::iterator 
            radioIter2 = radioKeyToRadios.find(radioKey);
        if (radioIter2 == radioKeyToRadios.end())
        {
            radioKeyToRadios[radioKey] = newRadio;
        }
        else
        {
            VRLinkVerify(radioIter2 == radioKeyToRadios.end(),
                   "Radio with duplicate markingData and radioIndex",
                    line, i + 1);
        }

        entity->AddRadio(newRadio);
    }
    fclose(fpRadios);
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
// FUNCTION :: AddRadio
// PURPOSE :: Add a new radio child/reference to a specific entity
// PARAMETERS :: VRLinkRadio* : radio : radio begin added
// RETURN :: void : NULL
// **/
void VRLinkEntity::AddRadio(VRLinkRadio* radio)
{
    VRLinkVerify(numRadioPtrs < g_vrlinkMaxRadiosPerEntity,
         "Exceeded max radios per entity");
    radioPtrs[numRadioPtrs++] = radio;
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
// FUNCTION :: ParseRecordFromFile
// PURPOSE :: To parse a record from the radios file.
// PARAMETERS ::
// + nodeInputStr : char* : Pointer to the record to be parsed.
// RETURN :: void : NULL.
// **/
void VRLinkRadio::ParseRecordFromFile(char* nodeInputStr)
{
    char* tempStr = nodeInputStr;
    char* p = NULL;
    char fieldStr[MAX_STRING_LENGTH] = "";

    p = IO_GetDelimitedToken(fieldStr, tempStr, ",", &tempStr);
    IO_TrimLeft(fieldStr);
    IO_TrimRight(fieldStr);
    VRLinkVerify(p != NULL, "Can't read nodeId", nodeInputStr);
    nodeId = atoi(fieldStr);

    p = IO_GetDelimitedToken(fieldStr, tempStr, ",", &tempStr);
    IO_TrimLeft(fieldStr);
    IO_TrimRight(fieldStr);
    markingData = fieldStr;
    VRLinkVerify(p != NULL, "Can't read markingData", nodeInputStr);

    p = IO_GetDelimitedToken(fieldStr, tempStr, ",", &tempStr);
    IO_TrimLeft(fieldStr);
    IO_TrimRight(fieldStr);
    VRLinkVerify(p != NULL, "Can't read RadioIndex", nodeInputStr);
    radioIndex = atoi(fieldStr);

    p = IO_GetDelimitedToken(fieldStr, tempStr, ",", &tempStr);
    IO_TrimLeft(fieldStr);
    IO_TrimRight(fieldStr);
    VRLinkVerify(p != NULL, "Can't read RelativePosition (x)", nodeInputStr);
    relativePosition.setX(atof(fieldStr));

    p = IO_GetDelimitedToken(fieldStr, tempStr, ",", &tempStr);
    IO_TrimLeft(fieldStr);
    IO_TrimRight(fieldStr);
    VRLinkVerify(p != NULL, "Can't read RelativePosition (y)", nodeInputStr);
    relativePosition.setY(atof(fieldStr));

    p = IO_GetDelimitedToken(fieldStr, tempStr, ",", &tempStr);
    IO_TrimLeft(fieldStr);
    IO_TrimRight(fieldStr);
    VRLinkVerify(p != NULL, "Can't read RelativePosition (z)", nodeInputStr);
    relativePosition.setZ(atof(fieldStr));

    char* endPtr = NULL;
    errno = 0;

    // The seven fields of the RadioSystemType attribute follow.

    // EntityKind.

    p = IO_GetDelimitedToken(fieldStr, tempStr, ",", &tempStr);
    VRLinkVerify(p != NULL, "Can't read EntityKind", nodeInputStr);

    errno = 0;
    int kind = (int) strtoul(fieldStr, &endPtr, 10);

    VRLinkVerify(
        endPtr != fieldStr && errno == 0,
        "Can't parse EntityKind",
        nodeInputStr);

    // Domain.

    p = IO_GetDelimitedToken(fieldStr, tempStr, ",", &tempStr);
    VRLinkVerify(p != NULL, "Can't read Domain", nodeInputStr);

    errno = 0;
    int domain = (int) strtoul(fieldStr, &endPtr, 10);

    VRLinkVerify(
        endPtr != fieldStr && errno == 0,
        "Can't parse Domain",
        nodeInputStr);

    // CountryCode.

    p = IO_GetDelimitedToken(fieldStr, tempStr, ",", &tempStr);
    VRLinkVerify(p != NULL, "Can't read CountryCode", nodeInputStr);

    errno = 0;
    int countryCode = (int) strtoul(fieldStr, &endPtr, 10);

    VRLinkVerify(
        endPtr != fieldStr && errno == 0,
        "Can't parse CountryCode",
        nodeInputStr);

    // Category.

    p = IO_GetDelimitedToken(fieldStr, tempStr, ",", &tempStr);
    VRLinkVerify(p != NULL, "Can't read Category", nodeInputStr);

    errno = 0;
    int category = (int) strtoul(fieldStr, &endPtr, 10);

    VRLinkVerify(
        endPtr != fieldStr && errno == 0,
        "Can't parse Category",
        nodeInputStr);

    // NomenclatureVersion.

    p = IO_GetDelimitedToken(fieldStr, tempStr, ",", &tempStr);
    VRLinkVerify(p != NULL, "Can't read NomenclatureVersion",
              nodeInputStr);

    errno = 0;
    int nomenclatureVersion = (int) strtoul(fieldStr, &endPtr, 10);

    VRLinkVerify(
        endPtr != fieldStr && errno == 0,
        "Can't parse NomenclatureVersion",
        nodeInputStr);

    // Nomenclature.

    p = IO_GetDelimitedToken(fieldStr, tempStr, ",", &tempStr);
    VRLinkVerify(p != NULL, "Can't read Nomenclature", nodeInputStr);

    errno = 0;
    int nomenclature = (int) strtoul(fieldStr, &endPtr, 10);
    VRLinkVerify(
        endPtr != fieldStr && errno == 0,
        "Can't parse Nomenclature",
        nodeInputStr);

    radioType = DtRadioEntityType(kind, domain, countryCode, category,
        nomenclatureVersion, nomenclature);
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
// FUNCTION :: GetRadioType
// PURPOSE :: Selector to get the radio system type of a radio
// PARAMETERS :: None.
// RETURN : DtRadioEntityType
// **/
DtRadioEntityType VRLinkRadio::GetRadioType()
{
    return radioType;
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
    velocity.zero();
}

// /**
// FUNCTION :: ParseRecordFromFile
// PURPOSE :: To parse a record from the entities file.
// PARAMETERS ::
// + nodeInputStr : char* : Pointer to the record to be parsed.
// RETURN :: void : NULL.
// **/
void VRLinkEntity::ParseRecordFromFile(char* nodeInputStr)
{
    char* tempStr = nodeInputStr;
    char* p = NULL;
    char fieldStr[MAX_STRING_LENGTH] = "";

    p = IO_GetDelimitedToken(fieldStr , tempStr, ",", &tempStr);
    IO_TrimLeft(fieldStr);
    IO_TrimRight(fieldStr);
    VRLinkVerify(p != NULL, "Can't read markingData", nodeInputStr);
    markingData = fieldStr;

    p = IO_GetDelimitedToken(fieldStr, tempStr, ",", &tempStr);
    IO_TrimLeft(fieldStr);
    IO_TrimRight(fieldStr);
    VRLinkVerify(p != NULL, "Can't read ForceID", nodeInputStr);
    switch(fieldStr[0])
    {
        case 'F':
            forceType = DtForceFriendly;
            break;
        case 'O':
            forceType = DtForceOpposing;
            break;
        case 'N':
            forceType = DtForceNeutral;
            break;
        default:
            VRLinkReportError("Invalid ForceID", nodeInputStr);
    }

    // Country string (skip, since the numeric value is in EntityType).
    p = IO_GetDelimitedToken(fieldStr, tempStr, ",", &tempStr);
    VRLinkVerify(p != NULL, "Can't read country string", nodeInputStr);

    p = IO_GetDelimitedToken(fieldStr, tempStr, ",", &tempStr);
    IO_TrimLeft(fieldStr);
    IO_TrimRight(fieldStr);
    VRLinkVerify(p != NULL, "Can't read latitude", nodeInputStr);
    latLonAlt.setX(atof(fieldStr));

    p = IO_GetDelimitedToken(fieldStr, tempStr, ",", &tempStr);
    IO_TrimLeft(fieldStr);
    IO_TrimRight(fieldStr);
    VRLinkVerify(p != NULL, "Can't read longitude", nodeInputStr);
    latLonAlt.setY(atof(fieldStr));

    p = IO_GetDelimitedToken(fieldStr, tempStr, ",", &tempStr);
    IO_TrimLeft(fieldStr);
    IO_TrimRight(fieldStr);
    VRLinkVerify(p != NULL, "Can't read altitude", nodeInputStr);
    latLonAlt.setZ(atof(fieldStr));

    //latLonAltScheduled = latLonAlt;

    VRLinkVerify(
         latLonAlt.x() >= -90.0 && latLonAlt.x() <= 90.0
         && latLonAlt.y() >= -180.0 && latLonAlt.y() <= 180.0,
         "Invalid geodetic coordinates", nodeInputStr);

    ConvertLatLonAltToGcc(latLonAlt, xyz);
    //xyzScheduled = xyz;

    // The seven fields of the EntityType attribute follow.
    // EntityKind.

    p = IO_GetDelimitedToken(fieldStr, tempStr, ",", &tempStr);
    IO_TrimLeft(fieldStr);
    IO_TrimRight(fieldStr);
    VRLinkVerify(p != NULL, "Can't read EntityKind", nodeInputStr);
    type.setKind(atoi(fieldStr));

    // Domain.

    p = IO_GetDelimitedToken(fieldStr, tempStr, ",", &tempStr);
    IO_TrimLeft(fieldStr);
    IO_TrimRight(fieldStr);
    VRLinkVerify(p != NULL, "Can't read Domain", nodeInputStr);
    type.setDomain(atoi(fieldStr));

    // CountryCode.

    p = IO_GetDelimitedToken(fieldStr, tempStr, ",", &tempStr);
    IO_TrimLeft(fieldStr);
    IO_TrimRight(fieldStr);
    VRLinkVerify(p != NULL, "Can't read CountryCode", nodeInputStr);
    type.setCountry(atoi(fieldStr));

    // Category.

    p = IO_GetDelimitedToken(fieldStr, tempStr, ",", &tempStr);
    IO_TrimLeft(fieldStr);
    IO_TrimRight(fieldStr);
    VRLinkVerify(p != NULL, "Can't read Category", nodeInputStr);
    type.setCategory(atoi(fieldStr));

    // Subcategory.

    p = IO_GetDelimitedToken(fieldStr, tempStr, ",", &tempStr);
    IO_TrimLeft(fieldStr);
    IO_TrimRight(fieldStr);
    VRLinkVerify(p != NULL, "Can't read Subcategory", nodeInputStr);
    type.setSubCategory(atoi(fieldStr));

    // Specific.

    p = IO_GetDelimitedToken(fieldStr, tempStr, ",", &tempStr);
    IO_TrimLeft(fieldStr);
    IO_TrimRight(fieldStr);
    VRLinkVerify(p != NULL, "Can't read Specific", nodeInputStr);
    type.setSpecific(atoi(fieldStr));

    // Extra.

    p = IO_GetDelimitedToken(fieldStr, tempStr, ",", &tempStr);
    IO_TrimLeft(fieldStr);
    IO_TrimRight(fieldStr);
    VRLinkVerify(p != NULL, "Can't read Extra", nodeInputStr);
    type.setExtra(atoi(fieldStr));
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
// FUNCTION :: GetVelocity
// PURPOSE :: Selector to get the velocity of an entity (GCC coordinate system)
// PARAMETERS :: None.
// RETURN : DtVector
// **/
DtVector VRLinkEntity::GetVelocity()
{
    return velocity;
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
// FUNCTION :: SetVelocity
// PURPOSE :: Set the velocity of an entity
// PARAMETERS :: DtVector : val : new velocity value
// RETURN : void : NULL
// **/
void VRLinkEntity::SetVelocity(DtVector val)
{
    velocity = val;
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
// FUNCTION :: VRLinkRadio
// PURPOSE :: Initializing function for VRLink radio.
// PARAMETERS :: None.
// **/
VRLinkRadio::VRLinkRadio()
{
    entityPtr = NULL;
    networkPtr = NULL;

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
//FUNCTION :: PublishObjects
//PURPOSE :: Entity/Radio vrlink publisher that is independent of any protocol
//PARAMETERS :: None
//RETURN :: void : NULL
// **/
void VRLink::PublishObjects()
{
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

void Hton(void* ptr, unsigned size)
{
    char t;
    unsigned int i;
    char *a = (char*) ptr;

    unsigned int hostInt = 0xfeedbeef;
    unsigned int netInt = htonl(hostInt);

    if (hostInt != netInt)
    {
        for (i = 0; i < size / 2; i++)
        {
            t = a[i];
            a[i] = a[size - 1 - i];
            a[size - 1 - i] = t;
        }
    }
}

void Ntoh(void* ptr, unsigned size)
{
    assert(size <= 256);
    if (size <= 1) return;

#if defined(_M_IX86) || defined(__i386__)
    unsigned char* a = (unsigned char*) ptr;
    unsigned char b[256];

    unsigned i;
    for (i = 0; i < size; i++)
    {
        b[i] = a[size - 1 - i];
    }

    for (i = 0; i < size; i++)
    {
        a[i] = b[i];
    }
#endif /* _M_IX86, __i386 */
}

unsigned GetTimestamp()
{
    return ConvertDoubleToTimestamp(GetNumSecondsPastHour(), 1);
}

double ConvertTimestampToDouble(unsigned timestamp)
{
    // Convert DIS timestamp to seconds.

    // It is NOT guaranteed that converting from one type to the other,
    // then back to the original type, will provide the original value.

    timestamp >>= 1;
    return ((double) timestamp) / g_vrlinkTimestampRatio;
}

unsigned ConvertDoubleToTimestamp(double double_timestamp, bool absolute)
{
    // Convert seconds to DIS timestamp.

    // It is NOT guaranteed that converting from one type to the other,
    // then back to the original type, will provide the original value.

    // DIS timestamps are flipped to 0 after one hour (3600 seconds).

    assert(double_timestamp < 3600.0);
    unsigned timestamp = (unsigned) (double_timestamp * g_vrlinkTimestampRatio);

    // Check high-order bit is still 0.

    assert((timestamp & 0x80000000) == 0);
    timestamp <<= 1;

    if (absolute) { return timestamp |= 1; }
    else { return timestamp; }
}

double GetNumSecondsPastHour()
{
    time_t timeValue;
    unsigned numMicroseconds;

#ifdef _WIN32
    _timeb ftimeValue;
    _ftime(&ftimeValue);
    timeValue = ftimeValue.time;
    numMicroseconds = ftimeValue.millitm * 1000;
#else /* _WIN32 */
    timeval gettimeofdayValue;
    gettimeofday(&gettimeofdayValue, NULL);
    timeValue = (time_t) gettimeofdayValue.tv_sec;
    numMicroseconds = gettimeofdayValue.tv_usec;
#endif /* _WIN32 */

    tm* localtimeValue = localtime(&timeValue);

    double numSecondsPastHour
        = ((double) ((localtimeValue->tm_min * 60) + localtimeValue->tm_sec))
          + ((double) numMicroseconds) / 1e6;

    return numSecondsPastHour;
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
// FUNCTION :: CbDataIxnReceived
// PURPOSE :: Callback to receive data interaction.
// PARAMETERS ::
// + inter : DtDataInteraction* : Received data interaction.
// + usr : void* : User data.
// RETURN :: void : NULL
// **/
void VRLink::CbDataIxnReceived(DtDataInteraction* data, void* usr)
{  
    if (data->numVarFields() == 1)
    {
        int varDatumId = (int) data->datumId(DtVar, 1);

        if (varDatumId == processMsgNotificationDatumId)
        {
            //This format needs to change to match old testfed output
#if DtDIS
            cout << "Received Data PDU - Process MSG Notify: {\n";
#else
            cout << "Received Data Interaction - Process MSG Notify: {\n";
#endif

            const char* msg = data->datumValByteArray(DtVar, 1);
            unsigned short site, host, num;

            memcpy(&site, msg, sizeof(site));
            memcpy(&host, msg+2, sizeof(host));
            memcpy(&num, msg+4, sizeof(num));

            Hton(&site, sizeof(site));
            Hton(&host, sizeof(host));
            Hton(&num, sizeof(num));

            unsigned short srcRadioIndex;
            memcpy(&srcRadioIndex, msg+6, sizeof(srcRadioIndex));
            Hton(&srcRadioIndex, sizeof(srcRadioIndex));

            std::cout << "Sender (site: " << site << " host: " << host << 
                " num: " << num << " radioIndex: " << srcRadioIndex << ")\n";

            unsigned long timestamp;
            memcpy(&timestamp, msg+8, sizeof(timestamp));
            Hton(&timestamp, sizeof(timestamp));

            unsigned numEntityIds;
            memcpy(&numEntityIds, msg+12, sizeof(numEntityIds));
            Hton(&numEntityIds, sizeof(numEntityIds));

            std::cout << "timestamp: " << timestamp << " numReceivingEntities: " << numEntityIds << "\n";

            memcpy(&site, msg+16, sizeof(site));
            memcpy(&host, msg+18, sizeof(host));
            memcpy(&num, msg+20, sizeof(num));

            Hton(&site, sizeof(site));
            Hton(&host, sizeof(host));
            Hton(&num, sizeof(num));

            double delay;
            memcpy(&delay, msg+22, sizeof(delay));
            Hton(&delay, sizeof(delay));

            std::cout << "Receiver (site: " << site << " host: " << host << 
                " num: " << num << " delay: " << delay << ")\n";

            std::cout << "Size of Msg in bytes: " << data->datumSizeBytes(DtVar, 1);
            std::cout << "\n}\n";

        }
        else if (varDatumId == timeoutNotificationDatumId)
        {
#if DtDIS
            cout << "Received Data PDU - Timeout Notify: {\n";
#else
            cout << "Received Data Interaction - Timeout Notify: {\n";
#endif

            const char* msg = data->datumValByteArray(DtVar, 1);
            unsigned short site, host, num;

            memcpy(&site, msg, sizeof(site));
            memcpy(&host, msg+2, sizeof(host));
            memcpy(&num, msg+4, sizeof(num));

            Hton(&site, sizeof(site));
            Hton(&host, sizeof(host));
            Hton(&num, sizeof(num));

            unsigned short srcRadioIndex;
            memcpy(&srcRadioIndex, msg+6, sizeof(srcRadioIndex));
            Hton(&srcRadioIndex, sizeof(srcRadioIndex));

            std::cout << "Sender (site: " << site << " host: " << host << 
                " num: " << num << " radioIndex: " << srcRadioIndex << ")\n";

            unsigned long timestamp;
            memcpy(&timestamp, msg+8, sizeof(timestamp));
            Hton(&timestamp, sizeof(timestamp));

            unsigned numPackets;
            memcpy(&numPackets, msg+12, sizeof(numPackets));
            Hton(&numPackets, sizeof(numPackets));

            unsigned numEntityIds;
            memcpy(&numEntityIds, msg+16, sizeof(numEntityIds));
            Hton(&numEntityIds, sizeof(numEntityIds));

            std::cout << "timestamp: " << timestamp << " numPacket: " << numPackets <<
                " numReceivingEntities: " << numEntityIds << "\n";

            bool success;
            int offset;
            for (unsigned i = 0; i < numEntityIds; i++)
            {
                offset = 20 + (7*i);

                memcpy(&site, msg+offset, sizeof(site));
                memcpy(&host, msg+(2+offset), sizeof(host));
                memcpy(&num, msg+(4+offset), sizeof(num));
            
                Hton(&site, sizeof(site));
                Hton(&host, sizeof(host));
                Hton(&num, sizeof(num));

                memcpy(&success, msg+(6+offset), sizeof(success));
                Hton(&success, sizeof(success));
                
                std::cout << "Receiver (site: " << site << " host: " << host << 
                    " num: " << num << " success: " << success << ")\n";
            }

            std::cout << "Size of Msg in bytes: " << data->datumSizeBytes(DtVar, 1);
            std::cout << "\n}\n";
        }
        else if (varDatumId == readyToSendSignalNotificationDatumId)
        {
#if DtDIS
            cout << "Received Data PDU - RTSS Notify: {\n";
#else
            cout << "Received Data Interaction - RTSS Notify: {\n";
#endif

            const char* msg = data->datumValByteArray(DtVar, 1);
            unsigned short site, host, num;

            memcpy(&site, msg, sizeof(site));
            memcpy(&host, msg+2, sizeof(host));
            memcpy(&num, msg+4, sizeof(num));

            Hton(&site, sizeof(site));
            Hton(&host, sizeof(host));
            Hton(&num, sizeof(num));

            unsigned short srcRadioIndex;
            memcpy(&srcRadioIndex, msg+6, sizeof(srcRadioIndex));
            Hton(&srcRadioIndex, sizeof(srcRadioIndex));

            std::cout << "Sender (site: " << site << " host: " << host << 
                " num: " << num << " radioIndex: " << srcRadioIndex << ")\n";

            unsigned timestamp;
            memcpy(&timestamp, msg+8, sizeof(timestamp));
            Hton(&timestamp, sizeof(timestamp));

            float windowTime; 
            memcpy(&windowTime, msg+12, sizeof(windowTime));
            Hton(&windowTime, sizeof(windowTime));
           
            std::cout << "timestamp: " << timestamp << " windowTime: " << windowTime << "\n";

            std::cout << "Size of Msg in bytes: " << data->datumSizeBytes(DtVar, 1);
            std::cout << "\n}\n";
        }
    }
}

// /**
// FUNCTION :: CbCommentIxnReceived
// PURPOSE :: Callback to receive comment interaction.
// PARAMETERS ::
// + inter : DtCommentInteraction* : Received comment interaction.
// + usr : void* : User data.
// RETURN :: void : NULL
// **/
void VRLink::CbCommentIxnReceived(DtCommentInteraction* comment, void* usr)
{
    if (comment->numVarFields() == 1)
    {
        int varDatumId = (int) comment->datumId(DtVar, 1);
        int size = comment->datumSizeBytes(DtVar, 1);
        char msg[992];  //max comment size
        memcpy (msg, comment->datumValByteArray(DtVar, 1), size);

        if (varDatumId == nodeIdDescriptionNotificationDatumId)
        {
            std::cout << "Received Comment Interaction - NodeIDDescription: {\n";
            std::cout << msg;
            std::cout << "}\n";
        }
        else if (varDatumId == metricDefinitionNotificationDatumId)
        {
            std::cout << "Received Comment Interaction - MetricDefinition: {\n";
            std::cout << msg;
            std::cout << "}\n";
        }
        else if (varDatumId == metricUpdateNotificationDatumId)
        {
            std::cout << "Received Comment Interaction - MetricUpdate: {\n";
            std::cout << msg;
            std::cout << "}\n";
        }
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
    DtDataInteraction::addCallback(
                           exConn,
                           VRLink::CbDataIxnReceived,
                           this);

    DtCommentInteraction::addCallback(
                           exConn,
                           VRLink::CbCommentIxnReceived,
                           this);
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
    DtDataInteraction::removeCallback(
                           exConn,
                           VRLink::CbDataIxnReceived,
                           this);

    DtCommentInteraction::removeCallback(
                           exConn,
                           VRLink::CbCommentIxnReceived,
                           this);
}

// /**
//FUNCTION :: MoveEntity
//PURPOSE :: Called from testfed app when handling a move entity command,
//           set the world position for the entity state repository, and calls tick
//           on the publisher object
//PARAMETERS :: unsigned : nodeId : node Id of parent entity object
//              double : lat : new value of lat
//              double : lon : new value of lon
//              double : alt : new value of alt
//RETURN :: void : NULL
// **/
void VRLink::MoveEntity(unsigned nodeId,
        double lat,
        double lon,
        double alt)
{    
    hash_map <unsigned, VRLinkRadio*>::iterator 
        radioIter = radios.find(nodeId);
   
    VRLinkVerify(radioIter != radios.end(),
        "Cannot find radio for requested nodeId", __FILE__, __LINE__);

    VRLinkVerify(radioIter->second->GetEntityPtr() != NULL,
        "No entity associated with radio for requested nodeId", __FILE__, __LINE__);

    VRLinkEntity* entity = radioIter->second->GetEntityPtr();

    DtVector newLatLon(lat, lon, alt);
    DtVector newXYZ;

    ConvertLatLonAltToGcc(newLatLon, newXYZ);

    if (newXYZ != entity->GetXYZ())
    {
        //value has changed, so update the state repository
        // store that entity publisher
        hash_map <string, DtEntityPublisher*>::iterator 
            entitiesPubIter = entityPublishers.find(entity->GetMarkingData());

        VRLinkVerify(entitiesPubIter != entityPublishers.end(),
            "Cannot find entity publisher for requested nodeId, registerObjects failed",
            __FILE__, __LINE__);

        // Hold on to the entity's state repository, where we can set data.
        DtEntityStateRepository* esr = entitiesPubIter->second->entityStateRep();

        esr->setLocation(newXYZ);
        
        entitiesPubIter->second->tick();

        entity->SetLatLon(newLatLon);
        entity->SetXYZ(newXYZ);
    }

     cout << "Moved (" << entity->GetEntityIdString() << ") "
        << entity->GetMarkingData() << " to ("
        << newLatLon.x() << ", "
        << newLatLon.y() << ", "
        << newLatLon.z() << ")." << endl;
}

// /**
//FUNCTION :: ChangeEntityOrientation
//PURPOSE :: Called from testfed app when handling a change orientation command,
//           set the orientation for the entity state repository, and calls tick
//           on the publisher object
//PARAMETERS :: unsigned : nodeId : node Id of parent entity object
//              double : orientationPsiDegrees : new value of psi
//              double : orientationThetaDegrees : new value of theta
//              double : orientationPhiDegrees : new value of phi
//RETURN :: void : NULL
// **/
void VRLink::ChangeEntityOrientation(unsigned nodeId,        
        double orientationPsiDegrees,
        double orientationThetaDegrees,
        double orientationPhiDegrees)
{
    hash_map <unsigned, VRLinkRadio*>::iterator 
        radioIter = radios.find(nodeId);
   
    VRLinkVerify(radioIter != radios.end(),
        "Cannot find radio for requested nodeId", __FILE__, __LINE__);

    VRLinkVerify(radioIter->second->GetEntityPtr() != NULL,
        "No entity associated with radio for requested nodeId", __FILE__, __LINE__);

    VRLinkEntity* entity = radioIter->second->GetEntityPtr();

    DtTaitBryan newOrientation = DtTaitBryan(orientationPsiDegrees,
        orientationThetaDegrees,
        orientationPhiDegrees);

    if (newOrientation != entity->GetOrientation())
    {
        //value has changed, so update the state repository
        // store that entity publisher
        hash_map <string, DtEntityPublisher*>::iterator 
            entitiesPubIter = entityPublishers.find(entity->GetMarkingData());

        VRLinkVerify(entitiesPubIter != entityPublishers.end(),
            "Cannot find entity publisher for requested nodeId, registerObjects failed",
            __FILE__, __LINE__);

        // Hold on to the entity's state repository, where we can set data.
        DtEntityStateRepository* esr = entitiesPubIter->second->entityStateRep();

        // IMPORTANT:
        // DtEntityStateRepository::setOrientation(...) expects
        // all TaitBryan angles to be represented in RADIANS.
        esr->setOrientation(newOrientation);
        entitiesPubIter->second->tick();

        entity->SetOrientation(newOrientation);
    }

    cout << "Changed Orientation (" << entity->GetEntityIdString() << ") "
        << entity->GetMarkingData() << " to ("
        << newOrientation.psi() << ", "
        << newOrientation.theta() << ", "
        << newOrientation.phi() << ")." << endl;
}

// /**
//FUNCTION :: SendCommEffectsRequest
//PURPOSE :: sends signal interaction/pdu for a cer
//PARAMETERS :: bool : isData : whether the CER is data only
//              unsigned : msgSize : size of the message, either bytes or secs
//              double : timeout : timeout delay for the CER
//              bool : dstNodeIdPresent : whether a destination node was specified
//              unsigned : dstNodeId : node ID of destination node
//RETURN :: void : NULL
// **/
void VRLink::SendCommEffectsRequest(unsigned   srcNodeId,
        bool      isData,
        unsigned   msgSize,
        double     timeoutDelay,
        bool       dstNodeIdPresent,
        unsigned   dstNodeId)
{
    //Go to Protocol specific implementations
}

void VRLink::ChangeDamageState(unsigned nodeId, 
        unsigned damageState)
{
    hash_map <unsigned, VRLinkRadio*>::iterator 
        radioIter = radios.find(nodeId);
   
    VRLinkVerify(radioIter != radios.end(),
        "Cannot find radio for requested nodeId", __FILE__, __LINE__);

    VRLinkVerify(radioIter->second->GetEntityPtr() != NULL,
        "No entity associated with radio for requested nodeId", __FILE__, __LINE__);

    VRLinkEntity* entity = radioIter->second->GetEntityPtr();

    if (DtDamageState(damageState) != entity->GetDamageState())
    {
        //value has changed, so update the state repository
        // store that entity publisher
        hash_map <string, DtEntityPublisher*>::iterator 
            entitiesPubIter = entityPublishers.find(entity->GetMarkingData());

        VRLinkVerify(entitiesPubIter != entityPublishers.end(),
            "Cannot find entity publisher for requested nodeId, registerObjects failed",
            __FILE__, __LINE__);

        // Hold on to the entity's state repository, where we can set data.
        DtEntityStateRepository* esr = entitiesPubIter->second->entityStateRep();

        esr->setDamageState(DtDamageState(damageState));
        entitiesPubIter->second->tick();

        entity->SetDamageState(DtDamageState(damageState));
    }

    cout << "Set (" << entity->GetEntityIdString()
        << ") " << entity->GetMarkingData()
        << " DamageState to " << damageState << "." << endl;
}

// /**
//FUNCTION :: ChangeTxOperationalStatus
//PURPOSE :: Called from testfed app when handling a change tx state command,
//           set the tx state for the radio tx state repository, and calls tick
//           on the publisher object
//PARAMETERS :: unsigned : nodeId : node Id of radio
//              unsigned char : txOperationalStatus : new value of tx state
//RETURN :: void : NULL
// **/
void VRLink::ChangeTxOperationalStatus(unsigned nodeId,
        unsigned char txOperationalStatus)
{
    hash_map <unsigned, VRLinkRadio*>::iterator 
        radioIter = radios.find(nodeId);
   
    VRLinkVerify(radioIter != radios.end(),
        "Cannot find radio for requested nodeId", __FILE__, __LINE__);

    VRLinkRadio* radio = radioIter->second;
    VRLinkEntity* entity = radioIter->second->GetEntityPtr();

    if (DtRadioTransmitState(txOperationalStatus) != radio->GetTransmitState())
    {   
        //value has changed, so update the state repository for the radio transmitter pub
        hash_map <unsigned, DtRadioTransmitterPublisher*>::iterator 
            radiosPubIter = radioPublishers.find(nodeId);

        VRLinkVerify(radiosPubIter != radioPublishers.end(),
            "Cannot find radio publisher for requested nodeId, registerObjects failed",
            __FILE__, __LINE__);

        // Hold on to the entity's state repository, where we can set data.
        DtRadioTransmitterRepository* tsr = radiosPubIter->second->tsr();

        tsr->setTransmitState(DtRadioTransmitState(txOperationalStatus));
        radiosPubIter->second->tick();
        
        radio->SetTransmitState(DtRadioTransmitState(txOperationalStatus));
    }

    cout << "Set (" << entity->GetEntityIdString()
        << ", " << radio->GetRadioIndex() << ") " << entity->GetMarkingData()
        << " TransmitterOperationalStatus to "
        << (unsigned) txOperationalStatus << "." << endl;
}


void CopyToOffset(void* dst, unsigned& offset, const void* src, unsigned size)
{
    unsigned char* uchar_dst = (unsigned char*) dst;

    memcpy(&uchar_dst[offset], src, size);
    offset += size;
}

void CopyToOffsetAndHton(void* dst, unsigned& offset, const void* src, unsigned size)
{
    unsigned char* uchar_dst = (unsigned char*) dst;

    memcpy(&uchar_dst[offset], src, size);
    if (size > 1) { Hton(&uchar_dst[offset], size); }
    offset += size;
}

// /**
//FUNCTION :: SetSimTime
//PURPOSE :: Called from testfed app when you need to set the exer conn sim time
//PARAMETERS :: clocktype : simTime: desired time
//RETURN :: void : NULL
// **/
void VRLink::SetSimTime(clocktype simTime)
{
    double tmpTime = double(simTime) / SECOND;
    exConn->clock()->setSimTime(DtTime(tmpTime));
}

// /**
//FUNCTION :: IterationSleepOperation
//PURPOSE :: Called from testfed app when at the end of a sim frame
//PARAMETERS :: None
//RETURN :: void : NULL
// **/
void VRLink::IterationSleepOperation(clocktype curSimTime)
{
    double diff = curSimTime - (exConn->clock()->elapsedRealTime() * SECOND);
    double tmpTime = diff / SECOND;
    DtSleep(DtTime(tmpTime));
}

// /**
//FUNCTION :: Tick
//PURPOSE :: Called from testfed app when all publishers need to tick
//PARAMETERS :: None
//RETURN :: void : NULL
// **/
void VRLink::Tick()
{
    for (hash_map <string, DtEntityPublisher*>::iterator entityIt = entityPublishers.begin();
        entityIt != entityPublishers.end();
        entityIt++)
    {
        entityIt->second->tick();
    }

    //publish radios
    for (hash_map <unsigned, DtRadioTransmitterPublisher*>::iterator radioIt = radioPublishers.begin();
        radioIt != radioPublishers.end();
        radioIt++)
    {
        radioIt->second->tick();
    }
}

// /**
//FUNCTION :: ChangeEntityVelocity
//PURPOSE :: Called from testfed app when handling a change entity velocity command,
//           set the velocity for the entity state repository, and calls tick
//           on the publisher object (x,y,z in the GCC coordinate system)
//PARAMETERS :: unsigned : nodeId : node Id of parent entity object
//              float : x : new value of x
//              float : y : new value of y
//              float : z : new value of z
//RETURN :: void : NULL
// **/
void VRLink::ChangeEntityVelocity(unsigned nodeId,        
        float x,
        float y,
        float z)
{
    hash_map <unsigned, VRLinkRadio*>::iterator 
        radioIter = radios.find(nodeId);
   
    VRLinkVerify(radioIter != radios.end(),
        "Cannot find radio for requested nodeId", __FILE__, __LINE__);

    VRLinkVerify(radioIter->second->GetEntityPtr() != NULL,
        "No entity associated with radio for requested nodeId", __FILE__, __LINE__);

    VRLinkEntity* entity = radioIter->second->GetEntityPtr();

    DtVector newVelocity = DtVector(x, y, z);

    if (newVelocity != entity->GetVelocity())
    {
        //value has changed, so update the state repository
        // store that entity publisher
        hash_map <string, DtEntityPublisher*>::iterator 
            entitiesPubIter = entityPublishers.find(entity->GetMarkingData());

        VRLinkVerify(entitiesPubIter != entityPublishers.end(),
            "Cannot find entity publisher for requested nodeId, registerObjects failed",
            __FILE__, __LINE__);

        // Hold on to the entity's state repository, where we can set data.
        DtEntityStateRepository* esr = entitiesPubIter->second->entityStateRep();

        esr->setVelocity(newVelocity);
        
        entitiesPubIter->second->tick();

        entity->SetVelocity(newVelocity);
    }

    cout << "Change Velocity (" << entity->GetEntityIdString() << ") "
        << entity->GetMarkingData() << " to ("
        << newVelocity.x() << ", "
        << newVelocity.y() << ", "
        << newVelocity.z() << ")." << endl;
}

// /**
//FUNCTION :: IncrementEntitiesPositionIfVelocityIsSet
//PURPOSE :: Called from testfed app when positions need to be updated if a velocity
//           setting is being used 
//PARAMETERS :: double timeSinceLastPositionUpdate: tick delta for testfed
//RETURN :: void : NULL
// **/
void VRLink::IncrementEntitiesPositionIfVelocityIsSet(
    double timeSinceLastPostionUpdate)
{
    for (hash_map <string, DtEntityPublisher*>::iterator entityIt = entityPublishers.begin();
        entityIt != entityPublishers.end();
        entityIt++)
    {
        VRLinkEntity* ent = entities[entityIt->first];

        if (DtVector(0,0,0) != ent->GetVelocity())
        {
            double xDelta = ent->GetVelocity().x() * (timeSinceLastPostionUpdate / SECOND);
            double yDelta = ent->GetVelocity().y() * (timeSinceLastPostionUpdate / SECOND);
            double zDelta = ent->GetVelocity().z() * (timeSinceLastPostionUpdate / SECOND);
            
            DtVector delta = DtVector(xDelta, yDelta, zDelta);

            DtVector newxyz = delta + ent->GetXYZ();

            // Hold on to the entity's state repository, where we can set data.
            DtEntityStateRepository* esr = entityIt->second->entityStateRep();

            esr->setLocation(newxyz);
            
            entityIt->second->tick();

            DtVector newLatLon;
            ConvertGccToLatLonAlt(newxyz, newLatLon);

            //cout << "newX: " << newxyz.x() << " newY: " << newxyz.y() << " newZ: " << newxyz.z() << "\n";
            //cout << "newLat: " << newLatLon.x() << " newLong: " << newLatLon.y() << " newAlt: " << newLatLon.z() << "\n";

            ent->SetLatLon(newLatLon);
            ent->SetXYZ(newxyz);
        }
    }
}
