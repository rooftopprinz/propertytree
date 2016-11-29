#include <interface/protocol.hpp>

namespace ptree
{
namespace common
{

typedef std::vector<uint8_t> Buffer;

struct MessageCreationHelper
{
    inline Buffer createHeader(protocol::MessageType type, uint32_t size, uint32_t transactionId)
    {
        Buffer header(sizeof(protocol::MessageHeader));
        protocol::MessageHeader& headerRaw = *((protocol::MessageHeader*)header.data());
        headerRaw.type = type;
        headerRaw.size = size;
        headerRaw.transactionId = transactionId;
        return header;
    }

    inline Buffer createSigninRequestMessage(uint32_t transactionId, uint32_t version, uint32_t refreshRate)
    {
        protocol::SigninRequest signin;
        signin.version = version;
        signin.refreshRate = refreshRate;
        uint32_t sz = signin.size() + sizeof(protocol::MessageHeader);

        Buffer message = createHeader(protocol::MessageType::SigninRequest, sz, transactionId);
        Buffer enbuff(signin.size());
        protocol::BufferView enbuffv(enbuff);
        protocol::Encoder en(enbuffv);
        signin >> en;
        message.insert(message.end(), enbuff.begin(), enbuff.end());

        return message;
    }

    inline Buffer createSigninResponseMessage(uint32_t transactionId, uint32_t version)
    {
        protocol::SigninResponse signin;
        signin.version = version;
        uint32_t sz = signin.size() + sizeof(protocol::MessageHeader);

        Buffer message = createHeader(protocol::MessageType::SigninResponse, sz, transactionId);
        Buffer enbuff(signin.size());
        protocol::BufferView enbuffv(enbuff);
        protocol::Encoder en(enbuffv);
        signin >> en;
        message.insert(message.end(), enbuff.begin(), enbuff.end());

        return message;
    }

    inline Buffer createCreateRequestMessage(uint32_t transactionId, Buffer valueContainer, protocol::PropertyType type,
        std::string path)
    {
        protocol::CreateRequest createReq;
        createReq.type = type;
        createReq.data = valueContainer;
        createReq.path = path;

        uint32_t sz = createReq.size() + sizeof(protocol::MessageHeader);

        Buffer message = createHeader(protocol::MessageType::CreateRequest, sz, transactionId);
        Buffer enbuff(createReq.size());
        protocol::BufferView enbuffv(enbuff);
        protocol::Encoder en(enbuffv);
        createReq >> en;
        message.insert(message.end(), enbuff.begin(), enbuff.end());

        return message;
    }

    inline Buffer createDeleteRequestMessage(uint32_t transactionId, std::string path)
    {
        protocol::DeleteRequest deleteReq;
        deleteReq.path = path;

        uint32_t sz = deleteReq.size() + sizeof(protocol::MessageHeader);

        Buffer message = createHeader(protocol::MessageType::DeleteRequest, sz, transactionId);
        Buffer enbuff(deleteReq.size());
        protocol::BufferView enbuffv(enbuff);
        protocol::Encoder en(enbuffv);
        deleteReq >> en;
        message.insert(message.end(), enbuff.begin(), enbuff.end());

        return message;
    }

    inline Buffer createSetValueIndicationMessage(uint32_t transactionId, protocol::Uuid uuid, Buffer value)
    {
        protocol::SetValueIndication setval;
        setval.uuid = uuid;
        setval.data = value;
        uint32_t sz = setval.size() + sizeof(protocol::MessageHeader);

        Buffer message = createHeader(protocol::MessageType::SetValueIndication, sz, transactionId);
        Buffer enbuff(setval.size());
        protocol::BufferView enbuffv(enbuff);
        protocol::Encoder en(enbuffv);
        setval >> en;
        message.insert(message.end(), enbuff.begin(), enbuff.end());

        return message;
    }

    inline Buffer createSubscribePropertyUpdateRequestMessage(uint32_t transactionId, protocol::Uuid uuid)
    {
        protocol::SubscribePropertyUpdateRequest request;
        request.uuid = uuid;
        uint32_t sz = request.size() + sizeof(protocol::MessageHeader);

        Buffer message = createHeader(protocol::MessageType::SubscribePropertyUpdateRequest, sz, transactionId);
        Buffer enbuff(request.size());
        protocol::BufferView enbuffv(enbuff);
        protocol::Encoder en(enbuffv);
        request >> en;
        message.insert(message.end(), enbuff.begin(), enbuff.end());

        return message;
    }

    inline Buffer createUnsubscribePropertyUpdateRequestMessage(uint32_t transactionId, protocol::Uuid uuid)
    {
        protocol::UnsubscribePropertyUpdateRequest request;
        request.uuid = uuid;
        uint32_t sz = request.size() + sizeof(protocol::MessageHeader);

        Buffer message = createHeader(protocol::MessageType::UnsubscribePropertyUpdateRequest, sz, transactionId);
        Buffer enbuff(request.size());
        protocol::BufferView enbuffv(enbuff);
        protocol::Encoder en(enbuffv);
        request >> en;
        message.insert(message.end(), enbuff.begin(), enbuff.end());

        return message;
    }

