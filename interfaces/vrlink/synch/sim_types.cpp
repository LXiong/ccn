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
// use in compliance with the license agreement as part of the EXata
// software.  This source code and certain of the algorithms contained
// within it are confidential trade secrets of Scalable Network
// Technologies, Inc. and may not be used as the basis for any other
// software, hardware, product or service.


#include <iostream>
#include "Config.h"
#include "synch_types.h"
#include "sim_types.h"
#include <sstream>
#include "vrlink.h"

namespace SNT_SYNCH
{

unsigned int Channel::nextID = 0;

Channel::Channel(std::string model, ForceIdentifierEnum8 force) :
        phyModelName(model),
        forceID(force)
{
    channelID = nextID++;
}

int Hierarchy::count = 0;
Hierarchy* Hierarchy::rootHierarchy = 0;
unsigned char Network::nextIPv4 = 0;

Network::Network(std::string mac,
                 std::string proto,
                 ForceIdentifierEnum8 force,
                 Hierarchy* hierarchy) :    hier(hierarchy),
                                            macProtocol(mac),
                                            forceID(force),
                                            protocol(proto),
                                            commonParameters(0)
{
    address = getNextAddress(protocol, hier->nodes.size());
}

void Network::addRouterModel(std::string routerModel)
{
    models.insert(routerModel);
}

std::string Network::getGroup(std::string model)
{
    int numNodes = 0;
    std::stringstream group;
    std::set<Node*>::iterator nit = nodes.begin();
    while (nit != nodes.end())
    {
        RadioStruct* radio = (*nit)->radio;
        if (radio)
        {
            ParameterMap* pm = Config::instance().getModelParameterList(
                                                    radio->radioSystemType);
            if (pm && pm->getParameter("ROUTER-MODEL").value == model)
            {
                if (numNodes == 0)
                {
                    group << "[";
                }
                else
                {
                    group << " ";
                }
                group << (*nit)->NodeId;
                numNodes++;
                break;
            }
        }
        nit++;
    }
    if (numNodes > 0)
    {
        group << "]";
    }

    return group.str();
}

std::string Network::getNextAddress(std::string protocol, size_t numberNodes)
{
    std::stringstream addr;
    //if (protocol == "IP")
    {
        addr << "N8-";
        addr << (unsigned int) Config::instance().networkPrefix;
        addr << ".1.";
        addr << (unsigned int) nextIPv4++;
        addr << ".0";
        return addr.str();
    }
}
void Hierarchy::setName(const char* str)
{
    const char *p = str;
    while (*p)
    {
        char c = *p;
        if (c == ' ')
        {
            c = '_';
        }
        name.push_back(c);
        p++;
    }
}
void Hierarchy::computeTerrainBounds()
{
    TerrainBounds bounds;
    bounds.computeTerrainBounds(&nodes);
    position.x = bounds.centerX;
    position.y = bounds.centerY;
    position.z = bounds.centerZ;
    if (bounds.eastLon > bounds.westLon)
    {
        dimensions.x = bounds.eastLon - bounds.westLon;
    }
    else
    {
        dimensions.x = bounds.westLon - bounds.eastLon;
    }
    dimensions.y = bounds.maxLat - bounds.minLat;
    dimensions.z = bounds.maxAlt - bounds.minAlt;
}

Network* NetworkSet::find(ForceIdentifierEnum8 forceID,
                          const std::string macProtocol,
                          const std::string networkProtocol,
                          Hierarchy* hier )
{
    Network dummy;
    dummy.macProtocol = macProtocol;
    dummy.protocol = networkProtocol;
    dummy.forceID = forceID;
    dummy.hier = hier;
    iterator it = std::set<Network*, Network::less>::find(&dummy);
    if (it != end())
    {
        return *it;
    }
    else
    {
        return 0;
    }
}

void NetworkSet::assignNodeToNetwork(Node* node,
                                     int interfaceNum,
                                     ParameterMap* pm)
{
    if (!pm)
    {
        return;
    }

    const Parameter &routerModelparm = pm->getParameter("ROUTER-MODEL");
    if (routerModelparm == Parameter::unknownParameter)
    {
        return;
    }
    const std::string& routerModel = routerModelparm.value;

    const Parameter &phyModelparm = pm->getParameter("PHY-MODEL");
    if (phyModelparm == Parameter::unknownParameter)
    {
        return;
    }
    const std::string& phyModel = phyModelparm.value;

    const Parameter &macProtocolparm = pm->getParameter("MAC-PROTOCOL");
    if (macProtocolparm == Parameter::unknownParameter)
    {
        return;
    }
    const std::string& macProtocol = macProtocolparm.value;

    const Parameter &networkProtocolparm =
                                        pm->getParameter("NETWORK-PROTOCOL");
    if (networkProtocolparm == Parameter::unknownParameter)
    {
        return;
    }
    const std::string& networkProtocol = networkProtocolparm.value;

    Network* net = 0;
    if (interfaceNum == 0)
    {
        net = find(node->entity->forceIdentifier, macProtocol,
                   networkProtocol, node->hier);
        if (!net)
        {
            std::cout << "creating new subnet for MAC protocol "
                << macProtocol << " NETWORK protocol " << networkProtocol
                << " Force " << node->entity->forceIdentifier
                << " hierarchy " << node->hier->name.c_str() << "(" << node->hier->id
                << ")" << std::endl << std::flush;
            net = new Network(macProtocol,
                              networkProtocol,
                              node->entity->forceIdentifier,
                              node->hier);
            node->hier->addNetwork(net);
            net->channel = channels->getChannel(node->entity->forceIdentifier,
                                                phyModel);
            insert(net);
        }
    }
    else
    {
        Hierarchy* hier = node->hier;
        while (!net && hier)
        {
            net = find(node->entity->forceIdentifier,
                       macProtocol,
                       networkProtocol,
                       hier);
            if (net && (net->nodes.find(node) != net->nodes.end()))
            {
                net = 0;
            }
            hier = hier->parent;
        }

        if (!net)
        {
            int i = 0;
            for (; i < interfaceNum; i++)
            {
                net = find(node->entity->forceIdentifier,
                           macProtocol,
                           networkProtocol,
                           getDummyHierarchy(i));
                if (!net)
                {
                    break;
                }
                else if (net->nodes.find(node) == net->nodes.end())
                {
                    break;
                }
            }
            if (!net)
            {
                std::cout << "creating new subnet for MAC protocol "
                    << macProtocol << " NETWORK protocol " << networkProtocol
                    << " Force " << node->entity->forceIdentifier
                    << " hierarchy " << node->hier->name.c_str() << "("
                    << node->hier->id << ")" << std::endl << std::flush;
                net = new Network(macProtocol,
                                  networkProtocol,
                                  node->entity->forceIdentifier,
                                  getDummyHierarchy(i));
                net->channel =
                    channels->getChannel(node->entity->forceIdentifier,
                                         phyModel);
                insert(net);
            }
        }
        std::cout << "adding node " << node->nodeName << "(" << node->NodeId
            << ") to subnet " << net->address << std::endl;
        net->addNode(node);
    }
    std::cout << "adding node " << node->nodeName << "(" << node->NodeId
        << ") to subnet " << net->address << std::endl;
    net->addNode(node);
    net->addRouterModel(routerModel);
}


void NodeSet::extractNodes(VRLinkExternalInterface* vrlink)
{
    AggregateEntityStruct* aggregate = NULL;
    EntityStruct* entity = NULL;
    RadioStruct* radio = NULL;

    do{
        aggregate = vrlink->GetNextAggregateStruct();
        if (aggregate)
        {
            updateAggregate(aggregate);
        }
    }while (aggregate != NULL);


    do{
        entity = vrlink->GetNextEntityStruct();
        if (entity)
        {
            updateEntity(entity);
        }
    }while (entity != NULL);

    do{
        radio = vrlink->GetNextRadioStruct();
        if (radio)
        {
            updateRadio(radio);
        }
    }while (radio != NULL);

}


void NodeSet::processExtractedNodes()
{
    if (Config::instance().allEntitiesHaveRadios)
    {
        createRadios();
    }
    purgeIncompleteNodes();
    assignHierarchies();
    assignNodeIds();
    computeTerrainBounds();
    createNetworks();
}



std::set<std::string>& NodeSet::modelsUsed()
{
    iterator it = begin();
    while (it != end())
    {
        RadioStruct* radio = (*it)->radio;
        if (radio)
        {
            RadioTypeStruct type = radio->radioSystemType;
            ParameterMap* pm = Config::instance().getModelParameterList(
                                                    radio->radioSystemType);
            if (pm)
            {
                const Parameter& p = pm->getParameter("ROUTER-MODEL");
                if (p != Parameter::unknownParameter)
                {
                    models.insert(p.value);
                }
            }
        }
        it++;
    }
    return models;
}
void NodeSet::updateAggregate(AggregateEntityStruct* aggregate)
{
    if (!aggregate)
    {
        return;
    }
    aggregates.insert(aggregate);
}

void NodeSet::updateEntity(EntityStruct* entity)
{
    if (!entity)
    {
        return;
    }
    bool flag = es.updateEntity(entity);

    // If flag evaluates to false, then this is a new entity.
    // In case of new entity, find a node in the NodeSet with entity id.
    // If no such node exists, then add a new node in NodeSet with NULL radio.
    // This will take care of entity with no radio.
    if (!flag)
    {
        Node* newNode = new Node;
        newNode->radioIndex = 0;
        newNode->id = entity->entityIdentifier;
        newNode->entity = entity;

        newNode->setIconName(Config::instance().
                                getIconForType(entity->entityType));
        iterator it = find(newNode);
        if (it == end())
        {
            newNode->radio = NULL;
            insert(newNode);
        }
    }
}
void NodeSet::updateRadio(RadioStruct* radio)
{
    if (!radio)
    {
        return;
    }
    Node* newNode = new Node;
    newNode->id = radio->entityId;
    newNode->radioIndex = radio->radioIndex;
    newNode->radio = radio;

    // Find a node in the NodeSet having same entityId and radioIndex
    // as that of the received radio.
    // 1. If such node exists, then update that node.
    // 2. If the entityId is same but radioIndex is different, then
    //    add a new node in the NodeSet.
    // 3. If entityId is same but its radio is NULL, then update this node.
    iterator it = find(newNode);
    if (it == end())
    {
        SNT_SYNCH::EntityStruct* existingEntity =
                                        es.findByEntityID(radio->entityId);
        newNode->entity = existingEntity;
        insert(newNode);
    }
    else
    {
        delete newNode;
        (*it)->radio = radio;
        (*it)->radioIndex = radio->radioIndex;
        incrementRadioUpdateCount();
    }
}

unsigned int NodeSet::getRadioAddCount()
{
    iterator it = begin();
    unsigned int radioCount = 0;

    for (; it != end(); it++)
    {
        if ((*it)->radio)
        {
            radioCount = radioCount + 1;
        }
    }
    return radioCount;
}



unsigned int NodeSet::getRadioUpdateCount()
{
    return radioUpdateCount;
}

void NodeSet::incrementRadioUpdateCount()
{
    radioUpdateCount++;
}


Node* NodeSet::findByNodeID(int nodeId)
{
    iterator it = begin();
    while (it != end())
    {
        if ((*it)->entity && (*it)->NodeId == nodeId)
        {
            return *it;
        }
        it++;
    }
    return 0;
}

Node* NodeSet::findByObjectName(std::string objName)
{
    iterator it = begin();
    std::string myName;

    while (it != end())
    {
        if ((*it)->entity && (*it)->entity->myName)
        {
            myName = (*it)->entity->myName;
            if (myName == objName)
            {
                return *it;
            }
        }
        it++;
    }
    return 0;
}

Network* NodeSet::getNetwork(Node* node, RadioStruct* radio)
{
    ParameterMap* pm = Config::instance().getModelParameterList(
                                                     radio->radioSystemType);
    if (!pm)
    {
        pm = Config::instance().getModelParameterList(
                                      Config::instance().defaultRouterModel);
    }
    if (!pm)
    {
        return 0;
    }
    ForceIdentifierEnum8 forceID = node->entity->forceIdentifier;
    const std::string macProtocol = pm->getParameter("MAC-PROTOCOL").value;
    const std::string networkProtocol =
                                  pm->getParameter("NETWORK-PROTOCOL").value;
    Hierarchy* hier = node->hier;
    return networks.find(forceID, macProtocol, networkProtocol, hier);
}

Network* NodeSet::getNetwork(const std::string& addr)
{
    NetworkSet::iterator it = networks.begin();
    while (it != networks.end())
    {
        if ((*it)->address == addr)
        {
            return *it;
        }
        it++;
    }
    return 0;
}
std::string NodeSet::getNetworkAddress(Node* node, RadioStruct* radio)
{
    Network* net = getNetwork(node, radio);
    if (net)
    {
        return net->address;
    }
    else
    {
        return "";
    }
}
bool NodeSet::usesSlots()
{
    std::set<std::string>::iterator it = modelsUsed().begin();
    while (it != modelsUsed().end())
    {
        ParameterMap* parameters =
                               Config::instance().getModelParameterList(*it);
        const Parameter& model = parameters->getParameter("MAC-PROTOCOL");
        if (model != Parameter::unknownParameter)
        {
            if (model.value == "MAC-LINK-16")
            {
                return true;
            }
        }
        it++;
    }
    return false;
}

void NodeSet::createRadios()
{
    iterator it = begin();
    while (it != end())
    {
        if ((*it)->radio == NULL)
        {
            RadioStruct* radio = new RadioStruct;
            radio->entityId = (*it)->id;
            radio->radioIndex = 0;
            (*it)->radio = radio;
        }
        it++;
    }
}

void NodeSet::assignHierarchies()
{
    std::set<AggregateEntityStruct*,AggregateEntityStruct::less>::iterator it
                                                        = aggregates.begin();
    while (it != aggregates.end())
    {
        AggregateEntityStruct* ae = *it;
        Hierarchy* hier = new Hierarchy();
        hier->position.x = ae->worldLocation.lon;
        hier->position.y = ae->worldLocation.lat;
        hier->position.z = ae->worldLocation.alt;
        hier->dimensions.x = ae->dimensions.xAxisLength;
        hier->dimensions.y = ae->dimensions.yAxisLength;
        hier->dimensions.z = ae->dimensions.zAxisLength;
        hier->setName((char *)ae->aggregateMarking.markingData);
        unsigned int i = 0;

        std::string ids;
        std::string token;

        if ((*it)->containedEntityIDs)
        {
            ids = (*it)->containedEntityIDs;
        }

        while (ids.find(",", 0) != std::string::npos)
        {
            // does the string contain comma in it?
            size_t  pos = ids.find(",", 0);  //store the position of the delimiter
            token = ids.substr(0, pos);      //get the token
            ids.erase(0, pos + 1);           //erase it from the source

            Node* node = findByObjectName(token);
            if (node)
            {
                node->hier = hier;
                hier->addNode(node);
            }
        }
        // for the last token of entity id
        if (ids != "")
        {
            Node* node = findByObjectName(ids);
            if (node)
            {
                node->hier = hier;
                hier->addNode(node);
            }
        }

        if (hier->nodes.size())
        {
            hierarchies[*it] = hier;
        }
        else
        {
            delete hier;
        }
        it++;
    }
    it = aggregates.begin();
    while (it != aggregates.end())
    {
        AggregateEntity2HierarchyMap::iterator ait = hierarchies.find(*it);
        if (ait == hierarchies.end())
        {
            it++;
            continue;
        }
        Hierarchy* parent = ait->second;
        if (!parent)
        {
            it++;
            continue;
        }

        std::string ids;
        std::string token;

        if ((*it)->subAggregateIDs)
        {
            ids = (*it)->subAggregateIDs;
        }

        while (ids.find(",", 0) != std::string::npos)
        {
            // does the string contain comma in it?
            size_t  pos = ids.find(",", 0);  //store the position of the delimiter
            token = ids.substr(0, pos);      //get the token
            ids.erase(0, pos + 1);           //erase it from the source

            Hierarchy* hier = hierarchies.findByID(token);
            parent->children.insert(hier);
            hier->parent = parent;
        }
        // for the last token of subAggregate id
        if (ids != "")
        {
            Hierarchy* hier = hierarchies.findByID(ids);
            parent->children.insert(hier);
            hier->parent = parent;
        }

        it++;
    }
    Hierarchy* rootHierarchy = new Hierarchy(Hierarchy::Root);
    AggregateEntity2HierarchyMap::iterator hit = hierarchies.begin();
    while (hit != hierarchies.end())
    {
        if (!hit->second->parent)
        {
            rootHierarchy->children.insert(hit->second);
            hit->second->parent = rootHierarchy;
        }
        hit++;
    }
    iterator nit = begin();
    while (nit != end())
    {
        if (!(*nit)->hier)
        {
            rootHierarchy->addNode(*nit);
            (*nit)->hier = rootHierarchy;
        }
        nit++;
    }
}

void NodeSet::purgeIncompleteNodes()
{
    iterator it = begin();
    while (it != end())
    {
        if (!(*it)->entity || !(*it)->radio)
        {
            iterator deadManWalking = it;
            it++;
            erase(deadManWalking);
        }
        else if ((*it)->entity->marking.markingData[0] == 0)
        {
            iterator deadManWalking = it;
            it++;
            erase(deadManWalking);
        }
        else
        {
            it++;
        }
    }
}
void NodeSet::assignNodeIds()
{
    std::map<int, Node*> assignedNodes;
    iterator it = begin();
    while (it != end())
    {
        if ((*it)->NodeId != -1)
        {
            assignedNodes[(*it)->NodeId] = *it;
        }
        it++;
    }
    int Id = Config::instance().firstNodeId;
    it = begin();
    while (it != end())
    {
        if ((*it)->NodeId == -1)
        {
            while (assignedNodes.find(Id) != assignedNodes.end())
            {
                Id++;
            }
            (*it)->NodeId = Id++;
            assignedNodes[(*it)->NodeId] = *it;
        }
        it++;
    }
}

void  NodeSet::computeTerrainBounds()
{
    TerrainBounds bounds;
    bounds.computeTerrainBounds(this);

    std::stringstream swCorner;
    swCorner.precision(9);
    swCorner << "( " << bounds.minLat << ", " << bounds.westLon << " )";
    std::stringstream neCorner;
    neCorner.precision(9);
    neCorner << "( " << bounds.maxLat << ", " << bounds.eastLon << " )";
    Config::instance().setParameter(Parameter("TERRAIN-SOUTH-WEST-CORNER",
                                    swCorner.str()));
    Config::instance().getParameter("TERRAIN-NORTH-EAST-CORNER");
    Config::instance().setParameter(Parameter("TERRAIN-NORTH-EAST-CORNER",
                                    neCorner.str()));
}

void NodeSet::createNetworks()
{
    iterator it = begin();
    while (it != end())
    {
        Node* node = *it;
        int interfaceNum = 0;
        RadioStruct* radio = node->radio;
        if (radio)
        {
            RadioTypeStruct type = radio->radioSystemType;
            ParameterMap* pm = Config::instance().getModelParameterList(
                                                        radio->radioSystemType);
            networks.assignNodeToNetwork(node, interfaceNum, pm);

            interfaceNum++;
        }
        it++;
    }
}

bool EntitySet::updateEntity(EntityStruct* entity)
{
    // If no such entity exists, then add this new entity in EntitySet.
    // Otherwise update the existing entity.
    if (find(entity) == end())
    {
        insert(entity);
        return false;
    }
    else
    {
        incrementEntityUpdateCount();
        return true;
    }
}

EntityStruct* EntitySet::findByEntityID(EntityIdentifierStruct entIdentifier)
{
    EntityStruct* e1 = new EntityStruct();
    e1->entityIdentifier = entIdentifier;
    iterator it = find(e1);
    return *it;
}

unsigned int EntitySet::getEntityUpdateCount()
{
    return entityUpdateCount;
}

void EntitySet::incrementEntityUpdateCount()
{
    entityUpdateCount++;
}
};
