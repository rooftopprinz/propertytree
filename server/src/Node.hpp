#ifndef __NODE_HPP__
#define __NODE_HPP__

#include <map>
#include <memory>
#include <unordered_map>

#include <bfc/EpollReactor.hpp>

#include <logless/Logger.hpp>

namespace propertytree
{

struct Node
{
    Node() = delete;
    Node(const std::string& pName, uint32_t pSessionId, std::weak_ptr<Node> pParent, uint64_t pUuid)
        : name(pName)
        , sessionId(pSessionId)
        , parent(pParent)
        , uuid(pUuid)
    {}

    std::string name;
    uint32_t sessionId;
    std::weak_ptr<Node> parent;
    uint64_t uuid;

    std::vector<uint8_t> data;
    std::map<std::string, std::shared_ptr<Node>> children;
    std::unordered_map<uint32_t, std::weak_ptr<IConnectionSession>> listener;

    std::mutex dataMutex;
    std::mutex childrenMutex;
    std::mutex listenerMutex;
};

} // propertytree

#endif // __NODE_HPP__