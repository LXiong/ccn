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
#include "prop_urban.h"

#ifndef PROP_HATA_H
#define PROP_HATA_H

#define DEFAULT_PROPAGATION_OKUMURA_HATA_ENVIRONMENT URBAN

// /**
// FUNCTION :: Okumura_HataInitialize
// LAYER :: Physical Layer.
// PURPOSE :: Initialize Okumura_Hata Propagation Models
// PARAMETERS ::
// + propChannel: PropChannel* : Pointer to Propagation Channel data-struct
// + channelIndex: Int: Channel Index
// + nodeInput: NodeInput*: Pointer to Node Input
// RETURN :: void : NULL
//
// **/
void Okumura_HataInitialize(PropChannel *propChannel,
                            int channelIndex,
                            const NodeInput *nodeInput);

// /**
// FUNCTION :: PathlossHata
// LAYER :: Physical Layer.
// PURPOSE :: Calculatedue to Okumura-Hata Propagation model
// PARAMETERS ::
// + distance: double : Tx-Rx distance
// + wavelength: double : Wavelength
// + txAntennaHeight: float : Transmitter Antenna height
// + rxAntennaHeight: float : Receiver Antenna height
// + propProfile: PropProfile * : Propagation profile
// RETURN :: double : pathloss
//
// **/
double PathlossHata(double distance,
                    double waveLength,
                    float txAntennaHeight,
                    float rxAntennaHeight,
                    PropProfile *propProfile);
#endif
