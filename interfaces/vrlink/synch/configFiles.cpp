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

#include "archspec.h"
#include "configFiles.h"
#include "Config.h"

#include <fstream>
#include <sstream>
#include <string.h>
#include <errno.h>

using namespace SNT_SYNCH;

const unsigned short SYNCH_COUNTRYCODE_OTHER = 0;
const unsigned short SYNCH_COUNTRYCODE_AUS   = 13;
const unsigned short SYNCH_COUNTRYCODE_CAN   = 39;
const unsigned short SYNCH_COUNTRYCODE_FRA   = 71;
const unsigned short SYNCH_COUNTRYCODE_CIS   = 222;
const unsigned short SYNCH_COUNTRYCODE_GBR   = 224;
const unsigned short SYNCH_COUNTRYCODE_USA   = 225;

std::string getCountryString(unsigned short countryCode)
{
    switch (countryCode)
    {
    case SYNCH_COUNTRYCODE_AUS: return "AUS";
    case SYNCH_COUNTRYCODE_CAN: return "CAN";
    case SYNCH_COUNTRYCODE_FRA: return "FRA";
    case SYNCH_COUNTRYCODE_CIS: return "CIS";
    case SYNCH_COUNTRYCODE_GBR: return "GBR";
    case SYNCH_COUNTRYCODE_USA: return "USA";
    default:
        std::stringstream code;
        code << (unsigned int) countryCode;
        return code.str();
    }
}


void ConfigFileWriter::writeEntitiesFile(std::string filename, NodeSet& ns)
{
    std::fstream out;
    out.open(filename.c_str(), std::fstream::out | std::fstream::trunc);

    EntitySet entitySet = ns.getEntitySet();
    EntitySet::iterator it = entitySet.begin();
    while (it != entitySet.end())
    {
        EntityStruct* ent = *it;
        if (ent)
        {
            int len = strlen((char*)ent->marking.markingData);
            if (len == 0 || len > 11)
            {
                it++;
                continue;
            }
            std::stringstream text;
            text << ent->marking << ", ";
            text << ent->forceIdentifier << ", ";
            text << getCountryString(ent->entityType.countryCode) << ", ";
            text << ent->worldLocation << ", ";
            text << ent->entityType;
            out << ent->marking << ", ";

            if (ent->forceIdentifier ==
                                    SNT_SYNCH::ForceIdentifierEnum::Friendly)
            {
                out << "F" << ", ";
            }
            else if (ent->forceIdentifier ==
                                    SNT_SYNCH::ForceIdentifierEnum::Opposing)
            {
                out << "O" << ", ";
            }
            else
            {
                out << "N" << ", ";
            }
            out << getCountryString(ent->entityType.countryCode) << ", ";
            out << ent->worldLocation << ", ";
            out << ent->entityType;
            out << std::endl;
        }
        it++;
    }
    out.close();
}

void ConfigFileWriter::createScenarioDir(std::string scenarioPath)
{
    size_t pos = 1;
    while (pos != scenarioPath.npos)
    {
        pos = scenarioPath.find(Config::instance().dirSep, pos+1);
        std::string dir = scenarioPath.substr(0,pos);
        _mkdir(dir.c_str());
    }
}

void ConfigFileWriter::writeRadiosFile(std::string filename, NodeSet& ns)
{
    std::fstream out;
    out.open(filename.c_str(), std::fstream::out | std::fstream::trunc);

    NodeSet::iterator it =  ns.begin();
    while (it != ns.end())
    {
        EntityStruct* ent = (*it)->entity;
        int len = strlen((char*)ent->marking.markingData);
        if (len == 0 || len > 11)
        {
            it++;
            continue;
        }

        RadioStruct* radio = (*it)->radio;
        if (radio)
        {
            out << (*it)->NodeId << ", ";
            out << ent->marking << ", ";
            out << radio->radioIndex << ", ";
            out << radio->relativePosition << ", ";
            out << radio->radioSystemType ;
            out << std::endl;
        }
        it++;
    }
    out.close();
}

