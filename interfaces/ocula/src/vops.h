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

#ifndef _VOPS_H_
#define _VOPS_H_

#ifdef _WIN32
#include <windows.h>
#endif

#include "ops_util.h"
#include "VopsRpcInterface.h"
#include "ExecSettings.h"
#include "CommonFunctions.h"

enum MessageSeverity {MSG_CRITICAL, MSG_ERROR, MSG_WARNING, 
                        MSG_NOTICE, MSG_DEBUG};

class Vops
{
public:
    VopsPropertyManager* m_state;

    Vops();
    virtual ~Vops();

    // Launch qualnet in the background.  Assumptions:
    // 1) The current working directory is a valid scenario directory, ie
    //    QUALNET_HOME/scenarios/default
    //    -OR- the configFIle parameter is a fully specified path, ie
    //    c:\qualnet\scenarios\default
    // 2) The scenario file is under a qualnet installation directory or the
    //    QUALNET_HOME environment variable is set correctly
    // 2) The execSettings.scenarioFilePath string is a valid scenario name, ie default.config
    // 3) There is a qualnet.exe in the QUALNET_HOME/bin directory
    void launchSimulator(const ExecSettings& execSettings);

    void setupSimulatorConnection(const ExecSettings& execSettings);

    static const std::string& getAppPath();
    static const std::string& getProductHome();

    // Connect to a launched simulator
    // registerStdoutCallback should be called before this function
    bool connectToSimulator();

    bool connectedToSimulator();

    void disconnectFromSimulator();

    // Return true if data was read from the simulator
    // Return false if no data was read
    bool readFromSimulator();

    VopsPropertyManager* getState(){return VopsPropertyManager::getInstance();};

    // Set a callback for receiving stdout data from the simulator
    // The stdout data will be streamed (each call may not end on a newline)
    // The data is passed in the newVal parameter
    void registerStdoutCallback(VisualizerCallback callback);

    // Return true if the simulator is paused.  Return false if the
    // simulator is playing.  The simulator begins in the playing
    // state.
    bool isPaused();

    // Pause the simulator if it is playing.  Calling pause() while the
    // simulator is already paused has no effect.
    void pause();
    
    // Play the simulator if it is paused.  Calling play() while the
    // simulator is already playing has no effect.
    void play();

    // Issue a HITL command to the simulator
    void issueHitlCommand(const std::string& command);
    static void logMessage(MessageSeverity severity, const std::string& msg);
private:
    bool m_paused;
    VisualizerCallback m_stdoutCallback;

#ifdef _WIN32
    HANDLE m_qualnetStdoutRead;
    HANDLE m_qualnetStdoutWrite;
#endif

    void readSimulatorStdout();

protected:
#ifdef _WIN32
    PROCESS_INFORMATION m_simProcInfo;
#else
    pid_t m_pid;
#endif
};

#endif /* _VOPS_H_ */

