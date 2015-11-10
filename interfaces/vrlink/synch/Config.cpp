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

#include "Config.h"
#include "archspec.h"
#include "lic.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <sys/stat.h>

// max macro is not standard C++. Undefine it so it doesn't clobber
// the std::max template function.
#ifdef max
#undef max
#endif

#include <algorithm>


using namespace SNT_SYNCH;

Parameter Parameter::unknownParameter("", "Unknown Parameter");

Config::Config() :
        dirSep(DIR_SEP),
        scenarioName("synchscenario"),
        scenarioDir("synchscenario"),
        externalInterfaceType("HLA13"),
        federationName("VR-Link"),
        fedPath("VR-Link.fed"),
        federateName("extractor"),
        rprVersion("1.0"),
        defaultRouterModel("802.11b"),
        disDestIpAddress("127.255.255.255"),
        disDeviceAddress("127.0.0.1"),
        disSubnetMask(""),
        disPort("3000"),
        firstNodeId(1),
        networkPrefix(60),
        timeout(5),
        merge(false),
        allEntitiesHaveRadios(false),
        debug(false),
        debugSetOnCmdLine(false),
        defaultRadioSystemType("1,1,225,1,1,1")
{
    char currentWorkingDirectory[FILENAME_MAX];
    std::string cwdStr;
    memset(currentWorkingDirectory, 0, FILENAME_MAX);
    if (GetCurrentDir(currentWorkingDirectory, FILENAME_MAX))
    {
        qualnetHome = currentWorkingDirectory;
        cwdStr = currentWorkingDirectory;
                
#ifdef _WIN32
        std::string currFolderName = qualnetHome.substr(qualnetHome.find_last_of("\\") + 1);
        if (currFolderName.compare("bin") != 0)
        {
            qualnetHome = qualnetHome.substr(0, qualnetHome.find_last_of("\\"));  //  This will back up one directory to HOME/gui/lib
            qualnetHome = qualnetHome.substr(0, qualnetHome.find_last_of("\\"));  //  This will back up one directory to HOME/gui
            qualnetHome = qualnetHome.substr(0, qualnetHome.find_last_of("\\"));  //  This will back up one directory
            cwdStr = qualnetHome + "\\bin";
        }
        else
        {
            qualnetHome = qualnetHome.substr(0, qualnetHome.find_last_of("\\"));  //  This will back up one directory
        }

        if (FileExists(cwdStr + "\\qualnet.exe"))
        {
            productName = "QualNet";
        }
        else if (FileExists(cwdStr + "\\exata.exe"))
        {
            productName = "EXata";
        }
        else if (FileExists(cwdStr + "\\jne.exe"))
        {
            productName = "JNE";
        }
#else
        std::string currFolderName = qualnetHome.substr(qualnetHome.find_last_of("/") + 1);
        if (currFolderName.compare("bin") != 0)
        {
            qualnetHome = qualnetHome.substr(0, qualnetHome.find_last_of("/"));  //  This will back up one directory to HOME/gui/lib
            qualnetHome = qualnetHome.substr(0, qualnetHome.find_last_of("/"));  //  This will back up one directory to HOME/gui
            qualnetHome = qualnetHome.substr(0, qualnetHome.find_last_of("/"));  //  This will back up one directory
            cwdStr = qualnetHome + "/bin";
        }
        else
        {
            qualnetHome = qualnetHome.substr(0, qualnetHome.find_last_of("/"));  //  This will back up one directory
        }
        
        if (FileExists(cwdStr + "/qualnet"))
        {
            productName = "QualNet";
        }
        else if (FileExists(cwdStr + "/exata"))
        {
            productName = "EXata";
        }
        else if (FileExists(cwdStr + "/jne"))
        {
            productName = "JNE";
        }
#endif
    }
    else
    {
        qualnetHome = ".";
        productName = "QualNet";
    }

    licenseCheckout();
    setParameter(Parameter("VRLINK-DEFAULT-ROUTER-MODEL",
                 defaultRouterModel));
}

