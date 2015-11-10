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


//--------------------------------------------------------------------------
// File     : route_map.h
// Objective: Header file for route_map.c (CISCO implementation
//              of route map)
// Reference: CISCO document Set
//--------------------------------------------------------------------------

// Order of contents of this file
// includes, defines, enums, structs, function declarations


#ifndef ROUTE_MAP_H
#define ROUTE_MAP_H


// file includes

#include "network_ip.h"
#include "route_parse_util.h"


//--------------------------------------------------------------------------
//                  DEFINE's
//--------------------------------------------------------------------------

#define ROUTE_MAP_COMMON_MAX                 4294967295
#define ROUTE_MAP_SET_MAX_METRIC             294967295
#define ROUTE_MAP_SET_MIN_METRIC             -294967295
#define ROUTE_MAP_MATCH_MAX_LENGTH           0x7FFFFFFF
#define ROUTE_MAP_DEF_SEQ                    10
#define ROUTE_MAP_DEF_LOCAL_PREF             100
#define ROUTE_MAP_MAX_AS_PATH                199
#define ROUTE_MAP_MIN_AS_PATH                1


struct struct_route_map_command_params;

typedef BOOL (*RouteMapMatchFunc) (Node*,
                                   void*,
                                   struct struct_route_map_command_params*);
typedef void (*RouteMapSetFunc)(Node*,
                                void*,
                                struct struct_route_map_command_params*);


//--------------------------------------------------------------------------
//                  ENUMERATIONS's
//--------------------------------------------------------------------------


// match result for the MATCH command
typedef enum
{
    // if this is a match and permit
    ROUTE_MAP_MATCH_PERMIT,
    // if its a match and deny
    ROUTE_MAP_MATCH_DENY,
    // no match
    ROUTE_MAP_NO_MATCH
} RouteMapTriStateType;



// type of route map
typedef enum
{
    ROUTE_MAP_PERMIT,
    ROUTE_MAP_DENY
} RouteMapType;



// enumeration for all match command's
typedef enum
{
    AS_PATH,
    COMMUNITY,
    EXTCOMMUNITY,
    INTERFACE,
    IP_ADDRESS,
    IP_NEXT_HOP,
    IP_ROUTE_SRC,
    LENGTH,
    METRIC,
    ROUTE_TYPE,
    TAG
} RouteMapMatchType;



// enumeration for all set command's
typedef enum
{
    AUTO_TAG,
    DEF_INTF,
    SET_INTERFACE,
    IP_DEF_NEXT_HOP,
    IP_DEF_NEXT_HOP_VERFY_AVAIL,
    IP_PRECEDENCE,
    SET_IP_NEXT_HOP,
    IP_NEXT_HOP_VERFY_AVAIL,
    LEVEL,
    LOCAL_PREF,
    SET_METRIC,
    METRIC_TYPE,
    NEXT_HOP,
    SET_TAG
} RouteMapSetType;



// interface types for route type
typedef enum
{
    LOCAL,
    RT_TYPE_INTERNAL,
    EXTERNAL_TYPE_1,
    EXTERNAL_TYPE_2,
    RT_TYPE_LEVEL_1,
    RT_TYPE_LEVEL_2
} RouteMapRouteType;



// precedence types
typedef enum
{
    ROUTINE = 0,
    PRIORITY,
    MM_IMMEDIATE,
    FLASH,
    FLASH_OVERRIDE,
    CRITICAL,
    INTERNET,
    NETWORK,
    // just in case if its not defined in the set statements
    NO_PREC_DEFINED
} RouteMapPrecedence;



// interface types for level
typedef enum
{
    LEVEL_1,
    LEVEL_2,
    LEVEL_1_2,
    STUB_AREA,
    BACKBONE
} RouteMapLevel;



// metric type
typedef enum
{
    INTERNAL,
    EXTERNAL,
    TYPE_1,
    TYPE_2
} RouteMapMetricType;



//--------------------------------------------------------------------------
//                              STRUCT's
//--------------------------------------------------------------------------


