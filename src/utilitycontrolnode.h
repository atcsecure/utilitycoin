// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef UTILITYCONTROLNODE_H
#define UTILITYCONTROLNODE_H

#include "utilitynodemessage.h"
#include "utilitynode.h"

// slave service node information
class CSlaveNodeInfo: public CServiceNodeInfo
{
protected:
    std::string fAlias;
    CBitcoinAddress fWalletAddress;
    CKey fSharedPrivateKey;
    int64_t fProcessingStartTime;

public:
    CSlaveNodeInfo();

    bool Init(std::string strAlias, std::string strWalletAddress, std::string strSharedPrivatekey, std::string strInetAddress, std::string &strErrorMessage);

    virtual bool IsValid();
    virtual bool IsRemove();

    virtual bool Check(std::string& strErrorMessage);

    bool GetStartMessage(CStartServiceNodeMessage& message, std::string& strErrorMessage);
    bool GetStopMessage(CStopServiceNodeMessage& message, std::string& strErrorMessage);

    bool UpdateWalletPublicKey(std::string &strErrorMessage);

    bool UpdateTxIn(std::string& strErrorMessage);
    bool UpdateTxIn(std::vector<COutput> vOutput, std::string& strErrorMessage);

    bool FindTxIn(CTxIn& txIn, std::string &strErrorMessage);
    bool FindTxIn(std::vector<COutput> vOutput, CTxIn &txIn, std::string &strErrorMessage);

    std::string GetAlias()
    {
        return fAlias;
    }

    CBitcoinAddress GetWalletAddress()
    {
        return fWalletAddress;
    }

    int64_t GetProcessingStartTime()
    {
        return fProcessingStartTime;
    }

    void SetProcessingStartTime(int64_t value)
    {
        fProcessingStartTime = value;
    }


};

// control node, used to activate, deactivate and monitor slave service
// nodes. Slave service nodes can only be activated and stay active with
// a valid input with the required amount of coins
class CControlNode: public CUtilityNode
{
protected:
    // TODO shared_ptr vector with slave nodes only for easy reference
    //std::vector< boost::shared_ptr<CServiceNodeInfo> > fSlaveNodes;

public:
    virtual std::string Test();

    virtual bool ProcessMessage(CNode* pfrom, std::string strCommand, CDataStream& data);

    virtual void UpdateLocks();

    virtual bool AcceptStartMessage(CServiceNodeInfo* node, CStartServiceNodeMessage& message);

    std::string GenerateSharedKey();

    bool HasSlaveNode(std::string alias);
    bool HasSlaveNode(CPubKey sharedPublicKey);

    CSlaveNodeInfo* GetSlaveNode(std::string alias);
    CSlaveNodeInfo* GetSlaveNode(CPubKey sharedPublicKey);

    std::vector<std::string> GetSlaveAliases();

    void RegisterSlaveNode(CSlaveNodeInfo &slave);

    bool StartSlaveNode(std::string alias, std::string& strErrorMessage);
    bool StopSlaveNode(std::string alias, std::string& strErrorMessage);

    bool GetMessageSignature(CSlaveNodeInfo slave, std::string strMessage, std::vector<unsigned char>& vsig,  std::string& strErrorMessage);
};

#endif // UTILITYCONTROLNODE_H
