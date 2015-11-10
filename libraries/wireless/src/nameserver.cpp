// Copyright (c) 2001-2013, SCALABLE Network Technologies, Inc.  All Rights Reserved.
// All Rights Reserved.
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
#define __UTIL_NAMESERVER_C__ 

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <iostream>
#include <memory>
#include <map>

#include <math.h>

#include "qualnet_mutex.h"

#include "api.h"
#include "partition.h"

#include "util_nameservice.h"

using namespace std;

///
/// @file nameserver.cpp
///
/// @brief Implementation code for Utility nameservice
///

///
/// @def DEBUG
///
/// @brief Local debugging MACRO
///

#ifdef DEBUG
# undef DEBUG
#endif 

#define DEBUG 0

///
/// @brief Private mutex lock and for node lookup service
///

static QNPartitionMutex *mapLock = new QNPartitionMutex();
static QNPartitionMutex *nsLock = new QNPartitionMutex();

/// string to object lookup
static map<string, void*> nsMap;

///
/// nodeId to node data struture lookup
/// Not used when across cluster nodes
///

static map<NodeId, Node*> nodeMap;

///
/// @brief Indicate to the name service that all initialization
/// in the simulation is complete
///

void UTIL_NameServiceReady(void)  
{ 

}

///
/// @brief Request the name service initialize itself
///

void UTIL_NameServiceInit(void) 
{ 

}

///
/// @brief Initialize the node portion of the name service
///
/// @param node a pointer to the local node context being initialized
///

void UTIL_NameServiceInitLocal(Node *node) 
{ 
    UTIL_NameServiceRegisterNode(node);
}

///
/// @brief Indicate to the name service that the simulation has
/// reached an epoch (reset point or end of simulation)
///

void UTIL_NameServiceEpoch(void) 
{
    // nsMap.~map();
    // nodeMap.~map();
}

///
/// @brief Indicate to the name service that the local node's
/// name service is to be reset
///
/// @param node a pointer to the local node context data
///

void UTIL_NameServiceEpochLocal(Node *node) 
{
    node->localMap.~map();
    UTIL_NameServiceDeregisterNode(node);
}

///
/// @brief UTIL_NameServicePutImmutable
///
/// @param key a textual string of the key
/// @obj a pointer to the name service value
/// @node a pointer to the local node data context
///
/// @returns TRUE if the object is installed in the
/// name service (i.e. is the first object with that
/// key) and FALSE if the object cannot be added because
/// the store would no longer be immutable.
///

BOOL UTIL_NameServicePutImmutable(string key, 
                                  void* obj, 
                                  Node* node)
{
    return UTIL_NameServicePutImmutable(key, 
                                        obj, 
                                        node, 
                                        -1);
}

///
/// @brief UTIL_NameServicePutImmutable
///
/// @param key a textual string of the key
/// @obj a pointer to the name service value
///
/// @returns TRUE if the object is installed in the
/// name service (i.e. is the first object with that
/// key) and FALSE if the object cannot be added because
/// the store would no longer be immutable.
///

BOOL UTIL_NameServicePutImmutable(string key, 
                                  void* obj) 
{
    return UTIL_NameServicePutImmutable(key, 
                                        obj, 
                                        0, 
                                        -1);
}


///
/// @brief UTIL_NameServicePutImmutable
///
/// @param key a textual string of the key
/// @param obj a pointer to the name service value
/// @param node a pointer to the local node data context
/// @param ifidx the logical index of the local data
///
/// @returns TRUE if the object is installed in the
/// name service (i.e. is the first object with that
/// key) and FALSE if the object cannot be added because
/// the store would no longer be immutable.
///