void ConfigFileWriter::writeNetworksFile(std::string filename, NodeSet& ns)
{
    std::fstream out;
    out.open(filename.c_str(), std::fstream::out | std::fstream::trunc);

    NodeSet::iterator it =  ns.begin();
    out << "N1;3000000000;";
    while (it != ns.end())
    {
        out << (*it)->NodeId;
        it++;
        if (it != ns.end())
        {
            out << ", ";
        }
    }
    out << ";255.255.255.255";
    out.close();
}
void ConfigFileWriter::writeNodesFile(std::string filename, NodeSet& ns)
{
    std::fstream out;
    out.open(filename.c_str(), std::fstream::out | std::fstream::trunc);
    out.precision(10);

    NodeSet::iterator it =  ns.begin();
    while (it != ns.end())
    {
        EntityStruct* ent = (*it)->entity;

        double lat = ent->worldLocation.lat;
        double lon = ent->worldLocation.lon;

        if (fabs(lat) > 90.0 || fabs(lon) > 180.0)
        {
            lat = 0.0;
            lon = 0.0;
        }

        RadioStruct* radio = (*it)->radio;
        if (radio)
        {
            double lat, lon, alt;
            lat = radio->absoluteNodePosInLatLonAlt.lat;
            lon = radio->absoluteNodePosInLatLonAlt.lon;
            alt = radio->absoluteNodePosInLatLonAlt.alt;
            out << (*it)->NodeId << " 0 ";
            out << "(" << lat << ", " << lon << ", " << alt << ") ";
            out << ent->orientation.azimuth << " ";
            out << ent->orientation.elevation;
            out << std::endl;
        }
        it++;
    }
    out.close();
}
void ConfigFileWriter::writeSlotsFile(std::ostream& configFile,
                                      Config& config,
                                      NodeSet& ns)
{
    const Parameter &slotParm = config.getParameter("LINK-16-SLOT-FILE");
    if (ns.usesSlots() && slotParm != Parameter::unknownParameter)
    {
        configFile << slotParm << std::endl;

        std::string filename = slotParm.value;
        std::fstream out;
        out.open(filename.c_str(), std::fstream::out | std::fstream::trunc);
    }
}
void ConfigFileWriter::writeRouterModelFile(std::string filename,
                                            NodeSet& ns)
{
    std::fstream out;

    std::set<std::string>::iterator it = ns.modelsUsed().begin();

    if (it != ns.modelsUsed().end())
    {
        out.open(filename.c_str(), std::fstream::out | std::fstream::trunc);
    }

    while (it != ns.modelsUsed().end())
    {
        ParameterMap* parameters = Config::instance().getModelParameterList(*it);
        const Parameter& model = parameters->getParameter("ROUTER-MODEL");
        if (model != Parameter::unknownParameter)
        {
            out << model << std::endl;
            ParameterMap::iterator pit = parameters->begin();
            while (pit != parameters->end())
            {
                if (pit->first != "ROUTER-MODEL")
                    out << pit->second << std::endl;
                pit++;
            }
            out << std::endl;
        }
        it++;
    }
}

void writeGeneralSection(std::ostream& out, Config& config)
{
    out << "# General" << std::endl;
    out << config.getParameter("VERSION") << std::endl;
    out << config.getParameter("EXPERIMENT-NAME") << std::endl;
    out << config.getParameter("SIMULATION-TIME") << std::endl;
    out << config.getParameter("SEED") << std::endl;
    out << std::endl;
}

void writeTerrainSection(std::ostream& out, Config& config)
{
    out << "# Terrain" << std::endl;
    out << config.getParameter("COORDINATE-SYSTEM") << std::endl;
    out << config.getParameter("TERRAIN-SOUTH-WEST-CORNER") << std::endl;
    out << config.getParameter("TERRAIN-NORTH-EAST-CORNER") << std::endl;
    out << config.getParameter("TERRAIN-DATA-BOUNDARY-CHECK") << std::endl;
    out << std::endl;
}

void writePositionSection(std::ostream& out, Config& config)
{
    out << "# Node positions" << std::endl;
    out << config.getParameter("NODE-PLACEMENT") << std::endl;
    out << config.getParameter("NODE-POSITION-FILE") << std::endl;
    out << std::endl;
}

