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
#include "serialize.h"

#define CONTROLNODE_MIN_CONFIRMATIONS                   60
#define CONTROLNODE_COINS_REQUIRED                      2500

#define UTILITYNODE_MIN_PROTOVERSION                    6014
#define SERVICENODE_MIN_PROTOVERSION                    6014
#define CONTROLNODE_MIN_PROTOVERSION                    6014

#define UTILITYNODE_REQ_PROTOVERSION                    -1
#define SERVICENODE_REQ_PROTOVERSION                    -1
#define CONTROLNODE_REQ_PROTOVERSION                    -1

#define UTILITYNODE_SECONDS_BETWEEN_GETSERVICENODEINFO   5 * 60
#define UTILITYNODE_SECONDS_BETWEEN_GETSERVICENODELIST   60 * 60

#define SERVICENODE_MIN_CONFIRMATIONS                   CONTROLNODE_MIN_CONFIRMATIONS
#define SERVICENODE_COINS_REQUIRED                      CONTROLNODE_COINS_REQUIRED
#define SERVICENODE_MAINNET_PORT                        39999
#define SERVICENODE_TESTNET_PORT                        39998
#define SERVICENODE_SECONDS_BETWEEN_UPDATES             4*60
#define SERVICENODE_SECONDS_BETWEEN_EXPIRATION          60*60
#define SERVICENODE_SECONDS_BETWEEN_REMOVAL             60*60
#define SERVICENODE_TIME_PENALTY                        2*60*60
#define SERVICENODE_MAX_PROCESSING_TIME                 60*10

enum ServiceNodeState {

    kStopped,
    kStarted,
    kProcessingStart,
    kProcessingStop
};

std::string GetServiceNodeStateString(ServiceNodeState state);








// service node information
class CServiceNodeInfo
{
protected:
    CAddress fInetAddress;
    CTxIn fTxIn;
    CPubKey fWalletPublicKey;
    CPubKey fSharedPublicKey;
    std::vector<unsigned char> fSignature;
    int64_t fSignatureTime;
    int64_t fLastPing;
    int64_t fLastStart;
    int64_t fLastStop;
    int64_t fLastSeen;
    int64_t fTimeStopped;

    int fCurrent;
    int fCount;

    ServiceNodeState fState;

    bool GetNodeMessage(CNodeMessage& message, std::string& strErrorMessage);

public:
    CServiceNodeInfo();

    bool Init(std::string strInetAddress, std::string &strErrorMessage);

    virtual bool IsValid();
    virtual bool IsRemove();

    virtual bool Check(std::string& strErrorMessage);

    virtual void UpdateState();

    bool GetStartMessage(CStartServiceNodeMessage& message, int serviceNodeCount, int serviceNodeIndex, std::string& strErrorMessage);

    bool CheckInetAddressValid(std::string& strErrorMessage);

    bool Connect(std::string& strErrorMessage);

    std::string ToString(bool extensive = false);

    bool IsUpdatedWithin(int seconds)
    {
        return (GetAdjustedTime() - fLastSeen) < seconds;
    }

    bool IsProcessing()
    {
        return fState == kProcessingStart || fState == kProcessingStop;
    }

    bool IsStarted()
    {
        return fState == kStarted;
    }