BOOL UTIL_NameServicePutImmutable(string key, 
                                  void* obj, 
                                  Node* node, 
                                  int ifidx) 
{
    bool added = false;
    bool isLocal;
    
    if (node == NULL) 
    {
        isLocal = false;
    }
    else
    {
        isLocal = true;
    }

    if (DEBUG)
    {
        string isLocalStr;
        
        if (isLocal)
        {
            isLocalStr = "LOCAL";
        }
        else
        {
            isLocalStr = "GLOBAL";
        }
        
        printf("PutImmutable(%s, %p) [%s]\n", 
               key.c_str(), 
               obj,
               isLocalStr.c_str());
    }

    if (ifidx != -1) 
    {
        char newkey[64];
        sprintf(newkey, "/Interface %d", ifidx);

        key.insert(0, newkey);

        if (DEBUG)
            {
            printf("-->transformed key: %s\n", 
                   key.c_str());
            }
        
    }

    if (isLocal) 
    {
        map<string,void*>::iterator pos = node->localMap.find(key);
        
        bool atEnd = pos == node->localMap.end();

        if (atEnd) 
        {
            node->localMap[key] = obj;
            added = true;
            
            if (DEBUG)
            {
                printf("--> Successfully added as %p\n", 
                       node->localMap[key]);
            }            
        } 
        else 
        {
            
            if (DEBUG)
            {
                printf("--> cannot add to an immutable entry.\n");
            }
        }
    } 
    else
    {
        QNPartitionLock lock(nsLock);

        map<string,void*>::iterator pos = nsMap.find(key);
        bool atEnd = pos == nsMap.end();

        if (atEnd) 
        {
            nsMap[key] = obj;
            added = true;
            
            if (DEBUG)
            {
                printf("--> Successfully added as %p\n", 
                       nsMap[key]);
            }            
        } 
        else
        {
            
            if (DEBUG)
            {
                printf("--> cannot add to an immutable entry.\n");
            }
        }
    }


    return added;
}

///
/// @brief Remove selected object from name service
///
/// @param key a textual string representing the key name
/// @returns the objected removed from the name service
///

void *UTIL_NameServiceRemove(string key) 
{
    return UTIL_NameServiceRemove(key, 
                                  0, 
                                  -1);
}

///
/// @brief Remove selected object from name service
///
/// @param key a textual string representing the key name
/// @param node a pointer to the local node context data
/// @returns the objected removed from the name service
///

void *UTIL_NameServiceRemove(string key, 
                             Node* node) 
{
    return UTIL_NameServiceRemove(key, 
                                  node, 
                                  -1);
}

///
/// @brief Remove selected object from name service
///
/// @param key a textual string representing the key name
/// @param node a pointer to the local node context data
/// @param ifidx the logical index of the current process
///
/// @returns the objected removed from the name service
///

void *UTIL_NameServiceRemove(string key, 
                             Node* node, 
                             int ifidx) 
{
    void* obj = NULL;
    bool isLocal = node == 0;

    if (ifidx != -1) 
    {
        char newkey[64];
        sprintf(newkey, 
                "/Interface %d", 
                ifidx);

        key.insert(0, newkey);
    }

    if (isLocal) 
    {
        obj = node->localMap[key];
        node->localMap.erase(key);
    } 
    else
    {
        QNPartitionLock lock(nsLock);

        obj = nsMap[key];
        nsMap.erase(key);
    }

    return obj;
}

///
/// @brief Return the immutable object for a given key
///
/// @param key a textual representation of the key name
/// @returns a reference to the object
///

void *UTIL_NameServiceGetImmutable(string key) 
{
    return UTIL_NameServiceGetImmutable(key, 
                                        FALSE, 
                                        0, 
                                        -1);
}

///
/// @brief Return the immutable object for a given key
///
/// @param key a textual representation of the key name
/// @param weak TRUE if weak, FALSE if strong
///
/// @returns a reference to the object
///

void *UTIL_NameServiceGetImmutable(string key, 
                                   BOOL weak)
{
    return UTIL_NameServiceGetImmutable(key, 
                                        weak, 
                                        0, 
                                        -1);
}

///
/// @brief Return the immutable object for a given key
///
/// @param key a textual representation of the key name
/// @param weak TRUE if weak, FALSE if strong
/// @param node a pointer to the local node data context
///
/// @returns a reference to the object
///

void *UTIL_NameServiceGetImmutable(string key, 
                                   BOOL weak, 
                                   Node* node) 
{
    return UTIL_NameServiceGetImmutable(key, 
                                        weak, 
                                        node, 
                                        -1);
}

