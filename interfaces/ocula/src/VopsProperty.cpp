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

#include <errno.h>
#include <limits>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "VopsProperty.h"
#include "ops_util.h"
#include "VopsRpcInterface.h"
#include "ExecSettings.h"

#ifdef VOPS_DEBUG
#include <stdarg.h>
FILE *vopslog = NULL;
void VopsLog(const char *printstr, ...)
{
    va_list args;
    va_start(args, printstr);
    if (!vopslog)
    {
        vopslog = fopen("vops.log","w");
    }
    vfprintf(vopslog, printstr, args);
    fflush(vopslog);
    va_end(args);
}
#endif

//
// FUNCTION    :: dummyCallback
// DESCRIPTION :: This function outputs the key, oldval, and newval
//                parameters for test & debug use. It can be registered for 
//                all keys.
//
void dummyCallback
    (const char* key, const char* oldval, const char* newval, void** userData)
{
    printf("Callback: key=%s, oldval=%s, newval=%s\n",
        key,
        (oldval) ? oldval : "NULL", 
        (newval) ? newval : "NULL");
}

//
// FUNCTION    :: VopsProperty::addCallback
// DESCRIPTION :: Adds an VisualizerCallback to the callbacks associated with 
//                this property. Then calls the callback with NULL as
//                the old value.
//
void VopsProperty::addCallback(VisualizerCallback callback)
{
    m_callbacks.push_back(callback);
}

//
// FUNCTION    :: VopsProperty::removeCallback
// DESCRIPTION :: Removes an VisualizerCallback from the callbacks associated 
//                with this property.
//
void VopsProperty::removeCallback(VisualizerCallback callback)
{
    // Look for matching callbacks in the registry.
    std::vector<VisualizerCallback>::iterator re;
    pthread_rwlock_wrlock(&m_rwlock);
    for (re = m_callbacks.begin(); re != m_callbacks.end(); re++)
    {
        if (callback == *re)
        {
            m_callbacks.erase(re);
            break;
        }
    }
    pthread_rwlock_unlock(&m_rwlock);
}

//
// FUNCTION    :: VopsProperty::getCallback
// DESCRIPTION :: Gets the ith callback for the property. Returns a NULL
//                if that callback doesn't exist. See nCallbacks().
//
VisualizerCallback VopsProperty::getCallback(unsigned int i)
{
    if (i >= m_callbacks.size())
    {
        return NULL;
    }
    return m_callbacks[i];
}

//
// FUNCTION    :: VopsProperty::getValue
// DESCRIPTION :: Overrides the base function to add thread-safety.
//
std::string VopsProperty::getValue()
{
    pthread_rwlock_rdlock(&m_rwlock);
    std::string value = Property::getValue();
    pthread_rwlock_unlock(&m_rwlock);
    return value;
}

//
// FUNCTION    :: VopsProperty::setValue
// DESCRIPTION :: Overrides the base function to add thread-safety.
//
void VopsProperty::setValue(const std::string& value)
{
    pthread_rwlock_wrlock(&m_rwlock);
    Property::setValue(value);
    pthread_rwlock_unlock(&m_rwlock);
}

//
// POINTER    :: SopsPropertyManager* single
// DESCRIPTION:: Pointer to the instantiation object.
//
VopsPropertyManager* VopsPropertyManager::single = NULL;

//
// FUNCTION    :: ~VopsPropertyManager()
// DESCRIPTION :: Destructor.
//
VopsPropertyManager::~VopsPropertyManager()
{
    delete m_sops;
}

//
// FUNCTION    :: SopsPropertyManager *getInstance()
// DESCRIPTION :: Implement singleton pattern by returning the pointer after
//                the single instantiation has been made.
//
VopsPropertyManager *VopsPropertyManager::getInstance()
{
    if (single == NULL)
    {
        single = new VopsPropertyManager;
    }
    return single;
}

//
// FUNCTION    :: destroyInstance()
// DESCRIPTION :: Destroy instance of singleton pattern
//
void VopsPropertyManager::destroyInstance()
{
    if (single != NULL)
    {
        delete single;
        single = NULL;
    }
}

bool VopsPropertyManager::isConnected() {return m_sops->isConnected();}

