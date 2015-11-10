#ifndef _SPINTERFACE_H_
#define _SPINTERFACE_H_

#include "external.h"

// /**
// FUNCTION    :: SPReceive
// DESCRIPTION :: The EXTERNAL_Function to handle receive for the
//                EXTERNAL_Interface SPInterface. It calls non-blocking
//                reads from each Scenario Player connection.
// PARAMETERS  :: iface : EXTERNAL_Interface*   : Pointer to SPInterface
// **/
void SPReceive(EXTERNAL_Interface *iface);

// /**
// FUNCTION    :: SPSimulationHorizon
// DESCRIPTION :: The EXTERNAL_Function to advance the simulation horizon
// PARAMETERS  :: iface : EXTERNAL_Interface*   : Pointer to SPInterface
// **/
void SPSimulationHorizon(EXTERNAL_Interface *iface);

#endif