#ifndef CLIENT_MESSAGEHANDLERS_METAUPDATENOTIFICATIONMESSAGEHANDLER_HPP_
#define CLIENT_MESSAGEHANDLERS_METAUPDATENOTIFICATIONMESSAGEHANDLER_HPP_

#include "MessageHandler.hpp"
#include <client/src/LocalPTree.hpp>

namespace ptree
{
namespace client
{

class MetaUpdateNotificationMessageHandler : public MessageHandler
{
public:
    MetaUpdateNotificationMessageHandler(TransactionsCV& transactionsCV, LocalPTree& ptree);
    ~MetaUpdateNotificationMessageHandler() = default;
    void handle(protocol::MessageHeader& header, Buffer& message);
private:
    TransactionsCV& transactionsCV;
    LocalPTree& ptree;
};


} // namespace client
} // namespace ptree

#endif  // CLIENT_MESSAGEHANDLERS_METAUPDATENOTIFICATIONMESSAGEHANDLER_HPP_