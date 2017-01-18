#include <memory>
#include <string>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <common/src/Logger.hpp>
#include <server/src/PTree.hpp>
#include <common/src/Logger.hpp>
#include <server/src/Serverlet/ClientServer.hpp>

#include <common/src/Utils.hpp>
#include "MessageMatchers/CreateObjectMetaUpdateNotificationMatcher.hpp"
#include "MessageMatchers/DeleteObjectMetaUpdateNotificationMatcher.hpp"
#include "MessageMatchers/MetaUpdateNotificationMatcher.hpp"
#include "MessageMatchers/PropertyUpdateNotificationMatcher.hpp"
#include <common/TestingFramework/MessageMatcher.hpp>
#include <common/TestingFramework/MessageCreationHelper.hpp>

using namespace testing;

namespace ptree
{
namespace server
{

typedef common::MatcherFunctor MatcherFunctor;
typedef common::ActionFunctor ActionFunctor;
typedef common::DefaultAction DefaultAction;
typedef common::MessageMatcher MessageMatcher;

struct ClientServerTests : public common::MessageCreationHelper, public ::testing::Test
{
    ClientServerTests() :

        testCreationAction(std::bind(&ClientServerTests::propTestCreationAction, this)),
        valueCreationDeleteImmediatelyAction(std::bind(&ClientServerTests::propValueCreationActionValueDelete, this)),
        valueCreationSubscribeAction(std::bind(&ClientServerTests::propValueCreationActionSubscribe, this)),
        endpoint(std::make_shared<common::EndPointMock>()),
        idgen(std::make_shared<core::IdGenerator>()),
        monitor(std::make_shared<ClientNotifier>()),
        ptree(std::make_shared<core::PTree>(idgen)),
        server(std::make_shared<ClientServer>(endpoint, ptree, monitor)),
        log("TEST")
    {
        auto signinRequestMsg = createSigninRequestMessage(signinRqstTid, 1, 100);
        endpoint->queueToReceive(signinRequestMsg);

        signinRspMsgMatcher = std::make_shared<MessageMatcher>(createSigninResponseMessage(signinRqstTid, 1));
        testCreationMatcher = std::make_shared<CreateObjectMetaUpdateNotificationMatcher>("/Test");
        valueCreationMatcher = std::make_shared<CreateObjectMetaUpdateNotificationMatcher>("/Test/Value");
        valueDeletionMatcher = std::make_shared<DeleteObjectMetaUpdateNotificationMatcher>();
        rpcCreationMatcher = std::make_shared<CreateObjectMetaUpdateNotificationMatcher>("/RpcTest");

        using protocol::CreateResponse;
        using protocol::DeleteResponse;
        using protocol::SubscribePropertyUpdateResponse;
        using protocol::UnsubscribePropertyUpdateResponse;
        using protocol::MessageType;

        createTestResponseFullMatcher = createCreateResponseMessage(createTestRequestTid,
            CreateResponse::Response::OK, protocol::Uuid(100));
        createValueResponseFullMatcher = createCreateResponseMessage(createValueRequestTid,
            CreateResponse::Response::OK, protocol::Uuid(101));
        createValueResponseAlreadyExistFullMatcher = createCreateResponseMessage(createValueRequest2Tid,
            CreateResponse::Response::ALREADY_EXIST, protocol::Uuid(0));
        createValueResponseInvalidPathFullMatcher = createCreateResponseMessage(createValueRequestTid,
            CreateResponse::Response::MALFORMED_PATH, protocol::Uuid(0));
        createValueResponseInvalidParentFullMatcher = createCreateResponseMessage(createValueRequestTid,
            CreateResponse::Response::PARENT_NOT_FOUND, protocol::Uuid(0));

        deleteValueResponseOkMatcher = createCommonResponse<DeleteResponse, MessageType::DeleteResponse>
                (deleteValueRequestTid, DeleteResponse::Response::OK);
        deleteValueResponseNotFoundMatcher = createCommonResponse<DeleteResponse, MessageType::DeleteResponse>
                (deleteValueRequestTid, DeleteResponse::Response::OBJECT_NOT_FOUND);
        deleteTestResponseNotEmptyMatcher = createCommonResponse<DeleteResponse, MessageType::DeleteResponse>
                (deleteTestRequestTid, DeleteResponse::Response::NOT_EMPTY);
        subscribeValueResponseOkMatcher =  createCommonResponse<SubscribePropertyUpdateResponse, MessageType::SubscribePropertyUpdateResponse>
                (subscribeValueRqstTid, SubscribePropertyUpdateResponse::Response::OK);
        subscribeValueResponseUuidNotFoundMatcher = createCommonResponse<SubscribePropertyUpdateResponse, MessageType::SubscribePropertyUpdateResponse>
                (subscribeValueRqstTid, SubscribePropertyUpdateResponse::Response::UUID_NOT_FOUND);
        subscribeTestResponseNotAValueMatcher = createCommonResponse<SubscribePropertyUpdateResponse, MessageType::SubscribePropertyUpdateResponse>
                (subscribeTestRqstTid, SubscribePropertyUpdateResponse::Response::NOT_A_VALUE);
        subscribeTestResponseNotAValueMatcher = createCommonResponse<SubscribePropertyUpdateResponse, MessageType::SubscribePropertyUpdateResponse>
                (subscribeTestRqstTid, SubscribePropertyUpdateResponse::Response::NOT_A_VALUE);
        unsubscribeValueResponseOkMatcher = createCommonResponse<UnsubscribePropertyUpdateResponse, MessageType::UnsubscribePropertyUpdateResponse>
                (unsubscribeValueRqstTid, UnsubscribePropertyUpdateResponse::Response::OK);

        auto testVal = valueToBuffer<uint32_t>(42);
        createTestRequestMessage = createCreateRequestMessage(
            createTestRequestTid, Buffer(), protocol::PropertyType::Node, "/Test");
        createValueRequestMessage = createCreateRequestMessage(
            createValueRequestTid, testVal, protocol::PropertyType::Value, "/Test/Value");
        createValueRequestMessageForAlreadyExist = createCreateRequestMessage(
            createValueRequest2Tid, testVal, protocol::PropertyType::Value, "/Test/Value");
        createValueRequestMessageForAlreadyInvalidPath = createCreateRequestMessage(
            createValueRequestTid, testVal, protocol::PropertyType::Value, "/Test//Value");
        deleteRequestMessageForValueMessage = createDeleteRequestMessage(deleteValueRequestTid, "/Test/Value");
        deleteRequestMessageForTestMessage = createDeleteRequestMessage(deleteTestRequestTid, "/Test");
        createRpcTestMessage = createCreateRequestMessage(
            createRpcRequestTid, Buffer(), protocol::PropertyType::Rpc, "/RpcTest");
    }
    void TearDown()
    {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(1ms);
    }

