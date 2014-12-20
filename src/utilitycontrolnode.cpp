#include "utilitycontrolnode.h"
#include "utilitynodemessage.h"
#include "init.h"

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

    bool bCompressed;

    CSecret secret = bitcoinSecret.GetSecret(bCompressed);

    fSharedPrivateKey.SetSecret(secret, bCompressed);
    fSharedPublicKey = fSharedPrivateKey.GetPubKey();

    return true;
}

bool CSlaveNodeInfo::IsStarted()
{
    return fState == kStarted;
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

bool CSlaveNodeInfo::UpdateTxIn(std::vector<COutput> vOutput, std::string& strErrorMessage)
{
    return FindTxIn(vOutput, fTxIn, strErrorMessage);
}

bool CSlaveNodeInfo::UpdateTxIn(std::string& strErrorMessage)
{
    return FindTxIn(fTxIn, strErrorMessage);
}

bool CSlaveNodeInfo::UpdateWalletPublicKey(std::string strErrorMessage)
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



bool CSlaveNodeInfo::GetNodeMessage(CNodeMessage &message, std::string &strErrorMessage)
{
    message.SetTime(GetAdjustedTime());

    return true;
}

bool CSlaveNodeInfo::GetStartMessage(CStartServiceNodeMessage& message, std::string& strErrorMessage)
{
    if (!GetNodeMessage(message, strErrorMessage))
        return false;

    message.SetTxIn(fTxIn);
    message.SetWalletPublicKey(fWalletPublicKey);
    message.SetSharedPublicKey(fSharedPublicKey);
    message.SetCount(fCount);
    message.SetCurrent(fCurrent);

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

bool CSlaveNodeInfo::Update(std::string &strErrorMessage)
{
    if (!UpdateTxIn(strErrorMessage))
        return false;

    if (!UpdateWalletPublicKey(strErrorMessage))
        return false;

    return true;
}

bool CControlNode::ProcessMessage(CNode* pfrom, std::string strCommand, CDataStream& data)
{
    if (pfrom->nVersion < MIN_CONTROLNODE_PROTOVERSION) {
        return false;
    }

    if (REQ_CONTROLNODE_PROTOVERSION > 0)
    {
        if (pfrom->nVersion != REQ_CONTROLNODE_PROTOVERSION)
            return false;
    }

    return CUtilityNode::ProcessMessage(pfrom, strCommand, data);
}

void CControlNode::UpdateLocks()
{
    CUtilityNode::UpdateLocks();

    std::vector<COutput> vOutput;

    pwalletMain->AvailableCoins(vOutput, true);

    std::pair<std::string, CSlaveNodeInfo> pair;

    BOOST_FOREACH(pair, fSlaveNodes)
    {
        CSlaveNodeInfo slave = pair.second;

        CTxIn txIn;

        std::string strErrorMessage;

        // TODO probably costs too much with a lot of inputs
        if (slave.FindTxIn(vOutput, txIn, strErrorMessage))
            if (!IsLockedOutPoint(txIn.prevout))
                LockOutPoint(txIn.prevout);
    }
}

std::string CControlNode::GenerateSharedKey()
{
    CKey key;

    bool compressed = false;

    key.MakeNewKey(compressed);

    CBitcoinSecret bitcoinsecret;

    bitcoinsecret.SetSecret(key.GetSecret(compressed), compressed);

    return bitcoinsecret.ToString();
}

std::vector<std::string> CControlNode::GetSlaveAliases()
{
    std::vector<std::string> valiases;

    std::pair<std::string, CSlaveNodeInfo> pair;

    BOOST_FOREACH(pair, fSlaveNodes)
        valiases.push_back(pair.first);

    return valiases;
}

bool CControlNode::HasSlave(std::string alias)
{
    return fSlaveNodes.find(alias) != fSlaveNodes.end();
}

void CControlNode::RegisterSlave(CSlaveNodeInfo slave)
{
    assert(!HasSlave(slave.GetAlias()));

    fSlaveNodes[slave.GetAlias()] = slave;
}

void CControlNode::UnregisterSlave(std::string alias)
{
    assert(HasSlave(alias));

    fSlaveNodes.erase(alias);
}

bool CControlNode::StartSlave(std::string alias, std::string& strErrorMessage)
{
    // check if the wallet is syncing
    if (!CheckInitialBlockDownloadFinished(strErrorMessage))
        return false;

    CSlaveNodeInfo slave = fSlaveNodes[alias];

    if (slave.IsStarted())
    {
        strErrorMessage = "Slave already started";

        return false;
    }

    if (!slave.Check(strErrorMessage))
        return false;

    if (!slave.Update(strErrorMessage))
        return false;

    if (!slave.Connect(strErrorMessage))
        return false;

    slave.SetCount(-1);
    slave.SetCurrent(-1);

    CStartServiceNodeMessage message;

    if (!slave.GetStartMessage(message, strErrorMessage))
        return false;

    slave.SetLastSignature(message.GetTime());
    slave.SetLastSeen(message.GetTime());

    // relay start message
    RelayMessage(message);

    BOOST_FOREACH(CServiceNodeInfo node, fServiceNodes)
    {
        if (node.GetTxIn() == slave.GetTxIn())
        {
            strErrorMessage = "Input is already associated to a service node";
            return false;
        }
    }

    // add to the service node list
    fServiceNodes.push_back(slave);

    return true;
}

bool CControlNode::StopSlave(std::string alias, std::string &strErrorMessage)
{
    // check if the wallet is syncing
    if (!CheckInitialBlockDownloadFinished(strErrorMessage))
        return false;

    CSlaveNodeInfo slave = fSlaveNodes[alias];

    if (!slave.IsStarted())
    {
        strErrorMessage = "Slave not started";

        return false;
    }

    if (!slave.Check(strErrorMessage))
        return false;

    if (!slave.Update(strErrorMessage))
        return false;

    if (!slave.Connect(strErrorMessage))
        return false;

    CStopServiceNodeMessage message;

    if (!slave.GetStopMessage(message, strErrorMessage))
        return false;

    RelayMessage(message);

    return true;
}