bool Config::FileExists(std::string filename) 
{
#ifdef _WIN32
    struct _stat info;
#else
    struct stat info;
#endif
    int ret = -1;

    //get the file attributes
#ifdef _WIN32
    ret = _stat(filename.c_str(), &info);
#else
    ret = stat(filename.c_str(), &info);
#endif

    if (ret == 0) 
    {
        //stat() is able to get the file attributes,
        //so the file obviously exists
        return true;
    }
    else 
    {
        //stat() is not able to get the file attributes,
        //so the file obviously does not exist
        return false;
    }
}

Config::~Config()
{
    licenseCheckin();
}
int Config::processFlag(const char* flag, const char* values[])
{
    if (*(flag+1))
    {
        return 0;
    }
    switch (*flag)
    {
        case 'h':
        case '?':
            return 0;
            break;
        // below functionalities are currently abandoned/incomplete feature
        // that could later be added again as an enhancement
        //case 'd':
        //    debug = true;
        //    return 1;
        //    break;
        //case 'm':
        //    merge = true;
        //    return 1;
        //    break;
        //case 'o':
        //    allEntitiesHaveRadios = true;
        //    return 1;
        //    break;
        case 'P':
            if (values[0])
            {
                externalInterfaceType = values[0];
                if (externalInterfaceType == "DIS"
                    || externalInterfaceType == "HLA13"
                    || externalInterfaceType == "HLA1516")
                {
                    return 2;
                }
                else
                {
                    return 0;
                }
            }
            else
            {
                return 0;
            }
            break;
        case 'r':
            if (values[0])
            {
                rprVersion = values[0];
                if (rprVersion == "1.0"
                    || rprVersion == "2.0017")
                {
                    return 2;
                }
                else
                {
                    return 0;
                }
            }
            else
            {
                return 0;
            }
            break;
        case 'f':
            if (values[0])
            {
                federationName = values[0];
                return 2;
            }
            else
            {
                return 0;
            }
            break;
        case 'F':
            if (values[0])
            {
                fedPath = values[0];
                return 2;
            }
            else
            {
                return 0;
            }
            break;
        case 'A':
            if (values[0])
            {
                disDestIpAddress = values[0];
                return 2;
            }
            else
            {
                return 0;
            }
            break;
        case 'I':
            if (values[0])
            {
                disDeviceAddress = values[0];
                return 2;
            }
            else
            {
                return 0;
            }
            break;
        case 'M':
            if (values[0])
            {
                disSubnetMask = values[0];
                return 2;
            }
            else
            {
                return 0;
            }
            break;
        case 'O':
            if (values[0])
            {
                disPort = values[0];
                return 2;
            }
            else
            {
                return 0;
            }
            break;
        case 'N':
            if (values[0])
            {
                firstNodeId = atoi(values[0]);
                return 2;
            }
            else
            {
                return 0;
            }
            break;
        case 's':
            if (values[0])
            {
                networkPrefix = atoi(values[0]);
                if (networkPrefix > 0 && networkPrefix < 224)
                    return 2;
                else
                    return 0;
            }
            else
            {
                return 0;
            }
            break;
        case 't':
            if (values[0])
            {
                timeout = atoi(values[0]);
                if (timeout > 0)
                    return 2;
                else
                    return 0;
            }
            else
            {
                return 0;
            }
            break;
        default:
            return 0;
            break;
    }
}

#ifdef FLEXLM
void Config::licenseCheckout()
{
    std::string qualnetLicensePath = qualnetHome + dirSep + "license_dir";
    lm_job = ::licenseCheckout(qualnetLicensePath.c_str());
}
void Config::licenseCheckin()
{
    ::licenseCheckin(lm_job);
}
#else
void Config::licenseCheckout() {}
void Config::licenseCheckin() {}
#endif