///
/// @brief Return the immutable object for a given key
///
/// @param key a textual representation of the key name
/// @param weak TRUE if weak, FALSE if strong
/// @param node a pointer to the local node data context
/// @param ifidx the logical index of the local process
///
/// @returns a reference to the object
///

void *UTIL_NameServiceGetImmutable(string key, 
                                   BOOL weak, 
                                   Node* node, 
                                   int ifidx) 
{
    void* obj = NULL;
    bool isLocal = (node != 0);

    if (DEBUG)
    {
        string isLocalStr;
        
        if (isLocal)
        {
            isLocalStr = "LOCAL";
        }
        else
        {
            isLocalStr = "GLOBAL";
        }
        
        printf("GetImmutable(%s) [%s]\n", 
               key.c_str(), 
               isLocalStr.c_str());
    
    }

    if (ifidx != -1) 
    {
        char newkey[64];
        sprintf(newkey, 
                "/Interface %d", 
                ifidx);

        key.insert(0, newkey);

        if (DEBUG)
        {
            printf("-->transformed key: %s\n", 
                   key.c_str());
        }
        
    }

    if (isLocal) 
    {
        map<string, void*>::iterator pos = node->localMap.find(key);
        
        if (pos != node->localMap.end()) 
        {
            obj = pos->second;
            
            if (DEBUG)
            {
                printf("--> Successfully found as %p\n", 
                       obj);
            }
            
        } 
        else
        {
            
            if (DEBUG)
            {
                printf("--> object does not exist.\n");
            }
            
        }
    } 
    else
    {
        QNPartitionLock lock(nsLock);

        map<string, void*>::iterator pos = nsMap.find(key);
        
        if (pos != nsMap.end()) 
        {
            obj = pos->second;
            
            if (DEBUG)
            {
                printf("--> Successfully found as %p\n", 
                       obj);
            }
            
        } 
        else
        {
            
            if (DEBUG)
            {
                printf("--> object does not exist.\n");
            }
            
        }
    }

    return obj;
}

///
/// @brief Lookup a pointer to the node data based on the nodeID
///
/// @param nodeID a logical node index
///
/// @returns a pointer to node data structure
///
/// Be careful with this routine.  It is deprecated and all
/// models use other methods based on sending messages. 
/// 
/// @see MESSAGE_Send()
///

Node* UTIL_NameServiceLookupNode(int nodeID) 
{
    return nodeMap[nodeID];
}

/// 
/// @brief Remove a node pointer association from the name service
///
/// @param node a pointer to the node data structure
///

void UTIL_NameServiceDeregisterNode(Node *node) 
{
    QNPartitionLock lock(mapLock);
    nodeMap.erase(node->nodeId);
}

/// 
/// @brief Register a node pointer association in the name service
///
/// @param node a pointer to the node data structure
///

void UTIL_NameServiceRegisterNode(Node *node) 
{
    QNPartitionLock lock(mapLock);
    nodeMap[node->nodeId] = node;
}

///
/// @brief Allocate a programming kernel data value
///
/// @param node a pointer to the local node data structure
/// @param attr a textual string representation of the key name
/// @param data a pointer to the value association
/// @param type the type of association
///
/// @see UTIL_KptrScope
///

void UTIL_KptrAlloc(Node* node, 
                    string attr, 
                    void* data, int type) 
{
    node->localMap[attr] = data;
}

///
/// @brief Retrieve a reference to a value in the name service
///
/// @param node a pointer to the local node data
/// @param attr a textual string representation of the key name
/// 
/// @returns a pointer to the object in the name service
///

void *UTIL_KptrTake(Node* node, 
                    string attr) 
{
    return node->localMap[attr];
}

///
/// @brief Return a reference to a value in the name service
///
/// @param node a pointer to the local node data
/// @param attr a textual string representation of the key name
/// @param data a pointer to the returned object
///

void UTIL_KptrGive(Node* node, 
                   string attr, 
                   void * data) 
{
    
}

///
/// @brief Free an object in the name service
///
/// @param node a pointer to the local node data
/// @param attr a textual string representation of the key name
///

void UTIL_KptrFree(Node* node, 
                   string attr)
{
    node->localMap.erase(attr);
}
