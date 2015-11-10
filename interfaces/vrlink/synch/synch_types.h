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

#ifndef _RPR_TYPES_H_
#define _RPR_TYPES_H_

#include "data_types.h"
#include <map>

namespace SNT_SYNCH
{

bool operator<(const EntityTypeStruct& e1, const EntityTypeStruct& e2);


bool operator<(const RadioTypeStruct& r1, const RadioTypeStruct& r2);

namespace TransmitterOperationalStatusEnum
{
typedef enum
{
    Off = 0,
    OnButNotTransmitting = 1,
    OnAndTransmitting = 2
} TransmitterOperationalStatusEnum8;
};
using namespace TransmitterOperationalStatusEnum;


std::ostream& operator<<(std::ostream& out, const MarkingStruct& mark);
std::ostream& operator<<(std::ostream& out, const WorldLocationStruct& loc);
std::ostream& operator<<(std::ostream& out,
                         const RelativePositionStruct& pos);
std::ostream& operator<<(std::ostream& out, const RadioTypeStruct& rad);
std::ostream& operator<<(std::ostream& out, const EntityTypeStruct& ent);
std::ostream& operator<<(std::ostream& out,
                         const EntityIdentifierStruct& ent);


template <class T> class EntityTypeMapWithWildCards :
                                   public std::map<class EntityTypeStruct, T>
{
    public:
        typedef typename std::map<class EntityTypeStruct, T>::iterator _it;
        _it matchWC(EntityTypeStruct entityType)
        {
            _it it = this->find(entityType);
            if (it != this->end())
                return it;
            it = this->lower_bound(entityType);
            if (it == this->end())
                return it;
            if (it->first.entityKind != entityType.entityKind)
            {
                entityType.entityKind = EntityTypeStruct::ANY_KIND;
                return matchWC(entityType);
            }
            else if (it->first.domain != entityType.domain)
            {
                entityType.domain = EntityTypeStruct::ANY_DOMAIN;
                return matchWC(entityType);
            }
            else if (it->first.countryCode != entityType.countryCode)
            {
                entityType.countryCode = EntityTypeStruct::ANY_COUNTRY;
                return matchWC(entityType);
            }
            else if (it->first.category != entityType.category)
            {
                entityType.category = EntityTypeStruct::ANY_CATEGORY;
                return matchWC(entityType);
            }
            else if (it->first.subcategory != entityType.subcategory)
            {
                entityType.subcategory = EntityTypeStruct::ANY_SUBCATEGORY;
                return matchWC(entityType);
            }
            else if (it->first.specific != entityType.specific)
            {
                entityType.specific = EntityTypeStruct::ANY_SPECIFIC;
                return matchWC(entityType);
            }
            else if (it->first.extra != entityType.extra)
            {
                entityType.extra = EntityTypeStruct::ANY_EXTRA;
                return matchWC(entityType);
            }

            return it;
        }
};

template <class T> class RadioTypeMapWithWildCards :
                                    public std::map<class RadioTypeStruct, T>
{
    public:
        typedef typename std::map<class RadioTypeStruct, T>::iterator _it;
        _it matchWC(RadioTypeStruct radioType)
        {
            _it it = this->find(radioType);
            if (it != this->end())
            {
                return it;
            }
            it = this->lower_bound(radioType);
            if (it == this->end())
            {
                return it;
            }
            if (it->first.entityKind != radioType.entityKind)
            {
                radioType.entityKind = RadioTypeStruct::ANY_KIND;
                return matchWC(radioType);
            }
            else if (it->first.domain != radioType.domain)
            {
                radioType.domain = RadioTypeStruct::ANY_DOMAIN;
                return matchWC(radioType);
            }
            else if (it->first.countryCode != radioType.countryCode)
            {
                radioType.countryCode = RadioTypeStruct::ANY_COUNTRY;
                return matchWC(radioType);
            }
            else if (it->first.category != radioType.category)
            {
                radioType.category = RadioTypeStruct::ANY_CATEGORY;
                return matchWC(radioType);
            }
            else if (it->first.nomenclatureVersion !=
                                               radioType.nomenclatureVersion)
            {
                radioType.nomenclatureVersion =
                                        NomenclatureVersionEnum::ANY_VERSION;
                return matchWC(radioType);
            }
            else if (it->first.nomenclature != radioType.nomenclature)
            {
                radioType.nomenclature = NomenclatureEnum::ANY_NOMENCLATURE;
                return matchWC(radioType);
            }

            return it;
        }

};


};

std::ostream& operator<<(std::ostream& out, SNT_SYNCH::ForceIdentifierEnum8);


#endif

