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

#include <vector>

#include "ocula_state.h"

OculaState::OculaState()
{
    m_locked = false;
}

void OculaState::setProperty(const std::string& key, const std::string& val)
{
    bool insert;
    std::string oldVal;

    // Search for existing property
    std::map<std::string, OculaProperty>::iterator it = m_state.find(key);
    OculaProperty* prop;
    if (it == m_state.end())
    {
        // Insert
        insert = true;
        OculaProperty p;
        p.m_value = val;
        m_state[key] = p;
        prop = &m_state[key];
    }
    else
    {
        // Update
        insert = false;
        oldVal = it->second.m_value;
        it->second.m_value = val;
        prop = &it->second;
    }
    //printf("OculaState set \"%s\" to \"%s\"\n", key.c_str(), val.c_str());

    if (key == "/locked")
    {
        if (m_locked && val == "0")
        {
            // Unlocked, trigger all callbacks
            for (std::list<DelayedCallback>::iterator it = m_lockedCallbacks.begin();
                it != m_lockedCallbacks.end();
                ++it)
            {
                (*it->callback)(it->key.c_str(), NULL, it->prop->m_value.c_str(), &it->prop->m_callbackUserData[it->callback]);
            }

            m_lockedCallbacks.clear();
            m_locked = false;
        }
        else if (!m_locked && val == "1")
        {
            m_locked = true;
        }
    }

    // Check for callbacks
    for (std::map<std::string, OculaCallback>::iterator it = m_callbacks.begin();
        it != m_callbacks.end();
        it++)
    {
        if (OculaState::callbackMatches(key, it->first))
        {
            if (m_locked)
            {
                DelayedCallback dc;
                dc.key = key;
                dc.prop = prop;
                dc.callback = it->second;

                m_lockedCallbacks.push_back(dc);
            }
            else if (insert)
            {
                (*it->second)(key.c_str(), NULL, val.c_str(), &prop->m_callbackUserData[it->second]);
            }
            else
            {
                (*it->second)(key.c_str(), oldVal.c_str(), val.c_str(), &prop->m_callbackUserData[it->second]);
            }
        }
    }
}

bool OculaState::deleteProperty(const std::string& key)
{
    printf("OculaState::deleteProperty is not implemented\n");
    return true;
}

bool OculaState::getProperty(const std::string& key, OculaProperty** val)
{
    std::map<std::string, OculaProperty>::iterator it = m_state.find(key);
    if (it == m_state.end())
    {
        *val = NULL;
        return false;
    }
    else
    {
        *val = &it->second;
        return true;
    }
}

bool OculaState::registerCallback(const std::string& key, OculaCallback callback)
{
    m_callbacks[key] = callback;
    return true;
}

bool OculaState::removeCallback(const std::string& key, OculaCallback callback)
{
    printf("OculaState::removeCallback is not implemented\n");
    return true;
}

void OculaState::StringSplit(
    const std::string& str,
    const std::string& delim,
    std::vector<std::string>& out)
{
    unsigned int pos = 0;
    int next;

    out.clear();

    // Get each token up to the last delimiter
    next = (int)str.find(delim, pos);
    while (next != -1)
    {
        if (next != (int)pos)
        {
            out.push_back(str.substr(pos, next - pos));
        }

        pos = next + 1;
        next = (int)str.find(delim, pos);
    }

    // Get the token after the final delimiter
    if (pos < str.size())
    {
        out.push_back(str.substr(pos));
    }
}

std::string OculaState::getSegment(const std::string& key, int index)
{
    std::string str;
    const char* ch = key.c_str();

    // Skip past first '/'
    if (*ch == '/')
    {
        ch++;
    }

    // Advance index
    while (index > 0)
    {
        if (*ch == '/')
        {
            // Reached next index
            index--;
        }
        else if (*ch == 0)
        {
            // Did not reach segment, return empty string
            return str;
        }
        ch++;
    }

    // Now we are at the first char of this index
    // Determine how many characters are in this segment
    const char* ch2 = ch + 1;
    while (*ch2 != 0 && *ch2 != '/')
    {
        ch2++;
    }

    str.insert(0, ch, ch2 - ch);

    return str;
}

bool OculaState::callbackMatches(const std::string& key, const std::string callback)
{
    const char *keyChar = key.c_str();
    const char *callbackChar = callback.c_str();

    while (*keyChar != 0 && *callbackChar != 0)
    {
        if (*keyChar == *callbackChar)
        {
            // Chars match.  Continue to next character.
            keyChar++;
            callbackChar++;
        }
        else if (*callbackChar == '*')
        {
            // Callback is a wildcard
            // Advance past the '*' character
            // Should end up on a '/' character
            callbackChar++;

            // Advance i up to the next '/' character
            // key[i] should not begin on a '/' character
            while (*keyChar != 0 && *keyChar != '/')
            {
                keyChar++;
            }
        }
        else
        {
            // Chars do not match
            return false;
        }
    }

    // We got to the end of one string
    // Must be at the end of both strings to match (no remaining chars)
    return *keyChar == 0 && *callbackChar == 0;
}
