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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#include <iostream>

#include "geometry.h"

#define DEBUG 0
#define NODEBUG 0

double Box::getArea(int coordinateSystem) const {
    // returns the area in square meters.
    if (coordinateSystem == CARTESIAN) {
        return ((upper.cartesian.x - lower.cartesian.x) *
                (upper.cartesian.y - lower.cartesian.y));
    }
#ifdef MGRS_ADDON
    else if (coordinateSystem == MGRS)
    {
        return (upper.common.c1 - lower.common.c1) *
            (upper.common.c2 - lower.common.c2);
    }
#endif // MGRS_ADDON
    else // LATLONALT
    {
        double midLatitude;
        double latDistance;
        double longDistance;
        Coordinates east;
        Coordinates northWest;
        Coordinates west;

        // get the EW distance at the mid-latitude
        midLatitude = (lower.latlonalt.latitude +
                       upper.latlonalt.latitude) / 2.0;

        west.latlonalt.latitude  = midLatitude;
        west.latlonalt.longitude = lower.latlonalt.longitude;
        west.latlonalt.altitude  = 0.0;
        east.latlonalt.latitude  = midLatitude;
        east.latlonalt.longitude = upper.latlonalt.longitude;
        east.latlonalt.altitude  = 0.0;
        northWest.latlonalt.latitude  = upper.latlonalt.latitude;
        northWest.latlonalt.longitude = lower.latlonalt.longitude;
        northWest.latlonalt.altitude  = 0.0;
        COORD_CalcDistance(LATLONALT, &west, &east, &longDistance);
        COORD_CalcDistance(LATLONALT, &lower, &northWest, &latDistance);

        return (latDistance * longDistance);
    }
}

void Box::print() const {
    printf("Box lowerLeft = (%f,%f), upperRight = (%f,%f)\n",
           lower.common.c1, lower.common.c2,
           upper.common.c1, upper.common.c2);
}

void Cube::print() const {
    printf("Cube lowerLeft = (%f,%f,%f), upperRight = (%f,%f,%f)\n",
           lower.common.c1, lower.common.c2,
           upper.common.c1, upper.common.c2,
           upper.common.c3, upper.common.c3);
}

bool Geometry::contains(const Polygonl*     polygon,
                        const Coordinates* point)
{
    return false;
}

bool Geometry::overlaps(const Box* rect1,
                        const Box* rect2)
{
    return false;
}

bool Geometry::intersection(const LineSegment* line,
                            const Box*   plane,
                            Coordinates*       intersection)
{
    return false;
}

bool Geometry::intersection(const LineSegment* line,
                            const Polygonl*     shape,
                            Coordinates*       intersection)
{
    return false;
}

std::list<Coordinates> Geometry::intersections(const LineSegment* line,
                                               const Cube* cube)
{
    std::list<Coordinates> newList;
    newList.clear();
    return newList;
}

std::list<Coordinates> Geometry::intersections(const LineSegment* line,
                                               const Object3D* object)
{
    std::list<Coordinates> newList;
    newList.clear();
    return newList;
}


//  public domain function by Darel Rex Finley, 2006
//  Determines the intersection point of the line segment defined by points
//  A and B  with the line segment defined by points C and D.
//
//  Returns true if the intersection point was found, and stores that point
//  in X,Y.
//  Returns false if there is no determinable intersection point, in which
//  case X,Y will be unmodified.

bool Geometry::intersection(const LineSegment* line1,
                            const LineSegment* line2,
                            Coordinates* intersection) {
    // Bug, this probably only works for Cartesian
    double  distAB, theCos, theSin, newX, ABpos;
    double  Ax, Bx, Cx, Dx, Ay, By, Cy, Dy;

    Ax = line1->point1.common.c1;
    Ay = line1->point1.common.c2;
    Bx = line1->point2.common.c1;
    By = line1->point2.common.c2;
    Cx = line2->point1.common.c1;
    Cy = line2->point1.common.c2;
    Dx = line2->point2.common.c1;
    Dy = line2->point2.common.c2;

    //  Fail if either line segment is zero-length.
    if (Ax == Bx &&
        Ay == By ||
        Cx == Dx &&
        Cy == Dy) {

        return false;
    }

    //  Fail if the segments share an end-point.
    if (Ax == Cx && Ay == Cy ||
        Bx == Cx && By == Cy ||
        Ax == Dx && Ay == Dy ||
        Bx == Dx && By == Dy) {

        return false;
    }

    //  (1) Translate the system so that point A is on the origin.
    Bx -= Ax;
    By -= Ay;
    Cx -= Ax;
    Cy -= Ay;
    Dx -= Ax;
    Dy -= Ay;

    //  Discover the length of segment A-B.
    distAB = sqrt(Bx * Bx + By * By);

    //  (2) Rotate the system so that point B is on the positive X axis.
    theCos = Bx / distAB;
    theSin = By / distAB;
    newX = Cx * theCos+Cy * theSin;
    Cy = Cy * theCos-Cx * theSin;
    Cx = newX;
    newX = Dx * theCos+Dy * theSin;
    Dy = Dy * theCos-Dx * theSin;
    Dx = newX;

    //  Fail if segment C-D doesn't cross line A-B.
    if (Cy < 0.0  && Dy < 0.0 ||
        Cy >= 0.0 && Dy >= 0.0) return false;

    // (3) Discover the position of the intersection point along line A-B.
    ABpos = Dx + (Cx - Dx) *
        Dy / (Dy - Cy);

    //  Fail if segment C-D crosses line A-B outside of segment A-B.
    if (ABpos < 0.0 || ABpos > distAB) return false;

    // (4) Apply the discovered position to line A-B in the original
    // coordinate system.
    intersection->common.c1 = Ax + ABpos * theCos;
    intersection->common.c2 = Ay + ABpos * theSin;

    //  Success.
    return true;
}
