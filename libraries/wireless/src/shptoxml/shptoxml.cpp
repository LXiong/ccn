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

#include <errno.h>
#include <iostream>
using namespace std;
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <climits>

// GCC 4.3+ requires CSTRING in this library, gcc 4.2- does not
#define GCC_VERSION (__GNUC__ * 10000 \
                     + __GNUC_MINOR__ * 100 \
                     + __GNUC_PATCHLEVEL__)
/* Test for GCC > 4.2.0 */
#if GCC_VERSION > 40200
#include <cstring>
#endif


#include "shptoxml_shared.h"
#include "shptoxml.h"



int
main(int argc, char* argv[])
{
    ShptoxmlData sxData;

    ShptoxmlInitData(sxData);

    ProcessCommandLineArguments(sxData, argc, argv);

    ShptoxmlOpenShapefile(sxData);
    ShptoxmlReadShpHeader(sxData);

    if (sxData.m_hDbf != NULL)
    {
        ShptoxmlReadDbfFieldNames(sxData);
        ShptoxmlCheckShpDbfRecordCountConsistency(sxData);
    }

    ShptoxmlPrintInfo(sxData);
    //ShptoxmlPrintShapes(sxData);

    if (sxData.m_writeNewShp) { ShptoxmlCreateShapefile(sxData); }

    ShptoxmlReadShapefile(sxData, SHPTOXML_READ_SHP_MODE_SCAN);

    cout << endl;
    ShptoxmlPrintBoundaries(sxData);
    cout << endl;
    ShptoxmlPrintNumFeaturesOutput(sxData);

    ShptoxmlReadShapefile(sxData, SHPTOXML_READ_SHP_MODE_WRITE);

    ShptoxmlCloseShapefile(sxData);

    return 0;
}

