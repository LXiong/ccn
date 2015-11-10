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


// /**
// PACKAGE :: Library Information
// DESCRIPTION :: This file describes the APIs used to determined some
//                library related information.
// **/


#ifndef _LIBRARY_INFO_H_
#define _LIBRARY_INFO_H_

#include "types.h"

// /**
// FUNCTION     :: INFO_MultimediaEnterpriseLibCompiled
// LAYER        :: ANY LAYER
// PURPOSE      :: Whether MultiMedia & Enterprise Library is enabled
//                 in compilation
// PARAMETERS   :: None
// RETURN       :: BOOL : TRUE if enabled, FALSE otherwise
// **/
BOOL INFO_MultimediaEnterpriseLibCompiled();

// /**
// FUNCTION     :: INFO_WirelessLibCompiled
// LAYER        :: ANY LAYER
// PURPOSE      :: Whether Wireless Library is enabled in compilation
// PARAMETERS   :: None
// RETURN       :: BOOL : TRUE if enabled, FALSE otherwise
// **/
BOOL INFO_WirelessLibCompiled();

// /**
// FUNCTION     :: INFO_AdvancedWirelessLibCompiled
// LAYER        :: ANY LAYER
// PURPOSE      :: Whether Advanced Wireless Library is enabled
//                 in compilation
// PARAMETERS   :: None
// RETURN       :: BOOL : TRUE if enabled, FALSE otherwise
// **/
BOOL INFO_AdvancedWirelessLibCompiled();

// /**
// FUNCTION     :: INFO_AleAsapsLibCompiled
// LAYER        :: ANY LAYER
// PURPOSE      :: Whether Ale/Asaps Library is enabled in compilation
// PARAMETERS   :: None
// RETURN       :: BOOL : TRUE if enabled, FALSE otherwise
// **/
BOOL INFO_AleAsapsLibCompiled();

// /**
// FUNCTION     :: INFO_CellularLibCompiled
// LAYER        :: ANY LAYER
// PURPOSE      :: Whether Cellular Library is enabled in compilation
// PARAMETERS   :: None
// RETURN       :: BOOL : TRUE if enabled, FALSE otherwise
// **/
BOOL INFO_CellularLibCompiled();

// /**
// FUNCTION     :: INFO_CyberLibCompiled
// LAYER        :: ANY LAYER
// PURPOSE      :: Whether Cyber Library is enabled in compilation
// PARAMETERS   :: None
// RETURN       :: BOOL : TRUE if enabled, FALSE otherwise
// **/
BOOL INFO_CyberLibCompiled();

// /**
// FUNCTION     :: INFO_SensorNetworksLibCompiled
// LAYER        :: ANY LAYER
// PURPOSE      :: Whether Sensor Networks Library is enabled in compilation
// PARAMETERS   :: None
// RETURN       :: BOOL : TRUE if enabled, FALSE otherwise
// **/
BOOL INFO_SensorNetworksLibCompiled();

// /**
// FUNCTION     :: INFO_TiremLibCompiled
// LAYER        :: ANY LAYER
// PURPOSE      :: Whether TIREM Library is enabled in compilation
// PARAMETERS   :: None
// RETURN       :: BOOL : TRUE if enabled, FALSE otherwise
// **/
BOOL INFO_TiremLibCompiled();

// /**
// FUNCTION     :: INFO_UmtsLibCompiled
// LAYER        :: ANY LAYER
// PURPOSE      :: Whether UMTS Library is enabled in compilation
// PARAMETERS   :: None
// RETURN       :: BOOL : TRUE if enabled, FALSE otherwise
// **/
BOOL INFO_UmtsLibCompiled();

// /**
// FUNCTION     :: INFO_MuosLibCompiled
// LAYER        :: ANY LAYER
// PURPOSE      :: Whether MUOS Library is enabled in compilation
// PARAMETERS   :: None
// RETURN       :: BOOL : TRUE if enabled, FALSE otherwise
// **/
BOOL INFO_MuosLibCompiled();

// /**
// FUNCTION     :: INFO_UrbanPropLibCompiled
// LAYER        :: ANY LAYER
// PURPOSE      :: Whether Urban Propagation Library is enabled
//                 in compilation
// PARAMETERS   :: None
// RETURN       :: BOOL : TRUE if enabled, FALSE otherwise
// **/
BOOL INFO_UrbanPropLibCompiled();

// /**
// FUNCTION     :: INFO_MilitaryRadiosLibCompiled
// LAYER        :: ANY LAYER
// PURPOSE      :: Whether Military Radios Library is enabled in compilation
// PARAMETERS   :: None
// RETURN       :: BOOL : TRUE if enabled, FALSE otherwise
// **/
BOOL INFO_MilitaryRadiosLibCompiled();

// /**
// FUNCTION     :: INFO_DisInterfaceCompiled
// LAYER        :: ANY LAYER
// PURPOSE      :: Whether DIS interface is enabled in compilation
// PARAMETERS   :: None
// RETURN       :: BOOL : TRUE if enabled, FALSE otherwise
// **/
BOOL INFO_DisInterfaceCompiled();

// /**
// FUNCTION     :: INFO_HlaInterfaceCompiled
// LAYER        :: ANY LAYER
// PURPOSE      :: Whether HLA interface is enabled in compilation
// PARAMETERS   :: None
// RETURN       :: BOOL : TRUE if enabled, FALSE otherwise
// **/
BOOL INFO_HlaInterfaceCompiled();

// /**
// FUNCTION     :: INFO_AgiInterfaceCompiled
// LAYER        :: ANY LAYER
// PURPOSE      :: Whether AGI interface is enabled in compilation
// PARAMETERS   :: None
// RETURN       :: BOOL : TRUE if enabled, FALSE otherwise
// **/
BOOL INFO_AgiInterfaceCompiled();

// /**
// FUNCTION     :: INFO_LteCompiled
// LAYER        :: ANY LAYER
// PURPOSE      :: Whether LTE is enabled in compilation
// PARAMETERS   :: None
// RETURN       :: BOOL : TRUE if enabled, FALSE otherwise
// **/
BOOL INFO_LteCompiled();

// /**
// FUNCTION     :: INFO_VRlinkCompiled
// LAYER        :: ANY LAYER
// PURPOSE      :: Whether VRLINK is enabled in compilation
// PARAMETERS   :: None
// RETURN       :: BOOL : TRUE if enabled, FALSE otherwise
// **/
BOOL INFO_VRlinkCompiled();

// /**
// FUNCTION     :: INFO_SocketInterfaceCompiled
// LAYER        :: ANY LAYER
// PURPOSE      :: Whether SocketInterface is enabled in compilation
// PARAMETERS   :: None
// RETURN       :: BOOL : TRUE if enabled, FALSE otherwise
// **/
BOOL INFO_SocketInterfaceCompiled();

// /**
// FUNCTION     :: INFO_ScenarioPlayerLibCompiled
// LAYER        :: ANY LAYER
// PURPOSE      :: Whether Ocula Interface is enabled in compilation
// PARAMETERS   :: None
// RETURN       :: BOOL : TRUE if enabled, FALSE otherwise
// **/
BOOL INFO_ScenarioPlayerLibCompiled();

#endif // _LIBRARY_INFO_H_
