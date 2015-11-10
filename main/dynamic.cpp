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

// /**
// PACKAGE     :: DYNAMIC
// DESCRIPTION :: Implements the Dynamic API
// **/

#include <stdio.h>
#include <stdlib.h>
#include <algorithm>


#include "api.h"
#include "partition.h"
#include "dynamic.h"
#include "network_ip.h"

// Fix for vc9 compilation
#if (_MSC_VER >= 1500) //vc9
#ifdef GetObject
#undef GetObject
#endif
#endif

//#define DEBUG
//#define DEBUG_LISTEN

//
// new - Overloaded new operation.  Makes all calls to "new" use MEM_malloc
//     instead
//

void* operator new(size_t num_bytes)
{
    return MEM_malloc(num_bytes);
}

void operator delete(void* p)
{
    MEM_free(p);
}

void StringSplit(
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

void D_Exception::GetFullErrorString(std::string& str)
{
    str = "An exception occurred in the Dynamic API: " + error + "\n";
}

D_Level::D_Level()
{
    name = "";
    description = "";
    id = 0;
    parent = NULL;
    object = NULL;
    link = NULL;
    useHash = FALSE;
    childrenHash = NULL;
#ifdef PARALLEL
    m_Partition = NO_PARTITION;
#endif // PARALELL

    // children and linksToThisLevel arrays are constructed automatically
}

D_Level::~D_Level()
{
    // Nothing to do.  Array deconstructors get called automatically.
}

D_Object* D_Level::GetObject()
{
    if (object == NULL)
    {
        throw D_ExceptionNotObject(GetFullPath().c_str());
    }

    return object;
}

std::string D_Level::GetFullPath()
{
    std::string path;

    // Recursively build path
    if (parent != NULL)
    {
        path = parent->GetFullPath() + "/" + name;
    }
    else
    {
        path = "";
    }

    return path;
}

void D_Level::SetFullPath(const std::string& newName, const std::string& newPath)
{
    name = newName;
}

void D_Level::SetDescription(const std::string& newDescription)
{
    description = newDescription;
}

struct LevelSorter
{
    bool operator () (D_Level* a, D_Level* b)
    {
        return a->GetName() < b->GetName();
    }
};

struct LevelSearcher
{
    bool operator () (D_Level* a, const std::string& b)
    {
        return a->GetName() < b;
    }
};


UInt32 D_Level::ChildrenHash(const std::string& name)
{
    UInt32 sum = 5381;

    // TODO: redo hash function
    for (unsigned int i = 0; i < name.size(); i++)
    {
        sum = ((sum << 5) + sum) ^ name[i];
        //sum += name[i];
    }

    return sum % hashSize;
}

void D_Level::AddChild(D_Level* child)
{
#ifdef DEBUG
    printf(
        "Dynamic API: Adding child \"%s\" to \"%s\"\n",
        child->GetName().c_str(),
        GetFullPath().c_str());
#endif // DEBUG

    // Insert child maintaining sorted order
    std::vector<D_Level*>::iterator it;
    it = lower_bound(children.begin(), children.end(), child->name, LevelSearcher());
    if (it == children.end())
    {
        children.push_back(child);
    }
    else
    {
        children.insert(it, child);
    }

    if (useHash)
    {
        UInt32 key = ChildrenHash(child->name);

        // Insert to children hash maintaining sorted order
        it = lower_bound(childrenHash[key].begin(), childrenHash[key].end(), child->name, LevelSearcher());
        if (it == childrenHash[key].end())
        {
            childrenHash[key].push_back(child);
        }
        else
        {
            childrenHash[key].insert(it, child);
        }
    }
}

void D_Level::RemoveChild(D_Level* child)
{
#ifdef DEBUG
    printf(
        "Dynamic API: Removing child \"%s\" from \"%s\"\n",
        child->GetName().c_str(),
        GetFullPath().c_str());
#endif // DEBUG

    std::vector<D_Level*>::iterator it;

    it = std::find(children.begin(), children.end(), child);
    assert(it != children.end());
    children.erase(it);

    // If using hash remove child
    if (useHash)
    {
        UInt32 key = ChildrenHash(child->name);
        it = std::find(childrenHash[key].begin(), childrenHash[key].end(), child);
        assert(it != childrenHash[key].end());
        childrenHash[key].erase(it);
    }
}

BOOL D_Level::HasChild(const std::string& name)
{
    unsigned int i;

    // Look through all children for the given name
    for (i = 0; i < children.size(); i++)
    {
        // If found, return TRUE
        if (children[i]->GetName() == name)
        {
            return TRUE;
        }
    }

    // Not found, return FALSE
    return FALSE;
}

D_Level* D_Level::GetChild(const std::string& name)
{
    std::vector<D_Level*>::iterator it;

    if (useHash)
    {
        UInt32 key = ChildrenHash(name);

        it = lower_bound(childrenHash[key].begin(), childrenHash[key].end(), name, LevelSearcher());
        if (it != childrenHash[key].end() && (**it).GetName() == name)
        {
           return *it;
        }
    }
    else
    {
        it = lower_bound(children.begin(), children.end(), name, LevelSearcher());
        if (it != children.end() && (**it).GetName() == name)
        {
            return *it;
        }
    }

    // Not found, throw exception
    std::string badPath = GetFullPath() + "/" + name;
    throw D_ExceptionInvalidPath(badPath);
}

D_Level* D_Level::GetParent()
{
    if (!HasParent())
    {
        throw D_ExceptionNoParent(GetFullPath());
    }

    return parent;
}

void D_Level::SetObject(D_Object* newObject)
{
    // Check that this level does not have a link or object
    if (object != NULL)
    {
        std::string err;

        err = "Level " + GetFullPath() + " already has an object";
        throw D_Exception(err);
    }
    if (link != NULL)
    {
        std::string err;

        err = "Level " + GetFullPath() + " already has a link";
        throw D_Exception(err);
    }

    object = newObject;
}

void D_Level::RemoveObject()
{
    if (object == NULL)
    {
        std::string err;

        err = "Level " + GetFullPath() + " has no object";
        throw D_Exception(err);
    }

    object = NULL;
}

void D_Level::SetLink(D_Level* newLink)
{
    // Check that this level does not have an object or link
    if (object != NULL)
    {
        std::string err;

        err = "Level " + GetFullPath() + " already has an object";
        throw D_Exception(err);
    }
    if (link != NULL)
    {
        std::string err;

        err = "Level " + GetFullPath() + " already has a link";
        throw D_Exception(err);
    }

    link = newLink;
}

void D_Level::AddLinkToThisLevel(D_Level* link)
{
#ifdef DEBUG
    printf(
        "Dynamic API: Adding link \"%s\" to \"%s\"\n",
        link->GetFullPath().c_str(),
        GetFullPath().c_str());
#endif // DEBUG

    linksToThisLevel.push_back(link);
}

void D_Level::RemoveLinkToThisLevel(D_Level* link)
{
#ifdef DEBUG
    printf(
        "Dynamic API: Removing link \"%s\" to \"%s\"\n",
        link->GetFullPath().c_str(),
        GetFullPath().c_str());
#endif // DEBUG

    std::vector<D_Level*>::iterator it;

    it = std::find(linksToThisLevel.begin(), linksToThisLevel.end(), link);
    assert(it != linksToThisLevel.end());
    linksToThisLevel.erase(it);
}

D_Level* D_Level::ResolveLinks()
{
    D_Level* level = this;

    // While the level is a link, resole the link
    while (level->IsLink())
    {
        level = level->link;
    }

    return level;
}

void D_Level::AddPartition(int partition)
{
    // If no partition, set partition
    // If there is already one partition and this is a new partition, set multiple partitions
    // Otherwise it is already multiple partitions, don't change

    if (m_Partition == NO_PARTITION)
    {
        m_Partition = partition;
    }
    else if (m_Partition >= 0 && m_Partition != partition)
    {
        m_Partition = MULTIPLE_PARTITIONS;
    }
}

D_Listener::~D_Listener()
{
    // Free the callback class
    delete callback;
}

D_Variable* D_Listener::GetVariable()
{
    if (variable == NULL)
    {
        std::string message = "a listener";
        throw D_ExceptionNoObject(message);
    }

    return variable;
}

void D_Listener::Changed()
{
    // Always call the callback for the default listener
    CallCallback();
}

void D_Listener::CallCallback()
{
    std::string newValue;

    variable->ReadAsString(newValue);
    (*callback)(newValue);
}

D_ListenerPercent::D_ListenerPercent(
    D_Variable* newVariable,
    const std::string& arguments,
    const std::string& tag,
    D_ListenerCallback* newCallback) : D_Listener(newVariable, tag, newCallback)
{
    // Test that the variable can be read as a double.  If it can't get it
    // as a double an exception will be thrown, causing the listener to not
    // be added.
    newVariable->GetDouble();

    // If the argument is x% then listen based on x% change.  If the
    // argument is x then listen based on x * 100 % change
    if (std::find(arguments.begin(), arguments.end(), '%') != arguments.end())
    {
        percent = atof(arguments.c_str()) / 100.0;
    }
    else
    {
        percent = atof(arguments.c_str());
    }

    lastVal = 0;
}

void D_ListenerPercent::Changed()
{
    double val;
    double diff;

    // Convert the variable to a double.  We have verified that this can be
    // done in the D_ListenerPercent::SetVariable function.
    val = variable->GetDouble();

    // Now calculate the change in percentage, considering whether values
    // are zero, positive or negative
    if (val == 0.0 && lastVal == 0.0)
    {
        // Current and last value is 0, the difference is 0%
        diff = 0.0;
    }
    else if (val == 0.0)
    {
        // New value is 0, old value not 0, the difference is 100%
        diff = 1.0;
    }
    else if (lastVal == 0.0)
    {
        // Old value is 0, new value not 0, the difference is infinite
        diff = 1.0e6;
    }
    else if (val * lastVal < 0)
    {
        // Value changed from + to -, or - to +
        diff = 1.0 - val / lastVal;
    }
    else
    {
        if (val < 0)
        {
            // Values are negative
            if (val < lastVal)
            {
                diff = val / lastVal - 1.0;
            }
            else
            {
                diff = 1.0 - val / lastVal;
            }
        }
        else
        {
            // Values are positive
            if (val > lastVal)
            {
                diff = val / lastVal - 1.0;
            }
            else
            {
                diff = 1.0 - val / lastVal;
            }
        }
    }

    // If the difference overcomes the required percentage, call the
    // callback and update the last value
    if (diff >= percent)
    {
        lastVal = val;
        CallCallback();
    }
}

D_Object::D_Object(D_Type newType) :
    type(newType), level(NULL), readable(FALSE),
    writeable(FALSE), executable(FALSE)
{
}

D_Object::~D_Object()
{
}

void D_Object::ReadAsString(std::string& out)
{
    // The subclass of this object must implement this function
    throw D_ExceptionNotImplemented(GetFullPath(), "read");
}

void D_Object::WriteAsString(const std::string& in)
{
    // The subclass of this object must implement this function
    throw D_ExceptionNotImplemented(GetFullPath(), "write");
}

void D_Object::ExecuteAsString(const std::string& in, std::string& out)
{
    // The subclass of this object must implement this function
    throw D_ExceptionNotImplemented(GetFullPath(), "execute");
}

D_HierarchyId D_Object::GetId()
{
    if (level == NULL)
    {
        throw D_ExceptionNotInHierarchy();
    }

    return level->GetId();
}

std::string D_Object::GetFullPath()
{
    if (level == NULL)
    {
        throw D_ExceptionNotInHierarchy();
    }

    return level->GetFullPath();
}

void D_Object::SetLevel(D_Level* newLevel)
{
    // Make sure the object is not already in the hierarchy
    if (IsInHierarchy())
    {
        throw D_ExceptionIsInHierarchy(
            newLevel->GetFullPath(),
            GetFullPath());
    }

    // Set the object's level
    level = newLevel;
}

void D_Object::RemoveFromHierarchy(D_Hierarchy* hierarchy)
{
    // First make sure that the object is in the hierarchy
    if (level == NULL)
    {
        throw D_ExceptionNotInHierarchy();
    }

    // Remove the level containing this object.  This will also remove
    // the object from the level, eventually calling
    // this->RemoveFromLevel().
    hierarchy->RemoveLevel(level->GetFullPath());
}

void D_Object::RemoveFromLevel()
{
    // First make sure that the object is in the hierarchy
    if (level == NULL)
    {
        throw D_ExceptionNotInHierarchy();
    }

#ifdef D_LISTENING_ENABLED
    // If there are any listeners on this object, then remove them and free
    // their memory
    while (listeners.size() > 0)
    {
        RemoveListener(listeners[0]);
    }
#endif // D_LISTENING_ENABLED

#ifdef DEBUG
    printf("Removing object from %s\n", level->GetFullPath().c_str());
#endif

    // Remove the level from this object
    level = NULL;
}

#ifdef D_LISTENING_ENABLED
void D_Object::AddListener(D_Listener* newListener)
{
#ifdef DEBUG_LISTEN
    printf("Dynamic API: Adding listener to \"%s\"\n", GetFullPath().c_str());
#endif // DEBUG

    listeners.push_back(newListener);
}

void D_Object::RemoveListener(D_Listener* listener)
{
#ifdef DEBUG_LISTEN
    printf("Dynamic API: Removing listener from \"%s\"\n", GetFullPath().c_str());
#endif // DEBUG

    std::vector<D_Listener*>::iterator it;

    it = std::find(listeners.begin(), listeners.end(), listener);
    assert(it != listeners.end());
    listeners.erase(it);
    delete listener;
}
#endif // D_LISTENING_ENABLED

D_Variable::D_Variable() : D_Object(D_VARIABLE)
{
    readable = TRUE;
    writeable = TRUE;
}

D_Variable::D_Variable(D_Type newType) : D_Object(newType)
{
    readable = TRUE;
    writeable = TRUE;
}

double D_Variable::GetDouble()
{
    std::string errString;
    errString = "The variable \"" + GetFullPath() + "\" is not a numeric type";
    throw D_Exception(errString);
}

void D_StringObj::ReadAsString(std::string& out)
{
    out = (const char*) *value;
}

void D_StringObj::WriteAsString(const std::string& in)
{
    value->Set(in);
#ifdef D_LISTENING_ENABLED
    Changed();
#endif // D_LISTENING_ENABLED
}

void D_Hierarchy::ReadConfigFile(NodeInput* nodeInput)
{
    BOOL out;
    BOOL read;

    IO_ReadBool(
        ANY_DEST,
        ANY_ADDRESS,
        nodeInput,
        "DYNAMIC-ENABLED",
        &read,
        &out);
    if (read)
    {
        enabled = out;
    }
    else
    {
        enabled = FALSE;
    }

    IO_ReadBool(
        ANY_DEST,
        ANY_ADDRESS,
        nodeInput,
        "DYNAMIC-PARTITION-ENABLED",
        &read,
        &out);
    if (read)
    {
        partitionEnabled = out;
    }
    else
    {
        partitionEnabled = TRUE;
    }

    IO_ReadBool(
        ANY_DEST,
        ANY_ADDRESS,
        nodeInput,
        "DYNAMIC-NODE-ENABLED",
        &read,
        &out);
    if (read)
    {
        nodeEnabled = out;
    }
    else
    {
        nodeEnabled = TRUE;
    }

    IO_ReadBool(
        ANY_DEST,
        ANY_ADDRESS,
        nodeInput,
        "DYNAMIC-PHY-ENABLED",
        &read,
        &out);
    if (read)
    {
        phyEnabled = out;
    }
    else
    {
        phyEnabled = TRUE;
    }

    IO_ReadBool(
        ANY_DEST,
        ANY_ADDRESS,
        nodeInput,
        "DYNAMIC-MAC-ENABLED",
        &read,
        &out);
    if (read)
    {
        macEnabled = out;
    }
    else
    {
        macEnabled = TRUE;
    }

    IO_ReadBool(
        ANY_DEST,
        ANY_ADDRESS,
        nodeInput,
        "DYNAMIC-NETWORK-ENABLED",
        &read,
        &out);
    if (read)
    {
        networkEnabled = out;
    }
    else
    {
        networkEnabled = TRUE;
    }

    IO_ReadBool(
        ANY_DEST,
        ANY_ADDRESS,
        nodeInput,
        "DYNAMIC-TRANSPORT-ENABLED",
        &read,
        &out);
    if (read)
    {
        transportEnabled = out;
    }
    else
    {
        transportEnabled = TRUE;
    }

    IO_ReadBool(
        ANY_DEST,
        ANY_ADDRESS,
        nodeInput,
        "DYNAMIC-APP-ENABLED",
        &read,
        &out);
    if (read)
    {
        appEnabled = out;
    }
    else
    {
        appEnabled = TRUE;
    }
}

D_Level* D_Hierarchy::CreateLevel(
    D_Level* parentLevel,
    const std::string& levelName
#ifdef PARALLEL
    , int partition
#endif // PARALLEL
)
{
    std::string fullPath;
    D_Level* level;

    // Create the full path
    fullPath = std::string(parentLevel->GetFullPath()) + "/" + levelName;

    // If the path exists then throw an exception (cannot create a path that
    // already exists)
    if (IsValidPath(fullPath))
    {
        throw D_ExceptionPathExists(fullPath.c_str());
    }

#ifdef DEBUG
    printf("Dynamic API: ... Created level \"%s\" on p%d from %d\n", fullPath.c_str(), m_Partition->partitionId, partition);
#endif // DEBUG

    // Allocate the level and add it to the level array
    level = new D_Level;

    levels.push_back(level);

    // Fill in the level
    level->SetFullPath(levelName, fullPath);
    level->SetId(nextId++);
    level->SetParent(parentLevel);
#ifdef PARALLEL
    level->AddPartition(partition);
#endif // PARALLEL

    // Create hash if it is node or interface level
    if (fullPath == "/node" || fullPath == "/interface")
    {
        level->useHash = TRUE;
        level->hashSize = m_Partition->numNodes;
        level->childrenHash = new std::vector<D_Level*>[level->hashSize];
    }


    // Add the level to its parent
    parentLevel->AddChild(level);

#ifdef PARALLEL
    // Send create level message to other partitions if we created this partition
    if (m_Partition->partitionId == partition)
    {
        ParallelCreateLevel(fullPath);
    }
#endif // PARALLEL

    // Return the newly created level
    return level;
}

D_Level* D_Hierarchy::GetNextLevel(
    D_Level* parentLevel,
    const std::string& nextLevel,
    BOOL resolveLinks)
{
    if (parentLevel->HasChild(nextLevel))
    {
        D_Level* child = parentLevel->GetChild(nextLevel);

        // Resolve all links if specified
        if (resolveLinks)
        {
            child = child->ResolveLinks();
        }
        ERROR_Assert(child != NULL, "Bad child relationship");

        return child;
    }
    else
    {
        return NULL;
    }
}

D_Level* D_Hierarchy::GetLevelRecursive(
    D_Level* parentLevel,
    const std::string& fullPath,
    const std::string& remainingPath,
    BOOL resolveLinks)
{
    char* nextPathC;
    std::string nextPath;
    char tokenC[MAX_STRING_LENGTH];
    std::string token;
    D_Level* nextLevel;

    // TODO: re-write this for C++

    if (remainingPath.size() == 0)
    {
        return parentLevel;
    }

    // Get the next token in the path
    IO_GetDelimitedToken(
        tokenC,
        remainingPath.c_str(),
        "/",
        &nextPathC);
    token = tokenC;
    nextPath = nextPathC;

    // Look up the next level relative to the current level, resolving links
    // if specified
    nextLevel = GetNextLevel(parentLevel, token, resolveLinks);

    if (nextLevel == NULL)
    {
        throw D_ExceptionInvalidPath(fullPath.c_str());
    }
    else
    {
        return GetLevelRecursive(nextLevel, fullPath, nextPath, resolveLinks);
    }
}

D_Level* D_Hierarchy::GetLevelById(
    D_HierarchyId id,
    BOOL resolveLinks)
{
    unsigned int i;

    // Look through all levels for id.  This function is slow and can be
    // optimized.
    for (i = 0; i < levels.size(); i++)
    {
        if (id == levels[i]->GetId())
        {
            return levels[i];
        }
    }

    // Not found, throw exception
    throw D_ExceptionInvalidId(id);
}

D_Level* D_Hierarchy::GetLevelByPath(
    const std::string& path,
    BOOL resolveLinks)
{
    // If the path is "/" then return the root node
    if (path == "/")
    {
        return root;
    }

    std::vector<std::string> levels;

    // First split up the path into levels then loop over each level
    StringSplit(path, "/", levels);

    D_Level* nextLevel = root;
    for (unsigned int i = 0; i < levels.size(); i++)
    {
        if (levels[i] == "")
        {
            continue;
        }

        nextLevel = GetNextLevel(nextLevel, levels[i], resolveLinks);

        if (nextLevel == NULL)
        {
            throw D_ExceptionInvalidPath(path.c_str());
        }
    }

    // If found, return the level
    return nextLevel;
}

void D_Hierarchy::PrintRecursive(D_Level* level)
{
    std::string value;
    unsigned int i;

    ERROR_Assert(level != NULL, "Invalid hierarchy (level == NULL)");

    // Print the level id and path
    printf(
        "Level %d: %s\n",
        level->GetId(),
        level->GetFullPath().c_str());

    // If it is an object, print object info
    if (level->IsObject())
    {
        D_Object* object = level->GetObject();
        BOOL printValue = TRUE;

        // Print the object's type
        printf("    Object type: ");
        switch(object->GetType())
        {
            case D_VARIABLE:
            {
                printf("Variable\n");
                break;
            }

            case D_STATISTIC:
            {
                printf("Statistic\n");
                break;
            }

            case D_COMMAND:
            {
                printf("Command\n");
                printValue = FALSE;
                break;
            }

            default:
            {
                // This should never occur
                ERROR_Assert(0, "Unknown type");
                break;
            }
        }

        // If the object has value, print it
        if (printValue)
        {
            object->ReadAsString(value);
            printf("    Object value: %s\n", value.c_str());
        }
    }

    // If it is a link then print link info
    if (level->IsLink())
    {
        printf(
            "    Links to: %s (%d)\n",
            level->GetLink()->GetFullPath().c_str(),
            level->GetLink()->GetId());
    }

    // If other levels link to this level then print info
    if (level->IsLinkedTo())
    {
        printf("    Linked to from: ");

        // Print all of the IDs but the last one
        i = 0;
        while (i < level->linksToThisLevel.size() - 1)
        {
            printf(
                "%s (%d), ",
                level->linksToThisLevel[i]->GetFullPath().c_str(),
                level->linksToThisLevel[i]->GetId());
            i++;
        }

        // Print the last ID
        printf(
            "%s (%d)\n",
            level->linksToThisLevel[i]->GetFullPath().c_str(),
            level->linksToThisLevel[i]->GetId());
    }

    if (level->HasDescription())
    {
        printf("    Description: %s\n", level->GetDescription().c_str());
    }

    // If it has children then print them
    if (level->children.size() > 0)
    {
        printf("    Child IDs: ");

        // Print all of the IDs but the last one
        i = 0;
        while (i < level->children.size() - 1)
        {
            printf(
                "%s (%d), ",
                level->children[i]->GetFullPath().c_str(),
                level->children[i]->GetId());
            i++;
        }

        // Print the last ID
        printf(
            "%s (%d)\n",
            level->children[i]->GetFullPath().c_str(),
            level->children[i]->GetId());
    }

    printf("\n");

    // Recursively print this level's children
    for (i = 0; i < level->children.size(); i++)
    {
        PrintRecursive(level->children[i]);
    }
}

D_Hierarchy::D_Hierarchy()
{
    m_Partition = NULL;
    root = NULL;
    nextId = 0;
    enabled = FALSE;
    partitionEnabled = FALSE;
    nodeEnabled = FALSE;
    phyEnabled = FALSE;
    macEnabled = FALSE;
    networkEnabled = FALSE;
    transportEnabled = FALSE;
    appEnabled = FALSE;

#ifdef PARALLEL
    nextCallbackId = 0;
#endif
}

D_Hierarchy::~D_Hierarchy()
{
}

void D_Hierarchy::Initialize(PartitionData* partition, int initialNumLevels)
{
    m_Partition = partition;

    ReadConfigFile(partition->nodeInput);

    if (enabled)
    {
        // Zero all values in the hierarchy
        nextId = 1;

        // Create the root level
        root = new D_Level;
        levels.push_back(root);
        root->SetId(nextId++);
        root->SetParent(NULL);
        root->SetFullPath("", "");
        root->AddPartition(partition->partitionId);
    }
    else
    {
        root = NULL;
        nextId = 0;
    }
}

void D_Hierarchy::CreatePath(const std::string& path)
{
    AddLevel(path, m_Partition->partitionId);
}

D_Level* D_Hierarchy::AddLevel(
    const std::string& path,
#ifdef PARALLEL
    int partition,
#endif
    const std::string& newDescription)
{
    if (!enabled)
    {
        throw D_ExceptionNotEnabled();
    }

#ifdef DEBUG
    printf("Dynamic API: Adding level \"%s\" from p%d\n", path.c_str(), partition);
#endif // DEBUG
    D_Level* parentLevel = root;
    D_Level* nextLevel = NULL;

    std::vector<std::string> levels;

    // First split up the path into levels then loop over each level
    StringSplit(path, "/", levels);

    for (unsigned int i = 0; i < levels.size(); i++)
    {
        // Look up the next level relative to the parent level
        nextLevel = GetNextLevel(parentLevel, levels[i]);

        // If this is the last token in the path, verify that the next level
        // (the level the caller wanted to create) does not already exist
        if ((nextLevel != NULL) && i == levels.size() - 1)
        {
#ifdef PARALLEL
            // If we are running in parallel and a remote partition created the
            // path just mark that it is also owned by the remote partition and
            // return the level.
            if (m_Partition->partitionId != partition)
            {
                nextLevel->AddPartition(partition);
                return nextLevel;
            }
#endif // PARALLEL

            throw D_ExceptionPathExists(nextLevel->GetFullPath());
        }

        // If the next level does not exist, then create it
        if (nextLevel == NULL)
        {
#ifdef PARALLEL
            nextLevel = CreateLevel(parentLevel, levels[i], partition);
#else // PARALLEL
            nextLevel = CreateLevel(parentLevel, levels[i]);
#endif // PARALLEL
        }
        else
        {
            // If the level does exist, then resolve links and verify that it is
            // not an object
            nextLevel = nextLevel->ResolveLinks();
            if (nextLevel->IsObject())
            {
                throw D_ExceptionIsObject(nextLevel->GetFullPath());
            }

#ifdef PARALLEL
            // Mark that this partition also owns this path
            nextLevel->AddPartition(partition);
#endif // PARALLEL
        }

        // Process next level
        parentLevel = nextLevel;
    }

    // If there are no more tokens, then the level has been added.
    // Return pointer to this level.
    nextLevel->SetDescription(newDescription);
    return nextLevel;
}

D_Level* D_Hierarchy::AddObject(
    const std::string& path,
    D_Object* object,
    const std::string& newDescription)
{
    if (!enabled)
    {
        throw D_ExceptionNotEnabled();
    }

    D_Level* level;

    // Make sure the object is not already in the hierarchy
    if (object->IsInHierarchy())
    {
        throw D_ExceptionIsInHierarchy(
            path,
            object->GetFullPath());
    }

    // Get the level
    level = GetLevelByPath(path);

#ifdef PARALLEL
    // Objects can only exist on one partition
    if (level->GetPartition() == MULTIPLE_PARTITIONS)
    {
        std::string err = "The level " + level->GetFullPath() + " cannot have an object because it is on multiple partitions";
        throw D_Exception(err);
    }
#endif

    level->SetDescription(newDescription);

    // Add the object to the level
    level->SetObject(object);
    object->SetLevel(level);

#ifdef PARALLEL
    ParallelAddObject(path);
#endif // PARALLEL

#ifdef D_LISTENING_ENABLED
    // Let each object listener know a new object was added
    for (unsigned int i = 0; i < objectListeners.size(); i++)
    {
        (*objectListeners[i])(path);
    }
#endif

    // Return the added level
    return level;
}

D_Level* D_Hierarchy::AddLink(
    const std::string& path,
    const std::string& link,
    const std::string& newDescription)
{
    if (!enabled)
    {
        throw D_ExceptionNotEnabled();
    }

    // TODO: support for parallel

    D_Level* level;
    D_Level* linkLevel;

    // Look up the link
    linkLevel = GetLevelByPath(link);

    // Add the level
#ifdef PARALLEL
    level = AddLevel(path, m_Partition->partitionId, newDescription);
    ParallelAddLink(path, link);
#else // PARALLEL
    level = AddLevel(path, newDescription);
#endif // PARALLEL

    // Set the link, and the reverse link
    level->SetLink(linkLevel);
    linkLevel->AddLinkToThisLevel(level);

#ifdef D_LISTENING_ENABLED
    // Let each object listener know a new object was added
    for (unsigned int i = 0; i < linkListeners.size(); i++)
    {
        (*linkListeners[i])(path + " " + link);
    }
#endif

    // Return the added level
    return level;
}

void D_Hierarchy::RemoveLevel(const std::string& path)
{
    if (!enabled)
    {
        throw D_ExceptionNotEnabled();
    }

    D_Level* level = GetLevelByPath(path, FALSE);
#ifdef DEBUG
    printf("Removing %s\n", path.c_str());
#endif

    // Remove all children
    while (level->children.size() > 0)
    {
#ifdef DEBUG
        printf("Removing child %s\n", level->children[0]->GetFullPath().c_str());
#endif
        RemoveLevel(level->children[0]->GetFullPath());
    }

    // Recursively remove all links to this level.  The linking level will
    // remove itself from our linksToThislevel list
    while (level->linksToThisLevel.size() > 0)
    {
#ifdef DEBUG
        printf("Removing link %s->%s\n", path.c_str(), level->linksToThisLevel[0]->GetFullPath().c_str());
#endif
        RemoveLevel(level->linksToThisLevel[0]->GetFullPath());
    }

    // Finally remove this level from the hierarchy if it is not the root
    // level
    if (!level->IsRoot())
    {
        // Remove this level from the hierarchy
        std::vector<D_Level*>::iterator it;
        it = std::find(levels.begin(), levels.end(), level);
        assert(it != levels.end());
        levels.erase(it);

        // Remove this level from its parent's list of children
        level->parent->RemoveChild(level);

        // If this level is a link to another level, remove this level from the
        // other level's list of levels that link to it
#ifdef DEBUG
        printf("%s is link %d\n", level->GetFullPath().c_str(), level->IsLink());
#endif
        if (level->IsLink())
        {
#ifdef DEBUG
            printf("%s removing its link from %s\n", level->GetFullPath().c_str(), level->link->GetFullPath().c_str());
#endif
            level->link->RemoveLinkToThisLevel(level);
        }

        // If this level has an object, remove the object from this level
        if (level->IsObject())
        {
            level->GetObject()->RemoveFromLevel();
            level->RemoveObject();

#ifdef D_LISTENING_ENABLED
            // Let each object listener know a new object was added
            for (unsigned int i = 0; i < removeListeners.size(); i++)
            {
                (*removeListeners[i])(path);
            }
#endif
        }

#ifdef DEBUG
        printf("Dynamic API: Removing level \"%s\"\n", level->GetFullPath().c_str());
#endif // DEBUG

    // Free memory for this level
        delete level;
    }

#ifdef DEBUG
    printf("Leaving remove %s\n", path.c_str());
#endif
}

BOOL D_Hierarchy::IsValidPath(const std::string& path)
{
    if (!enabled)
    {
        return FALSE;
    }

    BOOL exists = TRUE;

    // Try to get the level.  If an invalid path is thrown then the level
    // does not exist.
    try
    {
        GetLevelByPath(path);
    }
    catch (D_ExceptionInvalidPath)
    {
        exists = FALSE;
    }

    return exists;
}

void D_Hierarchy::Print()
{
    if (!enabled)
    {
        throw D_ExceptionNotEnabled();
    }

    ERROR_Assert(root != NULL, "Invalid hierarchy (root == NULL)");

    PrintRecursive(root);
}

int D_Hierarchy::GetNumChildren(const std::string& path)
{
    if (!enabled)
    {
        throw D_ExceptionNotEnabled();
    }

    D_Level* level;

    // Try getting the level.  Will throw D_ExceptionInvalidPath if the path
    // is not valid.
    level = GetLevelByPath(path);

    // Return the level's number of children
    return (int)level->children.size();
}

D_Level* D_Hierarchy::GetChild(const std::string& path, int index)
{
    D_Level* level;

    if (!enabled)
    {
        throw D_ExceptionNotEnabled();
    }

    // Try getting the level.  Will throw D_ExceptionInvalidPath if the path
    // is not valid.
    level = GetLevelByPath(path);

    if (index < 0 || index >= (int)level->children.size())
    {
        throw D_ExceptionInvalidChild(level->GetFullPath());
    }

    return level->children[index];
}

std::string D_Hierarchy::GetChildName(const std::string& path, int index)
{
    // GetChild wiill throw an exception if path or index is invalid
    return GetChild(path, index)->GetName();
}

std::string D_Hierarchy::GetChildName(const std::string& path, std::string& child)
{
    D_Level* level;

    if (!enabled)
    {
        throw D_ExceptionNotEnabled();
    }

    // Try getting the level.  Will throw D_ExceptionInvalidPath if the path
    // is not valid.
    level = GetLevelByPath(path);

    for (unsigned int i = 0; i < level->children.size(); i++)
    {
        if (level->children[i]->name == child)
        {
            return level->children[i]->GetFullPath();
        }
    }

    throw D_ExceptionInvalidChild(level->GetFullPath());
}

std::string D_Hierarchy::EncodeString(const std::string& str)
{
    std::string newStr = "";
    
    for (unsigned int i = 0; i < str.size(); i++)
    {
        if (str[i] == ' ')
        {
            newStr += "%1";
        }
        else if (str[i] == '/')
        {
            newStr += "%2";
        }
        else if (str[i] == '%')
        {
            newStr += "%%";
        }
        else
        {
            newStr += str[i];
        }
    }

    return newStr;
}
std::string D_Hierarchy::EncodePath(const std::string& path)
{
    std::string newPath = "";
    int loc = 0;
    int newLoc = 0;

    // First let's check for quotes. There should be either 2 quotes or no  
    // quotes in the path.
    int pos = 0;
    int nextPos = 0;
    int numQuote = 0;
    nextPos = path.find("\"", pos);
    while (nextPos != -1)
    {
        numQuote++;
        pos = ++nextPos;
        nextPos = path.find("\"", pos);
    }
    if (numQuote != 2 && numQuote != 0)
    {
        throw D_Exception();
    }

    while (path.size() > (unsigned int)loc)
    {
        if (path[loc] == '"')
        {
            // get till the next quote.
            loc ++;
            newLoc = path.find_first_of("\"", loc);
            newPath += this->EncodeString(
                path.substr(loc, newLoc - loc));
        }
        else
        {
            // extract information till the first /
            newLoc = path.find_first_of("/", loc);
            if (newLoc != -1)
            {
                newPath += this->EncodeString(
                    path.substr(loc , newLoc - loc));
            }
            else
            {
                newPath += this->EncodeString(
                    path.substr(loc , path.size() - loc));
                break;
            }
            newPath += "/";
        }
        
        loc = ++newLoc;
    }
    return newPath;
}

BOOL D_Hierarchy::HasParent(const std::string& path)
{
    if (!enabled)
    {
        throw D_ExceptionNotEnabled();
    }

    if (path == "" || path == "/")
    {
        return FALSE;
    }

    return TRUE;
}

void D_Hierarchy::GetParent(std::string& path)
{
    if (!enabled)
    {
        throw D_ExceptionNotEnabled();
    }

    if (!HasParent(path))
    {
        path = "/";
    }
    else
    {
        // Find last "/" and delete everything after it
        std::string::size_type pos = path.rfind("/");
        printf("pos is %" TYPES_SIZEOFMFT "u \n", pos);
        path = path.substr(0, pos);
        if (path == "")
        {
            path = "/";
        }
    }

    printf("dynamic parent is %s\n", path.c_str());
}

void D_Hierarchy::ResolveLinks(std::string& path)
{
    if (!enabled)
    {
        throw D_ExceptionNotEnabled();
    }

    D_Level* level;

    // Try getting the level.  Will throw D_ExceptionInvalidPath if the path
    // is not valid.
    level = GetLevelByPath(path);

    level = level->ResolveLinks();
    path = level->GetFullPath();
}

BOOL D_Hierarchy::IsObject(const std::string& path)
{
    if (!enabled)
    {
        throw D_ExceptionNotEnabled();
    }

    // First look up the level.  This will throw D_ExceptionInvalidPath if
    // the level is not valid.
    D_Level* level = GetLevelByPath(path);

    // A level with children is not an object
    if (level->children.size() > 0)
    {
        return false;
    }

#ifdef PARALLEL
    // If the level is not local check remote partition
    if (!IsLevelLocal(level))
    {
        return ParallelIsObject(level->GetPartition(), path);
    }
#endif // PARALLEL

    // Return TRUE if the level is an object
    return level->IsObject();
}

D_Object* D_Hierarchy::GetObject(const std::string& path)
{
    // First look up the level.  This will throw D_ExceptionInvalidPath if
    // the level is not valid.
    D_Level* level = GetLevelByPath(path);

    if (level->GetPartition() != m_Partition->partitionId)
    {
        throw D_ExceptionRemotePartition(path.c_str());
    }

    return level->GetObject();
}

void D_Hierarchy::SetReadable(const std::string& path, BOOL newReadable)
{
    // Get the object and set its readable value.  Will throw an exception
    // if the path is invalid or not an object.
    GetObject(path)->SetReadable(newReadable);

#ifdef D_LISTENING_ENABLED
    // Let each object listener know a new object was added
    for (unsigned int i = 0; i < objectPermissionsListeners.size(); i++)
    {
        (*objectPermissionsListeners[i])(path);
    }
#endif
}

BOOL D_Hierarchy::IsReadable(const std::string& path)
{
    if (!enabled)
    {
        throw D_ExceptionNotEnabled();
    }

#ifdef PARALLEL
    D_Level* level = GetLevelByPath(path);

    if (!IsLevelLocal(level))
    {
        return ParallelIsReadable(level->GetPartition(), path);
    }
    else
#endif // PARALLEL
    {
        // Get the object and get its readable value.  Will throw an exception
        // if the path is invalid or not an object.
        return GetObject(path)->IsReadable();
    }
}

void D_Hierarchy::SetWriteable(const std::string& path, BOOL newWriteable)
{
    if (!enabled)
    {
        throw D_ExceptionNotEnabled();
    }

    // Get the object and set its writeable value.  Will throw an exception
    // if the path is invalid or not an object.
    GetObject(path)->SetWriteable(newWriteable);

#ifdef D_LISTENING_ENABLED
    // Let each object listener know a new object was added
    for (unsigned int i = 0; i < objectPermissionsListeners.size(); i++)
    {
        (*objectPermissionsListeners[i])(path);
    }
#endif
}

BOOL D_Hierarchy::IsWriteable(const std::string& path)
{
    if (!enabled)
    {
        throw D_ExceptionNotEnabled();
    }

#ifdef PARALLEL
    D_Level* level = GetLevelByPath(path);

    if (!IsLevelLocal(level))
    {
        return ParallelIsWriteable(level->GetPartition(), path);
    }
    else
#endif // PARALLEL
    {
        // Get the object and get its writeable value.  Will throw an exception
        // if the path is invalid or not an object.
        return GetObject(path)->IsWriteable();
    }
}

void D_Hierarchy::SetExecutable(const std::string& path, BOOL newExecutable)
{
    if (!enabled)
    {
        throw D_ExceptionNotEnabled();
    }

    // Get the object and set its executable value.  Will throw an exception
    // if the path is invalid or not an object.
    GetObject(path)->SetExecutable(newExecutable);

#ifdef D_LISTENING_ENABLED
    // Let each object listener know a new object was added
    for (unsigned int i = 0; i < objectPermissionsListeners.size(); i++)
    {
        (*objectPermissionsListeners[i])(path);
    }
#endif
}

BOOL D_Hierarchy::IsExecutable(const std::string& path)
{
    if (!enabled)
    {
        throw D_ExceptionNotEnabled();
    }

#ifdef PARALLEL
    D_Level* level = GetLevelByPath(path);

    if (!IsLevelLocal(level))
    {
        return ParallelIsExecutable(level->GetPartition(), path);
    }
    else
#endif // PARALLEL
    {
        // Get the object and get its executable value.  Will throw an exception
        // if the path is invalid or not an object.
        return GetObject(path)->IsExecutable();
    }
}

void D_Hierarchy::ReadAsString(
    const std::string& path,
    std::string& out)
{
    if (!enabled)
    {
        throw D_ExceptionNotEnabled();
    }

    D_Level* level = GetLevelByPath(path);

#ifdef PARALLEL
    if (!IsLevelLocal(level))
    {
        //printf("reading %s on partition %d\n", path.c_str(), level->GetPartition());
        assert(level->GetPartition() >= 0);

        ParallelRead(level->GetPartition(), path, out);
    }
    else
#endif // PARALLEL
    {
        // Get the object, throws D_ExceptionNotObject if it's not an object
        D_Object* object = level->GetObject();

        // Verify that the object is readable
        if (!object->IsReadable())
        {
            throw D_ExceptionNotReadable(object->GetFullPath());
        }

        // Call the object's ReadAsString function
        object->ReadAsString(out);
    }
}

void D_Hierarchy::WriteAsString(
    const std::string& path,
    const std::string& in)
{
    D_Level* level;
    D_Object* object;

    if (!enabled)
    {
        throw D_ExceptionNotEnabled();
    }

    // Get the level in the hierarchy
    level = GetLevelByPath(path);

#ifdef PARALLEL
    if (!IsLevelLocal(level))
    {
        ParallelWrite(level->GetPartition(), path, in);
    }
    else
#endif // PARALLEL
    {
        // Get the object, throws D_ExceptionNotObject if it's not an object
        object = level->GetObject();

        // Verify that the object is writeable
        if (!object->IsWriteable())
        {
            throw D_ExceptionNotWriteable(object->GetFullPath());
        }

        // Call the object's WriteAsString function
        object->WriteAsString(in);
    }
}

void D_Hierarchy::ExecuteAsString(
    const std::string& path,
    const std::string& in,
    std::string& out)
{
    D_Level* level;
    D_Object* object;

    if (!enabled)
    {
        throw D_ExceptionNotEnabled();
    }

    // Get the level in the hierarchy
    level = GetLevelByPath(path);

#ifdef PARALLEL
    if (!IsLevelLocal(level))
    {
        printf("executing %s on partition %d\n", path.c_str(), level->GetPartition());
        ParallelExecute(level->GetPartition(), path, in, out);
    }
    else
#endif // PARALLEL
    {
        // Get the object, throws D_ExceptionNotObject if it's not an object
        object = level->GetObject();

        // Call the object's ExecuteAsString function
        object->ExecuteAsString(in, out);
    }
}

BOOL D_Hierarchy::CreatePartitionPath(
    PartitionData* partition,
    const char* name,
    std::string& path)
{
    char intStr[MAX_STRING_LENGTH];

    if (!partitionEnabled || !enabled)
    {
        return FALSE;
    }

    // Create the path
    sprintf(intStr, "%d", partition->partitionId);
    path = std::string("/partition/") + intStr + std::string("/") + name;

    // Add the level.  Catch an exception if the level could not be added
    // and return FALSE.
    try
    {
#ifdef PARALLEL
        AddLevel(path, m_Partition->partitionId);
#else // PARALLEL
        AddLevel(path);
#endif // PARALLEL
    }
    catch (D_Exception)
    {
        return FALSE;
    }

    // Success, return TRUE
    return TRUE;
}

BOOL D_Hierarchy::CreateNodePath(
    Node* node,
    const char* name,
    std::string& path)
{
    if (!nodeEnabled || !enabled)
    {
        return FALSE;
    }
#ifdef USE_MPI
    if (node->partitionId != node->partitionData->partitionId)
    {
        return FALSE;
    }
#endif

    // Create the path
    char nodeIdStr[MAX_STRING_LENGTH];
    sprintf(nodeIdStr, "%d", node->nodeId);
    path = std::string("/node/") + nodeIdStr + "/" + name;

    // Add the level.  Catch an exception if the level could not be added
    // and return FALSE.
    try
    {
#ifdef PARALLEL
        AddLevel(path, m_Partition->partitionId);
#else // PARALLEL
        AddLevel(path);
#endif // PARALLEL
    }
    catch (D_Exception)
    {
        return FALSE;
    }

    // Success, return TRUE
    return TRUE;
}

BOOL D_Hierarchy::CreateNodeInterfacePath(
    Node* node,
    int interfaceIndex,
    const char* name,
    std::string& path)
{
    char addr[MAX_STRING_LENGTH];

    if (!phyEnabled || !enabled)
    {
        return FALSE;
    }
#ifdef USE_MPI
    if (node->partitionId != node->partitionData->partitionId)
    {
        return FALSE;
    }
#endif

    // Create the path
    IO_ConvertIpAddressToString(
        node->networkData.networkVar->interfaceInfo[interfaceIndex]->ipAddress, addr);

    // Create the path
    char str[MAX_STRING_LENGTH];
    sprintf(str, "%d", node->nodeId);
    path = std::string("/node/") + str + "/interface/" + addr + "/" + name;

    // Add the level.  Catch an exception if the level could not be added
    // and return FALSE.
    try
    {
#ifdef PARALLEL
        AddLevel(path, m_Partition->partitionId);
#else // PARALLEL
        AddLevel(path);
#endif // PARALLEL
    }
    catch (D_Exception)
    {
        return FALSE;
    }

    // Success, return TRUE
    return TRUE;
}

BOOL D_Hierarchy::CreateNodeInterfaceIndexPath(
    Node* node,
    int interfaceIndex,
    const char* name,
    std::string& path)
{
    if (!phyEnabled || !enabled)
    {
        return FALSE;
    }
#ifdef USE_MPI
    if (node->partitionId != node->partitionData->partitionId)
    {
        return FALSE;
    }
#endif

    // Create the path
    char str[MAX_STRING_LENGTH];
    sprintf(str, "%d/interface/index_%d/", node->nodeId, interfaceIndex);
    path = std::string("/node/") + str + name;

    // Add the level.  Catch an exception if the level could not be added
    // and return FALSE.
    try
    {
#ifdef PARALLEL
        AddLevel(path, m_Partition->partitionId);
#else // PARALLEL
        AddLevel(path);
#endif // PARALLEL
    }
    catch (D_Exception)
    {
        return FALSE;
    }

    // Success, return TRUE
    return TRUE;
}


BOOL D_Hierarchy::BuildNodeInterfaceIndexPath(
    Node* node,
    int interfaceIndex,
    const char* name,
    std::string& path)
{
    if (!phyEnabled || !enabled)
    {
        return FALSE;
    }

    // build the path
    char str[MAX_STRING_LENGTH];
    sprintf(str, "%d/interface/index_%d/", node->nodeId, interfaceIndex);
    path = std::string("/node/") + str + name;

    // Success, return TRUE
    return TRUE;
}

BOOL D_Hierarchy::CreateNodeInterfaceChannelPath(
    Node* node,
    int interfaceIndex,
    int channelIndex,
    const char* name,
    char* path)
{
    char addr[MAX_STRING_LENGTH];

    if (!phyEnabled || !enabled)
    {
        return FALSE;
    }
#ifdef USE_MPI
    if (node->partitionId != node->partitionData->partitionId)
    {
        return FALSE;
    }
#endif

    // Create the path
    IO_ConvertIpAddressToString(NetworkIpGetInterfaceAddress(node, interfaceIndex), addr);
    sprintf(path, "/node/%d/interface/%s/channel/%d/%s",
        node->nodeId,
        addr,
        channelIndex,
        name);

    // Add the level.  Catch an exception if the level could not be added
    // and return FALSE.
    try
    {
#ifdef PARALLEL
        AddLevel(path, m_Partition->partitionId);
#else // PARALLEL
        AddLevel(path);
#endif // PARALLEL
    }
    catch (D_Exception)
    {
        return FALSE;
    }

    // Success, return TRUE
    return TRUE;
}

BOOL D_Hierarchy::CreateExternalInterfacePath(
    const char* interfaceName,
    const char* name,
    std::string& path)
{
    if (!enabled)
    {
        return FALSE;
    }

    // Create the path
    path = std::string(interfaceName) + "/" + name;

    // Add the level.  Catch an exception if the level could not be added
    // and return FALSE.
    try
    {
#ifdef PARALLEL
        AddLevel(path, m_Partition->partitionId);
#else // PARALLEL
        AddLevel(path);
#endif // PARALLEL
    }
    catch (D_Exception)
    {
        return FALSE;
    }

    // Success, return TRUE
    return TRUE;
}

void D_Hierarchy::BuildApplicationPathString(
    NodeId nodeId,
    const std::string& app,
    int port,
    const std::string& name,
    std::string& path)
{
    if (!appEnabled || !enabled)
    {
        path = "";
        return;
    }

    char intString[MAX_STRING_LENGTH];
    sprintf(intString, "%d", nodeId);
    path = std::string("/node/") + intString + "/" + app + "/";
    sprintf(intString, "%d", port);
    path += std::string(intString) + "/" + name;
}
        
void D_Hierarchy::BuildApplicationServerPathString(
    NodeId nodeId,
    const std::string& app,
    NodeId sourceNodeId,
    int port,
    const std::string& name,
    std::string& path)
{
    if (!appEnabled || !enabled)
    {
        path = "";
        return;
    }

    char intString[MAX_STRING_LENGTH];
    sprintf(intString, "%d", nodeId);
    path = std::string("/node/") + intString + "/" + app + "/";
    sprintf(intString, "%d", sourceNodeId);
    path += std::string(intString) + "/" + name;
    sprintf(intString, "%d", port);
    path += std::string(intString) + "/" + name;
}

BOOL D_Hierarchy::CreateApplicationPath(
    Node* node,
    const char* app,
    int port,
    const char* name,
    std::string& path)
{
    if (!appEnabled || !enabled)
    {
        return FALSE;
    }
#ifdef USE_MPI
    if (node->partitionId != node->partitionData->partitionId)
    {
        return FALSE;
    }
#endif

    // Create the path
    BuildApplicationPathString(node->nodeId, app, port, name, path);
    if (path == "")
    {
        return FALSE;
    }

    // Add the level.  Catch an exception if the level could not be added
    // and return FALSE.
    try
    {
#ifdef PARALLEL
        AddLevel(path, m_Partition->partitionId);
#else // PARALLEL
        AddLevel(path);
#endif // PARALLEL
    }
    catch (D_Exception)
    {
        return FALSE;
    }

    // Success, return TRUE
    return TRUE;
}

BOOL D_Hierarchy::CreateTransportPath(
    Node* node,
    const char* protocol,
    const char* name,
    std::string& path)
{
    if (!transportEnabled || !enabled)
    {
        return FALSE;
    }
#ifdef USE_MPI
    if (node->partitionId != node->partitionData->partitionId)
    {
        return FALSE;
    }
#endif

    // Create the path
    char intString[MAX_STRING_LENGTH];
    sprintf(intString, "%d", node->nodeId);
    path = std::string("/node/") + intString + "/" + protocol + "/" + name;

    // Add the level.  Catch an exception if the level could not be added
    // and return FALSE.
    try
    {
#ifdef PARALLEL
        AddLevel(path, m_Partition->partitionId);
#else // PARALLEL
        AddLevel(path);
#endif // PARALLEL
    }
    catch (D_Exception)
    {
        return FALSE;
    }

    // Success, return TRUE
    return TRUE;
}

BOOL D_Hierarchy::CreateNetworkPath(
    Node* node,
    const char* protocol,
    const char* name,
    std::string& path)
{
    if (!networkEnabled || !enabled)
    {
        return FALSE;
    }
#ifdef USE_MPI
    if (node->partitionId != node->partitionData->partitionId)
    {
        return FALSE;
    }
#endif

    // Create the path
    char intStr[MAX_STRING_LENGTH];
    sprintf(intStr, "%d", node->nodeId);
    path = std::string("/node/") + intStr + "/" + protocol + "/" + name;

    // Add the level.  Catch an exception if the level could not be added
    // and return FALSE.
    try
    {
#ifdef PARALLEL
        AddLevel(path, m_Partition->partitionId);
#else
        AddLevel();
#endif
    }
    catch (D_Exception)
    {
        return FALSE;
    }

    // Success, return TRUE
    return TRUE;
}


BOOL D_Hierarchy::CreateNetworkPath(
    Node* node,
    int interfaceIndex,
    const char* protocol,
    const char* name,
    std::string& path)
{
    if (!networkEnabled || !enabled)
    {
        return FALSE;
    }
#ifdef USE_MPI
    if (node->partitionId != node->partitionData->partitionId)
    {
        return FALSE;
    }
#endif

    // Create the path
    char intStr[MAX_STRING_LENGTH];
    sprintf(intStr, "%d", node->nodeId);
    path = std::string("/node/") + intStr + "/" ;
    sprintf(intStr, "%d", interfaceIndex);
    path += std::string(intStr) + "/" + protocol + "/" + name;

    // Add the level.  Catch an exception if the level could not be added
    // and return FALSE.
    try
    {
#ifdef PARALLEL
        AddLevel(path, m_Partition->partitionId);
#else // PARALLEL
        AddLevel(path);
#endif // PARALLEL
    }
    catch (D_Exception)
    {
        return FALSE;
    }

    // Success, return TRUE
    return TRUE;
}

void CreateInterfaceLink(
    D_Hierarchy* hierarchy,
    char *nodeStr,
    char *addr)
{
    std::string path2;
    std::string path3;

    // Check if the links to this interface have been created
    path2 = std::string("/interface/") + addr;
    if (!hierarchy->IsValidPath(path2))
    {
        // Path doesn't exist, Create links!
#ifdef DEBUG
        printf("created path %s\n", path2.c_str());
#endif
        path3 = std::string("/node/") + nodeStr + "/interface/" + addr;
        hierarchy->AddLink(path2, path3);
    }
}

BOOL D_Hierarchy::CreateRoutingPath(
    Node* node,
    int phyIndex,
    const char* protocol,
    const char* name,
    std::string& path)
{
    char addr[MAX_STRING_LENGTH];
    char intStr[MAX_STRING_LENGTH];
    std::string path2;
    std::string path3;

    if (!networkEnabled || !enabled)
    {
        return FALSE;
    }
#ifdef USE_MPI
    if (node->partitionId != node->partitionData->partitionId)
    {
        return FALSE;
    }
#endif

    // Create the path
    IO_ConvertIpAddressToString(NetworkIpGetInterfaceAddress(node, phyIndex), addr);
    sprintf(intStr, "%d", node->nodeId);
    path = std::string("/node/")  + intStr + "/interface/" + addr + "/" + protocol + "/" + name;

    // Add the level.  Catch an exception if the level could not be added
    // and return FALSE.
    try
    {
#ifdef PARALLEL
        AddLevel(path, m_Partition->partitionId);
#else // PARALLEL
        AddLevel(path);
#endif // PARALLEL
    }
    catch (D_Exception)
    {
        return FALSE;
    }

    CreateInterfaceLink(
        this,
        intStr,
        addr);

    // Success, return TRUE
    return TRUE;
}

BOOL D_Hierarchy::CreateNetworkInterfacePath(
    Node* node,
    int phyIndex,
    const char* protocol,
    const char* name,
    std::string& path,
    BOOL isParameter)
{
    char addr[MAX_STRING_LENGTH];
    char intStr[MAX_STRING_LENGTH];
    char path2[MAX_STRING_LENGTH];
    char path3[MAX_STRING_LENGTH];

    if (!networkEnabled || !enabled)
    {
        return FALSE;
    }
#ifdef USE_MPI
    if (node->partitionId != node->partitionData->partitionId)
    {
        return FALSE;
    }
#endif

    // Create the path
    IO_ConvertIpAddressToString(NetworkIpGetInterfaceAddress(node, phyIndex), addr);
    sprintf(intStr, "%d", node->nodeId);
    if (isParameter)
    {
        path = std::string("/node/") + intStr + "/interface/" + addr + "/" + name;
    }
    else
    {
        path = std::string("/node/") + intStr + "/interface/" + addr + "/" + protocol + "/" + name;
    }

    // Add the level.  Catch an exception if the level could not be added
    // and return FALSE.
    try
    {
#ifdef PARALLEL
        AddLevel(path, m_Partition->partitionId);
#else // PARALLEL
        AddLevel(path);
#endif // PARALLEL
    }
    catch (D_Exception)
    {
        return FALSE;
    }

    // Check if the links to this interface have been created
    sprintf(path2, "/interface/%s", addr);
    try
    {
        GetLevelByPath(path2, FALSE);
    }
    catch (D_Exception)
    {
        // Path doesn't exist, Create links!
#ifdef DEBUG
        printf("created path %s\n", path2);
#endif
        sprintf(path3, "/node/%d/interface/%s", node->nodeId, addr);
        AddLink(path2, path3);
    }

    // Success, return TRUE
    return TRUE;
}

BOOL D_Hierarchy::CreateMacPath(
    Node* node,
    int phyIndex,
    const char* protocol,
    const char* name,
    std::string& path)
{
    char addr[MAX_STRING_LENGTH];
    char intStr[MAX_STRING_LENGTH];

    if (!macEnabled || !enabled)
    {
        return FALSE;
    }
#ifdef USE_MPI
    if (node->partitionId != node->partitionData->partitionId)
    {
        return FALSE;
    }
#endif

    // Create the path
    IO_ConvertIpAddressToString(NetworkIpGetInterfaceAddress(node, phyIndex), addr);
    sprintf(intStr, "%d", node->nodeId);
    path = std::string("/node/") + intStr + "/interface/" + addr + "/mac" + protocol + "/" + name;

    // Add the level.  Catch an exception if the level could not be added
    // and return FALSE.
    try
    {
#ifdef PARALLEL
        AddLevel(path, m_Partition->partitionId);
#else // PARALLEL
        AddLevel(path);
#endif // PARALLEL
    }
    catch (D_Exception)
    {
        return FALSE;
    }

    CreateInterfaceLink(
        this,
        intStr,
        addr);

    // Success, return TRUE
    return TRUE;
}

BOOL D_Hierarchy::CreatePhyPath(
    Node* node,
    int phyIndex,
    const char* protocol,
    const char* name,
    std::string& path)
{
    char addr[MAX_STRING_LENGTH];
    char intStr[MAX_STRING_LENGTH];

    if (!phyEnabled || !enabled)
    {
        return FALSE;
    }
#ifdef USE_MPI
    if (node->partitionId != node->partitionData->partitionId)
    {
        return FALSE;
    }
#endif

    // Create the path
    IO_ConvertIpAddressToString(NetworkIpGetInterfaceAddress(node, phyIndex), addr);
    sprintf(intStr, "%d", node->nodeId);
    path = std::string("/node/") + intStr + "/interface/" + addr + "/phy" + protocol + "/" + name;

    // Add the level.  Catch an exception if the level could not be added
    // and return FALSE.
    try
    {
#ifdef PARALLEL
        AddLevel(path, m_Partition->partitionId);
#else // PARALLEL
        AddLevel(path);
#endif // PARALLEL
    }
    catch (D_Exception)
    {
        return FALSE;
    }

    CreateInterfaceLink(
        this,
        intStr,
        addr);

    // Success, return TRUE
    return TRUE;
}


BOOL D_Hierarchy::CreatePhyInterfacePath(
    Node* node,
    int phyIndex,
    const char* protocol,
    const char* name,
    std::string& path,
    BOOL isParameter)
{
    char addr[MAX_STRING_LENGTH];
    char intStr[MAX_STRING_LENGTH];
    char path2[MAX_STRING_LENGTH];
    char path3[MAX_STRING_LENGTH];

    if (!phyEnabled || !enabled)
    {
        return FALSE;
    }
#ifdef USE_MPI
    if (node->partitionId != node->partitionData->partitionId)
    {
        return FALSE;
    }
#endif

    // Create the path
    IO_ConvertIpAddressToString(NetworkIpGetInterfaceAddress(node, phyIndex), addr);
    sprintf(intStr, "%d", node->nodeId);
    if (isParameter)
    {
        path = std::string("/node/") + intStr + "/interface/" + addr + "/" + name;
    }
    else
    {
        path = std::string("/node/") + intStr + "/interface/" + addr + "/" + protocol + "/" + name;
    }

    // Add the level.  Catch an exception if the level could not be added
    // and return FALSE.
    try
    {
#ifdef PARALLEL
        AddLevel(path, m_Partition->partitionId);
#else // PARALLEL
        AddLevel(path);
#endif // PARALLEL
    }
    catch (D_Exception)
    {
        return FALSE;
    }

    // Check if the links to this interface have been created
    sprintf(path2, "/interface/%s", addr);
    try
    {
        GetLevelByPath(path2, FALSE);
    }
    catch (D_Exception)
    {
        // Path doesn't exist, Create links!
        sprintf(path3, "/node/%d/interface/%s", node->nodeId, addr);
        AddLink(path2, path3);
    }

    // Success, return TRUE
    return TRUE;
}

BOOL D_Hierarchy::CreateQueuePath(
    Node* node,
    int interfaceIndex,
    int queueNum,
    const char* name,
    std::string& path)
{
    char intStr[MAX_STRING_LENGTH];
    char queueStr[MAX_STRING_LENGTH];
    char addr[MAX_STRING_LENGTH];

    if (!networkEnabled || !enabled)
    {
        return FALSE;
    }
#ifdef USE_MPI
    if (node->partitionId != node->partitionData->partitionId)
    {
        return FALSE;
    }
#endif

    // Create the path
    IO_ConvertIpAddressToString(NetworkIpGetInterfaceAddress(node, interfaceIndex), addr);
    sprintf(intStr, "%d", node->nodeId);
    sprintf(queueStr, "%d", queueNum);
    path = std::string("/node/") + intStr + "/interface/" + addr + "/queue/" + queueStr + "/" + name;

    // Add the level.  Catch an exception if the level could not be added
    // and return FALSE.
    try
    {
#ifdef PARALLEL
        AddLevel(path, m_Partition->partitionId);
#else // PARALLEL
        AddLevel(path);
#endif // PARALLEL
    }
    catch (D_Exception)
    {
        return FALSE;
    }

    // Success, return TRUE
    return TRUE;
}


BOOL D_Hierarchy::CreateGlobalDescriptorStatPath(
    const char* statName,
    const char* appName,
    char* path)
{
    // Create the path
    sprintf(path, "/stats/GlobalDescriptor/%s/%s", appName, statName);

    // Add the level.  Catch an exception if the level could not be added
    // and return FALSE.
    try
    {
#ifdef PARALLEL
        AddLevel(path, m_Partition->partitionId);
#else // PARALLEL
        AddLevel(path);
#endif // PARALLEL
    }
    catch (D_Exception)
    {
        return FALSE;
    }

    // Success, return TRUE
    return TRUE;
}


BOOL D_Hierarchy::CreateGlobalDescriptorMOPPath(
    const char* statName,
    const char* appName,
    char* path)
{
    // Create the path
    sprintf(path, "/MOPs/GlobalDescriptor/%s/%s", appName, statName);

    // Add the level.  Catch an exception if the level could not be added
    // and return FALSE.
    try
    {
#ifdef PARALLEL
        AddLevel(path, m_Partition->partitionId);
#else // PARALLEL
        AddLevel(path);
#endif // PARALLEL
    }
    catch (D_Exception)
    {
        return FALSE;
    }

    // Success, return TRUE
    return TRUE;
}


BOOL D_Hierarchy::CreateGlobalRadioStatPath(
    const char* statName,
    char* path)
{
    // Create the path
    sprintf(path, "/stats/GlobalRadio/%s", statName);

    // Add the level.  Catch an exception if the level could not be added
    // and return FALSE.
    try
    {
#ifdef PARALLEL
        AddLevel(path, m_Partition->partitionId);
#else // PARALLEL
        AddLevel(path);
#endif // PARALLEL
    }
    catch (D_Exception)
    {
        return FALSE;
    }

    // Success, return TRUE
    return TRUE;
}

BOOL D_Hierarchy::CreateGlobalNetworkConnectivityStatPath(
    const char* statName,
    char* path)
{
    // Create the path
    sprintf(path, "/stats/GlobalNetworkConnectivity/%s", statName);

    // Add the level.  Catch an exception if the level could not be added
    // and return FALSE.
    try
    {
#ifdef PARALLEL
        AddLevel(path, m_Partition->partitionId);
#else // PARALLEL
        AddLevel(path);
#endif // PARALLEL
    }
    catch (D_Exception)
    {
        return FALSE;
    }

    // Success, return TRUE
    return TRUE;
}


BOOL D_Hierarchy::CreateGlobalResourceUtilizationStatPath(
    const char* statName,
    char* path)
{
    // Create the path
    sprintf(path, "/stats/GlobalResourceUtilization/%s", statName);

    // Add the level.  Catch an exception if the level could not be added
    // and return FALSE.
    try
    {
#ifdef PARALLEL
        AddLevel(path, m_Partition->partitionId);
#else // PARALLEL
        AddLevel(path);
#endif // PARALLEL
    }
    catch (D_Exception)
    {
        return FALSE;
    }

    // Success, return TRUE
    return TRUE;
}


BOOL D_Hierarchy::CreateGlobalControlTrafficStatPath(
    const char* statName,
    char* path)
{
    // Create the path
    sprintf(path, "/stats/GlobalControlTraffic/%s", statName);

    // Add the level.  Catch an exception if the level could not be added
    // and return FALSE.
    try
    {
#ifdef PARALLEL
        AddLevel(path, m_Partition->partitionId);
#else // PARALLEL
        AddLevel(path);
#endif // PARALLEL
    }
    catch (D_Exception)
    {
        return FALSE;
    }

    // Success, return TRUE
    return TRUE;
}

BOOL D_Hierarchy::CreateGlobalRadioMOPPath(
    const char* statName,
    char* path)
{
    // Create the path
    sprintf(path, "/MOPs/GlobalRadio/%s", statName);

    // Add the level.  Catch an exception if the level could not be added
    // and return FALSE.
    try
    {
#ifdef PARALLEL
        AddLevel(path, m_Partition->partitionId);
#else // PARALLEL
        AddLevel(path);
#endif // PARALLEL
    }
    catch (D_Exception)
    {
        return FALSE;
    }

    // Success, return TRUE
    return TRUE;
}


BOOL D_Hierarchy::CreateGlobalNetworkConnectivityMOPPath(
    const char* statName,
    char* path)
{
    // Create the path
    sprintf(path, "/MOPs/GlobalNetworkConnectivity/%s", statName);

    // Add the level.  Catch an exception if the level could not be added
    // and return FALSE.
    try
    {
#ifdef PARALLEL
        AddLevel(path, m_Partition->partitionId);
#else // PARALLEL
        AddLevel(path);
#endif // PARALLEL
    }
    catch (D_Exception)
    {
        return FALSE;
    }

    // Success, return TRUE
    return TRUE;
}


BOOL D_Hierarchy::CreateGlobalResourceUtilizationMOPPath(
    const char* statName,
    char* path)
{
    // Create the path
    sprintf(path, "/MOPs/GlobalResourceUtilization/%s", statName);

    // Add the level.  Catch an exception if the level could not be added
    // and return FALSE.
    try
    {
#ifdef PARALLEL
        AddLevel(path, m_Partition->partitionId);
#else // PARALLEL
        AddLevel(path);
#endif // PARALLEL
    }
    catch (D_Exception)
    {
        return FALSE;
    }

    // Success, return TRUE
    return TRUE;
}

BOOL D_Hierarchy::CreateSubnetStatsPath(
    int subnetId,
    const char* statName,
    char* path)
{
    // Create the path
    sprintf(path, "/SubnetStats/%d/%s", subnetId, statName);

    // Add the level.  Catch an exception if the level could not be added
    // and return FALSE.
    try
    {
#ifdef PARALLEL
        AddLevel(path, m_Partition->partitionId);
#else // PARALLEL
        AddLevel(path);
#endif // PARALLEL
    }
    catch (D_Exception)
    {
        return FALSE;
    }

    // Success, return TRUE
    return TRUE;
}

BOOL D_Hierarchy::CreateNodeMobilityPath(
    Node* node,
    const char* name,
    std::string& path)
{
    char intStr[MAX_STRING_LENGTH];

#ifdef USE_MPI
    if (node->partitionId != node->partitionData->partitionId)
    {
        return FALSE;
    }
#endif

    // Create the path
    sprintf(intStr, "%d", node->nodeId);
    path = std::string("/node/") + intStr + "/mobility/" + name;

    // Add the level.  Catch an exception if the level could not be added
    // and return FALSE.
    try
    {
#ifdef PARALLEL
        AddLevel(path, m_Partition->partitionId);
#else // PARALLEL
        AddLevel(path);
#endif // PARALLEL
    }
    catch (D_Exception)
    {
        return FALSE;
    }

    // Success, return TRUE
    return TRUE;
}

BOOL D_Hierarchy::CreatePropagationPath(
    const char* name,
    int i,
    std::string& path)
{
    char intStr[MAX_STRING_LENGTH];

    // Create the path
    sprintf(intStr, "%d", i);
    path = std::string("/propagation/channel") + intStr + "/" + name;

    // Add the level.  Catch an exception if the level could not be added
    // and return FALSE.
    try
    {
#ifdef PARALLEL
        AddLevel(path, m_Partition->partitionId);
#else // PARALLEL
        AddLevel(path);
#endif // PARALLEL
    }
    catch (D_Exception)
    {
        return FALSE;
    }

    // Success, return TRUE
    return TRUE;
}

BOOL D_Hierarchy::CreateNodeMopPath(
    int nodeId,
    const char* name,
    std::string& path)
{
    char intStr[MAX_STRING_LENGTH];

    // Create the path
    sprintf(intStr, "%d", nodeId);
    path = std::string("/node/") + intStr + "/mop/" + name;

    // Add the level.  Catch an exception if the level could not be added
    // and return FALSE;
    try
    {
#ifdef PARALLEL
        AddLevel(path, m_Partition->partitionId);
#else // PARALLEL
        AddLevel(path);
#endif // PARALLEL
    }
    catch (D_Exception)
    {
        return FALSE;
    }

    // Success, return TRUE
    return TRUE;
}

// FOR MIBS STUFF
BOOL D_Hierarchy::CreateNodeMibsPath(
    Node* node,
    char* mibsId,
    char* name,
    char* path)
{
    // TODO: what if hierarchy not enabled?
#ifdef USE_MPI
    if (node->partitionId != node->partitionData->partitionId)
    {
        return FALSE;
    }
#endif

    // Create the path
    sprintf(path, "/node/%d/snmp/%s/%s", node->nodeId, name, mibsId);


    // Add the level.  Catch an exception if the level could not be added
    // and return FALSE.
    try
    {
#ifdef PARALLEL
        AddLevel(path, m_Partition->partitionId);
#else // PARALLEL
        AddLevel(path);
#endif // PARALLEL
    }
    catch (D_Exception)
    {
        return FALSE;
    }

    // Success, return TRUE
    return TRUE;
}


BOOL D_Hierarchy::CreateNodeInterfaceIndexPath(
    Node* node,
    int interfaceIndex,
    char* name,
    std::string& path)
{

    if (!phyEnabled || !enabled)
    {
        return FALSE;
    }
#ifdef USE_MPI
    if (node->partitionId != node->partitionData->partitionId)
    {
        return FALSE;
    }
#endif

    // Create the path
    char str[MAX_STRING_LENGTH];
    sprintf(str, "%d", node->nodeId);
    path = std::string("/node/") + str + "/";
    sprintf(str, "%d", interfaceIndex);
    path += std::string(str) + "/" + name;

    // Add the level.  Catch an exception if the level could not be added
    // and return FALSE.
    try
    {
#ifdef PARALLEL
        AddLevel(path, m_Partition->partitionId);
#else // PARALLEL
        AddLevel(path);
#endif // PARALLEL
    }
    catch (D_Exception)
    {
        return FALSE;
    }

    // Success, return TRUE
    return TRUE;
}

// END OF MIBS STUFF
#ifdef D_LISTENING_ENABLED
void D_Hierarchy::AddListener(
    const std::string& path,
    const std::string& listenerType,
    const std::string& arguments,
    const std::string& tag,
    D_ListenerCallback* callback)
{
    D_Level* level;
    D_Listener* listener;

    if (!enabled)
    {
        throw D_ExceptionNotEnabled();
    }

    // First get the path.  Throws D_ExceptionInvalidPath if invalid path.
    level = GetLevelByPath(path);

#ifdef PARALLEL
    callback->SetCallbackId(GetNewCallbackId());

    if (!IsLevelLocal(level))
    {
        ParallelListen(level->GetPartition(), path, listenerType, arguments, tag, callback->GetCallbackId());

        int id = callback->GetCallbackId();
#ifdef DEBUG_LISTEN
        printf("assigning callback id %d to listener for %s\n", id, path.c_str());
#endif
        callbackMap[id] = callback;
    }
    else
#endif // PARALLEL
    {
        // Throws D_ExceptionNotObject if the level is not an object
        D_Variable* variable = dynamic_cast<D_Variable*>(GetObject(path));

        if (listenerType == "" || listenerType == "listener")
        {
            listener = new D_Listener(
                variable,
                tag,
                callback);
        }
        else if (listenerType == "percent")
        {
            listener = new D_ListenerPercent(
                variable,
                arguments,
                tag,
                callback);
        }
        else
        {
            throw D_ExceptionUnknownListener(listenerType);
        }

        // Try setting the variable for this listener.  If an exception was
        // thrown it means that the variable cannot be used for this listener.
        // An example would be listening for a percentage change in a string.
        // If successful, add the listener to the object.
        level->GetObject()->AddListener(listener);
    }
}

void D_Hierarchy::AddObjectListener(D_ListenerCallback* callback)
{
    objectListeners.push_back(callback);
}

void D_Hierarchy::AddObjectPermissionsListener(D_ListenerCallback* callback)
{
    objectPermissionsListeners.push_back(callback);
}

void D_Hierarchy::AddLinkListener(D_ListenerCallback* callback)
{
    linkListeners.push_back(callback);
}

void D_Hierarchy::AddRemoveListener(D_ListenerCallback* callback)
{
    removeListeners.push_back(callback);
}

void D_Hierarchy::RemoveListeners(
    const std::string& path,
    const std::string& tag)
{
    D_Level* level;
    D_Object* object;

    if (!enabled)
    {
        throw D_ExceptionNotEnabled();
    }

    // First get the path.  Throws D_ExceptionInvalidPath if invalid path.
    level = GetLevelByPath(path);

    if (!IsLevelLocal(level))
    {
        ParallelRemoveListener(level->GetPartition(), path, tag);

        // TODO: remove from callback map
    }
    else
    {
        // Get the object.  Throws D_ExceptionNotObject if it's not an object.
        object = level->GetObject();

        // Remove all listeners with matching tags from the object
        unsigned int i = 0;
        while (i < object->listeners.size())
        {
            if (object->listeners[i]->GetTag() == tag)
            {
                object->RemoveListener(object->listeners[i]);
            }
            else
            {
                i++;
            }
        }
    }
}
#endif // D_LISTENING_ENABLED

#ifdef PARALLEL
int D_Hierarchy::GetNewCallbackId()
{
    return nextCallbackId++;
}
#endif

D_ObjectIterator::D_ObjectIterator(D_Hierarchy *h, std::string& path)
{
    m_Recursive = false;
    m_InRecursive = false;
    m_UseType = false;
    m_RecursiveTop = 0;
    m_Type = D_VARIABLE;

    // Count number of wildcard paths

    // First split up the path into levels then loop over each level
    StringSplit(path, "/", m_Levels);

    int numWildcards = 0;
    for (unsigned int i = 0; i < m_Levels.size(); i++)
    {
        if (m_Levels[i] == "*")
        {
            numWildcards++;
            m_WildcardStack.push_back(0);
        }
    }

    m_Hierarchy = h;
    m_Level = 0;
    m_CurrentLevel = h->root;
    m_WildcardLevel = -1;
}

void D_ObjectIterator::SetType(D_Type type)
{
    m_UseType = true;
    m_Type = type;
}

bool D_ObjectIterator::GetNextRecursive()
{
    // If we are at a child back up
    // If at the bottom (top == 0) then there is nothing to do we are done
    do
    {
        if (m_CurrentLevel->children.size() == 0)
        {
            m_RecursiveStack[m_RecursiveTop] = 0;
            m_RecursiveTop--;
            m_CurrentLevel = m_CurrentLevel->parent;
        }

        while (m_RecursiveTop >= 0)
        {
            if (m_CurrentLevel->children.size() == 0)
            {
                // Reached the leaf.  Either it is a matching object or we
                // need to fall back and check other leaves.
                if (m_CurrentLevel->object != NULL &&
                    (!m_UseType
                     || m_CurrentLevel->object->GetType() == m_Type))
                {
                    m_RecursiveStack[m_RecursiveTop]++;
                    return true;
                }
                else
                {
                    break;
                }
            }
            else if ((unsigned int)m_RecursiveStack[m_RecursiveTop]
                == m_CurrentLevel->children.size())
            {
                // Reached end of children for this level, back up
                m_RecursiveStack[m_RecursiveTop] = 0;
                if (m_RecursiveTop > 0)
                {
                    m_CurrentLevel = m_CurrentLevel->parent;   
                }
                m_RecursiveTop--;
            }
            else
            {
                // Go to next child
                m_CurrentLevel = m_CurrentLevel->children[m_RecursiveStack[m_RecursiveTop]];
                m_RecursiveStack[m_RecursiveTop]++;
                if ((unsigned int)m_RecursiveTop + 1 ==
                                                     m_RecursiveStack.size())
                {
                    m_RecursiveStack.push_back(0);
                }
                m_RecursiveTop++;
            }
        }

        // If top is less than 0 then we are done
        if (m_RecursiveTop < 0)
        {
            m_InRecursive = false;
            return false;
        }
    } while (m_CurrentLevel->object == NULL || (m_UseType && m_CurrentLevel->object->GetType() != m_Type));

    return !m_UseType || m_CurrentLevel->object->GetType() == m_Type;
}

bool D_ObjectIterator::GetNext()
{
    // My sincere apologies to anyone who has to work on this function, my
    // future self included.  This function implements a recursive search
    // through the dynamic hierarchy using a few stacks:
    //
    //     m_Levels: An array of each level: 0/1/2/3/4/etc.  The variable
    //         m_CurrentLevel is the current index of the level.
    //     m_WildcardStack: A stack of the level index of each wildcard.
    //         The path 0/*/2/3/* will have two entries.  On function entry
    //         the value m_WildcardLevel will be the index of the stack - 1.
    //     m_RecursiveStack: When using SetRecursive(true) this will
    //         maintain a stack of all sublevels of each matching level.

    if (m_Level < 0 || m_CurrentLevel == NULL)
    {
        return false;
    }

    if (m_InRecursive)
    {
        // When GetNextRecursive() returns false we are finished
        // with the current level
        if (GetNextRecursive())
        {
            return true;
        }
    }

    // If empty path then do recursive if recursive.  Otherwise return
    // false.
    if (m_Levels.size() == 0)
    {
        if (m_Recursive && m_RecursiveTop >= 0)
        {
            m_InRecursive = true;
            m_RecursiveStack.push_back(0);
            m_RecursiveTop = 0;
            return GetNextRecursive();
        }
        else
        {
            return false;
        }
    }

    do
    {
        // If we're at the end then rewind
        if ((unsigned int)m_Level == m_Levels.size())
        {
            m_Level--;
            while (1)
            {   
                if (m_Levels[m_Level] == "*")
                {
                    m_CurrentLevel = m_CurrentLevel->parent;
                    m_WildcardLevel--;
                    break;
                }

                m_Level--;
                if (m_Level < 0)
                {
                    return false;
                }

                m_CurrentLevel = m_CurrentLevel->parent;
            }
        }

        while ((unsigned int)m_Level < m_Levels.size())
        {
            if (m_Levels[m_Level] == "*")
            {
                m_WildcardLevel++;

                // At this point m_Level is the next level to look for and
                // m_WildcardLevel is the current wildcard
                if ((unsigned int)m_WildcardStack[m_WildcardLevel]
                    < m_CurrentLevel->children.size())
                {
                    m_CurrentLevel = m_Hierarchy->GetNextLevel(
                        m_CurrentLevel,
                        m_CurrentLevel->children[m_WildcardStack[m_WildcardLevel]]->name,
                        false);
                    m_Level++;  

                    m_WildcardStack[m_WildcardLevel]++;
                }
                else
                {
                    // Out of wildcards at this level.  If wildcard level is 0
                    // then we're done (out of wildcards).  Otherwise fall back
                    // to the previous wildcard.
                    if (m_WildcardLevel == 0)
                    {
                        m_Level = -1;
                        return false;
                    }
                    else
                    {
                        m_WildcardStack[m_WildcardLevel] = 0;
                        m_Level--;
                        m_WildcardLevel--;
                        while (1)
                        {
                            if (m_Levels[m_Level] == "*")
                            {
                                m_CurrentLevel = m_CurrentLevel->parent;
                                m_WildcardLevel--;
                                break;
                            }

                            m_Level--;
                            if (m_Level < 0)
                            {
                                return false;
                            }

                            m_CurrentLevel = m_CurrentLevel->parent;
                        }
                    }
                }
            }
            else
            {
                D_Level* nextLevel;
                nextLevel = m_Hierarchy->GetNextLevel(m_CurrentLevel, m_Levels[m_Level], true);

                if (nextLevel == NULL)
                {
                    // No next level, fall back to next wildcard or finish
                    while (1)
                    {
                        m_Level--;
                        if (m_Level < 0)
                        {
                            return false;
                        }

                        m_CurrentLevel = m_CurrentLevel->parent;
                        if (m_Levels[m_Level] == "*")
                        {
                            if (m_CurrentLevel != m_Hierarchy->root)
                            {
                                //m_CurrentLevel = m_CurrentLevel->parent;
                            }
                            m_WildcardLevel--;
                            break;
                        }
                    }
                }
                else
                {
                    m_CurrentLevel = nextLevel;
                    m_Level++;
                }
            }
        }

        if (m_CurrentLevel != NULL && m_Recursive)
        {
            m_InRecursive = true;
            if (m_RecursiveStack.size() == 0)
            {
                m_RecursiveStack.push_back(0);
            }
            else
            {
                m_RecursiveStack[0] = 0;
            }
            m_RecursiveTop = 0;
            return GetNextRecursive();
        }

        // Keep looping while we have a level but a null object
        // Also loop if we have an object but it doesn't match our type
    } while (m_CurrentLevel != NULL
        && (m_CurrentLevel->object == NULL
            || (m_UseType && m_CurrentLevel->object->GetType() != m_Type)));

    return m_CurrentLevel != NULL
        && m_CurrentLevel->object != NULL
        && (!m_UseType || m_CurrentLevel->object->GetType() == m_Type);
}

D_Object* D_ObjectIterator::GetObject()
{
    if (m_CurrentLevel)
    {
        return m_CurrentLevel->object;
    }
    else
    {
        return NULL;
    }
}
