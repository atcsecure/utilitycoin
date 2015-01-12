#include "utilitycontrolnode.h"
#include "utilitynodemessage.h"
#include "init.h"

#include <boost/make_shared.hpp>
#include <boost/format.hpp>

CSlaveNodeInfo::CSlaveNodeInfo()
    : CServiceNodeInfo()
{
}


bool CSlaveNodeInfo::Init(std::string strAlias, std::string strWalletAddress, std::string strSharedPrivatekey, std::string strInetAddress, std::string& strErrorMessage)
{
    if (!CServiceNodeInfo::Init(strInetAddress, strErrorMessage))
        return false;

    fAlias = strAlias;
    fState = kStopped;

    // wallet address
    fWalletAddress = CBitcoinAddress(strWalletAddress);

    if (!fWalletAddress.IsValid())
    {
        strErrorMessage = "Invalid wallet address";

        return false;
    }

    // shared private key
    CBitcoinSecret bitcoinSecret;

    bitcoinSecret.SetString(strSharedPrivatekey);

    if (!bitcoinSecret.IsValid())
    {
        strErrorMessage = "Invalid shared private key";

        return false;
    }

    bool compressed;

    CSecret secret = bitcoinSecret.GetSecret(compressed);

    fSharedPrivateKey.SetSecret(secret, compressed);
    fSharedPublicKey = fSharedPrivateKey.GetPubKey();

    return true;
}

bool CSlaveNodeInfo::IsValid()
{
    if (!CServiceNodeInfo::IsValid())
        return false;

    if (!fSharedPrivateKey.IsValid())
        return false;

    if (!fWalletAddress.IsValid())
        return false;

    return true;
}

bool CSlaveNodeInfo::IsRemove()
{
    // never remove slave nodes
    return false;
}

bool CSlaveNodeInfo::UpdateTxIn(std::vector<COutput> vOutput, std::string& strErrorMessage)
{
    return FindTxIn(vOutput, fTxIn, strErrorMessage);
}

bool CSlaveNodeInfo::UpdateTxIn(std::string& strErrorMessage)
{
    return FindTxIn(fTxIn, strErrorMessage);
}

bool CSlaveNodeInfo::UpdateWalletPublicKey(std::string& strErrorMessage)
{
    // get public key from wallet address
    if (!GetPublicKey(fWalletAddress, fWalletPublicKey, strErrorMessage))
        return false;

    return true;
}

bool CSlaveNodeInfo::FindTxIn(std::vector<COutput> vOutput, CTxIn &txIn, std::string &strErrorMessage)
{
    BOOST_FOREACH(const COutput& output, vOutput)
    {
        CTxOut txout = output.tx->vout[output.i];

        if (txout.nValue == (CONTROLNODE_COINS_REQUIRED * COIN))
        {
            CTxDestination destination;

            ExtractDestination(txout.scriptPubKey, destination);

            CBitcoinAddress address = CBitcoinAddress(destination);

            if (fWalletAddress == address)
            {
                if (output.nDepth < CONTROLNODE_MIN_CONFIRMATIONS)
                {
                    strErrorMessage = (boost::format("Input must have least %1% confirmations - %2% confirmations\n") % CONTROLNODE_MIN_CONFIRMATIONS % output.nDepth).str();

                    return false;
                }

                txIn = CTxIn(output.tx->GetHash(), output.i);

                return true;
            }
        }
    }

    strErrorMessage = (boost::format("Valid input of %1% UTIL or more not found") % CONTROLNODE_COINS_REQUIRED).str();

    return false;
}

bool CSlaveNodeInfo::FindTxIn(CTxIn& txIn, std::string &strErrorMessage)
{
    std::vector<COutput> vOutput;

    pwalletMain->AvailableCoins(vOutput, true);

    return FindTxIn(vOutput, txIn, strErrorMessage);
}





