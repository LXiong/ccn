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
#include <sstream>
#include <float.h>

#include "api.h"
#include "partition.h"
#include "propagation.h"
#include "network_ip.h"
#include "mac_phy_802_11n.h"


#ifdef ADDON_DB
#include "dbapi.h"
#endif

#ifdef WIRELESS_LIB
#include "antenna.h"
#include "antenna_switched.h"
#include "antenna_steerable.h"
#include "antenna_patterned.h"
#include "phy_802_11.h"
#include "phy_802_11n.h"
#include "phy_abstract.h"
#endif // WIRELESS_LIB

//#include "energy_model.h"

#ifdef SENSOR_NETWORKS_LIB
#include "phy_802_15_4.h"
#endif //SENSOR_NETWORKS_LIB

#ifdef CELLULAR_LIB
#include "phy_gsm.h"
#endif

#ifdef WIRELESS_LIB
#include "phy_cellular.h"

#ifdef UMTS_LIB
#include "phy_umts.h"
#endif

#ifdef MUOS_LIB
#include "phy_muos.h"
#endif
#endif 

#ifdef MILITARY_RADIOS_LIB
#include "phy_fcsc.h"
#endif /* MILITARY_RADIOS_LIB */

#ifdef ADDON_LINK16
#include "link16_phy.h"
#endif // ADDON_LINK16

#ifdef SATELLITE_LIB
#include "phy_satellite_rsv.h"
#endif // SATELLITE_LIB

#ifdef ADVANCED_WIRELESS_LIB
#include "phy_dot16.h"
#endif // ADVANCED_WIRELESS_LIB

#ifdef LTE_LIB
#include "phy_lte.h"
#include "phy_lte_establishment.h"
#endif // LTE_LIB

#ifdef ADDON_BOEINGFCS
//pcom
#include "mapping.h"
#include "phy_srw_abstract.h"
#include "controller.h"
#endif
#include "prop_flat_binning.h"

#ifdef URBAN_LIB
#include "prop_cost_hata.h"
#endif // URBAN_LIB

#ifdef MODE5_LIB
#include "phy_mode5.h"
#endif


#ifdef CYBER_LIB
#include "app_jammer.h"
#endif

#define DEBUG 0
#define DYNAMIC_STATS 0

// /**
// FUNCTION   :: ParseChannels
// LAYER      :: Physical
// PURPOSE    :: Read in the channel names and create a channel mask.
// PARAMETERS ::
// + node      : Node*       : The node of interest.
// + buf       : char*       : Pointer to the list of channels 
//                              specified by their names
// RETURN     :: std::string : The set channel mask
// **/
static
std::string ParseChannels(Node* node, char* buf)
{   
    std::string channels(buf);
    std::string singleChannel;
    std::string channelMask;
    std::string name;
    std::map<std::string, int> channelNameToIdMap;
    std::map<double, int> channelFrequencyToIdMap;
    std::pair<std::map<double, int>::iterator, bool> insertedPair;
    double frequency;
    int foundAt;
    int channelId = 0;
    int i = 0;
    int numberChannels = PROP_NumberChannels(node);

    channelMask.insert(0, numberChannels, '0');
    for (i = 0; i < numberChannels; i++)
    { 
        name = PHY_GetChannelName(node, i);        
        channelNameToIdMap[name] = i;
    }
    for (i = 0; i < numberChannels; i++)
    { 
        frequency = PHY_GetFrequency(node,i);
        insertedPair = channelFrequencyToIdMap.insert(make_pair(frequency, i));
        if (!insertedPair.second)
        {
            insertedPair.first->second = -1; // multiple channels with same frequency            
        }
    }
    while (!channels.empty())
    {
        foundAt = channels.find(",");
        if (foundAt != std::string::npos)
        {
            singleChannel = channels.substr(0,foundAt);
            channels = channels.substr(foundAt+1);           
        }
        else
        {
            singleChannel = channels;
            channels.clear();
        }
        int begin = singleChannel.find_first_not_of(" \t");
        int end = singleChannel.find_last_not_of(" \t");
        int len = end - begin + 1;

        if (begin == -1)
        {
            continue;
        }
        singleChannel = singleChannel.substr(begin, len);
        frequency = strtod(singleChannel.c_str(),NULL);
        if (frequency != 0)
        {
            // Get the channel number for this frequency
            std::map<double, int>::iterator it
                = channelFrequencyToIdMap.find(frequency);
            if (it == channelFrequencyToIdMap.end())
            {
                // Check if floating point error
                it = channelFrequencyToIdMap.lower_bound(frequency - DBL_EPSILON);
                if ((it == channelFrequencyToIdMap.end()) ||
                    ((it->first - frequency) > DBL_EPSILON))
                {
                    ERROR_ReportErrorArgs("Error: A channel with frequency"
                        " %s does not exist.", singleChannel.c_str());
                }
            }
            channelId = it->second;
            if (channelId == -1)
            {                
                ERROR_ReportErrorArgs("Error: There are multiple"
                    " channels with frequency %s .\nUse the channel names"
                    " or PHY-LISTEN[ING/ABLE]-CHANNEL-MASK parameter to"
                    " set the channels." , singleChannel.c_str());                    
            }

            ERROR_Assert(channelId < numberChannels, "channelId is not"
                " less than the total number of channels.");
            if (channelMask[channelId] == '0')
            {
                channelMask[channelId] = '1';
            }
            else
            {
                ERROR_ReportErrorArgs("Error: Channel with"
                    " frequency '%s' is present multiple times"
                    " in the string %s.", singleChannel.c_str(), buf);                        
            }

        }
        else
        {
            // Get the channel number for this channel name
            std::map<std::string, int>::iterator pos
                = channelNameToIdMap.find(singleChannel);
            if (pos != channelNameToIdMap.end())
            {   
                channelId = pos->second;                
                ERROR_Assert(channelId < numberChannels, "channelId is not"
                    " less than the total number of channels.");
                if (channelMask[channelId] == '0')
                {
                    channelMask[channelId] = '1';
                }                
                else
                {
                    ERROR_ReportErrorArgs("Error: Channel '%s' is" 
                        " present multiple times in the string %s.",
                        singleChannel.c_str(), buf);
                }                
            }        
            else
            {
                ERROR_ReportErrorArgs("Error: A channel with name '%s'" 
                    " does not exist.", singleChannel.c_str());
            } 
        }
    }
    return channelMask;
}

void PHY_Init(
    Node *node,
    const NodeInput *nodeInput)
{
    node->numberPhys = 0;
}


void PHY_SetRxSNRThreshold (
      Node *node,
      int phyIndex,
      double snr)
{

    PhyData* thisPhy = node->phyData[phyIndex];

    assert(thisPhy->phyRxModel == SNR_THRESHOLD_BASED);
    thisPhy->phyRxSnrThreshold = NON_DB(snr);

    return;

}

void PHY_SetDataRate(
      Node *node,
      int phyIndex,
      Int64 dataRate)
{

    PhyData* thisPhy = node->phyData[phyIndex];

    switch(thisPhy->phyModel)
    {
#ifdef WIRELESS_LIB
        case PHY_ABSTRACT:
        {
           PhyAbstractSetDataRate(node,phyIndex,dataRate);
           break;
        }
#endif // WIRELESS_LIB
#ifdef MODE5_LIB
        case PHY_MODE5: {
          Mode5::Phy::setDataRate(node, phyIndex, dataRate);
        } break;
#endif
#ifdef ADDON_BOEINGFCS
        case PHY_SRW_ABSTRACT: {
          SRW::Nexus::PHY::SetDataRate(node, phyIndex, dataRate);
        } break;
#endif // FCS
        default:
        {
           break;
        }
    }

    return;

}

void PHY_SetTxDataRate(
      Node *node,
      int phyIndex,
      Int64 dataRate)
{
    PhyData* thisPhy = node->phyData[phyIndex];

    switch(thisPhy->phyModel)
    {
        case PHY_ABSTRACT:
        {
           PhyAbstractSetDataRate(node,phyIndex,dataRate);
           break;
        }
default:
        {
           break;
        }
    }

    return;

}

void PHY_SetRxDataRate(
      Node *node,
      int phyIndex,
      Int64 dataRate)
{
    PhyData* thisPhy = node->phyData[phyIndex];

    switch(thisPhy->phyModel)
    {
        case PHY_ABSTRACT:
        {
           PhyAbstractSetDataRate(node,phyIndex,dataRate);
           break;
        }
        default:
        {
           break;
        }
    }

    return;
}
// Dynamic variables

class D_PhyTxPower : public D_Object
{
    // Makes it possible to obtain the value returned by GetPhyTxPower ()

    private:
        Node *  m_node;
        int     m_phyIndex;

    public:
        D_PhyTxPower (Node * node, int phyIndex) : D_Object(D_COMMAND)
        {
            m_phyIndex = phyIndex;
            m_node = node;
        }

        void ExecuteAsString(const std::string& in, std::string& out)
        {
            double txPower;
            PHY_GetTransmitPower (m_node, m_phyIndex, &txPower);
            std::stringstream ss;
            ss << txPower;
            out = ss.str ();
        }
};


/*
 * FUNCTION    PHY_CreateAPhyForMac
 * PURPOSE     Initialization function for the phy layer.
 *
 * Parameters:
 *     node:      node being initialized.
 *     nodeInput: structure containing contents of input file
 */