bool Config::parseCommandLine(int argc, const char* argv[])
{
    std::string cmd(argv[0]);
    std::string cmdDir;
    size_t pos = cmd.rfind(dirSep);
    if (pos == cmd.npos)
        cmdDir = fullpath(".");
    else
        cmdDir = fullpath(cmd.substr(0, pos));
    pos = cmdDir.rfind(dirSep + "bin");
    if (pos != cmdDir.npos)
        exeHome = cmdDir.substr(0, pos);

    bool isOptionPFound = false;
    bool isOptionFFound = false;
    std::string optionPValue;

    int i = 1;
    while (i < argc)
    {
        const char* arg = argv[i];

        if (arg[0] == '-' || arg[0] == '/')
        {
            if (*(arg + 1) == 'P')
            {
                isOptionPFound = true;
                if (!*(argv+i+1))
                {
                    // value of -P dose not exist
                    return false;
                }
                optionPValue = *(argv+i+1);
            }
            if (*(arg + 1) == 'F')
            {
                isOptionFFound = true;
            }
            int used = processFlag(arg+1, argv+i+1);
            if (used)
                i += used;
            else
                return false;
        }
        else if (i + 1 == argc)
        {
            // check for mandatory options first
            if (!isOptionPFound)
            {
                // -P is mandatory option.
                return false;
            }
            else
            {
                if ((optionPValue == "HLA13"
                    || optionPValue == "HLA1516")
                    && isOptionFFound == false)
                {
                    // -F is mandatory option for HLA13 and HLA1516
                    return false;
                }
            }

            scenarioDir = argv[i];
            size_t dir_end = scenarioDir.rfind(dirSep);
            if (dir_end == scenarioDir.npos)
                scenarioName = scenarioDir;
            else
                scenarioName = scenarioDir.substr(dir_end + 1);
            std::string path = fullpath(scenarioDir);
            if (!path.empty())
            {
                scenarioDir = path;
            }
            scenarioPath = scenarioDir + dirSep + scenarioName;
            i++;
        }
        else
        {
            return false;
        }
    }

    if (argc == 1)
    {
        return false;
    }
    else
    {
        if (isOptionPFound
            && (optionPValue == "HLA13"
                || optionPValue == "HLA1516")
            && isOptionFFound == false)
        {
            // -F is mandatory option for HLA13 and HLA1516
            return false;
        }
    }
    if (allEntitiesHaveRadios && merge)
        return false;

    return true;
}

void Config::Usage(const char* program_name)
{
    std::cout << productName << " Synchronizer" << std::endl
         << std::endl
         << "Syntax:" << std::endl
         << std::endl
         << program_name
         << "  [options] base-filename" << std::endl
         << std::endl
         << "    (Default values in parentheses)" << std::endl
         << "    -P DIS/HLA13/HLA1516" << std::endl
         << "\tSet protocol. Its a mandatory parameter." << std::endl
         << "    -A ip-address" << std::endl
         << "\tFor DIS. Set DIS destination IP address (127.255.255.255)"
         << std::endl
         << "    -I device-address" << std::endl
         << "\tFor DIS. Set DIS device address (127.0.0.1)" << std::endl
         << "    -O port-number" << std::endl
         << "\tFor DIS. Set DIS port number (3000)" << std::endl
         << "    -M subnet-mask" << std::endl
         << "\tFor DIS. Set subnet mask for destination IP address" << std::endl                  
         << "    -f federation-name" << std::endl
         << "\tFor HLA. Set federation name (VR-Link)" << std::endl
         << "    -F FED/FDD-file" << std::endl
         << "\tFor HLA. Set FED or FDD file. Its a mandatory parameter for HLA."
         << std::endl
         // merge functionality is currently an abandoned/incomplete feature
         // that could later be added again as an enhancement
         //<< "    -m" << std::endl
         //<< "\tMerge into existing .config file" << std::endl
         << "    -N nodeId" << std::endl
         << "\tSet initial nodeId (1)" << std::endl
         // this functionality is currently an abandoned/incomplete feature
         // that could later be added again as an enhancement
         //<< "    -o" << std::endl
         //<< "\tIgnore RadioTransmitter objects, create one node/radio"
         //   " per entity" << std::endl
         << "    -r number" << std::endl
         << "\tUse RPR FOM 1.0 or 2.0017 (1.0)" << std::endl
         << "    -s number" << std::endl
         << "\tSet class A network number for new networks (60)" << std::endl
         << "    -t timeout" << std::endl
         << "\tSet number of seconds since last entity discovered before"
            " program exit (5)" << std::endl
         << "    -h" << std::endl
         << "\tHelp" << std::endl;
         // this functionality is currently an abandoned/incomplete feature
         // that could later be added again as an enhancement
         //<< "    -d" << std::endl
         //<< "\tDebug mode" << std::endl;
}

