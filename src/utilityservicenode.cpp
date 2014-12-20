#include "utilityservicenode.h"

bool CServiceNode::ProcessMessage(CNode* pfrom, std::string strCommand, CDataStream& data)
{

    if (pfrom->nVersion < MIN_SERVICENODE_PROTOVERSION) {
        return false;
    }

    if (REQ_SERVICENODE_PROTOVERSION > 0)
    {
        if (pfrom->nVersion != REQ_SERVICENODE_PROTOVERSION)
            return false;
    }

    return CUtilityNode::ProcessMessage(pfrom, strCommand, data);
}

bool CServiceNode::Enable(CTxIn txIn, int64_t time)
{
    fTxIn = txIn;
    fLastSignature = time;
}
