#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "api.h"
#include "partition.h"

#include "layer2_lte.h"
#include "lte_rrc_config.h"

LteRrcConfig* GetLteRrcConfig(Node* node, int interfaceIndex)
{
    Layer2DataLte* layer2Data =
        LteLayer2GetLayer2DataLte(node, interfaceIndex);
    ERROR_Assert(layer2Data != NULL, "");
    return layer2Data->rrcConfig;
}

LtePdcpConfig* GetLtePdcpConfig(Node* node, int interfaceIndex)
{
    return &(GetLteRrcConfig(node, interfaceIndex)->ltePdcpConfig);
}

LteRlcConfig* GetLteRlcConfig(Node* node, int interfaceIndex)
{
    return &(GetLteRrcConfig(node, interfaceIndex)->lteRlcConfig);
}

LteMacConfig* GetLteMacConfig(Node* node, int interfaceIndex)
{
    return &(GetLteRrcConfig(node, interfaceIndex)->lteMacConfig);
}

LtePhyConfig* GetLtePhyConfig(Node* node, int interfaceIndex)
{
    return &(GetLteRrcConfig(node, interfaceIndex)->ltePhyConfig);
}

void SetLteRrcConfig(Node* node, int interfaceIndex, LteRrcConfig& rrcConfig)
{
    LteRrcConfig* currentRrcConfig = GetLteRrcConfig(node, interfaceIndex);
    *currentRrcConfig = rrcConfig;
}

