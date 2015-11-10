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
#include <stdlib.h>
#include "SopsProperty.h"
#include "SopsRpcInterface.h"
#include "SPConnection.h"
#include "ops_util.h"

#ifdef SOPS_DEBUG
#include <stdarg.h>
FILE *sopslog = NULL;
void SopsLog(const char *printstr, ...)
{
    va_list args;
    va_start(args, printstr);
    if (!sopslog)
    {
        sopslog = fopen("sops.log","w");
    }
    vfprintf(sopslog, printstr, args);
    fflush(sopslog);
    va_end(args);
}
#endif

// FUNCTION    :: SopsProperty::delta
// DESCRIPTION :: Returns the amount of change between m_value and m_lastSent.
//
//                If the strings contain non-numeric, the delta is either 0.0
//                (no change) or 1.0 (changed). 
//
//                For numeric strings, delta returns 
//                     abs(OPS_ToNumber(m_value) - OPS_ToNumber(m_lastSent)). 
//
//                If a string has numeric values separated by commas, 
//                e.g., "18.5, -12.91, 0.0", it will return the hypotenuse of 
//                the deltas between the numbers paired by location in the 
//                string.
//
double SopsProperty::delta()
{
    double delt = 0.0;

    // Short cut for identical strings
    pthread_rwlock_rdlock(&m_rwlock);
    std::string v = m_value;
    pthread_rwlock_unlock(&m_rwlock);

    if (v == m_lastSentValue)
    {
        return 0.0;
    }
    
    // Determine if the property is a comma delimited multi-value.
    if (v.find(',') != std::string::npos)
    {
        double currval;
        double lastval;
        double deltsq = 0.0;

        // Split the current and last sent values into an array of strings
        std::vector<std::string> curr;
        std::vector<std::string> last;
        OPS_SplitString(v, curr);
        OPS_SplitString(m_lastSentValue, last);
        std::vector<std::string>::iterator currIt = curr.begin();
        std::vector<std::string>::iterator lastIt = last.begin();

        // Get the hypotenuse of the deltas betwee each part of the values.
        for(; currIt != curr.end(); currIt++)
        {
            currval = OPS_ToNumber(*currIt);
            if (lastIt == last.end())
            {
                // There are more values in the current string, fill in 
                // missing last values with zeros
                lastval = 0.0;
            }
            else
            {
                lastval = OPS_ToNumber(*lastIt);
                lastIt++;
            }
            delt = currval - lastval;
            deltsq += delt * delt;
        }

        // All the value sin the current string have been used.
        // Are there more values in last string than current? 
        while (lastIt != last.end())
        {
            // If so, add them to the sum of the squares.
            lastval = OPS_ToNumber(*lastIt);
            deltsq += lastval * lastval;
            lastIt++;
        }
        // Get the sqrt of the sum of the squares.
        delt = sqrt(deltsq);
    }
    else 
    {
        // There is only one value.
        delt = abs(OPS_ToNumber(v) - OPS_ToNumber(m_lastSentValue));
    }

    // If the delta is zero, since at this point the current and last sent
    // strings are not identical, they must be non-numeric. Non-identical
    // non-numeric strings are represented by a delta of 1.0
    if (delt <= std::numeric_limits<double>::epsilon())
    {
        delt = 1.0;
    }

    return delt;
}

// Thread safe wrapper on base function
std::string SopsProperty::getValue()
{
    pthread_rwlock_rdlock(&m_rwlock);
    std::string value = Property::getValue();
    pthread_rwlock_unlock(&m_rwlock);
    return value;
}

// FUNCTION    :: SopsProperty::isReadyToSend
// DESCRIPTION :: If the time since sending the last update has passed
//                a threshold, returns true.
//
//                If the delta between the current value and the last
//                value sent to Vops is greater than or equal to the
//                quantum, returns true.
//
//                Otherwise returns false.
//
bool SopsProperty::isReadyToSend()
{
    // If the string is exactly the same, there's no need to update.
    pthread_rwlock_rdlock(&m_rwlock);
    std::string v = m_value;
    pthread_rwlock_unlock(&m_rwlock);

    if (v == m_lastSentValue)
    {
        return false;
    }

    // Check for significant change. First, if quantum is zero, it is ready.
    if (m_quantum <= std::numeric_limits<double>::epsilon())
    {
        return true;
    }

    // Check for time expiry
    if (OPS_GetRealtime() - m_lastSentTime > m_maxwait)
    {
        return true;
    }

    if (delta() >= m_quantum)
    {
        return true;
    }

    // It's too soon to send a non-significant change. 
    return false;
}

// Thread safe wrapper on base function
void SopsProperty::setValue(const std::string& value)
{
    pthread_rwlock_wrlock(&m_rwlock);
    Property::setValue(value);
    pthread_rwlock_unlock(&m_rwlock);
}