//
// FUNCTION    :: VopsPropertyManager::deleteProperty
// DESCRIPTION :: If the property is not found, returns false.
//
//                Otherwise, calls all the callbacks with newval
//                set NULL, removes the property from the map,
//                deletes the object, and returns true.
//
bool VopsPropertyManager::deleteProperty(const std::string& key)
{
#ifdef VOPS_DEBUG
    VopsLog("VopsPropertyManager::deleteProperty(%s)\n", key.c_str());
#endif
    // Needs a read lock to do a find
    pthread_rwlock_rdlock(&m_rwlock_properties);
    PropertyManagerIterator it = m_properties.find(key);
    pthread_rwlock_unlock(&m_rwlock_properties);
    if (it == m_properties.end())
    {
        // property not found:
        return false;
    }

    VopsProperty *prop = (VopsProperty*)it->second;
    std::string oldval = prop->getValue();

    // Call all the callbacks to indicate the property is removed
    if (prop->nCallbacks() > 0)
    {
        pthread_mutex_lock(&m_queue_mutex);
        for (int i = 0; i < prop->nCallbacks(); i++)
        {
            CallbackQueueEntry *cbqe = new CallbackQueueEntry(
                    prop->getCallback(i), prop->getKey(), oldval, std::string());
            addToCallbacks(cbqe);
        }
        pthread_mutex_unlock(&m_queue_mutex);
    }

    // This is where the write lock is required
    pthread_rwlock_wrlock(&m_rwlock_properties);
    m_properties.erase(it);
    pthread_rwlock_unlock(&m_rwlock_properties);
    delete prop;
    return true;
}

//
// FUNCTION    :: VopsPropertyManager::executeCallbacks
// DESCRIPTION :: Executes the queued callbacks. 
//
void VopsPropertyManager::executeCallbacks()
{
    const char *newstr;
    const char *oldstr;
    CallbackQueueEntry *cbqe;

#ifdef VOPS_EXTRA_DEBUG
    VopsLog("VopsPropertyManager::executeCallbacks() with %d in map.\n", 
        m_callbackMap.size());
#endif

    if (m_callbackMap.empty() || m_locked)
    {
        return;
    }

    pthread_mutex_lock(&m_queue_mutex);
    // First send all partition level properties
    std::map<std::string, CallbackQueueEntry*>::iterator it;
    it = m_callbackMap.lower_bound("/partition");
    while (it != m_callbackMap.end() && it->first.find_first_of("/partition") == 0)
    {
        cbqe = it->second;

        // Prepare the char pointer versions of the strings.
        if (cbqe->m_vold.empty())
        {
            oldstr = NULL;
        }
        else
        {
            oldstr = cbqe->m_vold.c_str();
        }

        if (cbqe->m_vnew.empty())
        {
            newstr = NULL;
        }
        else
        {
            newstr = cbqe->m_vnew.c_str();
        }

        // Call the callback function
        (*cbqe->m_callback)(cbqe->m_key.c_str(), oldstr, newstr, NULL);

        // mutex needed to update to the queue structure
        delete cbqe;
        it->second = NULL;
        ++it;
    }

    // Send the entire queue
    while (!m_callbackMap.empty())
    {
        // reading the front is safe since this is a FIFO queue.
        it = m_callbackMap.begin();
        cbqe = it->second;

        if (cbqe == NULL)
        {
            // Partition event
            m_callbackMap.erase(it->first);
            continue;
        }

        // Prepare the char pointer versions of the strings.
        if (cbqe->m_vold.empty())
        {
            oldstr = NULL;
        }
        else
        {
            oldstr = cbqe->m_vold.c_str();
        }

        if (cbqe->m_vnew.empty())
        {
            newstr = NULL;
        }
        else
        {
            newstr = cbqe->m_vnew.c_str();
        }

        // Call the callback function
        (*cbqe->m_callback)(cbqe->m_key.c_str(), oldstr, newstr, NULL);

        // mutex needed to update to the queue structure
        m_callbackMap.erase(it->first);
        delete cbqe;
    }
    pthread_mutex_unlock(&m_queue_mutex);
}

//
// FUNCTION    :: getProperty
// DESCRIPTION :: Tread safe overload of base class getProperty
//
bool VopsPropertyManager::getProperty(
    const std::string& key, Property** prop)
{
    pthread_rwlock_rdlock(&m_rwlock_properties);
    bool res = PropertyManager::getProperty(key, prop);
    pthread_rwlock_unlock(&m_rwlock_properties);
    return res;
}