bool CSlaveNodeInfo::GetStartMessage(CStartServiceNodeMessage& message, std::string& strErrorMessage)
{
    if (!GetNodeMessage(message, strErrorMessage))
        return false;

    message.SetInetAddress(fInetAddress);
    message.SetTxIn(fTxIn);
    message.SetWalletPublicKey(fWalletPublicKey);
    message.SetSharedPublicKey(fSharedPublicKey);
    message.SetServiceNodeCount(fCount);
    message.SetServiceNodeIndex(fCurrent);

    if (!message.Sign(fWalletAddress, strErrorMessage))
        return false;

    if (!message.Verify(fWalletPublicKey, strErrorMessage))
        return false;

    return true;
}

bool CSlaveNodeInfo::GetStopMessage(CStopServiceNodeMessage& message, std::string& strErrorMessage)
{
    if (!GetNodeMessage(message, strErrorMessage))
        return false;

    std::vector<unsigned char> vSignature;

    message.SetTxIn(fTxIn);
    message.SetSharedPublicKey(fSharedPublicKey);

    if (!message.Sign(fWalletAddress, strErrorMessage))
        return false;

    if (!message.Verify(fWalletPublicKey, strErrorMessage))
        return false;

    return false;
}

bool CSlaveNodeInfo::Check(std::string &strErrorMessage)
{
    return CServiceNodeInfo::Check(strErrorMessage);
}








std::string CControlNode::Test()
{
    return "finished";
}


bool CControlNode::ProcessMessage(CNode* pfrom, std::string strCommand, CDataStream& data)
{
    if (pfrom->nVersion < CONTROLNODE_MIN_PROTOVERSION) {
        return false;
    }

    if (CONTROLNODE_REQ_PROTOVERSION > 0)
    {
        if (pfrom->nVersion != CONTROLNODE_REQ_PROTOVERSION)
            return false;
    }

    return CUtilityNode::ProcessMessage(pfrom, strCommand, data);
}

bool CControlNode::AcceptStartMessage(CServiceNodeInfo* node, CStartServiceNodeMessage& message)
{
    if (HasSlaveNode(message.GetSharedPublicKey()))
    {
        return !node->IsUpdatedWithin(SERVICENODE_SECONDS_BETWEEN_UPDATES);
    }

    return CUtilityNode::AcceptStartMessage(node, message);
}


void CControlNode::UpdateLocks()
{
    CUtilityNode::UpdateLocks();

    std::vector<COutput> vOutput;

    pwalletMain->AvailableCoins(vOutput, true);

    BOOST_FOREACH(boost::shared_ptr<CServiceNodeInfo> node, fServiceNodes)
    {
        if (IsSlaveNodeInfo(node.get()))
        {
            CSlaveNodeInfo* slave = (CSlaveNodeInfo*) node.get();

            CTxIn txIn;

            std::string strErrorMessage;

            // TODO probably costs too much with a lot of inputs
            if (slave->FindTxIn(vOutput, txIn, strErrorMessage))
                if (!IsLockedOutPoint(txIn.prevout))
                    LockOutPoint(txIn.prevout);
        }
    }
}

std::string CControlNode::GenerateSharedKey()
{
    CKey key;

    key.MakeNewKey(true);

    CBitcoinSecret bitcoinsecret;

    bool compressed;

    CSecret secret = key.GetSecret(compressed);

    bitcoinsecret.SetSecret(secret, compressed);

    return bitcoinsecret.ToString();
}

//void CControlNode::UpdateSlaveNodeList()
//{
//    for (int i = 0; i < fSlaveNodes.size(); i++)
//    {
//        if (fSlaveNodes[i].IsProcessing() && (GetAdjustedTime() - fSlaveNodes[i].GetProcessingStartTime() > SERVICENODE_MAX_PROCESSING_TIME))
//        {
//            if (fSlaveNodes[i].GetState() == kProcessingStart)
//                fSlaveNodes[i].SetState(kStopped);

//            if (fSlaveNodes[i].GetState() == kProcessingStop)
//                fSlaveNodes[i].SetState(kStarted);
//        }
//    }
//}


std::vector<std::string> CControlNode::GetSlaveAliases()
{
    std::vector<std::string> valiases;

    BOOST_FOREACH(boost::shared_ptr<CServiceNodeInfo> node, fServiceNodes)
    {
        if (IsSlaveNodeInfo(node.get()))
        {
            valiases.push_back(((CSlaveNodeInfo*)node.get())->GetAlias());
        }
    }

    return valiases;
}

