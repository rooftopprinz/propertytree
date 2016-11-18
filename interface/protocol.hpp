#ifndef INTERFACE_PROTOOCOL_HPP_
#define INTERFACE_PROTOOCOL_HPP_

#include <cstdint>
#include <memory>
#include "MessageEssential.hpp"

#define PACKED __attribute__ ((packed))

namespace ptree
{

namespace protocol
{

/** It is really frustrating how I can't express some of these into a c++ data structure.
    Data serialization library might really be needed. Let's see.
**/

typedef uint32_t Uuid;

enum class PropertyType : uint8_t
{
    Value,
    Node,
    Rpc
};

enum class MessageType : uint8_t
{
    SigninRequest = 42, // 2a
    SigninResponse, // 2b
    CreateRequest, // 2c
    CreateResponse, //2d
    MetaUpdateNotification, // 2e
    DeleteRequest, // 2f
    DeleteResponse, // 30
    SetValueIndication, // 31
    SubscribePropertyUpdateRequest, // 32 
    SubscribePropertyUpdateResponse, // 33 
    PropertyUpdateNotification, // 34
    UnsubscribePropertyUpdateRequest, // 35
    UnsubscribePropertyUpdateResponse, // 36
    GetValueRequest, // <-----------------
    GetValueResponse, // 
    RpcRequest,
    RpcResponse,
    HandleRpcRequest,
    HandleRpcResponse,

    AquireOwnershipRequest,
    AquireOwnershipResponse,
    ReleaseOwnershipRequest,
    ReleaseOwnershipResponse
};

struct PACKED MessageHeader
{
    MessageType type;
    uint32_t size;
    uint32_t transactionId;
};


struct SigninRequest
{
    Simple<uint32_t> version;
    Simple<uint32_t> refreshRate;
    MESSAGE_FIELDS(version, refreshRate);
};

struct SigninResponse
{
    Simple<uint32_t> version;
    MESSAGE_FIELDS(version);
};


struct CreateRequest
{
    Simple<PropertyType> type;
    BufferBlock data;
    String path;
    MESSAGE_FIELDS(type, data, path);
};

struct CreateResponse
{
    enum class Response : uint8_t {OK, PARENT_NOT_FOUND, MALFORMED_PATH, ALREADY_EXIST, TYPE_ERROR};
    Simple<Response> response;
    MESSAGE_FIELDS(response);
};

struct MetaCreate
{
    MetaCreate()
    {}

    MetaCreate(Uuid uuid, PropertyType ptype, std::string path):
        uuid(uuid), propertyType(ptype), path(path)
    {}

    Simple<Uuid> uuid;
    Simple<PropertyType> propertyType;
    String path;
    MESSAGE_FIELDS(uuid, propertyType, path);
};

struct MetaDelete
{
    MetaDelete()
    {}

    MetaDelete(Uuid uuid):
        uuid(uuid)
    {}

    Simple<Uuid> uuid;
    MESSAGE_FIELDS(uuid);
};

struct MetaUpdateNotification
{
    BlockArray<MetaCreate> creations;
    BlockArray<MetaDelete> deletions;
    MESSAGE_FIELDS(creations, deletions);
};


struct DeleteRequest
{
    String path;
    MESSAGE_FIELDS(path);
};

struct DeleteResponse
{
    enum class Response : uint8_t {OK, OBJECT_NOT_FOUND, NOT_PERMITTED, NOT_EMPTY};
    Simple<Response> response;
    MESSAGE_FIELDS(response);
};


typedef std::shared_ptr<MessageHeader> MessageHeaderPtr;

} // namespace protocol
} // namespace ptree


// Notes:
// CreateResponse::response size

#endif  // INTERFACE_PROTOOCOL_HPP_