void PHY_CreateAPhyForMac(
    Node *node,
    const NodeInput *nodeInput,
    int interfaceIndex,
    Address *networkAddress,
    PhyModel phyModel,
    int* phyNumber)
{
    char buf[10*MAX_STRING_LENGTH];
    BOOL wasFound;
    BOOL parseChannelList;
    BOOL fallbackIfNoInstanceMatch = FALSE;
    double temperature;
    double noiseFactor;
    double noise_mW_hz;
    double snr;

    int phyIndex = node->numberPhys;

    PhyData *thisPhy;
    NodeInput berTableInput;
    NodeInput perTableInput;
    int i;
    int parameterInstanceNumber = ANY_INSTANCE;

    int numberChannels = PROP_NumberChannels(node);

    node->numberPhys++;
    *phyNumber = phyIndex;
    assert(phyIndex < MAX_NUM_PHYS);

    thisPhy = (PhyData *)MEM_malloc(sizeof(PhyData));
    memset(thisPhy, 0, sizeof(PhyData));
    node->phyData[phyIndex] = thisPhy;

    thisPhy->phyIndex = phyIndex;
    thisPhy->macInterfaceIndex = interfaceIndex;
#ifdef CYBER_LIB
    thisPhy->jammerStartTime = 0;
    thisPhy->jammerEndTime = 0;
    thisPhy->jamDuration = 0;
    thisPhy->jamInstances = 0;
    memset(buf, 0, MAX_STRING_LENGTH);
    IO_ReadString(
        node,
        node->nodeId,
        interfaceIndex,
        nodeInput,
        "JAMMER-STATISTICS",
        &wasFound,
        buf);

    if (wasFound)
    {
        if (strcmp(buf, "YES") == 0)
        {
            // Create the dataBase.
           thisPhy->jammerStatistics = TRUE;
        }
        else if (strcmp(buf, "NO") != 0)
        {
            // We have invalid values.
            ERROR_ReportWarning
                ("Invalid Value for JAMMER-STATISTICS parameter, using Default\n");
            thisPhy->jammerStatistics = FALSE;
        }
    }
    else
    {
        // default do not record jammer statistics
        thisPhy->jammerStatistics = FALSE;
    }
#endif

    thisPhy->networkAddress = (Address*) MEM_malloc(sizeof(Address));

    memcpy(thisPhy->networkAddress, networkAddress, (sizeof(Address)));

    thisPhy->phyModel = phyModel;

    assert(phyModel == PHY802_11a ||
           phyModel == PHY802_11b ||
           phyModel == PHY802_11n ||
           phyModel == PHY_GSM ||
           phyModel == PHY_CELLULAR ||
           phyModel == PHY_ABSTRACT ||
           phyModel == PHY_MODE5 ||
           phyModel == PHY_FCSC_PROTOTYPE ||
           phyModel == PHY_SPAWAR_LINK16 ||
           phyModel == PHY_BOEINGFCS ||
           phyModel == PHY_SATELLITE_RSV ||
           phyModel == PHY802_16 ||
           phyModel == PHY_JAMMING ||
           phyModel == PHY802_15_4 ||
           phyModel == PHY_SRW_ABSTRACT ||
           phyModel == PHY_LTE);

#ifndef ADDON_BOEINGFCS
    // enable the contention free propagation abstraction
    // NOTE: Removed ABSTRACT-CONTENTION-FREE-PROPAGATION parameter
    // and forced on all the time.
    thisPhy->contentionFreeProp = TRUE;
#else
    // temporary fix until ces code conforms to
    // ABSTRACT-CONTENTION-FREE-PROPAGATION
    thisPhy->contentionFreeProp = FALSE;
#endif

    thisPhy->channelListenable = new D_BOOL[numberChannels];
    thisPhy->channelListening = new D_BOOL[numberChannels];


#ifdef ADDON_BOEINGFCS
    if (phyModel == PHY_SRW_ABSTRACT) {
        parameterInstanceNumber = controller()->radioId();
        fallbackIfNoInstanceMatch = TRUE;
    }
#endif //FCS
    IO_ReadChannelMask(
        node,
        node->nodeId,
        networkAddress,
        interfaceIndex,
        nodeInput,
        "PHY-LISTENABLE-CHANNEL-MASK",
        parameterInstanceNumber,
        fallbackIfNoInstanceMatch,
        &wasFound,
        &parseChannelList,
        buf);

    if (wasFound) {
        if (parseChannelList) {
            
            // buf contains a list of channels
            std::string mask = ParseChannels(node, buf);

            for (i = 0; i < numberChannels; i++) {
                if (mask[i] == '1') {
                    thisPhy->channelListenable[i] = TRUE;
                }
                // else do nothing as it is initialized to FALSE            
            }
        }
        else {
            
            // buf contains the channel mask
            if (strlen(buf) > (unsigned)numberChannels) {
                char addr[MAX_STRING_LENGTH];
                NodeAddress subnetAddress;

                subnetAddress = MAPPING_GetSubnetAddressForInterface(node,
                    node->nodeId, interfaceIndex);

                IO_ConvertIpAddressToString(subnetAddress, addr);

                ERROR_ReportErrorArgs("[%s] PHY-LISTENABLE-CHANNEL-MASK %s \n"
                    "contains channels more than the total number of channels\n"
                    "total number of channels: %d", addr, buf, numberChannels);
            }
            if (strlen(buf) < (unsigned)numberChannels) {
                char addr[MAX_STRING_LENGTH];
                NodeAddress subnetAddress;

                subnetAddress = MAPPING_GetSubnetAddressForInterface(node,
                    node->nodeId, interfaceIndex);

                IO_ConvertIpAddressToString(subnetAddress, addr);
                
                ERROR_ReportErrorArgs("[%s] PHY-LISTENABLE-CHANNEL-MASK %s \n"
                    "contains channels less than the total number of channels\n"
                    "total number of channels %d", addr, buf, numberChannels);
            }
            assert(strlen(buf) == (unsigned) numberChannels);

            for (i = 0; i < numberChannels; i++) {
                if (buf[i] == '1') {
                    thisPhy->channelListenable[i] = TRUE;
                }
                else if (buf[i] == '0') {
                    // Do nothing as it is initialized to FALSE
                }
                else {
                    ERROR_ReportError(
                        "Error: PHY-LISTENABLE-CHANNEL-MASK is "
                        "incorrectly formatted.\n");
                }
            }
        }
    }
    IO_ReadChannelMask(
        node,
        node->nodeId,
        networkAddress,
        interfaceIndex,
        nodeInput,
        "PHY-LISTENING-CHANNEL-MASK",
        parameterInstanceNumber,
        fallbackIfNoInstanceMatch,
        &wasFound,
        &parseChannelList,
        buf);  
    if (wasFound) {
        if (parseChannelList) {
            
            // buf contains a list of channels
            std::string mask = ParseChannels(node, buf);

            for (i = 0; i < numberChannels; i++) {
                if (mask[i] == '1')
                {
                    if (thisPhy->channelListenable[i] == TRUE)
                    {
                        PHY_StartListeningToChannel(node, phyIndex, i);
                    }
                    else
                    {
                        ERROR_ReportErrorArgs(
                            "Attempting to listen to channel %d, "
                            "which is not configured to be a "
                            "listenable channel. Check your configuration"
                            " and run the simulation again.\n", i);
                    }
                }
                // else do nothing as it is initialized to FALSE            
            }
        }
        else {
            
            // buf contains the channel mask
            if (strlen(buf) > (unsigned)numberChannels) {
                char addr[MAX_STRING_LENGTH];
                NodeAddress subnetAddress;

                subnetAddress = MAPPING_GetSubnetAddressForInterface(node,
                    node->nodeId, interfaceIndex);

                IO_ConvertIpAddressToString(subnetAddress, addr);
                
                ERROR_ReportErrorArgs("[%s] PHY-LISTENING-CHANNEL-MASK %s \n"
                    "contains channels more than the total number of channels\n"
                    "total number of channels %d", addr, buf, numberChannels);
            }
            if (strlen(buf) < (unsigned)numberChannels) {
                char addr[MAX_STRING_LENGTH];
                NodeAddress subnetAddress;

                subnetAddress = MAPPING_GetSubnetAddressForInterface(node,
                    node->nodeId, interfaceIndex);

                IO_ConvertIpAddressToString(subnetAddress, addr);

                ERROR_ReportErrorArgs("[%s] PHY-LISTENING-CHANNEL-MASK %s \n"
                    "contains channels less than the total number of channels\n"
                    "total number of channels %d", addr, buf, numberChannels);
            }
            assert(strlen(buf) == (unsigned)numberChannels);

            for (i = 0; i < numberChannels; i++) {
                if (buf[i] == '1') {
                    PHY_StartListeningToChannel(node, phyIndex, i);
                }
                else if (buf[i] == '0') {
                    // Do nothing as it is initialized to FALSE
                }
                else {
                    char addr[MAX_STRING_LENGTH];
                    NodeAddress subnetAddress;

                    subnetAddress = MAPPING_GetSubnetAddressForInterface(node,
                        node->nodeId, interfaceIndex);

                    IO_ConvertIpAddressToString(subnetAddress, addr); 

                    ERROR_ReportErrorArgs("[%s] PHY-LISTENABLE-CHANNEL-MASK"
                        " %s \n is incorrectly formatted\n", addr, buf);
                }
            }
        }
    }
    //
    // Get the temperature
    //
#ifdef ADDON_BOEINGFCS
    if (phyModel != PHY_SRW_ABSTRACT) {
#endif //FCS
      IO_ReadDouble(
        node,
        node->nodeId,
        interfaceIndex,
        nodeInput,
        "PHY-TEMPERATURE",
        &wasFound,
        &temperature);
#ifdef ADDON_BOEINGFCS
    } else {
      IO_ReadDoubleInstance(
        node,
        node->nodeId,
        interfaceIndex,
        nodeInput,
        "PHY-TEMPERATURE",
        controller()->radioId(),
        TRUE,
        &wasFound,
        &temperature);
    }
#endif //FCS

    if (wasFound == FALSE) {
        temperature = PHY_DEFAULT_TEMPERATURE;
    }
    else {
        assert(wasFound == TRUE);
    }


    //
    // Get the noise factor
    //
#ifdef ADDON_BOEINGFCS
    if (phyModel != PHY_SRW_ABSTRACT) {
#endif //FCS
      IO_ReadDouble(
        node,
        node->nodeId,
        interfaceIndex,
        nodeInput,
        "PHY-NOISE-FACTOR",
        &wasFound,
        &noiseFactor);
#ifdef ADDON_BOEINGFCS
   } else {
      IO_ReadDoubleInstance(
        node,
        node->nodeId,
        interfaceIndex,
        nodeInput,
        "PHY-NOISE-FACTOR",
        controller()->radioId(),
        TRUE,
        &wasFound,
        &noiseFactor);
    }
#endif //FCS

    if (wasFound == FALSE) {
        noiseFactor = PHY_DEFAULT_NOISE_FACTOR;
    }
    else {
        assert(wasFound == TRUE);
    }


    //
    // Calculate thermal noise
    //
    noise_mW_hz
        = BOLTZMANN_CONSTANT * temperature * noiseFactor * 1000.0;

    thisPhy->noise_mW_hz = noise_mW_hz;
    thisPhy->noiseFactor = noiseFactor;

    //
    // Set PHY-RX-MODEL
    //


#ifdef ADDON_BOEINGFCS
    if (phyModel != PHY_SRW_ABSTRACT) {
#endif //FCS
    IO_ReadString(
        node,
        node->nodeId,
        interfaceIndex,
        nodeInput,
        "PHY-RX-MODEL",
        &wasFound,
        buf);
#ifdef ADDON_BOEINGFCS
    } else {
    IO_ReadStringInstance(
        node,
        node->nodeId,
        interfaceIndex,
        nodeInput,
        "PHY-RX-MODEL",
        controller()->radioId(),
        TRUE,
        &wasFound,
        buf);
    }
#endif //FCS


    if (wasFound) {
#ifdef WIRELESS_LIB
        if (strcmp(buf, "PHY802.11a") == 0) {
            thisPhy->phyRxModel = RX_802_11a;
            Phy802_11aSetBerTable(thisPhy);
        }
        else if (strcmp(buf, "PHY802.11b") == 0) {
            thisPhy->phyRxModel = RX_802_11b;
            Phy802_11bSetBerTable(thisPhy);
        }
        else if (strcmp(buf, "PHY802.11n") == 0) {
            thisPhy->phyRxModel = RX_802_11n;
                Phy802_11nSetBerTable(thisPhy);
        }
        else
#endif // WIRELESS_LIB
#ifdef SENSOR_NETWORKS_LIB
        if (strcmp(buf, "PHY802.15.4") == 0) {
            thisPhy->phyRxModel = RX_802_15_4;
            //PhyDot16SetBerTable(thisPhy);
        }
        else
#endif // SENSOR_NETWORKS_LIB
#ifdef MODE5_LIB
        if (strcmp(buf, "PHYMODE5") == 0) {
          thisPhy->phyRxModel = RX_MODE5;
        }
        else
#endif
#ifdef ADVANCED_WIRELESS_LIB
        if (strcmp(buf, "PHY802.16") == 0) {
            thisPhy->phyRxModel = RX_802_16;
            PhyDot16SetBerTable(thisPhy);
        }
        else
#endif //ADVANCED_WIRELESS_LIB
#ifdef LTE_LIB
        if (strcmp(buf, "PHY-LTE-BER-BASED") == 0) {
            int i;
            int numBerTables = 0;

            thisPhy->phyRxModel = PHY_LTE_BER_BASED;

            //
            // PHY-RX-BER-TABLE-FILE[0]  - [28]
            //          are the BER curves for DL MCS[0] - [29]
            //
            // PHY-RX-BER-TABLE-FILE[32] - [60]
            //          are the BER curves for UL MCS[0] - [29]
            //
            // Note that PHY-RX-BER-TABLE-FILE [29]-[31],[61]- are not used.

            //
            // Read PHY-RX-BER-TABLE-FILE
            //
            IO_ReadCachedFileInstance(
                node,
                node->nodeId,
                interfaceIndex,
                nodeInput,
                "PHY-RX-BER-TABLE-FILE",
                0,
                TRUE,
                &wasFound,
                &berTableInput);

            if (wasFound != TRUE) {
                ERROR_ReportError(
                    "PHY-RX-BER-TABLE-FILE is not specified\n");
            }

            assert(wasFound == TRUE);

            numBerTables++;

            // Scan all PHY-RX-BER-TABLE-FILE variables
            // and see how many of them are defined
            while (TRUE) {
                IO_ReadCachedFileInstance(
                    node,
                    node->nodeId,
                    interfaceIndex,
                    nodeInput,
                    "PHY-RX-BER-TABLE-FILE",
                    numBerTables,
                    FALSE,
                    &wasFound,
                    &berTableInput);

                if (wasFound != TRUE) {
                    break;
                }
                numBerTables++;
            }

            // Regardless of numBerTalbes, 64 BER tables are allocated.

            thisPhy->numBerTables = PHY_LTE_NUM_BER_TALBES;
            thisPhy->snrBerTables =
                        PHY_BerTablesAlloc(PHY_LTE_NUM_BER_TALBES);

            // Load BER table for downlink
            for (i = 0; i < PHY_LTE_REGULAR_MCS_INDEX_LEN; i++) {
                IO_ReadCachedFileInstance(
                    node,
                    node->nodeId,
                    interfaceIndex,
                    nodeInput,
                    "PHY-RX-BER-TABLE-FILE",
                    i,
                    (i == 0),
                    &wasFound,
                    &berTableInput);

                if (wasFound == FALSE )
                {
                    static char errBuf[MAX_STRING_LENGTH];
                    snprintf(errBuf,
                            sizeof(errBuf),
                            "PHY-RX-BER-TABLE-FILE[%d] is missing.\n",
                            i);
                    ERROR_ReportError(errBuf);
                }

                memcpy(&(thisPhy->snrBerTables[i]),
                       PHY_GetSnrBerTableByName(berTableInput.ourName),
                       sizeof(PhyBerTable));
            }

            // Load BER table for uplink
            for (i = PHY_LTE_MCS_INDEX_LEN;
                 i < (PHY_LTE_MCS_INDEX_LEN + PHY_LTE_REGULAR_MCS_INDEX_LEN);
                 i++) {

                IO_ReadCachedFileInstance(
                    node,
                    node->nodeId,
                    interfaceIndex,
                    nodeInput,
                    "PHY-RX-BER-TABLE-FILE",
                    i,
                    (i == 0),
                    &wasFound,
                    &berTableInput);

                if (wasFound == FALSE )
                {
                    static char errBuf[MAX_STRING_LENGTH];
                    snprintf(errBuf,
                            sizeof(errBuf),
                            "PHY-RX-BER-TABLE-FILE[%d] is missing.\n",
                            i);
                    ERROR_ReportError(errBuf);
                }

                memcpy(&(thisPhy->snrBerTables[i]),
                       PHY_GetSnrBerTableByName(berTableInput.ourName),
                       sizeof(PhyBerTable));
            }

        }
        else
#endif //LTE_LIB
#ifdef UMTS_LIB
        if (strcmp(buf, "PHY-UMTS") == 0) {
            thisPhy->phyRxModel = RX_UMTS;
            PhyUmtsSetBerTable(thisPhy);
        }
        else
#endif //UMTS_LIB
#ifdef MUOS_LIB
        if (strcmp(buf, "PHY-MUOS") == 0) {
            thisPhy->phyRxModel = RX_MUOS;
            PhyMuosSetBerTable(thisPhy);
        }
        else
#endif //MUOS_LIB
#ifdef SENSOR_NETWORKS_LIB
        if (strcmp(buf, "PHY802.15.4") == 0) {
            thisPhy->phyRxModel = RX_802_15_4;
            //PhyDot16SetBerTable(thisPhy);
        }
        else
#endif //SENSOR_NETWORKS_LIB

        if (strcmp(buf, "SNR-THRESHOLD-BASED") == 0) {
            thisPhy->phyRxModel = SNR_THRESHOLD_BASED;


#ifdef ADDON_BOEINGFCS
           if (phyModel != PHY_SRW_ABSTRACT) {
#endif //FCS
             IO_ReadDouble(
                node,
                node->nodeId,
                interfaceIndex,
                nodeInput,
                "PHY-RX-SNR-THRESHOLD",
                &wasFound,
                &snr);
#ifdef ADDON_BOEINGFCS
           } else {
             IO_ReadDoubleInstance(
                node,
                node->nodeId,
                interfaceIndex,
                nodeInput,
                "PHY-RX-SNR-THRESHOLD",
                controller()->radioId(),
                TRUE,
                &wasFound,
                &snr);
           }
#endif //FCS

            if (wasFound) {
                thisPhy->phyRxSnrThreshold = NON_DB(snr);
            }
            else {
                ERROR_ReportError("PHY-RX-SNR-THRESHOLD is missing.\n");
            }
        }
        else if (strcmp(buf, "PER-BASED") == 0)
        {
            int i;
            int numPerTables = 0;

            thisPhy->phyRxModel = PER_BASED;

            //
            // Read PHY-RX-PER-TABLE-FILE
            //
#ifdef ADDON_BOEINGFCS
        if (phyModel != PHY_SRW_ABSTRACT) {
#endif //FCS
            IO_ReadCachedFileInstance(
                node,
                node->nodeId,
                interfaceIndex,
                nodeInput,
                "PHY-RX-PER-TABLE-FILE",
                0,
                TRUE,
                &wasFound,
                &perTableInput);
#ifdef ADDON_BOEINGFCS
        } else {
            IO_ReadCachedFileInstance(
                node,
                node->nodeId,
                interfaceIndex,
                nodeInput,
                "PHY-RX-PER-TABLE-FILE",
                controller()->radioId(),
                TRUE,
                &wasFound,
                &perTableInput);
       }
#endif

            if (wasFound != TRUE)
            {
                ERROR_ReportError(
                    "PHY-RX-PER-TABLE-FILE is not specified\n");

                assert(wasFound == TRUE);

                numPerTables++;
            }

            // Scan all PHY-RX-PER-TABLE-FILE variables
            // and see how many of them are defined
            /// \todo Not sure how this applies to PHY_SRW_ABSTRACT
            while (TRUE)
            {
                IO_ReadCachedFileInstance(
                    node,
                    node->nodeId,
                    interfaceIndex,
                    nodeInput,
                    "PHY-RX-PER-TABLE-FILE",
                    numPerTables,
                    FALSE,
                    &wasFound,
                    &perTableInput);

                if (wasFound != TRUE)
                {
                    break;
                }
                numPerTables++;
            }
            thisPhy->numPerTables = numPerTables;
            thisPhy->snrPerTables = PHY_PerTablesAlloc (numPerTables);

            for (i = 0; i < numPerTables; i++)
            {
                IO_ReadCachedFileInstance(
                    node,
                    node->nodeId,
                    interfaceIndex,
                    nodeInput,
                    "PHY-RX-PER-TABLE-FILE",
                    i,
                    (i == 0),
                    &wasFound,
                    &perTableInput);

                assert(wasFound == TRUE);

                memcpy(&(thisPhy->snrPerTables[i]),
                       PHY_GetSnrPerTableByName(perTableInput.ourName),
                       sizeof(PhyPerTable));
            }
#ifdef ADDON_BOEINGFCS
            // SPECIAL CASE FOR LINK ADAPTATION.
            int numSerTables = 0;
            NodeInput serTableInput;

            //
            // Read PHY-RX-SER-TABLE-FILE
            //
           if (phyModel != PHY_SRW_ABSTRACT) {
             IO_ReadCachedFileInstance(
                node,
                node->nodeId,
                interfaceIndex,
                nodeInput,
                "PHY-RX-SER-TABLE-FILE",
                0,
                TRUE,
                &wasFound,
                &serTableInput);
            } else {
             IO_ReadCachedFileInstance(
                node,
                node->nodeId,
                interfaceIndex,
                nodeInput,
                "PHY-RX-SER-TABLE-FILE",
                controller()->radioId(),
                TRUE,
                &wasFound,
                &serTableInput);
           }

            if (wasFound != TRUE)
            {
                ERROR_ReportError(
                    "PHY-RX-SER-TABLE-FILE is not specified\n");

                assert(wasFound == TRUE);

                numSerTables++;
            }

            // Scan all PHY-RX-SER-TABLE-FILE variables
            // and see how many of them are defined
            while (TRUE)
            {
                IO_ReadCachedFileInstance(
                    node,
                    node->nodeId,
                    interfaceIndex,
                    nodeInput,
                    "PHY-RX-SER-TABLE-FILE",
                    numSerTables,
                    FALSE,
                    &wasFound,
                    &serTableInput);

                if (wasFound != TRUE)
                {
                    break;
                }
                numSerTables++;
            }
            thisPhy->numSerTables = numSerTables;
            thisPhy->snrSerTables = PHY_SerTablesAlloc (numSerTables);

            for (i = 0; i < numSerTables; i++)
            {
                IO_ReadCachedFileInstance(
                    node,
                    node->nodeId,
                    interfaceIndex,
                    nodeInput,
                    "PHY-RX-SER-TABLE-FILE",
                    i,
                    (i == 0),
                    &wasFound,
                    &serTableInput);

                assert(wasFound == TRUE);

                memcpy(&(thisPhy->snrSerTables[i]),
                       PHY_GetSnrSerTableByName(serTableInput.ourName),
                       sizeof(PhySerTable));
            }
#endif
        } // end of PER BASED
        else if (strcmp(buf, "BER-BASED") == 0)
        {
            int i;
            int numBerTables = 0;

            thisPhy->phyRxModel = BER_BASED;

            //
            // Read PHY-RX-BER-TABLE-FILE
            //
#ifdef ADDON_BOEINGFCS
            if (phyModel != PHY_SRW_ABSTRACT) {
#endif //FCS
              IO_ReadCachedFileInstance(
                node,
                node->nodeId,
                interfaceIndex,
                nodeInput,
                "PHY-RX-BER-TABLE-FILE",
                0,
                TRUE,
                &wasFound,
                &berTableInput);
#ifdef ADDON_BOEINGFCS
            } else {
              IO_ReadCachedFileInstance(
                node,
                node->nodeId,
                interfaceIndex,
                nodeInput,
                "PHY-RX-BER-TABLE-FILE",
                controller()->radioId(),
                TRUE,
                &wasFound,
                &berTableInput);
            }
#endif //FCS

            if (wasFound != TRUE) {
                ERROR_ReportError(
                    "PHY-RX-BER-TABLE-FILE is not specified\n");
            }

            assert(wasFound == TRUE);

            numBerTables++;

            // Scan all PHY-RX-BER-TABLE-FILE variables
            // and see how many of them are defined
            while (TRUE) {
                IO_ReadCachedFileInstance(
                    node,
                    node->nodeId,
                    interfaceIndex,
                    nodeInput,
                    "PHY-RX-BER-TABLE-FILE",
                    numBerTables,
                    FALSE,
                    &wasFound,
                    &berTableInput);

                if (wasFound != TRUE) {
                    break;
                }
                numBerTables++;
            }

            thisPhy->numBerTables = numBerTables;
            thisPhy->snrBerTables = PHY_BerTablesAlloc (numBerTables);

            for (i = 0; i < numBerTables; i++) {
                IO_ReadCachedFileInstance(
                    node,
                    node->nodeId,
                    interfaceIndex,
                    nodeInput,
                    "PHY-RX-BER-TABLE-FILE",
                    i,
                    (i == 0),
                    &wasFound,
                    &berTableInput);

                assert(wasFound == TRUE);

                memcpy(&(thisPhy->snrBerTables[i]),
                       PHY_GetSnrBerTableByName(berTableInput.ourName),
                       sizeof(PhyBerTable));
            }
        }
#ifdef ADDON_BOEINGFCS
        else if (strcmp(buf, "PCOM-BASED") == 0) {
            int i;
            BOOL retVal;
            thisPhy->phyRxModel = PCOM_BASED;

                //read in min-pcom-value
            double minPcom = PHY_DEFAULT_MIN_PCOM_VALUE;
            IO_ReadDouble(node->nodeId,
                          networkAddress,
                          nodeInput,
                          "MIN-PCOM-VALUE",
                          &retVal,
                          &minPcom);
            thisPhy->minPcomValue = minPcom;

            clocktype collisionWindow = PHY_DEFAULT_SYNC_COLLISION_WINDOW;
            IO_ReadTime(node->nodeId,
                        networkAddress,
                        nodeInput,
                        "SYNC-COLLISION-WINDOW",
                        &retVal,
                        &collisionWindow);
            thisPhy->syncCollisionWindow = collisionWindow;
            thisPhy->collisionWindowActive = FALSE;

                //read in file, generate data structure
            thisPhy->pcomTable =
                    (PhyPcomItem**)MEM_malloc((node->numNodes + 1) *
                    sizeof(PhyPcomItem*));
            for (i = 0; i < node->numNodes; i++)
            {
                thisPhy->pcomTable[i] = NULL;
            }

            //char clockStr[MAX_STRING_LENGTH];
            //TIME_PrintClockInSecond(collisionWindow, clockStr);
            //printf("Node %d ~ min-pcom: %f, sync-collision-window: %s\n",
            //       node->nodeId, minPcom, clockStr);
        }
#endif
        else {
            char errorbuf[MAX_STRING_LENGTH];
            sprintf(errorbuf, "Unknown PHY-RX-MODEL %s.\n", buf);
            ERROR_ReportError(errorbuf);
        }
    }
    else {
        ERROR_ReportError("PHY-RX-MODEL is missing.");
    }


    //
    // Stats option
    //
#ifdef ADDON_BOEINGFCS
    if (phyModel != PHY_SRW_ABSTRACT) {
#endif //FCS
      IO_ReadString(
        node,
        node->nodeId,
        interfaceIndex,
        nodeInput,
        "PHY-LAYER-STATISTICS",
        &wasFound,
        buf);
#ifdef ADDON_BOEINGFCS
    } else {
      IO_ReadStringInstance(
        node,
        node->nodeId,
        interfaceIndex,
        nodeInput,
        "PHY-LAYER-STATISTICS",
        controller()->radioId(),
        TRUE,
        &wasFound,
        buf);
    }
#endif //FCS

    if (wasFound) {
        if (strcmp(buf, "YES") == 0) {
            thisPhy->phyStats = TRUE;
        }
        else if (strcmp(buf, "NO") == 0) {
            thisPhy->phyStats = FALSE;
        }
        else {
            ERROR_ReportErrorArgs("%s is not a valid choice.\n", buf);
        }
    }
    else {
        thisPhy->phyStats = FALSE;
    }

    if (thisPhy->phyStats == FALSE)
    {
#ifdef ADDON_DB
        if (node->partitionData->statsDb)
        {
            if (node->partitionData->statsDb->statsAggregateTable->createPhyAggregateTable)
            {
                ERROR_ReportError(
                    "Invalid Configuration settings: Use of StatsDB PHY_Aggregate table requires\n"
                    " PHY-LAYER-STATISTICS to be set to YES\n");
            }
            if (node->partitionData->statsDb->statsSummaryTable->createPhySummaryTable)
            {
                ERROR_ReportError(
                    "Invalid Configuration settings: Use of StatsDB PHY_Summary table requires\n"
                    " PHY-LAYER-STATISTICS to be set to YES\n");
            }
        }
#endif
    }

    RANDOM_SetSeed(thisPhy->seed,
                   node->globalSeed,
                   node->nodeId,
                   interfaceIndex,
                   *phyNumber);

    //
    // Energy Stats option
    //
#ifdef ADDON_BOEINGFCS
    if (phyModel != PHY_SRW_ABSTRACT) {
#endif //FCS
      IO_ReadString(
        node->nodeId,
        networkAddress,
        nodeInput,
        "ENERGY-MODEL-STATISTICS",
        &wasFound,
        buf);
#ifdef ADDON_BOEINGFCS
    } else {
      IO_ReadStringInstance(
        node->nodeId,
        networkAddress,
        nodeInput,
        "ENERGY-MODEL-STATISTICS",
        controller()->radioId(),
        TRUE,
        &wasFound,
        buf);
    }
#endif //FCS

    if (wasFound) {
        if (strcmp(buf, "YES") == 0) {
            thisPhy->energyStats = TRUE;
        }
        else if (strcmp(buf, "NO") == 0) {
            thisPhy->energyStats = FALSE;
        }
        else {
            ERROR_ReportErrorArgs("%s is not a valid choice.\n", buf);
        }
    }
    else {
        thisPhy->energyStats = FALSE;
    }


//

    // PHY model initialization
    //
    switch(thisPhy->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11b:
        case PHY802_11a: {
            Phy802_11Init(node, phyIndex, nodeInput);
            break;
        }
        case PHY802_11n: {
            Phy802_11Init(node, phyIndex, nodeInput, TRUE);
            break;
        }
        case PHY_ABSTRACT: {
            PhyAbstractInit(node, phyIndex, nodeInput);

            break;
        }
#endif // WIRELESS_LIB
#ifdef MODE5_LIB
        case PHY_MODE5: {
          Mode5::Phy* phy = new Mode5::Phy(node, phyIndex, nodeInput);
          // registers self
        } break;
#endif
#ifdef SENSOR_NETWORKS_LIB
        case PHY802_15_4: {
            Phy802_15_4Init(node, phyIndex, nodeInput);

            break;
        }
#endif //SENSOR_NETWORKS_LIB
#ifdef CELLULAR_LIB
        case PHY_GSM: {
            PhyGsmInit(node, phyIndex, nodeInput);

            break;
        }
#endif 

#ifdef WIRELESS_LIB
        case PHY_CELLULAR: {
             PhyCellularInit(node, phyIndex, nodeInput);

             break;
        }
#endif //WIRELESS_LIB

#ifdef MILITARY_RADIOS_LIB
        case PHY_FCSC_PROTOTYPE: {
            PhyFcscInit(node, phyIndex, nodeInput);

            break;
        }
#endif // MILITARY_RADIOS_LIB

#ifdef ADDON_LINK16
        case PHY_SPAWAR_LINK16: {
            PhySpawar_Link16Init(node, phyIndex, nodeInput);

            break;
        }
#endif // ADDON_LINK16

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            PhySatelliteRsvInit(node, phyIndex, nodeInput);
            break;
        }
#endif

#ifdef ADVANCED_WIRELESS_LIB
        case PHY802_16: {
            if (thisPhy->phyRxModel != RX_802_16) {
                char errorStr[MAX_STRING_LENGTH];
                 sprintf(errorStr,
                    "\nPHY802.16: Specified PHY-RX-MODEL is not supported!\n"
                    "Please specify PHY802.16 for it.");
                ERROR_ReportError(errorStr);
           }

            PhyDot16Init(node, phyIndex, nodeInput);

            break;
        }
#endif // ADVANCED_WIRELESS_LIB

#ifdef LTE_LIB
        case PHY_LTE: {
            PhyLteInit(node, phyIndex, nodeInput);
            break;
        }
#endif // LTE_LIB

#ifdef ADDON_BOEINGFCS
        case PHY_SRW_ABSTRACT: {
          SRW::Nexus::PHY::Init(node, phyIndex, nodeInput);
          break;
        }
#endif // FCS

        default: {
            ERROR_ReportError("Unknown or disabled PHY model");
        }
    }/*switch*/

#ifdef WIRELESS_LIB
    // ENERGY model initialization
    //
    ENERGY_Init(node, phyIndex, nodeInput);
    //
#endif // WIRELESS_LIB

} //PHY_CreateAPhyForMacLayer//


