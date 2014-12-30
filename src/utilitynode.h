// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef UTILITYNODE_H
#define UTILITYNODE_H

#include "utilitynodemessage.h"
#include "base58.h"
#include "protocol.h"
#include "net.h"
#include "script.h"
#include "wallet.h"
#include "main.h"

#define CONTROLNODE_MIN_CONFIRMATIONS           15
#define CONTROLNODE_COINS_REQUIRED              2500

#define MIN_UTILITYNODE_PROTOVERSION            6014
#define MIN_SERVICENODE_PROTOVERSION            6014
#define MIN_CONTROLNODE_PROTOVERSION            6014

#define REQ_UTILITYNODE_PROTOVERSION            -1
#define REQ_SERVICENODE_PROTOVERSION            -1
#define REQ_CONTROLNODE_PROTOVERSION            -1

#define SERVICENODE_MIN_CONFIRMATIONS           CONTROLNODE_MIN_CONFIRMATIONS
#define SERVICENODE_COINS_REQUIRED              CONTROLNODE_COINS_REQUIRED
#define SERVICENODE_MAINNET_PORT                39999
#define SERVICENODE_TESTNET_PORT                39998
#define SERVICENODE_SECONDS_BETWEEN_UPDATES     5*60
#define SERVICENODE_TIME_PENALTY                2*60*60

enum SlaveNodeState {
    kStarted,
    kStopped
};

// service node information
class CServiceNodeInfo
{
protected:
    CAddress fInetAddress;
    CTxIn fTxIn;
    CPubKey fWalletPublicKey;
    CPubKey fSharedPublicKey;

    int64_t fLastSignature;
    int64_t fLastPing;
    int64_t fLastSeen;

    int fCurrent;
    int fCount;

    SlaveNodeState fState;


public:
    CServiceNodeInfo();

    bool Init(std::string strInetAddress, std::string strErrorMessage);
    bool Init(CStartServiceNodeMessage& message);
    bool Update(CStartServiceNodeMessage& message, std::string strErrorMessage);

    virtual bool IsValid();

    virtual bool Check(std::string& strErrorMessage);
    virtual bool Update(std::string& strErrorMessage);

    bool CheckInetAddressValid(std::string& strErrorMessage);

    bool Connect(std::string& strErrorMessage);

    bool IsUpdatedWithin(int seconds)
    {
        return (GetAdjustedTime() - fLastSeen) < seconds;
    }

    CAddress GetInetAddress()
    {
        return fInetAddress;
    }

    void SetInetAddress(CAddress value)
    {
        fInetAddress = value;
    }

    CTxIn GetTxIn()
    {
        return fTxIn;
    }

    void SetTxIn(CTxIn value)
    {
        fTxIn = value;
    }

    CPubKey GetWalletPublicKey()
    {
        return fWalletPublicKey;
    }

    void SetWalletPublicKey(CPubKey value)
    {
        fWalletPublicKey = value;
    }

    CPubKey GetSharedPublicKey()
    {
        return fSharedPublicKey;
    }

    void SetSharedPublicKey(CPubKey value)
    {
        fSharedPublicKey = value;
    }

    SlaveNodeState GetState()
    {
        return fState;
    }

    void SetState(SlaveNodeState value)
    {
        fState = value;
    }

    int64_t GetLastSignature()
    {
        return fLastSignature;
    }

    void SetLastSignature(int64_t value)
    {
        fLastSignature = value;
    }

    int64_t GetLastPing()
    {
        return fLastPing;
    }

    void SetLastPing(int64_t value)
    {
        fLastPing = value;
    }

    int64_t GetLastSeen()
    {
        return fLastSeen;
    }

    void SetLastSeen(int64_t value)
    {
        fLastSeen = value;
    }

    int GetCount()
    {
        return fCount;
    }

    void SetCount(int value)
    {
        fCount = value;
    }

    int GetCurrent()
    {
        return fCurrent;
    }

    void SetCurrent(int value)
    {
        fCurrent = value;
    }
};

// basic node, all wallets are a node
class CUtilityNode
{
protected:
    std::vector<CServiceNodeInfo> fServiceNodes;
    std::set<COutPoint> fLockedOutPoints;

public:
    virtual bool ProcessMessage(CNode* pfrom, std::string strCommand, CDataStream& data);
    virtual void UpdateLocks();

    virtual bool HandleMessage(CNode* pfrom, CGetServiceNodeInfoMessage& message);
    virtual bool HandleMessage(CNode* pfrom, CGetServiceNodeListMessage& message);
    virtual bool HandleMessage(CNode* pfrom, CPingServiceNodeMessage& message);
    virtual bool HandleMessage(CNode* pfrom, CStartServiceNodeMessage& message);
    virtual bool HandleMessage(CNode* pfrom, CStopServiceNodeMessage& message);

    void LockOutPoint(COutPoint output);
    void UnlockOutPoint(COutPoint output);
    bool IsLockedOutPoint(COutPoint output) const;
    bool IsLockedOutPoint(uint256 hash, unsigned int n) const;
    bool HasLockedOutPoint(CTransaction tx);
    std::vector<COutPoint> GetLockedOutPoints();

    bool HasServiceNode(CServiceNodeInfo node);
    bool HasServiceNode(CTxIn txIn);
    bool GetServiceNode(CTxIn txIn, CServiceNodeInfo &node);

    void UpdateServiceNodeList();

    void RelayMessage(CNodeMessage& message);
};

bool IsServiceNode(CUtilityNode *node);
bool IsControlNode(CUtilityNode *node);
bool CheckWalletLocked(std::string &strErrorMessage);
bool CheckInitialBlockDownloadFinished(std::string strErrorMessage);
bool CheckPublicKey(CPubKey key, std::string strErrorMessage);
bool CheckServiceNodeInetAddressValid(CAddress address, std::string &strErrorMessage);
bool CheckTxInAssociation(CTxIn txIn, CPubKey pubKey, std::string &strErrorMessage);
bool CheckTxInUnspent(CTxIn txIn, std::string &strErrorMessage);
bool CheckTxInConfirmations(CTxIn txIn, std::string &strErrorMessage);
bool GetPublicKey(CBitcoinAddress address, CPubKey& pubKey, std::string &strErrorMessage);
bool GetPrivateKey(CBitcoinAddress address, CKey &key, std::string &strErrorMessage);

// threads
void ThreadUtilityNodeTimers(void *parg);

#endif
