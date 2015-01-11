#include "utilityservicenode.h"

bool CServiceNode::Init(std::string strSharedPrivateKey, std::string &strErrorMessage)
{
    // shared private key
    CBitcoinSecret bitcoinSecret;

    bitcoinSecret.SetString(strSharedPrivateKey);

    if (!bitcoinSecret.IsValid())
    {
        strErrorMessage = "Invalid shared private key";

        return false;
    }

    bool compressed;

    CSecret secret = bitcoinSecret.GetSecret(compressed);

    fSharedPrivateKey.SetSecret(secret, true);
    fSharedPublicKey = fSharedPrivateKey.GetPubKey();

    return true;
}

bool CServiceNode::ProcessMessage(CNode* pfrom, std::string strCommand, CDataStream& data)
{

    if (pfrom->nVersion < SERVICENODE_MIN_PROTOVERSION) {
        return false;
    }

    if (SERVICENODE_REQ_PROTOVERSION > 0)
    {
        if (pfrom->nVersion != SERVICENODE_REQ_PROTOVERSION)
            return false;
    }

    return CUtilityNode::ProcessMessage(pfrom, strCommand, data);
}

bool CServiceNode::StartServiceNode(CStartServiceNodeMessage& message, std::string& strErrorMessage)
{
    if (!CUtilityNode::StartServiceNode(message, strErrorMessage))
        return false;

    if (fSharedPublicKey == message.GetSharedPublicKey())
    {
        fState = kStarted;
        fTxIn = message.GetTxIn();
        fSignatureTime = message.GetTime();
    }

    return true;
}

bool CServiceNode::StopServiceNode(CStopServiceNodeMessage& message, std::string& strErrorMessage)
{
    if (!CUtilityNode::StopServiceNode(message, strErrorMessage))
        return false;

    if (fSharedPublicKey == message.GetSharedPublicKey())
    {
        fState = kStopped;
    }

    return true;
}


bool CServiceNode::GetPingMessage(CPingServiceNodeMessage& message, std::string& strErrorMessage)
{
    message.SetTime(GetAdjustedTime());
    message.SetTxIn(fTxIn);
    message.SetSharedPublicKey(fSharedPublicKey);

    if (!message.Sign(fSharedPrivateKey, strErrorMessage))
        return false;

    if (!message.Verify(fSharedPublicKey, strErrorMessage))
        return false;

    return true;
}

bool CServiceNode::Ping(std::string &strErrorMessage)
{
    printf("ServiceNode::Ping\n");

    // check if the wallet is syncing
    if (!CheckInitialBlockDownloadFinished(strErrorMessage))
        return false;

    // TODO check if it exists in the list of servicenodes

    if (!IsStarted())
    {
        strErrorMessage = "Slave not started";

        return false;
    }

    CPingServiceNodeMessage message;

    if (!GetPingMessage(message, strErrorMessage))
        return false;

    RelayMessage(message);

    return true;
}