void Config::initializeScenarioParameters()
{
    setParameter(Parameter("EXPERIMENT-NAME", scenarioName));
    setParameter(Parameter("VRLINK", "YES"));

    if (externalInterfaceType == "DIS")
    {
        setParameter(Parameter("VRLINK-PROTOCOL", "DIS"));
    }
    else if (externalInterfaceType == "HLA13")
    {
        setParameter(Parameter("VRLINK-PROTOCOL", "HLA13"));
    }
    else if (externalInterfaceType == "HLA1516")
    {
        setParameter(Parameter("VRLINK-PROTOCOL", "HLA1516"));
    }

    setParameter(Parameter("VRLINK-FEDERATION-NAME", federationName));

    std::string fedFileName;
    std::string orgPath = fedPath;
    size_t dir_end = orgPath.rfind(dirSep);
    fedFileName = orgPath.substr(dir_end+1);

    setParameter(Parameter("VRLINK-FED-FILE-PATH", fedFileName));
    setParameter(Parameter("VRLINK-FEDERATE-NAME", federateName));
    setParameter(Parameter("VRLINK-RPR-FOM-VERSION", rprVersion));

    setParameter(Parameter("VRLINK-DIS-IP-ADDRESS", disDestIpAddress));
    setParameter(Parameter("VRLINK-DIS-NETWORK-INTERFACE",
                 disDeviceAddress));
    setParameter(Parameter("VRLINK-DIS-PORT", disPort));

    if (disSubnetMask != "")
    {
        setParameter(Parameter("VRLINK-DIS-SUBNET-MASK", disSubnetMask));
    }

    setParameter(Parameter("CONFIG-FILE-PATH", scenarioName + ".config"));
    setParameter(Parameter("NODE-POSITION-FILE", scenarioName + ".nodes"));
    setParameter(Parameter("APP-CONFIG-FILE", scenarioName + ".app"));
    setParameter(Parameter("ROUTER-MODEL-CONFIG-FILE",
                 scenarioName + ".router-models"));
    setParameter(Parameter("LINK-16-SLOT-FILE", scenarioName + ".slot"));

    setParameter(Parameter("VRLINK-ENTITIES-FILE-PATH",
                 scenarioName + ".vrlink-entities"));
    setParameter(Parameter("VRLINK-RADIOS-FILE-PATH",
                 scenarioName + ".vrlink-radios"));
    setParameter(Parameter("VRLINK-NETWORKS-FILE-PATH",
                 scenarioName + ".vrlink-networks"));
    setParameter(Parameter("LINK-16-SLOT-FILE", scenarioName + ".slot"));

    setParameter(Parameter("NODE-PLACEMENT", "FILE"));
}

void Config::readParameterFiles()
{

    const char *p = getenv("HOMEPATH");
    if (!p)
    {
        p = ".";
    }
    std::string userHome(p);

    std::vector<std::string> files;
    findFilesByExtension("vrlink", qualnetHome + dirSep + "gui" + dirSep + "devices", files);
    findFilesByExtension("vrlink", qualnetHome + dirSep + "addons", files);
    findFilesByExtension("vrlink", userHome + dirSep + ".qualnetUserDir", files);
    if (files.size())
    {
        std::cout << "Reading parameter files"  << std::endl << std::flush;
    }
    for (size_t i = 0; i < files.size(); i++)
    {
        readParameterFile(files[i]);
    }

    initializeScenarioParameters();
}

void Config::readParameterFile(std::string filename)
{
    std::cout << "Reading parameter file " << filename
              << std::endl << std::flush;
    std::fstream in;
    in.open(filename.c_str(), std::fstream::in);
    readParameterFile(in);
    in.close();
}

