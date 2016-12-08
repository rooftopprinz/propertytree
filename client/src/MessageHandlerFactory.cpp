#include "MessageHandlerFactory.hpp"
#include <common/src/Logger.hpp>

namespace ptree
{
namespace client
{

class MessageHandler;

std::unique_ptr<MessageHandler>
    MessageHandlerFactory::
        get(protocol::MessageType type, PTreeClientPtr& pc, IEndPointPtr& ep)
{
    logger::Logger log("MessageHandlerFactory");
    using Enum = uint8_t;
    switch (uint8_t(type))
    {
        case (Enum) protocol::MessageType::SigninResponse:
        case (Enum) protocol::MessageType::CreateResponse:
        case (Enum) protocol::MessageType::GetSpecificMetaResponse:
        case (Enum) protocol::MessageType::GetValueResponse:
            return std::make_unique<GenericResponseMessageHandler>(*pc.get(), *ep.get());
    }

    log << logger::ERROR << "Unregconize message type.";
    return std::make_unique<MessageHandler>(*pc.get(), *ep.get());
}

} // namespace server
} // namespace ptree