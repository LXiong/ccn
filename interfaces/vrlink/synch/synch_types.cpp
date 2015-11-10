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

#include "synch_types.h"
#include "Config.h"

#include <iostream>
#include <sstream>
#include <string.h>

namespace SNT_SYNCH
{

const unsigned char EntityTypeStruct::ANY_KIND = 0xff;
const unsigned char EntityTypeStruct::ANY_DOMAIN = 0xff;
const unsigned short EntityTypeStruct::ANY_COUNTRY = 0xffff;
const unsigned char EntityTypeStruct::ANY_CATEGORY = 0xff;
const unsigned char EntityTypeStruct::ANY_SUBCATEGORY = 0xff;
const unsigned char EntityTypeStruct::ANY_SPECIFIC = 0xff;
const unsigned char EntityTypeStruct::ANY_EXTRA = 0xff;

bool operator<(const EntityTypeStruct& e1, const EntityTypeStruct& e2)
{
    if (e1.entityKind < e2.entityKind)
    {
        return true;
    }
    else if (e1.entityKind > e2.entityKind)
    {
        return false;
    }
    else if (e1.domain < e2.domain)
    {
        return true;
    }
    else if (e1.domain > e2.domain)
    {
        return false;
    }
    else if (e1.countryCode < e2.countryCode)
    {
        return true;
    }
    else if (e1.countryCode > e2.countryCode)
    {
        return false;
    }
    else if (e1.category < e2.category)
    {
        return true;
    }
    else if (e1.category > e2.category)
    {
        return false;
    }
    else if (e1.subcategory < e2.subcategory)
    {
        return true;
    }
    else if (e1.subcategory > e2.subcategory)
    {
        return false;
    }
    else if (e1.specific < e2.specific)
    {
        return true;
    }
    else if (e1.specific > e2.specific)
    {
        return false;
    }
    else
    {
        return e1.extra < e2.extra;
    }
}
const unsigned char RadioTypeStruct::ANY_KIND = 0xff;
const unsigned char RadioTypeStruct::ANY_DOMAIN = 0xff;
const unsigned short RadioTypeStruct::ANY_COUNTRY = 0xffff;
const unsigned char RadioTypeStruct::ANY_CATEGORY = 0xff;



bool operator<(const RadioTypeStruct& r1, const RadioTypeStruct& r2)
{
    if (r1.entityKind < r2.entityKind)
    {
        return true;
    }
    else if (r1.entityKind > r2.entityKind)
    {
        return false;
    }
    else if (r1.domain < r2.domain)
    {
        return true;
    }
    else if (r1.domain > r2.domain)
    {
        return false;
    }
    else if (r1.countryCode < r2.countryCode)
    {
        return true;
    }
    else if (r1.countryCode > r2.countryCode)
    {
        return false;
    }
    else if (r1.category < r2.category)
    {
        return true;
    }
    else if (r1.category > r2.category)
    {
        return false;
    }
    else if (r1.nomenclatureVersion < r2.nomenclatureVersion)
    {
        return true;
    }
    else if (r1.nomenclatureVersion > r2.nomenclatureVersion)
    {
        return false;
    }
    else
    {
        return r1.nomenclature < r2.nomenclature;
    }
}


bool operator<(const EntityIdentifierStruct& id1,
               const EntityIdentifierStruct& id2)
{
    if (id1.siteId < id2.siteId)
    {
        return true;
    }
    else if (id1.siteId > id2.siteId)
    {
        return false;
    }
    else if (id1.applicationId < id2.applicationId)
    {
        return true;
    }
    else if (id1.applicationId > id2.applicationId)
    {
        return false;
    }
    else if (id1.entityNumber < id2.entityNumber)
    {
        return true;
    }
    else
    {
        return false;
    }
}

std::ostream& operator<<(std::ostream& out, const MarkingStruct& mark)
{
    int len = strlen((char*)mark.markingData);
    if (len > 11)
    {
        char buf[12];
        memcpy(buf, mark.markingData, 11);
        buf[11] = 0;
        out << buf;
    }
    int leadingBlanks = sizeof(mark.markingData) - len - 1;
    int i = 0;
    for (; i < leadingBlanks; i++)
    {
        out << ' ';
    }
    out << mark.markingData;
    return out;
}
std::ostream& operator<<(std::ostream& out, const WorldLocationStruct& loc)
{
    out << loc.lat << ", " << loc.lon << ", " << loc.alt;
    return out;
}
std::ostream& operator<<(std::ostream& out,
                         const RelativePositionStruct& pos)
{
    out << pos.bodyXDistance << ", "
        << pos.bodyYDistance << ", "
        << pos.bodyZDistance;
    return out;
}
std::ostream& operator<<(std::ostream& out, const RadioTypeStruct& rad)
{
    if (rad.entityKind != 255)
    {
        out << (unsigned int) rad.entityKind << ", ";
    }
    else
    {
        out << "-1, ";
    }
    if (rad.domain!= 255)
    {
        out << (unsigned int) rad.domain << ", ";
    }
    else
    {
        out << "-1, ";
    }
    if (rad.countryCode != 65535)
    {
        out << (unsigned int) rad.countryCode << ", ";
    }
    else
    {
        out << "-1, ";
    }
    if (rad.category != 255)
    {
        out << (unsigned int) rad.category << ", ";
    }
    else
    {
        out << "-1, ";
    }
    if (rad.nomenclatureVersion != -1)
    {
        out << (unsigned int) rad.nomenclatureVersion << ", ";
    }
    else
    {
        out << "-1, ";
    }
    if (rad.nomenclature != -1)
    {
        out << (unsigned int) rad.nomenclature;
    }
    else
    {
        out << "-1";
    }
    return out;
}
std::ostream& operator<<(std::ostream& out,
                         const EntityIdentifierStruct& ent)
{
    out << ent.siteId << ":"
        << ent.applicationId << ":"
        << ent.entityNumber;
    return out;
}
std::ostream& operator<<(std::ostream& out, const EntityTypeStruct& ent)
{
    out << (unsigned int) ent.entityKind << ", "
        << (unsigned int) ent.domain << ", "
        << (unsigned int) ent.countryCode << ", "
        << (unsigned int) ent.category << ", "
        << (unsigned int) ent.subcategory << ", "
        << (unsigned int) ent.specific << ", "
        << (unsigned int) ent.extra;
    return out;
}

};

std::ostream& operator<<(std::ostream& out,
                         SNT_SYNCH::ForceIdentifierEnum8 forceId)
{
    switch (forceId)
    {
        case SNT_SYNCH::ForceIdentifierEnum::Other:
            {
                out << "Other";
                break;
            }
        case SNT_SYNCH::ForceIdentifierEnum::Friendly:
            {
                out << "Friendly";
                break;
            }
        case SNT_SYNCH::ForceIdentifierEnum::Opposing:
            {
                out << "Opposing";
                break;
            }
        case SNT_SYNCH::ForceIdentifierEnum::Neutral:
            {
                out << "Neutral";
                break;
            }
        default:
            {
                unsigned int id = forceId;
                out << "Unknown (" << id << ")";
                break;
            }
    }
    return out;
}


