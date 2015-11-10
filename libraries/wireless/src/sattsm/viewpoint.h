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
//
#ifndef __SATTSM_VIEWPOINT_H__
# define __SATTSM_VIEWPOINT_H__

#include "util_constants.h"

///
/// @file viewpoint.h
///
/// @brief The class header definition file for the TWTA module
///


///
/// @namespace PhySattsm
/// 
/// @brief The default PHY Sattsm namespace
///

namespace PhySattsm
{

    ///
    /// @enum SolidPathType
    ///
    /// @brief A enumerated set of coordinate frames
    ///
    /// This is called SolidPathType because it was originally
    /// developed to support a beam of finite thickness through
    /// a coordinate system.  This is only partially implemented
    /// in the current models.
    ///
    
    enum SolidPathType {
        
        /// Geocentric <latitude, longitude, altitude>
        GeoCentricLatLonAlt = 1,
        
        /// Space-centric <u,v,range>
        SpaceCentricUVRange = 2,
        
        /// Geo-centric <declination, longitude, altitude>
        GeoCentricDecLonAlt = 3,
        
        /// Geo-surface-centric <azimuth, elevation, range>
        GeoCentricAzElRange = 4,
        
        /// Unspecified coordinate frame
        TypeUnspecified = 5
    } ;

    /// 
    /// @enum AngleType
    ///
    /// @brief The unit type of an angle 
    ///
    enum AngleType {
        /// Angle in in radians
        AngleRadian = 1,
        
        /// Angle is in degrees
        AngleDegree = 2
    } ;

    ///
    /// @struct SolidPath
    ///
    /// @brief A description of a path although in this 
    /// definition it is merely two points in a path
    ///
    
    struct SolidPath 
    {
        /// Whether or not the path is fully specified
        BOOL isCompletePath;
        
        ///
        /// @brief The path type 
        ///
        /// @see SolidPathType
        ///
        
        SolidPathType type;
        
        /// A union of defined path information based on type
        union {
            /// Latitude
            double u_l_lat;
            
            /// Declination
            double u_l_dec;
            
            /// "u" of <u,v>
            double u_l_u;
            
            /// Azimuth
            double u_l_az;
        } u1;
        
    /// Short version for latitude
    #define l_lat u1.u_l_lat
        
    /// Short version for declination
    #define l_dec u1.u_l_dec
        
    /// Short version for u of <u,v> space
    #define l_u u1.u_l_u
        
    /// Short version for azimuth
    #define l_az u1.u_l_az
        
        /// A union for the second parameter in the coordinate
        union {
            /// Longitude
            double u_l_lon;
            
            /// "v" of <u,v>
            double u_l_v;
            
            /// Elevation
            double u_l_el;
        } u2;
        
    /// Short form for longitude
    #define l_lon u2.u_l_lon
        
    /// Short form for "v" of <u,v>
    #define l_v u2.u_l_v
        
    /// Short form for elevation
    #define l_el u2.u_l_el
        
        /// Tilt of orbital axis
        double l_tilt;
        
        /// A union for the third parameter in a coordinate triplet
        union {
            
            /// Altitude of object in meters
            double u_l_alt;
            
            /// Range of object in meters
            double u_l_range;
        } u3;
    
    /// Short for for altitude
    #define l_alt u3.u_l_alt
        
    /// Short form for range
    #define l_range u3.u_l_range
        
        /// 
        /// @brief Transform the current SolidPath into a Coordinates
        /// data struture
        ///
        /// @returns a Coordinates data structure
        ///
        /// @see Coordinates
        ///
        
        Coordinates toCoordinates()
        {
            Coordinates q;
            
            q.latlonalt.latitude = UTIL::rad2deg(l_lat);
            q.latlonalt.longitude = UTIL::rad2deg(l_lon);
            q.latlonalt.altitude = l_alt;
            
            return q;
        }
        
        /// 
        /// @brief Transform current SolidPath to <dec,lon,alt>
        /// coordinate frame
        ///
        