void Config::readParameterFile(std::istream& in)
{
    static std::string whiteSpace(" \t");
    while (in.good())
    {
        std::string line;
        getline(in, line);
        if (line.empty())
        {
            continue;
        }

        size_t leadingWS = line.find_first_not_of(whiteSpace);
        if (leadingWS)
        {
            line = line.substr(leadingWS);
        }
        if (line.empty())
        {
            continue;
        }

        size_t startOfComment = line.find_first_of("#");
        if (startOfComment < line.npos)
        {
            line = line.substr(0, startOfComment);
        }
        if (line.empty())
        {
            continue;
        }

        size_t tailingWS = line.find_last_not_of(whiteSpace);
        if (tailingWS == line.npos)
        {
            continue;
        }
        line = line.substr(0, tailingWS+1);
        if (line.empty())
        {
            continue;
        }

        size_t endOfName = line.find_first_of(whiteSpace);
        if (endOfName == line.npos)
        {
            continue;
        }

        size_t startOfValue = line.find_first_not_of(whiteSpace, endOfName);
        if (startOfValue == line.npos)
        {
            continue;
        }

        std::string name = line.substr(0, endOfName);
        std::string value = line.substr(startOfValue, line.npos);
        if (name.find("NODE-ICON") == 0)
        {
            value = findIconFile(value);
        }
        setParameter(Parameter(name, value));
    }
}

void Config::readModelFiles()
{

    const char* p = getenv("HOMEPATH");
    if (!p)
    {
        p = ".";
    }
    std::string userHome(p);

    std::vector<std::string> files;
    findFilesByExtension("device-models",
                         qualnetHome + dirSep + "gui" + dirSep + "devices",
                         files);
    findFilesByExtension("router-models",
                         qualnetHome + dirSep + "gui" + dirSep + "devices",
                         files);
    findFilesByExtension("device-models", qualnetHome + dirSep + "addons", files);
    findFilesByExtension("router-models", qualnetHome + dirSep + "addons", files);
    findFilesByExtension("device-models",
                         userHome + dirSep + ".qualnetUserDir",
                         files);
    findFilesByExtension("router-models",
                         userHome + dirSep + ".qualnetUserDir",
                         files);
    if (files.size())
        std::cout << "Reading Model files" << std::endl << std::flush;
    size_t i = 0;
    for (i = 0; i < files.size(); i++)
    {
        readModelFile(files[i]);
    }
}

void Config::readModelFile(std::string filename)
{
    std::cout << "Reading Model file " << filename << std::endl<< std::flush;
    std::fstream in;
    in.open(filename.c_str(), std::fstream::in);
    readModelFile(in);
    in.close();
}

void Config::readModelFile(std::istream& in)
{
    static std::string whiteSpace(" \t");
    ParameterMap* currentModel = 0;
    while (in.good())
    {
        std::string line;
        getline(in, line);
        if (line.empty())
        {
            continue;
        }

        size_t leadingWS = line.find_first_not_of(whiteSpace);
        if (leadingWS)
        {
            line = line.substr(leadingWS);
        }
        if (line.empty())
        {
            continue;
        }

        size_t startOfComment = line.find_first_of("#");
        if (startOfComment < line.npos)
        {
            line = line.substr(0, startOfComment);
        }
        if (line.empty())
        {
            continue;
        }

        size_t tailingWS = line.find_last_not_of(whiteSpace);
        if (tailingWS == line.npos)
        {
            continue;
        }
        line = line.substr(0, tailingWS+1);
        if (line.empty())
        {
            continue;
        }

        size_t endOfName = line.find_first_of(whiteSpace);
        if (endOfName == line.npos)
        {
            continue;
        }

        size_t startOfValue = line.find_first_not_of(whiteSpace, endOfName);
        if (startOfValue == line.npos)
        {
            continue;
        }

        Parameter param(line.substr(0, endOfName),
                        line.substr(startOfValue, line.npos));
        if (param.name == "ROUTER-MODEL")
        {
            std::map<std::string, ParameterMap*>::iterator it =
                                                    models.find(param.value);
            if (it != models.end())
            {
                std::cout << "Updating Model " << param.value << std::endl;
                currentModel = it->second;
            }
            else
            {
                std::cout << "Loading Model " << param.value << std::endl;
                currentModel = new ParameterMap;
                std::pair<std::string, ParameterMap*> p(param.value,
                                                        currentModel);
                models.insert(p);
            }
        }
        else if (param.name == "VRLINK-RADIO-SYSTEM-TYPE")
        {
            RadioTypeStruct radioType(param.value);
            radioTypeToRouterModel[radioType] = currentModel;
        }
        if (currentModel)
        {
            currentModel->setParameter(param);
        }
    }
}

