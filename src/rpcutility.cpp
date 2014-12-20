//// Copyright (c) 2010 Satoshi Nakamoto
//// Copyright (c) 2009-2012 The Bitcoin developers
//// Distributed under the MIT/X11 software license, see the accompanying
//// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "main.h"
#include "init.h"
#include "bitcoinrpc.h"
#include "utilitycontrolnode.h"
#include <boost/format.hpp>

json_spirit::Value generatesharedkey(const json_spirit::Array& params, bool fHelp)
{
    if (IsControlNode(pNodeMain))
    {
        if ( fHelp || params.size() != 0)
            throw std::runtime_error(
                    "generatesharedkey\n"
                    "Generates a key to be shared between the control node and service node");

        return ((CControlNode*)pNodeMain)->GenerateSharedKey();
    }
    else
    {
        if (fHelp)
            throw std::runtime_error("");

        throw std::runtime_error("generatesharedkey is only available to control nodes");
    }
}

std::string startservicenode(CControlNode* node, std::string alias)
{
    if (!node->HasSlave(alias))
        return (boost::format("Service node %1% not found\n") % alias).str();

    std::string strErrorMessage ;


    if (node->StartSlave(alias, strErrorMessage))
        return (boost::format("Service node %1% succesfully started\n") % alias).str();
    else
        return (boost::format("Service node %1% failed to start - %2%\n") % alias % strErrorMessage).str();
}

std::string stopservicenode(CControlNode* node, std::string alias)
{
    if (!node->HasSlave(alias))
        return (boost::format("Service node %1% not found\n") % alias).str();

    std::string strErrorMessage;

    if (node->StopSlave(alias, strErrorMessage))
        return (boost::format("Service node %1% succesfully stopped") % alias).str();
    else
        return (boost::format("Service node %1% failed to stop - %2%\n") % alias % strErrorMessage).str();
}

json_spirit::Value startservicenodes(const json_spirit::Array& params, bool fHelp)
{
    if (IsControlNode(pNodeMain))
    {
        if (fHelp)
            throw std::runtime_error(
                    "startservicenodes [alias1] [alias2] .. [aliasN] \n"
                    "Start remote service node(s)");

        // check if the wallet is syncing
        if (IsInitialBlockDownload())
            return "Initial block download in progress";

        if(pwalletMain->IsLocked())
            return "Wallet needs to be unlocked";

        CControlNode* node = ((CControlNode*)pNodeMain);

        std::string message = "";

        if (params.size() == 0)
        {
            std::vector<std::string> vAliases = node->GetSlaveAliases();

            if (vAliases.size() == 0)
                message += "no registered slave service nodes";
            else
                BOOST_FOREACH(std::string alias, vAliases)
                    message += startservicenode(node, alias);
        }
        else
        {
            BOOST_FOREACH(json_spirit::Value param, params)
                message += startservicenode(node, param.get_str());
        }

        return message;
    }
    else
    {
        if (fHelp)
            throw std::runtime_error("");

        throw std::runtime_error("generatesharedkey is only available to control nodes");
    }
}

json_spirit::Value stopservicenodes(const json_spirit::Array& params, bool fHelp)
{
    if (IsControlNode(pNodeMain))
    {
        if (fHelp)
            throw std::runtime_error(
                    "stopservicenodes [alias1] [alias2] .. [aliasN] \n"
                    "Stops remote service node(s)");

        // check if the wallet is syncing
        if (IsInitialBlockDownload())
            return "Initial block download in progress";

        if(pwalletMain->IsLocked())
            return "Wallet needs to be unlocked";

        CControlNode* node = ((CControlNode*)pNodeMain);

        std::string message = "";

        if (params.size() == 0)
        {
            std::vector<std::string> vAliases = node->GetSlaveAliases();

            if (vAliases.size() == 0)
                message += "no registered slave service nodes";
            else
                BOOST_FOREACH(std::string alias, vAliases)
                    message += stopservicenode(node, alias);
        }
        else
        {
            BOOST_FOREACH(json_spirit::Value param, params)
                message += stopservicenode(node, param.get_str());
        }

        return message;
    }
    else
    {
        if (fHelp)
            throw std::runtime_error("");

        throw std::runtime_error("generatesharedkey is only available to control nodes");
    }
}


