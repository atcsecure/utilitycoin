#include "utilitynode.h"
#include "utilityservicenode.h"
#include "utilitycontrolnode.h"

#include "txdb-leveldb.h"
#include "init.h"
#include "main.h"

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

bool IsServiceNode(CUtilityNode* node)
{
    return (typeid(*node) == typeid(CServiceNode));
}

bool IsControlNode(CUtilityNode* node)
{
    return (typeid(*node) == typeid(CControlNode));
}

bool CheckWalletLocked(std::string &strErrorMessage)
{
    if (pwalletMain->IsLocked())
    {
        strErrorMessage = "Wallet needs to be unlocked";

        return false;
    }

    return true;
}

// check if the wallet is syncing
bool CheckInitialBlockDownloadFinished(std::string strErrorMessage)
{
    if (IsInitialBlockDownload())
    {
        strErrorMessage = "Initial block download in progress";

        return false;
    }

    return true;
}

bool CheckPublicKey(CPubKey key, std::string strErrorMessage)
{
    CScript script;

    script.SetDestination(key.GetID());

    if (script.size() != 25)
    {
        strErrorMessage = "Wrong size";
        return false;
    }

    return true;
}


bool CheckServiceNodeInetAddressValid(CAddress address, std::string &strErrorMessage)
{
    // check valid port
    unsigned short port;

    if(fTestNet)
        port = SERVICENODE_TESTNET_PORT;
    else
        port = SERVICENODE_MAINNET_PORT;

    if (address.GetPort() != port)
    {
        strErrorMessage = (boost::format("Invalid inet address port %1%, port %2% is required") % address.GetPort() % port).str();

        return false;
    }

    return true;
}

bool CheckTxInAssociation(CTxIn txIn, CPubKey pubKey, std::string &strErrorMessage)
{
    CScript script;
    script.SetDestination(pubKey.GetID());

    CTransaction tx;
    uint256 hash;

    if (!GetTransaction(txIn.prevout.hash, tx, hash))
    {
        strErrorMessage = "Transaction not found";

        return false;
    }

    BOOST_FOREACH(CTxOut out, tx.vout)
        if(out.nValue == SERVICENODE_COINS_REQUIRED*COIN && out.scriptPubKey == script)
            return true;

    strErrorMessage = "Required coins not found";

    return false;
}


bool CheckTxInUnspent(CTxIn txIn, std::string& strErrorMessage)
{
    // Check for conflicts with in-memory transactions
    if (mempool.mapNextTx.count(txIn.prevout))
    {
        strErrorMessage = "Tx in used in memory";

        return false;
    }

    CTransaction tx;

    tx.vin.push_back(txIn);
    tx.vout.push_back(CTxOut((SERVICENODE_COINS_REQUIRED*COIN) - MIN_TX_FEE, CScript()));

    CTxDB txdb("r");
    MapPrevTx mapInputs;
    std::map<uint256, CTxIndex> mapUnused;
    bool bInvalid = false;

    if (!tx.FetchInputs(txdb, mapUnused, true, true, mapInputs, bInvalid))
    {
        if (bInvalid)
        {
            strErrorMessage = "Invalid tx inputs";

            return false;
        }

        strErrorMessage = "Unable to get tx inputs";

        return false;
    }

    //
    //
    // TODO check unspent

//    // Check against previous transactions
//    // This is done last to help prevent CPU exhaustion denial-of-service attacks.
//    if (!tx.ConnectInputs(txdb, mapInputs, mapUnused, CDiskTxPos(1,1,1), pindexBest, true, true))
//    {
//        strErrorMessage = "Unable to connect tx inputs";

//        return false;
//    }

    return true;
}

bool CheckTxInConfirmations(CTxIn txIn, int confirmations, std::string &strErrorMessage)
{
    // TODO
    return true;
}

bool GetPublicKey(CBitcoinAddress address, CPubKey &pubKey, std::string &strErrorMessage)
{
    CKey key;

    if (!GetPrivateKey(address, key, strErrorMessage))
        return false;

    pubKey = key.GetPubKey();

    return true;
}


bool GetPrivateKey(CBitcoinAddress address, CKey& key, std::string& strErrorMessage)
{
    if(!CheckWalletLocked(strErrorMessage))
        return false;

    CKeyID keyID;

    if (!address.GetKeyID(keyID)) {
        strErrorMessage = "Address does not refer to a key\n";
        return false;
    }

    if (!pwalletMain->GetKey(keyID, key)) {
        strErrorMessage = "Private key for address is not known\n";
        return false;
    }

    return true;
}






CServiceNodeInfo::CServiceNodeInfo()
{
}

bool CServiceNodeInfo::Init(std::string strInetAddress, std::string strErrorMessage)
{
    fInetAddress = CAddress(CService(strInetAddress));

    if (!fInetAddress.IsValid())
    {
        strErrorMessage = "Invalid ip address";

        return false;
    }

    return true;
}