    template<typename TT, protocol::MessageType TR, typename T>
    inline Buffer createCommonResponse(uint32_t transactionId, T response)
    {
        TT responseMsg;
        responseMsg.response = response;

        uint32_t sz = responseMsg.size() + sizeof(protocol::MessageHeader);

        Buffer message = createHeader(TR, sz, transactionId);
        Buffer enbuff(responseMsg.size());
        protocol::BufferView enbuffv(enbuff);
        protocol::Encoder en(enbuffv);
        responseMsg >> en;
        message.insert(message.end(), enbuff.begin(), enbuff.end());

        return message;
    }

    inline Buffer createGetValueRequestMessage(uint32_t transactionId, protocol::Uuid uuid)
    {
        protocol::GetValueRequest request;
        request.uuid = uuid;

        uint32_t sz = request.size() + sizeof(protocol::MessageHeader);

        Buffer message = createHeader(protocol::MessageType::GetValueRequest, sz, transactionId);
        Buffer enbuff(request.size());
        protocol::BufferView enbuffv(enbuff);
        protocol::Encoder en(enbuffv);
        request >> en;
        message.insert(message.end(), enbuff.begin(), enbuff.end());

        return message;
    }

    inline Buffer createGetValueResponseMessage(uint32_t transactionId, Buffer value)
    {
        protocol::GetValueResponse response;
        response.data = value;

        uint32_t sz = response.size() + sizeof(protocol::MessageHeader);

        Buffer message = createHeader(protocol::MessageType::GetValueResponse, sz, transactionId);
        Buffer enbuff(response.size());
        protocol::BufferView enbuffv(enbuff);
        protocol::Encoder en(enbuffv);
        response >> en;
        message.insert(message.end(), enbuff.begin(), enbuff.end());

        return message;
    }

    inline Buffer createRpcRequestMessage(uint32_t transactionId, protocol::Uuid uuid, Buffer parameter)
    {
        protocol::RpcRequest request;
        request.uuid = uuid;
        request.parameter = parameter;

        uint32_t sz = request.size() + sizeof(protocol::MessageHeader);

        Buffer message = createHeader(protocol::MessageType::RpcRequest, sz, transactionId);
        Buffer enbuff(request.size());
        protocol::BufferView enbuffv(enbuff);
        protocol::Encoder en(enbuffv);
        request >> en;
        message.insert(message.end(), enbuff.begin(), enbuff.end());

        return message;
    }

    inline Buffer createHandleRpcRequestMessage(uint32_t transactionId, uint64_t callerId, uint32_t callerTransactionId, protocol::Uuid uuid, Buffer parameter)
    {
        protocol::HandleRpcRequest request;
        request.callerId = callerId;
        request.callerTransactionId = callerTransactionId;
        request.uuid = uuid;
        request.parameter = parameter;

        uint32_t sz = request.size() + sizeof(protocol::MessageHeader);

        Buffer message = createHeader(protocol::MessageType::HandleRpcRequest, sz, transactionId);
        Buffer enbuff(request.size());
        protocol::BufferView enbuffv(enbuff);
        protocol::Encoder en(enbuffv);
        request >> en;
        message.insert(message.end(), enbuff.begin(), enbuff.end());

        return message;
    }

    inline Buffer createHandleRpcResponseMessage(uint32_t transactionId, uint64_t callerId, uint32_t callerTransactionId, Buffer returnValue)
    {
        protocol::HandleRpcResponse response;
        response.returnValue = returnValue;
        response.callerId = callerId;
        response.callerTransactionId = callerTransactionId;

        uint32_t sz = response.size() + sizeof(protocol::MessageHeader);

        Buffer message = createHeader(protocol::MessageType::HandleRpcResponse, sz, transactionId);
        Buffer enbuff(response.size());
        protocol::BufferView enbuffv(enbuff);
        protocol::Encoder en(enbuffv);
        response >> en;
        message.insert(message.end(), enbuff.begin(), enbuff.end());

        return message;
    }

    inline Buffer createRpcResponseMessage(uint32_t transactionId, Buffer returnValue)
    {
        protocol::RpcResponse response;
        response.returnValue = returnValue;

        uint32_t sz = response.size() + sizeof(protocol::MessageHeader);

        Buffer message = createHeader(protocol::MessageType::RpcResponse, sz, transactionId);
        Buffer enbuff(response.size());
        protocol::BufferView enbuffv(enbuff);
        protocol::Encoder en(enbuffv);
        response >> en;
        message.insert(message.end(), enbuff.begin(), enbuff.end());

        return message;
    }
};

}
}