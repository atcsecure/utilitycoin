#include "utilitynodemessage.h"

#include "utilitynode.h"
#include "init.h"

#include <boost/lexical_cast.hpp>

const char UTILITY_MESSAGE_GETSERVICENODEINFO[]   = "sninfo";
const char UTILITY_MESSAGE_GETSERVICENODELIST[]   = "snlist";
const char UTILITY_MESSAGE_PINGSERVICENODE[]      = "snping";
const char UTILITY_MESSAGE_STARTSERVICENODE[]     = "snstrt";
const char UTILITY_MESSAGE_STOPSERVICENODE[]      = "snstop";

bool SignMessage(CBitcoinAddress address, std::string strMessage, std::vector<unsigned char>& vSignature, std::string& strErrorMessage)
{
    if(!CheckWalletLocked(strErrorMessage))
        return false;

    CKey key;

    if (!GetPrivateKey(address, key, strErrorMessage))
        return false;

    return SignMessage(key, strMessage, vSignature, strErrorMessage);
}

bool SignMessage(CKey key, std::string strMessage, std::vector<unsigned char>& vSignature, std::string& strErrorMessage)
{
    CHashWriter ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << strMessage;

    if (!key.SignCompact(ss.GetHash(), vSignature)) {
        strErrorMessage = "Sign failed";
        return false;
    }

    return true;
}

bool VerifyMessage(CBitcoinAddress address, std::string strMessage, std::vector<unsigned char> vSignature, std::string &strErrorMessage)
{
    CKeyID keyID;
    if (!address.GetKeyID(keyID))
    {
        strErrorMessage = "Wallet address does not refer to key";

        return false;
    }

    return VerifyMessage(keyID, strMessage, vSignature, strErrorMessage);

}

bool VerifyMessage(CKeyID keyID, std::string strMessage, std::vector<unsigned char> vSignature, std::string& strErrorMessage)
{
    CDataStream ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << strMessage;

    CKey key;
    if (!key.SetCompactSignature(Hash(ss.begin(), ss.end()), vSignature))
    {
        strErrorMessage = "Signature invalid";

        return false;
    }

    if (key.GetPubKey().GetID() != keyID)
    {
        strErrorMessage = "Key mismatch";

        return false;
    }

    return true;
}





bool CSignedNodeMessage::Sign(CBitcoinAddress address, std::string strErrorMessage)
{
    return SignMessage(address, GetMessageString(), fSignature, strErrorMessage);
}

bool CSignedNodeMessage::Verify(CPubKey pubKey, std::string strErrorMessage)
{
    return VerifyMessage(pubKey.GetID(), GetMessageString(), fSignature, strErrorMessage);
}






CGetServiceNodeInfoMessage::CGetServiceNodeInfoMessage(CDataStream &data)
{
    data >> fTxIn;
}

bool CGetServiceNodeInfoMessage::IsValid()
{
    return true;
}

void CGetServiceNodeInfoMessage::Relay(CNode *destination)
{
    destination->PushMessage
        (
            UTILITY_MESSAGE_GETSERVICENODEINFO,
            fTxIn
        );
}






bool CGetServiceNodeListMessage::IsValid()
{
    return true;
}

void CGetServiceNodeListMessage::Relay(CNode *destination)
{
    destination->PushMessage
        (
            UTILITY_MESSAGE_GETSERVICENODELIST
        );
}






CPingServiceNodeMessage::CPingServiceNodeMessage(CDataStream& data)
{
}

bool CPingServiceNodeMessage::IsValid()
{
    if (!fInetAddress.IsValid())
        return false;

    return true;
}

void CPingServiceNodeMessage::Relay(CNode *destination)
{
    destination->PushMessage
        (
            UTILITY_MESSAGE_PINGSERVICENODE,
            fTxIn,
            fSignature,
            fTime
        );
}

std::string CPingServiceNodeMessage::GetMessageString()
{
    return
        UTILITY_MESSAGE_PINGSERVICENODE +
        fInetAddress.ToString() +
        boost::lexical_cast<std::string>(fTime);
}






CStartServiceNodeMessage::CStartServiceNodeMessage(CDataStream& data)
{
    data >> fTxIn >> fInetAddress >> fSignature >> fTime >> fWalletPublicKey >> fSharedPublicKey >> fCount >> fCurrent;
}


bool CStartServiceNodeMessage::IsValid()
{
    if (!fInetAddress.IsValid())
        return false;

    if (!fWalletPublicKey.IsValid())
        return false;

    if (!fSharedPublicKey.IsValid())
        return false;

    return true;
}


void CStartServiceNodeMessage::Relay(CNode *destination)
{
    destination->PushMessage
        (
            UTILITY_MESSAGE_STARTSERVICENODE,
            fTxIn,
            fInetAddress,
            fSignature,
            fTime,
            fWalletPublicKey,
            fSharedPublicKey,
            fCount,
            fCurrent
        );
}

std::string CStartServiceNodeMessage::GetMessageString()
{
    return
        UTILITY_MESSAGE_STARTSERVICENODE +
        fInetAddress.ToString() +
        boost::lexical_cast<std::string>(fTime) +
        fWalletPublicKey.ToString() +
        fSharedPublicKey.ToString();
}





CStopServiceNodeMessage::CStopServiceNodeMessage(CDataStream& data)
{
    data >> fTxIn >> fInetAddress >> fSignature >> fTime;
}

bool CStopServiceNodeMessage::IsValid()
{
    if (!fInetAddress.IsValid())
        return false;

    return true;
}

void CStopServiceNodeMessage::Relay(CNode *destination)
{
    destination->PushMessage
        (
            UTILITY_MESSAGE_STOPSERVICENODE,
            fTxIn,
            fInetAddress,
            fSignature,
            fTime
        );
}

std::string CStopServiceNodeMessage::GetMessageString()
{
    return
        UTILITY_MESSAGE_STOPSERVICENODE +
        fInetAddress.ToString() +
        boost::lexical_cast<std::string>(fTime);
}
