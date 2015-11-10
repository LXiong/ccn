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
// use in compliance with the license agreement as part of the Qualnet
// software.  This source code and certain of the algorithms contained
// within it are confidential trade secrets of Scalable Network
// Technologies, Inc. and may not be used as the basis for any other
// software, hardware, product or service.

#ifndef _VOPS_PROPERTY_H_
#define _VOPS_PROPERTY_H_

#include "Property.h"
#include "pthread.h"
#include <map>

class VopsRpcInterface;

// /**
// CLASS       :: VopsProperty 
// DESCRIPTION :: Derived from Property to contain the specific methods
//                and data for the Visualizer Object Property Store.
// **/
class VopsProperty : public Property
{
public:
    VopsProperty(const std::string& key, const std::string& value) :
      Property(key, value)
      {
          pthread_rwlock_init(&m_rwlock, NULL);
      }

    // Adds a callback pointer to the callbacks.
    void addCallback(VisualizerCallback callback);

    // Removes a callback pointer from the callbacks.
    void removeCallback(VisualizerCallback callback);

    // getCallback(i) returns the ith callback. If i is out of range,
    // returns NULL.
    VisualizerCallback getCallback(unsigned int i);

    // nCallbacks returns the number of callbacks for this property
    int nCallbacks() {return m_callbacks.size();}

    // Override the getValue function to make it thread-safe.
    std::string getValue();

    // Override the setValue function to make it thread-safe.
    void setValue(const std::string& value);

private:
    // The property has zero or more function pointers to the functions in 
    // the visualizer that it will call when the value changes.
    std::vector<VisualizerCallback> m_callbacks;

    // Since a separate thread in the visualizer may call for property values
    // to be sent via callbacks, or may read property values directly, this
    // rwlock provides thread-safe operations on the value string. It is
    // also used to lock the callback list.
    pthread_rwlock_t m_rwlock;

};


// /**
// STRUCT      :: CallbackRegistryEntry
// DESCRIPTION :: Associates a wildcarded property key with a callback.
//                The wilcard used is the * character. For example, the 
//                callback key may be "/partition/*/numberOfNodes". This
//                matches "/partition/0/numberOfNodes", 
//                "/partition/1/numberOfNodes", etc. 
// **/
struct CallbackRegistryEntry
{
    std::string   m_wildkey;
    VisualizerCallback m_callback;
};

// /**
// STRUCT      :: CallbackQueueEntry
// DESCRIPTION :: Since the visualizer is presumed non-thread-safe, the 
//                callbacks are scheduled in a queue. The queue is sent 
//                when the visualizer calls 
//                VopsPropertyManager::executeCallbacks.
// **/
struct CallbackQueueEntry
{
    VisualizerCallback m_callback;
    std::string   m_key;
    std::string   m_vold;
    std::string   m_vnew;
    CallbackQueueEntry(
        VisualizerCallback callback, 
        const std::string& key, 
        const std::string& vold,
        const std::string& vnew) :
    m_callback(callback),
        m_key(key),
        m_vold(vold),
        m_vnew(vnew)
    {}
};

// /**
// CLASS       :: VopsPropertyManager
// DESCRIPTION :: The VOPS property manager extends the PropertyManager to
//                provide the capability to register visualizer callbacks and 
//                to call the registered functions when the property value is
//                changed.
// **/
class VopsPropertyManager : public PropertyManager
{
public:
    // External access to properties iterators
    PropertyManagerIterator begin() { return m_properties.begin(); }
    PropertyManagerIterator end() { return m_properties.end(); }

    // The deleteProperty method tries to find the property for key.
    // NOT FOUND:
    //    - Returns false. 
    // FOUND:
    //    - Calls each registered callback for the property using NULL
    //      for the new value to indicate the property is deleted.
    //    - Deletes the property entry.
    //    - Returns true.
    bool deleteProperty(const std::string& key);

    // When the visualizer is ready for callbacks, it invokes this method.
    // It must do this periodically to get the latest updates to the state.
    void executeCallbacks();

    // Singleton instance access
    static VopsPropertyManager* getInstance();

    // Destroy singleton instance
    static void destroyInstance();

    // Overrides base class to read-lock map access.
    bool getProperty(const std::string& key, Property** prop);

    // Status of connection to SOPS
    bool isConnected();

    // Getter for the VopsRpcInterface
    VopsRpcInterface *getInterface() {return m_sops;}

    // Needed for the tester program to access the properties in 
    // a thread safe manner
    pthread_rwlock_t* getLock() {return &m_rwlock_properties;}

    // Register a callback for a key. The callback will be stored in the
    // callback registry, then every existing property key will be compared.
    // For each matching property/registry pair, the callback is added to the
    // property. The callback will remain in the registry and will be applied 
    // to any new property that is added with a matching key.
    bool registerCallback(
        const std::string& wildkey, VisualizerCallback callback);

    // Remove a callback. Returns true if the callback was successfully removed,
    // false if it could not be removed.
    bool removeCallback(
        const std::string& wildkey, VisualizerCallback callback);
    
    // The setProperty method tries to find the property for key. 
    // NOT FOUND: 
    //    - Create the new property.
    //    - Call addCallback for each matching key in the registry.
    //    - Call each callback using NULL for the old value to indicate 
    //      that this is a new property.
    // FOUND EXISTING PROPERTY:
    //    - Saves the old value of the property.
    //    - Updates the property value.
    //    - Calls each callback in the property's callback list using both 
    //      oldval and newval. 
    void setProperty(const std::string& key, const std::string& value);

    // getter for number of properties
    unsigned int size() { return m_properties.size(); }

    void setOculaPort(int port);
    int oculaPort();

    void setHostAddress(std::string);
    std::string getHostAddress();
    
private:
    // Singleton instance pointer
    static VopsPropertyManager* single;

    VopsPropertyManager() : 
    PropertyManager(), 
        m_sops(NULL)
    {
        m_locked = true;
        pthread_mutex_init(&m_queue_mutex, NULL);
        pthread_rwlock_init(&m_rwlock_properties, NULL);
        pthread_rwlock_init(&m_rwlock_registry, NULL);
        initialize();
    }

    virtual ~VopsPropertyManager();

    // Schedule a callback due to a property change
    void addToCallbacks(CallbackQueueEntry* cbqe);

    // initialize creates the VopsRpcInterface and connects to the Sops.
    void initialize();

    // The list of all the callbacks that have been registered.
    std::vector<CallbackRegistryEntry> m_registry;

    VopsRpcInterface* m_sops; // Interface to Sops

    // This map holds updates for the visualizer that have been 
    // delayed for the visualizer to indicate it is ready.
    std::map<std::string, CallbackQueueEntry*> m_callbackMap;

    // If the state is locked by the simulator
    volatile bool m_locked;

    // This mutex is used to make the callback queue safe for mutiple threads.
    pthread_mutex_t m_queue_mutex;

    // This read/write lock is used to protect the properties map from being
    // updated by insert() or erase() in the RPC thread during a find()
    pthread_rwlock_t m_rwlock_properties;

    // Likewise the callback registry could be modified by the visualizer while
    // the VOPS thread needs to iterate through it.
    pthread_rwlock_t m_rwlock_registry;
};
#endif // #ifndef _VOPS_PROPERTY_H_
