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
// use in compliance with the license agreement as part of the EXata
// software.  This source code and certain of the algorithms contained
// within it are confidential trade secrets of Scalable Network
// Technologies, Inc. and may not be used as the basis for any other
// software, hardware, product or service.

#ifndef _SYNCH_DATA_TYPES_H_
#define _SYNCH_DATA_TYPES_H_

#include <string>
#include <vector>
#include <sstream>

namespace SNT_SYNCH
{


typedef struct EntityIdentifierStruct
{
    unsigned short siteId;
    unsigned short applicationId;
    unsigned short entityNumber;
    EntityIdentifierStruct() : siteId(0), applicationId(0), entityNumber(0){}
} EntityIdentifierStruct;



class EntityTypeStruct
{
public:
        static const unsigned char ANY_KIND;
        static const unsigned char ANY_DOMAIN;
        static const unsigned short ANY_COUNTRY;
        static const unsigned char ANY_CATEGORY;
        static const unsigned char ANY_SUBCATEGORY;
        static const unsigned char ANY_SPECIFIC;
        static const unsigned char ANY_EXTRA;
        unsigned char  entityKind;
        unsigned char  domain;
        unsigned short countryCode;
        unsigned char  category;
        unsigned char  subcategory;
        unsigned char  specific;
        unsigned char  extra;
        EntityTypeStruct() :
            entityKind(0),
            domain(0),
            countryCode(0),
            category(0),
            subcategory(0),
            specific(0),
            extra(0) {}

};


typedef struct OrientationStruct
{
    float psi;
    float theta;
    float phi;
    float azimuth;
    float elevation;
    OrientationStruct() :
        psi(0.0),
        theta(0.0),
        phi(0.0),
        azimuth(0.0),
        elevation(0.0) {}

} OrientationStruct;

typedef struct WorldLocationStruct
{
    double x;
    double y;
    double z;
    double lat;
    double lon;
    double alt;
    WorldLocationStruct() :
        x(0.0),
        y(0.0),
        z(0.0),
        lat(0.0),
        lon(0.0),
        alt(0.0) {}

} WorldLocationStruct;

namespace DamageStatusEnum
{
    typedef enum
    {
        NoDamage = 0,
        SlightDamage = 1,
        ModerateDamage = 2,
        Destroyed = 3
    } DamageStatusEnum32;
};
using namespace DamageStatusEnum;

namespace ForceIdentifierEnum
{
    typedef enum
    {
        Other = 0,
        Friendly = 1,
        Opposing = 2,
        Neutral = 3
    } ForceIdentifierEnum8;
};
using namespace ForceIdentifierEnum;

namespace  MarkingEncodingEnum
{
    typedef enum
    {
        Other = 0,
        ASCII = 1,
        ArmyMarkingCCTT = 2,
        DigitChevron = 3,
    } MarkingEncodingEnum8;
};
using namespace MarkingEncodingEnum;

typedef struct MarkingStruct
{
    MarkingEncodingEnum8 markingEncodingType;
    unsigned char markingData[12];
    MarkingStruct() : markingEncodingType(MarkingEncodingEnum::Other)
    {
        markingData[0] = 0;
    }
} MarkingStruct;





namespace  NomenclatureVersionEnum
{
    typedef enum
    {
        ANY_VERSION = -1
    } NomenclatureVersionEnum8;
};
using namespace NomenclatureVersionEnum;

namespace  NomenclatureEnum
{
    typedef enum
    {
        ANY_NOMENCLATURE = -1
    } NomenclatureEnum16;
};
using namespace NomenclatureEnum;

class RadioTypeStruct
{
    public:
        static const unsigned char ANY_KIND;
        static const unsigned char ANY_DOMAIN;
        static const unsigned short ANY_COUNTRY;
        static const unsigned char ANY_CATEGORY;

        unsigned char entityKind;
        unsigned char domain;
        unsigned short countryCode;
        unsigned char category;
        NomenclatureVersionEnum8 nomenclatureVersion;
        NomenclatureEnum16 nomenclature;