#ifdef ADDON_BOEINGFCS
//initialize pcom data structure
void PHY_PcomInit(PartitionData *partitionData)
{
    int i, j;
    BOOL wasFound;
    NodeInput pcomTableInput;
    Node *node = partitionData->firstNode;

    while (node)
    {
        for (i = 0; i < node->numberInterfaces; i++)
        {
            PhyData *thisPhy = node->phyData[i];
            if (thisPhy && thisPhy->phyRxModel == PCOM_BASED)
            {
                IO_ReadCachedFile(
                        node->nodeId,
                        thisPhy->networkAddress,
                        partitionData->nodeInput,
                        "PHY-PCOM-FILE",
                        &wasFound,
                        &pcomTableInput);

                ERROR_Assert(wasFound == TRUE, "Need to specify PHY-PCOM-FILE!");

                for (j = 0; j < pcomTableInput.numLines; j++) {
                    NodeId srcNodeId;
                    NodeId destNodeId;
                    NodeAddress sourceAddr;
                    NodeAddress destAddr;
                    NodeAddress networkId;
                    int numHostBits;
                    char iotoken[MAX_STRING_LENGTH];
                    char currentLine[MAX_STRING_LENGTH];
                    char networkIdStr[MAX_STRING_LENGTH];
                    char *delims = " \t\n";
                    char *token = NULL;
                    char *next;
                    char *strPtr;

                    clocktype startTime;
                    double pcomValue;

                    BOOL isValid;
                    BOOL validEntry = TRUE;
                    BOOL inSubnet = FALSE;

                    strcpy(currentLine, pcomTableInput.inputStrings[j]);
                    //get the start time for this pcom value
                    token = IO_GetDelimitedToken(iotoken, currentLine, delims, &next);

                    if (token == NULL)
                    {
                        ERROR_ReportError("Error reading in PCOM file, check formatting");
                    }

                    startTime = TIME_ConvertToClock(token);

                    //get the network id
                    token = IO_GetDelimitedToken(iotoken, next, delims, &next);

                    if (token == NULL)
                    {
                        ERROR_ReportError("Error reading in PCOM file, check formatting");
                    }

                    strcpy(networkIdStr, token);

                    IO_ParseNodeIdHostOrNetworkAddress(
                            networkIdStr,
                            &networkId,
                            &numHostBits,
                            &isValid);

                    //save network Id into networkId

                    //get the source node
                    token = IO_GetDelimitedToken(iotoken, next, delims, &next);

                    if (token == NULL)
                    {
                        ERROR_ReportError("Error reading in PCOM file, check formatting");
                    }

                    srcNodeId = atoi(token);

                    //get the destination node
                    token = IO_GetDelimitedToken(iotoken, next, delims, &next);

                    if (token == NULL)
                    {
                        ERROR_ReportError("Error reading in PCOM file, check formatting");
                    }

                    destNodeId = atoi(token);

                    token = IO_GetDelimitedToken(iotoken, next, delims, &next);

                    if (token == NULL)
                    {
                        ERROR_ReportError("Error reading in PCOM file, check formatting");
                    }

                    pcomValue = (double)atof(token);

                    PhyPcomItem *index;
                    PhyPcomItem *iterator;
                    PhyPcomItem *temp;
                    NodeId otherNodeId;
                    int ifaceIndex;

                    //remember, we are creating a table at the receiver for
                    //all potential transmitting nodes
                    //create and store data structure if this line
                    //has pcom data for this node
                    //assume links are symmetric
                    if ((srcNodeId == node->nodeId || destNodeId == node->nodeId) &&
                         (srcNodeId != destNodeId))
                    {
                        //printf("Node %d init: reading in nodes %d to %d\n",
                        //       node->nodeId, srcNodeId, destNodeId);
                        NodeAddress myAddress;
                        if (srcNodeId == node->nodeId)
                        {
                            otherNodeId = destNodeId;
                            myAddress =
                                    MAPPING_GetInterfaceAddressForInterface(node,
                                                                            srcNodeId,
                                                                            i);
                            numHostBits = MAPPING_GetNumHostBitsForInterface(node,
                                                                             srcNodeId,
                                                                             i);
                        }
                        else if (destNodeId == node->nodeId)
                        {
                            otherNodeId = srcNodeId;
                            myAddress =
                                    MAPPING_GetInterfaceAddressForInterface(node,
                                                                            destNodeId,
                                                                            i);
                            numHostBits = MAPPING_GetNumHostBitsForInterface(node,
                                                                             destNodeId,
                                                                             i);
                        }

                        //check to see if we are in the subnet
                        if (!IsIpAddressInSubnet(myAddress, networkId, numHostBits))
                        {
                            //printf("not valid address for subnet!\n");
                            validEntry = FALSE;
                        }

                        //this doesnt work in parallel
                        Node *otherNode = MAPPING_GetNodePtrFromHash(
                                                partitionData->nodeIdHash,
                                                otherNodeId);

#ifdef PARALLEL
                        if (otherNode == NULL)
                        {
                            //printf("pointer to node %d is null\n", otherNodeId);
                            //it might be in another partition
                            Node *oNode = MAPPING_GetNodePtrFromHash(
                                            partitionData->remoteNodeIdHash,
                                            otherNodeId);
                            if (oNode)
                            {
                                otherNode = oNode;
                            }
                        }
#endif

                        //check that other node is not null
                        if (otherNode)
                        {

                            //now that we have a pointer to the other node, we should loop through
                            //the interfaces and get the interfaceAddress for each one
                            //then we can see if the node has an interface in the subnet
                            //printf("number phys %d, interfaces %d\n", node->numberPhys, node->numberInterfaces);
                            //printf("finding interface address for %d(%d-%d)\n", otherNode->nodeId, otherNode->numberPhys, otherNode->numberInterfaces);
                            for (ifaceIndex = 0; ifaceIndex < otherNode->numberInterfaces; ifaceIndex++)
                            {
                                NodeAddress otherAddress =
                                        MAPPING_GetInterfaceAddressForInterface(otherNode,
                                        otherNodeId,
                                        ifaceIndex);
                                char othernodeaddress[MAX_STRING_LENGTH];
                                IO_ConvertIpAddressToString(otherAddress, othernodeaddress);
                                //printf("  interface %d ~ %s\n", ifaceIndex, othernodeaddress);
                                if (IsIpAddressInSubnet(otherAddress, networkId, numHostBits))
                                {
                                    //printf("valid!\n");
                                    //they are in the subnet
                                    inSubnet = TRUE;
                                    break;
                                }
                            }

                            if (validEntry && inSubnet)
                            {

                                temp = (PhyPcomItem*) MEM_malloc(sizeof(PhyPcomItem));
                                temp->startTime = startTime;
                                temp->endTime = CLOCKTYPE_MAX;
                                temp->pcom = pcomValue;
                                temp->next = NULL;

                                char clockStr[MAX_STRING_LENGTH];
                                TIME_PrintClockInSecond(startTime, clockStr);
                                //printf("  pcom table entry ~ time: %s, nodes %d to %d, pcom: %f\n",
                                //       clockStr, srcNodeId, destNodeId, pcomValue);

                                if (thisPhy->pcomTable[otherNodeId] == NULL) //first entry
                                {
                                    thisPhy->pcomTable[otherNodeId] = temp;
                                }
                                else //loop through linked list until we get to last entry
                                {
                                    iterator = thisPhy->pcomTable[otherNodeId];
                                    while (iterator)
                                    {
                                        index = iterator;
                                        iterator = iterator->next;
                                    }

                                    if (index->startTime >= temp->startTime)
                                    {
                                    //time is not in order, discard this entry
                                        ERROR_ReportWarning("Out of order time value");
                                        MEM_free(temp);
                                    }
                                    else
                                    {
                                        index->endTime = temp->startTime;
                                        index->next = temp;
                                        char clockStr[MAX_STRING_LENGTH];
                                        TIME_PrintClockInSecond(startTime, clockStr);
                                        //printf("Node %d added entry for node %d: time %s, pcom %f\n",
                                        //       node->nodeId, otherNodeId, clockStr, pcomValue);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        node = node->nextNodeData;
    }
}
#endif
/*
 * FUNCTION    PHY_Finalize
 * PURPOSE     Called at the end of simulation to collect the results of
 *             the simulation of the Phy Layer.
 *
 * Parameter:
 *     node:     node for which results are to be collected.
 */
void PHY_Finalize(Node *node) {
    int phyNum;
#ifdef ADDON_BOEINGFCS
    int i;
#endif
    for (phyNum = 0; (phyNum < node->numberPhys); phyNum++)
    {
        node->enterInterface(node->phyData[phyNum]->macInterfaceIndex);
#ifdef ADDON_BOEINGFCS
        //free pcom table
        if (node->phyData[phyNum]->phyRxModel == PCOM_BASED)
        {
            PhyPcomItem *iterator;
            PhyPcomItem *temp;
            for (i = 0; i < node->numNodes; i++)
            {
                iterator = node->phyData[phyNum]->pcomTable[i];
                while (iterator)
                {
                    temp = iterator;
                    iterator = iterator->next;
                    MEM_free(temp);
                }
                node->phyData[phyNum]->pcomTable[i] = NULL;
            }
            MEM_free(node->phyData[phyNum]->pcomTable);
        }
#endif
        switch(node->phyData[phyNum]->phyModel) {
#ifdef WIRELESS_LIB
            case PHY802_11b:
            case PHY802_11a:
            case PHY802_11n: {
                Phy802_11Finalize(node, phyNum);

                break;
            }
            case PHY_ABSTRACT: {
                PhyAbstractFinalize(node, phyNum);

                break;
            }
#endif // WIRELESS_LIB
#ifdef MODE5_LIB
            case PHY_MODE5: {
                Mode5::Phy::finalize(node, phyNum);
            } break;
#endif
#ifdef CELLULAR_LIB
            case PHY_GSM: {
                PhyGsmFinalize(node, phyNum);

                break;
            }
#endif //CELLULAR_LIB
#ifdef WIRELESS_LIB
            case PHY_CELLULAR: {
                PhyCellularFinalize(node, phyNum);

                break;
            }
#endif //WIRELESS_LIB

#ifdef MILITARY_RADIOS_LIB
            case PHY_FCSC_PROTOTYPE: {
                PhyFcscFinalize(node, phyNum);

                break;
            }
#endif // MILITARY_RADIOS_LIB

#ifdef ADDON_LINK16
            case PHY_SPAWAR_LINK16: {
                PhySpawar_Link16Finalize(node, phyNum);

                break;
            }
#endif // ADDON_LINK16

#ifdef SATELLITE_LIB
            case PHY_SATELLITE_RSV: {
                PhySatelliteRsvFinalize(node, phyNum);
                break;
            }
#endif // SATELLITE_LIB

#ifdef ADVANCED_WIRELESS_LIB
            case PHY802_16: {
                PhyDot16Finalize(node, phyNum);
                break;
            }
#endif // ADVANCED_WIRELESS_LIB

#ifdef LTE_LIB
            case PHY_LTE: {
                PhyLteFinalize(node, phyNum);
                break;
            }
#endif // LTE_LIB

#ifdef SENSOR_NETWORKS_LIB
            case PHY802_15_4: {
                Phy802_15_4Finalize(node, phyNum);
                break;
            }
#endif // SENSOR_NETWORKS_LIB

#ifdef ADDON_BOEINGFCS
            case PHY_SRW_ABSTRACT: {
              SRW::Nexus::PHY::Finalize(node, phyNum);
              break;
            }
#endif // FCS
            default: {
                ERROR_ReportError("Unknown or disabled PHY model");
            }
        }

        ENERGY_PrintStats(node,phyNum);

        node->exitInterface();
    }
}



/*
 * FUNCTION    PHY_ProcessEvent
 * PURPOSE     Models the behaviour of the Phy Layer on receiving the
 *             message encapsulated in msgHdr
 *
 * Parameters:
 *     node:     node which received the message
 *     msgHdr:   message received by the layer
 */
void PHY_ProcessEvent(Node *node, Message *msg) {
    int phyIndex = MESSAGE_GetInstanceId(msg);
    node->enterInterface(node->phyData[phyIndex]->macInterfaceIndex);

    switch(node->phyData[phyIndex]->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11n:
        case PHY802_11b:
        case PHY802_11a: {
            switch (msg->eventType) {
                case MSG_PHY_TransmissionEnd: {
                    Phy802_11TransmissionEnd(node, phyIndex);
                    MESSAGE_Free(node, msg);

                    break;
                }

                default: abort();
            }

            break;
        }
#ifdef MODE5_LIB
        case PHY_MODE5: {
            Mode5::Phy::layer(node, phyIndex, msg);
        } break;
#endif
        case PHY_ABSTRACT: {
            switch (msg->eventType) {
                case MSG_PHY_TransmissionEnd: {
                    PhyAbstractTransmissionEnd(node, phyIndex);
                    MESSAGE_Free(node, msg);

                    break;
                }
                case MSG_PHY_CollisionWindowEnd: {
                    PhyAbstractCollisionWindowEnd(node, phyIndex);
                    MESSAGE_Free(node, msg);

                    break;
                }

                default: abort();
            }

            break;
        }
#endif // WIRELESS_LIB
#ifdef SENSOR_NETWORKS_LIB
       case PHY802_15_4: {
            switch (msg->eventType) {
                case MSG_PHY_TransmissionEnd: {
                    Phy802_15_4TransmissionEnd(node, phyIndex);
                    MESSAGE_Free(node, msg);

                    break;
                }

                default: abort();
            }

            break;
        }
#endif //SENSOR_NETWORKS_LIB
#ifdef CELLULAR_LIB
        case PHY_GSM: {
            switch (msg->eventType) {
                case MSG_PHY_TransmissionEnd: {
                    PhyGsmTransmissionEnd(node, phyIndex);
                    MESSAGE_Free(node, msg);

                    break;
                }

                 default: abort();
            }

            break;
        }
#endif //CELLULAR_LIB
#ifdef WIRELESS_LIB
        case PHY_CELLULAR: {
            switch (msg->eventType) {
                case MSG_PHY_TransmissionEnd: {
                    PhyCellularTransmissionEnd(node, phyIndex);
                    MESSAGE_Free(node, msg);

                    break;
                }

                 default: abort();
           }

            break;
        }
#endif //WIRELESS_LIB

#ifdef MILITARY_RADIOS_LIB
        case PHY_FCSC_PROTOTYPE: {
            switch (msg->eventType) {
                case MSG_PHY_TransmissionEnd: {
                    PhyFcscTransmissionEnd(node, phyIndex);
                    MESSAGE_Free(node, msg);

                    break;
                }

                default: abort();
            }

            break;
        }
#endif /* MILITARY_RADIOS_LIB */

#ifdef ADDON_LINK16
        case PHY_SPAWAR_LINK16: {
            switch (msg->eventType) {
                case MSG_PHY_TransmissionEnd: {
                    PhySpawar_Link16TransmissionEnd(node, phyIndex);
                    MESSAGE_Free(node, msg);

                    break;
                }

                default: abort();
            }

            break;
        }
#endif // ADDON_LINK16

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            switch (msg->eventType) {
                case MSG_PHY_TransmissionEnd: {
                    PhySatelliteRsvTransmissionEnd(node, phyIndex, msg);
                    MESSAGE_Free(node, msg);

                    break;
                }

                default: abort();
            }

            break;
        }
#endif // SATELLITE_LIB

#ifdef ADVANCED_WIRELESS_LIB
        case PHY802_16: {
            switch (msg->eventType) {
                case MSG_PHY_TransmissionEnd: {
                    PhyDot16TransmissionEnd(node, phyIndex, msg);
                    MESSAGE_Free(node, msg);
                    break;
                }
                default: abort();
            }
            break;
        }
#endif // ADVANCED_WIRELESS_LIB

#ifdef LTE_LIB
        case PHY_LTE: {
            switch (msg->eventType) {
                case MSG_PHY_TransmissionEnd: {
                    PhyLteTransmissionEnd(node, phyIndex, msg);
                    MESSAGE_Free(node, msg);
                    break;
                }
                case MSG_PHY_LTE_TransmissionEndInEstablishment: {
                    PhyLteTransmissionEndInEstablishment(
                                                        node,
                                                        phyIndex,
                                                        msg);
                    MESSAGE_Free(node, msg);
                    break;
                }
                case MSG_PHY_LTE_NonServingCellMeasurementInterval: {
                    PhyLteNonServingCellMeasurementIntervalExpired(
                                                         node,
                                                         phyIndex);
                    MESSAGE_Free(node, msg);
                    break;
                }
                case MSG_PHY_LTE_NonServingCellMeasurementPeriod: {
                    PhyLteNonServingCellMeasurementPeriodExpired(
                                                            node,
                                                            phyIndex);
                    MESSAGE_Free(node, msg);
                    break;
                }
                case MSG_PHY_LTE_StartTransmittingSignalInEstablishment: {
                    PhyLteStartTransmittingSignalInEstablishment(
                                                            node,
                                                            phyIndex,
                                                            msg);
                    MESSAGE_Free(node, msg);
                    break;
                }
                case MSG_PHY_LTE_CheckingConnection: {
                    PhyLteCheckingConnectionExpired(node, phyIndex, msg);
                    MESSAGE_Free(node, msg);
                    break;
                }

                case MSG_PHY_LTE_InterferenceMeasurementTimerExpired: {
                    PhyLteInterferenceMeasurementTimerExpired(
                                                    node, phyIndex, msg);
                    MESSAGE_Free(node, msg);
                    break;
                }
                default: abort();
            }
            break;
        }
#endif // LTE_LIB


#ifdef ADDON_BOEINGFCS
        case PHY_SRW_ABSTRACT: {
          switch (msg->eventType) {
            case MSG_PHY_TransmissionEnd: {
              SRW::Nexus::PHY::TransmissionEnd(node, phyIndex);
              MESSAGE_Free(node, msg);
            } break;
            case MSG_PHY_CollisionWindowEnd: {
               SRW::Nexus::PHY::CollisionWindowEnd(node, phyIndex);
               MESSAGE_Free(node, msg);
            } break;
            default: abort(); break;
          }
        }
        break;
#endif // FCS
        default:
            ERROR_ReportError("Unknown or disabled PHY model\n");
    }
    node->exitInterface();
}

//
// FUNCTION    PHY_GetStatus
// PURPOSE     Retrieves the Phy's current status.
//

PhyStatusType
PHY_GetStatus(Node *node, int phyNum) {
    switch(node->phyData[phyNum]->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11n:
        case PHY802_11b:
        case PHY802_11a: {
            return Phy802_11GetStatus(node, phyNum);

            break;
        }
        case PHY_ABSTRACT: {
            return PhyAbstractGetStatus(node, phyNum);

            break;
        }
#endif // WIRELESS_LIB
#ifdef MODE5_LIB
        case PHY_MODE5: {
          Mode5::Phy::state(node, phyNum);
        } break;
#endif
#ifdef SENSOR_NETWORKS_LIB
        case PHY802_15_4: {
            return Phy802_15_4GetStatus(node, phyNum);

            break;
        }
#endif //SENSOR_NETWORKS_LIB

#ifdef CELLULAR_LIB
        case PHY_GSM: {
            return PhyGsmGetStatus(node, phyNum);

            break;
        }
#endif //CELLULAR_LIB

#ifdef WIRELESS_LIB
        case PHY_CELLULAR: {
            return PhyCellularGetStatus(node, phyNum);

            break;
         }
#endif //WIRELESS_LIB

#ifdef ADDON_LINK16
        case PHY_SPAWAR_LINK16: {
            return PhySpawar_Link16GetStatus(node, phyNum);

            break;
        }
#endif // ADDON_LINK16

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            return PhySatelliteRsvGetStatus(node, phyNum);

            break;
        }
#endif // SATELLITE_LIB

#ifdef ADVANCED_WIRELESS_LIB
        case PHY802_16: {
            return PhyDot16GetStatus(node, phyNum);

            break;
        }
#endif // ADVANCED_WIRELESS_LIB

#ifdef LTE_LIB
        case PHY_LTE: {
            //return PhyLteGetStatus(node, phyNum);

            break;
        }
#endif // LTE_LIB

#ifdef ADDON_BOEINGFCS
        case PHY_SRW_ABSTRACT: {
          return SRW::Nexus::PHY::GetStatus(node, phyNum);
        } break;
#endif // FCS
        default: {
            ERROR_ReportError("Unknown or disabled PHY model\n");
        }
    }

    // Never reachable, but required to suppress a warning by some compilers.
    return (PhyStatusType) 0;
}

// /**
// FUNCTION   :: AbstractAddCFPropInfo
// LAYER      :: Physical
// Purpose    :: Add the corresponding info field to indicate that a packet
//               wants to be treated using the abstract contention free
//               propagation.
// PARAMETERS ::
// + node      : Node *      : pointer to node
// + msg       : Message *   : Packet to be sent
// + destAddr  : NodeAddress : destination (next hop) of this packet
// RETURN     :: void        : NULL
// **/
static
void AbstractAddCFPropInfo(Node* node, Message *msg, NodeAddress destAddr)
{
    NodeId destId;
    char* infoPtr;

    //destId = MAPPING_GetNodeIdFromInterfaceAddress(node, destAddr);
    destId = destAddr;

    infoPtr = MESSAGE_AddInfo(node,
                              msg,
                              sizeof(NodeId),
                              INFO_TYPE_AbstractCFPropagation);

    ERROR_Assert(infoPtr != NULL, "Unable to add an info field!");

    memcpy(infoPtr, (char*) &destId, sizeof(NodeId));
}


// /**
// API                        :: PHY_StartTransmittingSignal
// LAYER                      :: Physical
// PURPOSE                    :: starts transmitting a packet,
//                               accepts a parameter called duration which
//                               specifies transmission delay.Used for
//                               non-byte aligned messages.
//                               Function is being overloaded
// PARAMETERS                 ::
// + node                      : Node *    : node pointer to node
// + phyNum                    : int       : interface index
// + msg                       : Message*  : packet to be sent
// + useMacLayerSpecifiedDelay : BOOL      : use delay specified by MAC
// + delayUntilAirborne        : clocktype : delay until airborne
// RETURN                     :: void
// **/
void PHY_StartTransmittingSignal(
    Node *node,
    int phyNum,
    Message *msg,
    clocktype duration,
    BOOL useMacLayerSpecifiedDelay,
    clocktype delayUntilAirborne,
    NodeAddress destAddr)
{
#ifdef ADDON_DB
    StatsDBAddMessageNextPrevHop(
        node,
        msg,
        destAddr,
        NetworkIpGetInterfaceAddress(
            node,
            node->phyData[phyNum]->macInterfaceIndex));
    if (node->phyData[phyNum]->phyModel != PHY_LTE)
    {
        StatsDB_PhyRecordStartTransmittingSignal(node, phyNum, msg);
    }
#endif

    // check if need abstract contention free propagation
    if (node->phyData[phyNum]->contentionFreeProp && destAddr != ANY_DEST)
    {
        AbstractAddCFPropInfo(node, msg, destAddr);
    }

#ifdef ADDON_DB
    HandleMacDBConnectivity(node,
        node->phyData[phyNum]->macInterfaceIndex, msg, MAC_SendToPhy);
#endif

    switch(node->phyData[phyNum]->phyModel) {

#ifdef WIRELESS_LIB
        case PHY802_11n:
        case PHY802_11b:
        case PHY802_11a: {
            Phy802_11StartTransmittingSignal(
                node, phyNum, msg,
                useMacLayerSpecifiedDelay, delayUntilAirborne);
            break;
        }
        //As of now only PHY abstract has been modified to carry
        //transmission delay

        case PHY_ABSTRACT: {
            PhyAbstractStartTransmittingSignal(
                node, phyNum, msg, duration,
                useMacLayerSpecifiedDelay, delayUntilAirborne);
            break;
        }
#endif // WIRELESS_LIB
#ifdef MODE5_LIB
        case PHY_MODE5: {
            Mode5::Phy::txStart(node, phyNum, msg, useMacLayerSpecifiedDelay,
                                delayUntilAirborne, FALSE, 0.0);
        } break;
#endif
#ifdef SENSOR_NETWORKS_LIB

      case PHY802_15_4: {
            Phy802_15_4StartTransmittingSignal(
                node, phyNum, msg, duration,
                useMacLayerSpecifiedDelay, delayUntilAirborne);
            break;
        }
#endif //
#ifdef CELLULAR_LIB
        case PHY_GSM: {
            PhyGsmStartTransmittingSignal(
                node, phyNum, msg,
                useMacLayerSpecifiedDelay, delayUntilAirborne);
            break;
        }
#endif //CELLULAR_LIB

#ifdef WIRELESS_LIB
        case PHY_CELLULAR: {
            PhyCellularStartTransmittingSignal(
                node, phyNum, msg, duration,
                useMacLayerSpecifiedDelay, delayUntilAirborne);
            break;
         }
#endif //WIRELESS_LIB

#ifdef ADDON_LINK16
        case PHY_SPAWAR_LINK16: {
            PhySpawar_Link16StartTransmittingSignal(
                node, phyNum, msg,
                useMacLayerSpecifiedDelay, delayUntilAirborne);
            break;
        }
#endif // ADDON_LINK16

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            PhySatelliteRsvStartTransmittingSignal(node, phyNum, msg,
              useMacLayerSpecifiedDelay, delayUntilAirborne);
            break;
        }
#endif // SATELLITE_LIB

#ifdef ADVANCED_WIRELESS_LIB
        case PHY802_16: {
            PhyDot16StartTransmittingSignal(node, phyNum, msg,
              useMacLayerSpecifiedDelay, delayUntilAirborne);
            break;
        }
#endif // ADVANCED_WIRELESS_LIB

#ifdef LTE_LIB
        case PHY_LTE: {
            PhyLteStartTransmittingSignal(
                node, phyNum, msg,
              useMacLayerSpecifiedDelay, delayUntilAirborne);
            break;
        }
#endif // LTE_LIB

#ifdef ADDON_BOEINGFCS
        case PHY_SRW_ABSTRACT: {
          SRW::Nexus::PHY::StartTransmittingSignal(node, phyNum, msg, duration,
                                                useMacLayerSpecifiedDelay,
                                                delayUntilAirborne);
        } break;
#endif // FCS
        default: {
            ERROR_ReportError("Unknown or disabled PHY model\n");
            break;
        }
    }//switch//
}


//
// FUNCTION    PHY_StartTransmittingSignal
// PURPOSE     Starts transmitting a packet.
//

void PHY_StartTransmittingSignal(
    Node *node,
    int phyNum,
    Message *msg,
    int bitSize,
    BOOL useMacLayerSpecifiedDelay,
    clocktype delayUntilAirborne,
    NodeAddress destAddr)
{
#ifdef ADDON_DB
    StatsDBAddMessageNextPrevHop(
        node,
        msg,
        destAddr,
        NetworkIpGetInterfaceAddress(
            node,
            node->phyData[phyNum]->macInterfaceIndex));
    if (node->phyData[phyNum]->phyModel != PHY_LTE)
    {
        StatsDB_PhyRecordStartTransmittingSignal(node, phyNum, msg);
    }
#endif

    // check if need abstract contention free propagation
    if (node->phyData[phyNum]->contentionFreeProp && destAddr != ANY_DEST)
    {
        AbstractAddCFPropInfo(node, msg, destAddr);
    }

#ifdef ADDON_DB
    HandleMacDBConnectivity(node,
        node->phyData[phyNum]->macInterfaceIndex, msg, MAC_SendToPhy);
#endif

    switch(node->phyData[phyNum]->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11n:
        case PHY802_11b:
        case PHY802_11a: {
            Phy802_11StartTransmittingSignal(
                node, phyNum, msg,
                useMacLayerSpecifiedDelay, delayUntilAirborne);

            break;
        }
        case PHY_ABSTRACT: {
            PhyAbstractStartTransmittingSignal(
                node, phyNum, msg, bitSize,
                useMacLayerSpecifiedDelay, delayUntilAirborne);

            break;
        }
#endif // WIRELESS_LIB
#ifdef MODE5_LIB
        case PHY_MODE5: {
            Mode5::Phy::txStart(node, phyNum, msg, useMacLayerSpecifiedDelay,
                                delayUntilAirborne, FALSE, 0.0);
        } break;
#endif
#ifdef SENSOR_NETWORKS_LIB

       case PHY802_15_4: {
            Phy802_15_4StartTransmittingSignal(
                node, phyNum, msg,
                useMacLayerSpecifiedDelay, delayUntilAirborne);
            break;
        }
#endif

#ifdef CELLULAR_LIB
        case PHY_GSM: {
            PhyGsmStartTransmittingSignal(
                node, phyNum, msg,
                useMacLayerSpecifiedDelay, delayUntilAirborne);

            break;
        }
#endif //CELLULAR_LIB

#ifdef WIRELESS_LIB
        case PHY_CELLULAR: {
            PhyCellularStartTransmittingSignal(
                node, phyNum, msg, 0,
                useMacLayerSpecifiedDelay, delayUntilAirborne);

            break;
        }
#endif //WIRELESS_LIB

#ifdef ADDON_LINK16
        case PHY_SPAWAR_LINK16: {
            PhySpawar_Link16StartTransmittingSignal(
                node, phyNum, msg,
                useMacLayerSpecifiedDelay, delayUntilAirborne);

            break;
        }
#endif // ADDON_LINK16

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            PhySatelliteRsvStartTransmittingSignal(
                                                node, phyNum, msg,
                                                useMacLayerSpecifiedDelay, delayUntilAirborne);

            break;
        }
#endif // SATELLITE_LIB

#ifdef ADVANCED_WIRELESS_LIB
        case PHY802_16: {
            PhyDot16StartTransmittingSignal(
                  node, phyNum, msg,
                  useMacLayerSpecifiedDelay, delayUntilAirborne);

            break;
        }
#endif // ADVANCED_WIRELESS_LIB

#ifdef LTE_LIB
        case PHY_LTE: {
            PhyLteStartTransmittingSignal(
                  node, phyNum, msg,
                  useMacLayerSpecifiedDelay, delayUntilAirborne);

            break;
        }
#endif // LTE_LIB

#ifdef ADDON_BOEINGFCS
        case PHY_SRW_ABSTRACT: {
          SRW::Nexus::PHY::StartTransmittingSignal(node, phyNum, msg, bitSize,
                                                useMacLayerSpecifiedDelay,
                                                delayUntilAirborne);
        } break;
#endif // FCS
        default: {
            ERROR_ReportError("Unknown or disabled PHY model\n");
        }
    }//switch//
}

//
// FUNCTION    PHY_StartTransmittingSignal
// PURPOSE     Starts transmitting a packet.
//

void PHY_StartTransmittingSignal(
    Node *node,
    int phyNum,
    Message *msg,
    BOOL useMacLayerSpecifiedDelay,
    clocktype delayUntilAirborne,
    NodeAddress destAddr)
{
#ifdef ADDON_DB
    StatsDBAddMessageNextPrevHop(
        node,
        msg,
        destAddr,
        NetworkIpGetInterfaceAddress(
            node,
            node->phyData[phyNum]->macInterfaceIndex));
    if (node->phyData[phyNum]->phyModel != PHY_LTE)
    {
        StatsDB_PhyRecordStartTransmittingSignal(node, phyNum, msg);
    }
#endif

    // check if need abstract contention free propagation
    if (node->phyData[phyNum]->contentionFreeProp && destAddr != ANY_DEST)
    {
        AbstractAddCFPropInfo(node, msg, destAddr);
    }

#ifdef ADDON_DB
    HandleMacDBConnectivity(node,
        node->phyData[phyNum]->macInterfaceIndex, msg, MAC_SendToPhy);
#endif
    switch(node->phyData[phyNum]->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11n:
        case PHY802_11b:
        case PHY802_11a: {
            Phy802_11StartTransmittingSignal(
                node, phyNum, msg,
                useMacLayerSpecifiedDelay, delayUntilAirborne);

            break;
        }
        case PHY_ABSTRACT: {
            PhyAbstractStartTransmittingSignal(
                node, phyNum, msg,
                useMacLayerSpecifiedDelay, delayUntilAirborne);

            break;
        }
#endif // WIRELESS_LIB
#ifdef MODE5_LIB
        case PHY_MODE5: {
            Mode5::Phy::txStart(node, phyNum, msg, useMacLayerSpecifiedDelay,
                                delayUntilAirborne, FALSE, 0.0);
        } break;
#endif
#ifdef SENSOR_NETWORKS_LIB

      case PHY802_15_4: {
            Phy802_15_4StartTransmittingSignal(
                node, phyNum, msg,
                useMacLayerSpecifiedDelay, delayUntilAirborne);
            break;
        }
#endif
#ifdef CELLULAR_LIB
        case PHY_GSM: {
            PhyGsmStartTransmittingSignal(
                node, phyNum, msg,
                useMacLayerSpecifiedDelay, delayUntilAirborne);

            break;
        }
#endif //CELLULAR_LIB

#ifdef WIRELESS_LIB
        case PHY_CELLULAR: {
            PhyCellularStartTransmittingSignal(
                node, phyNum, msg, 0,
                useMacLayerSpecifiedDelay, delayUntilAirborne);

            break;
        }
#endif //WIRELESS_LIB

#ifdef ADDON_LINK16
        case PHY_SPAWAR_LINK16: {
            PhySpawar_Link16StartTransmittingSignal(
                node, phyNum, msg,
                useMacLayerSpecifiedDelay, delayUntilAirborne);

            break;
        }
#endif // ADDON_LINK16

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            PhySatelliteRsvStartTransmittingSignal(
                                                node, phyNum, msg,
                                                useMacLayerSpecifiedDelay, delayUntilAirborne);

            break;
        }
#endif // SATELLITE_LIB

#ifdef ADVANCED_WIRELESS_LIB
        case PHY802_16: {
            PhyDot16StartTransmittingSignal(
                  node, phyNum, msg,
                  useMacLayerSpecifiedDelay, delayUntilAirborne);

            break;
        }
#endif // ADVANCED_WIRELESS_LIB

#ifdef LTE_LIB
        case PHY_LTE: {
            PhyLteStartTransmittingSignal(
                  node, phyNum, msg,
                  useMacLayerSpecifiedDelay, delayUntilAirborne);

            break;
        }
#endif // LTE_LIB

#ifdef ADDON_BOEINGFCS
        case PHY_SRW_ABSTRACT: {
          SRW::Nexus::PHY::StartTransmittingSignal(node, phyNum, msg,
                                                useMacLayerSpecifiedDelay,
                                                delayUntilAirborne);
        } break;
#endif // FCS
        default: {
            ERROR_ReportError("Unknown or disabled PHY model\n");
        }
    }//switch//
}


void PHY_SignalArrivalFromChannel(
   Node* node,
   int phyIndex,
   int channelIndex,
   PropRxInfo *propRxInfo)
{
#ifdef CYBER_LIB
    /* Drop the packet at the mac layer if the mac protocl is not
    supported for app jamming */
    if (node->phyData[phyIndex]->jammerStatistics == TRUE)
    {
        JamType *infoPtr = (JamType *) MESSAGE_ReturnInfo(propRxInfo->txMsg,INFO_TYPE_JAM);
        if (infoPtr != NULL)
        {
            if (*infoPtr == JAM_TYPE_APP)
            {
                node->phyData[phyIndex]->jamInstances++;
                if (node->phyData[phyIndex]->jamInstances == 1)
                {
                    node->phyData[phyIndex]->jammerStartTime = node->getNodeTime();
                    node->phyData[phyIndex]->jammerEndTime = clocktype(node->partitionData->maxSimClock);
                    node->phyData[phyIndex]->jamDuration += (node->phyData[phyIndex]->jammerEndTime -
                        node->phyData[phyIndex]->jammerStartTime);
                }
            }
        }
    }
#endif
    node->enterInterface(node->phyData[phyIndex]->macInterfaceIndex);

    switch(node->phyData[phyIndex]->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11n:
        case PHY802_11b:
        case PHY802_11a: {
            Phy802_11SignalArrivalFromChannel(
                node, phyIndex, channelIndex, propRxInfo);

            break;
        }
        case PHY_ABSTRACT: {
            PhyAbstractSignalArrivalFromChannel(
                node,
                phyIndex,
                channelIndex,
                propRxInfo);

            break;
        }
#endif // WIRELESS_LIB
#ifdef MODE5_LIB
        case PHY_MODE5: {
            Mode5::Phy::arrival(node, phyIndex, channelIndex, propRxInfo);
        } break;
#endif
#ifdef SENSOR_NETWORKS_LIB
       case PHY802_15_4: {
            Phy802_15_4SignalArrivalFromChannel(
                node, phyIndex, channelIndex, propRxInfo);

            break;
        }

#endif

#ifdef CELLULAR_LIB
        case PHY_GSM: {
            PhyGsmSignalArrivalFromChannel(
                node, phyIndex, channelIndex, propRxInfo);

            break;
        }
#endif //CELLULAR_LIB

#ifdef WIRELESS_LIB
        case PHY_CELLULAR: {
            PhyCellularSignalArrivalFromChannel(
                node, phyIndex, channelIndex, propRxInfo);

            break;
        }
#endif //WIRELESS_LIB

#ifdef MILITARY_RADIOS_LIB
        case PHY_FCSC_PROTOTYPE: {
            PhyFcscSignalArrivalFromChannel(
                node, phyIndex, channelIndex, propRxInfo);

            break;
        }
#endif /* FCSC */

#ifdef ADDON_LINK16
        case PHY_SPAWAR_LINK16: {
            PhySpawar_Link16SignalArrivalFromChannel(
                node, phyIndex, channelIndex, propRxInfo);

            break;
        }
#endif // ADDON_LINK16

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            PhySatelliteRsvSignalArrivalFromChannel(
                node, phyIndex, channelIndex, propRxInfo);

            break;
        }
#endif // SATELLITE_LIB

#ifdef ADVANCED_WIRELESS_LIB
        case PHY802_16: {
            PhyDot16SignalArrivalFromChannel(
                node, phyIndex, channelIndex, propRxInfo);

            break;
        }
#endif // ADVANCED_WIRELESS_LIB

#ifdef LTE_LIB
        case PHY_LTE: {
            PhyLteSignalArrivalFromChannel(
                node, phyIndex, channelIndex, propRxInfo);

            break;
        }
#endif // LTE_LIB

#ifdef ADDON_BOEINGFCS
        case PHY_SRW_ABSTRACT: {
          SRW::Nexus::PHY::SignalArrivalFromChannel(node, phyIndex, channelIndex, propRxInfo);
        } break;
#endif // FCS
        default: {
            ERROR_ReportError("Unknown or disabled PHY model\n");
        }
    }//switch//
    node->exitInterface();
}


void PHY_SignalEndFromChannel(
   Node* node,
   int phyIndex,
   int channelIndex,
   PropRxInfo *propRxInfo)
{
#ifdef CYBER_LIB
    /* Drop the packet at the mac layer if the mac protocl is not
    supported for app jamming */
    if (node->phyData[phyIndex]->jammerStatistics == TRUE)
    {
        JamType *infoPtr = (JamType *) MESSAGE_ReturnInfo(propRxInfo->txMsg,INFO_TYPE_JAM);
        if (infoPtr != NULL)
        {
            if (*infoPtr == JAM_TYPE_APP)
            {
                node->phyData[phyIndex]->jamInstances--;
                if (node->phyData[phyIndex]->jamInstances == 0)
                {
                    node->phyData[phyIndex]->jamDuration -= (node->phyData[phyIndex]->jammerEndTime -
                        node->phyData[phyIndex]->jammerStartTime);
                    node->phyData[phyIndex]->jammerEndTime = node->getNodeTime();
                    node->phyData[phyIndex]->jamDuration += (node->phyData[phyIndex]->jammerEndTime -
                        node->phyData[phyIndex]->jammerStartTime);
                }
            }
        }
    }
#endif
    node->enterInterface(node->phyData[phyIndex]->macInterfaceIndex);

    switch(node->phyData[phyIndex]->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11n:
        case PHY802_11b:
        case PHY802_11a: {
            Phy802_11SignalEndFromChannel(
                node, phyIndex, channelIndex, propRxInfo);

            break;
        }
        case PHY_ABSTRACT: {
            PhyAbstractSignalEndFromChannel(
                node, phyIndex, channelIndex, propRxInfo);

            break;
        }
#endif // WIRELESS_LIB
#ifdef MODE5_LIB
        case PHY_MODE5: {
          Mode5::Phy::end(node, phyIndex, channelIndex, propRxInfo);
        } break;
#endif
#ifdef SENSOR_NETWORKS_LIB
         case PHY802_15_4: {
            Phy802_15_4SignalEndFromChannel(
                node, phyIndex, channelIndex, propRxInfo);

            break;
        }
#endif //SENSOR_NETWORKS_LIB

#ifdef CELLULAR_LIB
        case PHY_GSM: {
            PhyGsmSignalEndFromChannel(
                node, phyIndex, channelIndex, propRxInfo);

            break;
        }
#endif //CELLULAR_LIB

#ifdef WIRELESS_LIB
        case PHY_CELLULAR: {
            PhyCellularSignalEndFromChannel(
                node, phyIndex, channelIndex, propRxInfo);

            break;
        }
#endif //WIRELESS_LIB

#ifdef MILITARY_RADIOS_LIB
        case PHY_FCSC_PROTOTYPE: {
            PhyFcscSignalEndFromChannel(
                node, phyIndex, channelIndex, propRxInfo);

            break;
        }
#endif /* MILITARY_RADIOS_LIB */

#ifdef ADDON_LINK16
        case PHY_SPAWAR_LINK16: {
            PhySpawar_Link16SignalEndFromChannel(
                node, phyIndex, channelIndex, propRxInfo);

            break;
        }
#endif // ADDON_LINK16

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            PhySatelliteRsvSignalEndFromChannel(node, phyIndex, channelIndex, propRxInfo);

            break;
        }
#endif // SATELLITE_LIB_RSV

#ifdef ADVANCED_WIRELESS_LIB
        case PHY802_16: {
            PhyDot16SignalEndFromChannel(
                node, phyIndex, channelIndex, propRxInfo);

            break;
        }
#endif // ADVANCED_WIRELESS_LIB

#ifdef LTE_LIB
        case PHY_LTE: {
            PhyLteSignalEndFromChannel(
                node, phyIndex, channelIndex, propRxInfo);

            break;
        }
#endif // LTE_LIB

#ifdef ADDON_BOEINGFCS
        case PHY_SRW_ABSTRACT: {
          SRW::Nexus::PHY::SignalEndFromChannel(node, phyIndex, channelIndex, propRxInfo);
        } break;
#endif // FCS
        default: {
            ERROR_ReportError("Unknown or disabled PHY model\n");
        }
    }//switch//

    node->exitInterface();
}

void PHY_ChannelListeningSwitchNotification(
   Node* node,
   int phyIndex,
   int channelIndex,
   BOOL startListening)
{
    switch (node->phyData[phyIndex]->phyModel) {
#ifdef ADVANCED_WIRELESS_LIB
        case PHY802_16: {

            PhyDot16ChannelListeningSwitchNotification(
                node,
                phyIndex,
                channelIndex,
                startListening);

            break;
        }
#endif // ADVANCED_WIRELESS_LIB


#ifdef WIRELESS_LIB
        case PHY802_11n:
        case PHY802_11b:
        case PHY802_11a: {
            Phy802_11ChannelListeningSwitchNotification(
                node,
                phyIndex,
                channelIndex,
                startListening);
            break;
        }
        case PHY_ABSTRACT:{
            PhyAbstractChannelListeningSwitchNotification(
                node,
                phyIndex,
                channelIndex,
                startListening);
            break;
        }
#endif // WIRELESS_LIB

        default: {
            // the PHY model doesn't want to response to this notification
            break;
        }
    }
}

Int64 PHY_GetTxDataRate(Node *node, int phyIndex) {
    switch(node->phyData[phyIndex]->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11n:
        case PHY802_11b:
        case PHY802_11a: {
            return Phy802_11GetTxDataRate(node->phyData[phyIndex]);
        }
        case PHY_ABSTRACT: {
            return PhyAbstractGetDataRate(node->phyData[phyIndex]);
        }
#endif // WIRELESS_LIB
#ifdef MODE5_LIB
        case PHY_MODE5: {
            return Mode5::Phy::getDataRate(node, phyIndex);
        } break;
#endif
#ifdef SENSOR_NETWORKS_LIB
         case PHY802_15_4: {
            return Phy802_15_4GetDataRate(node->phyData[phyIndex]);
        }
#endif //SENSOR_NETWORKS_LIB

#ifdef CELLULAR_LIB
        case PHY_GSM: {
            return PhyGsmGetDataRate(node->phyData[phyIndex]);
        }
#endif //CELLULAR_LIB

#ifdef WIRELESS_LIB
        case PHY_CELLULAR: {
            return PhyCellularGetDataRate(node, node->phyData[phyIndex]);
        }
#endif //WIRELESS_LIB

#ifdef ADDON_LINK16
        case PHY_SPAWAR_LINK16: {
            return PhySpawar_Link16GetDataRate(node->phyData[phyIndex]);
        }
#endif // ADDON_LINK16

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            return PhySatelliteRsvGetTxDataRate(node->phyData[phyIndex]);
        }
#endif // SATELLITE_LIB

#ifdef ADVANCED_WIRELESS_LIB
        case PHY802_16: {
            return PhyDot16GetTxDataRate(node->phyData[phyIndex]);
        }
#endif // ADVANCED_WIRELESS_LIB

#ifdef LTE_LIB
        case PHY_LTE: {
            //TODO return PhyLteGetTxDataRate(node->phyData[phyIndex]);
            return 0;
        }
#endif // LTE_LIB

#ifdef ADDON_BOEINGFCS
        case PHY_SRW_ABSTRACT: {
          return SRW::Nexus::PHY::GetDataRate(node->phyData[phyIndex]);
        } break;
#endif // FCS
        default: {
            ERROR_ReportError("Unknown or disabled PHY model\n");
        }
    }//switch//

    // Never reachable, but required to suppress a warning by some compilers.
    return 0;
}