void writeMobilitySection(std::ostream& out, Config& config)
{
    out << "# Mobility" << std::endl;
    out << config.getParameter("MOBILITY") << std::endl;
    out << config.getParameter("MOBILITY-POSITION-GRANULARITY") << std::endl;
    out << config.getParameter("MOBILITY-GROUND-NODE") << std::endl;
    out << std::endl;
}

void writeProgagationSection(std::ostream& out,
                             Config& config,
                             ChannelSet& channels)
{
    out << "# Propagation" << std::endl;
    for (size_t i=0; i<channels.size(); i++)
    {
        std::stringstream channel;
        channel << "[" << i << "]";
        out << config.getParameter(
            std::string("PROPAGATION-CHANNEL-FREQUENCY") + channel.str())
            << std::endl;
        out << config.getParameter(
            std::string("PROPAGATION-LIMIT") + channel.str())
            << std::endl;
        out << config.getParameter(
            std::string("PROPAGATION-PATHLOSS-MODEL") + channel.str())
            << std::endl;
        out << config.getParameter(
            std::string("PROPAGATION-SHADOWING-MODEL") + channel.str())
            << std::endl;
        out << config.getParameter(
            std::string("PROPAGATION-SHADOWING-MEAN") + channel.str())
            << std::endl;
        out << config.getParameter(
            std::string("PROPAGATION-FADING-MODEL") + channel.str())
            << std::endl;
        out << std::endl;
    }
}
void writeParameters(std::ostream& out,
                     std::string grouping,
                     ParameterMap* parameters)
{
    ParameterMap::iterator pit = parameters->begin();
    while (pit != parameters->end())
    {
        if (!grouping.empty())
        {
            out << grouping << " ";
        }
        out << pit->second << std::endl;
        pit++;
    }
    out << std::endl;
}
void writePhysicalSection(std::ostream& out, Config& config, NodeSet& ns)
{
    out << "# Physical" << std::endl;
    NetworkSet& nets = ns.networksUsed();

    std::string defaultModel =
                    config.getParameter("VRLINK-DEFAULT-ROUTER-MODEL").value;

    std::set<std::string> models;
    NetworkSet::iterator nit = nets.begin();
    while (nit != nets.end())
    {
        if ((*nit)->models.size() == 0)
        {
            models.insert(defaultModel);
        }
        else
        {
            models.insert((*nit)->models.begin(), (*nit)->models.end());
        }
        nit++;
    }
    if (models.size() == 1)
    {
        ParameterMap* parameters = config.getModelParameterList(*models.begin());
        if (parameters)
        {
            writeParameters(out, "", parameters);
        }
    }
    else if (models.size() > 1)
    {
        nit = nets.begin();
        while (nit != nets.end())
        {
            Network* n = *nit;
            ParameterMap* common = n->getCommonParameters();
            std::stringstream subnet;
            subnet << "[" << n->address << "]";
            writeParameters(out, subnet.str(), common);

            std::set<std::string>::iterator mit = n->models.begin();
            while (mit != n->models.end())
            {
                ParameterMap* parameters = config.getModelParameterList(*mit);
                ParameterMap* specfic = *parameters - *common;
                std::string group = n->getGroup(*mit);
                if (parameters && !group.empty())
                {
                    writeParameters(out, group, specfic);
                }
                delete specfic;
                mit++;
            }
            nit++;
        }
    }

    std::set<std::string>::iterator it = ns.modelsUsed().begin();
    if (it != ns.modelsUsed().end())
    {
        out << config.getParameter("ROUTER-MODEL-CONFIG-FILE") << std::endl;
    }
}