        RadioTypeStruct()
        {
            // assign with Config::instance().defaultRadioSystemType;
            *this = RadioTypeStruct("1,1,225,1,1,1");
        }
        RadioTypeStruct(const std::string& str)
        {
            unsigned char comma;
            unsigned int num;
            std::stringstream sss(str);
            sss >> num;
            entityKind = num;
            sss >> comma;
            sss >> num;
            domain = num;
            sss >> comma;
            sss >> num;
            countryCode = num;
            sss >> comma;
            sss >> num;
            category = num;
            sss >> comma;
            sss >> num;
            nomenclatureVersion = NomenclatureVersionEnum8(num);
            sss >> comma;
            sss >> num;
            nomenclature = NomenclatureEnum16(num);

        }

};


typedef struct RelativePositionStruct
{
    float bodyXDistance;
    float bodyYDistance;
    float bodyZDistance;
} RelativePositionStruct;

typedef struct RelativePositionStructinLatLonAlt
{
    double lat;
    double lon;
    double alt;
} PositionStructinLatLonAlt;

class RadioStruct
{
    public:
    EntityIdentifierStruct    entityId;
    unsigned                  nodeId;
    unsigned short            radioIndex;
    char*                     markingData;
    RelativePositionStruct    relativePosition;
    PositionStructinLatLonAlt absoluteNodePosInLatLonAlt;
    RadioTypeStruct           radioSystemType;

    RadioStruct()
    {
        markingData = NULL;
    }
};


bool operator<(const EntityIdentifierStruct& id1,
               const EntityIdentifierStruct& id2);

class EntityStruct
{
    public:
    class less
    {
        public:
            bool operator()(const EntityStruct* e1, const EntityStruct* e2) const
            {
                return e1->entityIdentifier < e2->entityIdentifier;
            }
    };

    EntityIdentifierStruct      entityIdentifier;
    MarkingStruct               marking;
    ForceIdentifierEnum8        forceIdentifier;
    DamageStatusEnum32          damageState;
    WorldLocationStruct         worldLocation;
    OrientationStruct           orientation;
    EntityTypeStruct            entityType;

    char*                       myName;

    EntityStruct()
    {
        myName = NULL;
    }

};


typedef struct AggregateMarkingStruct
{
    MarkingEncodingEnum8 markingEncodingType;
    unsigned char markingData[32];
} AggregateMarkingStruct;

typedef std::vector<std::string> ObjectIdArrayStruct;

typedef struct DimensionStruct
{
    float xAxisLength;
    float yAxisLength;
    float zAxisLength;
} DimensionStruct;


namespace AggregateStateEnum
{
    typedef enum
    {
        Other = 0,
        Aggregated = 1,
        Disaggregated = 2,
        FullyDisaggregated = 3,
        PseudoDisaggregated = 4,
        PartiallyDisaggregated = 5
    } AggregateStateEnum8;
};
using namespace AggregateStateEnum;

namespace FormationEnum
{
    typedef enum
    {
        Other = 0,
        Assembly = 1,
        Vee = 2,
        Wedge = 3,
        Line = 4,
        Column = 5
    } FormationEnum32;
};
using namespace FormationEnum;

typedef struct SilentAggregateStruct
{
    EntityTypeStruct AggregateType;
    unsigned short NumberOfAggregatesOfThisType;
} SilentAggregateStruct;

typedef struct SilentEntityStruct
{
    unsigned short NumberOfEntitiesOfThisType;
    unsigned short NumberOfAppearanceRecords;
    EntityTypeStruct EntityType;
    unsigned int EntityAppearance;
} SilentEntityStruct;




class AggregateEntityStruct
{
    public:

    AggregateMarkingStruct aggregateMarking;
    EntityIdentifierStruct entityIdentifier;
    WorldLocationStruct worldLocation;
    DimensionStruct dimensions;
    char* containedEntityIDs;
    char* myName;
    char* subAggregateIDs;

    class less
    {
        public:
            bool operator()(const AggregateEntityStruct* a1,
                            const AggregateEntityStruct* a2) const
            {
                return a1->entityIdentifier < a2->entityIdentifier;
            }
    };

    AggregateEntityStruct()
    {
        containedEntityIDs = NULL;
        subAggregateIDs = NULL;
        myName = NULL;
    }
};

};

#endif

