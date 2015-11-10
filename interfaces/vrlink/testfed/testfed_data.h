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

#ifndef TESTFED_DATA_H
#define TESTFED_DATA_H

#include "vrlink.h"
#include "testfed_shared.h"
#include <vector>

const unsigned g_vrlinkFederationNameBufSize = 64;
const unsigned g_vrlinkFederateNameBufSize   = 64;
const unsigned g_vrlinkDeviceAddressBufSize   = 64;
const unsigned g_vrlinkDestinationAddressBufSize   = 64;
const unsigned g_vrlinkPathBufSize            = 256;
const unsigned g_vrlinkScenarioNameBufSize   = 64;

#define MAX_OUTGOING_MESSAGES 50000

typedef UInt8 Testfed_CommandType;

#define Testfed_CommandType_RegisterObjects 0
#define Testfed_CommandType_Quit 1
#define Testfed_CommandType_MoveEntity 2
#define Testfed_CommandType_ChangeEntityDamage 3
#define Testfed_CommandType_ChangeEntityOrientation 4
#define Testfed_CommandType_ChangeRadioTxState 5
#define Testfed_CommandType_SendCER 6
#define Testfed_CommandType_ChangeEntityVelocity 7

class Testfed_Command
{
public:
    virtual ~Testfed_Command() { };
    virtual Testfed_CommandType GetType() = 0;
    virtual void Print(char* time) = 0;
};

class Testfed_RegisterObjectsCommand : Testfed_Command
{
public:
    ~Testfed_RegisterObjectsCommand() {};
    Testfed_CommandType GetType() { return Testfed_CommandType_RegisterObjects; }
    void Print(char* time);
};

class Testfed_QuitCommand : Testfed_Command
{
public:
    ~Testfed_QuitCommand() {};
    Testfed_CommandType GetType() { return Testfed_CommandType_Quit; }
    void Print(char* time);
};

class Testfed_MoveEntityCommand : Testfed_Command
{
public:
    unsigned nodeId;
    double lat;
    double lon;
    double alt;

    ~Testfed_MoveEntityCommand() {};
    Testfed_CommandType GetType() { return Testfed_CommandType_MoveEntity; }
    void Print(char* time);
};

class Testfed_ChangeEntityVelocityCommand : Testfed_Command
{
public:
    unsigned nodeId;
    float x;
    float y;
    float z;

    ~Testfed_ChangeEntityVelocityCommand() {};
    Testfed_CommandType GetType() { return Testfed_CommandType_ChangeEntityVelocity; }
    void Print(char* time);
};


class Testfed_ChangeEntityOrientationCommand : Testfed_Command
{
public:
    unsigned nodeId;
    double psi;
    double theta;
    double phi;

    ~Testfed_ChangeEntityOrientationCommand() {};
    Testfed_CommandType GetType() { return Testfed_CommandType_ChangeEntityOrientation; }
    void Print(char* time);
};

class Testfed_ChangeEntityDamageCommand : Testfed_Command
{
public:
    unsigned nodeId;
    unsigned damageState;

    ~Testfed_ChangeEntityDamageCommand() {};
    Testfed_CommandType GetType() { return Testfed_CommandType_ChangeEntityDamage; }
    void Print(char* time);
};

class Testfed_ChangeRadioTxStateCommand : Testfed_Command
{
public:
    unsigned nodeId;
    unsigned txState;

    ~Testfed_ChangeRadioTxStateCommand() {};
    Testfed_CommandType GetType() { return Testfed_CommandType_ChangeRadioTxState; }
    void Print(char* time);
};

class Testfed_SendCERCommand : Testfed_Command
{
public:
    unsigned nodeId;
    double timeoutDelay;
    bool isData;
    unsigned dstNodeId;
    bool dstNodeIdPresent;
    unsigned size;

    ~Testfed_SendCERCommand() {};
    Testfed_CommandType GetType() { return Testfed_CommandType_SendCER; }
    void Print(char* time);
};

struct Testfed_CommandList
{
    clocktype time;
    Testfed_Command *message;
    Testfed_CommandList *next;
};

class TestfedData
{
public:
    TestfedData();

    void ProcessCommandLineArguments(unsigned argc, char* argv[]);

    void AddCommandToList(clocktype time,
        Testfed_Command *msg);
    
    Testfed_CommandList* RemoveFirstNode();

    void PrintMessageList(const char* prefix, std::vector<Testfed_CommandList*>& list);

    void HandleCommand(Testfed_Command* msg);

    void ReadTestfedCommandFile();

    void ParseRegisterCommand(char* iotoken,
        char* next,
        clocktype* time,
        Testfed_RegisterObjectsCommand** message);

    void ParseQuitCommand(char* iotoken,
        char* next,
        clocktype* time,
        Testfed_QuitCommand** message);

    void ParseMoveEntityCommand(char* iotoken,
        char* next,
        clocktype* time,
        Testfed_MoveEntityCommand** message);

    void ParseChangeEntityVelocityCommand(char* iotoken,
        char* next,
        clocktype* time,
        Testfed_ChangeEntityVelocityCommand** message);

    void ParseChangeEntityOrientationCommand(char* iotoken,
        char* next,
        clocktype* time,
        Testfed_ChangeEntityOrientationCommand** message);

    void ParseChangeEntityDamageCommand(char* iotoken,
        char* next,
        clocktype* time,
        Testfed_ChangeEntityDamageCommand** message);

    void ParseChangeRadioTxStateCommand(char* iotoken,
        char* next,
        clocktype* time,
        Testfed_ChangeRadioTxStateCommand** message);

    void ParseSendCommRequestCommand(char* iotoken,
        char* next,
        clocktype* time,
        Testfed_SendCERCommand** message);

    VRLinkExternalInterface* GetVRLink();
    void SetVRLink(VRLinkExternalInterface* obj);
    clocktype GetTickDelta();
    VRLinkInterfaceType GetProtocol();
    char* GetScenarioName();
    bool IsRegistered();
    void SetRegisteredFlag(bool val);
    char* GetConnectionVariable(int i);
    bool GetDebug();
    unsigned GetCmdListSize();

private:

    VRLinkInterfaceType protocol;
    char        m_scenarioName[g_vrlinkScenarioNameBufSize];
    char        m_testfedConnectionVariable1[32];
    char        m_testfedConnectionVariable2[64];
    char        m_testfedConnectionVariable3[64];
    char        m_testfedConnectionVariable4[256];

    VRLinkExternalInterface* vrlink;

    bool        m_debug;

    char        m_federationName[g_vrlinkFederationNameBufSize];
    char        m_fedFilePath[g_vrlinkPathBufSize];
    char        m_federateName[g_vrlinkFederateNameBufSize];
    double      m_rprFomVersion;

    unsigned    m_portNumber;
    char        m_deviceAddress[g_vrlinkDeviceAddressBufSize];
    char        m_destinationAddress[g_vrlinkDestinationAddressBufSize];
    char        m_subnetMask[g_vrlinkDeviceAddressBufSize];

    unsigned short m_siteId;
    unsigned short m_applicationId;

    bool        m_objectsRegistered;

    clocktype   m_tickDelta;

    std::vector<Testfed_CommandList*> cmdList;
};

void ShowUsage();

extern TestfedData*   g_testfedData;

#endif /* TESTFED_DATA_H */