void writeStatisticsSection(std::ostream& out, Config& config)
{
    out << "# Statistics" << std::endl;
    std::vector<Parameter> parameters =
                      config.getParameterList("-STATISTICS", Config::SUFFIX);
    for (size_t i=0; i<parameters.size(); i++)
    {
        const std::string pn = parameters[i].name;
        if (parameters[i].name.find("VRLINK-HLA-") != 0)
        {
            out << parameters[i] << std::endl;
        }
    }
    out << std::endl;
}
void writeNetworksSection(std::ostream& out, NodeSet& ns)
{
    out << "# Networks" << std::endl;
    NetworkSet& nets = ns.networksUsed();
    NetworkSet::iterator it = nets.begin();
    while (it != nets.end())
    {
        Network* n = *it;
        out << "SUBNET " << n->address;
        out << " { ";
        std::set<Node*>::iterator nit = n->nodes.begin();
        double lat = 0.0;
        double lon = 0.0;
        double alt = 0.0;
        while (nit != n->nodes.end())
        {
            out << (*nit)->NodeId << " ";
            lat += (*nit)->entity->worldLocation.lat;
            lon += (*nit)->entity->worldLocation.lon;
            alt += (*nit)->entity->worldLocation.alt;
            nit++;
        }
        lat = lat / (1.0*n->nodes.size());
        lon = lon / (1.0*n->nodes.size());
        alt = alt / (1.0*n->nodes.size());
        out << "} " << lat << " " << lon << " " << alt << std::endl;
        std::stringstream mask;
        ChannelSet::iterator cit = ns.channelsUsed().begin();
        while (cit != ns.channelsUsed().end())
        {
            if (*cit == n->channel)
            {
                mask << "1";
            }
            else
            {
                mask << "0";
            }
            cit++;
        }
        out << "[ " << n->address << "] PHY-LISTENABLE-CHANNEL-MASK "
            << mask.str() << std::endl;
        out << "[ " << n->address << "] PHY-LISTENING-CHANNEL-MASK "
            << mask.str() << std::endl;
        out << std::endl;
        it++;
    }
}

void writeHostnameSection(std::ostream& out, NodeSet& ns)
{
    NodeSet::iterator it = ns.begin();
    while (it != ns.end())
    {
        out << "[" << (*it)->NodeId << "] HOSTNAME\t\""
            << (*it)->entity->marking.markingData << "\"" << std::endl;
        it++;
    }
    out << std::endl;
}

std::set<std::string> iconNames;

void writeNodeIconSection(std::ostream& out, NodeSet& ns, Config& config)
{
    iconNames.clear();
    NodeSet::iterator it = ns.begin();
    while (it != ns.end())
    {
        EntityTypeStruct type = (*it)->entity->entityType;
        const std::string& icon = (*it)->getIconName();
        if (!icon.empty())
        {
            std::string filename = config.getFileName(icon);
            out << "[" << (*it)->NodeId << "] NODE-ICON";
            out << "\t\ticons" << config.dirSep << filename << std::endl;
            iconNames.insert(icon);
        }
        it++;
    }
    out << std::endl;
}
void writeHLASection(std::ostream& out, Config& config)
{
    out << "# HLA" << std::endl;
    out << config.getParameter("VRLINK") << std::endl;
    out << config.getParameter("VRLINK-PROTOCOL") << std::endl;

    out << config.getParameter("VRLINK-FEDERATION-NAME") << std::endl;
    out << config.getParameter("VRLINK-FED-FILE-PATH") << std::endl;
    out << config.getParameter("VRLINK-FEDERATE-NAME") << std::endl;
    out << config.getParameter("VRLINK-RPR-FOM-VERSION") << std::endl;

    out << config.getParameter("VRLINK-ENTITIES-FILE-PATH") << std::endl;
    out << config.getParameter("VRLINK-RADIOS-FILE-PATH") << std::endl;
    out << config.getParameter("VRLINK-NETWORKS-FILE-PATH") << std::endl;

    out << config.getParameter("VRLINK-DEBUG") << std::endl;
    out << config.getParameter("VRLINK-DEBUG-2") << std::endl;
    out << config.getParameter("VRLINK-TICK-INTERVAL") << std::endl;
    out << config.getParameter("VRLINK-XYZ-EPSILON") << std::endl;
    out << config.getParameter("VRLINK-MOBILITY-INTERVAL") << std::endl;
    out << config.getParameter("VRLINK-HLA-DYNAMIC-STATISTICS") << std::endl;
    out << config.getParameter(
        "VRLINK-HLA-DYNAMIC-STATISTICS-METRIC-UPDATE-MODE") << std::endl;
    out << config.getParameter(
        "VRLINK-HLA-DYNAMIC-STATISTICS-SEND-NODEID-DESCRIPTIONS")<<std::endl;
    out << config.getParameter(
        "VRLINK-HLA-DYNAMIC-STATISTICS-SEND-METRIC-DEFINITIONS") <<std::endl;

    out << std::endl;
}