void
ProcessCommandLineArguments(
    ShptoxmlData& sxData, unsigned argc, char* argv[])
{
    unsigned i;
    for (i = 1; i < argc; i++)
    {
        char* arg = argv[i];

        if (arg[0] == '-' || arg[0] == '/')
        {
            // This argument has a - or / as the first character.
            // Skip over this character and process the rest of the argument.

            arg++;

            if (arg[0] == 'h' || arg[0] == '?')
            {
                ShowUsage();
                exit(EXIT_SUCCESS);
            }
            else
            if (arg[0] == 'd')
            {
                sxData.m_debug = true;
            }
            else
            if (arg[0] == 'b')
            {
                i++;

                if (!ParseUintArg(i, argc, argv[i], sxData.m_defaultHeight))
                {
                    cout << "Bad m_defaultHeight." << endl;
                    exit(EXIT_FAILURE);
                }

                i++;

                char unitString[10];
                bool goodUnit = false;

                if (ParseStringArg(
                         i,
                         argc,
                         argv[i],
                         unitString,
                         sizeof(unitString)))
                {
                    if (!strcmp(unitString, "ft") ||
                        !strcmp(unitString, "Feet") ||
                        !strcmp(unitString, "FEET"))
                    {
                        sxData.m_shpUnits = SHPTOXML_FEET;
                        sxData.m_defaultHeightInMeters
                            = (unsigned) ((double) sxData.m_defaultHeight
                                          * SHPTOXML_FEET_TO_METERS);

                        goodUnit = true;
                    }
                    else
                    if (!strcmp(unitString, "m") ||
                        !strcmp(unitString, "Meters") ||
                        !strcmp(unitString, "METERS"))
                    {
                        sxData.m_shpUnits = SHPTOXML_METERS;
                        sxData.m_defaultHeightInMeters
                            = sxData.m_defaultHeight;

                        goodUnit = true;
                    }
                }//if//

                if (!goodUnit)
                {
                    cout << "Bad m_shpUnits." << unitString << endl;
                    exit(EXIT_FAILURE);
                }
            }
            else
            if (arg[0] == 'B')
            {
                i++;

                if (!ParseStringArg(
                         i,
                         argc,
                         argv[i],
                         sxData.m_dbfHeightFieldName,
                         sizeof(sxData.m_dbfHeightFieldName)))
                {
                    cout << "Bad m_dbfHeightFieldName." << endl;
                    exit(EXIT_FAILURE);
                }
            }
            else
            if (arg[0] == 'c')
            {
                i++;

                if (!ParseUintArg(i, argc, argv[i], sxData.m_fileId))
                {
                    cout << "Bad m_fileId." << endl;
                    exit(EXIT_FAILURE);
                }
            }
            else
            if (arg[0] == 'D')
            {
                i++;

                if (!ParseDblArg(i, argc, argv[i], sxData.m_defaultDensity, 0.0, 1.0)
                    || sxData.m_defaultDensity > 1.0)
                {
                    cout << "Bad m_defaultDensity." << endl;
                    exit(EXIT_FAILURE);
                }
            }
            else
            if (arg[0] == 'E')
            {
                i++;

                if (!ParseStringArg(
                         i,
                         argc,
                         argv[i],
                         sxData.m_dbfDensityFieldName,
                         sizeof(sxData.m_dbfDensityFieldName)))
                {
                    cout << "Bad m_dbfDensityFieldName." << endl;
                    exit(EXIT_FAILURE);
                }
            }
            else
            if (arg[0] == 'F')
            {
                sxData.m_featureType = SHPTOXML_FOLIAGE;
            }
            else
            if (arg[0] == 'm')
            {
                i++;

                if (!ParseDblArg(i, argc, argv[i], sxData.m_minUserLat, -90.0, 90.0))
                {
                    cout << "Bad m_minUserLat." << endl;
                    exit(EXIT_FAILURE);
                }

                i++;

                if (!ParseDblArg(i, argc, argv[i], sxData.m_maxUserLat, -90.0, 90.0))
                {
                    cout << "Bad m_maxUserLat." << endl;
                    exit(EXIT_FAILURE);
                }

                i++;

                if (!ParseDblArg(i, argc, argv[i], sxData.m_minUserLon, -180.0, 180.0))
                {
                    cout << "Bad m_minUserLon." << endl;
                    exit(EXIT_FAILURE);
                }

                i++;

                if (!ParseDblArg(i, argc, argv[i], sxData.m_maxUserLon, -180.0, 180.0))
                {
                    cout << "Bad m_maxUserLon." << endl;
                    exit(EXIT_FAILURE);
                }
            }
            else
            if (arg[0] == 'n')
            {
                i++;

                if (!ParseUintArg(i, argc, argv[i], sxData.m_maxUserFeatures))
                {
                    cout << "Bad m_maxUserFeatures." << endl;
                    exit(EXIT_FAILURE);
                }
            }
            else
            if (arg[0] == 'o')
            {
                sxData.m_writeNewShp = true;
            }
            else
            if (arg[0] == 'U')
            {
                i++;

                char unitString[3];

                if (ParseStringArg(
                         i,
                         argc,
                         argv[i],
                         unitString,
                         sizeof(unitString)))
                {
                    if (!strcmp(unitString, "ft"))
                    { sxData.m_dbfUnits = SHPTOXML_FEET; }
                    else
                    if (!strcmp(unitString, "m"))
                    { sxData.m_dbfUnits = SHPTOXML_METERS; }
                    else
                    {
                        cout << "Bad m_dbfUnits." << endl;
                        exit(EXIT_FAILURE);
                    }
                }//if//
            }
            else
            {
                ShowUsage();
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            ShptoxmlVerify(
                strlen(arg) + 1 <= sizeof(sxData.m_shpPath),
                ".shp filename too long");

            ShptoxmlSetFileSuffix(sxData.m_shpPath, ".shp");

            strcpy(sxData.m_shpPath, arg);
        }//if//
    }//for//

    if (sxData.m_shpPath[0] == 0)
    {
        ShowUsage();
        exit(EXIT_FAILURE);
    }

    strcpy(sxData.m_xmlPath, sxData.m_shpPath);
    ShptoxmlSetFileSuffix(sxData.m_xmlPath, ".xml");

    if (sxData.m_writeNewShp)
    {
        strcpy(sxData.m_newShpPath, sxData.m_shpPath);
        ShptoxmlSetFileSuffix(sxData.m_newShpPath, "-new.xml");
    }
}

void
ShowUsage()
{
    cout << "shptoxml (Convert shapefile to QualNet .xml terrain features"
            " file)" << endl
         << endl
         << "Syntax:" << endl
         << endl
         << "    shptoxml [options] filename[.shp]" << endl
         << endl
         << "    (Default values in parentheses)" << endl
         << "    -h" << endl
         << "\tPrint usage" << endl
         << "    -b height units" << endl
         << "\tDefault feature height and units (35 ft)" << endl
         << "    -B field-name" << endl
         << "\tFeature height .dbf field name (LV)" << endl
         << "    -c id" << endl
         << "\tFile ID (0)" << endl
         << "    -D foliage-density" << endl
         << "\tFoliage density [0:1) (0.15)" << endl
         << "    -E field-name" << endl
         << "\tDefault foliage density .dbf field name (DENSITY)" << endl
         << "    -F" << endl
         << "\tShapefile contains foliage data (buildings)" << endl
         << "    -m minlat maxlat minlon maxlon" << endl
         << "\tRegion filter (-90 90 -180 180)" << endl
         << "    -n count" << endl
         << "\tMaximum number of buildings to output to file (2^32 - 1)"
            << endl
         << "    -U units" << endl
         << "\tDefault .dbf units (ft)" << endl;
}

bool
ParseStringArg(
    int i,
    int argc,
    const char* arg,
    char* value,
    size_t bufSize)
{
    if (i == argc)
    {
        return false;
    }

    if (strlen(arg) + 1 > bufSize)
    {
        return false;
    }

    strcpy(value, arg);

    return true;
}

bool
ParseUintArg(
    int i,
    int argc,
    const char* arg,
    unsigned &value)
{
    if (i == argc)
    {
        return false;
    }

    char* endPtr = NULL;
    errno = 0;
    value = strtoul(arg, &endPtr, 10);

    if (endPtr == arg || errno != 0)
    {
        return false;
    }

    return true;
}

bool
ParseDblArg(
    int i,
    int argc,
    const char* arg,
    double &value,
    double min,
    double max)
{
    if (i == argc)
    {
        return false;
    }

    char* endPtr = NULL;
    errno = 0;
    value = strtod(arg, &endPtr);

    if (endPtr == arg || errno != 0 || value < min || value > max)
    {
        return false;
    }

    return true;
}
