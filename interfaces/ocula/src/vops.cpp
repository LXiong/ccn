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

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#else
#include <unistd.h>
#include <signal.h>
#endif

#include <time.h>
#include <sys/stat.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "vops.h"
#include "ops_util.h"

#ifndef OCULA_TESTER
#include "BaseApplication.h"
#include "TerrainApplication.h"
#endif

static std::string gAppPath;
static std::string gProductHome;

static std::string GetAppPath(const std::string& rootPath, const std::string appName)
{
    std::vector<std::string> paths;
#if defined(_WIN32) || defined(_WIN64)
    std::string separator = "\\";
#else
    std::string separator = "/";
#endif

    OPS_SplitString(rootPath, paths, separator);
    for (int i = paths.size(); i > 0; i--)
    {
#if defined(_WIN32) || defined(_WIN64)
        std::string path;
#else
        std::string path = separator;
#endif
        for (int j = 0; j < i; j++)
        {
            path += paths[j] + separator;
        }
        path += "bin" + separator + appName;

#if defined(_WIN32) || defined(_WIN64)
        if (GetFileAttributes(path.c_str()) != 0xFFFFFFFF)
#else
        struct stat st;
        if (stat(path.c_str(), &st) == 0)
#endif
        {
            return path;
        }
    }

    return "";
}

Vops::Vops()
{
    m_paused = true;
    m_state = VopsPropertyManager::getInstance();
    m_stdoutCallback = NULL;

#ifdef _WIN32
    m_qualnetStdoutRead = NULL;
    m_qualnetStdoutWrite = NULL;
#else
    m_pid = -1;
#endif
}

Vops::~Vops()
{
    VopsPropertyManager::destroyInstance();

#ifdef _WIN32
    // Close simulator process
    TerminateProcess(m_simProcInfo.hProcess, 0);
#else
    kill(m_pid, SIGKILL);
#endif
}