Int64 PHY_GetRxDataRate(Node *node, int phyIndex) {
    switch(node->phyData[phyIndex]->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11n:
        case PHY802_11b:
        case PHY802_11a: {
            return Phy802_11GetRxDataRate(node->phyData[phyIndex]);
        }
        case PHY_ABSTRACT: {
            return PhyAbstractGetDataRate(node->phyData[phyIndex]);
        }
#endif // WIRELESS_LIB

#ifdef SENSOR_NETWORKS_LIB
        case PHY802_15_4: {
            return Phy802_15_4GetDataRate(node->phyData[phyIndex]);
        }
#endif //SENSOR_NETWORKS_LIB

#ifdef CELLULAR_LIB
        case PHY_GSM: {
            return PhyGsmGetDataRate(node->phyData[phyIndex]);
        }
#endif //CELLULAR_LIB

#ifdef WIRELESS_LIB
        case PHY_CELLULAR: {
            return PhyCellularGetDataRate(node, node->phyData[phyIndex]);
        }
#endif //WIRELESS_LIB

#ifdef ADDON_LINK16
        case PHY_SPAWAR_LINK16: {
            return PhySpawar_Link16GetDataRate(node->phyData[phyIndex]);
        }
#endif // ADDON_LINK16

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            return PhySatelliteRsvGetRxDataRate(node->phyData[phyIndex]);
        }
#endif // SATELLITE_LIB

#ifdef ADVANCED_WIRELESS_LIB
        case PHY802_16: {
            return PhyDot16GetRxDataRate(node->phyData[phyIndex]);
        }
#endif // ADVANCED_WIRELESS_LIB

#ifdef ADDON_BOEINGFCS
        case PHY_SRW_ABSTRACT: {
          return SRW::Nexus::PHY::GetDataRate(node->phyData[phyIndex]);
        } break;
#endif // FCS
        default: {
            ERROR_ReportError("Unknown or disabled PHY model\n");
        }
    }//switch//

    // Never reachable, but required to suppress a warning by some compilers.
    return 0;
}


