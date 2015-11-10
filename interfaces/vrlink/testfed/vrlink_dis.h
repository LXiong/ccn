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

#ifndef VRLINK_DIS_H
#define VRLINK_DIS_H

// /**
// CLASS        ::  VRLinkDIS
// DESCRIPTION  ::  Class to store DIS information.
// **/
class VRLinkDIS : public VRLink
{
private:
    unsigned    m_portNumber;
    DtInetAddr  m_deviceAddress;
    DtInetAddr  m_destinationAddress;
    DtInetAddr  m_subnetMask;
    bool        isSubnetMaskSet;

    unsigned char exerciseIdentifier;
public:
    VRLinkDIS();
    virtual ~VRLinkDIS();

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
#endif //VRLINK_DIS_H