// POINTER      :: SopsPropertyManager* m_single
// DESCRIPTIONi :: Singleton pattern requires a definition of the static
//                 pointer to the single object.
//
SopsPropertyManager* SopsPropertyManager::m_single = NULL;

// FUNCTION    :: SopsPropertyManager *getInstance()
// DESCRIPTION :: Implement singleton pattern by returning the pointer after
//                the single instantiation has been made.
//
SopsPropertyManager *SopsPropertyManager::getInstance()
{
    if (m_single == NULL)
    {
        m_single = new SopsPropertyManager;
    }

    return m_single;
}

//
// FUNCTION    :: SopsPropertyManager::deleteProperty
// DESCRIPTION :: Deletes the property referenced by key if found
//                and returns true. If not found, just returns false.
//
bool SopsPropertyManager::deleteProperty(const std::string& key)
{
#ifdef SOPS_DEBUG
    SopsLog("SopsPropertyManager::deleteProperty(%s)\n", key.c_str());
#endif

    // Does the property to be deleted exist?
    pthread_rwlock_rdlock(&m_rwlock_properties);
    PropertyManagerIterator it = m_properties.find(key);
    pthread_rwlock_unlock(&m_rwlock_properties);

    if (it == m_properties.end())
    {
        // property not found:
        return false;
    }

    // First get the pointer to the Property.
    SopsProperty *prop = (SopsProperty*)it->second;

    // Remove it from the map
    pthread_rwlock_wrlock(&m_rwlock_properties);
    m_properties.erase(it);
    pthread_rwlock_unlock(&m_rwlock_properties);

    // Now delete the Property object.
    delete prop;

    // If the RpcInterface is not running yet, it doesn't matter that 
    // the deleteProperty RPC can't be called because so far the Vops has no 
    // properties.
    if (SPConnectionManager::getInstance()->getNumberOfSubscribers() < 1)
    {
#ifdef SOPS_DEBUG
        SopsLog("SopsProperty::deleteProperty - no Vops subscriptions.\n");
#endif
        m_lastErrno = ENOLINK;
        return true;
    }
    else
    {
        m_vops->deleteProperty(key);
        m_lastErrno = errno;
    }
    return true;
}

// setter for the partition pointer used by pause and play.
void SopsPropertyManager::setPartition(PartitionData* partition)
{
    this->m_partition = partition;
}

//
// FUNCTION    :: SopsPropertyManager::initialize
// DESCRIPTION :: Create the RPC interface.
//
void SopsPropertyManager::initialize()
{
    PropertyManager::initialize();
    // Note: OculaInterfaceEnabled() now returns the port number for SOPS.
    m_vops = new SopsRpcInterface(OculaInterfacePort(), this);
}

//
// FUNCTION    :: SopsPropertyManager::sendOneProperty
// DESCRIPTION :: If a connection has been made with the Vops RPC 
//                Interface, sends one property by means of calling 
//                m_vops->setProperty.
//
//                If there is no connection yet, the properties will 
//                all be sent when one is established.
//
int SopsPropertyManager::sendOneProperty(SopsProperty *prop)
{
    if (SPConnectionManager::getInstance()->getNumberOfSubscribers() < 1)
    {
        m_lastErrno = ENOLINK;
        return ENOLINK;
    }

    // Invoke the RPC
    m_vops->setProperty(prop->getKey(), prop->getValue());

    // Notify the property to update its lastSent data items
    prop->sent(); 

    m_lastErrno = errno;
    return errno;
}

//
// FUNCTION    :: SopsPropertyManager::sendAllProperties
// DESCRIPTION :: This is performed once for each subscription from Vops.
//
int SopsPropertyManager::sendAllProperties(SPConnection *spc)
{
    int cmdLen;
    PropertyManagerIterator it;
    SopsProperty *prop;
    int result = 0;
    char setCmd[2048];

#ifdef SOPS_DEBUG
    SopsLog("SopsPropertyManager::sendAllProperties(sock=%d)\n",
            spc->getSock());
#endif

    // Don't allow updates to m_properties during this loop that is called from
    // a SopsRpcInterface server thread in response to a new subscription.
    pthread_rwlock_rdlock(&m_rwlock_properties);

    for (it = m_properties.begin(); 
        it != m_properties.end(); it++)
    {
        // Don't send /locked until the end
        prop = (SopsProperty*)it->second;
        if (prop->getKey() == "/locked")
        {
            continue;
        }
        cmdLen = sprintf(setCmd, "S(%s=%s%c", 
            prop->getKey().c_str(), prop->getValue().c_str(), ETX);
        result = spc->send(setCmd, cmdLen);
        if (result < cmdLen)
        {
            // Something went wrong
#ifdef SOPS_DEBUG
            SopsLog("send failed to send %d bytes, result=%d, errno=%d\n",
                cmdLen, result, errno);
#endif
            return result;
        }
    }
    pthread_rwlock_unlock(&m_rwlock_properties);

    // Now send the /locked property 
    pthread_rwlock_rdlock(&m_rwlock_properties);
    it = m_properties.find("/locked");
    pthread_rwlock_unlock(&m_rwlock_properties);

    if (it != m_properties.end())
    {
        prop = (SopsProperty*)it->second;
        cmdLen = sprintf(setCmd, "S(%s=%s%c", 
            prop->getKey().c_str(), prop->getValue().c_str(), ETX);
        result = spc->send(setCmd, cmdLen);
    }
#ifdef SOPS_DEBUG
    else
    {
        SopsLog("sendAllProperties did not find /locked\n");
    }
#endif

#ifdef SOPS_DEBUG
    SopsLog("SopsPropertyManager::sendAllProperties() done\n");
#endif

    return result;
}

