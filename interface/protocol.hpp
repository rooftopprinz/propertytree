#ifndef INTERFACE_PROTOOCOL_HPP_
#define INTERFACE_PROTOOCOL_HPP_

#include "MessageCoDec.hpp"
#include "MessageCoDecHelpers.hpp"

namespace ptree
{

namespace protocol_x
{

#define PACKED __attribute__ ((packed))

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
    GetValueRequest, // 37
    GetValueResponse, // 38
    RpcRequest, // 39
    RpcResponse, // 3a
    HandleRpcRequest, // 3b
    HandleRpcResponse, // 3c
    GetSpecificMetaRequest, // 3d
    GetSpecificMetaResponse, // 3e

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
    SigninRequest():
        featureFlag(0)
    {}
    enum class FeatureFlag : uint8_t
    {
        ENABLE_METAUPDATE = 0
    };

    uint32_t version;
    uint32_t refreshRate;
    uint64_t featureFlag;

    inline void setFeature(FeatureFlag flag)
    {
        featureFlag |= 1 << (uint8_t)flag;
    }

    inline void resetFeature(FeatureFlag flag)
    {
        featureFlag &= ~(uint64_t(1) << (uint8_t)flag);
    }

    inline bool isSetFeature(FeatureFlag flag)
    {
        return featureFlag & uint64_t(1) << (uint8_t)flag;
    }

    MESSAGE_FIELDS_PROTOX(version, refreshRate, featureFlag);
};

struct SigninResponse
{
    uint32_t version;
    MESSAGE_FIELDS_PROTOX(version);
};

struct CreateRequest
{
    PropertyType type;
    std::vector<uint8_t> data;
    std::string path;
    MESSAGE_FIELDS_PROTOX(type, data, path);
};

struct CreateResponse
{
    enum class Response : uint8_t {OK, PARENT_NOT_FOUND, MALFORMED_PATH, ALREADY_EXIST, TYPE_ERROR};
    Response response;
    Uuid uuid;
    MESSAGE_FIELDS_PROTOX(response, uuid);
};

struct MetaCreate : public BlockBase
{
    MetaCreate()
    {}

    MetaCreate(Uuid uuid, PropertyType ptype, std::string path):
        uuid(uuid), propertyType(ptype), path(path)
    {}

    Uuid uuid;
    PropertyType propertyType;
    std::string path;
    MESSAGE_FIELDS_PROTOX(uuid, propertyType, path);
};

struct MetaDelete : public BlockBase
{
    MetaDelete()
    {}

    MetaDelete(Uuid uuid):
        uuid(uuid)
    {}

    Uuid uuid;
    MESSAGE_FIELDS_PROTOX(uuid);
};

struct MetaUpdateNotification
{
    BlockArray<MetaCreate> creations;
    BlockArray<MetaDelete> deletions;
    MESSAGE_FIELDS_PROTOX(BLOCK creations, BLOCK deletions);
};

struct DeleteRequest
{
    std::string path;
    MESSAGE_FIELDS_PROTOX(path);
};

struct DeleteResponse
{
    enum class Response : uint8_t {OK, OBJECT_NOT_FOUND, NOT_PERMITTED, NOT_EMPTY, MALFORMED_PATH};
    Response response;
    MESSAGE_FIELDS_PROTOX(response);
};

struct SetValueIndication
{
    Uuid uuid;
    std::vector<uint8_t> data;
    MESSAGE_FIELDS_PROTOX(uuid, data);
};

struct SubscribePropertyUpdateRequest
{
    Uuid uuid;
    MESSAGE_FIELDS_PROTOX(uuid);
};

struct SubscribePropertyUpdateResponse
{
    enum class Response : uint8_t {OK, UUID_NOT_FOUND, NOT_A_VALUE};
    Response response;
    MESSAGE_FIELDS_PROTOX(response);
};

struct PropertyUpdateNotificationEntry
{
    PropertyUpdateNotificationEntry(){}
    PropertyUpdateNotificationEntry(Uuid uuid, Buffer& buffer):
        uuid(uuid), data(buffer) {}
    Uuid uuid;
    std::vector<uint8_t> data;
    MESSAGE_FIELDS_PROTOX(uuid, data);
};

struct PropertyUpdateNotification
{
    BlockArray<PropertyUpdateNotificationEntry> propertyUpdateNotifications;
    MESSAGE_FIELDS_PROTOX(propertyUpdateNotifications);
};


struct UnsubscribePropertyUpdateRequest
{
    Uuid uuid;
    MESSAGE_FIELDS_PROTOX(uuid);
};

struct UnsubscribePropertyUpdateResponse
{
    enum class Response : uint8_t {OK, NOT_SUBSCRIBED, NOT_A_VALUE, UUID_NOT_FOUND};
    Response response;
    MESSAGE_FIELDS_PROTOX(response);
};

struct GetValueRequest
{
    Uuid uuid;
    MESSAGE_FIELDS_PROTOX(uuid);
};

struct GetValueResponse
{
    std::vector<uint8_t> data;
    MESSAGE_FIELDS_PROTOX(data);
};

struct RpcRequest
{
    Uuid uuid;
    std::vector<uint8_t> parameter;
    MESSAGE_FIELDS_PROTOX(uuid, parameter);
};

struct RpcResponse
{
    std::vector<uint8_t> returnValue;
    MESSAGE_FIELDS_PROTOX(returnValue);
};

struct HandleRpcRequest
{
    uint64_t callerId;
    uint32_t callerTransactionId;
    Uuid uuid;
    std::vector<uint8_t> parameter;
    MESSAGE_FIELDS_PROTOX(callerId, callerTransactionId, uuid, parameter);
};

struct HandleRpcResponse
{
    uint64_t callerId;
    uint32_t callerTransactionId;
    std::vector<uint8_t> returnValue;
    MESSAGE_FIELDS_PROTOX(callerId, callerTransactionId, returnValue);
};


struct GetSpecificMetaRequest
{
    std::string path;
    MESSAGE_FIELDS_PROTOX(path);
};

struct GetSpecificMetaResponse
{
    MetaCreate meta;
    MESSAGE_FIELDS_PROTOX(BLOCK meta);
};


} // namespace protocol
} // namespace ptree

#include "protocol_x.hpp"

#endif  // INTERFACE_PROTOOCOL_HPP_