//
// FUNCTION    :: VopsPropertyManager::initialize
// DESCRIPTION :: Creates the RPC Interface to get calls from Sops. 
//
void VopsPropertyManager::initialize()
{
    PropertyManager::initialize();
    m_sops = new VopsRpcInterface(DEFAULT_OCULA_PORT, this);
    run();
}

void VopsPropertyManager::setOculaPort(int port)
{
    if (m_sops == NULL) 
    {
        m_sops = new VopsRpcInterface(port, this);
    }
    else 
    {
        m_sops->m_portno = port;
    }
}

int VopsPropertyManager::oculaPort()
{
    if (m_sops == NULL) 
    {
        m_sops = new VopsRpcInterface(DEFAULT_OCULA_PORT, this);
    }
    return m_sops->m_portno;
}

void VopsPropertyManager::setHostAddress(std::string hostAddress)
{
    if (m_sops == NULL) 
    {
        m_sops = new VopsRpcInterface(DEFAULT_OCULA_PORT, this);
    }
    m_sops->setHostAddress(hostAddress);
}

std::string VopsPropertyManager::getHostAddress()
{
    if (m_sops == NULL) 
    {
        m_sops = new VopsRpcInterface(DEFAULT_OCULA_PORT, this);
    }
    return m_sops->getHostAddress();
}

//
// FUNCTION    :: VopsPropertyManager::addToCallbacks
// DESCRIPTION :: Add a property to the list of properties that has changed
//
void VopsPropertyManager::addToCallbacks(CallbackQueueEntry* cbqe)
{
    // Check if empty or if over-writing old value
    std::map<std::string, CallbackQueueEntry*>::iterator it =
        m_callbackMap.find(cbqe->m_key);
    if (it == m_callbackMap.end())
    {
        // New entry
        m_callbackMap[cbqe->m_key] = cbqe;
    }
    else
    {
        // Over-write existing entry
        cbqe->m_vold = it->second->m_vold;
        delete it->second;
        it->second = cbqe;
    }
}

//
// FUNCTION    :: VopsPropertyManager::registerCallback
// DESCRIPTION :: Adds the callback to the registry for use with future new 
//                properties. Finds all properties that match the wildcard 
//                key, adds the callback to the property, and then calls it.
//                
//
bool VopsPropertyManager::registerCallback(
    const std::string& wildkey, VisualizerCallback callback)
{
    // Add the callback to the registry
    CallbackRegistryEntry entry;
    entry.m_wildkey = wildkey;
    entry.m_callback = callback;
    pthread_rwlock_wrlock(&m_rwlock_registry);
    m_registry.push_back(entry);
    pthread_rwlock_unlock(&m_rwlock_registry);

    // Look for matching properties
    PropertyManagerIterator it;
    // Have to read lock to iterate through the properties
    pthread_rwlock_rdlock(&m_rwlock_properties);
    for (it = m_properties.begin(); it != m_properties.end(); it++)
    {
        VopsProperty *prop = (VopsProperty*)it->second;
        if (OPS_WildcardMatch(prop->getKey(), wildkey))
        {
            // Add the callback to the property and call it
            prop->addCallback(callback);
            CallbackQueueEntry *cbqe = 
                new CallbackQueueEntry(
                    callback, prop->getKey(), std::string(), prop->getValue());
            pthread_mutex_lock(&m_queue_mutex);
            addToCallbacks(cbqe);
            pthread_mutex_unlock(&m_queue_mutex);
        }
    }
    pthread_rwlock_unlock(&m_rwlock_properties);
#ifdef VOPS_DEBUG
    VopsLog("RegisterCallback for key \"%s\"\n",
        wildkey.c_str());
#endif
    return true;
}