void PHY_SetTxDataRateType(Node *node, int phyIndex, int dataRateType) {
    PhyData* thisPhy = node->phyData[phyIndex];

    switch(thisPhy->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11b:
        case PHY802_11a: {
            Phy802_11SetTxDataRateType(thisPhy, dataRateType);
            return;
        }
        case PHY802_11n:{
            ERROR_ReportError("Incorrect API used");
            break;
        }
#endif // WIRELESS_LIB

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            PhySatelliteRsvSetTxDataRateType(thisPhy, dataRateType);
            return;
        }
#endif // SATELLITE_LIB

        default:{
        }
    }
}


int PHY_GetRxDataRateType(Node *node, int phyIndex) {
    PhyData* thisPhy = node->phyData[phyIndex];

    switch(thisPhy->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11b:
        case PHY802_11a: {
            return Phy802_11GetRxDataRateType(thisPhy);
        }

        case PHY802_11n:{
            ERROR_ReportError("Incorrect API used");
            break;
        }
#endif // WIRELESS_LIB

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            return PhySatelliteRsvGetRxDataRateType(thisPhy);
        }
#endif // SATELLITE_LIB

        default: {
        }
    }
    return 0;
}


int PHY_GetTxDataRateType(Node *node, int phyIndex) {
    PhyData* thisPhy = node->phyData[phyIndex];

    switch(thisPhy->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11n:
        case PHY802_11b:
        case PHY802_11a: {
            return Phy802_11GetTxDataRateType(thisPhy);
        }
#endif // WIRELESS_LIB

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            return PhySatelliteRsvGetTxDataRateType(thisPhy);
        }
#endif // SATELLITE_LIB

        default: {
        }
    }
    return 0;
}


void PHY_SetLowestTxDataRateType(Node *node, int phyIndex) {
    PhyData* thisPhy = node->phyData[phyIndex];

    switch(thisPhy->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11n:
        case PHY802_11b:
        case PHY802_11a: {
            Phy802_11SetLowestTxDataRateType(thisPhy);
            return;
        }
#endif // WIRELESS_LIB

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            PhySatelliteRsvSetLowestTxDataRateType(thisPhy);
            return;
        }
#endif // SATELLITE_LIB

        default: {
        }
    }
}


void PHY_GetLowestTxDataRateType(Node* node, int phyIndex, int* dataRateType) {
    PhyData* thisPhy = node->phyData[phyIndex];

    switch(thisPhy->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11n:
        case PHY802_11b:
        case PHY802_11a: {
            Phy802_11GetLowestTxDataRateType(thisPhy, dataRateType);
            return;
        }
        case PHY_ABSTRACT: {
            *dataRateType = 0;
            return;
        }
#endif // WIRELESS_LIB

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            PhySatelliteRsvGetLowestTxDataRateType(thisPhy, dataRateType);
            return;
        }
#endif // SATELLITE_LIB

        default: {
        }
    }
}


void PHY_SetHighestTxDataRateType(Node* node, int phyIndex) {
    PhyData* thisPhy = node->phyData[phyIndex];

    switch(thisPhy->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11n:
        case PHY802_11b:
        case PHY802_11a: {
            Phy802_11SetHighestTxDataRateType(thisPhy);
            return;
        }
#endif // WIRELESS_LIB

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            PhySatelliteRsvSetHighestTxDataRateType(thisPhy);
            return;
        }
#endif // SATELLITE_LIB

        default: {
        }
    }
}


void PHY_GetHighestTxDataRateType(Node* node, int phyIndex, int* dataRateType) {
    PhyData* thisPhy = node->phyData[phyIndex];

    switch(thisPhy->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11n:
        case PHY802_11b:
        case PHY802_11a: {
            Phy802_11GetHighestTxDataRateType(thisPhy, dataRateType);
            return;
        }
        case PHY_ABSTRACT: {
            *dataRateType = 0;
            return;
        }
#endif // WIRELESS_LIB

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            PhySatelliteRsvGetHighestTxDataRateType(thisPhy, dataRateType);
            return;
        }
#endif // SATELLITE_LIB

        default: {
        }
    }
}


void PHY_SetHighestTxDataRateTypeForBC(
    Node* node,
    int phyIndex)
{
    PhyData* thisPhy = node->phyData[phyIndex];

    switch(thisPhy->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11n:
        case PHY802_11b:
        case PHY802_11a: {
            Phy802_11SetHighestTxDataRateTypeForBC(thisPhy);
            return;
        }
#endif // WIRELESS_LIB

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            PhySatelliteRsvSetHighestTxDataRateTypeForBC(thisPhy);
            return;
        }
#endif // SATELLITE_LIB

        default: {
        }
    }
}


void PHY_GetHighestTxDataRateTypeForBC(
    Node* node,
    int phyIndex,
    int* dataRateType)
{
    PhyData* thisPhy = node->phyData[phyIndex];

    switch(thisPhy->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11n:
        case PHY802_11b:
        case PHY802_11a: {
            Phy802_11GetHighestTxDataRateTypeForBC(thisPhy, dataRateType);
            return;
        }
        case PHY_ABSTRACT: {
            *dataRateType = 0;
            return;
        }
#endif // WIRELESS_LIB

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            PhySatelliteRsvGetHighestTxDataRateTypeForBC(thisPhy, dataRateType);
            return;
        }
#endif // SATELLITE_LIB

        default: {
        }
    }
}

PhyModel PHY_GetModel(
    Node* node,
    int phyNum)
{
    return node->phyData[phyNum]->phyModel;
}

AntennaModelType PHY_GetAntennaModelType(
    Node* node,
    int phyNum)
{
    return node->phyData[phyNum]->antennaData->antennaModelType;
}

//
// FUNCTION    PHY_GetTransmissionDuration
// PURPOSE     Calculates the transmission duration
//
clocktype PHY_GetTransmissionDuration(
    Node *node,
    int phyIndex,
    int dataRateIndex,
    int size)
{
    PhyData* thisPhy = node->phyData[phyIndex];

    switch(thisPhy->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11b:
        case PHY802_11a: {
            return Phy802_11GetFrameDuration(thisPhy, dataRateIndex, size);
        }
        case PHY802_11n:{
            ERROR_ReportError("Incorrect API used");
            break;
        }

        case PHY_ABSTRACT: {
            Int64 dataRate = PHY_GetTxDataRate(node, phyIndex);
            return PhyAbstractGetFrameDuration(thisPhy, size, dataRate);
        }
#endif // WIRELESS_LIB
#ifdef MODE5_LIB
        case PHY_MODE5: {
            return Mode5::Phy::frameDuration(node, phyIndex, size);
        } break;
#endif
#ifdef SENSOR_NETWORKS_LIB
      case PHY802_15_4: {
            int dataRate = PHY_GetTxDataRate(node, phyIndex);
            PhyData802_15_4* phy802_15_4 = (PhyData802_15_4*)thisPhy->phyVar;
            clocktype turnaroundTime = phy802_15_4->RxTxTurnAroundTime;
            return Phy802_15_4GetFrameDuration(thisPhy, size, dataRate) +
                    turnaroundTime;
        }
#endif

#ifdef CELLULAR_LIB
        case PHY_GSM: {
            int dataRate;
            clocktype delay;
            dataRate = PHY_GetTxDataRate(node, phyIndex);
            delay = PhyGsmGetTransmissionDuration(size, dataRate);
            return delay;
        }
#endif //CELLULAR_LIB

#ifdef WIRELESS_LIB
        case PHY_CELLULAR: {
            // not implemented yet.
            clocktype delay;
            delay = 10000;
            return delay;
        }
#endif //WIRELESS_LIB

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            int dataRate;
            clocktype delay;

            dataRate = PHY_GetTxDataRate(node, phyIndex);
            delay= PhySatelliteRsvGetTransmissionDuration(size, dataRate);

            return delay;
        }
#endif // SATELLITE_LIB
#ifdef ADVANCED_WIRELESS_LIB
        case PHY802_16: {
            // not implemented yet.
            return 10000;
        }
#endif // ADVANCED_WIRELESS_LIB

#ifdef ADDON_BOEINGFCS
        case PHY_SRW_ABSTRACT: {
          int dataRate = PHY_GetTxDataRate(node, phyIndex);
          return SRW::Nexus::PHY::GetFrameDuration(thisPhy, size, dataRate);
        } break;
#endif // FCS
        default: {
            ERROR_ReportError("Unknown or disabled PHY model\n");
        }
    }//switch//

    // Never reachable, but required to suppress a warning by some compilers.
    return 0;
}


//
// FUNCTION    PHY_GetTransmissionDelay
// PURPOSE     Calculates the transmission duration
//             based on the first (usually lowest) data rate
//             WARNING: This function call is to be replaced with
//             PHY_GetTransmissionDuration() with an appropriate data rate
//
clocktype PHY_GetTransmissionDelay(Node *node, int phyIndex, int size) {

    PhyData* thisPhy = node->phyData[phyIndex];
    int txDataRateType = 0;

#ifdef WIRELESS_LIB
    if (thisPhy->phyModel == PHY802_11b || thisPhy->phyModel == PHY802_11a
        || thisPhy->phyModel == PHY802_11n)
    {
        txDataRateType = Phy802_11GetTxDataRateType(thisPhy);
    }
#endif // WIRELESS_LIB

    return PHY_GetTransmissionDuration(node, phyIndex, txDataRateType, size);
}



//
// FUNCTION    PHY_SetTransmitPower
// PURPOSE     Sets the Radio's transmit power in mW.
//
void PHY_SetTransmitPower(
    Node *node,
    int phyIndex,
    double newTxPower_mW)
{
    switch(node->phyData[phyIndex]->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11n:
        case PHY802_11b:
        case PHY802_11a: {
            Phy802_11SetTransmitPower(node->phyData[phyIndex], newTxPower_mW);

            return;
        }

        case PHY_ABSTRACT: {
            PhyAbstractSetTransmitPower(node->phyData[phyIndex], newTxPower_mW);

            return;
        }
#endif // WIRELESS_LIB
#ifdef MODE5_LIB
        case PHY_MODE5: {
          Mode5::Phy::setTransmitPower(node, phyIndex, newTxPower_mW);

          return;
        } break;
#endif
#ifdef SENSOR_NETWORKS_LIB
   case PHY802_15_4: {
            Phy802_15_4SetTransmitPower(node->phyData[phyIndex], newTxPower_mW);

            return;
        }
#endif

#ifdef CELLULAR_LIB
        case PHY_GSM: {
            PhyGsmSetTransmitPower(node->phyData[phyIndex], newTxPower_mW);

            return;
        }
#endif //CELLULAR_LIB

#ifdef WIRELESS_LIB
        case PHY_CELLULAR: {
            PhyCellularSetTransmitPower(node, node->phyData[phyIndex], newTxPower_mW);

            return;
        }
#endif // WIRELESS_LIB

#ifdef ADDON_LINK16
        case PHY_SPAWAR_LINK16: {
            PhySpawar_Link16SetTransmitPower(node->phyData[phyIndex],
                                      newTxPower_mW);


            return;
        }
#endif // ADDON_LINK16

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            PhySatelliteRsvSetTransmitPower(node->phyData[phyIndex],
                                         newTxPower_mW);

            return;
        }
#endif // SATELLITE_LIB

#ifdef ADVANCED_WIRELESS_LIB
        case PHY802_16: {
            PhyDot16SetTransmitPower(node, phyIndex,
                                         newTxPower_mW);

            return;
        }
#endif // ADVANCED_WIRELESS_LIB

#ifdef ADDON_BOEINGFCS
        case PHY_SRW_ABSTRACT: {
          SRW::Nexus::PHY::SetTransmitPower(node->phyData[phyIndex], newTxPower_mW);
          return;
        } break;
#endif // FCS
        default: {
            ERROR_ReportError("Unknown or disabled PHY model\n");
        }
    }//switch//

    return;
}



//
// FUNCTION    PHY_GetTransmitPower
// PURPOSE     Gets the Radio's transmit power in mW.
//
void PHY_GetTransmitPower(
    Node *node,
    int phyIndex,
    double *txPower_mW)
{
    if (node->numberPhys < 1)
        {
        *txPower_mW = -1.0;
        return;
        }
    switch(node->phyData[phyIndex]->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11n:
        case PHY802_11b:
        case PHY802_11a: {
            Phy802_11GetTransmitPower(node->phyData[phyIndex], txPower_mW);

            return;
        }

        case PHY_ABSTRACT: {
            PhyAbstractGetTransmitPower(node->phyData[phyIndex], txPower_mW);

            return;
        }
#endif // WIRELESS_LIB
#ifdef MODE5_LIB
        case PHY_MODE5: {
            Mode5::Phy::getTransmitPower(node, phyIndex, txPower_mW);
            return;
        } break;
#endif
#ifdef SENSOR_NETWORKS_LIB
     case PHY802_15_4: {
            Phy802_15_4GetTransmitPower(node->phyData[phyIndex], txPower_mW);

            return;
        }
#endif


#ifdef CELLULAR_LIB
        case PHY_GSM: {
            PhyGsmGetTransmitPower(node->phyData[phyIndex], txPower_mW);

            return;
        }
#endif //CELLULAR_LIB

#ifdef WIRELESS_LIB
        case PHY_CELLULAR: {
            PhyCellularGetTransmitPower(node, node->phyData[phyIndex], txPower_mW);

            return;
        }
#endif //WIRELESS_LIB

#ifdef ADDON_LINK16
        case PHY_SPAWAR_LINK16: {
            PhySpawar_Link16GetTransmitPower(node->phyData[phyIndex], txPower_mW);

            return;
        }
#endif // ADDON_LINK16

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            PhySatelliteRsvGetTransmitPower(node->phyData[phyIndex], txPower_mW);

            return;
        }
#endif // SATELLITE_LIB

#ifdef ADVANCED_WIRELESS_LIB
        case PHY802_16: {
            PhyDot16GetTransmitPower(node, phyIndex, txPower_mW);

            return;
        }
#endif // ADVANCED_WIRELESS_LIB

#ifdef ADDON_BOEINGFCS
        case PHY_SRW_ABSTRACT: {
          SRW::Nexus::PHY::GetEstimatedTransmitPower(node->phyData[phyIndex], txPower_mW);
          return;
        } break;
#endif // FCS

#ifdef LTE_LIB
        case PHY_LTE:
        {
            *txPower_mW = NON_DB(PhyLteGetMaxTxPower_dBm(node, phyIndex));
            break;
        }
#endif // LTE_LIB
        default: {
            ERROR_ReportError("Unknown or disabled PHY model\n");
        }
    }//switch//

    return;
}



double PHY_GetLastSignalsAngleOfArrival(Node* node, int phyNum) {
    switch(node->phyData[phyNum]->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11n:
        case PHY802_11b:
        case PHY802_11a: {
            return Phy802_11GetLastAngleOfArrival(node, phyNum);

            break;
        }
#endif // WIRELESS_LIB

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            return PhySatelliteRsvGetLastAngleOfArrival(node, phyNum);

            break;
        }
#endif // SATELLITE_LIB
#ifdef WIRELESS_LIB
        case PHY_ABSTRACT: {
            return PhyAbstractGetLastAngleOfArrival(node, phyNum);
            break;
        }
#endif
#ifdef MODE5_LIB
        case PHY_MODE5: {
            return Mode5::Phy::aoa(node, phyNum);
        } break;
#endif
#ifdef ADDON_BOEINGFCS
        case PHY_SRW_ABSTRACT: {
          return SRW::Nexus::PHY::GetLastAngleOfArrival(node, phyNum);
        } break;
#endif // FCS
        default: {
            ERROR_ReportError("Selected radio does not support this function\n");
        }
    }
    // Never reachable, but required to suppress a warning by some compilers.
    return 0.0;
}


void PHY_TerminateCurrentReceive(
    Node* node, int phyIndex, const BOOL terminateOnlyOnReceiveError,
    BOOL* receiveError, clocktype* endSignalTime)
{
    switch(node->phyData[phyIndex]->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11n:
        case PHY802_11b:
        case PHY802_11a: {
            Phy802_11TerminateCurrentReceive(
                node, phyIndex, terminateOnlyOnReceiveError, receiveError,
                endSignalTime);

            break;
        }

       case PHY_ABSTRACT:
        {
            PhyAbstractTerminateCurrentReceive(
                node, phyIndex, terminateOnlyOnReceiveError, receiveError,
                endSignalTime);
        }
        break;
#endif // WIRELESS_LIB
#ifdef MODE5_LIB
        case PHY_MODE5: {
            Mode5::Phy::terminate(node, phyIndex,
                                  terminateOnlyOnReceiveError, receiveError,
                                  endSignalTime);
        } break;
#endif
#ifdef ADVANCED_WIRELESS_LIB
       case PHY802_16:
        {
                PhyDot16TerminateCurrentReceive(node,
                                                phyIndex,
                                                terminateOnlyOnReceiveError,
                                                receiveError,
                                                endSignalTime);
                break;
        }
#endif // ADVANCED_WIRELESS_LIB
#ifdef SENSOR_NETWORKS_LIB
     case PHY802_15_4:
        {
            Phy802_15_4TerminateCurrentReceive(
                node, phyIndex, terminateOnlyOnReceiveError, receiveError,
                endSignalTime);

            break;
        }
#endif
#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            PhySatelliteRsvTerminateCurrentReceive(
                 node, phyIndex, terminateOnlyOnReceiveError, receiveError,
                 endSignalTime);

            break;
        }
#endif // SATELLITE_LIB

#ifdef ADDON_BOEINGFCS
        case PHY_SRW_ABSTRACT: {
          SRW::Nexus::PHY::TerminateCurrentReceive(node, phyIndex,
                                                terminateOnlyOnReceiveError,
                                                receiveError,
                                                endSignalTime);
        } break;
#endif // FCS
        default: {
            ERROR_ReportError("Selected radio does not support this function\n");
        }
    }//switch//
}


//
// FUNCTION    PHY_StartTransmittingDirectionally
// PURPOSE     Starts transmitting a packet using directional antenna.
//