bool CServiceNodeInfo::Update(CStartServiceNodeMessage& message, std::string strErrorMessage)
{

}

bool CServiceNodeInfo::IsValid()
{
    return fInetAddress.IsValid();
}

bool CServiceNodeInfo::Check(std::string &strErrorMessage)
{
    if (!CheckInetAddressValid(strErrorMessage))
        return false;

    return true;
}

bool CServiceNodeInfo::Update(std::string &strErrorMessage)
{
    return true;
}

bool CServiceNodeInfo::CheckInetAddressValid(std::string& strErrorMessage)
{
    return CheckServiceNodeInetAddressValid(fInetAddress, strErrorMessage);
}

bool CServiceNodeInfo::Connect(std::string& strErrorMessage)
{
    // check connection
    if(ConnectNode(fInetAddress) == NULL)
    {
        strErrorMessage = "Unable to connect to service node";

        return false;
    }

    return true;
}




bool CUtilityNode::ProcessMessage(CNode* pfrom, std::string strCommand, CDataStream& data)
{
    if (pfrom->nVersion < MIN_UTILITYNODE_PROTOVERSION) {
        return false;
    }

    if (REQ_UTILITYNODE_PROTOVERSION > 0)
    {
        if (pfrom->nVersion != REQ_UTILITYNODE_PROTOVERSION)
            return false;
    }

    // start service node
    if (strCommand == UTILITY_MESSAGE_STARTSERVICENODE)
    {
        CStartServiceNodeMessage message(data);

        return HandleMessage(pfrom, message);
    }

    if (strCommand == UTILITY_MESSAGE_STOPSERVICENODE)
    {
        CStopServiceNodeMessage message(data);

        return HandleMessage(pfrom, message);
    }

    if (strCommand == UTILITY_MESSAGE_PINGSERVICENODE)
    {
        CPingServiceNodeMessage message(data);

        return HandleMessage(pfrom, message);
    }

    if (strCommand == UTILITY_MESSAGE_GETSERVICENODEINFO)
    {
        CGetServiceNodeInfoMessage message(data);

        return HandleMessage(pfrom, message);
    }

    if (strCommand == UTILITY_MESSAGE_GETSERVICENODELIST)
    {
        CGetServiceNodeListMessage message;

        return HandleMessage(pfrom, message);
    }

    return false;
}

void CUtilityNode::UpdateLocks()
{
}

void CUtilityNode::LockOutPoint(COutPoint output)
{
    fLockedOutPoints.insert(output);
}

void CUtilityNode::UnlockOutPoint(COutPoint output)
{
    fLockedOutPoints.erase(output);
}

bool CUtilityNode::IsLockedOutPoint(COutPoint output) const
{
    return (fLockedOutPoints.count(output) > 0);
}

bool CUtilityNode::IsLockedOutPoint(uint256 hash, unsigned int n) const
{
    return IsLockedOutPoint(COutPoint(hash, n));
}

bool CUtilityNode::HasLockedOutPoint(CTransaction tx)
{
    BOOST_FOREACH(CTxIn txIn, tx.vin)
    {
        if (IsLockedOutPoint(txIn.prevout))
            return true;
    }

    return false;
}

std::vector<COutPoint> CUtilityNode::GetLockedOutPoints()
{
    std::vector<COutPoint> outs;

    std::set<COutPoint>::iterator it;

    for (it = fLockedOutPoints.begin(); it != fLockedOutPoints.end(); it++)
        outs.push_back((COutPoint) *it);

    return outs;
}

bool CUtilityNode::HasServiceNode(CServiceNodeInfo node)
{
    return HasServiceNode(node.GetTxIn());
}

bool CUtilityNode::HasServiceNode(CTxIn txIn)
{
    CServiceNodeInfo node;

    return GetServiceNode(txIn, node);
}

bool CUtilityNode::GetServiceNode(CTxIn txIn, CServiceNodeInfo& node)
{
    BOOST_FOREACH(CServiceNodeInfo nodeCompare, fServiceNodes)
    {
        if (nodeCompare.GetTxIn() == node.GetTxIn())
        {
            node = nodeCompare;

            return true;
        }
    }

    return false;
}

void CUtilityNode::UpdateServiceNodeList()
{
    if (IsInitialBlockDownload())
        return;

    // do we still need to sync the whole list?

    //CGetServiceNodeListMessage message;

    //RelayMessage(message);
}

void CUtilityNode::RelayMessage(CNodeMessage &message)
{
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes)
    {
        message.Relay(pnode);
    }
}

bool CUtilityNode::HandleMessage(CNode* pfrom, CGetServiceNodeInfoMessage& message)
{
    return true;
}

bool CUtilityNode::HandleMessage(CNode* pfrom, CGetServiceNodeListMessage& message)
{
    return true;
}

