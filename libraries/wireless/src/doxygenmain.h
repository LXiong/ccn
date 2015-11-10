/* This is a dummy code file that only contains doxygen main page documentation. */

/*!
 * \file doxygenmain.h
 * \brief Doxygen main page.
 */

/** \mainpage Dot11 Addon
 * <center> Qualnet 802.11 MAC addon. </center>
 
 * \section intro Introduction
 *
 * Dot11 is an implementation of the 802.11 for the Qualnet MAC layer 
 * according to the IEEE 802.11 MAC specification. 
 * This code is based on the existing Qualnet 802.11 (mac_802_11) with
 * the addition of dynamic link management (AP joining) and extended
 * Access Point functionality.
 *
 * The code is implemented as several code modules based on functionality.
 * There are currently six modules and are as follows:
 * 
 * - mac_dot11
 * - mac_dot11-ap
 * - mac_dot11-mgmt
 * - mac_dot11-mib
 * - mac_dot11-pc
 * - mac_dot11-sta
 *
 * <BR><BR>
 * \section desc Module Descriptions
 *
 * Functionality provided by the above modules are as follows:
 *
 * - Station related functionality.  
 (<a class="el" href="mac__dot11-sta_8cpp.html"> mac_dot11-sta.cpp </a> )
 *   - Framer and deframer
 *   - DCF media access
 *   - Helper functions
 *
 * - Access Point functionality.
(<a class="el" href="mac__dot11-ap_8cpp.html"> mac_dot11-ap.cpp </a> )
 *   - Station catching
 *   - Beaconing
 *
 * - Link management functionality.
(<a class="el" href="mac__dot11-mgmt_8cpp.html"> mac_dot11-mgmt.cpp </a> )
 *   - AP Join management
 *   - Link management
 *   - Helper functions
 *
 * - Point Controller functionality.
(<a class="el" href="mac__dot11-pc_8cpp.html"> mac_dot11-pc.cpp </a> )
 *   - PCF media access
 *   - Polling routines
 *
 * - MIBs
(<a class="el" href="mac__dot11-mib_8cpp.html"> mac_dot11-mib.cpp </a> )
 *  - MIB structures
 *  - Access routines.
 *
 * - MAC layer interface functionality.
(<a class="el" href="mac__dot11_8cpp.html"> mac_dot11.cpp </a> )
 *   - Network layer interface
 *   - Physical layer interface
 *   - Frame filtering
 *   - Message interface
 *   - Timer message handling
 *
 * - 802.11 structures.
(<a class="el" href="mac__dot11_8h.html"> mac_dot11.h </a> )
 *   - Global structure
 *   - Frame structures
 *   - Helper functions
 * 
 *<BR><BR>
 * \section config Configuration
 *
 * Dot11 has several new configuration options in addition to the previous 802.11
 * configuration parameters that where defined in the original mac_801_11 MAC code.
 * In Dot11, these parameters are now prefixed with MAC-DOT11. 
 * ( i.e. MAC-802.11-RELAY-FRAMES is now MAC-DOT11-RELAY-FRAMES. It is used the same
 *  way.)
 *
 * The new additional parameters is as follows:
 *
 * <BR>
 * \subsection MAC-PROTOCOL
 *
 * MACDOT11 is a new MAC protocol type used to invoke the new Dot11 MAC protocol.
 *
 *  \par Parameter:
 *  MAC-PROTOCOL  \e MACDOT11
 *
 *  \par Usage: 
 *  Set the MAC protocol to be Dot11.\n\n
 *  <tt>MAC-PROTOCOL    MACDOT11</tt>
 *
 * <BR>
 * \subsection MAC-DOT11-ASSOCIATION
 *
 * This will allow the nodes to associate dynamically with the AP with the SSID.
 *
 *  \par Parameter: 
 *  \e MAC-DOT11-ASSOCIATION    {DYNAMIC | STATIC}
 *
 *  \arg \c DYNAMIC - Dynamic Association will dynamically associate with a AP of the appropriate SSID.
 *  \arg \c STATIC - Static Association will use a preassigned subnet for association.
 *
 *  \par Default:  
 *  <tt> STATIC </tt>
 *
 *  \par Usage:  
 *  Set the association type as DYNAMIC.\n\n
 *  <tt>MAC-DOT11-ASSOCIATION   DYNAMIC</tt>
 *
 * <BR>
 * \subsection MAC-DOT11-SSID
 *  This sets the SSID of the station. This is the SSID of the AP (BSS) to associate 
 *  with.
 * 
 *  \par Parameter: 
 *  \e MAC-DOT11-SSID  {STRING}
 *
 *  \par Default:  
 *  <tt> TEST1 </tt>
 *
 *  \par Usage:  
 *  Set the SSID to TEST1.\n\n
 *  <tt>MAC-DOT11-SSID  TEST1</tt>
 *
 * <BR>
 * \subsection MAC-DOT11-AP
 *  This parameter is used to set if the station is an AP.
 * 
 *  \par Parameter: 
 *  \e MAC-DOT11-AP  {YES | NO}
 *
 *  \par Default:  
 *  <tt> NO </tt>
 *
 *  \par Usage: 
 *  Set this station as an AP.\n\n
 *   <tt> MAC-DOT11-AP YES</tt>
 *
 * <BR>
 * \subsection MAC-DOT11-SCAN-TYPE
 *  Sets the type of scanning the dynamic association will use to find an AP to 
 * associate with.
 * 
 *  \par Parameter: 
 *  \e MAC-DOT11-SCAN-TYPE  {ACTIVE | PASSIVE}
 
 *  \arg \c ACTIVE - Active probing of a channel to find an AP.
 *  \arg \c PASSIVE - No probing. Passively listen to channel for beacon.
 *
 *  \par Default:  
 *  <tt> PASSIVE </tt>
 *
 *  \par Usage:
 *   Set this station scan type to PASSIVE.\n\n 
 *   <tt>MAC-DOT11-SCAN-TYPE  PASSIVE </tt>
 *
 * <BR>
 * \subsection MAC-DOT11-STA-CHANNEL
 *  Sets the physical channel to start listening on for a dynamic association.
 * 
 *  \par Parameter: 
 *  \e MAC-DOT11-STA-CHANNEL  {1 to number of channels}
 *
 *  \par Default:  
 *  <tt> 1 </tt>
 *
 *  \par Usage:
 *  Set this station scan channel to 1.
 *   <tt>MAC-DOT11-STA-CHANNEL  1</tt>
 *
 * <BR><BR>
 * \section docs Support Documents
 *
 * A detailed explanation of the implementation of the 802.11 functionality can be
 * found <A HREF="pcf.htm">HERE</A>
 *
 */