    template<typename T>
    Buffer valueToBuffer(T in)
    {
        Buffer bf(sizeof(in));
        *((T*)bf.data()) = in;
        return bf;
    }

    void propTestCreationAction()
    {
        this->log << logger::DEBUG << "/Test is created with uuid: " <<
            this->testCreationMatcher->getUuidOfLastMatched();

        createValueRequestMessage = createCreateRequestMessage(createValueRequestTid, valueToBuffer<uint32_t>(42),
            protocol::PropertyType::Value, "/Test/Value");

        this->endpoint->queueToReceive(createValueRequestMessage);
    }

    void propValueCreationActionValueDelete()
    {
        this->log << logger::DEBUG << "/Test/Value is created with uuid: " << this->valueCreationMatcher->getUuidOfLastMatched(); // nolint
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(10ms);
        this->log << logger::DEBUG << "Requesting deletion of /Test/Value";
        endpoint->queueToReceive(deleteRequestMessageForValueMessage);
    }

    void propValueCreationActionSubscribe()
    {
        uuidOfValue = this->valueCreationMatcher->getUuidOfLastMatched(); 
        log << logger::DEBUG << "/Test/Value is created with uuid: " << this->valueCreationMatcher->getUuidOfLastMatched();
        this->endpoint->queueToReceive(createSubscribePropertyUpdateRequestMessage(subscribeValueRqstTid, uuidOfValue));
    };

    uint32_t uuidOfValue = static_cast<uint32_t>(-1);
    const uint32_t signinRqstTid = 0;
    const uint32_t createTestRequestTid = 1;
    const uint32_t createValueRequestTid = 2;
    const uint32_t createValueRequest2Tid = 3;
    const uint32_t deleteValueRequestTid = 4;
    const uint32_t deleteTestRequestTid = 5;
    const uint32_t setValueInd1stTid = 6;
    const uint32_t subscribeTestRqstTid = 7;
    const uint32_t subscribeValueRqstTid = 8;
    const uint32_t setValueInd2ndTid = 9;
    const uint32_t unsubscribeValueRqstTid = 10;
    const uint32_t setValueInd3rdTid = 11;
    const uint32_t getValueReqTid = 12;
    const uint32_t createRpcRequestTid = 13;

/*****

Test common timeline

0) SignIn
1) create /Test
2) create /Test/Value
3) create /Test/Value again
4) delete /Test/Value
5) delete /Test
6) set /Test/Value
7) subscribe /Test
8) subscribe /Test/Value
9) set /Test/Value
10) unsubscribe /Test/Value
11) set /Test/Value
12) get /Test/Value

******/


    Buffer createTestRequestMessage;
    Buffer createValueRequestMessage;
    Buffer createValueRequestMessageForAlreadyExist;
    Buffer createValueRequestMessageForAlreadyInvalidPath;
    Buffer deleteRequestMessageForValueMessage;
    Buffer deleteRequestMessageForTestMessage;
    Buffer createRpcTestMessage;

    std::shared_ptr<MessageMatcher> signinRspMsgMatcher;
    std::shared_ptr<CreateObjectMetaUpdateNotificationMatcher> testCreationMatcher;
    std::shared_ptr<CreateObjectMetaUpdateNotificationMatcher> valueCreationMatcher;
    std::shared_ptr<DeleteObjectMetaUpdateNotificationMatcher> valueDeletionMatcher;
    std::shared_ptr<CreateObjectMetaUpdateNotificationMatcher> rpcCreationMatcher;