void Vops::launchSimulator(const ExecSettings& execSettings)
{
    m_state->setOculaPort(execSettings.interactivePort);

    printf("Vops is launching the simulator\n");

    // Go back directories until we see a license_dir
    // Launch qualnet using default.config
    std::string curDir = getcwd(NULL, 0);

    // Look for the simulator executable
    std::vector<std::string> executables;
#ifdef _WIN32
    if (execSettings.isRemote) 
    {
        executables.push_back("exata.remote.bat");
        executables.push_back("qualnet.remote.bat");
        executables.push_back("jne.remote.bat");
    }
    else
    {
        executables.push_back("exata.exe");
        executables.push_back("qualnet.exe");
        executables.push_back("jne.exe");
    }
#else
    if (execSettings.isRemote) 
    {
        executables.push_back("exata.remote.sh");
        executables.push_back("qualnet.remote.sh");
        executables.push_back("jne.remote.sh");
    }
    else
    {
        executables.push_back("exata");
        executables.push_back("qualnet");
        executables.push_back("jne");
    }
#endif

    std::string appPath;
    std::string appFile;
    for (int i = 0; i < executables.size(); ++i)
    {
        // Check in current directory
        appPath = GetAppPath(curDir, executables[i]);
        if (appPath.size())
        {
            appFile = executables[i];
            break;
        }

        // Check in config file's directory
        appPath = GetAppPath(execSettings.scenarioFilePath, executables[i]);
        if (appPath.size())
        {
            appFile = executables[i];
            break;
        }
    }

    if (appPath.size() == 0)
    {
        logMessage(MSG_CRITICAL, "Could not find app path from working"
            " directory:\n\n  " + curDir + "\n\n or configFile:\n\n  " 
            + execSettings.scenarioFilePath  + "\n");
    }

    // Change working directory to scenario file
    std::string scenarioDir = extractDirPath(execSettings.scenarioFilePath);
    if (scenarioDir.size() == 0)
    {
        scenarioDir = ".";
    }

    // child
    std::string simulationMode("-simulation");
    if (execSettings.isEmulationMode)
    {
        simulationMode = "-emulation";
    }
    std::stringstream command_buf;

    std::string n_configFile = normalizedFilePath(execSettings.scenarioFilePath);
    n_configFile = extractFileName(n_configFile);
    std::string n_scenarioDir = normalizedFilePath(scenarioDir);
    std::string n_appDir = normalizedFilePath(appPath);
    n_appDir = extractDirPath(n_appDir);
    std::string remoteRunMode("Non-Distributed");
    
    // Get the stat file name as configFileName_MMM_dd_yy_hh_mm_ss
    time_t now = time(0);
    struct tm* tstruct;
    char buf[80];
    tstruct = localtime(&now);
    strftime(buf, sizeof(buf), "_%b_%d_%y_%H_%M_%S", tstruct);
    std::string statFileName(buf);
    // Strip out the .config
    std::string config_file_name = n_configFile.substr(0, n_configFile.find_last_of('.'));
    statFileName = config_file_name + statFileName;

    if (execSettings.isRemote) 
    {
        if (execSettings.isDistributed)
        {
            remoteRunMode = "Distributed";
        }
        command_buf << appPath                    // SIM_EXEC
            << " \"" << n_configFile << "\" "     // CONFIG_FILE
            << " " << execSettings.interactivePort << " "          // INTERACTIVE_PORT
            << " \"" << n_scenarioDir << "\" "    // CONFIG_DIR
            << " " << execSettings.numberOfProcessors << " "       // NUM_PROCESSORS
            << " \"" << execSettings.remoteLoginAndHost << "\" "   // LOGIN_AT_HOST
            << " \"" << execSettings.distributedHostFile << "\" "  // HOST_FILE
            << " \"" << execSettings.remoteProductHome << "\" "    // REMOTE_PRODUCT_HOME
            << " \"" << remoteRunMode << "\" "    // REMOTE_RUN_MODE (ie. Distributed)
            << " \"" << n_appDir << "\" "         // PRODUCT_HOME/bin (needed for windows bat)
            << " " << simulationMode << " "       // SIMULATION_MODE
            << " " << statFileName << " "         // STAT_FILE_NAME
            << " player ";                        // GUI_MODE
    }
    else if (execSettings.isDistributed)
    {
        command_buf << "mpirun -np " << execSettings.numberOfProcessors << " "      //NUM_PROCESSORS
            << " -hostfile " << " \"" << execSettings.distributedHostFile << "\" "  //HOST_FILE
            << " \"" << appPath << "\" "          // SIM_EXEC
            << " \"" << n_configFile << "\" "     // CONFIG_FILE
            << " -with_ocula_gui " << execSettings.interactivePort // INTERACTIVE_OPTIONS
            << " " << simulationMode << " ";      // SIMULATION_MODE
    }
    else 
    {
        command_buf << appPath << " \"" << n_configFile << "\" "
            << " -with_ocula_gui " << execSettings.interactivePort
            << " -np " << execSettings.numberOfProcessors
            << " " << simulationMode;
    }
    std::string command = command_buf.str();
    printf("Executing %s\n", command.c_str()); fflush(stdout);
#ifdef _WIN32
    // Receive stdout
    SECURITY_ATTRIBUTES saAttr; 
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
    saAttr.bInheritHandle = TRUE; 
    saAttr.lpSecurityDescriptor = NULL; 

    if (!CreatePipe(&m_qualnetStdoutRead, &m_qualnetStdoutWrite, &saAttr, 0))
    {
        logMessage(MSG_CRITICAL, "Could not create qualnet stdout pipe\n");
    }

    // Ensure the read handle to the pipe for STDOUT is not inherited.
    if (!SetHandleInformation(m_qualnetStdoutRead, HANDLE_FLAG_INHERIT, 0))
    {
        logMessage(MSG_CRITICAL, "Could not send handle information\n");
    }

    STARTUPINFO si;
    memset(&si, 0, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    si.hStdOutput = m_qualnetStdoutWrite;
    si.dwFlags |= STARTF_USESTDHANDLES;
    
    if (!CreateProcess(NULL, // no module name (use command line)
       (LPSTR) command.c_str(), // szcmdline, // command line
       NULL, // process handle not inheritable
       NULL, // thread handle not inheritable
       true, // set handle inheritance to true
       CREATE_NO_WINDOW, // CREATE_DEFAULT_ERROR_MODE | NORMAL_PRIORITY_CLASS, 
                         // process create flags
       NULL, // use parent's environment block
       scenarioDir.c_str(), // working directory 
       &si, // pointer to startupinfo structure
       &m_simProcInfo)) // pointer to process_information structure 
    {
        logMessage(MSG_CRITICAL, "Could not execute simulator\n");
    }
#else
    pid_t m_pid = fork();
    if (m_pid == 0)
    {
        chdir(scenarioDir.c_str());
        std::string argNumberOfProcessors;
        {
            std::stringstream out;
            out << execSettings.numberOfProcessors;
            argNumberOfProcessors = out.str();
        }
        if (execSettings.isRemote) 
        {
            execl(
                appPath.c_str(), 
                appFile.c_str(), 
                n_configFile.c_str(), 
                execSettings.interactivePortString().c_str(),
                n_scenarioDir.c_str(),
                argNumberOfProcessors.c_str(),
                execSettings.remoteLoginAndHost.c_str(),
                execSettings.distributedHostFile.c_str(),
                execSettings.remoteProductHome.c_str(),
                remoteRunMode.c_str(),
                n_appDir.c_str(),
                simulationMode.c_str(),
                statFileName.c_str(),
                "player", 
                NULL
            );
        }
        else if (execSettings.isDistributed)
        {
            execl(
                "mpirun",
                "mpirun",
                "-np",
                argNumberOfProcessors.c_str(),
                "-hostfile",
                execSettings.distributedHostFile.c_str(),
                appPath.c_str(),
                execSettings.scenarioFilePath.c_str(),
                "-with_ocula_gui",
                execSettings.interactivePortString().c_str(), 
                simulationMode.c_str(),
                NULL
            );
        }
        else 
        {
            execl(
                appPath.c_str(), 
                appFile.c_str(), 
                execSettings.scenarioFilePath.c_str(), 
                "-np",
                argNumberOfProcessors.c_str(),
                "-with_ocula_gui", 
                execSettings.interactivePortString().c_str(), 
                NULL
            );
        }
        logMessage(MSG_CRITICAL, "Could not execute simulator\n");
    }
#endif
}

void Vops::setupSimulatorConnection(const ExecSettings& execSettings)
{
    m_state->getInterface()->setHostAddress(execSettings.interactiveHostAddress);
    m_state->setOculaPort(execSettings.interactivePort);
}

const std::string& Vops::getAppPath()
{
    // Build paths if we don't have them yet
    if (gAppPath == "")
    {
        getProductHome();
    }

    return gAppPath;
}

const std::string& Vops::getProductHome()
{
    // Return product home if we already have it
    if (gProductHome != "")
    {
        return gProductHome;
    }

    // Look for the simulator executable
    std::vector<std::string> executables;
#ifdef _WIN32
    executables.push_back("exata.exe");
    executables.push_back("jne.exe");
    executables.push_back("qualnet.exe");
#else
    executables.push_back("exata");
    executables.push_back("jne");
    executables.push_back("qualnet");
#endif

    // Go back directories until we see a license_dir
    // Launch qualnet using default.config
    std::string curDir = getcwd(NULL, 0);

    std::string appPath;
    for (int i = 0; i < executables.size(); ++i)
    {
        // Check in current directory
        appPath = GetAppPath(curDir, executables[i]);
        if (appPath.size())
        {
            break;
        }
    }

    if (appPath.size() == 0)
    {
        logMessage(MSG_CRITICAL, "Could not find app path from" 
            "working directory:\n\n  " + curDir + "\n");
    }

    gAppPath = appPath;

    // Create product home by going up 2 directories
    gProductHome = appPath;
    int numErasures = 0;
    while (gProductHome.size() > 0
        && numErasures < 2)
    {
        if (gProductHome[gProductHome.size() - 1] == '\\'
            || gProductHome[gProductHome.size() - 1] == '/')
        {
            numErasures++;
        }
        gProductHome.erase(gProductHome.size() - 1);
    }

    return gProductHome;
}

bool Vops::connectToSimulator()
{
    printf("\nTrying to connect to simulator\n");
    m_state->getInterface()->connectToSops();
    time_t timeStarted = time(NULL);
    time_t timeNow = timeStarted;
    while (!connectedToSimulator() && 
           (timeNow - timeStarted) < SOCKET_CONNECT_TIMEOUT_SECONDS)
    {
        OPS_Sleep(1);
        readSimulatorStdout();
        timeNow = time(NULL);
    }
    if (!connectedToSimulator())
    {
        logMessage(MSG_CRITICAL, "Could not connect to simulator. "
            "The server process might not be available on the specified port.\n");
        return false;
    }
    printf("\nConnected to simulator\n");
    m_state->getInterface()->subscribe();    
    return true;
}

bool Vops::connectedToSimulator()
{
    return m_state->isConnected();
}

void Vops::disconnectFromSimulator()
{
    m_state->getInterface()->unsubscribe();
}

bool Vops::readFromSimulator()
{
    m_state->executeCallbacks();
    readSimulatorStdout();
    return true;
}

void Vops::readSimulatorStdout()
{
#ifdef _WIN32
   DWORD dwRead, dwAvail;
   // Character buffer should be large enough to contain
   // detailed error messages.
   CHAR chBuf[2001];
   BOOL bSuccess = FALSE;

   while (true)
   { 
        // Check for data
        bSuccess = PeekNamedPipe(
            m_qualnetStdoutRead,
            chBuf,
            2000,
            &dwRead,
            &dwAvail,
            NULL);
        if (!bSuccess || dwAvail == 0)
        {
            break;
        }

        bSuccess = ReadFile(m_qualnetStdoutRead, chBuf, 2000, &dwRead, NULL);
        if (!bSuccess || dwRead == 0)
        {
            break; 
        }
        chBuf[dwRead] = 0;

        if (m_stdoutCallback)
        {
            (*m_stdoutCallback)("stdout", NULL, chBuf, NULL);
        }
   } 
#endif
}

void Vops::registerStdoutCallback(VisualizerCallback callback)
{
    m_stdoutCallback = callback;
}

bool Vops::isPaused()
{
    return m_paused;
}

void Vops::pause()
{
    m_state->getInterface()->pause();
    m_paused = true;
}

void Vops::play()
{
    m_state->getInterface()->play();
    m_paused = false;
}

// Issue a HITL command to the simulator
void Vops::issueHitlCommand(const std::string& command)
{
    m_state->getInterface()->sendRpcCommand(Hitl, command);
}

void Vops::logMessage(MessageSeverity severity, const std::string& msg)
{
#ifndef OCULA_TESTER
    getApp()->outputMessage(static_cast<BaseApplication::MessageSeverity>(severity), msg);
#endif
}