void writeDISSection(std::ostream& out, Config& config)
{
    out << "# DIS" << std::endl;
    out << config.getParameter("VRLINK") << std::endl;
    out << config.getParameter("VRLINK-PROTOCOL") << std::endl;

    out << config.getParameter("VRLINK-DIS-IP-ADDRESS") << std::endl;
    out << config.getParameter("VRLINK-DIS-NETWORK-INTERFACE") << std::endl;
    out << config.getParameter("VRLINK-DIS-PORT") << std::endl;
    if (config.parameterExists("VRLINK-DIS-SUBNET-MASK"))
    {
        out << config.getParameter("VRLINK-DIS-SUBNET-MASK") << std::endl;
    }

    out << config.getParameter("VRLINK-ENTITIES-FILE-PATH") << std::endl;
    out << config.getParameter("VRLINK-RADIOS-FILE-PATH") << std::endl;
    out << config.getParameter("VRLINK-NETWORKS-FILE-PATH") << std::endl;

    out << config.getParameter("VRLINK-DEBUG-PRINT-COMMS") << std::endl;
    out << config.getParameter("VRLINK-DEBUG-PRINT-COMMS-2") << std::endl;
    out << config.getParameter("VRLINK-DEBUG-PRINT-MAPPING") << std::endl;
    out << config.getParameter("VRLINK-DEBUG-PRINT-DAMAGE") << std::endl;
    out << config.getParameter("VRLINK-DEBUG-PRINT-TX-STATE") << std::endl;
    out << config.getParameter("VRLINK-DEBUG-PRINT-TX-POWER") << std::endl;
    out << config.getParameter("VRLINK-DEBUG-PRINT-MOBILITY") << std::endl;
    out << config.getParameter(
        "VRLINK-DEBUG-PRINT-TRANSMITTER-PDU") << std::endl;
    out << config.getParameter("VRLINK-DEBUG-PRINT-PDUS") << std::endl;
    out << config.getParameter("VRLINK-RECEIVE-DELAY") << std::endl;
    out << config.getParameter("VRLINK-MAX-RECEIVE-DURATION") << std::endl;
    out << config.getParameter("VRLINK-XYZ-EPSILON") << std::endl;
    out << config.getParameter("VRLINK-MOBILITY-INTERVAL") << std::endl;

    out << std::endl;
}
void writeComponentLine(std::ostream& out, Hierarchy* hier)
{
    out << "COMPONENT " << hier->id;
    out << " {";
    std::set<Node*, Node::less>::iterator nit = hier->nodes.begin();
    while (nit != hier->nodes.end())
    {
        if ((*nit)->NodeId != -1)
        {
            out << (*nit)->NodeId;
            nit++;
            if (nit != hier->nodes.end())
                out << " ";
        }
        else
        {
            nit++;
        }
    }
    if (hier->children.size() > 0 && hier->nodes.size() > 0)
    {
        out << " ";
    }
    std::set<Hierarchy*>::iterator hit = hier->children.begin();
    while (hit != hier->children.end())
    {
        out << "H" << (*hit)->id;
        hit++;
        if (hit != hier->children.end())
        {
            out << " ";
        }
    }
    if (hier->nodes.size() > 0 &&  hier->networks.size() > 0)
    {
        out << " ";
    }
    std::set<Network*>::iterator netIt = hier->networks.begin();
    while (netIt != hier->networks.end())
    {
        out << (*netIt)->address;
        netIt++;
        if (netIt != hier->networks.end())
        {
            out << " ";
        }
    }
    out << "} ";
    out << hier->nodes.size() + hier->children.size();

    hier->computeTerrainBounds();
    out << " " << hier->position.x
        << " " << hier->position.y
        << " " << hier->position.z;
    out << " " << hier->dimensions.x
        << " " << hier->dimensions.y
        << " " << hier->dimensions.z;

    if (!hier->name.empty())
    {
        out << " HOSTNAME " << hier->name.c_str();
    }

    out << std::endl;
}
void writeComponentSection(std::ostream& out, NodeSet& ns)
{
    out << "# COMPONENTS" << std::endl;

    AggregateEntity2HierarchyMap hierarchies = ns.hierarchiesUsed();
    if (hierarchies.size() == 0)
    {
        return;
    }

    Hierarchy* hier = Hierarchy::rootHierarchy;
    if (hier && !(hier->children.size() == 0 && hier->nodes.size() == 0))
    {
        writeComponentLine(out, hier);
    }

    AggregateEntity2HierarchyMap::iterator it = hierarchies.begin();
    while (it != hierarchies.end())
    {
        hier = it->second;
        if (hier->children.size() == 0 && hier->nodes.size() == 0)
        {
            continue;
        }
        writeComponentLine(out, hier);
        it++;
    }
    out << std::endl;
}