// for storing interface params, for both match and set commands
typedef struct
struct_route_map_interface
{
    // for the time being we just have a integer, till we dont have
    //  the original CISCO style
    int intf;
    // interface type
    RtInterfaceType type;

    // interface number
    char* intfNumber;

    // whether default setting, TRUE if its default case, used in case of
    //  set commands
    BOOL defaultSetting;

    // the next possible set
    struct struct_route_map_interface* next;
} RouteMapIntf;



// for match ip address, match ip next hop, match ip route source
typedef struct
struct_route_map_acl_list
{
    // access list ID
    char* aclId;

    // the next possible set
    struct struct_route_map_acl_list* next;
} RouteMapAclList;



// for match length
typedef struct
struct_route_map_match_length
{
    // min length
    unsigned int min;

    // max length
    unsigned int max;
} RouteMapMatchLength;



// for match and set metric
// note that the limit for match metric is from 0 to 4294967295 and for
//   set metric is from -294967295 to 294967295.
typedef struct
struct_route_map_metric
{
    // metric value
    unsigned int metric;
} RouteMapMetric;



// match route type
typedef struct
struct_route_map_route_type
{
    // route type value
    RouteMapRouteType rtType;
} RouteMapMatchRouteType;



// for match tag
typedef struct
struct_route_map_match_tag
{
    // tag values
    unsigned int tag;
    struct struct_route_map_match_tag* next;
} RouteMapMatchTag;



// Note:
//  no struct needed for set automatic tag. Also OSPF is the
//  only protocol supported for tags by Cisco. QualNet's OSPF does
//  not support tag as of yet.
//

typedef struct
struct_route_map_add_list
{
    // next hop address
    NodeAddress address;

    // the next address
    struct struct_route_map_add_list* next;
} RouteMapAddressList;


typedef struct
struct_route_map_set_ip_next_hop
{
    // whether default setting
    BOOL defaultSetting;

    // first next hop address
    RouteMapAddressList* addressHead;

    // last next hop address
    RouteMapAddressList* addressTail;
} RouteMapSetIPNextHop;



// Note:
//  no struct needed for set precedence. Use the enum RouteMapPrecedence
//  no struct needed for set level. Use the enum RouteMapLevel
//  no struct needed for set metric-type. Use the enum RouteMapMetricType



// struct for set local preference
typedef struct
struct_route_map_set_local_pref
{
    // preference value
    unsigned int prefValue;
} RouteMapSetLocalPref;



// Note:
//  no struct needed for set next-hop.


// struct for set tag value
typedef struct
struct_route_map_set_tag
{
    // tag value
    unsigned int tagVal;
} RouteMapSetTag;



// Match rules deposited
typedef struct
struct_route_map_match
{
    // Type of match command  < Only the CISCO ones are implemented.>
    RouteMapMatchType matchType;

    // Match params
    void* params;

    // Corresponding match function
    RouteMapMatchFunc matchFunc;

    // the next match command
    struct struct_route_map_match* next;

} RouteMapMatchCmd;



// Set rules defined
typedef struct
struct_route_map_set
{
    // type of set command < Only the CISCO ones are implemented.>
    RouteMapSetType setType;

    // we hold the params
    void* params;

    // Corresponding set function
    RouteMapSetFunc setFunc;

    // Corresponding free function
    //void (*freeSet)(void*);

    // the next match command
    struct struct_route_map_set* next;

} RouteMapSetCmd;


// route map structure
typedef struct
struct_route_map_entry
{
    // tag of the route map
    char* mapTag;

    // the seq number
    int seq;

    // route map type...permit or deny
    RouteMapType type;

    // match commands
    RouteMapMatchCmd* matchHead;
    RouteMapMatchCmd* matchTail;

    // set commands
    RouteMapSetCmd* setHead;
    RouteMapSetCmd* setTail;

    // link to the next entry
    struct struct_route_map_entry* next;
} RouteMapEntry;



// route map structure
typedef struct
struct_route_map
{
    // tag of the route map
    char* mapTag;

    // does it have default sets
    BOOL hasDefault;

    // command set
    RouteMapEntry* entryHead;

    // the tail of the list
    RouteMapEntry* entryTail;

    // link to the next route map
    struct struct_route_map* next;
} RouteMap;