bool CControlNode::HasSlaveNode(std::string alias)
{
    return GetSlaveNode(alias) != NULL;
}

bool CControlNode::HasSlaveNode(CPubKey sharedPublicKey)
{
    return GetSlaveNode(sharedPublicKey) != NULL;
}

CSlaveNodeInfo* CControlNode::GetSlaveNode(std::string alias)
{
    for (int i = 0; i < fServiceNodes.size(); i++)
    {
        if (IsSlaveNodeInfo(fServiceNodes[i].get()))
        {
            CSlaveNodeInfo* slave = (CSlaveNodeInfo*)fServiceNodes[i].get();

            if (slave->GetAlias() == alias)
            {
                return slave;
            }
        }
    }

    return NULL;
}

CSlaveNodeInfo* CControlNode::GetSlaveNode(CPubKey sharedPublicKey)
{
    for (int i = 0; i < fServiceNodes.size(); i++)
    {
        if (IsSlaveNodeInfo(fServiceNodes[i].get()))
        {
            CSlaveNodeInfo* slave = (CSlaveNodeInfo*)fServiceNodes[i].get();

            if (slave->GetSharedPublicKey() == sharedPublicKey)
            {
                return slave;
            }
        }
    }

    return NULL;
}

void CControlNode::RegisterSlaveNode(CSlaveNodeInfo& slave)
{
    assert(!HasSlaveNode(slave.GetAlias()));

    fServiceNodes.push_back(boost::make_shared<CSlaveNodeInfo>(slave));

    bool test = IsSlaveNodeInfo(fServiceNodes.back().get());

    assert(test);
}

bool CControlNode::StartSlaveNode(std::string alias, std::string& strErrorMessage)
{
    // check if the wallet is syncing
    if (!CheckInitialBlockDownloadFinished(strErrorMessage))
        return false;

    CSlaveNodeInfo* slave = GetSlaveNode(alias);

    if (slave == NULL)
    {
        strErrorMessage = (boost::format("Service node \"%1%\" not found") % alias).str();

        return false;
    }

    // TODO check if it exists in the list of servicenodes

    if (slave->IsProcessing())
    {
        strErrorMessage = "Service node is still being processed";

        return false;
    }

    if (slave->IsStarted())
    {
        strErrorMessage = "Service node already started";

        return false;
    }

    if (!slave->Check(strErrorMessage))
        return false;

    if (!slave->UpdateTxIn(strErrorMessage))
        return false;

    if (!slave->UpdateWalletPublicKey(strErrorMessage))
        return false;

    if (!slave->Connect(strErrorMessage))
        return false;

    slave->SetCount(-1);
    slave->SetCurrent(-1);

    CStartServiceNodeMessage message;

    if (!slave->GetStartMessage(message, strErrorMessage))
        return false;

    slave->SetSignatureTime(message.GetTime());
    slave->SetLastSeen(message.GetTime());

    slave->SetState(kProcessingStart);
    slave->SetProcessingStartTime(GetAdjustedTime());

    // relay start message
    RelayMessage(message);

    return true;
}

bool CControlNode::StopSlaveNode(std::string alias, std::string &strErrorMessage)
{
    // check if the wallet is syncing
    if (!CheckInitialBlockDownloadFinished(strErrorMessage))
        return false;

    // TODO check if it exists in the list of servicenodes

    CSlaveNodeInfo* slave = GetSlaveNode(alias);

    if (slave == NULL)
    {
        strErrorMessage = (boost::format("Service node \"%1%\" not found") % alias).str();

        return false;
    }

    if (slave->IsProcessing())
    {
        strErrorMessage = "Service node is still processing, please wait 5 minutes before retrying";

        return false;
    }

    if (!slave->IsStarted())
    {
        strErrorMessage = "Service node not started";

        return false;
    }

    CStopServiceNodeMessage message;

    if (!slave->GetStopMessage(message, strErrorMessage))
        return false;

    slave->SetState(kProcessingStop);
    slave->SetProcessingStartTime(GetAdjustedTime());

    RelayMessage(message);

    return true;
}
