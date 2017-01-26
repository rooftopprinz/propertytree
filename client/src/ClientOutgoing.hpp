#ifndef CLIENT_CLIENTOUTGOING_HPP_
#define CLIENT_CLIENTOUTGOING_HPP_

#include <common/src/Logger.hpp>
#include "IClientOutgoing.hpp"
#include "TransactionsCV.hpp"
#include "Types.hpp"

namespace ptree
{
namespace client
{

class TransactionIdGenerator
{
public:
    TransactionIdGenerator():
        id(0)
    {
    }
    uint32_t get()
    {
        return id++;
    }
private:
    std::atomic<uint32_t> id;
};

class ClientOutgoing : public IClientOutgoing
{
public:
    ClientOutgoing(TransactionsCV& transactionsCV, common::IEndPoint& endpoint);
    ~ClientOutgoing();
    std::pair<uint32_t,std::shared_ptr<TransactionCV>>
        signinRequest(int refreshRate, const std::list<protocol::SigninRequest::FeatureFlag> features);
    // uint32_t createRequest();
    // uint32_t deleteRequest();
    // uint32_t setValueIndicationRequest();
    // uint32_t subscribePropertyUpdateRequest();
    // uint32_t unsubscribePropertyUpdateRequest();
    // uint32_t getValueRequest();
    // uint32_t rpcRequest();
    // uint32_t handleRpcRequest();
    // uint32_t getSpecificMetaRequest();
private:
    Buffer createHeader(protocol::MessageType type, uint32_t payloadSize, uint32_t transactionId);
    void sendToClient(uint32_t tid, protocol::MessageType mtype, protocol::Message& msg);
    std::mutex sendLock;

    TransactionsCV& transactionsCV;
    common::IEndPoint& endpoint;
    TransactionIdGenerator transactionIdGenerator;
    logger::Logger log;
};
}
}
#endif  // CLIENT_ICLIENTOUTGOING_HPP_