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

#ifndef _PHY_LTE_ESTABLISHMENT_H_
#define _PHY_LTE_ESTABLISHMENT_H_

#include <list>
#include <map>
#include "layer2_lte_establishment.h"

// /**
// FUNCTION   :: PhyLteSignalArrivalFromChannelInEstablishment
// LAYER      :: PHY
// PURPOSE    :: Handle signal arrival from the channel
// PARAMETERS ::
// + node         : Node* : Pointer to node.
// + phyIndex     : int   : Index of the PHY
// + channelIndex : int   : Index of the channel receiving signal from
// + propRxInfo   : PropRxInfo* : Propagation information
// RETURN     :: BOOL : TRUE: some message info are appended
// **/
BOOL PhyLteSignalArrivalFromChannelInEstablishment(
                                    Node* node,
                                    int phyIndex,
                                    int channelIndex,
                                    PropRxInfo* propRxInfo);

// /**
// FUNCTION   :: LteSignalEndFromChannelInEstablishment
// LAYER      :: PHY
// PURPOSE    :: Handle signal end from a channel
// PARAMETERS ::
// + node         : Node* : Pointer to node.
// + phyIndex     : int   : Index of the PHY
// + channelIndex : int   : Index of the channel receiving signal from
// + propRxInfo   : PropRxInfo* : Propagation information
// RETURN     :: void : NULL
// **/
void PhyLteSignalEndFromChannelInEstablishment(
                                Node* node,
                                int phyIndex,
                                int channelIndex,
                                PropRxInfo* propRxInfo);

// /**
// FUNCTION   :: PhyLteStartTransmittingSignalInEstablishment
// LAYER      :: PHY
// PURPOSE    :: Start transmitting a frame
// PARAMETERS ::
// + node      : Node*    : Pointer to node.
// + phyIndex  : int      : Index of the PHY
// + rxmsg     : Message* : Message associated with start transmitting event
// RETURN     :: void : NULL
// **/
void PhyLteStartTransmittingSignalInEstablishment(
                                Node* node,
                                int phyIndex,
                                Message* msg);

// /**
// FUNCTION   :: PhyLteTransmissionEndInEstablishment
// LAYER      :: PHY
// PURPOSE    :: End of the transmission
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY
// + msg       : Message* : Tx End event
// RETURN     :: void  : NULL
// **/
void PhyLteTransmissionEndInEstablishment(
                                Node* node,
                                int phyIndex,
                                Message* msg);

// /**
// FUNCTION   :: PhyLteSetNonServingCellMeasurementIntervalTimer
// LAYER      :: PHY
// PURPOSE    :: Set a NonServingCellMeasurementInterval Timer
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY
// RETURN     :: void  : NULL
// **/
void PhyLteSetNonServingCellMeasurementIntervalTimer(
                                Node* node,
                                int phyIndex);


// /**
// FUNCTION   :: PhyLteSetNonServingCellMeasurementPeriodTimer
// LAYER      :: PHY
// PURPOSE    :: Set a NonServingCellMeasurementPeriod Timer
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY
// RETURN     :: void  : NULL
// **/
void PhyLteSetNonServingCellMeasurementPeriodTimer(
                                Node* node,
                                int phyIndex,
                                BOOL isForHoMeasurement);

// /**
// FUNCTION   :: PhyLteNonServingCellMeasurementIntervalExpired
// LAYER      :: PHY
// PURPOSE    :: End of the measurement interval
// NOTE       :: Note that this function is never called
//               if LTE_LIB_USE_ONOFF is not defined.
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY
// RETURN     :: void  : NULL
// **/
void PhyLteNonServingCellMeasurementIntervalExpired(
                                Node* node,
                                int phyIndex);

// /**
// FUNCTION   :: PhyLteNonServingCellMeasurementPeriodExpired
// LAYER      :: PHY
// PURPOSE    :: Callback of end of the measurement period
// NOTE       :: Note that this function is called only one time
//               if LTE_LIB_USE_ONOFF is not defined.
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY
// RETURN     :: void  : NULL
// **/
void PhyLteNonServingCellMeasurementPeriodExpired(
                                Node* node,
                                int phyIndex);

// /**
// FUNCTION   :: PhyLteIsInStationaryState
// LAYER      :: PHY
// PURPOSE    :: Get whether the PHY state is a stationary state.
// PARAMETERS ::
// + node              : Node* : Pointer to node.
// + phyIndex          : int   : Index of the PHY
// RETURN     :: bool  : true  : PHY state is a stationary state
// **/
bool PhyLteIsInStationaryState(Node* node,
                               int phyIndex);


// /**
// FUNCTION   :: PhyLteRaGrantTransmissionIndication
// LAYER      :: PHY
// PURPOSE    :: Sending RA Grant
// PARAMETERS ::
// + node              : Node* : Pointer to node.
// + phyIndex          : int   : Index of the PHY
// + ueRnti                    : Rnti of UE which receives RA Grant
// RETURN     :: void  : NULL
// **/
void PhyLteRaGrantTransmissionIndication(
                                Node* node,
                                int phyIndex,
                                LteRnti ueRnti);