        void asDecLonAlt()
        {            
            if (type == GeoCentricDecLonAlt)
                return;
            
            ERROR_Assert(type == GeoCentricLatLonAlt,
                         "Only conversion from Dec/Lon/Alt"
                         " from Lat/Lon/Alt is permitted in"
                         " this model release.  A programmatic"
                         " inconsistency has occurred and this"
                         " simulation cannot continue.");
            
            double lat = l_lat;
            double lon = l_lon;
            double alt = l_alt;
            
            type = GeoCentricDecLonAlt;
            
            l_dec = lat;
            l_lon = lon;
            l_alt = alt;
        }
            
        ///
        /// @brief Constructure for an empty solid path
        ///
        
        SolidPath() 
        : isCompletePath(FALSE), type(TypeUnspecified)
        {
            l_lat = 0;
            l_dec = 0;
            l_u = 0;
            l_az = 0;
        }
        
        ///
        /// @brief Constructor for a GSO satellite at a given 
        /// longitude
        ///
        /// @param lon_deg longitude of satellite in degrees
        /// West of prime meridian is negative per convention
        ///
        
        SolidPath(double lon_deg)
        {
            isCompletePath = FALSE;
            type = GeoCentricDecLonAlt;
            
            l_dec = 0.0;
            l_lon = UTIL::deg2rad(lon_deg);
            l_alt = UTIL::Constants::GsoOrbitRadius * 1000.0;
        }
        
        ///
        /// @brief Constructor for a solid path given a 
        /// Coordinates data struture
        /// 
        /// @param c a reference to a coordinates data struture
        ///
        
        SolidPath(Coordinates& c)
        {
            isCompletePath = FALSE;
            
            type = GeoCentricLatLonAlt;
            
            l_lat = c.latlonalt.latitude;
            l_lon = c.latlonalt.longitude;
            l_alt = c.latlonalt.altitude;
            
        }
        
        ///
        /// @brief Constructor for a solid path given
        /// <lat,lon,alt> information
        ///
        /// @param lat_deg Latitude of the object in degrees
        /// @param lon_deg Longitude of the object in degrees such
        /// that west of the prime meridian is negative
        /// @param altitude_m Altitude of the object in meters
        ///
        
        SolidPath(double lat_deg,
                  double lon_deg,
                  double alt_m)
        {
            isCompletePath = FALSE;
            type = GeoCentricLatLonAlt;
            
            l_lat = UTIL::deg2rad(lat_deg);
            l_lon = UTIL::deg2rad(lon_deg);
            l_alt = alt_m;
            
        }
        
        ///
        /// @brief Constructor to copy a solid path
        ///
        /// @param p a reference to a solid path data structure
        ///
        
        SolidPath(SolidPath& p)
        {
            isCompletePath = p.isCompletePath;
            type = p.type;
            
            l_lat = p.l_lat;
            l_lon = p.l_lon;
            l_alt = p.l_alt;
        }
        
        ///
        /// @brief Constructor for a complete solid path based
        /// on entry and exit points
        ///
        /// @param entryPoint a reference to an entry point for the 
        /// signal
        /// @param exitPoint a reference to an exit point for a signal
        ///
        
