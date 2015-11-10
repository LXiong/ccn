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

#ifndef _PROPERTY_H_
#define _PROPERTY_H_

#include <string>
#include <vector>
#include <assert.h>
#include "ops_util.h"

#include <boost/unordered_map.hpp>

// Windows and XSI-Compliant systems (Linux) use a different value for 
// CLOCKS_PER_SECOND. Property times are stored in clocks, but all 
// the API uses milliseconds. This constant converts MS to CLOCKS.
const realtime_t MS_TO_CLOCKS = CLOCKS_PER_SEC / 1000;

// This is the default max time for all properties in Sops.
// Currently 1 second (1000 milliseconds).
const realtime_t DEFAULT_MAXWAIT = 1000 * MS_TO_CLOCKS;

// /**
// CLASS       :: Property 
// DESCRIPTION :: A property consists of a key string and a value. The key
//                uses path style notation, e.g., /partition/0/numberOfNodes.
//                Values are strings. This is a base class with common property
//                methods and data.
// **/
class Property
{
public:
    // This constructor sets the key and the current value.
    Property(const std::string& key, const std::string& value) :
        m_key(key), m_value(value) {}

    // Getter for key
    virtual std::string& getKey() {return m_key;}

    // Getter for property value
    virtual std::string getValue() {return m_value;}

    // Setter for property value
    virtual void setValue(const std::string& value)
        {m_value = value;}

protected:
    std::string    m_key;         // path style hierarchical property name
    std::string    m_value;       // the current value of the property
};


// /**
// CLASS       :: PropertyManager
// DESCRIPTION :: The property manager maintains an associative container of 
//                Property objects indexed by the property Key.
// **/
class PropertyManager
{
public:
    enum ManagerState {Inactive, Initialized, Running};

    PropertyManager() : m_state(Inactive) {}

    virtual ~PropertyManager() {}

    // This function may be overloaded to add initialization actions.  
    virtual void initialize()
    {
        assert(m_state == Inactive);
        m_state = Initialized;
    }

    // This function may be overloaded to add actions to start running. 
    virtual void run()
    {
        assert(m_state == Initialized);
        m_state = Running;
    }

    // The deleteProperty function should try to find the property. 
    // If found it deletes it and returns true. Otherwise, it returns false.
    virtual bool deleteProperty(const std::string& key) = 0;
    
    // Getter for property. If the key is not found, returns false.
    // Otherwise sets the value and returns true.
    virtual bool getProperty(const std::string& key, Property** prop)
    {
        PropertyManagerIterator it = m_properties.find(key);
        if (it == m_properties.end())
        {
            // Property was not found.
            *prop = NULL;
            return false;
        }
        *prop = it->second;
        return true;
    }

    // Getter for manager state
    ManagerState getState() {return m_state;}

    // The setProperty method should try to find the property for key. 
    // If it is not found, the property is created and added to the map. 
    // Otherwise the value is set on the existing property.
    virtual void setProperty(
        const std::string& key, const std::string& value) = 0;

    typedef boost::unordered_map<std::string, Property*>::iterator 
        PropertyManagerIterator;

protected:
    boost::unordered_map<std::string, Property*> m_properties;
   
    ManagerState m_state;
};





// /**
// CALLBACK    :: VisualizerCallback
// DESCRIPTION :: Implements a callback function for changing states. The
//                key parameter is the state that has changed.  The old
//                parameter will be NULL the first time the callback is
//                invoked.  The new parameter will be NULL if the key is
//                removed from the state.  Otherwise the old parameter
//                will contain the previous value of the key and the new
//                parameter will contain the new value.
// **/
typedef void (*VisualizerCallback)
    (const char* key, const char* oldval, const char* newval, void** userData);

#endif // #ifndef _PROPERTY_H_
