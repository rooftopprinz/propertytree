#include "GetValueRequestMessageHandler.hpp"

#include <server/src/Serverlet/ClientServer.hpp>
#include <common/src/Logger.hpp>
#include <common/src/Utils.hpp>

namespace ptree
{
namespace server
{

GetValueRequestMessageHandler::GetValueRequestMessageHandler
    (ClientServer& cs, IEndPoint& ep, core::PTree& pt, IClientServerMonitor&  csmon):
        MessageHandler(cs,ep,pt,csmon)
{
}

void GetValueRequestMessageHandler::handle(protocol::MessageHeaderPtr header, BufferPtr message)
{
    logger::Logger log("GetValueRequestMessageHandler");

    protocol_x::GetValueRequest request;
    request.unpackFrom(*message);

    protocol_x::GetValueResponse response;

    log << logger::DEBUG << "Requesting value for: " << request.uuid;
    try
    {
        auto value = ptree.getPropertyByUuid<core::Value>(request.uuid);
        if (value)
        {
            response.data = value->getValue();
        }
        else
        {
            response.data = Buffer();
        }
    }
    catch (core::ObjectNotFound)
    {
        log << logger::ERROR << "Object(uuid)" << request.uuid << " not found.";
    }

    messageSender(header->transactionId, protocol::MessageType::GetValueResponse, response);
}

} // namespace server
} // namespace ptree