//
// FUNCTION    :: VopsPropertyManager::removeCallback
// DESCRIPTION :: Removes the callback from the registry. 
//                Finds all properties that match the wildcard 
//                key, removes the callback from them as well.
//                
//
bool VopsPropertyManager::removeCallback(
    const std::string& wildkey, VisualizerCallback callback)
{
   // Look for matching properties
    PropertyManagerIterator it;
    // Have to read lock to iterate through the properties
    pthread_rwlock_rdlock(&m_rwlock_properties);
    for (it = m_properties.begin(); it != m_properties.end(); it++)
    {
        VopsProperty *prop = (VopsProperty*)it->second;
        if (OPS_WildcardMatch(prop->getKey(), wildkey))
        {
            // Add the callback to the property and call it
            prop->removeCallback(callback);
        }
    } 
    pthread_rwlock_unlock(&m_rwlock_properties);

    // Look for matching callbacks in the registry.
    std::vector<CallbackRegistryEntry>::iterator re;
    pthread_rwlock_wrlock(&m_rwlock_registry);
    for (re = m_registry.begin(); re != m_registry.end(); re++)
    {
        if (!wildkey.compare(re->m_wildkey))
        {
            if (callback == re->m_callback)
            {
                // When a matching callback is found, remove it from the registry
                m_registry.erase(re);
            }
        }
    }
    pthread_rwlock_unlock(&m_rwlock_registry);
    return true;
}
//
// FUNCTION    :: VopsPropertyManager::setProperty
// DESCRIPTION :: If the property is not found, it is created. The callback
//                registry, which is a list of all callbacks and the associated
//                wildcard key value. Each callback that matches the key is
//                added to the property and called.
//
//                If it does exist, update the property value and call all
//                the callbacks.
//
void VopsPropertyManager::setProperty(
        const std::string& key, const std::string& value)
{
    int ret;
#ifdef VOPS_DEBUG
    VopsLog("VopsPropertyManager::setProperty(%s, %s)\n", 
        key.c_str(), value.c_str());
#endif
    // Need a read lock to do a find()
    ret = pthread_rwlock_rdlock(&m_rwlock_properties);
    // printf("  properties.find() pthread_rwlock_rdlock(properties) ret=%d\n", ret);
    PropertyManagerIterator it = m_properties.find(key);
    ret = pthread_rwlock_unlock(&m_rwlock_properties);
    // printf("  properties.find() pthread_rwlock_unlock(properties) ret=%d\n", ret);
    if (it == m_properties.end())
    {
        // property not found:
        VopsProperty *newprop = new VopsProperty(key, value);

        // Need a write lock to insert a new entry in the map.
        // printf("Getting write-lock on properties.\n");
        ret = pthread_rwlock_wrlock(&m_rwlock_properties);
        // printf(
        //     "  New property. pthread_rwlock_wrlock(properties), ret=%d\n", 
        //     ret);
        m_properties[key] = newprop;
        ret = pthread_rwlock_unlock(&m_rwlock_properties);
        // printf(
        //     "  Added property. pthread_rwlock_unlock(properties) ret=%d\n", 
        //     ret);

        // Look for matching callbacks in the registry.
        std::vector<CallbackRegistryEntry>::iterator re;
        ret = pthread_rwlock_rdlock(&m_rwlock_registry);
        // printf("  pthread_rwlock_rdlock(registry) ret=%d\n", ret);
        for (re = m_registry.begin(); re != m_registry.end(); re++)
        {
            if (OPS_WildcardMatch(key, re->m_wildkey))
            {
                // When a matching callback is oufnd, add it to the property
                // and call it with the new property value.
                newprop->addCallback(re->m_callback);
                CallbackQueueEntry *cbqe = new CallbackQueueEntry(
                    re->m_callback, newprop->getKey(), std::string(), 
                    newprop->getValue());
                ret = pthread_mutex_lock(&m_queue_mutex);
                addToCallbacks(cbqe);
                ret = pthread_mutex_unlock(&m_queue_mutex);
            }
        }
        ret = pthread_rwlock_unlock(&m_rwlock_registry);
        // printf("  pthread_rwlock_unlock(registry) ret=%d\n", ret);
    }
    else
    {
        // property exists; this is an update
        VopsProperty *prop = (VopsProperty*)it->second;
        // Save the old value for the callback
        std::string oldval = prop->getValue();
        // Store the new value
        prop->setValue(value);

        // Call all registered callback functions with old and new values.
        if (prop->nCallbacks() > 0)
        {
            ret = pthread_mutex_lock(&m_queue_mutex);
            for (int i = 0; i < prop->nCallbacks(); i++)
            {
                CallbackQueueEntry *cbqe = new CallbackQueueEntry(
                    prop->getCallback(i), key, oldval, value);
                addToCallbacks(cbqe);
            }
            ret = pthread_mutex_unlock(&m_queue_mutex);
        }
    }

    // Set locked to false if value is "0"
    if (key == "/locked")
    {
        m_locked = value != "0";
    }
}
