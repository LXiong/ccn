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

#ifndef _SOPS_PROPERTY_H_
#define _SOPS_PROPERTY_H_

#include "Property.h"
#include "partition.h"
#include "WallClock.h"
#include "RpcInterface.h"

class SopsRpcInterface;
class SPConnection;

// /**
// CLASS       :: SopsProperty 
// DESCRIPTION :: Derived from Property to contain the specific methods
//                and data for the Simulator Object Property Store.
// **/
class SopsProperty : public Property
{
public:
    SopsProperty(const std::string& key, const std::string& value) :
        Property(key, value), 
        m_maxwait(DEFAULT_MAXWAIT), m_quantum (0.0), 
        m_lastSentValue(""), m_lastSentTime((realtime_t)0)
        {
            pthread_rwlock_init(&m_rwlock, NULL);
        }

    // Override the getValue function to make it thread-safe.
    std::string getValue();

    // isReadyToSend applies criteria to the difference between 
    // the current value and the last sent value. It returns
    // true when the value should be sent to update the visualizer.
    bool isReadyToSend();

    // The manager notifies the Property that it was sent by calling 
    // the sent() function so it can update the last sent value and time.
    void sent() 
    {
        m_lastSentValue = m_value; 
        m_lastSentTime = OPS_GetRealtime();
    }

    // Setter for the maxwait value in milliseconds. If the value changes 
    // less than the quantum it will still be "ready to send" if the max
    // wait time has elapsed since it was last sent.
    void setMaxwait(long max_millisecs) {m_maxwait = max_millisecs * MS_TO_CLOCKS;}

    // Setter for the quantum value. The value must change by this
    // amount to be considered significant enough to send.
    void setQuantum(double quantum) {m_quantum = quantum;}

    // Override the setValue function to make it thread-safe.
    void setValue(const std::string& value);

private:

    realtime_t m_maxwait;  // maximum clock ticks to delay sending if the
                        // value hasn't changed by quantum.

    double m_quantum;   // minimum change to the property value required
                        // to trigger conditional sending to the visualizer

    std::string m_lastSentValue; // This is the last value sent to Vops

    realtime_t m_lastSentTime;  // Time in clocks last value was sent.

    // delta returns the amount of change between m_value and m_lastSent.
    // If the strings contain non-numeric, the delta is either 0.0 (no
    // change) or 1.0 (changed). For numeric strings, delta returns
    // abs(OPS_ToNumber(m_value) - OPS_ToNumber(m_lastSent)). If a string has
    // numeric values separated by commas, e.g., "18.5, -12.91, 0.0",
    // it will return the hypotenuse of the deltas between the numbers.
    double delta();

    // For multithreaded processing the property value needs a lock.
    pthread_rwlock_t m_rwlock;
};

// /**
// STRUCT      :: SopsMaxwait
// DESCRIPTION :: Contains a key string, that may be wildcarded, and the
//                max wait time in millisecs to be applied to any property 
//                that matches the key.
// **/
struct SopsMaxwait
{
    std::string    wildkey;
    realtime_t     maxwait;
};


// /**
// STRUCT      :: SopsQuantum
// DESCRIPTION :: Contains a key string, that may be wildcarded, and the
//                quantum value to be applied to any property that matches
//                the key.
// **/
struct SopsQuantum
{
    std::string    wildkey;
    double         quantum;
};


// /**
// CLASS       :: SopsPropertyManager
// DESCRIPTION :: The SOPS property manager extends the PropertyManager to
//                provide the capability to determine when to send updates
//                and to send to VOPS using a SopsRpcInterface object.
// **/
class SopsPropertyManager : public PropertyManager
{
public:
    // Singleton
    static SopsPropertyManager* getInstance();

    // getters
    int getLastErrno() {return m_lastErrno;}

    // The deleteProperty method tries to find the property. If it does it
    // deletes it and, if Running, notifies VOPS that the property is deleted. 
    // Otherwise it returns false.
    bool deleteProperty(const std::string& key);

    // run allows the state to be checked to see if Vops is connected.
    void run() {if (m_state != Running) PropertyManager::run();}

    // sendAll is to init vops with all scripts and all properties.
    void sendAll(SPConnection *spc) {run(); sendAllProperties(spc);}

    // sendAllProperties() loops through m_properties and sends 
    // each property to one connection. 
    int sendAllProperties(SPConnection *spc);

    // The setProperty method tries to find the property for key. If it is 
    // not found, the property is created and added to the map. The value 
    // attribute is set to value. If it was created it is sent. Otherwise, 
    // if isReadyToSend() returns true sendOneProperty() sends it.
    void setProperty(
        const std::string& key, const std::string& value);

    // Set the maximum wait time after which the property will be sent if it
    // changes even if the change is not significant. May be wildcarded.
    void setMaxwait(const std::string& wildkey, realtime_t max_millisecs);

    // Set the minimum amount of change to a property for it to be sent.
    // Returns false if no matching property is found. May be wildcarded.
    void setQuantum(const std::string& wildkey, double quantum);

    // initialize starts the RPC interface.
    void initialize();
    void setPartition(PartitionData* partition);
    PartitionData* getPartition() {return m_partition;}
    SopsRpcInterface* getInterface() {return m_vops;}
    void setScenarioName(std::string name) {m_scenarioName = name;}
    std::string getScenarioName() {return m_scenarioName;}

private:
    SopsPropertyManager() : 
        PropertyManager(), 
        m_lastErrno(0),
		m_partition(NULL), 
        m_vops(NULL) 
    {   
        pthread_rwlock_init(&m_rwlock_properties, NULL);
        initialize();
    }

    static SopsPropertyManager *m_single; // Singleton instance pointer.

    PartitionData* m_partition;

    int m_lastErrno;           // The saved result from the last send.

    SopsRpcInterface *m_vops;  // Provides remote procedure calls in Vops.

    std::vector<SopsMaxwait>   m_maxwaits; 

    std::vector<SopsQuantum>   m_quantums;

    pthread_rwlock_t m_rwlock_properties;

    // Sends one property to all subscribed connections
    int sendOneProperty(SopsProperty *prop);

    std::string m_scenarioName;
};

#endif // #ifndef _SOPS_PROPERTY_H_
