// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef UTILITYNODEMESSAGE_H
#define UTILITYNODEMESSAGE_H

#include "wallet.h"
#include "main.h"
#include "util.h"

class CUtilityNode;

extern const char UTILITY_MESSAGE_GETSERVICENODEINFO[];
extern const char UTILITY_MESSAGE_GETSERVICENODELIST[];
extern const char UTILITY_MESSAGE_PINGSERVICENODE[];
extern const char UTILITY_MESSAGE_STARTSERVICENODE[];
extern const char UTILITY_MESSAGE_STOPSERVICENODE[];

class CNodeMessage;

bool IsGetServiceNodeInfoMessage(CNodeMessage* message);
bool IsGetServiceNodeListMessage(CNodeMessage* message);

class CNodeMessage
{
protected:
    int64_t fTime;

public:
    virtual bool Compare(CNodeMessage& message) = 0;

    int64_t GetTime()
    {
        return fTime;
    }

    void SetTime(int64_t value)
    {
        fTime = value;
    }

    virtual bool IsValid()
    {
        return true;
    }

    virtual void Relay(CNode* destination)
    {
    }
};

// abstract
class CSignedNodeMessage: public CNodeMessage
{
protected:
    std::vector<unsigned char> fSignature;

public:
    virtual bool Compare(CNodeMessage& message) = 0;

    virtual bool IsValid()
    {
        return true;
    }

    virtual void Relay(CNode* destination)
    {
    }

    virtual std::string GetMessageString()
    {
        return "";
    }

    bool Sign(CKey key, std::string& strErrorMessage);
    bool Sign(CBitcoinAddress address, std::string &strErrorMessage);
    bool Verify(CPubKey pubKey, std::string& strErrorMessage);

    std::vector<unsigned char> GetSignature()
    {
        return fSignature;
    }

    void SetSignature(std::vector<unsigned char>& vSignature)
    {
        fSignature = vSignature;
    }

};

class CGetServiceNodeInfoMessage: public CNodeMessage
{
protected:
    CTxIn fTxIn;

public:
    CGetServiceNodeInfoMessage() {}
    CGetServiceNodeInfoMessage(CDataStream &data);

    virtual bool Compare(CNodeMessage& message)
    {
        if (!IsGetServiceNodeInfoMessage(&message))
            return false;

        if (fTxIn != ((CGetServiceNodeInfoMessage*)&message)->GetTxIn())
            return false;

        return true;
    }

    CTxIn GetTxIn()
    {
        return fTxIn;
    }

    void SetTxIn(CTxIn value)
    {
        fTxIn = value;
    }

    virtual bool IsValid();

    virtual void Relay(CNode *destination);
};

class CGetServiceNodeListMessage: public CNodeMessage
{
public:
    CGetServiceNodeListMessage() {}

    virtual bool Compare(CNodeMessage& message)
    {
        if (!IsGetServiceNodeListMessage(&message))
            return false;

        return true;
    }

    virtual bool IsValid();
    virtual void Relay(CNode *destination);
};


class CPingServiceNodeMessage: public CSignedNodeMessage
{
protected:
    CTxIn fTxIn;
    CAddress fInetAddress;
    CPubKey fSharedPublicKey;

public:
    CPingServiceNodeMessage() {}
    CPingServiceNodeMessage(CDataStream &data);

    virtual bool Compare(CNodeMessage& message)
    {
        return false;
    }

    virtual bool IsValid();
    virtual void Relay(CNode *destination);
    virtual std::string GetMessageString();

    CTxIn GetTxIn()
    {
        return fTxIn;
    }

    void SetTxIn(CTxIn value)
    {
        fTxIn = value;
    }

    CPubKey GetSharedPublicKey()
    {
        return fSharedPublicKey;
    }

    void SetSharedPublicKey(CPubKey value)
    {
        fSharedPublicKey = value;
    }
};

class CStartServiceNodeMessage: public CSignedNodeMessage
{
protected:
    CAddress fInetAddress;
    CTxIn fTxIn;

    CPubKey fWalletPublicKey;
    CPubKey fSharedPublicKey;

    int fServiceNodeCount;
    int fServiceNodeIndex;

public:
    CStartServiceNodeMessage(){}
    CStartServiceNodeMessage(CDataStream &data);

    virtual bool Compare(CNodeMessage& message)
    {
        return false;
    }

    virtual bool IsValid();
    virtual void Relay(CNode *destination);
    virtual std::string GetMessageString();

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

    int GetServiceNodeCount()
    {
        return fServiceNodeCount;
    }

    void SetServiceNodeCount(int value)
    {
        fServiceNodeCount = value;
    }

    int GetServiceNodeIndex()
    {
        return fServiceNodeIndex;
    }

    void SetServiceNodeIndex(int value)
    {
        fServiceNodeIndex = value;
    }

};

class CStopServiceNodeMessage: public CSignedNodeMessage
{
protected:
    CAddress fInetAddress;
    CTxIn fTxIn;
    CPubKey fWalletPublicKey;
    CPubKey fSharedPublicKey;

public:
    CStopServiceNodeMessage() {}
    CStopServiceNodeMessage(CDataStream& data);

    virtual bool Compare(CNodeMessage& message)
    {
        return false;
    }

    virtual void Relay(CNode *destination);
    virtual bool IsValid();
    virtual std::string GetMessageString();

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

};

bool SignMessage(CBitcoinAddress address, std::string strMessage, std::vector<unsigned char> &vSignature, std::string &strErrorMessage);
bool SignMessage(CKey key, std::string strMessage, std::vector<unsigned char>& vSignature, std::string &strErrorMessage);
bool VerifyMessage(CBitcoinAddress address, std::string strMessage, std::vector<unsigned char> vSignature, std::string &strErrorMessage);
bool VerifyMessage(CKeyID keyID, std::string strMessage, std::vector<unsigned char> vSignature, std::string &strErrorMessage);


#endif // UTILITYNODEMESSAGE_H