ParameterMap* Config::getModelParameterList(std::string name)
{
    std::map<std::string, ParameterMap*>::iterator it = models.find(name);
    if (it != models.end())
    {
        return it->second;
    }
    else
    {
        return 0;
    }
}

ParameterMap* Config::getModelParameterList(RadioTypeStruct radioType)
{
    RadioTypeMapWithWildCards<ParameterMap*>::iterator it =
                                   radioTypeToRouterModel.matchWC(radioType);
    if (it != radioTypeToRouterModel.end())
    {
        return it->second;
    }
    else
    {
        return getModelParameterList(
                          getParameter("VRLINK-DEFAULT-ROUTER-MODEL").value);
    }
}

std::vector<Parameter> Config::getParameterList(std::string name,
                                                matchType match)
{
    std::vector<Parameter> parameters;
    std::map<std::string, Parameter>::iterator it =
                                                  scenarioParameters.begin();
    while (it != scenarioParameters.end())
    {
        bool matchFound = false;
        const std::string& key = it->first;
        switch( match )
        {
        case ANY:
            {
                size_t pos = key.find(name);
                if (pos != key.npos)
                    matchFound = true;
                break;
            }
        case PREFIX:
            {
                size_t pos = key.find(name);
                if (pos == 0)
                    matchFound = true;
                break;
            }
        case SUFFIX:
            {
                size_t pos = key.rfind(name);
                if (pos + name.size() == key.size())
                    matchFound = true;
                break;
            }
        default:
                break;
        }
        if (matchFound)
        {
            parameters.push_back(it->second);
        }
        it++;
    }
    return parameters;
}

const std::string& Config::getIconForType(const EntityTypeStruct& type)
{
    std::stringstream id;
    id << "NODE-ICON[";
    id << (unsigned int) type.entityKind << ".";
    id << (unsigned int) type.domain << ".";
    id << (unsigned int) type.countryCode << ".";
    id << (unsigned int) type.category << ".";
    id << (unsigned int) type.subcategory << ".";
    id << (unsigned int) type.specific << ".";
    id << (unsigned int) type.extra;
    id << "]";
    const Parameter& iconPath = getParameter(id.str());
    if (iconPath != Parameter::unknownParameter)
    {
        return iconPath.value;
    }
    else
    {
        return getParameter("NODE-ICON").value;
    }
}


bool fileIsReadable(const std::string& fileName)
{
    FILE* fp = fopen(fileName.c_str(), "r");
    if (!fp)
    {
        return false;
    }
    fclose(fp);
    return true;
}

std::string Config::getFileName(const std::string& path)
{
    size_t dir_end;
    size_t dir_end1 = path.rfind("\\");
    size_t dir_end2 = path.rfind("/");
    if (dir_end1 == path.npos && dir_end2 == path.npos)
    {
        return path;
    }
    else if (dir_end1 == path.npos)
    {
        dir_end = dir_end2;
    }
    else if (dir_end2 == path.npos)
    {
        dir_end = dir_end1;
    }
    else
    {
        dir_end = std::max(dir_end1, dir_end2);
    }
    return path.substr(dir_end+1);
}