    bool IsStopped()
    {
        return fState == kStopped;
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

    ServiceNodeState GetState()
    {
        return fState;
    }

    void SetState(ServiceNodeState value)
    {
        fState = value;
    }

    int64_t GetTimeStopped()
    {
        return fTimeStopped;
    }

    void SetTimeStopped(int64_t value)
    {
        fTimeStopped = value;
    }

    std::vector<unsigned char> GetSignature()
    {
        return fSignature;
    }

    void SetSignature(std::vector<unsigned char> value)
    {
        fSignature = value;
    }

    int64_t GetSignatureTime()
    {
        return fSignatureTime;
    }

    void SetSignatureTime(int64_t value)
    {
        fSignatureTime = value;
    }

    int64_t GetLastPing()
    {
        return fLastPing;
    }

    void SetLastPing(int64_t value)
    {
        fLastPing = value;
    }

    int64_t GetLastStart()
    {
        return fLastStart;
    }

    void SetLastStart(int64_t value)
    {
        fLastStart = value;
    }

    int64_t GetLastStop()
    {
        return fLastStop;
    }

    void SetLastStop(int64_t value)
    {
        fLastStop = value;
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

    std::string GetStateString()
    {
        return GetServiceNodeStateString(fState);
    }
};










class CNodeMessageRecord
{
protected:
    int64_t fTime;
    std::string fAddress;
    boost::shared_ptr<CNodeMessage> fMessage;

public:
    CNodeMessageRecord()
    {
    }

    CNodeMessageRecord(std::string address, boost::shared_ptr<CNodeMessage> message, int64_t time)
    {
        fAddress = address;
        fMessage = message;
        fTime = time;
    }

    std::string GetNodeAddress()
    {
        return fAddress;
    }

    CNodeMessage* GetNodeMessage()
    {
        return fMessage.get();
    }

    int64_t GetTime()
    {
        return fTime;
    }
};











// basic node, all wallets are a node
class CUtilityNode
{
protected:
    int64_t fLastSyncServiceNodeList;
    int fNumSyncServiceNodeListRequests;

    std::vector< boost::shared_ptr<CServiceNodeInfo> > fServiceNodes;

    std::set<COutPoint> fLockedOutPoints;

    // maps of responses and request per node address
    std::vector<CNodeMessageRecord> fResponses;
    std::vector<CNodeMessageRecord> fRequests;

    bool DelServiceNode(CTxIn txIn, std::string &strErrorMessage);

    bool IsRemoveRecord(CNodeMessageRecord& record);

public:
    CUtilityNode()
    {
        fLastSyncServiceNodeList = 0;
        fNumSyncServiceNodeListRequests = 0;
    }

    virtual std::string Test();

    virtual bool ProcessMessage(CNode* pfrom, std::string strCommand, CDataStream& data);
    virtual void UpdateLocks();

    virtual bool AcceptStartMessage(CServiceNodeInfo* node, CStartServiceNodeMessage& message);

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

    std::vector< boost::shared_ptr<CServiceNodeInfo> > GetServiceNodes()
    {
        return fServiceNodes;
    }

    bool HasServiceNode(CTxIn txIn);

    CServiceNodeInfo* GetServiceNode(CTxIn txIn);
    CServiceNodeInfo* GetServiceNode(CStartServiceNodeMessage& message);

    int GetServiceNodeIndex(CServiceNodeInfo* node);



    virtual bool StartServiceNode(CStartServiceNodeMessage& message, std::string& strErrorMessage);
    virtual bool StopServiceNode(CStopServiceNodeMessage& message, std::string& strErrorMessage);

    void CleanNodeMessageRecords();

    void SyncServiceNodeList();
    void UpdateServiceNodeList();

    void RelayMessage(CNodeMessage& message);

    bool HasRequestRecord(CNodeMessage& message, std::string address = "");
    bool HasResponseRecord(CNodeMessage& message, std::string address = "");

};

bool IsServiceNode(CUtilityNode *node);
bool IsControlNode(CUtilityNode *node);
bool IsSlaveNodeInfo(CServiceNodeInfo* info);
bool CheckWalletLocked(std::string &strErrorMessage);
bool CheckInitialBlockDownloadFinished(std::string &strErrorMessage);
bool CheckSignatureTime(int64_t time, std::string& strErrorMessage);
bool CheckPublicKey(CPubKey key, std::string &strErrorMessage);
bool CheckServiceNodeInetAddressValid(CAddress address, std::string &strErrorMessage);
bool CheckTxInAssociation(CTxIn txIn, CPubKey pubKey, std::string &strErrorMessage);
bool CheckTxInUnspent(CTxIn txIn, std::string &strErrorMessage);
bool CheckTxInConfirmations(CTxIn txIn, std::string &strErrorMessage);
bool GetBitcoinAddress(CPubKey pubKey, CBitcoinAddress& address, std::string &strErrorMessage);
bool GetPublicKey(CBitcoinAddress address, CPubKey& pubKey, std::string &strErrorMessage);
bool GetPrivateKey(CBitcoinAddress address, CKey &key, std::string &strErrorMessage);
std::string GetReadableTimeSpan(int64_t time);


// threads
void ThreadUtilityNodeTimers(void *parg);

#endif
