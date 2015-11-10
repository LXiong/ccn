#ifndef __ExecSettings_h_
#define __ExecSettings_h_

#define DEFAULT_OCULA_PORT 49909

#include <string>


class ExecSettings 
{
public:
    ExecSettings();
    virtual ~ExecSettings();
    
    int numberOfProcessors;
    bool isRemote;
    bool isDistributed;
    bool isEmulationMode;
    std::string distributedHostFile;
    std::string remoteLoginAndHost;
    std::string remoteProductHome;
    std::string scenarioFilePath;
    
    bool forceTerrainRebuild;
    bool skipOgreSettings;

    int interactivePort;
    std::string interactiveHostAddress;

    std::string interactivePortString() const;

private:

};


#endif // #ifndef __ExecSettings_h_