const std::string Config::findIconFile(const std::string& iconName)
{
    // check if the files exists as is
    if (fileIsReadable(iconName))
    {
        return iconName;
    }

    std::string filename = getFileName(iconName);
    std::string path;

    // look in $QUALNET_HOME/gui/icons  and subdirs
    path = qualnetHome + dirSep + "gui" + dirSep + "icons" + dirSep
                       + filename;
    if (fileIsReadable(path))
    {
        return path;
    }

    path = qualnetHome + dirSep + "gui" + dirSep + "icons" + dirSep
                       + "devices" + dirSep + filename;
    if (fileIsReadable(path))
    {
        return path;
    }

    path = qualnetHome + dirSep + "gui" + dirSep + "icons" + dirSep
                       + "military" + dirSep + filename;
    if (fileIsReadable(path))
    {
        return path;
    }

    path = qualnetHome + dirSep + "gui" + dirSep + "icons" + dirSep
                       + "people" + dirSep + filename;
    if (fileIsReadable(path))
    {
        return path;
    }

    path = qualnetHome + dirSep + "gui" + dirSep + "icons" + dirSep
                       + "app_icons" + dirSep + filename;
    if (fileIsReadable(path))
    {
        return path;
    }

    path = qualnetHome + dirSep + "gui" + dirSep + "icons" + dirSep
                       + "3Dvisualizer" + dirSep + filename;
    if (fileIsReadable(path))
    {
        return path;
    }

    // look in $QUALNET_HOME/interfaces/vrlink/synch/data/icons
    path = qualnetHome + dirSep + "interfaces" + dirSep + "vrlink" + dirSep
                       + "synch" + dirSep + "data" + dirSep + "icons"
                       + dirSep + filename;
    if (fileIsReadable(path))
    {
        return path;
    }

    // look in ../icons
    path = ".." + dirSep + "icons" + dirSep + filename;
    if (fileIsReadable(path))
    {
        return path;
    }

    return std::string("");
}

std::ostream& SNT_SYNCH::operator<<(std::ostream& out, const Parameter& para)
{
    out << para.name;
    out << "\t\t";
    out << para.value;

    return out;
}


#ifdef _WIN32
#include <io.h>
#include <set>

void findFilesByExtension(const std::string extension,
                          const std::string rootDir,
                          std::vector<std::string> &filenameList)
{
    std::string filespec = rootDir + "\\*";
    struct _finddata_t fileinfo;
    intptr_t findH = _findfirst(filespec.c_str(), &fileinfo);
    if (findH == -1)
    {
        return;
    }

    std::set<std::string> files;
    std::set<std::string> subDirs;
    int ret = 0;
    do {
        std::string filename(fileinfo.name);
        size_t pos = filename.rfind(std::string(".") + extension);
        if (fileinfo.attrib & _A_SUBDIR
            && filename != "."
            && filename != "..")
        {
            subDirs.insert(filename);
        }
        else if (pos != filename.npos
                 && pos == filename.size() - (extension.size() + 1))
        {
            files.insert(filename);
        }
        ret = _findnext(findH, &fileinfo);
    }     while (ret != -1);

    _findclose(findH);

    std::set<std::string>::iterator it;
    it = files.begin();
    while (it != files.end())
    {
        filenameList.push_back(rootDir + "\\" + *it);
        it++;
    }
    it = subDirs.begin();
    while (it != subDirs.end())
    {
        findFilesByExtension(extension, rootDir + "\\" + *it, filenameList);
        it++;
    }
}

#else

#include <dirent.h>
#include <set>

void findFilesByExtension(const std::string extension,
                          const std::string rootDir,
                          std::vector<std::string> &filenameList)
{
    const std::string& dirSep = Config::instance().dirSep;

    std::string filespec = rootDir + dirSep;
    DIR* dirFile = opendir(filespec.c_str());

    if (!dirFile)
    {
        return;
    }
    std::set<std::string> files;
    std::set<std::string> subDirs;

    struct dirent* entry = NULL;

    // if !entry then end of directory
    while ((entry = readdir(dirFile)) != NULL)
    {
        // in linux ignore hidden files all start with '.'
        if (entry->d_name[0] == '.')
        {
            continue;
        }

        if (entry->d_type == DT_DIR)
        {
            // if entry is a directory
            std::string dname = entry->d_name;
            if (dname != "." && dname != "..")
            {
                subDirs.insert(dname);
            }
        }
        else if (entry->d_type == DT_REG)
        {
            // if entry is a regular file
            std::string fname = entry->d_name;
            size_t pos = fname.rfind(std::string(".") + extension);

            if (pos != fname.npos
                && pos == fname.size() - (extension.size() + 1))
            {
                // add filename
                files.insert(fname);
            }
        }
    }

    std::set<std::string>::iterator it;
    it = files.begin();
    while (it != files.end())
    {
        filenameList.push_back(rootDir + dirSep + *it);
        it++;
    }
    it = subDirs.begin();
    while (it != subDirs.end())
    {
        findFilesByExtension(extension, rootDir + dirSep + *it, filenameList);
        it++;
    }
}

#endif

