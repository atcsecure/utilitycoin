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
    int64_t fLastSignature;


public:
    virtual bool ProcessMessage(CNode* pfrom, std::string strCommand, CDataStream& data);

    bool Enable(CTxIn txIn, int64_t time);

    CTxIn GetTxIn()
    {
        return fTxIn;
    }

    void SetTxIn(CTxIn value)
    {
        fTxIn = value;
    }


};

#endif // UTILITYSERVICENODE_H
