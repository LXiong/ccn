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

#ifndef VRLINK_HLA_H
#define VRLINK_HLA_H

const unsigned g_vrlinkFederationNameBufSize = 64;
const unsigned g_vrlinkFederateNameBufSize   = 64;
const unsigned g_vrlinkPathBufSize            = 256;

// /**
// CLASS        ::  VRLinkHLA
// DESCRIPTION  ::  Class to store HLA information.
// **/
class VRLinkHLA : public VRLink
{
private:
    double      m_rprFomVersion;
    char        m_federationName[g_vrlinkFederationNameBufSize];
    char        m_fedFilePath[g_vrlinkPathBufSize];
    char        m_federateName[g_vrlinkFederateNameBufSize];

public:
    VRLinkHLA();
    virtual ~VRLinkHLA();

    void CreateFederation();

    void InitConnectionSetting(char connectionVar1[],
                                char connectionVar2[],
                                char connectionVar3[],
                                char connectionVar4[]);
    void PublishObjects();

    void SendCommEffectsRequest(unsigned   srcNodeId,
        bool      isData,
        unsigned   msgSize,
        double     timeoutDelay,
        bool       dstNodeIdPresent,
        unsigned   dstNodeId);
};

// /**
// CLASS :: VRLinkHLA13
// DESCRIPTION :: Class to store HLA1.3 information.
// **/
class VRLinkHLA13 : public VRLinkHLA
{
public:
    VRLinkHLA13();
};

// /**
// CLASS :: VRLinkHLA1516
// DESCRIPTION :: Class to store HLA1516 information.
// **/
class VRLinkHLA1516 : public VRLinkHLA
{
public:
    VRLinkHLA1516();
};

#endif //VRLINK_HLA_H