    MessageMatcher createTestResponseFullMatcher;
    MessageMatcher createValueResponseFullMatcher;
    MessageMatcher createValueResponseAlreadyExistFullMatcher;
    MessageMatcher createValueResponseInvalidPathFullMatcher;
    MessageMatcher createValueResponseInvalidParentFullMatcher;
    MessageMatcher deleteValueResponseOkMatcher;
    MessageMatcher deleteValueResponseNotFoundMatcher;
    MessageMatcher deleteTestResponseNotEmptyMatcher;
    MessageMatcher subscribeValueResponseOkMatcher;
    MessageMatcher subscribeValueResponseUuidNotFoundMatcher;
    MessageMatcher subscribeTestResponseNotAValueMatcher;
    MessageMatcher unsubscribeValueResponseOkMatcher;
    MessageMatcher handleRpcRequestMatcher;
    MessageMatcher rpcResponseMatcher;

    std::function<void()> testCreationAction;
    std::function<void()> valueCreationDeleteImmediatelyAction;
    std::function<void()> valueCreationSubscribeAction;

    std::shared_ptr<common::EndPointMock> endpoint;
    core::IIdGeneratorPtr idgen;
    IClientNotifierPtr monitor;
    core::PTreePtr ptree;
    ClientServerPtr server;
    logger::Logger log;
};

class ClientNotifierMock : public IClientNotifier
{
public:
    ClientNotifierMock() {}
    ~ClientNotifierMock() {}
    using IClientNotifier::addClientServer;
    using IClientNotifier::removeClientServer;
    MOCK_METHOD1(addClientServer, void(ClientServerPtr));
    MOCK_METHOD1(removeClientServer, void(ClientServerPtr));
    MOCK_METHOD3(notifyCreation, void(uint32_t, protocol::PropertyType, std::string));
    MOCK_METHOD1(notifyDeletion, void(uint32_t));
};

TEST_F(ClientServerTests, shouldSigninRequestAndRespondSameVersionForOk)
{
    endpoint->expectSend(0, 0, false, 1, signinRspMsgMatcher->get(), DefaultAction::get());

    using namespace std::chrono_literals;
    std::this_thread::sleep_for(1ms);
    server->setup();
    endpoint->waitForAllSending(2500.0);
    server->teardown();
}

TEST_F(ClientServerTests, shouldCreateOnPTreeWhenCreateRequested)
{
    endpoint->queueToReceive(createTestRequestMessage);

    std::function<void()> valueCreationAction = [this]()
    {
        log << logger::DEBUG << "fetching /Test/Value";
        log << logger::DEBUG << "/Test/Value is created with uuid: " <<
            this->valueCreationMatcher->getUuidOfLastMatched();
        core::ValuePtr val;
        ASSERT_NO_THROW(val = this->ptree->getPropertyByPath<core::Value>("/Test/Value"));
        EXPECT_EQ(42u, val->getValue<uint32_t>());
    };

    endpoint->expectSend(0, 0, false, 1, signinRspMsgMatcher->get(), DefaultAction::get());
    endpoint->expectSend(1, 0, true, 1, testCreationMatcher->get(), testCreationAction);
    endpoint->expectSend(2, 1, true, 1, valueCreationMatcher->get(), valueCreationAction);

    using namespace std::chrono_literals;
    std::this_thread::sleep_for(100ms);

    server->setup();
    endpoint->waitForAllSending(2500.0);
    server->teardown();

    logger::loggerServer.waitEmpty();
}

TEST_F(ClientServerTests, shouldGenerateMessageCreateResponse)
{
    endpoint->queueToReceive(createTestRequestMessage);;

    std::function<void()> valueCreationAction = [this]()
    {
        log << logger::DEBUG << "/Test/Value is created with uuid: " <<
            this->valueCreationMatcher->getUuidOfLastMatched();
    };

    endpoint->expectSend(1, 0, true, 1, testCreationMatcher->get(), testCreationAction);
    endpoint->expectSend(2, 1, true, 1, valueCreationMatcher->get(), valueCreationAction);
    endpoint->expectSend(0, 0, false, 1, signinRspMsgMatcher->get(), DefaultAction::get());
    endpoint->expectSend(0, 0, false, 1, createTestResponseFullMatcher.get(), DefaultAction::get());
    endpoint->expectSend(0, 0, false, 1, createValueResponseFullMatcher.get(), DefaultAction::get());

    server->setup();
    endpoint->waitForAllSending(15000.0);
    server->teardown();

    logger::loggerServer.waitEmpty();
}

TEST_F(ClientServerTests, shouldNotCreateWhenAlreadyExisting)
{
    endpoint->queueToReceive(createTestRequestMessage);

    std::function<void()> valueCreationAction = [this]()
    {
        log << logger::DEBUG << "/Test/Value is created with uuid: " <<
            this->valueCreationMatcher->getUuidOfLastMatched();
        endpoint->queueToReceive(this->createValueRequestMessageForAlreadyExist);
    };

    endpoint->expectSend(1, 0, true, 1, testCreationMatcher->get(), testCreationAction);
    endpoint->expectSend(2, 1, true, 1, valueCreationMatcher->get(), valueCreationAction);
    endpoint->expectSend(3, 0, false, 1, signinRspMsgMatcher->get(), DefaultAction::get());
    endpoint->expectSend(4, 0, false, 1, createTestResponseFullMatcher.get(), DefaultAction::get());
    endpoint->expectSend(5, 0, false, 1, createValueResponseFullMatcher.get(), DefaultAction::get());
    endpoint->expectSend(6, 0, false, 1, createValueResponseAlreadyExistFullMatcher.get(), DefaultAction::get());

    server->setup();
    endpoint->waitForAllSending(2500.0);
    server->teardown();

    logger::loggerServer.waitEmpty();
}

TEST_F(ClientServerTests, shouldNotCreateWhenPathIsMalformed)
{
    endpoint->queueToReceive(createTestRequestMessage);

    std::function<void()> testCreationAction = [this]()
    {
        this->log << logger::DEBUG << "/Test is created with uuid: " << this->testCreationMatcher->getUuidOfLastMatched();  //nolint
        endpoint->queueToReceive(createValueRequestMessageForAlreadyInvalidPath);
    };

    endpoint->expectSend(1, 0, true, 1, testCreationMatcher->get(), testCreationAction);
    endpoint->expectSend(2, 0, false, 1, signinRspMsgMatcher->get(), DefaultAction::get());
    endpoint->expectSend(3, 0, false, 1, createTestResponseFullMatcher.get(), DefaultAction::get());
    endpoint->expectSend(4, 0, false, 1, createValueResponseInvalidPathFullMatcher.get(), DefaultAction::get());

    server->setup();
    endpoint->waitForAllSending(10000.0);
    server->teardown();

    logger::loggerServer.waitEmpty();
}

TEST_F(ClientServerTests, shouldNotCreateWhenParentObjectIsInvalid)
{
    endpoint->queueToReceive(createValueRequestMessage);

    endpoint->expectSend(2, 0, false, 1, signinRspMsgMatcher->get(), DefaultAction::get());
    endpoint->expectSend(4, 0, false, 1, createValueResponseInvalidParentFullMatcher.get(), DefaultAction::get());

    server->setup();
    endpoint->waitForAllSending(10000.0);
    server->teardown();

    logger::loggerServer.waitEmpty();
}

TEST_F(ClientServerTests, shouldDeleteOnPTree)
{
    endpoint->queueToReceive(createTestRequestMessage);

    std::function<void()> valueDeletionAction = [this]()
    {
        this->log << logger::DEBUG << "/Test/Value is created with uuid: " << this->valueCreationMatcher->getUuidOfLastMatched(); // nolint
        ASSERT_THROW(this->ptree->getPropertyByPath<core::Value>("/Test/Value"), core::ObjectNotFound);
    };

    endpoint->expectSend(1, 0, true, 1, testCreationMatcher->get(), testCreationAction);
    endpoint->expectSend(2, 1, true, 1, valueCreationMatcher->get(), valueCreationDeleteImmediatelyAction);
    endpoint->expectSend(3, 0, false, 1, signinRspMsgMatcher->get(), DefaultAction::get());
    endpoint->expectSend(4, 0, false, 1, createTestResponseFullMatcher.get(), DefaultAction::get());
    endpoint->expectSend(5, 0, false, 1, createValueResponseFullMatcher.get(), DefaultAction::get());

    server->setup();
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(1s);
    endpoint->waitForAllSending(10000.0);
    server->teardown();

    logger::loggerServer.waitEmpty();
}

TEST_F(ClientServerTests, shouldGenerateDeleteResponse)
{
    endpoint->queueToReceive(createTestRequestMessage);

    endpoint->expectSend(1, 0, true, 1, testCreationMatcher->get(), testCreationAction);
    endpoint->expectSend(2, 1, true, 1, valueCreationMatcher->get(), valueCreationDeleteImmediatelyAction);
    endpoint->expectSend(3, 0, false, 1, signinRspMsgMatcher->get(), DefaultAction::get());
    endpoint->expectSend(4, 0, false, 1, createTestResponseFullMatcher.get(), DefaultAction::get());
    endpoint->expectSend(5, 0, false, 1, createValueResponseFullMatcher.get(), DefaultAction::get());
    endpoint->expectSend(6, 0, false, 1, deleteValueResponseOkMatcher.get(), DefaultAction::get());

    server->setup();
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(1s);
    endpoint->waitForAllSending(10000.0);
    server->teardown();

    logger::loggerServer.waitEmpty();
}


TEST_F(ClientServerTests, shouldDeleteResponseNotFound)
{
    endpoint->queueToReceive(createTestRequestMessage);

    std::function<void()> testCreationAction = [this]()
    {
        this->log << logger::DEBUG << "/Test is created with uuid: " << this->testCreationMatcher->getUuidOfLastMatched();
        endpoint->queueToReceive(deleteRequestMessageForValueMessage);
    };

    std::function<void()> valueDeletionAction = [this]()
    {
        this->log << logger::DEBUG << "/Test/Value is created with uuid: " << this->valueCreationMatcher->getUuidOfLastMatched();
        ASSERT_THROW(this->ptree->getPropertyByPath<core::Value>("/Test/Value"), core::ObjectNotFound);
    };

    endpoint->expectSend(1, 0, true, 1, testCreationMatcher->get(), testCreationAction);
    endpoint->expectSend(3, 0, false, 1, signinRspMsgMatcher->get(), DefaultAction::get());
    endpoint->expectSend(4, 0, false, 1, createTestResponseFullMatcher.get(), DefaultAction::get());
    endpoint->expectSend(6, 0, false, 1, deleteValueResponseNotFoundMatcher.get(), valueDeletionAction);

    server->setup();
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(1s);
    endpoint->waitForAllSending(10000.0);
    server->teardown();

    logger::loggerServer.waitEmpty();
}


TEST_F(ClientServerTests, shouldDeleteResponseNotEmpty)
{
    endpoint->queueToReceive(createTestRequestMessage);

    std::function<void()> valueCreationAction = [this]()
    {
        this->log << logger::DEBUG << "/Test/Value is created with uuid: " << this->valueCreationMatcher->getUuidOfLastMatched(); // nolint
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(10ms);
        this->log << logger::DEBUG << "Requesting deletion of /Test";
        endpoint->queueToReceive(deleteRequestMessageForTestMessage);
    };

    endpoint->expectSend(1, 0, true, 1, testCreationMatcher->get(), testCreationAction);
    endpoint->expectSend(2, 1, true, 1, valueCreationMatcher->get(), valueCreationAction);
    endpoint->expectSend(3, 0, false, 1, signinRspMsgMatcher->get(), DefaultAction::get());
    endpoint->expectSend(4, 0, false, 1, createTestResponseFullMatcher.get(), DefaultAction::get());
    endpoint->expectSend(5, 0, false, 1, createValueResponseFullMatcher.get(), DefaultAction::get());
    endpoint->expectSend(6, 0, false, 1, deleteTestResponseNotEmptyMatcher.get(), DefaultAction::get());

    server->setup();
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(1s);
    endpoint->waitForAllSending(10000.0);
    server->teardown();

    logger::loggerServer.waitEmpty();
}

TEST_F(ClientServerTests, shouldDeleteWithMetaUpdateNotification)
{
    endpoint->queueToReceive(createTestRequestMessage);

    std::function<void()> valueCreationAction = [this]()
    {
        this->log << logger::DEBUG << "/Test/Value is created with uuid: " << this->valueCreationMatcher->getUuidOfLastMatched(); // nolint
        this->valueDeletionMatcher->setUuid(valueCreationMatcher->getUuidOfLastMatched());
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(10ms);
        this->log << logger::DEBUG << "Requesting deletion of /testing/Value";
        endpoint->queueToReceive(deleteRequestMessageForValueMessage);
    };

    endpoint->expectSend(1, 0, true, 1, testCreationMatcher->get(), testCreationAction);
    endpoint->expectSend(2, 1, true, 1, valueCreationMatcher->get(), valueCreationAction);
    endpoint->expectSend(3, 2, true, 1, valueDeletionMatcher->get(), DefaultAction::get());
    endpoint->expectSend(0, 0, false, 1, signinRspMsgMatcher->get(), DefaultAction::get());
    endpoint->expectSend(0, 0, false, 1, createTestResponseFullMatcher.get(), DefaultAction::get());
    endpoint->expectSend(0, 0, false, 1, createValueResponseFullMatcher.get(), DefaultAction::get());

    server->setup();
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(1s);
    endpoint->waitForAllSending(10000.0);
    server->teardown();

    logger::loggerServer.waitEmpty();
}

TEST_F(ClientServerTests, shouldSetSetValueWhenSetValueIndIsValid)
{
    endpoint->queueToReceive(createTestRequestMessage);

    std::function<void()> valueCreationAction = [this]()
    {
        uint32_t uuid = this->valueCreationMatcher->getUuidOfLastMatched(); 
        log << logger::DEBUG << "/Test/Value is created with uuid: " << this->valueCreationMatcher->getUuidOfLastMatched();
        auto data = utils::buildBufferedValue<uint32_t>(41);

        this->endpoint->queueToReceive(createSetValueIndicationMessage(setValueInd1stTid, uuid, data));
    };

    endpoint->expectSend(1, 0, true, 1, testCreationMatcher->get(), testCreationAction);
    endpoint->expectSend(2, 1, true, 1, valueCreationMatcher->get(), valueCreationAction);
    endpoint->expectSend(0, 0, false, 1, signinRspMsgMatcher->get(), DefaultAction::get());
    endpoint->expectSend(0, 0, false, 1, createTestResponseFullMatcher.get(), DefaultAction::get());
    endpoint->expectSend(0, 0, false, 1, createValueResponseFullMatcher.get(), DefaultAction::get());

    server->setup();

    log << logger::DEBUG << "Waiting for setval processing...";
    using namespace std::chrono_literals;
    /** TODO: use the value update notification matcher for this checking to avoid waiting **/
    std::this_thread::sleep_for(6s);
    core::ValuePtr val;
    ASSERT_NO_THROW(val = this->ptree->getPropertyByPath<core::Value>("/Test/Value"));
    EXPECT_EQ(41u, val->getValue<uint32_t>());
    std::this_thread::sleep_for(500ms);
    endpoint->waitForAllSending(15000.0);
    server->teardown();

    logger::loggerServer.waitEmpty();
}

TEST_F(ClientServerTests, shouldGenerateMessageSubscribePropertyUpdateResponseOk)
{
    endpoint->queueToReceive(createTestRequestMessage);

    std::function<void()> valueCreationAction = [this]()
    {
        uint32_t uuid = this->valueCreationMatcher->getUuidOfLastMatched();
        log << logger::DEBUG << "/Test/Value is created with uuid: " << uuid;
        this->endpoint->queueToReceive(createSubscribePropertyUpdateRequestMessage(subscribeValueRqstTid, uuid));
    };

    endpoint->expectSend(1, 0, true, 1, testCreationMatcher->get(), testCreationAction);
    endpoint->expectSend(2, 1, true, 1, valueCreationMatcher->get(), valueCreationAction);
    endpoint->expectSend(0, 0, false, 1, signinRspMsgMatcher->get(), DefaultAction::get());
    endpoint->expectSend(0, 0, false, 1, subscribeValueResponseOkMatcher.get(), DefaultAction::get());

    server->setup();
    endpoint->waitForAllSending(10000.0);
    server->teardown();

    logger::loggerServer.waitEmpty();
}

TEST_F(ClientServerTests, shouldGenerateMessageSubscribePropertyUpdateResponseUuidNotFound)
{
    endpoint->queueToReceive(createTestRequestMessage);

    std::function<void()> valueCreationAction = [this]()
    {
        uint32_t uuid = static_cast<uint32_t>(-1);
        this->endpoint->queueToReceive(createSubscribePropertyUpdateRequestMessage(subscribeValueRqstTid, uuid));
    };

    endpoint->expectSend(1, 0, true, 1, testCreationMatcher->get(), testCreationAction);
    endpoint->expectSend(2, 1, true, 1, valueCreationMatcher->get(), valueCreationAction);
    endpoint->expectSend(0, 0, false, 1, signinRspMsgMatcher->get(), DefaultAction::get());
    endpoint->expectSend(0, 0, false, 1, subscribeValueResponseUuidNotFoundMatcher.get(), DefaultAction::get());

    server->setup();
    endpoint->waitForAllSending(10000.0);
    server->teardown();

    logger::loggerServer.waitEmpty();
}


TEST_F(ClientServerTests, shouldGenerateMessageSubscribePropertyUpdateResponseUuidNotAValue)
{
    endpoint->queueToReceive(createTestRequestMessage);

    std::function<void()> testCreationAction = [this]()
    {
        uint32_t uuid = this->testCreationMatcher->getUuidOfLastMatched();
        this->log << logger::DEBUG << "/Test is created with uuid: " << uuid;
        this->endpoint->queueToReceive(createSubscribePropertyUpdateRequestMessage(subscribeTestRqstTid, uuid));
    };

    endpoint->expectSend(1, 0, true, 1, testCreationMatcher->get(), testCreationAction);
    endpoint->expectSend(0, 0, false, 1, signinRspMsgMatcher->get(), DefaultAction::get());
    endpoint->expectSend(0, 0, false, 1, subscribeTestResponseNotAValueMatcher.get(), DefaultAction::get());


    server->setup();
    endpoint->waitForAllSending(10000.0);
    server->teardown();

    logger::loggerServer.waitEmpty();
}

TEST_F(ClientServerTests, shouldSendPropertyUpdateNotificationWhenChanged)
{
    endpoint->queueToReceive(createTestRequestMessage);

    auto expectedValue = utils::buildSharedBufferedValue(6969);
    PropertyUpdateNotificationMatcher valueUpdateMatcher("/Test/Value", expectedValue, ptree);

    std::function<void()> subscribeValueRspAction = [this, &expectedValue]()
    {
        log << logger::DEBUG << "Subscribed to uuid: " << this->uuidOfValue;
        this->endpoint->queueToReceive(createSetValueIndicationMessage(setValueInd1stTid, this->uuidOfValue, *expectedValue));
    };

    endpoint->expectSend(1, 0, true, 1, testCreationMatcher->get(), testCreationAction);
    endpoint->expectSend(2, 1, true, 1, valueCreationMatcher->get(), valueCreationSubscribeAction);
    endpoint->expectSend(3, 0, false, 1, signinRspMsgMatcher->get(), DefaultAction::get());
    endpoint->expectSend(4, 0, false, 1, createTestResponseFullMatcher.get(), DefaultAction::get());
    endpoint->expectSend(5, 0, false, 1, createValueResponseFullMatcher.get(), DefaultAction::get());
    endpoint->expectSend(6, 0, false, 1, subscribeValueResponseOkMatcher.get(), subscribeValueRspAction);
    endpoint->expectSend(6, 0, false, 1, valueUpdateMatcher.get(), DefaultAction::get());

    server->setup();
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(100ms);
    endpoint->waitForAllSending(10000.0);
    server->teardown();

    logger::loggerServer.waitEmpty();
}

TEST_F(ClientServerTests, shouldNotSendPropertyUpdateNotificationWhenUnsubscribed)
{
    endpoint->queueToReceive(createTestRequestMessage);

    auto expectedValue = utils::buildSharedBufferedValue(6969);
    PropertyUpdateNotificationMatcher valueUpdateMatcher("/Test/Value", expectedValue, ptree);

    std::function<void()> testCreationAction = [this]()
    {
        this->log << logger::DEBUG << "/Test is created with uuid: " << this->testCreationMatcher->getUuidOfLastMatched();
        this->endpoint->queueToReceive(createValueRequestMessage);
    };

    std::function<void()> subscribeValueRspAction = [this, &expectedValue]()
    {
        uint32_t uuid = this->valueCreationMatcher->getUuidOfLastMatched();
        log << logger::DEBUG << "/Test/Value is created with uuid: " << uuid;
        this->endpoint->queueToReceive(createSetValueIndicationMessage(setValueInd1stTid, this->uuidOfValue, *expectedValue));
    };

    std::function<void()> valueUpdateAction = [this, &expectedValue]()
    {
        log << logger::DEBUG << "Unsubscribing /Test/Value";
        this->endpoint->queueToReceive(createUnsubscribePropertyUpdateRequestMessage(unsubscribeValueRqstTid, this->uuidOfValue));
    };

    std::function<void()> unsubscribeValueAction = [this, &expectedValue]()
    {
        log << logger::DEBUG << "Setting value again";
        this->endpoint->queueToReceive(createSetValueIndicationMessage(setValueInd2ndTid, this->uuidOfValue, *expectedValue));
    };

    endpoint->expectSend(1, 0, true, 1, testCreationMatcher->get(), testCreationAction);
    endpoint->expectSend(2, 1, true, 1, valueCreationMatcher->get(), valueCreationSubscribeAction);
    endpoint->expectSend(3, 0, false, 1, signinRspMsgMatcher->get(), DefaultAction::get());
    endpoint->expectSend(4, 0, false, 1, createTestResponseFullMatcher.get(), DefaultAction::get());
    endpoint->expectSend(5, 0, false, 1, createValueResponseFullMatcher.get(), DefaultAction::get());
    endpoint->expectSend(6, 0, false, 1, subscribeValueResponseOkMatcher.get(), subscribeValueRspAction);
    endpoint->expectSend(7, 0, false, 1, valueUpdateMatcher.get(), valueUpdateAction);
    endpoint->expectSend(8, 0, false, 1, unsubscribeValueResponseOkMatcher.get(), unsubscribeValueAction);

    server->setup();
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(500ms);
    endpoint->waitForAllSending(10000.0);
    server->teardown();

    logger::loggerServer.waitEmpty();
}

TEST_F(ClientServerTests, shouldGetValue)
{
    endpoint->queueToReceive(createTestRequestMessage);

    auto expectedValue = utils::buildSharedBufferedValue(6969);
    MessageMatcher getValueResponseFullMatcher(createGetValueResponseMessage(getValueReqTid, *expectedValue));

    PropertyUpdateNotificationMatcher valueUpdateMatcher("/Test/Value", expectedValue, ptree);

    std::function<void()> subscribeValueRspAction = [this, &expectedValue]()
    {
        log << logger::DEBUG << "Subscribed to uuid: " << this->uuidOfValue;
        /** TODO: setValueReqTid **/
        this->endpoint->queueToReceive(createSetValueIndicationMessage(getValueReqTid, this->uuidOfValue, *expectedValue));
    };

    std::function<void()> valueUpdateAction = [this]()
    {
        log << logger::DEBUG << "Getting value of uuid: " << this->uuidOfValue;
        this->endpoint->queueToReceive(createGetValueRequestMessage(getValueReqTid, this->uuidOfValue));
    };

    endpoint->expectSend(1, 0, true, 1, testCreationMatcher->get(), testCreationAction);
    endpoint->expectSend(2, 1, true, 1, valueCreationMatcher->get(), valueCreationSubscribeAction);
    endpoint->expectSend(3, 0, false, 1, signinRspMsgMatcher->get(), DefaultAction::get());
    endpoint->expectSend(4, 0, false, 1, createTestResponseFullMatcher.get(), DefaultAction::get());
    endpoint->expectSend(5, 0, false, 1, createValueResponseFullMatcher.get(), DefaultAction::get());
    endpoint->expectSend(6, 0, false, 1, subscribeValueResponseOkMatcher.get(), subscribeValueRspAction);
    endpoint->expectSend(7, 0, false, 1, valueUpdateMatcher.get(), valueUpdateAction);
    endpoint->expectSend(8, 0, false, 1, getValueResponseFullMatcher.get(), DefaultAction::get());

    server->setup();
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(500ms);
    endpoint->waitForAllSending(10000.0);
    server->teardown();

    logger::loggerServer.waitEmpty();
}

TEST_F(ClientServerTests, shouldForwardRcpRequestToExecutor)
{
    endpoint->queueToReceive(createRpcTestMessage);

    auto expectedParam = utils::buildSharedBufferedValue(6969);

    std::function<void()> rpcCreationAction = [this, &expectedParam]()
    {
        auto uuid = this->rpcCreationMatcher->getUuidOfLastMatched(); 
        log << logger::DEBUG << "Requesting Rpc to uuid: " << uuid;
        this->endpoint->queueToReceive(createRpcRequestMessage(createRpcRequestTid, uuid, *expectedParam));
        handleRpcRequestMatcher.set(
            createHandleRpcRequestMessage(static_cast<uint32_t>(-1), (uintptr_t)server.get(), createRpcRequestTid,
                uuid, *expectedParam));
    };

    endpoint->expectSend(1, 0, true, 1, rpcCreationMatcher->get(), rpcCreationAction);
    endpoint->expectSend(2, 0, true, 1, handleRpcRequestMatcher.get(), DefaultAction::get());

    server->setup();
    endpoint->waitForAllSending(500.0);
    server->teardown();

    logger::loggerServer.waitEmpty();
}

TEST_F(ClientServerTests, shouldForwardRcpResponseToCaller)
{
    endpoint->queueToReceive(createRpcTestMessage);

    auto expectedParam = utils::buildSharedBufferedValue(6969);
    rpcResponseMatcher.set(createRpcResponseMessage(createRpcRequestTid, *expectedParam));
    protocol::Uuid uuid = 0;

    std::function<void()> rpcCreationAction = [this, &expectedParam, &uuid]()
    {
        uuid = this->rpcCreationMatcher->getUuidOfLastMatched(); 
        log << logger::DEBUG << "Requesting Rpc to uuid: " << uuid;
        this->endpoint->queueToReceive(createRpcRequestMessage(createRpcRequestTid, uuid, *expectedParam));
        handleRpcRequestMatcher.set(
            createHandleRpcRequestMessage(static_cast<uint32_t>(-1), (uintptr_t)server.get(), createRpcRequestTid,
                uuid, *expectedParam));
    };

    std::function<void()> handleRpcRequestAction = [this, &expectedParam]()
    {
        log << logger::DEBUG << "Sending HandleRpcResponse ";
        this->endpoint->queueToReceive(
            createHandleRpcResponseMessage(static_cast<uint32_t>(-1), (uintptr_t)server.get(), createRpcRequestTid, *expectedParam));
    };

    endpoint->expectSend(1, 0, true, 1, rpcCreationMatcher->get(), rpcCreationAction);
    endpoint->expectSend(2, 0, true, 1, handleRpcRequestMatcher.get(), handleRpcRequestAction);
    endpoint->expectSend(3, 0, true, 1, rpcResponseMatcher.get(), DefaultAction::get());

    server->setup();
    endpoint->waitForAllSending(500.0);
    server->teardown();

    logger::loggerServer.waitEmpty();
}

// /** TODO: For reference only. new test will be created for new  GetSpecificMeta **/
// // TEST_F(ClientServerTests, shouldSigninRequestAndRespondWithMeta)
// // {
// //     auto fcs =  ptree->createProperty<core::Node>("/FCS"); // 100
// //     auto sens = ptree->createProperty<core::Node>("/SENSOR"); // 101
// //     auto aile = ptree->createProperty<core::Node>("/FCS/AILERON"); // 102
// //     auto acel = ptree->createProperty<core::Node>("/SENSOR/ACCELEROMETER"); // 103
// //     auto ther = ptree->createProperty<core::Node>("/SENSOR/THERMOMETER"); // 104
// //     auto val1 = ptree->createProperty<core::Value>("/SENSOR/THERMOMETER/VALUE"); // 105
// //     auto val2 = ptree->createProperty<core::Value>("/SENSOR/ACCELEROMETER/VALUE"); // 106
// //     auto val3 = ptree->createProperty<core::Value>("/FCS/AILERON/CURRENT_DEFLECTION"); // 107
// //     auto val4 = ptree->createProperty<core::Value>("/FCS/AILERON/TRIM"); //108

// //     std::list<std::tuple<std::string, protocol::Uuid, protocol::PropertyType>> metalist;

// //     metalist.emplace_back("/FCS", protocol::Uuid(100), protocol::PropertyType::Node);
// //     metalist.emplace_back("/FCS/AILERON", protocol::Uuid(102), protocol::PropertyType::Node);
// //     metalist.emplace_back("/FCS/AILERON/CURRENT_DEFLECTION", protocol::Uuid(107), protocol::PropertyType::Value);
// //     metalist.emplace_back("/FCS/AILERON/TRIM", protocol::Uuid(108), protocol::PropertyType::Value);
// //     metalist.emplace_back("/SENSOR", protocol::Uuid(101), protocol::PropertyType::Node);
// //     metalist.emplace_back("/SENSOR/ACCELEROMETER", protocol::Uuid(103), protocol::PropertyType::Node);
// //     metalist.emplace_back("/SENSOR/ACCELEROMETER/VALUE", protocol::Uuid(106), protocol::PropertyType::Value);
// //     metalist.emplace_back("/SENSOR/THERMOMETER", protocol::Uuid(104), protocol::PropertyType::Node);
// //     metalist.emplace_back("/SENSOR/THERMOMETER/VALUE", protocol::Uuid(105), protocol::PropertyType::Value);

// //     signinRspMsgMatcher = std::make_shared<MessageMatcher>(createSigninResponseMessage(signinRqstTid, 1, metalist));

// //     endpoint->expectSend(0, 0, false, 1, signinRspMsgMatcher->get(), DefaultAction::get());

// //     using namespace std::chrono_literals;
// //     std::this_thread::sleep_for(1ms);
// //     server->setup();
// //     endpoint->waitForAllSending(2500.0);
// //     server->teardown();
// // }

// TEST_F(ClientServerTests, shouldGetSpecificMetaRequestAndRespondTheCorrectMeta)
// {
//     ptree->createProperty<core::Node>("/FCS"); // 100
//     ptree->createProperty<core::Node>("/FCS/AILERON"); // 101
//     ptree->createProperty<core::Value>("/FCS/AILERON/CURRENT_DEFLECTION"); // 102

//     std::string path = "/FCS/AILERON/CURRENT_DEFLECTION";

//     endpoint->queueToReceive(createGetSpecificMetaRequestMessage(1, path));
//     MessageMatcher getSpecifiMetaResponsetMessageMatcher(
//         createGetSpecificMetaResponseMessage(1, 102, protocol::PropertyType::Value, path));

//     endpoint->expectSend(0, 0, false, 1, getSpecifiMetaResponsetMessageMatcher.get(), DefaultAction::get());

//     using namespace std::chrono_literals;
//     std::this_thread::sleep_for(1ms);
//     server->setup();
//     endpoint->waitForAllSending(2500.0);
//     server->teardown();
// }

} // namespace server
} // namespace ptree
