#ifndef SERVER_SERVERLET_MESSAGEHANDLERS_HANDLERPCRRESPONSEMESSAGEHANDLER_HPP_
#define SERVER_SERVERLET_MESSAGEHANDLERS_HANDLERPCRRESPONSEMESSAGEHANDLER_HPP_

#include <server/src/PTree.hpp>
#include <server/src/IPTreeServer.hpp>
#include <server/src/Serverlet/IPTreeOutgoing.hpp>
#include "MessageHandler.hpp"

namespace ptree
{
namespace server
{

class HandleRpcResponseMessageHandler : public MessageHandler
{
public:
    HandleRpcResponseMessageHandler(IPTreeServer& notifier);
    void handle(protocol::MessageHeader& header, Buffer& message);
private:
    IPTreeServer& notifier;
};

}
}
#endif // SERVER_SERVERLET_MESSAGEHANDLERS_HANDLERRESPONSEMESSAGEHANDLER_HPP_