void ConfigFileWriter::writeConfigFile(std::string filename, NodeSet& ns)
{
    std::fstream out;
    out.open(filename.c_str(), std::fstream::out | std::fstream::trunc);

    writeGeneralSection(out, Config::instance());
    writeTerrainSection(out, Config::instance());

    writePositionSection(out, Config::instance());
    writeMobilitySection(out, Config::instance());
    writeProgagationSection(out, Config::instance(), ns.channelsUsed());

    writeNetworksSection(out, ns);
    writePhysicalSection(out, Config::instance(), ns);

    writeStatisticsSection(out, Config::instance());
    writeHostnameSection(out, ns);
    writeNodeIconSection(out, ns, Config::instance());
    writeSlotsFile(out, Config::instance(), ns);

    if (Config::instance().externalInterfaceType == "HLA13"
        || Config::instance().externalInterfaceType == "HLA1516")
    {
        writeHLASection(out, Config::instance());
    }
    else if (Config::instance().externalInterfaceType == "DIS")
    {
        writeDISSection(out, Config::instance());
    }
    writeComponentSection(out, ns);

    out.close();
}

void ConfigFileWriter::copyFederationFile()
{
    std::string fedFileName;
    std::string orgPath = Config::instance().fedPath;
    const std::string& dirSep = Config::instance().dirSep;
    size_t dir_end = orgPath.rfind(dirSep);
    if (dir_end == orgPath.npos)
    {
        fedFileName = Config::instance().scenarioDir
                      + Config::instance().dirSep + orgPath;
    }
    else
    {
        fedFileName = Config::instance().scenarioDir
                      + Config::instance().dirSep
                      + orgPath.substr(dir_end+1);
    }
    copyFile(Config::instance().fedPath.c_str(), fedFileName.c_str());
}

void ConfigFileWriter::copyIconFiles()
{
    std::string iconDir = Config::instance().scenarioDir
                          + Config::instance().dirSep
                          + "icons" + Config::instance().dirSep;
    _mkdir(iconDir.c_str());
    std::set<std::string>::iterator it = iconNames.begin();
    while (it != iconNames.end())
    {
        std::string filename = Config::instance().getFileName(*it);
        std::string path = Config::instance().findIconFile(filename);
        if (!filename.empty() && !path.empty())
        {
            copyFile(path, iconDir+filename);
        }
        it++;
    }
}

void ConfigFileWriter::copyFile(const std::string& src,
                                const std::string& dst)
{
    std::cout << "Copying file \"" << src << "\" to \"" << dst
              << "\"" << std::endl << std::flush;
    FILE* in = fopen(src.c_str(), "rb");
    if (in == NULL)
        return;

    FILE* out = fopen(dst.c_str(), "wb");
    if (out == NULL)
    {
        fclose(in);
        return;
    }

    fseek(in, 0, SEEK_END);
    size_t sze = ftell(in);
    rewind(in);

    size_t total = 0;
    while (!feof(in))
    {
        char buf[4096];
        size_t r = fread(buf, 1, sizeof(buf), in);
        if (r < sizeof(buf))
        {
            char *estr = strerror(errno);
            int err = ferror(in);
            if (err)
                std::cout << "Error reading file \"" << src << "\" : "
                          << estr << std::endl << std::flush;
        }
        total += r;
        fwrite(buf, 1, r, out);
    }
    if (total < sze)
    {
        std::cout << "The file was not completely copied but no error was"
            " indicated. Please check the file." << std::endl << std::flush;
    }
    fclose(in);
    fclose(out);
}
