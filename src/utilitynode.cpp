#include "utilitynode.h"
#include "utilityservicenode.h"
#include "utilitycontrolnode.h"

#include "txdb-leveldb.h"
#include "init.h"
#include "main.h"

#include <boost/make_shared.hpp>
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

bool IsSlaveNodeInfo(CServiceNodeInfo* info)
{
    return (typeid(*info) == typeid(CSlaveNodeInfo));
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
bool CheckInitialBlockDownloadFinished(std::string& strErrorMessage)
{
    if (IsInitialBlockDownload())
    {
        strErrorMessage = "Initial block download in progress";

        return false;
    }

    return true;
}

bool CheckPublicKey(CPubKey key, std::string& strErrorMessage)
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

bool CheckSignatureTime(int64_t time, std::string& strErrorMessage)
{
    if (time > GetAdjustedTime() + 60 * 60)
    {
        strErrorMessage = "Signature too far into the future";

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
        strErrorMessage = "Tx is used in memory";

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

    // Check against previous transactions
    // This is done last to help prevent CPU exhaustion denial-of-service attacks.
    if (!tx.ConnectInputs(txdb, mapInputs, mapUnused, CDiskTxPos(1,1,1), pindexBest, true, true, true))
    {
        strErrorMessage = "Unable to connect tx inputs";

        return false;
    }

    return true;
}

bool CheckTxInConfirmations(CTxIn txIn, int confirmations, std::string &strErrorMessage)
{
    CTransaction tx;
    uint256 hash;

    if (!GetTransaction(txIn.prevout.hash, tx, hash))
    {
        strErrorMessage = "Transaction not found";

        return false;
    }

    return true;
}

bool GetBitcoinAddress(CPubKey pubKey, CBitcoinAddress &address, std::string &strErrorMessage)
{
    if (!pubKey.IsValid())
    {
        strErrorMessage = "Invalid public key";

        return false;
    }

    CScript script;

    script.SetDestination(pubKey.GetID());

    CTxDestination destination;

    if (!ExtractDestination(script, destination))
    {
        strErrorMessage = "Unabled to extract destination";

        return false;
    }

    address.Set(destination);

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

std::string GetReadableTimeSpan(int64_t time)
{
    std::string result;

    if (time > 86400)
        result += (boost::format("%1%d") % ((time / 86400) % 7)).str();

    if (time > 3600)
        result += (boost::format("%1%h") % ((time / 3600) % 24)).str();

    if (time > 60)
        result += (boost::format("%1%m") % ((time / 60) % 60)).str();

    result += (boost::format("%1%s") % (time % 60)).str();

    return result;
}

std::string GetServiceNodeStateString(ServiceNodeState state)
{
    switch (state)
    {
        case kStarted: return "Started"; break;
        case kStopped: return "Stopped"; break;
        case kProcessingStop:
        case kProcessingStart:
            return "Processing"; break;
    }

    return "Undefined";
}








CServiceNodeInfo::CServiceNodeInfo()
{
    fSignatureTime = 0;
    fLastPing = 0;
    fLastSeen = 0;
    fLastStart = 0;
    fLastStop = 0;
}

bool CServiceNodeInfo::Init(std::string strInetAddress, std::string& strErrorMessage)
{
    fInetAddress = CAddress(CService(strInetAddress));

    if (!fInetAddress.IsValid())
    {
        strErrorMessage = "Invalid ip address";

        return false;
    }

    return true;
}

bool CServiceNodeInfo::IsValid()
{
    return fInetAddress.IsValid();
}

bool CServiceNodeInfo::IsRemove()
{
    if (!IsStopped())
        return false;

    if (GetAdjustedTime() - GetTimeStopped() < SERVICENODE_SECONDS_BETWEEN_REMOVAL)
        return false;

    return true;
}

bool CServiceNodeInfo::Check(std::string &strErrorMessage)
{
    if (!CheckInetAddressValid(strErrorMessage))
        return false;

    return true;
}

void CServiceNodeInfo::UpdateState()
{
    if (fState == kStarted)
    {
        if (!IsUpdatedWithin(SERVICENODE_SECONDS_BETWEEN_EXPIRATION))
        {
            fState = kStopped;

            return;
        }

        std::string strErrorMessage;

        if (!CheckTxInUnspent(GetTxIn(), strErrorMessage))
        {
            fState = kStopped;

            return;
        }
    }
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

bool CServiceNodeInfo::GetNodeMessage(CNodeMessage &message, std::string &strErrorMessage)
{
    message.SetTime(GetAdjustedTime());

    return true;
}

bool CServiceNodeInfo::GetStartMessage(CStartServiceNodeMessage& message, int serviceNodeCount, int serviceNodeIndex, std::string& strErrorMessage)
{
    message.SetTime(GetSignatureTime());
    message.SetInetAddress(fInetAddress);
    message.SetTxIn(fTxIn);
    message.SetWalletPublicKey(fWalletPublicKey);
    message.SetSharedPublicKey(fSharedPublicKey);
    message.SetServiceNodeCount(serviceNodeCount);
    message.SetServiceNodeIndex(serviceNodeIndex);
    message.SetSignature(fSignature);

    return true;
}

std::string CServiceNodeInfo::ToString(bool extensive)
{
    std::string seperator = "; ";

    std::string result;
    std::string message;

    result += GetInetAddress().ToString();
    result += seperator;
    result += GetStateString();


    if (extensive)
    {
        result += "\n";

        CBitcoinAddress address;

        if (GetBitcoinAddress(GetWalletPublicKey(), address, message))
            result += (boost::format("Address   : %1%") % address.ToString()).str();
        else
            result += "Address   : ?";
        result += "\n";
        result += (boost::format("Active    : %1%") % (GetLastSeen() > 0 ? GetReadableTimeSpan(GetLastSeen() - GetSignatureTime()) : "?")).str();
        result += "\n";
        result += (boost::format("Last seen : %1%") % (GetLastSeen() > 0 ? GetReadableTimeSpan(GetAdjustedTime() - GetLastSeen()) : "?")).str();
        //result += GetTxIn().prevout.hash.ToString();
        //result += seperator;
        // TODO rank
    }

    return result;
}









std::string CUtilityNode::Test()
{
    return "finished";
}

bool CUtilityNode::ProcessMessage(CNode* pfrom, std::string strCommand, CDataStream& data)
{
    if (pfrom->nVersion < UTILITYNODE_MIN_PROTOVERSION)
    {
        return false;
    }

    if (UTILITYNODE_REQ_PROTOVERSION > 0)
    {
        if (pfrom->nVersion !=UTILITYNODE_REQ_PROTOVERSION)
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

bool CUtilityNode::HasServiceNode(CTxIn txIn)
{
    return GetServiceNode(txIn) != NULL;
}

CServiceNodeInfo* CUtilityNode::GetServiceNode(CTxIn txIn)
{
    BOOST_FOREACH(boost::shared_ptr<CServiceNodeInfo> info, fServiceNodes)
    {
        if (info->GetTxIn() == txIn)
            return info.get();
    }

    return NULL;
}

CServiceNodeInfo* CUtilityNode::GetServiceNode(CStartServiceNodeMessage& message)
{
    BOOST_FOREACH(boost::shared_ptr<CServiceNodeInfo> info, fServiceNodes)
    {
        if (info->GetTxIn() == message.GetTxIn())
            return info.get();

        if (info->GetInetAddress() == message.GetInetAddress())
            return info.get();

        if (info->GetSharedPublicKey() == message.GetSharedPublicKey())
            return info.get();

        if (info->GetWalletPublicKey() == message.GetWalletPublicKey())
            return info.get();
    }

    return NULL;
}

int CUtilityNode::GetServiceNodeIndex(CServiceNodeInfo* node)
{
    for (int i = 0; i < fServiceNodes.size(); i++)
    {
        if (fServiceNodes[i]->GetTxIn() == node->GetTxIn())
            return i;
    }

    return -1;
}

bool CUtilityNode::DelServiceNode(CTxIn txIn, std::string &strErrorMessage)
{
    std::vector< boost::shared_ptr<CServiceNodeInfo> >::iterator it;

    for (it = fServiceNodes.begin(); it != fServiceNodes.end(); it++)
    {
        if ((*it)->GetTxIn() == txIn)
        {
            fServiceNodes.erase(it);

            return true;
        }
    }

    strErrorMessage = (boost::format("Input %1% is not associated to a service node") % txIn.ToString()).str();

    return false;
}

bool CUtilityNode::StartServiceNode(CStartServiceNodeMessage& message, std::string& strErrorMessage)
{
    printf("CUtilityNode::RegisterServiceNode: %s\n", message.GetInetAddress().ToString().c_str());

    CServiceNodeInfo* node = GetServiceNode(message);

    if (node == NULL)
    {
        boost::shared_ptr<CServiceNodeInfo> ptr = boost::make_shared<CServiceNodeInfo>();

        fServiceNodes.push_back(ptr);

        node = ptr.get();

        node->SetTxIn(message.GetTxIn());
    }
    else
    {
        if (message.GetTime() < node->GetLastStart())
        {
            strErrorMessage = "newer start message already received";

            return false;
        }

        if (message.GetTime() < node->GetLastStop())
        {
            strErrorMessage = "newer stop message already received";

            return false;
        }
    }

    node->SetLastStart(message.GetTime());
    node->SetInetAddress(message.GetInetAddress());
    node->SetSignature(message.GetSignature());
    node->SetSignatureTime(message.GetTime());
    node->SetWalletPublicKey(message.GetWalletPublicKey());
    node->SetSharedPublicKey(message.GetSharedPublicKey());
    node->SetInetAddress(message.GetInetAddress());
    node->SetLastSeen(GetAdjustedTime());
    node->SetTxIn(message.GetTxIn());
    node->SetState(kStarted);

    return true;
}

bool CUtilityNode::StopServiceNode(CStopServiceNodeMessage& message, std::string& strErrorMessage)
{
    printf("CUtilityNode::UnregisterServiceNode: %s\n", message.GetInetAddress().ToString().c_str());

    CServiceNodeInfo* node = GetServiceNode(message.GetTxIn());

    if (node != NULL)
    {

        // check newer entries
        if (message.GetTime() < node->GetLastStop())
        {
            strErrorMessage = "newer stop message already received";

            return false;
        }

        if (message.GetTime() < node->GetLastStart())
        {
            strErrorMessage = "newer start message already received";

            return false;
        }

        node->SetState(kStopped);
        node->SetTimeStopped(GetAdjustedTime());
    }
    return true;
}

void CUtilityNode::SyncServiceNodeList()
{
    if (IsInitialBlockDownload())
        return;

    if (vNodes.size() == 0)
        return;

    if (GetAdjustedTime() - fLastSyncServiceNodeList < 60 * 5)
        return;

    if (fNumSyncServiceNodeListRequests > 3)
        return;

    fLastSyncServiceNodeList = GetAdjustedTime();
    fNumSyncServiceNodeListRequests++;

    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes)
    {
        CGetServiceNodeListMessage message;

        if (!HasRequestRecord(message, pnode->addr.ToString()))
        {
            printf("requesting service node list %s\n", pnode->addr.ToString().c_str());

            fRequests.push_back(CNodeMessageRecord(pnode->addr.ToString(), boost::make_shared<CGetServiceNodeListMessage>(message), GetAdjustedTime()));

            message.Relay(pnode);
        }
    }
}

void CUtilityNode::UpdateServiceNodeList()
{
    std::vector< boost::shared_ptr<CServiceNodeInfo> >::iterator it = fServiceNodes.begin();

    while(it != fServiceNodes.end())
    {
        boost::shared_ptr<CServiceNodeInfo> info = (*it);

        info->UpdateState();

        if (info->IsRemove())
        {
            printf("UpdateServiceNodeList: Inactive service node removed %s\n", info->GetInetAddress().ToString().c_str());

            it = fServiceNodes.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

bool CUtilityNode::IsRemoveRecord(CNodeMessageRecord& record)
{
    CNodeMessage* message = record.GetNodeMessage();

    // default time span of one hour
    int span = 60 * 60;

    if (IsGetServiceNodeInfoMessage(message))
        span = UTILITYNODE_SECONDS_BETWEEN_GETSERVICENODEINFO;

    if (IsGetServiceNodeListMessage(message))
        span = UTILITYNODE_SECONDS_BETWEEN_GETSERVICENODELIST;

    if (record.GetTime() < GetAdjustedTime() - span)
        return true;

    return false;
}


void CUtilityNode::CleanNodeMessageRecords()
{
    std::vector<CNodeMessageRecord>::iterator it;

    it = fRequests.begin();

    while (it != fRequests.end())
    {
        CNodeMessageRecord record = (*it);

        if (IsRemoveRecord(record))

            it = fRequests.erase(it);
        else
            it++;
    }

    it = fResponses.begin();

    while (it != fResponses.end())
    {
        CNodeMessageRecord record = (*it);

        if (IsRemoveRecord(record))

            it = fResponses.erase(it);
        else
            it++;
    }
}

void CUtilityNode::RelayMessage(CNodeMessage &message)
{
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes)
    {
        message.Relay(pnode);
    }
}

bool HasRecord(std::vector<CNodeMessageRecord> vrecords, CNodeMessage& message, std::string address)
{
    BOOST_FOREACH(CNodeMessageRecord record, vrecords)
    {
        if (address.size() > 0 && address != record.GetNodeAddress())
            continue;

        CNodeMessage* messageCompare = record.GetNodeMessage();

        if (messageCompare->Compare(message))
            return true;
    }

    return false;
}

bool CUtilityNode::HasRequestRecord(CNodeMessage& message, std::string address)
{
    return HasRecord(fRequests, message, address);
}

bool CUtilityNode::HasResponseRecord(CNodeMessage& message, std::string address)
{
    return HasRecord(fResponses, message, address);
}


bool CUtilityNode::AcceptStartMessage(CServiceNodeInfo* node, CStartServiceNodeMessage& message)
{
     if (message.GetServiceNodeCount() == -1)
         return false;

     if (!node->IsUpdatedWithin(SERVICENODE_SECONDS_BETWEEN_UPDATES))
         return false;

     return true;
}

bool CUtilityNode::HandleMessage(CNode* pfrom, CGetServiceNodeInfoMessage& message)
{
    // check if the wallet is syncing
    if (IsInitialBlockDownload())
        return false;

    std::string strErrorMessage;

    // TODO handle node being restarted
    if (HasResponseRecord(message, pfrom->addr.ToString()))
    {
        // should only ask for the list once
        printf("CGetServiceNodeInfoMessage - node %s already asked for the list\n", pfrom->addr.ToString().c_str());

        // TODO bit soft for now as the node might be restarting
        pfrom->Misbehaving(5);
    }

    // record response
    fResponses.push_back(CNodeMessageRecord(pfrom->addr.ToString(), boost::make_shared<CGetServiceNodeInfoMessage>(message), GetAdjustedTime()));

    printf("CGetServiceNodeInfoMessage - sending servicenode entry %s\n", message.GetTxIn().ToString().c_str());

    int count = fServiceNodes.size();

    CServiceNodeInfo* node = GetServiceNode(message.GetTxIn());

    if (node != NULL)
    {
        int index = GetServiceNodeIndex(node);

        node->UpdateState();

        if (node->IsStarted())
        {
            CStartServiceNodeMessage startMessage;

            if (!node->GetStartMessage(startMessage, count, index, strErrorMessage))
            {
                printf("GetServiceNodeListMessage - Unable to get start message, %s\n", strErrorMessage.c_str());

                return false;
            }

            startMessage.Relay(pfrom);
        }
    }

    return true;
}

bool CUtilityNode::HandleMessage(CNode* pfrom, CGetServiceNodeListMessage& message)
{
    // check if the wallet is syncing
    if (IsInitialBlockDownload())
        return false;

    std::string strErrorMessage;

    // TODO handle node being restarted
    if (HasResponseRecord(message, pfrom->addr.ToString()))
    {
        // should only ask for the list once
        printf("GetServiceNodeListMessage - node %s already asked for the list\n", pfrom->addr.ToString().c_str());

        // TODO bit soft for now as the node might be restarting
        pfrom->Misbehaving(5);
    }

    // record response
    fResponses.push_back(CNodeMessageRecord(pfrom->addr.ToString(), boost::make_shared<CGetServiceNodeListMessage>(message), GetAdjustedTime()));

    printf("GetServiceNodeListMessage - sending servicenode entries\n");

    int count = fServiceNodes.size();
    int index = 0;

    BOOST_FOREACH(boost::shared_ptr<CServiceNodeInfo> node, fServiceNodes)
    {
        if (node->IsStarted())
        {
            CStartServiceNodeMessage startMessage;

            if (!node->GetStartMessage(startMessage, count, index, strErrorMessage))
            {
                printf("GetServiceNodeListMessage - Unable to get start message, %s\n", strErrorMessage.c_str());

                continue;
            }

            startMessage.Relay(pfrom);
        }

        index++;
    }

    return true;
}

bool CUtilityNode::HandleMessage(CNode* pfrom, CPingServiceNodeMessage& message)
{
    // check if the wallet is syncing
    if (IsInitialBlockDownload())
        return false;

    std::string strErrorMessage;

    if (!CheckSignatureTime(message.GetTime(), strErrorMessage))
    {
        printf("PingServiceNodeMessage - CheckSignatureTime - %s\n", strErrorMessage.c_str());

        return false;
    }

    if (!CheckPublicKey(message.GetSharedPublicKey(), strErrorMessage))
    {
        printf("PingServiceNodeMessage - CheckWalletPublicKey - %s\n", strErrorMessage.c_str());

        pfrom->Misbehaving(100);

        return false;
    }

    if (!message.Verify(message.GetSharedPublicKey(), strErrorMessage))
    {
        printf("PingServiceNodeMessage - VerifyMessage - %s\n", strErrorMessage.c_str());

        pfrom->Misbehaving(100);

        return false;
    }

    CServiceNodeInfo* node = GetServiceNode(message.GetTxIn());

    if (node != NULL)
    {
        if (message.GetTime() < node->GetLastPing())
        {
            printf("PingServiceNodeMessage - message older then earlier ping\n");

            return false;
        }

        if (node->IsProcessing())
            node->SetState(kStarted);

        node->SetLastPing(message.GetTime());

        if (!node->IsUpdatedWithin(SERVICENODE_SECONDS_BETWEEN_UPDATES))
        {
            node->SetLastSeen(GetAdjustedTime());

            RelayMessage(message);
        }
    }
    else
    {
        printf("PingServiceNode - Service node not in our list, requesting info\n");

        // not in the list, request start message if not recently requested
        std::string address = pfrom->addr.ToString();

        CGetServiceNodeInfoMessage requestMessage;

        requestMessage.SetTime(GetAdjustedTime());
        requestMessage.SetTxIn(message.GetTxIn());

        if (HasRequestRecord(requestMessage, address))
        {
            printf("PingServiceNode - Service node info already requested\n");

            return false;
        }

        fRequests.push_back(CNodeMessageRecord(address, boost::make_shared<CGetServiceNodeInfoMessage>(requestMessage), GetAdjustedTime()));

        requestMessage.Relay(pfrom);
    }

    return true;
}

bool CUtilityNode::HandleMessage(CNode* pfrom, CStartServiceNodeMessage& message)
{
    // check if the wallet is syncing
    if (IsInitialBlockDownload())
        return false;

    std::string strErrorMessage;

    if (!CheckSignatureTime(message.GetTime(), strErrorMessage))
    {
        printf("StopServiceNodeMessage - CheckSignatureTime - %s\n", strErrorMessage.c_str());

        return false;
    }

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
        printf("StartServiceNodeMessage - Verify Message - %s\n", strErrorMessage.c_str());

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
        // TODO needs DOS
        pfrom->Misbehaving(10);

        return false;
    }

    CServiceNodeInfo* node = GetServiceNode(message);

    // don't check tx in if its already in the list
    if (node == NULL || node->GetTxIn() != message.GetTxIn())
    {
        // new service node entry
        if (!CheckTxInConfirmations(message.GetTxIn(), SERVICENODE_MIN_CONFIRMATIONS, strErrorMessage))
        {
            printf("StartServiceNodeMessage - CheckTxConfirmations - %s\n", strErrorMessage.c_str());

            pfrom->Misbehaving(20);

            return false;
        }
    }

    if (node != NULL)
    {        
        // existing service node entry        
        if (AcceptStartMessage(node, message))
        {
            node->SetLastSeen(GetAdjustedTime());

            if (!StartServiceNode(message, strErrorMessage))
            {
                printf("StartServiceNodeMessage - UpdateServiceNode - %s\n", strErrorMessage.c_str());

                return false;
            }

            RelayMessage(message);
        }
    }
    else
    {
        if (!StartServiceNode(message, strErrorMessage))
        {
            printf("StartServiceNodeMessage - RegisterServiceNode - %s\n", strErrorMessage.c_str());

            return false;
        }

        // add the node to the address manager
        addrman.Add(message.GetInetAddress(), pfrom->addr, SERVICENODE_TIME_PENALTY);

        if (message.GetServiceNodeCount() == -1)
            RelayMessage(message);
    }

    // handled
    return true;
}

bool CUtilityNode::HandleMessage(CNode* pfrom, CStopServiceNodeMessage& message)
{
    // check if the wallet is syncing
    if (IsInitialBlockDownload())
        return false;

    std::string strErrorMessage;

    if (!CheckSignatureTime(message.GetTime(), strErrorMessage))
    {
        printf("StopServiceNodeMessage - CheckSignatureTime - %s\n", strErrorMessage.c_str());

        return false;
    }

    if (!CheckPublicKey(message.GetWalletPublicKey(), strErrorMessage))
    {
        printf("StopServiceNodeMessage - CheckWalletPublicKey - %s\n", strErrorMessage.c_str());

        pfrom->Misbehaving(100);

        return false;
    }

    if (!CheckPublicKey(message.GetSharedPublicKey(), strErrorMessage))
    {
        printf("StopServiceNodeMessage - CheckSharedPublicKey - %s\n", strErrorMessage.c_str());

        pfrom->Misbehaving(100);

        return false;
    }

    if (!message.Verify(message.GetWalletPublicKey(), strErrorMessage))
    {
        printf("StopServiceNodeMessage - VerifyMessage - %s\n", strErrorMessage.c_str());

        pfrom->Misbehaving(100);

        return false;
    }

    if (!CheckServiceNodeInetAddressValid(message.GetInetAddress(), strErrorMessage))
    {
        printf("StopServiceNodeMessage - CheckServiceNodeInetAddressValid - %s\n", strErrorMessage.c_str());

        pfrom->Misbehaving(0);

        return false;
    }

    if (!CheckTxInAssociation(message.GetTxIn(), message.GetWalletPublicKey(), strErrorMessage))
    {
        printf("StopServiceNodeMessage - CheckVinAssociationWithPublicKey - %s\n", strErrorMessage.c_str());

        pfrom->Misbehaving(100);

        return false;
    }

    if (!CheckTxInUnspent(message.GetTxIn(), strErrorMessage))
    {
        printf("StopServiceNodeMessage - CheckTxInUnspent - %s\n", strErrorMessage.c_str());

        // not necessarily misbehaving as the input may have just been spent
        // TODO needs DOS
        pfrom->Misbehaving(10);

        return false;
    }

    if (!StopServiceNode(message, strErrorMessage))
    {
        printf("StopServiceNodeMessage - UnregisterServiceNode - %s\n", strErrorMessage.c_str());

        return false;
    }

    RelayMessage(message);

    // handled
    return true;
}

void ThreadUtilityNodeTimers(void* parg)
{
    RenameThread("utility-timers");

    std::string strErrorMessage;

    unsigned int s = 0;
    while (true)
    {
        MilliSleep(1000);

        // every 20 seconds
        if (s % 20 == 0)
        {
            pNodeMain->SyncServiceNodeList();
        }

        // every minute
        if (s % 60 == 0)
        {
            pNodeMain->UpdateServiceNodeList();
        }

        // every 5 minutes
        if (s % (5 * 60) == 0)
        {
            if (IsServiceNode(pNodeMain))
            {
                if (!((CServiceNode*)pNodeMain)->Ping(strErrorMessage))
                    printf("ThreadUtilityNodeTimers: ServiceNode Ping: %s\n", strErrorMessage.c_str());
            }
        }

        s++;
    }
}