// route map structure
typedef struct
struct_route_map_list
{
    struct struct_route_map* head;
    struct struct_route_map* tail;
} RouteMapList;


// this may be omitted, if the external applications invoking route map
//  have their own list of route maps
// list for the route map hooked in
typedef struct
struct_route_map_hook_list
{
    // head for the route map
    RouteMap* head;

    // tail for the route map
    RouteMap* tail;
} RouteMapHookList;


// struct for passing the values of path list number
// Developer comments: Not very sure whether this is the right format for
//  passing path list numbers. This structure may go for further change
//  if needed while matching in a BGP domain.
typedef struct
struct_route_map_path_list
{
    int pathListNumber;
    struct struct_route_map_path_list* next;
} RouteMapPathList;



// struct for passing the params needed for match and set commands
typedef struct
struct_route_map_command_params
{
    RouteMapPathList* matchPathList;

    // for match interface, index of outgoing interface
    RtInterfaceType matchDefIntfType;
    char* matchInterfaceIndex;

    // for match ip address, network number
    //  destination address
    NodeAddress matchDestAddress;
    NodeAddress matchDestAddressMask;
    NodeAddress matchSrcAddress;

    // for match ip next-hop, next hop router address
    //  expected specific dest address
    NodeAddress matchDestAddNextHop;

    // for match ip route-source, next hop router address
    //  its upto the user to decide whether to send the
    //  specific dest add or the dest number address
    NodeAddress matchDestAddrRtSrc;

    // for match length, level 3 length of a packet
    unsigned int matchLength;

    // for match metric
    //  metric and cost in qualnet are all the same
    unsigned int matchCost;

    // for match route-type
    RouteMapRouteType matchRtType;

    // for match tag
    unsigned int matchTag;

    // set params follow

    // for set automatic-tag
    BOOL setAutoTag;

    // for set default interface
    RouteMapIntf* setDefIntf;

    // for set interface
    RouteMapIntf* setIntf;

    // for set ip default next hop
    RouteMapSetIPNextHop* setDefNextHop;

    // for set ip default next-hop verify availability
    BOOL setIPDefNextHopVerAvail;

    // for set ip next-hop
    RouteMapSetIPNextHop* setIpNextHop;

    // for set ip next-hop verify availability
    BOOL setIPNextHopVerAvail;

    // for set precedence
    RouteMapPrecedence setIpPrecedence;

    // for set level
    RouteMapLevel setLevel;

    // for set local-preference
    unsigned int setLocalPref;

    // for set metric
    int setMetric;

    // for set metric-type
    RouteMapMetricType setMetricType;

    // for set next-hop
    NodeAddress setNextHop;

    // for set tag
    unsigned int setTag;

} RouteMapValueSet;




//--------------------------------------------------------------------------
//                  FUNCTION DECLARATION's
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
// FUNCTION  RouteMapInit
// PURPOSE   Initialize the route map
// ARGUMENTS node, node concerned
//           nodeInput, Main configuration file
// RETURN    None
//--------------------------------------------------------------------------

void RouteMapInit(Node* node, const NodeInput* nodeInput);



//--------------------------------------------------------------------------
// FUNCTION  RouteMapAddHook
// PURPOSE   Plug the route map
// ARGUMENTS routeMapName, name of the route map to be hooked
// RETURN    Pointer to the corresponding route map
//--------------------------------------------------------------------------

RouteMap* RouteMapAddHook(Node* node,
                     char* routeMapName);

//--------------------------------------------------------------------------
// FUNCTION  RouteMapFindByName
// PURPOSE   Get the corresponding route map pointer from the list or NULL
//              if not found.
// ARGUMENTS node, corresponding node
//           routeMapName, name of the route map to be looked
// RETURN    The corresponding route map or NULL if not found
//--------------------------------------------------------------------------

RouteMap* RouteMapFindByName(Node* node, char* routeMapName);


