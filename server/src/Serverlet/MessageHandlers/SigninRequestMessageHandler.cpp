#include "SigninRequestMessageHandler.hpp"
#include <server/src/Serverlet/ClientServer.hpp>
#include <server/src/Logger.hpp>

namespace ptree
{
namespace server
{

SigninRequestMessageHandler::
    SigninRequestMessageHandler(ClientServer& cs, IEndPoint& ep, core::PTree& pt, IClientServerMonitor&  csmon):
        MessageHandler(cs,ep,pt,csmon)
{}

inline void SigninRequestMessageHandler::handle(protocol::MessageHeaderPtr header, BufferPtr message)
{
    logger::Logger log("SigninRequestMessageHandler");

    protocol::SigninRequest request;
    protocol::Decoder de(message->data(),message->data()+message->size());
    request << de;

    bool supported = true;
    if (request.version != 1)
    {
        log << logger::WARNING << "Version not supported: " <<
            request.version;
        supported = false;
    }

    clientServer.setUpdateInterval(request.refreshRate);
    clientServer.clientSigned();

    protocol::SigninResponse response;
    response.version = supported ? *request.version : 0;

    uint32_t sz = response.size()+sizeof(protocol::MessageHeader);
    Buffer rspheader = createHeader(protocol::MessageType::SigninResponse, sz, header->transactionId);
    endpoint.send(rspheader.data(), rspheader.size());
    
    Buffer responseMessageBuffer(response.size());
    protocol::BufferView responseMessageBufferView(responseMessageBuffer);
    protocol::Encoder en(responseMessageBufferView);
    response >> en;
    endpoint.send(responseMessageBuffer.data(), responseMessageBuffer.size());
    log << logger::DEBUG << "response size: " << sz;
}

} // namespace server
} // namespace ptree