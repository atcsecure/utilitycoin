// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef UTILITYSERVICENODE_H
#define UTILITYSERVICENODE_H

#include "utilitynode.h"

// service node, the nodes that actually service all nodes in the
// network and can be activated by a control node
class CServiceNode: public CUtilityNode
{
protected:
    CKey fSharedPrivateKey;
    CPubKey fSharedPublicKey;
    CTxIn fTxIn;
    int64_t fSignatureTime;
    ServiceNodeState fState;

    bool GetPingMessage(CPingServiceNodeMessage& message, std::string& strErrorMessage);

public:
    CServiceNode()
    {
        fState = kStopped;
    }

    bool Init(std::string strSharedPrivateKey, std::string& strErrorMessage);

    virtual bool ProcessMessage(CNode* pfrom, std::string strCommand, CDataStream& data);

    virtual bool StartServiceNode(CStartServiceNodeMessage& message, std::string& strErrorMessage);
    virtual bool StopServiceNode(CStopServiceNodeMessage& message, std::string& strErrorMessage);

    virtual bool Ping(std::string& strErrorMessage);

    bool IsStarted()
    {
        return fState == kStarted;
    }

    CTxIn GetTxIn()
    {
        return fTxIn;
    }

    void SetTxIn(CTxIn value)
    {
        fTxIn = value;
    }

    int64_t GetSignatureTime()
    {
        return fSignatureTime;
    }

    void SetSignatureTime(int64_t value)
    {
        fSignatureTime = value;
    }

    CKey GetSharedPrivateKey()
    {
        return fSharedPrivateKey;
    }

    CPubKey GetSharedPublicKey()
    {
        return fSharedPublicKey;
    }



};

#endif // UTILITYSERVICENODE_H