bool CUtilityNode::HandleMessage(CNode* pfrom, CPingServiceNodeMessage& message)
{
    return true;
}

bool CUtilityNode::HandleMessage(CNode* pfrom, CStartServiceNodeMessage& message)
{
    // check if the wallet is syncing
    if (IsInitialBlockDownload())
        return false;

    std::string strErrorMessage;

    if (!CheckPublicKey(message.GetWalletPublicKey(), strErrorMessage))
    {
        printf("StartServiceNodeMessage - CheckWalletPublicKey - %s\n", strErrorMessage.c_str());

        pfrom->Misbehaving(100);

        return false;
    }

    if (!CheckPublicKey(message.GetSharedPublicKey(), strErrorMessage))
    {
        printf("StartServiceNodeMessage - CheckSharedPublicKey - %s\n", strErrorMessage.c_str());

        pfrom->Misbehaving(100);

        return false;
    }

    if (!message.Verify(message.GetWalletPublicKey(), strErrorMessage))
    {
        printf("StartServiceNodeMessage - VerifyMessage - %s\n", strErrorMessage.c_str());

        pfrom->Misbehaving(100);

        return false;
    }

    if (!CheckServiceNodeInetAddressValid(message.GetInetAddress(), strErrorMessage))
    {
        printf("StartServiceNodeMessage - CheckServiceNodeInetAddressValid - %s\n", strErrorMessage.c_str());

        pfrom->Misbehaving(0);

        return false;
    }

    if (!CheckTxInAssociation(message.GetTxIn(), message.GetWalletPublicKey(), strErrorMessage))
    {
        printf("StartServiceNodeMessage - CheckVinAssociationWithPublicKey - %s\n", strErrorMessage.c_str());

        pfrom->Misbehaving(100);

        return false;
    }

    if (!CheckTxInUnspent(message.GetTxIn(), strErrorMessage))
    {
        printf("StartServiceNodeMessage - CheckTxInUnspent - %s\n", strErrorMessage.c_str());

        // not necessarily misbehaving as the input may have just been spent
        // TODO needs work!
        pfrom->Misbehaving(10);

        return false;
    }

    CServiceNodeInfo node;

    if (GetServiceNode(message.GetTxIn(), node))
    {
        // existing service node entry
        if (message.GetCount() == -1 && !node.IsUpdatedWithin(SERVICENODE_SECONDS_BETWEEN_UPDATES))
        {
            node.SetLastSeen(message.GetTime());

            // check newer entry
            if (node.GetLastSignature() < message.GetTime())
            {
                printf("StartServiceNodeMessage - Updated - %s\n", message.GetInetAddress().ToString().c_str());

                node.SetLastSignature(message.GetTime());
                node.SetWalletPublicKey(message.GetWalletPublicKey());
                node.SetSharedPublicKey(message.GetSharedPublicKey());
                node.SetInetAddress(message.GetInetAddress());

                // are you talking about me?
                if (IsServiceNode(this) && ((CServiceNode*)this)->GetTxIn() == message.GetTxIn())
                    ((CServiceNode*)this)->Enable(message.GetTxIn(), message.GetTime());

                RelayMessage(message);
            }
        }
    }
    else
    {
        // new service node entry
        if (!CheckTxInConfirmations(message.GetTxIn(), SERVICENODE_MIN_CONFIRMATIONS, strErrorMessage))
        {
            printf("StartServiceNodeMessage - CheckTxConfirmations - %s\n", strErrorMessage.c_str());

            pfrom->Misbehaving(20);

            return false;
        }

        // add the node to our list of service nodes
        CServiceNodeInfo node;

        node.SetInetAddress(message.GetInetAddress());
        node.SetLastSignature(message.GetTime());
        node.SetWalletPublicKey(message.GetWalletPublicKey());
        node.SetSharedPublicKey(message.GetSharedPublicKey());
        node.SetInetAddress(message.GetInetAddress());
        node.SetTxIn(message.GetTxIn());
        node.SetLastSeen(message.GetTime());

        fServiceNodes.push_back(node);

        // add the node to the address manager
        addrman.Add(node.GetInetAddress(), pfrom->addr, SERVICENODE_TIME_PENALTY);

        // is this me?
        if (IsServiceNode(this) && ((CServiceNode*)this)->GetTxIn() == message.GetTxIn())
            ((CServiceNode*)this)->Enable(message.GetTxIn(), message.GetTime());

        if (message.GetCount() == -1)
            RelayMessage(message);
    }

    return true;
}

bool CUtilityNode::HandleMessage(CNode* pfrom, CStopServiceNodeMessage& message)
{
    return true;
}

void ThreadUtilityNodeTimers()
{
    RenameThread("utility-timers");

    unsigned int s = 0;
    while (true)
    {
        MilliSleep(1000);

        // every 20 seconds
        if (s % 20)
        {
            pNodeMain->UpdateServiceNodeList();
        }

        s++;
    }
}
