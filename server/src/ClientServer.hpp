#ifndef SERVER_CLIENTSERVER_HPP_
#define SERVER_CLIENTSERVER_HPP_

#include <mutex>
#include <interface/protocol.hpp>
#include "PTreeTcpServer.hpp"
#include "PTree.hpp"
#include "Logger.hpp"
#include "Types.hpp"

namespace ptree
{
namespace server
{   

class IClientServerMonitor
{
public:
    IClientServerMonitor() {}
    virtual ~IClientServerMonitor() {}
    virtual void addClientServer(ClientServerPtr clientServer) = 0;
    virtual void removeClientServer(ClientServerPtr clientServer) = 0;
    virtual void notifyCreation(uint32_t uuid, protocol::PropertyType type, std::string path) = 0;
    virtual void notifyDeletion(uint32_t uuid) = 0;
};

class ClientServerMonitor : public IClientServerMonitor
{
public:
    ClientServerMonitor();
    ~ClientServerMonitor();
    void addClientServer(ClientServerPtr clientServer);
    void removeClientServer(ClientServerPtr clientServer);
    void notifyCreation(uint32_t uuid, protocol::PropertyType type, std::string path);
    void notifyDeletion(uint32_t uuid);
private:
    std::list<ClientServerPtr> clientServers;
    std::mutex clientServersMutex;
    logger::Logger log;
};

class ClientServer : public std::enable_shared_from_this<ClientServer>
{
public:
    ClientServer(IEndPointPtr endpoint, core::PTreePtr ptree, IClientServerMonitorPtr monitor):
        endpoint(endpoint),
        ptree(ptree),
        monitor(monitor),
        handleIncomingIsRunning(false),
        handleOutgoingIsRunning(false),
        isSetup(false),
        isSignin(false),
        processMessageRunning(0),
        updateInterval(0),
        log(logger::Logger("ClientServer"))
    {
    }

    ClientServer(const ClientServer&) = delete;
    ClientServer(ClientServer&) = delete;
    void operator = (const ClientServer&) = delete;
    void operator = (ClientServer&) = delete;

    ~ClientServer()
    {
    }

    void setup();
    void teardown();

    void handleIncoming();
    void handleOutgoing();

    void notifyCreation(uint32_t uuid, protocol::PropertyType type, std::string path);
    void notifyDeletion(uint32_t uuid);
    void notifyValueUpdate(core::ValuePtr);

    void setUpdateInterval(uint32_t interval);
    void clientSigned();

    struct ActionTypeAndPath
    {
        protocol::MetaUpdateNotification::UpdateType utype;
        protocol::PropertyType ptype;
        std::string path;
    };

    typedef std::map<uint32_t, ActionTypeAndPath> UuidActionTypeAndPathMap;

private:
    void processMessage(protocol::MessageHeaderPtr header, BufferPtr message);

    UuidActionTypeAndPathMap metaUpdateNotification;
    std::mutex metaUpdateNotificationMutex;

    std::list<core::ValuePtr> valueUpdateNotification;
    std::mutex valueUpdateNotificationMutex;

    IEndPointPtr endpoint;
    std::mutex sendLock;

    core::PTreePtr ptree;
    IClientServerMonitorPtr monitor;
    bool handleIncomingIsRunning;
    bool handleOutgoingIsRunning;
    bool killHandleIncoming;
    bool killHandleOutgoing;
    bool isSetup;
    bool isSignin;
    uint32_t processMessageRunning;
    uint32_t updateInterval;
    enum class EIncomingState
    {
        WAIT_FOR_HEADER_EMPTY,
        WAIT_FOR_HEADER,
        WAIT_FOR_MESSAGE_EMPTY,
        WAIT_FOR_MESSAGE,
        ERROR_HEADER_TIMEOUT,
        ERROR_MESSAGE_TIMEOUT
    };
    EIncomingState incomingState;
    logger::Logger log;
};

} // namespace server
} // namespace ptree

#endif  // SERVER_CLIENTSERVER_HPP_