//--------------------------------------------------------------------------
// FUNCTION  RouteMapEntryFindBySeq
// PURPOSE   Get the corresponding route map with the mentioned sequence
//              number
// ARGUMENTS node, corresponding node
//           routeMap, head of the route map of same tag from where we will
//              search for the route map with the specifed tag
// RETURN    The corresponding route map entry or NULL if not found
//--------------------------------------------------------------------------
RouteMapEntry* RouteMapEntryFindBySeq(Node* node, RouteMap* rMap, int seq);


//--------------------------------------------------------------------------
// FUNCTION  RouteMapAction
// PURPOSE   Perform the match and set action
// ARGUMENTS node, the node where route map is set
//           valueSet, the set of values to match with the criteria
//           routeMapHookList, the list of route maps that are to be
//              analyzed
// RETURN    RouteMapTriStateType. See comments below.
// COMMENT   For necessity of re-distribution the return type is made
//              ROUTE_MAP_MATCH_PERMIT if its a match and PERMIT or
//              ROUTE_MAP_MATCH_DENY if its a match and DENY and
//              ROUTE_MAP_NO_MATCH if its not a match.
//--------------------------------------------------------------------------
BOOL RouteMapAction(Node* node,
                    RouteMapValueSet* valueSet,
                    RouteMapEntry* entry);



//--------------------------------------------------------------------------
// FUNCTION  RouteMapInitValueList
// PURPOSE   Allocate and initialize a new value set
// ARGUMENTS void
// RETURN    Pointer to the newly created value set.
// COMMENTS  Use this function when you want a new value set for passing
//              match values and retrieving set values.
//--------------------------------------------------------------------------

RouteMapValueSet* RouteMapInitValueList(void);



//--------------------------------------------------------------------------
// FUNCTION  RouteMapShowCmd
// PURPOSE   Simulating the 'show route map' command.
// ARGUMENTS node, the current node
//           rMapName, name of route map for showing specific route map
//           or NULL' keyword to show all.
// RETURN    void
//--------------------------------------------------------------------------
void RouteMapShowCmd(Node* node, char* rMapName);


//--------------------------------------------------------------------------
// FUNCTION  RouteMapPrintRMapEntry
// PURPOSE   Print the given route map entry in the console
// ARGUMENTS node, the current node
//           entry, name of route map entry to print
// RETURN    void
// ASSUMPTION Only the type of match and set command is printed.
//              The detailed param list is omitted.
//--------------------------------------------------------------------------
void RouteMapPrintRMapEntry(Node* node, RouteMapEntry* entry);


//--------------------------------------------------------------------------
// FUNCTION  RouteMapFinalize
// PURPOSE   Free all the route maps defined for this node
// ARGUMENTS node, concerned node
// RETURN    None
// COMMENT   Make sure to free all the hooked route maps before calling
//              this function
//--------------------------------------------------------------------------
void RouteMapFinalize(Node* node);


//--------------------------------------------------------------------------
// FUNCTION  RouteMapMatchEntry
// PURPOSE   Perform the match action
// ARGUMENTS node, the concerned node
//           entry, the node where route map is set
//           valueSet, the set of values to match with the criteria
// RETURN    TRUE if its a match, else
//--------------------------------------------------------------------------
BOOL RouteMapMatchEntry(Node* node,
                        RouteMapEntry* entry,
                        RouteMapValueSet* valueSet);


//--------------------------------------------------------------------------
// FUNCTION  RouteMapSetEntry
// PURPOSE   Perform the set action
// ARGUMENTS node, the concerned node
//           entry, the node where route map is set
//           valueSet, the set of values to match with the criteria
// RETURN    void
// COMMENTS:
// They are evaluated in the following order:
// set ip next-hop
// set interface
// set ip default next-hop
// set default interface
//  if all of them are defined. This has to be arranged in the correct order
//      by the caller functionality depending on the above available
//      commands.
//--------------------------------------------------------------------------
void RouteMapSetEntry(Node* node,
                      RouteMapEntry* entry,
                      RouteMapValueSet* valueSet);
#endif // ROUTE_MAP_H


