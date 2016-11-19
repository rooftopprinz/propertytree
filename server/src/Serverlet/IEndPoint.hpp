#ifndef SERVER_IENDPOINT_HPP_
#define SERVER_IENDPOINT_HPP_

#include <sys/types.h>
#include <sys/socket.h>
#include <server/src/Types.hpp>

namespace ptree
{
namespace server
{

struct IEndPoint
{
    virtual ssize_t send(const void *buffer, uint32_t size) = 0;
    virtual ssize_t receive(void *buffer, uint32_t size) = 0;
    // virtual void setReceiveTimeout(uint32_t timeout);
};

} // namespace server

} // namespace ptree

#endif  // SERVER_IENDPOINT_HPP_