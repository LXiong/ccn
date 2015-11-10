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

#ifndef SHPTOXML_SHARED_H
#define SHPTOXML_SHARED_H

#include "../shapelib/shapefil.h"

const unsigned g_shptoxmlPathBufSize      = 256;
const unsigned g_shptoxmlDbfFieldNameSize = 32;

enum ShptoxmlDistanceUnit
{
    SHPTOXML_FEET,
    SHPTOXML_METERS
};

enum ShptoxmlFeatureType
{
    SHPTOXML_BUILDING,
    SHPTOXML_FOLIAGE
};

const double SHPTOXML_FEET_TO_METERS = 0.3048;

struct ShptoxmlData
{
    bool m_debug;

    char m_shpPath[g_shptoxmlPathBufSize];
    char m_xmlPath[g_shptoxmlPathBufSize];

    ShptoxmlFeatureType  m_featureType;
    ShptoxmlDistanceUnit m_shpUnits;

    unsigned m_maxUserFeatures;
    double   m_minUserLat;
    double   m_maxUserLat;
    double   m_minUserLon;
    double   m_maxUserLon;

    unsigned m_numFeatures;
    double   m_minLat;
    double   m_maxLat;
    double   m_minLon;
    double   m_maxLon;

    SHPHandle m_hShp;
    unsigned  m_numShapes;
    int       m_shapeType;
    double    m_minExtents[4];
    double    m_maxExtents[4];

    DBFHandle            m_hDbf;
    ShptoxmlDistanceUnit m_dbfUnits;

    unsigned             m_defaultHeight;
    unsigned             m_defaultHeightInMeters;
    char                 m_dbfHeightFieldName[g_shptoxmlDbfFieldNameSize];
    bool                 m_dbfFoundHeightField;
    int                  m_dbfHeightFieldIndex;

    double               m_defaultDensity;
    char                 m_dbfDensityFieldName[g_shptoxmlDbfFieldNameSize];
    bool                 m_dbfFoundDensityField;
    int                  m_dbfDensityFieldIndex;

    bool      m_writeNewShp;
    char      m_newShpPath[g_shptoxmlPathBufSize];
    SHPHandle m_hNewShp;

    unsigned m_fileId;
};

#include "shptoxml_main.h"

void
ShptoxmlVerify(
    bool condition,
    const char* errorString,
    const char* path = NULL,
    unsigned lineNumber = 0);

void
ShptoxmlReportWarning(
    const char* warningString,
    const char* path = NULL,
    unsigned lineNumber = 0);

void
ShptoxmlReportError(
    const char* errorString,
    const char* path = NULL,
    unsigned lineNumber = 0);

char*
ShptoxmlStripFileExtension(char* s);

char*
ShptoxmlSetFileSuffix(char* path, const char* extension);

#endif /* SHPTOXML_SHARED_H */