// /**
// FUNCTION   :: PhyLteRaPreambleTransmissionIndication
// LAYER      :: PHY
// PURPOSE    :: End of the transmission
// PARAMETERS ::
// + node               : Node*         : Pointer to node.
// + phyIndex           : int           : Index of the PHY
// + useMacLayerSpecifiedDelay : BOOL  : Use MAC layer specified delay or not
// + initDelayUntilAirborne : clocktype : Delay until airborne
// + preambleInfoFromMac: LteRaPreamble*: Random Access Preamble Info
// RETURN     :: void  : NULL
// **/
void PhyLteRaPreambleTransmissionIndication(
                                Node* node,
                                int phyIndex,
                                BOOL useMacLayerSpecifiedDelay,
                                clocktype initDelayUntilAirborne,
                                const LteRaPreamble* preambleInfoFromMac);

// /**
// FUNCTION   :: PhyLteRaGrantWaitingTimerTimeoutNotification
// LAYER      :: PHY
// PURPOSE    :: RA grant waiting timer time out notification from MAC Layer
// PARAMETERS ::
// + node              : Node* : Pointer to node.
// + phyIndex          : int   : Index of the PHY
// RETURN     :: void  : NULL
// **/
void PhyLteRaGrantWaitingTimerTimeoutNotification(
                                Node* node,
                                int phyIndex);

// /**
// FUNCTION   :: PhyLteGetMacConfigInEstablishment
// LAYER      :: PHY
// PURPOSE    :: Pass the mac config to MAC Layer during establishment period
// PARAMETERS ::
// + node                      : Node* : Pointer to node.
// + phyIndex                  : int   : Index of the PHY
// RETURN     :: LteMacConfig* : Pointer of LteMacConfig
// **/
LteMacConfig* PhyLteGetMacConfigInEstablishment(
                                Node* node,
                                int phyIndex);

// /**
// FUNCTION   :: PhyLteStartCellSearch
// LAYER      :: PHY
// PURPOSE    :: Start cell search
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY
// RETURN     :: void  : NULL
// **/
void PhyLteStartCellSearch(Node* node,
                           int phyIndex);



// FUNCTION   :: PhyLteIFPHConfigureMeasConfig
// LAYER      :: PHY
// PURPOSE    :: IF for RRC to setup intra-freq meas config
// PARAMETERS ::
// + node                 : Node* : Pointer to node.
// + phyIndex             : int   : Index of the PHY
// + intervalSubframeNum  : int   : interval of measurement
// + offsetSubframe       : int   : offset of measurement subframe
// + filterCoefRSRP       : int   : filter Coefficient RSRP
// + filterCoefRSRQ       : int   : filter Coefficient RSRQ
// + gapInterval          : clocktype : non Serving Cell Measurement Interval
// RETURN     :: void  : NULL
// **/
void PhyLteIFPHConfigureMeasConfig(
                            Node *node,
                            int phyIndex,
                            int intervalSubframeNum,
                            int offsetSubframe,
                            int filterCoefRSRP,
                            int filterCoefRSRQ,
                            clocktype gapInterval);

// /**
// FUNCTION   :: PhyLteIFPHStartMeasIntraFreq
// LAYER      :: PHY
// PURPOSE    :: IF for RRC to start intra-freq measurement
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY
// RETURN     :: void  : NULL
// **/
void PhyLteIFPHStartMeasIntraFreq(
                            Node *node,
                            int phyIndex);

// /**
// FUNCTION   :: PhyLteIFPHStopMeasIntraFreq
// LAYER      :: PHY
// PURPOSE    :: IF for RRC to stop intra-freq measurement
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY
// RETURN     :: void  : NULL
// **/
void PhyLteIFPHStopMeasIntraFreq(
                            Node *node,
                            int phyIndex);

// /**
// FUNCTION   :: PhyLteIFPHStartMeasInterFreq
// LAYER      :: PHY
// PURPOSE    :: IF for RRC to start inter-freq measurement
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY
// RETURN     :: void  : NULL
// **/
void PhyLteIFPHStartMeasInterFreq(
                            Node *node,
                            int phyIndex);

// /**
// FUNCTION   :: PhyLteIFPHStopMeasInterFreq
// LAYER      :: PHY
// PURPOSE    :: IF for RRC to stop inter-freq measurement
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY
// RETURN     :: void  : NULL
// **/
void PhyLteIFPHStopMeasInterFreq(
                            Node *node,
                            int phyIndex);

// /**
// FUNCTION   :: PhyLtePrepareForHandoverExecution
// LAYER      :: PHY
// PURPOSE    :: clear information for H.O. execution
// PARAMETERS ::
// + node      : Node* : Pointer to node.
// + phyIndex  : int   : Index of the PHY
// + selectedRntieNB: const LteRnti&: target eNB's RNTI
// + selectedRrcConfig: const LteRrcConfig&: rrcConfig to connect to target
// RETURN     :: void  : NULL
// **/
void PhyLtePrepareForHandoverExecution(
    Node* node,
    int phyIndex,
    const LteRnti& selectedRntieNB,
    const LteRrcConfig& selectedRrcConfig);



#endif /* _PHY_LTE_ESTABLISHMENT_H_ */

