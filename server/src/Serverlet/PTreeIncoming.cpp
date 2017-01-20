#include "PTreeIncoming.hpp"
namespace ptree
{
namespace server
{

PTreeIncoming::PTreeIncoming(uint64_t clientServerId,
    ClientServerConfig& config, IEndPointPtr& endpoint, core::PTreePtr& ptree, IClientNotifierPtr& notifier):
        processMessageRunning(0),
        clientServerId(clientServerId), config(config), endpoint(endpoint), ptree(ptree),
        notifier(notifier), log("PTreeIncoming")
{
    log << logger::DEBUG << "construct";
}

PTreeIncoming::~PTreeIncoming()
{
    log << logger::DEBUG << "destruct";
}

void PTreeIncoming::setup(IPTreeOutgoingPtr o)
{
    outgoing = o;

    std::function<void()> incoming = std::bind(&PTreeIncoming::handleIncoming, this);
    killHandleIncoming = false;
    log << logger::DEBUG << "Creating incomingThread.";
    std::thread incomingThread(incoming); 
    incomingThread.detach();
    log << logger::DEBUG << "Created threads detached.";
    log << logger::DEBUG << "Setup complete.";
}

void PTreeIncoming::teardown()
{
    outgoing.reset();
    killHandleIncoming = true;
    log << logger::DEBUG << "teardown: waiting thread to stop...";
    log << logger::DEBUG << "teardown: handleIncoming " << handleIncomingIsRunning;
    log << logger::DEBUG << "teardown: prossesing " << processMessageRunning;
    while (handleIncomingIsRunning || processMessageRunning);
    log << logger::DEBUG << "Teardown complete.";
}

void PTreeIncoming::processMessage(protocol::MessageHeaderPtr header, BufferPtr message)
{
    processMessageRunning++;
    log << logger::DEBUG << "ClientServer::processMessage()";
    auto type = header->type;
    /** TODO: Remove after investigation is done **/
    // if (type==protocol::MessageType::SubscribePropertyUpdateRequest)
    //     std::make_unique<SubscribePropertyUpdateRequestMessageHandler>(lval_this, *endpoint.get(), *ptree.get(), *notifier.get())->handle(header, message);
    // else
        MessageHandlerFactory::get(clientServerId, type, config, outgoing, ptree, notifier)->handle(header, message);

    processMessageRunning--;
}

void PTreeIncoming::handleIncoming()
{
    handleIncomingIsRunning = true;
    const uint8_t HEADER_SIZE = sizeof(protocol::MessageHeader);
    log << logger::DEBUG << "handleIncoming: Spawned.";
    incomingState = EIncomingState::WAIT_FOR_HEADER_EMPTY;

    using namespace std::chrono_literals;

    while (!killHandleIncoming)
    {
        // Header is a shared for the reason that I might not block processMessage
        protocol::MessageHeaderPtr header = std::make_shared<protocol::MessageHeader>();

        if (incomingState == EIncomingState::WAIT_FOR_HEADER_EMPTY)
        {
            uint8_t cursor = 0;
            uint8_t retryCount = 0;

            log << logger::DEBUG << "handleIncoming: Waiting for header.";
            while (!killHandleIncoming)
            {
                ssize_t br = endpoint->receive(header.get()+cursor, HEADER_SIZE-cursor);
                cursor += br;

                if(cursor == HEADER_SIZE)
                {
                    log << logger::DEBUG << 
                        "Header received expecting message size: " <<
                        header->size << " with type " << (uint32_t)header->type;
                    incomingState = EIncomingState::WAIT_FOR_MESSAGE_EMPTY;
                    break;
                }

                if (incomingState != EIncomingState::WAIT_FOR_HEADER_EMPTY)
                {
                    log << logger::DEBUG <<  "handleIncoming: Header receive timeout!";
                    retryCount++;
                }
                else if (br != 0)
                {
                    log << logger::DEBUG << "handleIncoming: Header received.";
                    incomingState = EIncomingState::WAIT_FOR_HEADER;
                }

                if (retryCount >= 3)
                {
                    log << logger::DEBUG <<
                        "handleIncoming: Header receive failed!";
                    incomingState = EIncomingState::ERROR_HEADER_TIMEOUT;
                    // TODO: ERROR HANDLING
                    break;
                }

                std::this_thread::sleep_for(1ms);

            }
        }
        
        if (incomingState == EIncomingState::WAIT_FOR_MESSAGE_EMPTY)
        {
            uint8_t cursor = 0;
            uint8_t retryCount = 0;
            uint32_t expectedSize = (header->size)-HEADER_SIZE;
            BufferPtr message = std::make_shared<Buffer>(expectedSize);

            log << logger::DEBUG << "handleIncoming: Waiting for message with size: " <<
                expectedSize << " ( total " << header->size << ") with header size: " <<
                (uint32_t) HEADER_SIZE << ".";

            while (!killHandleIncoming)
            {
                ssize_t br = endpoint->receive(message->data()+cursor, expectedSize-cursor);
                cursor += br;

                if(cursor == expectedSize)
                {
                    log << logger::DEBUG << "handleIncoming: Message complete.";
                    using std::placeholders::_1;
                    incomingState = EIncomingState::WAIT_FOR_HEADER_EMPTY;
                    processMessage(header, message);
                    break;
                }

                if (incomingState != EIncomingState::WAIT_FOR_MESSAGE_EMPTY)
                {
                    log << logger::DEBUG <<
                        "handleIncoming: Message receive timeout!";
                    retryCount++;
                }
                else
                {
                    log << logger::DEBUG << "handleIncoming: Message received with size " << br;
                    incomingState = EIncomingState::WAIT_FOR_MESSAGE;
                }

                if (retryCount >= 3)
                {
                    log << logger::DEBUG <<
                        "handleIncoming: Message receive failed!";
                    incomingState = EIncomingState::ERROR_MESSAGE_TIMEOUT;
                    break;
                }

                std::this_thread::sleep_for(1ms);

            }
        }
    }

    log << logger::DEBUG << "handleIncoming: exiting.";
    handleIncomingIsRunning = false;
}

} // namespace server
} // namespace ptree
