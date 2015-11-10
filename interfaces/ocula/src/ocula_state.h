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

#ifndef OCULA_STATE_H_
#define OCULA_STATE_H_

#include <string>
#include <vector>
#include <map>
#include <list>

// Commands to pause/play the simulator
// Sent as single byte commands
#define PAUSE_COMMAND 0
#define PLAY_COMMAND 1
#define HITL_COMMAND 2

// /**
// CALLBACK    :: OculaCallback
// DESCRIPTION :: Implements a callback function for changing state.  The
//                key parameter is the state that has changed.  The oldVal
//                parameter will be NULL the first time the callback is
//                invoked.  The newVal parameter will be NULL if the key is
//                removed from the state.  Otherwise the oldVal parameter
//                will contain the previous value of the key and the newVal
//                parameter will contain the new value.
// **/
typedef void (*OculaCallback)(const char* key, const char* oldVal, const char* newVal, void** userData);

struct OculaProperty
{
    std::string m_value;
    long long m_lastUpdate;
    std::map<OculaCallback, void*> m_callbackUserData;

    OculaProperty() : m_lastUpdate(-1) {}

    OculaProperty& operator= (const OculaProperty& rhs)
    {
        m_value = rhs.m_value;
        m_lastUpdate = rhs.m_lastUpdate;
        return *this;
    }
};

// /**
// CLASS       :: OculaState
// DESCRIPTION :: Implements the Ocula state class.  This class is shared by
//                the simulator (SOPS) and the visualizer (VOPS).
// **/
class OculaState
{
public:
    OculaState();

    // Set the value for a given key.  For example, key = "/node/1/hostname"
    // and val = "host1".  This function will never fail.
    void setProperty(const std::string& key, const std::string& val);

    // Remove the property for a key.  Returns true if the key is valid, false
    // if the key is invalid.
    bool deleteProperty(const std::string& key);

    // Get the property for a key.  For a valid key, the value is returned in
    // the val parameter and getProperty returns true.  For an invalid key,
    // val is set to an empty string and setProperty returns false.
    bool getProperty(const std::string& key, OculaProperty** val);

    // Register a callback for a key.  The callback will be invoked every
    // time the value changes.  This function returns true if key is valid
    // and returns false if key is invalid.  The callback will be invoked
    // once and then removed if the property is deleted.
    bool registerCallback(const std::string& key, OculaCallback callback);

    // Remove a callback.  Returns true if the callback was remove, false if
    // it could not be removed.
    bool removeCallback(const std::string& key, OculaCallback callback);

    // Split a string based on a delimiter
    static void StringSplit(
        const std::string& str,
        const std::string& delim,
        std::vector<std::string>& out);
    
    // Return a segment of a string or empty string if not found
    // getSegment("/abc/def/ghi", 1) returns "def"
    static std::string getSegment(const std::string& key, int index);

    std::map<std::string, OculaProperty>::iterator begin() { return m_state.begin(); }
    std::map<std::string, OculaProperty>::iterator end() { return m_state.end(); }
private:
    bool m_locked;

    std::map<std::string, OculaProperty> m_state;

    std::map<std::string, OculaCallback> m_callbacks;

    struct DelayedCallback
    {
        std::string key;
        OculaProperty* prop;
        OculaCallback callback;
    };

    std::list<DelayedCallback> m_lockedCallbacks;

    static bool callbackMatches(const std::string& key, const std::string callback);
};

#endif /* OCULA_STATE_H_ */
