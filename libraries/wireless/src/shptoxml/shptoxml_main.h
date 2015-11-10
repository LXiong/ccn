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

#ifndef SHPTOXML_MAIN_H
#define SHPTOXML_MAIN_H

enum ShptoxmlReadShpMode
{
    SHPTOXML_READ_SHP_MODE_SCAN,
    SHPTOXML_READ_SHP_MODE_WRITE
};

void
ShptoxmlInitData(ShptoxmlData& sxData);

void
ShptoxmlOpenShapefile(ShptoxmlData& sxData);

void
ShptoxmlReadShpHeader(ShptoxmlData& sxData);

void
ShptoxmlReadDbfFieldNames(ShptoxmlData& sxData);

void
ShptoxmlCheckShpDbfRecordCountConsistency(ShptoxmlData& sxData);

void
ShptoxmlPrintInfo(ShptoxmlData& sxData);

void
ShptoxmlPrintShapes(ShptoxmlData& sxData);

void
ShptoxmlCreateShapefile(ShptoxmlData& sxData);

void
ShptoxmlReadShapefile(ShptoxmlData& sxData, ShptoxmlReadShpMode mode);

void
ShptoxmlPrintBoundaries(ShptoxmlData& sxData);

void
ShptoxmlPrintNumFeaturesOutput(ShptoxmlData& sxData);

void
ShptoxmlCloseShapefile(ShptoxmlData& sxData);

#endif /* SHPTOXML_MAIN_H */
