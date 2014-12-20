// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef UTILITYNODEMESSAGE_H
#define UTILITYNODEMESSAGE_H

#include "wallet.h"
#include "main.h"

class CUtilityNode;

extern const char UTILITY_MESSAGE_GETSERVICENODEINFO[];
extern const char UTILITY_MESSAGE_GETSERVICENODELIST[];
extern const char UTILITY_MESSAGE_PINGSERVICENODE[];
extern const char UTILITY_MESSAGE_STARTSERVICENODE[];
extern const char UTILITY_MESSAGE_STOPSERVICENODE[];


bool SignMessage(CBitcoinAddress address, std::string strMessage, std::vector<unsigned char> &vSignature, std::string &strErrorMessage);
bool SignMessage(CKey key, std::string strMessage, std::vector<unsigned char>& vSignature, std::string &strErrorMessage);
bool VerifyMessage(CBitcoinAddress address, std::string strMessage, std::vector<unsigned char> vSignature, std::string &strErrorMessage);
bool VerifyMessage(CKeyID keyID, std::string strMessage, std::vector<unsigned char> vSignature, std::string &strErrorMessage);

// abstract
class CNodeMessage
{
protected:
    std::vector<unsigned char> fSignature;
    int64_t fTime;


public:
    std::vector<unsigned char> GetSignature()
    {
        return fSignature;
    }

    void SetSignature(std::vector<unsigned char>& vSignature)
    {
        fSignature = vSignature;
    }

    int64_t GetTime()
    {
        return fTime;
    }

    void SetTime(int64_t value)
    {
        fTime = value;
    }

    virtual bool IsValid() = 0;
    virtual void Relay(CNode* destination) = 0;
};

// abstract
class CSignedNodeMessage: public CNodeMessage
{
public:
    virtual bool IsValid() = 0;
    virtual void Relay(CNode* destination) = 0;
    virtual std::string GetMessageString() = 0;


    bool Sign(CBitcoinAddress address, std::string strErrorMessage);
    bool Verify(CPubKey pubKey, std::string strErrorMessage);
};

class CGetServiceNodeInfoMessage: public CNodeMessage
{
protected:
    CTxIn fTxIn;

public:
    CGetServiceNodeInfoMessage() {}
    CGetServiceNodeInfoMessage(CDataStream &data);

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

    virtual bool IsValid();
    virtual void Relay(CNode *destination);
};


class CPingServiceNodeMessage: public CSignedNodeMessage
{
protected:
    CTxIn fTxIn;
    CAddress fInetAddress;

public:
    CPingServiceNodeMessage() {}
    CPingServiceNodeMessage(CDataStream &data);

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
};

class CStartServiceNodeMessage: public CSignedNodeMessage
{
protected:
    CAddress fInetAddress;
    CTxIn fTxIn;

    CPubKey fWalletPublicKey;
    CPubKey fSharedPublicKey;

    int fCount;
    int fCurrent;

public:
    CStartServiceNodeMessage(){}
    CStartServiceNodeMessage(CDataStream &data);

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

class CStopServiceNodeMessage: public CSignedNodeMessage
{
protected:
    CAddress fInetAddress;
    CTxIn fTxIn;

public:
    CStopServiceNodeMessage() {}
    CStopServiceNodeMessage(CDataStream& data);

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
};


#endif // UTILITYNODEMESSAGE_H
