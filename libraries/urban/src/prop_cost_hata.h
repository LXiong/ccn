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
#include "api.h"

#ifndef PROP_COST_HATA_H
#define PROP_COST_HATA_H

#define DEFAULT_PROPAGATION_COST231_HATA_ENVIRONMENT URBAN

// /**
// FUNCTION :: COST231_HataInitialize
// LAYER :: Physical Layer.
// PURPOSE :: Initialize COST 231 Hata Propagation Models
// PARAMETERS ::
// + propChannel: PropChannel* : Pointer to Propagation Channel data-struct
// + channelIndex: Int: Channel Index
// + nodeInput: NodeInput*: Pointer to Node Input
// RETURN :: void : NULL
//
// **/
void COST231_HataInitialize(PropChannel *propChannel,
                            int channelIndex,
                            const NodeInput *nodeInput);

// /**
// FUNCTION :: PathlossCOST231Hata
// LAYER :: Physical Layer.
// PURPOSE :: Calculate COST231Hata Propagation Loss
// PARAMETERS ::
// + distance: double : Tx-Rx distance
// + wavelength: double : Wavelength
// + txAntennaHeight: float : Transmitter antenna height
// + rxAntennaHeight: float : Receiver antenna height
// + propProfile: PropProfile * : propagation profile
// RETURN :: double : pathloss
//
// **/
double PathlossCOST231Hata(double distance,
                           double waveLength,
                           float txAntennaHeight,
                           float rxAntennaHeight,
                           PropProfile *propProfile);
#endif