void PHY_StartTransmittingSignalDirectionally(
    Node *node,
    int phyNum,
    Message *msg,
    BOOL useMacLayerSpecifiedDelay,
    clocktype delayUntilAirborne,
    double azimuthAngle)
{
#ifdef ADDON_DB
    HandleMacDBConnectivity(node,
        node->phyData[phyNum]->macInterfaceIndex, msg, MAC_SendToPhy);
#endif
    switch(node->phyData[phyNum]->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11n:
        case PHY802_11b:
        case PHY802_11a: {
            Phy802_11StartTransmittingSignalDirectionally(
                node, phyNum, msg,
                useMacLayerSpecifiedDelay,
                delayUntilAirborne,
                azimuthAngle);

            break;
        }
#endif // WIRELESS_LIB

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            PhySatelliteRsvStartTransmittingSignalDirectionally(
                                                             node, phyNum, msg,
                                                             useMacLayerSpecifiedDelay,
                                                             delayUntilAirborne,
                                                             azimuthAngle);

            break;
        }
#endif // SATELLITE_LIB
#ifdef WIRELESS_LIB
        case PHY_ABSTRACT: {
            PhyAbstractStartTransmittingSignalDirectionally(
                    node, phyNum, msg,
                    useMacLayerSpecifiedDelay,
                    delayUntilAirborne,
                    azimuthAngle);
            break;
        }
#endif
#ifdef MODE5_LIB
        case PHY_MODE5: {
            Mode5::Phy::directionalTxStart(node, phyNum, msg,
                useMacLayerSpecifiedDelay, delayUntilAirborne, azimuthAngle);
        } break;
#endif
#ifdef ADDON_BOEINGFCS
        case PHY_SRW_ABSTRACT: {
          SRW::Nexus::PHY::StartTransmittingSignalDirectionally(node, phyNum, msg,
                                                             useMacLayerSpecifiedDelay,
                                                             delayUntilAirborne,
                                                             azimuthAngle);
        } break;
#endif // FCS
        default: {
            ERROR_ReportError("Selected radio does not support this function\n");
        }
    }//switch//
}


//
// FUNCTION    PHY_LockAntennaDirection
// PURPOSE     Tells smart radios to lock the antenna direction
//             to the last received packet.
//
void PHY_LockAntennaDirection(Node* node, int phyNum) {
    switch(node->phyData[phyNum]->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11n:
        case PHY802_11b:
        case PHY802_11a: {
            Phy802_11LockAntennaDirection(node, phyNum);
            break;
        }
#endif // WIRELESS_LIB

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            PhySatelliteRsvLockAntennaDirection(node, phyNum);
            break;
        }
#endif // SATELLITE_LIB
#ifdef WIRELESS_LIB
        case PHY_ABSTRACT: {
            PhyAbstractLockAntennaDirection(node, phyNum);
            break;
        }
#endif
#ifdef MODE5_LIB
        case PHY_MODE5: {
            Mode5::Phy::lockDirection(node, phyNum);
        } break;
#endif
#ifdef ADDON_BOEINGFCS
        case PHY_SRW_ABSTRACT: {
          SRW::Nexus::PHY::LockAntennaDirection(node, phyNum);
        } break;
#endif // FCS
        default: {
            ERROR_ReportError("Selected radio does not support this function\n");
        }
    }//switch//
}


//
// FUNCTION    PHY_UnlockAntennaDirection
// PURPOSE     Tells smart radios to unlock the antenna direction
//             (go back to omnidirectional).
//
void PHY_UnlockAntennaDirection(Node* node, int phyNum) {
    switch(node->phyData[phyNum]->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11n:
        case PHY802_11b:
        case PHY802_11a: {
            Phy802_11UnlockAntennaDirection(node, phyNum);
            break;
        }
#endif // WIRELESS_LIB

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            PhySatelliteRsvUnlockAntennaDirection(node, phyNum);
            break;
        }
#endif // SATELLITE_LIB
#ifdef WIRELESS_LIB
        case PHY_ABSTRACT: {
            PhyAbstractUnlockAntennaDirection(node, phyNum);
            break;
        }
#endif
#ifdef MODE5_LIB
        case PHY_MODE5: {
            Mode5::Phy::unlockDirection(node, phyNum);
        } break;
#endif
#ifdef ADDON_BOEINGFCS
        case PHY_SRW_ABSTRACT: {
          SRW::Nexus::PHY::UnlockAntennaDirection(node, phyNum);
        } break;
#endif // FCS

        default: {
            ERROR_ReportError("Selected radio does not support this function\n");
        }
    }//switch//
}

BOOL PHY_MediumIsIdle(Node* node, int phyNum) {
    switch(node->phyData[phyNum]->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11n:
        case PHY802_11b:
        case PHY802_11a: {
            return Phy802_11MediumIsIdle(node, phyNum);
        }
        case PHY_ABSTRACT: {
            return PhyAbstractMediumIsIdle(node, phyNum);
        }
#endif // WIRELESS_LIB

        default: {
            ERROR_ReportError("Selected radio does not support this function\n");
        }
    }//switch//

    // Should not get here.
    return FALSE;
}

BOOL PHY_MediumIsIdleInDirection(Node* node, int phyNum, double azimuth) {
    switch(node->phyData[phyNum]->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11n:
        case PHY802_11b:
        case PHY802_11a: {
            return Phy802_11MediumIsIdleInDirection(node, phyNum, azimuth);
        }
#endif // WIRELESS_LIB

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            return PhySatelliteRsvMediumIsIdleInDirection(node, phyNum, azimuth);
        }
#endif // SATELLITE_LIB

#ifdef WIRELESS_LIB
        case PHY_ABSTRACT: {
            return PhyAbstractMediumIsIdleInDirection(node, phyNum, azimuth);
        }
#endif
#ifdef MODE5_LIB
        case PHY_MODE5: {
            return Mode5::Phy::isIdle(node, phyNum, azimuth);
        } break;
#endif
#ifdef ADDON_BOEINGFCS
        case PHY_SRW_ABSTRACT: {
          return SRW::Nexus::PHY::MediumIsIdleInDirection(node, phyNum, azimuth);
        } break;
#endif // FCS
        default: {
            ERROR_ReportError("Selected radio does not support this function\n");
        }
    }//switch//

    // Should not get here.
    return FALSE;
}

void PHY_SetSensingDirection(Node* node, int phyNum, double azimuth) {
    switch(node->phyData[phyNum]->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11n:
        case PHY802_11b:
        case PHY802_11a: {
            Phy802_11SetSensingDirection(node, phyNum, azimuth);
            break;
        }
#endif // WIRELESS_LIB

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            PhySatelliteRsvSetSensingDirection(node, phyNum, azimuth);
            break;
        }
#endif // SATELLITE_LIB
#ifdef WIRELESS_LIB
        case PHY_ABSTRACT: {
            PhyAbstractSetSensingDirection(node, phyNum, azimuth);
            break;
        }
#endif
#ifdef MODE5_LIB
        case PHY_MODE5: {
            Mode5::Phy::setDirection(node, phyNum, azimuth);
        } break;
#endif
#ifdef ADDON_BOEINGFCS
        case PHY_SRW_ABSTRACT: {
          SRW::Nexus::PHY::SetSensingDirection(node, phyNum, azimuth);
        } break;
#endif // FCS

        default: {
            ERROR_ReportError("Selected radio does not support this function\n");
        }
    }//switch//
}


#define LONGEST_DISTANCE 1000000.0  // 1000km
#define DELTA            0.001      // 1mm
#define PACKET_SIZE      (1024 * 8) // 1kbytes
#define RANGE_TO_RETURN_IN_CASE_OF_ERROR 350.0

double PHY_PropagationRange(Node* txnode,
                            Node* node,
                            int   txInterfaceIndex,
                            int   interfaceIndex,
                            int   channnelIndex,
                            BOOL  printAllDataRates) {
#ifdef WIRELESS_LIB
// if printAll is true, print out the range for all data rates because
// it's being called by radio_range.  Otherwise, figure out the lowest
// data rate and return the range for that.

    PropProfile*    propProfile;
    //int             radioNumber = interfaceIndex;
    PhyData*        thisRadio;
    PhyData*        tx;
    int             numIterations;
    int             index;

    PhyModel         phyModel = PHY_NONE;
    PhyRxModel       phyRxModel;
    PhyModel         txPhyModel = PHY_NONE;

#ifdef CELLULAR_LIB
    PhyDataGsm*      phyGsm;
    PhyDataGsm*      txPhyGsm;
#endif //CELLULAR_LIB

#if defined(WIRELESS_LIB)
    PhyCellularData* phyCellular;
#endif 

#ifdef ADVANCED_WIRELESS_LIB
    PhyDataDot16*    phy802_16 = NULL;
    PhyDataDot16*    txPhy802_16 = NULL;
#endif

    PhyData802_11*   phy802_11 = NULL;
    PhyData802_11*   txPhy802_11 = NULL;
    PhyDataAbstract* phyAbstract;
    PhyDataAbstract* txPhyAbstract;
#ifdef SENSOR_NETWORKS_LIB
    PhyData802_15_4* phy802_15_4;
    PhyData802_15_4* txPhy802_15_4;
#endif //SENSOR_NETWORKS_LIB

#ifdef ADDON_BOEINGFCS
    PhyDataSrwAbstract* phySrwAbstract;
    PhyDataSrwAbstract* txPhySrwAbstract;
#endif // FCS
    AntennaOmnidirectional* txOmniDirectional;
    AntennaOmnidirectional* omniDirectional;
    AntennaSwitchedBeam* txSwitchedBeam;
    AntennaSwitchedBeam* switchedBeam;
    AntennaSteerable* txSteerable;
    AntennaSteerable* steerable;
    AntennaPatterned* txPatterned;
    AntennaPatterned* patterned;

    double rxAntennaGain_dB = 0.0;
    double txAntennaGain_dB = 0.0;
    double rxSystemLoss_dB = 0.0;
    double txSystemLoss_dB = 0.0;
    float  rxAntennaHeight = 0.0;
    float  txAntennaHeight = 0.0;
    double distanceReachable = 0.0;
    double distanceNotReachable = LONGEST_DISTANCE;
    double distance = distanceNotReachable;
    double pathloss_dB = 0.0;
    double txPower_dBm = 0.0;
    double rxPower_dBm;
    double rxPower_mW;
    double noise_mW = 0.0;
    double rxThreshold_mW = 0.0;
    int     dataRateToUse = 0;

    propProfile = node->partitionData->propChannel[channnelIndex].profile;

    thisRadio = node->phyData[interfaceIndex];
    phyModel = node->phyData[interfaceIndex]->phyModel;

    tx = txnode->phyData[txInterfaceIndex];
    txPhyModel = txnode->phyData[txInterfaceIndex]->phyModel;

    if (!((txPhyModel == phyModel) &&
        ((node->phyData[interfaceIndex]->channelListenable
        [channnelIndex] == TRUE) &&
        (txnode->phyData[txInterfaceIndex]->channelListenable
        [channnelIndex] == TRUE))))
     {
         return 0.0;
     }

    switch (phyModel) {
        case PHY802_11a:
        case PHY802_11b:
            phy802_11 = (PhyData802_11*)thisRadio->phyVar;
            noise_mW =
                phy802_11->thisPhy->noise_mW_hz *
                phy802_11->channelBandwidth;
            txPhy802_11 = (PhyData802_11*)tx->phyVar;

            if (printAllDataRates) {
                txPower_dBm = (float)txPhy802_11->txDefaultPower_dBm[0];
                rxThreshold_mW = phy802_11->rxSensitivity_mW[0];
            }
            else {
                dataRateToUse = phy802_11->lowestDataRateType;
                if (phyModel == PHY802_11b && dataRateToUse > 1) {
                    noise_mW *= 11.0;
                }
                txPower_dBm = (float)txPhy802_11->txDefaultPower_dBm[
                dataRateToUse];
                rxThreshold_mW = phy802_11->rxSensitivity_mW[dataRateToUse];
            }
            phyRxModel = BER_BASED;
            break;

        case PHY802_11n:
            phy802_11 = (PhyData802_11*)thisRadio->phyVar;
            noise_mW =
                phy802_11->thisPhy->noise_mW_hz *
                phy802_11->channelBandwidth;
            txPhy802_11 = (PhyData802_11*)tx->phyVar;

            txPower_dBm = IN_DB(NON_DB(phy802_11->txPower_dBm)/
                            Phy802_11nGetNumAtnaElems(thisRadio));
            rxThreshold_mW = phy802_11->rxSensitivity_mW[0];
            phyRxModel = BER_BASED;

            break;

#ifdef CELLULAR_LIB
        case PHY_GSM:
            phyGsm = (PhyDataGsm*) thisRadio->phyVar;
            txPhyGsm = (PhyDataGsm*) tx->phyVar;
            txPower_dBm = (float)txPhyGsm->txPower_dBm;
            noise_mW =
                phyGsm->thisPhy->noise_mW_hz * PHY_GSM_CHANNEL_BANDWIDTH;
                //200kHz
            rxThreshold_mW = phyGsm->rxThreshold_mW;
            phyRxModel = BER_BASED;

            break;
#endif 

#if defined(WIRELESS_LIB) 
            case PHY_CELLULAR:
            {
                PhyCellularData *phyCellular =
                    (PhyCellularData*)thisRadio->phyVar;
                PhyCellularData *txPhyCellular =
                    (PhyCellularData*)tx->phyVar;

                CellularPhyProtocolType cpt = phyCellular->cellularPhyProtocolType;
 
                switch (cpt)
                {
                case Cellular_UMTS_Phy:
                {
#ifdef UMTS_LIB
                    PhyUmtsData* phyUmts = (PhyUmtsData* )phyCellular->phyUmtsData;
                    PhyUmtsData* txPhyUmts = (PhyUmtsData* )txPhyCellular->phyUmtsData;
                    txPower_dBm = (float)txPhyUmts->txPower_dBm;
                    rxThreshold_mW = phyUmts->rxSensitivity_mW[0];
                    noise_mW = phyUmts->noisePower_mW;
                    phyRxModel = BER_BASED;
#endif
                }
                break;
                case Cellular_MUOS_Phy:
                {
#ifdef MUOS_LIB
                    PhyMuosData* phyMuos = (PhyMuosData* )phyCellular->phyMuosData;
                    PhyMuosData* txPhyMuos = (PhyMuosData* )txPhyCellular->phyMuosData;
                    txPower_dBm = (float)txPhyMuos->txPower_dBm;
                    rxThreshold_mW = phyMuos->rxSensitivity_mW[0];
                    noise_mW = phyMuos->noisePower_mW;
                    phyRxModel = BER_BASED;
#endif
                }
                break;
                }
            }
            break;
#endif

        case PHY_ABSTRACT:
            phyAbstract = (PhyDataAbstract*)thisRadio->phyVar;
            txPhyAbstract = (PhyDataAbstract*)tx->phyVar;
            txPower_dBm = (float)txPhyAbstract->txPower_dBm;
#ifdef ADDON_BOEINGFCS
            noise_mW =
                phyAbstract->thisPhy->noise_mW_hz *
                phyAbstract->bandwidth;
#else
            noise_mW =
                phyAbstract->thisPhy->noise_mW_hz *
                phyAbstract->dataRate;
#endif
            rxThreshold_mW = phyAbstract->rxThreshold_mW;
            phyRxModel = SNR_THRESHOLD_BASED;

            break;
#ifdef MODE5_LIB
        case PHY_MODE5: {
            return 0.0;
        } break;
#endif
#ifdef SENSOR_NETWORKS_LIB
        case PHY802_15_4:
            phy802_15_4 = (PhyData802_15_4*)thisRadio->phyVar;
            txPhy802_15_4 = (PhyData802_15_4*)tx->phyVar;
            txPower_dBm = (float)txPhy802_15_4->txPower_dBm;
            noise_mW =
                phy802_15_4->noisePower_mW;
            rxThreshold_mW = phy802_15_4->rxSensitivity_mW;
            phyRxModel = SNR_THRESHOLD_BASED;

            break;
#endif

#ifdef ADVANCED_WIRELESS_LIB
        case PHY802_16:
            phy802_16 = (PhyDataDot16*)thisRadio->phyVar;
            txPhy802_16 = (PhyDataDot16*)tx->phyVar;
            noise_mW =
                phy802_16->thisPhy->noise_mW_hz *
                phy802_16->channelBandwidth;

            dataRateToUse = PHY802_16_QPSK_R_1_2;
            txPower_dBm = (float)txPhy802_16->txPower_dBm;
            rxThreshold_mW = phy802_16->rxSensitivity_mW[dataRateToUse];
            phyRxModel = BER_BASED;

            break;
#endif // ADVANCED_WIRELESS_LIB

#ifdef LTE_LIB
        case PHY_LTE:
            // TODO : Do not support for Phase1
            return RANGE_TO_RETURN_IN_CASE_OF_ERROR;
#endif
        default:
            if (printAllDataRates) {
                ERROR_ReportError("Unsupported Radio type.");
            }
            else {
                ERROR_ReportWarning("Cannot calculate the radio range");
                return RANGE_TO_RETURN_IN_CASE_OF_ERROR;
            }
    }

    switch (thisRadio->antennaData->antennaModelType)
    {
    case ANTENNA_OMNIDIRECTIONAL:
        {
            omniDirectional =
                (AntennaOmnidirectional*)thisRadio->antennaData->antennaVar;
            rxAntennaGain_dB = omniDirectional->antennaGain_dB;
            rxAntennaHeight = omniDirectional->antennaHeight;
            break;
        }
    case ANTENNA_SWITCHED_BEAM:
        {
            switchedBeam = (AntennaSwitchedBeam*)thisRadio->antennaData->
                antennaVar;
            rxAntennaGain_dB = switchedBeam->antennaGain_dB;
            rxAntennaHeight = switchedBeam->antennaHeight;
            break;
        }
    case ANTENNA_STEERABLE:
        {
            steerable =
                (AntennaSteerable*)thisRadio->antennaData->antennaVar;
            rxAntennaGain_dB = steerable->antennaGain_dB;
            rxAntennaHeight = steerable->antennaHeight;
            break;
        }
    case ANTENNA_PATTERNED:
        {
            patterned =
                (AntennaPatterned*)thisRadio->antennaData->antennaVar;
            rxAntennaGain_dB = patterned->antennaGain_dB;
            rxAntennaHeight = patterned->antennaHeight;
            break;
        }
    default:
        {
            char err[MAX_STRING_LENGTH];
            sprintf(err, "Unknown ANTENNA-MODEL-TYPE %d.\n",
                thisRadio->antennaData->antennaModelType);
            ERROR_ReportError(err);
            break;
        }

    }

    switch (tx->antennaData->antennaModelType)
     {
     case ANTENNA_OMNIDIRECTIONAL:
         {
             txOmniDirectional =
                 (AntennaOmnidirectional*)tx->antennaData->antennaVar;
             txAntennaGain_dB = txOmniDirectional->antennaGain_dB;
             txAntennaHeight = txOmniDirectional->antennaHeight;

             break;
         }
     case ANTENNA_SWITCHED_BEAM:
         {
             txSwitchedBeam = (AntennaSwitchedBeam*)tx->antennaData->
                 antennaVar;
             txAntennaGain_dB = txSwitchedBeam->antennaGain_dB;
             txAntennaHeight = txSwitchedBeam->antennaHeight;
             break;
         }
     case ANTENNA_STEERABLE:
         {
             txSteerable =
                 (AntennaSteerable*)tx->antennaData->antennaVar;
             txAntennaGain_dB = txSteerable->antennaGain_dB;
             txAntennaHeight = txSteerable->antennaHeight;
             break;
         }
     case ANTENNA_PATTERNED:
         {
             txPatterned =
                 (AntennaPatterned*)tx->antennaData->antennaVar;
             txAntennaGain_dB = txPatterned->antennaGain_dB;
             txAntennaHeight = txPatterned->antennaHeight;
             break;
         }
     default:
         {
             char err[MAX_STRING_LENGTH];
             sprintf(err, "Unknown ANTENNA-MODEL-TYPE %d.\n",
                 thisRadio->antennaData->antennaModelType);
             ERROR_ReportError(err);
             break;
         }

    }

    txSystemLoss_dB = tx->systemLoss_dB;
    rxSystemLoss_dB = thisRadio->systemLoss_dB;

    // Calculate radio range for all data rates
    numIterations = 1;
    if (printAllDataRates) {
        if (phyModel == PHY802_11a)
        {
            numIterations = PHY802_11a_NUM_DATA_RATES;
        } else if (phyModel == PHY802_11b) {
            numIterations = PHY802_11b_NUM_DATA_RATES;
        }
        else if (phyModel == PHY802_11n) {
            numIterations = 8;
        }
    }

#ifdef ADVANCED_WIRELESS_LIB
    if (printAllDataRates) {
        if (phyModel == PHY802_16 ) {
            numIterations = PHY802_16_NUM_DATA_RATES - 1;
        }
    }
#endif

    Coordinates fromPosition;
    Coordinates toPosition;

    MOBILITY_ReturnCoordinates(txnode, &fromPosition);
    MOBILITY_ReturnCoordinates(node, &toPosition);

    double txPlatformHeight = fromPosition.common.c3 + txAntennaHeight;
    double rxPlatformHeight = toPosition.common.c3 + rxAntennaHeight;

    for (index = dataRateToUse; index < (dataRateToUse + numIterations); index++)
    {
        while (distanceNotReachable - distanceReachable > DELTA) {
            BOOL reachable;

            switch (propProfile->pathlossModel) {
                case FREE_SPACE: {
                    pathloss_dB = PROP_PathlossFreeSpace(distance,
                                                         propProfile->wavelength);
                    break;
                }
                case TWO_RAY: {
                    pathloss_dB = PROP_PathlossTwoRay(distance,
                                                    propProfile->wavelength,
                                                    (float)txPlatformHeight,
                                                    (float)rxPlatformHeight);

                    break;
                }
#ifdef URBAN_LIB
                case COST231_HATA: {

                    pathloss_dB = PathlossCOST231Hata(distance,
                                                      propProfile->wavelength,
                                                      txAntennaHeight,
                                                      rxAntennaHeight,
                                                      propProfile);
                    break;

                }
#endif // URBAN_LIB
                default: {
                    if (printAllDataRates) {
                        ERROR_ReportError("Cannot estimate the range with this pathloss model.");
                    }
                    else { // use TWO_RAY as a bad estimate.
                        pathloss_dB =
                            PROP_PathlossTwoRay(distance,
                                                propProfile->wavelength,
                                                (float)txPlatformHeight,
                                                (float)rxPlatformHeight);
                    }
                }
            }

            rxPower_dBm = txPower_dBm + txAntennaGain_dB - txSystemLoss_dB -
                          pathloss_dB - propProfile->shadowingMean_dB +
                          rxAntennaGain_dB - rxSystemLoss_dB;
            rxPower_mW = NON_DB(rxPower_dBm);

#ifdef ADVANCED_WIRELESS_LIB

            if (phyModel == PHY802_16) {
                rxPower_mW *= phy802_16->PowerLossDueToCPTime;
            }
#endif
            if (phyRxModel == BER_BASED) {
                double BER, PER;
                double snr = rxPower_mW / noise_mW;

                if (rxPower_mW < rxThreshold_mW) {
                    reachable = FALSE;
                }
                else {
                    BER = PHY_BER(thisRadio, index, snr);

                    PER = 1.0 - pow((1.0 - BER), (double)PACKET_SIZE);

                    if (PER <= 0.1) {
                        reachable = TRUE;
                    }
                    else {
                        reachable = FALSE;
                    }
                }
            }
            else {
                // SNR_THRESHOLD_BASED

                if (rxPower_mW >= rxThreshold_mW) {
                    reachable = TRUE;
                }
                else {
                    reachable = FALSE;
                }
            }

            if (reachable == TRUE) {
                distanceReachable = distance;
                distance += (distanceNotReachable - distanceReachable) / 2.0;
            }
            else {
                assert(reachable == FALSE);

                distanceNotReachable = distance;
                distance -= (distanceNotReachable - distanceReachable) / 2.0;
            }
        } // end of while //

        if (phyRxModel == SNR_THRESHOLD_BASED) {
            if (rxThreshold_mW <
                noise_mW * thisRadio->phyRxSnrThreshold)
            {
                return(0);
            }
        }



        if (phyModel == PHY802_11a || phyModel == PHY802_11b)
        {
            int modelType = 'a';

            if (phyModel == PHY802_11b) {
                modelType = 'b';
            }

            // printf("%.15f %.15f %d %.15f %.3f\n",
            //        txPower_dBm, rxThreshold_mW, index,
            //        noise_mW, distanceReachable);

            if (printAllDataRates) {
                printf(" Radio range: %8.3fm, for 802.11%c data rate %4.1f Mbps\n",
                       distanceReachable, modelType,
                       ((float)phy802_11->dataRate[index] / 1000000.0));
            }
            else {
                return distanceReachable;
            }
            // Initialize for the next data rate
            rxThreshold_mW = phy802_11->rxSensitivity_mW[index + 1];
            txPower_dBm = txPhy802_11->txDefaultPower_dBm[index + 1];
            distanceReachable = 0.0;
            distanceNotReachable = LONGEST_DISTANCE;
            distance = distanceNotReachable;
            if (phyModel == PHY802_11b && index == 1) {
                noise_mW *= 11.0;
            }
        }
        else if (phyModel == PHY802_11n)
        {
            if (printAllDataRates) {
                printf(" Radio range: %8.3fm, for 802.11n MCS index %d \n",
                       distanceReachable, index);
            }
            else {
                return distanceReachable;
            }
            // Initialize for the next data rate
            rxThreshold_mW = phy802_11->rxSensitivity_mW[index + 1];
            distanceReachable = 0.0;
            distanceNotReachable = LONGEST_DISTANCE;
            distance = distanceNotReachable;
#ifdef ADVANCED_WIRELESS_LIB
        }
        else if (phyModel == PHY802_16)
        {
            if (printAllDataRates) {
                printf(" Radio range: %8.3fm, for 802.16 data rate %5.2f Mbps\n",
                distanceReachable,
                ((float)PhyDot16GetDataRate(thisRadio, index)
                 / 1000000.0));
            }
            else {
                return distanceReachable;
            }
            // Initialize for the next data rate
            rxThreshold_mW = phy802_16->rxSensitivity_mW[index + 1];
            distanceReachable = 0.0;
            distanceNotReachable = LONGEST_DISTANCE;
            distance = distanceNotReachable;
#endif
        } else {
            if (printAllDataRates) {
                printf(" Radio range: %.3fm\n", distanceReachable);
            }
            else {
                return distanceReachable;
            }
        }
    } // end of for //

    return distanceReachable;
#else // WIRELESS_LIB
    return 0.0;
#endif // WIRELESS_LIB
}