        SolidPath(SolidPath& entryPoint,
                  SolidPath& exitPoint)
        {
            ERROR_Assert(entryPoint.type == GeoCentricLatLonAlt
                         && exitPoint.type == GeoCentricDecLonAlt,
                         "The solid path routines presently"
                         " only operate on geocentric lat/lon/alt"
                         " systems.  The simulation cannot continue.");
            
            // Modified the pritchard algorithm to include a 
            // ground station that
            // was above the earth.  This new term is 'g' for 
            // ground station absolute
            // altitude.
            
             double satelliteLongitude = exitPoint.l_lon;
            
            double stationAltitude = entryPoint.l_alt + 
                1000.0 * UTIL::Constants::Re;
            
            double g(stationAltitude);
            
            double stationLatitude = entryPoint.l_lat;
            double stationLongitude = entryPoint.l_lon;
            
            double satelliteAltitude = exitPoint.l_alt + 
                1000.0 * UTIL::Constants::Re;
            double r(satelliteAltitude);
            
            // 
            // deltaLongitude should be positive when the station is 
            // further west
            // than the satellite (ie more negative).
            //
            
            double deltaLongitude = satelliteLongitude - 
                stationLongitude;
            
            double gamma = acos(cos(stationLatitude) * 
                                cos(deltaLongitude));
            
            if (gamma == 0.0) 
            {                
                isCompletePath = TRUE;
                type = GeoCentricAzElRange;
                l_az = UTIL::deg2rad(90.0);
                l_el = UTIL::deg2rad(90.0);
                l_tilt = UTIL::deg2rad(0.0);
                l_range = satelliteAltitude - stationAltitude;                
            }
            else
            {
                double slantRange = sqrt(g * g + 
                                         r * r - 
                                         2.0 * r * g * cos(gamma));
                
                double d(slantRange);
                
                double az = asin(sin(deltaLongitude) / sin(gamma));
            
                while (az < 0.0)
                {
                    az += UTIL::Constants::Pi / 2.0;
                }
            
                if (deltaLongitude > 0.0)
                {
                    while (az < UTIL::Constants::Pi / 2.0)
                    {
                        az += UTIL::Constants::Pi / 2.0;
                    }
                }
            
                double el = acos(r / d * sin(gamma));
                double tiltAngle = asin(g / d * sin(gamma));
                
                ERROR_Assert(fabs(tiltAngle + 
                                  el + 
                                  gamma - 
                                  UTIL::Constants::Pi / 2.0) < 1.0e-6,
                             "The sum of the tilt angle, the elevation,"
                             " and gamma must alawys be 90 degrees.  There"
                             " is a mathematical inconsistency and the"
                             " simulation cannot continue.");
                            
                isCompletePath = TRUE;
                type = GeoCentricAzElRange;
                
                l_az = az;
                l_el = el;
                l_tilt = tiltAngle;
                l_range = slantRange;
            }
        }
        
        ///
        /// @brief Construct a solid path based on a base station
        /// and a satellite point
        /// 
        /// This may be modified to be a constructor in the future
        /// but right now the term uvRange is more specific.  Could
        /// also be a derived class that references the base to 
        /// maintain specificity.
        ///
        /// @param p a solid path of the ground station
        /// @param sat a solid path of the satellite
        ///
        /// @returns a construct SolidPath entity representing the
        /// path between the two points
        ///
        
        static SolidPath uvRange(SolidPath p,
                                 SolidPath sat)
        {
            ERROR_Assert(sat.type == GeoCentricDecLonAlt &&
                         p.type == GeoCentricLatLonAlt,
                         "This model release can only calculate"
                         " the (u,v) coordinates for a satellite"
                         "/ground station pair.  The simulation"
                         " cannot support the requested operation and"
                         " cannot continue.");
            
            double g = UTIL::Constants::Re * 1000.0 + p.l_alt;
            double r = UTIL::Constants::Re * 1000.0 + sat.l_alt;
            
            double phig = p.l_lat - p.l_dec;
            
            double dl = sat.l_lon - p.l_lon; // west is positive
            
            double gm = acos(cos(phig) * cos(dl));
            
            double d = sqrt(g * g + 
                            r * r - 
                            2.0 * r * g * cos(gm));
            
            double v = asin(g / d * sin(phig));
            double u = asin(g / d * cos(phig) * sin(dl) / cos(v));
            
            SolidPath path;
            
            path.isCompletePath = TRUE;
            path.type = SpaceCentricUVRange;
            path.l_u = u;
            path.l_v = v;
            path.l_range = d;
            
            return path;
        }
        
        /// Destructor for a solid path
        ~SolidPath()
        {
            
        }
    } ;
}

#endif                          /* __SATTSM_VIEWPOINT_H__ */
