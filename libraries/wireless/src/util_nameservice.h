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
//
#ifndef __UTIL_UTIL_NAMESERVER_H__
# define __UTIL_UTIL_NAMESERVER_H__

#include <string>
#include <map>

#include "qualnet_mutex.h"

/// 
/// @file util_nameservice.h
///
/// @brief Definitions for the QualNet utility name service
///

/// 
/// @def WEAK
/// @brief Defines a weak memory reference
///
/// @def STRONG
/// @brief defines a strong memory reference
///
/// Weak and strong refer to the property of memory. A weak 
/// reference is not guaranteed to exist for all times.  A strong
/// reference is essentially ensuring the memory region exists and is
/// valid until the reference is released.  For many idempotent memory
/// applications, weak is sufficient and vastly faster.
///

#define WEAK TRUE
#define STRONG FALSE

///
/// @enum UTIL_KptrScope
/// 
/// @brief The enumerated set of scoping rules for nameserver pointers
///

enum UTIL_KptrScope
{ 
    /// Local to the node
    KPTR_PRIVATE = 0,
    
    /// Local to the partition/thread
    KPTR_PROCESS = 1,
    
    /// Local to the machine or set of threads
    KPTR_MACHINE = 2,
    
    /// Global to the cluster
    KPTR_NETWORK = 3 
};

///
/// @struct UTIL_KptrEntry
///
/// @brief A wrapper for the name service pointer that includes
/// both the object and its scope
///

struct UTIL_KptrEntry
{
    void* obj;
    UTIL_KptrScope protect;
} ;

///
/// @namespace UTIL
/// @brief Utility namespace
///
namespace UTIL
{
    
    /// 
    /// @class Lockable
    ///
    /// @brief A general class that can be locked using the
    /// QualNet Thread Mutex
    ///
    /// @see class QNThreadMutex
    ///
    
    class Lockable 
    {
    protected:
        /// A thread mutex to be inherited
        QNThreadMutex m_mutex;
        
    public:
        
        ///
        /// @brief Default(empty) constructor
        ///
        
        Lockable()
        {
            
        }
        
        ///
        /// @brief Class destructor
        ///
        
        ~Lockable()
        {
            
        }
        
        ///
        /// @brief Return a pointer to the class mutex
        ///
        /// @returns a pointer to the class mutex
        ///
        
        QNThreadMutex* getMutex()
        {
            return &m_mutex;
        }
    } ;
        
}


//
// Header declarations
//

void UTIL_NameServiceReady(void)  ;

void UTIL_NameServiceInit(void) ;

void UTIL_NameServicInitLocal(Node *node) ;

void UTIL_NameServiceEpoch(void) ;

void UTIL_NameServiceEpochLocal(Node *node) ;

BOOL UTIL_NameServicePutImmutable(std::string key, 
                                  void* obj, 
                                  Node* node);

BOOL UTIL_NameServicePutImmutable(std::string key, 
                                  void* obj) ;

BOOL UTIL_NameServicePutImmutable(std::string key, 
                                  void* obj, 
                                  Node* node, 
                                  int ifidx) ;

void* UTIL_NameServiceRemove(std::string key) ;

void* UTIL_NameServiceRemove(std::string key, 
                             Node* node) ;

void* UTIL_NameServiceRemove(std::string key, 
                             Node* node, 
                             int ifidx) ;

void* UTIL_NameServiceGetImmutable(std::string key) ;

void* UTIL_NameServiceGetImmutable(std::string key, 
                                   BOOL weak) ;

void* UTIL_NameServiceGetImmutable(std::string key, 
                                   BOOL weak, 
                                   Node* node) ;

void* UTIL_NameServiceGetImmutable(std::string key, 
                                   BOOL weak, 
                                   Node* node, 
                                   int ifidx) ;

Node* UTIL_NameServiceLookupNode(int nodeID) ;

void UTIL_NameServiceDeregisterNode(Node* node) ;

void UTIL_NameServiceRegisterNode(Node* node) ;

void UTIL_KptrAlloc(Node* node, 
                    std::string attr, 
                    void* data, 
                    int type) ;

void* UTIL_KptrTake(Node* node, 
                    std::string attr) ;

void UTIL_KptrGive(Node* node, 
                   std::string attr, 
                   void* data) ;

void UTIL_KptrFree(Node* node, 
                   std::string attr) ;



#endif /* __UTIL_NAMESERVER_H__ */