void
PHY_BerTablesPrepare (PhyBerTable * berTables, const int tableCount)
{
    int             tableIndex;
    PhyBerTable *   berTable;

    bool            isFixedInterval;
    double          interval;
    int             entryCount;
    int             entryIndex;
    double          prevSnr;

    double          epsilon;


    for (tableIndex = 0; tableIndex < tableCount; tableIndex++)
    {
        berTable = &(berTables [tableIndex]);
        berTable->isFixedInterval = false;
        entryCount = berTable->numEntries;
        if (entryCount < 3)
            continue;

        // This epislon (how accurate for floating point math errors)
        // is fairly coarse. This happens because our snr
        // tables are being filled from auto-init source code (not actualy math),
        // and the numeric strings in our source code
        // only have about 9 digits worth of precision.
        epsilon = 1.0e-13;
        interval = berTable->entries [1].snr - berTable->entries [0].snr;
        prevSnr = berTable->entries [1].snr;
        isFixedInterval = true;
        for (entryIndex = 2; entryIndex < entryCount; entryIndex++)
        {
            if (fabs ((berTable->entries [entryIndex].snr - prevSnr) - interval)
                > epsilon)
            {
                isFixedInterval = false;
                break;
            }
            prevSnr = berTable->entries [entryIndex].snr;
        }
        if (isFixedInterval)
        {
            berTable->isFixedInterval = true;
            berTable->interval = interval;
            berTable->snrStart = berTable->entries [0].snr;
            berTable->snrEnd = berTable->entries [entryCount - 1].snr;
        }
    }
}

PhyBerTable *
PHY_BerTablesAlloc (const int tableCount)
{
    PhyBerTable *   newBerTables;

    newBerTables = (PhyBerTable*) MEM_malloc(tableCount * sizeof(PhyBerTable));
    memset(newBerTables, 0, tableCount * sizeof(PhyBerTable));

    return newBerTables;
}

void
PHY_PerTablesPrepare (PhyPerTable * perTables, const int tableCount)
{
    int             tableIndex;
    PhyPerTable *   perTable;
    bool            isFixedInterval;
    double          interval;
    int             entryCount;
    int             entryIndex;
    double          prevSnr;
    double          epsilon;


    for (tableIndex = 0; tableIndex < tableCount; tableIndex++)
    {
        perTable = &(perTables [tableIndex]);
        perTable->isFixedInterval = false;
        entryCount = perTable->numEntries;
        if (entryCount < 3)
        {
            continue;
        }

        // This epislon (how accurate for floating point math errors)
        // is fairly coarse. This happens because our snr
        // tables are being filled from auto-init source code (not actualy math),
        // and the numeric strings in our source code
        // only have about 9 digits worth of precision.
        epsilon = 1.0e-13;
        interval = perTable->entries [1].snr - perTable->entries [0].snr;
        prevSnr = perTable->entries [1].snr;
        isFixedInterval = true;
        for (entryIndex = 2; entryIndex < entryCount; entryIndex++)
        {
            if (fabs ((perTable->entries [entryIndex].snr - prevSnr) - interval)
                > epsilon)
            {
                isFixedInterval = false;
                break;
            }
            prevSnr = perTable->entries [entryIndex].snr;
        }
        if (isFixedInterval)
        {
            perTable->isFixedInterval = true;
            perTable->interval = interval;
            perTable->snrStart = perTable->entries [0].snr;
            perTable->snrEnd = perTable->entries [entryCount - 1].snr;
        }
    }
}

PhyPerTable *
PHY_PerTablesAlloc (const int tableCount)
{
    PhyPerTable* newPerTables;

    newPerTables = (PhyPerTable*) MEM_malloc(tableCount * sizeof(PhyPerTable));
    memset(newPerTables, 0, tableCount * sizeof(PhyPerTable));

    return newPerTables;
}



//SER
void
PHY_SerTablesPrepare (PhySerTable * serTables, const int tableCount)
{
    int             tableIndex;
    PhySerTable *   serTable;
    bool            isFixedInterval;
    double          interval;
    int             entryCount;
    int             entryIndex;
    double          prevSnr;
    double          epsilon;


    for (tableIndex = 0; tableIndex < tableCount; tableIndex++)
    {
        serTable = &(serTables [tableIndex]);
        serTable->isFixedInterval = false;
        entryCount = serTable->numEntries;
        if (entryCount < 3)
        {
            continue;
        }

        // This epislon (how accurate for floating point math errors)
        // is fairly coarse. This happens because our snr
        // tables are being filled from auto-init source code (not actualy math),
        // and the numeric strings in our source code
        // only have about 9 digits worth of precision.
        epsilon = 1.0e-13;
        interval = serTable->entries [1].snr - serTable->entries [0].snr;
        prevSnr = serTable->entries [1].snr;
        isFixedInterval = true;
        for (entryIndex = 2; entryIndex < entryCount; entryIndex++)
        {
            if (fabs ((serTable->entries [entryIndex].snr - prevSnr) - interval)
                > epsilon)
            {
                isFixedInterval = false;
                break;
            }
            prevSnr = serTable->entries [entryIndex].snr;
        }
        if (isFixedInterval)
        {
            serTable->isFixedInterval = true;
            serTable->interval = interval;
            serTable->snrStart = serTable->entries [0].snr;
            serTable->snrEnd = serTable->entries [entryCount - 1].snr;
        }
    }
}

PhySerTable *
PHY_SerTablesAlloc (const int tableCount)
{
    PhySerTable* newSerTables;

    newSerTables = (PhySerTable*) MEM_malloc(tableCount * sizeof(PhySerTable));
    memset(newSerTables, 0, tableCount * sizeof(PhySerTable));

    return newSerTables;
}



// to get TxPower of a particular phy model.
double PHY_GetTxPower(Node* node, int phyIndex)
{
        switch(node->phyData[phyIndex]->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11n:
        case PHY802_11b:
        case PHY802_11a: {
            return Phy802_11GetTxPower(node, phyIndex);
            break;
        }
        case PHY_ABSTRACT: {
            return PhyAbstractGetTxPower(node, phyIndex);
            break;
        }
#endif // WIRELESS_LIB
#ifdef MODE5_LIB
        case PHY_MODE5: {
            return Mode5::Phy::getTransmitPower(node, phyIndex);
        } break;
#endif
#ifdef ADDON_BOEINGFCS
        case PHY_SRW_ABSTRACT: {
          return SRW::Nexus::PHY::GetTxPower(node, phyIndex);
        }
#endif
#ifdef SENSOR_NETWORKS_LIB
       case PHY802_15_4: {
        }
#endif //SENSOR_NETWORKS_LIB
#ifdef CELLULAR_LIB
        case PHY_GSM: {
        }
#endif // CELLULAR_LIB

#ifdef MILITARY_RADIOS_LIB
        case PHY_FCSC_PROTOTYPE: {
        }
#endif /* MILITARY_RADIOS_LIB */

#ifdef ADDON_LINK16
        case PHY_SPAWAR_LINK16: {
        }
#endif // ADDON_LINK16

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
        }
#endif // SATELLITE_LIB

#ifdef ADVANCED_WIRELESS_LIB
        case PHY802_16: {
        }
#endif // ADVANCED_WIRELESS_LIB

        default:
            ERROR_ReportError("Unknown or disabled PHY model\n");
    }

    return FALSE;
}

BOOL PHY_ProcessSignal(Node* node,
                       int phyIndex,
                       PropRxInfo* propRxInfo,
                       double rxPower_dBm)
{

    switch(node->phyData[phyIndex]->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11n:
        case PHY802_11b:
        case PHY802_11a: {
            return Phy802_11ProcessSignal(node,
                                          phyIndex,
                                          propRxInfo,
                                          rxPower_dBm);
            break;
        }
        case PHY_ABSTRACT: {
            return PhyAbstractProcessSignal(node,
                                            phyIndex,
                                            propRxInfo,
                                            rxPower_dBm);
            break;
        }
#endif // WIRELESS_LIB
#ifdef MODE5_LIB
        case PHY_MODE5: {
          Mode5::Phy::processSignal(node, phyIndex, propRxInfo, rxPower_dBm);
        } break;
#endif
#ifdef ADDON_BOEINGFCS
        case PHY_SRW_ABSTRACT: {
          SRW::Nexus::PHY::ProcessSignal(node, phyIndex, propRxInfo, rxPower_dBm);
        } break;
#endif // FCS
#ifdef SENSOR_NETWORKS_LIB
       case PHY802_15_4: {
        }
#endif //SENSOR_NETWORKS_LIB
#ifdef CELLULAR_LIB
        case PHY_GSM: {
        }
#endif // CELLULAR_LIB

#ifdef MILITARY_RADIOS_LIB
        case PHY_FCSC_PROTOTYPE: {
        }
#endif /* MILITARY_RADIOS_LIB */

#ifdef ADDON_LINK16
        case PHY_SPAWAR_LINK16: {
        }
#endif // ADDON_LINK16

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
        }
#endif // SATELLITE_LIB

#ifdef ADVANCED_WIRELESS_LIB
        case PHY802_16: {
        }
#endif // ADVANCED_WIRELESS_LIB

        default:
            ERROR_ReportError("Unknown or disabled PHY model\n");
    }

    return FALSE;
}

// /**
// FUNCTION :: PHY_NotificationOfPacketDrop
// PURPOSE :: To Notify the StatsDB module and other modules of the packet dropping event.
// PARAMETERS ::
// + node : Node*    : the node of interest.
// + phyIndex  : int : the PHY index.
// + channelIndex : int : the channelIndex
// + msg : const Message* : The dropped message
// + dropType : const string& : the reason for the drop
// + rxPower_mW : double : receving power of the signal
// + interferencePower_mW : double : interference power of the signal
// + passloss_dB : double : pathloss value of the signal
// RETURN           :: void :
// **/
void PHY_NotificationOfPacketDrop(Node* node,
                                  int phyIndex,
                                  int channelIndex,
                                  const Message* msg,
                                  const string& dropType,
                                  double rxPower_mW,
                                  double interferencePower_mW,
                                  double pathloss_dB)
{
#ifdef ADDON_DB
    StatsDb* db = node->partitionData->statsDb;
    if (db != NULL &&
        db->statsEventsTable != NULL &&
        db->statsEventsTable->createPhyEventsTable)
    {
        BOOL recordDrop = FALSE;

        StatsDBMessageNextPrevHop *nextPrevHop = (StatsDBMessageNextPrevHop*)
            MESSAGE_ReturnInfo(msg, INFO_TYPE_MessageNextPrevHop);
        NodeId rcvdId;
        NodeAddress prevHopAddr;

        if (nextPrevHop != NULL)
        {
            // use ANY_ADDRESS
            rcvdId  = nextPrevHop->nextHopId;
            prevHopAddr = nextPrevHop->prevHopAddr;
        }
        else
        {
            rcvdId = ANY_DEST;
            prevHopAddr = ANY_DEST;
        }

        if (rcvdId == node->nodeId)
        {
            recordDrop = TRUE;
        }
        else if (rcvdId == ANY_DEST)
        {
            unsigned int i = 0;

            for (i = 0; i < msg->infoArray.size(); i ++)
            {
                int incomingInterface = -1;
                if (phyIndex >= 0)
                {
                    incomingInterface = node->phyData[phyIndex]->
                                                          macInterfaceIndex;
                }

                MessageInfoHeader* hdrPtr =
                                   (MessageInfoHeader*)&(msg->infoArray[i]);
                if (hdrPtr->infoType == INFO_TYPE_MessageAddrInfo)
                {
                    StatsDBMessageAddrInfo* msgAddrInfo =
                                       (StatsDBMessageAddrInfo*)hdrPtr->info;

                    if (NetworkIpIsMyIP(node, msgAddrInfo->dstAddr)
                        || NetworkIpIsMyMulticastPacket(node,
                                                        msgAddrInfo->srcAddr,
                                                        msgAddrInfo->dstAddr,
                                                        prevHopAddr,
                                                        incomingInterface))
                    {
                        recordDrop = TRUE;

                        // just need to insert once, so exit the for loop
                        break;
                    }
                }
            }
        }

        if (recordDrop)
        {
            StatsDBMappingParam* mapParamInfo = (StatsDBMappingParam*)
                          MESSAGE_ReturnInfo(msg, INFO_TYPE_StatsDbMapping);

            if (mapParamInfo != NULL)
            {
                Int32 msgSize = 0;
                if (!msg->isPacked)
                {
                    msgSize = MESSAGE_ReturnPacketSize(msg);
                }
                else
                {
                    msgSize = MESSAGE_ReturnActualPacketSize(msg);
                }
                StatsDBPhyEventParam phyParam(node->nodeId,
                                              mapParamInfo->msgId,
                                              phyIndex,
                                              msgSize,
                                              "PhyDrop");
                phyParam.SetMessageFailureType(dropType);
                phyParam.SetChannelIndex(channelIndex);

                if (rxPower_mW > 0)
                {
                    phyParam.SetSignalPower(IN_DB(rxPower_mW));
                }
                else
                {
                    phyParam.SetSignalPower(0.0);
                }

                if (interferencePower_mW > 0)
                {
                    phyParam.SetInterference(
                        IN_DB(interferencePower_mW));
                }
                else
                {
                    phyParam.SetInterference(0.0);
                }

                if (pathloss_dB > 0)
                {
                    phyParam.SetPathLoss(pathloss_dB);
                }
                STATSDB_HandlePhyEventsTableInsert(node, phyParam);
            }
        }
    }
#endif // ADDON_DB
}

// /**
// FUNCTION :: PHY_NotificationOfSignalReceived
// PURPOSE :: To Notify the StatsDB module and other modules of the signal received event .
// PARAMETERS ::
// + node : Node*    : the node of interest.
// + phyIndex  : int : the PHY index.
// + channelIndex : int : the channelIndex
// + msg : const Message* : The dropped message
// + rxPower_mW : double : receving power of the signal
// + interferencePower_mW : double : interference power of the signal
// + passloss_dB : double : pathloss value of the signal
// RETURN           :: void :
// **/
void PHY_NotificationOfSignalReceived(Node* node,
                                      int phyIndex,
                                      int channelIndex,
                                      const Message* msg,
                                      double rxPower_mW,
                                      double interferencePower_mW,
                                      double pathloss_dB,
                                      int controlSize)
{
#ifdef ADDON_DB
    StatsDb* db = node->partitionData->statsDb;
    NodeId *nextHop = (NodeId*)
                       MESSAGE_ReturnInfo(msg,
                           INFO_TYPE_MessageNextPrevHop);
    NodeId rcvdId;
    if (nextHop != NULL)
    {
        rcvdId  = *nextHop;
    }
    else
    {
        // use ANY_ADDRESS
        rcvdId = ANY_DEST;
    }

    if (db != NULL && db->statsEventsTable->createPhyEventsTable &&
       (rcvdId == node->nodeId || rcvdId == ANY_DEST))
    {
        StatsDBMappingParam *mapParamInfo = (StatsDBMappingParam*)
                MESSAGE_ReturnInfo(msg, INFO_TYPE_StatsDbMapping);
        ERROR_Assert(mapParamInfo ,
                "Error in StatsDB handling ReceiveSignal!");
        int msgSize = 0;
        if (!msg->isPacked)
        {
            msgSize = MESSAGE_ReturnPacketSize(msg);
        }
        else
        {
            msgSize = MESSAGE_ReturnActualPacketSize(msg);
        }
        StatsDBPhyEventParam phyParam(node->nodeId,
                                     (std::string) mapParamInfo->msgId,
                                      phyIndex,
                                      msgSize,
                                      "PhyReceiveSignal");
        phyParam.SetChannelIndex(channelIndex);

        phyParam.SetPathLoss(pathloss_dB);
        if (rxPower_mW)
        {
            phyParam.SetSignalPower(IN_DB(rxPower_mW)) ;
        }
        else
        {
            phyParam.SetSignalPower(0) ;
        }
        if (interferencePower_mW)
        {
                phyParam.SetInterference(IN_DB(interferencePower_mW));
        }
        else
        {
            phyParam.SetInterference(0.0);
        }
        phyParam.SetControlSize(controlSize) ;
        STATSDB_HandlePhyEventsTableInsert(node,
                                           phyParam);
    }
#endif
}

// /**
// FUNCTION :: PHY_GetBandwidth
// LAYER    :: Physical Layer.
// PURPOSE  :: To get the bandwidth for the given PHY model.
//
// PARAMETERS ::
// + node      : Node*    : The node of interest.
// + phyIndex  : int      : The PHY index.
// RETURN     :: double   : The bandwidth
// **/