//
// FUNCTION    :: SopsPropertyManager::setProperty
// DESCRIPTION :: The property will be created if it does not exist. When 
//                created it is time to send the property to Vops.
//
//                If the property exists, the value is updated. The
//                isReadyToSend() function is called to determine if
//                the value should be sent to the Vops presently.
//
void SopsPropertyManager::setProperty(
        const std::string& key, const std::string& value)
{
    pthread_rwlock_rdlock(&m_rwlock_properties);
    PropertyManagerIterator it = m_properties.find(key);
    pthread_rwlock_unlock(&m_rwlock_properties);

    if (it == m_properties.end())
    {
        // property not found: create it
        SopsProperty *newprop = new SopsProperty(key, value);

        pthread_rwlock_wrlock(&m_rwlock_properties);
        m_properties[key] = newprop;
        pthread_rwlock_unlock(&m_rwlock_properties);

        // Apply maxwait and quantum values
        std::vector<SopsMaxwait>::iterator maxwait_it = m_maxwaits.begin();
        while (maxwait_it != m_maxwaits.end())
        {
            if (OPS_WildcardMatch(key, maxwait_it->wildkey))
            {
                newprop->setMaxwait(maxwait_it->maxwait);
                break; // Cut off at first match
            }
            maxwait_it++;
        }

        std::vector<SopsQuantum>::iterator quantum_it = m_quantums.begin();
        while (quantum_it != m_quantums.end())
        {
            if (OPS_WildcardMatch(key, quantum_it->wildkey))
            {
                newprop->setQuantum(quantum_it->quantum);
                break; // Cut off at first match
            }
            quantum_it++;
        }

        // Send the new property to Vops
        sendOneProperty(newprop);
    }
    else
    {
        // property exists; this is an update
        SopsProperty *prop = (SopsProperty*)it->second;
        if (prop->getValue() != value)
        {
            // Only come here when the new value is not exactly the same
            prop->setValue(value);
            if (prop->isReadyToSend())
            {
                sendOneProperty(prop);
            }
        }
    }
}

//
// FUNCTION    :: SopsPropertyManager::setMaxwait
// DESCRIPTION :: Sets the max wait time for each property that matches the  
//                wildcard key value, and save the setting for future 
//                new properties.
//
void SopsPropertyManager::setMaxwait(const std::string& wildkey, 
    realtime_t max_millisecs)
{
    PropertyManagerIterator it;
    for (it = m_properties.begin(); it != m_properties.end(); it++)
    {
        SopsProperty *prop = (SopsProperty*)it->second;
        if (OPS_WildcardMatch(prop->getKey(), wildkey))
        {
            prop->setMaxwait(max_millisecs);
        }
    }

    // Save for future new properties.
    SopsMaxwait *m = new SopsMaxwait;
    m->wildkey = wildkey;
    m->maxwait = max_millisecs;
    m_maxwaits.push_back(*m);
}

//
// FUNCTION    :: SopsPropertyManager::setQuantum
// DESCRIPTION :: Sets the quantum for each property that matches the  
//                wildcard key value and saves the quantum for future
//                new properties. 
//
void SopsPropertyManager::setQuantum(const std::string& wildkey, 
    double quantum)
{
    PropertyManagerIterator it;
    for (it = m_properties.begin(); it != m_properties.end(); it++)
    {
        SopsProperty *prop = (SopsProperty*)it->second;
        if (OPS_WildcardMatch(prop->getKey(), wildkey))
        {
            prop->setQuantum(quantum);
        }
    }

    // Save it for future new properties.
    SopsQuantum *q = new SopsQuantum;
    q->wildkey = wildkey;
    q->quantum = quantum;
    m_quantums.push_back(*q);
}