double PHY_GetBandwidth(
            Node *node,
            int phyIndex)
{
    PhyData* thisRadio;
    PhyModel phyModel = PHY_NONE;
    double bandwidth = PHY_NEGATIVE_BANDWIDTH;

#ifdef ADVANCED_WIRELESS_LIB
    PhyDataDot16*    phy802_16 = NULL;
#endif

#ifdef WIRELESS_LIB
    PhyData802_11*   phy802_11 = NULL;
    PhyDataAbstract* phyAbstract;
#endif

#ifdef SENSOR_NETWORKS_LIB
    PhyData802_15_4* phy802_15_4;
#endif //SENSOR_NETWORKS_LIB

#ifdef MILITARY_RADIOS_LIB
    PhyDataFcsc*     phyFcsc;
#endif // MILITARY_RADIOS_LIB

#ifdef SATELLITE_LIB
    PhySatelliteRsvState *phySatelliteRsv;
#endif

#ifdef NETSEC_LIB
#ifdef PHY_SYNC
    PhyJammingData* phyJamming;
#endif // PHY_SYNC
#endif // NETSEC_LIB

#ifdef LTE_LIB
    PhyDataLte* phylte;
#endif

#ifdef ADDON_BOEINGFCS
    PhyDataSrwAbstract* phySrw;
#endif

    thisRadio = node->phyData[phyIndex];
    phyModel = thisRadio->phyModel;

    switch (phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11a:
        case PHY802_11b:
        case PHY802_11n:
            phy802_11 = (PhyData802_11*)thisRadio->phyVar;
            bandwidth = phy802_11->channelBandwidth;

            break;
#endif

#ifdef CELLULAR_LIB
        case PHY_GSM:
        {
            PhyDataGsm* phyGsm = (PhyDataGsm*) thisRadio->phyVar;
            bandwidth = PHY_GSM_CHANNEL_BANDWIDTH;
        }
        break;
#endif 

#ifdef WIRELESS_LIB
        case PHY_CELLULAR:
        {
            PhyCellularData* phyCellular = (PhyCellularData*)thisRadio->phyVar;
            CellularPhyProtocolType cpt = phyCellular->cellularPhyProtocolType;

            switch (cpt)
            {
#ifdef UMTS_LIB
            case Cellular_UMTS_Phy:
            {
                PhyUmtsData* phyUmts = (PhyUmtsData*)phyCellular->phyUmtsData;
                bandwidth = phyUmts->channelBandwidth;
            }
            break;
#endif //UMTS_LIB
#ifdef MUOS_LIB
            case Cellular_MUOS_Phy:
            {
                PhyMuosData* phyMuos = (PhyMuosData*)phyCellular->phyMuosData;
                bandwidth = phyMuos->channelBandwidth;
            }
            break;
#endif //MUOS_LIB
            }
        }
        break;

        case PHY_ABSTRACT:
            phyAbstract = (PhyDataAbstract*)thisRadio->phyVar;
            bandwidth = phyAbstract->bandwidth;

            break;
#endif //WIRELESS_LIB

#ifdef SENSOR_NETWORKS_LIB
        case PHY802_15_4:
            phy802_15_4 = (PhyData802_15_4*)thisRadio->phyVar;
            bandwidth =  PHY802_15_4_DEFAULT_CHANNEL_BANDWIDTH;

            break;
#endif

#ifdef LTE_LIB
        case PHY_LTE:
            phylte = (PhyDataLte*)thisRadio->phyVar;
            bandwidth = phylte->channelBandwidth;

            break;
#endif

#ifdef ADVANCED_WIRELESS_LIB
        case PHY802_16:
            phy802_16 = (PhyDataDot16*)thisRadio->phyVar;
            bandwidth = phy802_16->channelBandwidth;

            break;
#endif // ADVANCED_WIRELESS_LIB

#ifdef MILITARY_RADIOS_LIB
       case PHY_FCSC_PROTOTYPE:
            phyFcsc =(PhyDataFcsc *)thisRadio->phyVar;
            bandwidth = phyFcsc->dataRate;

            break;
#endif // MILITARY_RADIOS_LIB

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV:
            phySatelliteRsv =
                (PhySatelliteRsvState*)thisRadio->phyVar;
            bandwidth = phySatelliteRsv->channelBandwidth;

            break;
#endif

#ifdef NETSEC_LIB
#ifdef PHY_SYNC
        case PHY_JAMMING:
            phyJamming = (PhyJammingData*)thisRadio->phyVar;
            bandwidth = phyJamming->dataRate;

            break;
#endif // PHY_SYNC
#endif // NETSEC_LIB

#ifdef ADDON_BOEINGFCS
        case PHY_SRW_ABSTRACT:
            phySrw = (PhyDataSrwAbstract*)thisRadio->phyVar;
            bandwidth = phySrw->bandwidth;
            break;
#endif

        default: {
            ERROR_ReportError("Unknown or disabled PHY model\n");
        }

    }

    return bandwidth;
}


// /**
// FUNCTION :: PHY_GetFrequency
// LAYER    :: Physical Layer.
// PURPOSE  :: To get the frequency for the given signal.
//
// PARAMETERS ::
// + node         : Node*    : The node of interest.
// + channelIndex : int      : Index of the propagation channel
// RETURN        :: double   : The frequency
// **/

double PHY_GetFrequency(
            Node *node,
            int channelIndex)
{
    PropChannel* propChannel = node->propChannel;
    PropProfile* propProfile;
    double frequency;

    ERROR_Assert(propChannel != NULL,
                     "Propagation channel not found");

    ERROR_Assert(channelIndex >= 0 &&
        channelIndex < node->partitionData->numChannels,
        "Unknown channel index while accessing propagation"
        " profile");

    propProfile = propChannel[channelIndex].profile;

    ERROR_Assert(propProfile != NULL,
                     "Propagation profile not found");

    frequency = propProfile->frequency;
    return frequency;

}

// /**
// FUNCTION :: PHY_GetPhyModel
// LAYER :: Physical Layer.
// PURPOSE :: Get the PhyModel for the node.
//
// PARAMETERS ::
// + node      : Node*    : The node of interest.
// + phyIndex  : int      : The PHY index.
// RETURN     :: PhyModel : The PhyModel
// **/

PhyModel  PHY_GetPhyModel(
                Node *node,
                int phyIndex)
{
    PhyData* thisPhy;
    PhyModel phyModel = PHY_NONE;

    ERROR_Assert(phyIndex >= 0 &&
        phyIndex < node->numberPhys,
        "Unknown phy index while accessing phy model");

    thisPhy = node->phyData[phyIndex];

    ERROR_Assert(thisPhy != NULL, "PHY model not found");

    phyModel = thisPhy->phyModel;

    return phyModel;
}

std::string PHY_GetPhyName(
                Node *node,
                int phyIndex)
{
    switch (PHY_GetPhyModel(node, phyIndex))
    {
        case PHY802_11a:
            return "802.11a";
        case PHY802_11b:
            return "802.11b";
        case PHY_ABSTRACT:
            return "Abstract";
        case PHY_GSM:
            return "GSM";
        case PHY_FCSC_PROTOTYPE:
            return "FCSC Prototype";
        case PHY_SPAWAR_LINK16:
            return "Link16";
        case PHY_BOEINGFCS:
            return "Boeing FCS";
        case PHY_SATELLITE_RSV:
            return "Satellite RSV";
        case PHY802_16:
            return "802.16";
        case PHY_CELLULAR:
            return "Cellular";
        case PHY_JAMMING:
            return "Jamming";
        case PHY802_15_4:
            return "802.15.4";
        case PHY_ABSTRACT_LAYER:
            return "Abstract Layer";
        case PHY_SRW_ABSTRACT:
            return "SRW";
        case PHY_LTE:
            return "LTE";
        case PHY_MODE5:
            return "Mode5";
        case PHY802_11n:
            return "802.11n";
        default:
            return "None";
    }
}

// /**
// FUNCTION :: PHY_isSignalFeatureMatchReceiverPhyModel
// LAYER :: Physical Layer.
// PURPOSE :: To check if the signal feature matches the receiver's phyModel.
//
// PARAMETERS ::
// + node      : Node*    : The node of interest.
// + phyIndex  : int      : The PHY index.
// RETURN     :: BOOL     : if the signal feature matches the receiver's phyModel
// **/

BOOL PHY_isSignalFeatureMatchReceiverPhyModel(
                Node *node,              //receiver node
                int phyIndex,            // receiver phy index
                PhyModel signalPhyModel) // signals PHY model
{
    BOOL phyModelMatch = FALSE;
    PhyModel receiverPhyModel = PHY_NONE;

    //Get the PHY model for the receiver node.
    receiverPhyModel = PHY_GetPhyModel(
                    node,
                    phyIndex);

    if (receiverPhyModel == signalPhyModel)
    {
        phyModelMatch = TRUE;
    }

    return phyModelMatch;
}


// /**
// API           :: PHY_InterferenceArrivalFromChannel
// LAYER         :: Physical
// PURPOSE       :: Called when a interference signal arrives
// PARAMETERS    ::
// + node         : Node *      : node pointer to node
// + phyIndex     : int         : interface index
// + channelIndex : int         : channel index
// + propRxInfo   : PropRxInfo* : information on the arrived signal
// + sigPower_mW  : double      : The inband interference power in mW
// RETURN        :: void :
// **/

void PHY_InterferenceArrivalFromChannel(
   Node* node,
   int phyIndex,
   int channelIndex,
   PropRxInfo *propRxInfo,
   double sigPower_mW)
{

    if (node->nodeId == propRxInfo->txNodeId)
    {
        return;
    }

    switch(node->phyData[phyIndex]->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11b:
        case PHY802_11a:
        case PHY802_11n:{
            Phy802_11InterferenceArrivalFromChannel(
                        node,
                        phyIndex,
                        channelIndex,
                        propRxInfo,
                        sigPower_mW);

            break;
        }
        case PHY_ABSTRACT: {

            PhyAbstractInterferenceArrivalFromChannel(
                        node,
                        phyIndex,
                        channelIndex,
                        propRxInfo,
                        sigPower_mW);

            break;
        }
#endif // WIRELESS_LIB
#ifdef LTE_LIB
        case PHY_LTE: {
            PhyLteInterferenceArrivalFromChannel(
                node, phyIndex, channelIndex, propRxInfo, sigPower_mW);

            break;
        }
#endif
#ifdef SENSOR_NETWORKS_LIB
       case PHY802_15_4: {
            Phy802_15_4InterferenceArrivalFromChannel(
                node, phyIndex, channelIndex, propRxInfo,
                sigPower_mW);

            break;
        }

#endif
#ifdef CELLULAR_LIB
        case PHY_GSM: {
            PhyGsmInterferenceArrivalFromChannel(
                node, phyIndex, channelIndex, propRxInfo, sigPower_mW);

            break;
        }
        case PHY_CELLULAR: {
            PhyCellularInterferenceArrivalFromChannel(
                node, phyIndex, channelIndex, propRxInfo, sigPower_mW);

            break;
        }
#elif UMTS_LIB
        case PHY_CELLULAR: {
            PhyCellularInterferenceArrivalFromChannel(
                node, phyIndex, channelIndex, propRxInfo, sigPower_mW);

            break;
        }
#endif // CELLULAR_LIB

#ifdef MILITARY_RADIOS_LIB
        case PHY_FCSC_PROTOTYPE: {
        //    PhyFcscInterferenceArrivalFromChannel(
        //        node, phyIndex, channelIndex, propRxInfo);

            break;
        }
#endif /* FCSC */

#ifdef ADDON_LINK16
        case PHY_SPAWAR_LINK16: {
        //    PhySpawar_Link16InterferenceArrivalFromChannel(
        //        node, phyIndex, channelIndex, propRxInfo);

            break;
        }
#endif // ADDON_LINK16

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            PhySatelliteRsvInterferenceArrivalFromChannel(
                node, phyIndex, channelIndex, propRxInfo, sigPower_mW);

            break;
        }
#endif // SATELLITE_LIB

#ifdef ADVANCED_WIRELESS_LIB
        case PHY802_16: {
            PhyDot16InterferenceArrivalFromChannel(
                node, phyIndex, channelIndex, propRxInfo, sigPower_mW);

            break;
        }
#endif // ADVANCED_WIRELESS_LIB

#ifdef ADDON_BOEINGFCS
        case PHY_SRW_ABSTRACT: {
          SRW::Nexus::PHY::InterferenceArrivalFromChannel(node, phyIndex, channelIndex, propRxInfo, sigPower_mW);
        } break;
#endif // FCS
        default: {
            ERROR_ReportError("Unknown or disabled PHY model\n");
        }
    }//switch//
}

// /**
// API           :: PHY_InterferenceEndFromChannel
// LAYER         :: Physical
// PURPOSE       :: Called when a interference signal ends
// PARAMETERS    ::
// + node         : Node *      : node pointer to node
// + phyIndex     : int         : interface index
// + channelIndex : int         : channel index
// + propRxInfo   : PropRxInfo* : information on the ended signal
// + sigPower_mW  : double      : The inband interference power in mW
// RETURN        :: void :
// **/

void PHY_InterferenceEndFromChannel(
   Node* node,
   int phyIndex,
   int channelIndex,
   PropRxInfo *propRxInfo,
   double sigPower_mW)
{

    if (node->nodeId == propRxInfo->txNodeId)
    {
        return;
    }

    switch(node->phyData[phyIndex]->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11b:
        case PHY802_11a:
        case PHY802_11n:
        {
            Phy802_11InterferenceEndFromChannel(
                node, phyIndex, channelIndex, propRxInfo, sigPower_mW);

            break;
        }
        case PHY_ABSTRACT: {
            PhyAbstractInterferenceEndFromChannel(
                node, phyIndex, channelIndex, propRxInfo, sigPower_mW);

            break;
        }
#endif // WIRELESS_LIB
#ifdef LTE_LIB
        case PHY_LTE: {
            PhyLteInterferenceEndFromChannel(
                node, phyIndex, channelIndex, propRxInfo, sigPower_mW);

            break;
        }
#endif
#ifdef SENSOR_NETWORKS_LIB
         case PHY802_15_4: {
            Phy802_15_4InterferenceEndFromChannel(
                node, phyIndex, channelIndex, propRxInfo, sigPower_mW);

            break;
        }
#endif //SENSOR_NETWORKS_LIB
#ifdef CELLULAR_LIB
        case PHY_GSM: {
            PhyGsmInterferenceEndFromChannel(
                node, phyIndex, channelIndex, propRxInfo, sigPower_mW);

            break;
        }
        case PHY_CELLULAR: {
            PhyCellularInterferenceEndFromChannel(
                node, phyIndex, channelIndex, propRxInfo);

            break;
        }
#elif UMTS_LIB
        case PHY_CELLULAR: {
            PhyCellularInterferenceEndFromChannel(
                node, phyIndex, channelIndex, propRxInfo);

            break;
        }
#endif // CELLULAR_LIB

#ifdef MILITARY_RADIOS_LIB
        case PHY_FCSC_PROTOTYPE: {
          //  PhyFcscInterferenceEndFromChannel(
          //      node, phyIndex, channelIndex, propRxInfo, sigPower_mW);

            break;
        }
#endif /* MILITARY_RADIOS_LIB */

#ifdef ADDON_LINK16
        case PHY_SPAWAR_LINK16: {
         //   PhySpawar_Link16InterferenceEndFromChannel(
         //       node, phyIndex, channelIndex, propRxInfo);

            break;
        }
#endif // ADDON_LINK16

#ifdef SATELLITE_LIB
        case PHY_SATELLITE_RSV: {
            PhySatelliteRsvInterferenceEndFromChannel(node, phyIndex,
                            channelIndex, propRxInfo, sigPower_mW);

            break;
        }
#endif // SATELLITE_LIB_RSV

#ifdef ADVANCED_WIRELESS_LIB
        case PHY802_16: {
            PhyDot16InterferenceEndFromChannel(
                node, phyIndex, channelIndex, propRxInfo);

            break;
        }
#endif // ADVANCED_WIRELESS_LIB

#ifdef ADDON_BOEINGFCS
        case PHY_SRW_ABSTRACT: {
          SRW::Nexus::PHY::InterferenceEndFromChannel(node, phyIndex, channelIndex, propRxInfo, sigPower_mW);
        } break;
#endif // FCS
        default: {
            ERROR_ReportError("Unknown or disabled PHY model\n");
        }
    }//switch//
}

/*********HT START*************************************************/

void PHY_SetTxVector(Node* node,
                     int phyIndex,
                     MAC_PHY_TxRxVector& txVector) {
    PhyData* thisPhy = node->phyData[phyIndex];

    switch(thisPhy->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11n:
            {
                const MAC_PHY_TxRxVector vector =
                                        (MAC_PHY_TxRxVector&)txVector;
                Phy802_11nSetTxVector(thisPhy, vector);
                break;
            }
        default:
            {
                ERROR_ReportError("Phy Model not 802.11n");
                break;
            }
#endif
    }
}

void PHY_GetTxVector(Node *node,
                     int phyIndex,
                     MAC_PHY_TxRxVector& txVector) {
    PhyData* thisPhy = node->phyData[phyIndex];

    switch(thisPhy->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11n:
            {
                MAC_PHY_TxRxVector vector;
                Phy802_11nGetTxVector(thisPhy, vector);
                memcpy(&txVector , &vector, sizeof(vector));
                break;
            }
        default:
            {
                ERROR_ReportError("Phy Model not 802.11n");
                break;
            }
#endif
    }
}

void PHY_GetTxVectorForBC(Node *node,
                     int phyIndex,
                     MAC_PHY_TxRxVector& txVector) {
    PhyData* thisPhy = node->phyData[phyIndex];

    switch(thisPhy->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11n:
            {
                MAC_PHY_TxRxVector vector;
                Phy802_11nGetTxVectorForBC(thisPhy, vector);
                memcpy(&txVector , &vector, sizeof(vector));
                break;
            }
        default:
            {
                ERROR_ReportError("Phy Model not 802.11n");
                break;
            }
#endif
    }
}


void PHY_GetTxVectorForHighestDataRate(Node *node,
                     int phyIndex,
                     MAC_PHY_TxRxVector& txVector) {
    PhyData* thisPhy = node->phyData[phyIndex];

    switch(thisPhy->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11n:
            {
                    MAC_PHY_TxRxVector vector;
                    Phy802_11nGetTxVectorForHighestDataRate(thisPhy, vector);
                    memcpy(&txVector , &vector, sizeof(vector));
                    break;
            }
        default:
            {
                ERROR_ReportError("Phy Model not 802.11n");
                break;
            }
#endif
    }
}

void PHY_GetTxVectorForLowestDataRate(Node *node,
                                      int phyIndex,
                                      MAC_PHY_TxRxVector& txVector) {
    PhyData* thisPhy = node->phyData[phyIndex];

    switch(thisPhy->phyModel) {

#ifdef WIRELESS_LIB
        case PHY802_11n:
        {
            MAC_PHY_TxRxVector vector;
            Phy802_11nGetTxVectorForLowestDataRate(thisPhy, vector);
            memcpy(&txVector , &vector, sizeof(vector));
            break;
        }
        default:
        {
            ERROR_ReportError("Phy Model not 802.11n");
            break;
        }
#endif
    }
}

void PHY_GetRxVector(Node *node,
                     int phyIndex,
                     MAC_PHY_TxRxVector& rxVector) {
    PhyData* thisPhy = node->phyData[phyIndex];

    switch(thisPhy->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11n:
            {
                MAC_PHY_TxRxVector vector;
                Phy802_11nGetRxVector(thisPhy, vector);
                memcpy(&rxVector , &vector, sizeof(vector));
                break;
            }
        default:
            {
                ERROR_ReportError("Phy Model not 802.11n");
                break;
            }
#endif
    }
}

clocktype PHY_GetTransmissionDuration(Node *node,
                                      int phyIndex,
                                      MAC_PHY_TxRxVector& txVector) {

         PhyData* thisPhy = node->phyData[phyIndex];
        switch(thisPhy->phyModel) {
#ifdef WIRELESS_LIB
        case PHY802_11n:
            {
                const MAC_PHY_TxRxVector vector =
                                        (MAC_PHY_TxRxVector&)txVector;
                return Phy802_11nGetFrameDuration(thisPhy, vector);
                break;
            }
#endif
        default:
            {
                ERROR_ReportError("Default Case");
                return 0;
                break;
            }
    }
}

BOOL PHY_IsPhyHTEnabled(Node *node,
                        int phyIndex)
{
    PhyData* thisPhy = node->phyData[phyIndex];
#ifdef WIRELESS_LIB
    if (thisPhy->phyModel == PHY802_11n)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
#endif
}
/*********HT END****************************************************/


std::string PHY_GetChannelName(
                Node *node,
                int channelIndex)
{
    PropChannel* propChannel = node->propChannel;
    
    ERROR_Assert(propChannel != NULL,
                     "Propagation channel not found");

    ERROR_Assert(channelIndex >= 0 &&
        channelIndex < node->partitionData->numChannels,
        "Unknown channel index while accessing propagation"
        " channel name");

    return propChannel[channelIndex].name;
}

// /**
// FUNCTION         :: PHY_GetChannelIndexForChannelName
// LAYER            :: Physical Layer.
// PURPOSE          :: To get the channel index.
//
// PARAMETERS       ::
// + node           : Node*         : The node of interest.
// +channelName     :: std::string  : The name of the channel.
// RETURN           :: Int32        : Channel index.
// **/

Int32 PHY_GetChannelIndexForChannelName(Node* node, std::string channelName)
{
    Int32 i;
    PropChannel* propChannel = node->propChannel;

    for (i = 0; i < PROP_NumberChannels(node); i++)
    {
        if (propChannel[i].name == channelName)
        {
            return i;
        }
    }

    return -1;
}

// /**
// FUNCTION         :: PHY_ChannelNameExists
// LAYER            :: Physical Layer.
// PURPOSE          :: To check whether channelName exist or not.
//
// PARAMETERS       ::
// + node           :: Node*        : The node of interest.
// + channelName    :: std::string  : The name of the channel.
// RETURN           :: BOOL         : TRUE if channelName is valid
//                                    False if channel name is invalid
// **/

BOOL PHY_ChannelNameExists(Node* node, std::string channelName)
{
    Int32 i;
    PropChannel* propChannel = node->propChannel;

    for (i = 0; i < PROP_NumberChannels(node); i++)
    {
        if (propChannel[i].name == channelName)
        {
            return TRUE;
        }
    }

    return FALSE;
}
