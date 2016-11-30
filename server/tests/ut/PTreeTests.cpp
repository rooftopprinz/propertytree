#include <cstring>
#include <cstdlib>
#include <ctime>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <server/src/PTree.hpp>
#include <server/src/PTreeTcpServer.hpp>

using namespace testing;

namespace ptree
{
namespace core
{

struct PTreeTests : public ::testing::Test
{

    void TearDown()
    {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(1ms);
    }

    ValueContainer makeValue(void* value, size_t size)
    {
        ValueContainer container;
        container.resize(size);
        std::memcpy(container.data(), value, size);
        return container;
    }

    template<class T>
    T* reconstructValue(void* value)
    {
        return reinterpret_cast<T*>(value);
    }

};

template <typename T>
class ValueSetGetTest : public ::testing::Test {
  
};

TYPED_TEST_CASE_P(ValueSetGetTest);

TYPED_TEST_P(ValueSetGetTest, shouldSetGetNative) {
    auto timeValue = time(NULL);

    // std::cout << "time:      " << timeValue << std::endl;
    srand(timeValue);
    double randomVal = (rand()/1024.1)*(timeValue%2?-1:1);

    // std::cout << "random:    " << randomVal << std::endl;
    TypeParam tval = static_cast<TypeParam>(randomVal);
    // std::cout << "converted: " << tval << std::endl;

    Value value;

    value.setValue(tval);
    EXPECT_EQ(value.getValue<TypeParam>(), tval);

}

REGISTER_TYPED_TEST_CASE_P(ValueSetGetTest, shouldSetGetNative);

typedef ::testing::Types<uint32_t, uint64_t, int32_t, int64_t, double, float> MyTypes;
INSTANTIATE_TYPED_TEST_CASE_P(ValueSetGetTest, ValueSetGetTest, MyTypes);


TEST_F(PTreeTests, shouldSetGetBuffered)
{
    struct TestStruct
    {
        int a;
        double b;
        char c[4];
    };

    TestStruct testvalue;
    testvalue.a = 42;
    testvalue.b = 42.0;
    testvalue.c[0] = 'a';
    testvalue.c[1] = 'b';
    testvalue.c[2] = 'c';
    testvalue.c[3] = 0;

    ValueContainer data = makeValue(&testvalue, sizeof(testvalue));

    Value value;

    value.setValue(testvalue);

    const ValueContainer& testget = value.getValue();

    EXPECT_EQ(testvalue.a, reinterpret_cast<const TestStruct*>(testget.data())->a);
    EXPECT_EQ(testvalue.b, reinterpret_cast<const TestStruct*>(testget.data())->b);
}

struct WatcherMock
{
    MOCK_METHOD1(watch, bool(ValueWkPtr));
};

TEST_F(PTreeTests, shouldCallWatcherWhenModified)
{
    using std::placeholders::_1;
    using ::testing::DoAll;

    WatcherMock watcher;
    ValueWatcher watcherfn = std::bind(&WatcherMock::watch, &watcher, _1);

    ValuePtr value = std::make_shared<Value>();
    value->setUuid(42);
    ValueWkPtr container;

    EXPECT_CALL(watcher, watch(_))
        .WillOnce(DoAll(SaveArg<0>(&container), Return(true)));

    value->addWatcher((void*)1, watcherfn);
    value->setValue<uint32_t>(420u);

    EXPECT_EQ(*reconstructValue<uint32_t>(container.lock()->getValue().data()), 420u);
}

TEST_F(PTreeTests, shouldNotCallWatcherWhenRemoved)
{
    using std::placeholders::_1;
    WatcherMock watcher1;
    WatcherMock watcher2;
    ValueWatcher watcher1fn = std::bind(&WatcherMock::watch, &watcher1, _1); 
    ValueWatcher watcher2fn = std::bind(&WatcherMock::watch, &watcher2, _1); 

    ValuePtr value = std::make_shared<Value>();
    value->setUuid(42);

    EXPECT_CALL(watcher1, watch(_))
        .Times(2)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(watcher2, watch(_))
        .Times(1)
        .WillRepeatedly(Return(true));

    value->addWatcher((void*)1, watcher1fn);
    value->addWatcher((void*)2, watcher2fn);
    value->setValue<uint32_t>(420u);
    value->removeWatcher((void*)2);
    value->setValue<uint32_t>(200u);

}

TEST_F(PTreeTests, shouldNotAddSameId)
{
    using std::placeholders::_1;
    WatcherMock watcher1;
    ValueWatcher watcher1fn = std::bind(&WatcherMock::watch, &watcher1, _1); 
    Value value;
    value.setUuid(42);

    EXPECT_TRUE(value.addWatcher((void*)1, watcher1fn));
    EXPECT_FALSE(value.addWatcher((void*)1, watcher1fn));
}

TEST_F(PTreeTests, shouldNotRemoveIfNone)
{
    using std::placeholders::_1;
    WatcherMock watcher1;
    ValueWatcher watcher1fn = std::bind(&WatcherMock::watch, &watcher1, _1); 
    ValuePtr value = std::make_shared<Value>();
    value->setUuid(42);

    EXPECT_TRUE(value->addWatcher((void*)1, watcher1fn));
    EXPECT_TRUE(value->addWatcher((void*)2, watcher1fn));
    EXPECT_TRUE(value->removeWatcher((void*)2));
    EXPECT_FALSE(value->removeWatcher((void*)2));
}

TEST_F(PTreeTests, shouldCreateAndGetNode)
{
    using std::placeholders::_1;
    NodePtr root = std::make_shared<Node>();

    auto node1 = root->createProperty<Node>("parent");
    auto node2 = node1->createProperty<Node>("children");
    auto valu1 = node1->createProperty<Value>("value1");
    auto valu2 = node2->createProperty<Value>("value2");

    auto gNode1 = root->getProperty<Node>("parent");
    auto gNode2 = node1->getProperty<Node>("children");
    auto gValu1 = node1->getProperty<Value>("value1");
    auto gValu2 = node2->getProperty<Value>("value2");

    auto test3 = root->getProperty<Value>("parent");
    auto test4 = root->getProperty<Node>("parent");

    EXPECT_EQ(node1, gNode1);
    EXPECT_EQ(node2, gNode2);
    EXPECT_EQ(valu1, gValu1);
    EXPECT_EQ(valu2, gValu2);
}


TEST_F(PTreeTests, getNodeByPath)
{
    using std::placeholders::_1;
    IIdGeneratorPtr idgen = std::make_shared<IdGenerator>();
    PTree ptree(idgen);

    auto root = ptree.getNodeByPath("/");
    auto fGen = root->createProperty<Node>("FCS");
    auto sGen = fGen->createProperty<Node>("AILERON");
    auto val = sGen->createProperty<Value>("VALUE");
    val->setValue<uint32_t>(420);

    auto gnode = ptree.getNodeByPath("/FCS/AILERON");
    auto val2 = gnode->getProperty<Value>("VALUE");

    ptree.getPTreeInfo();
    EXPECT_EQ(val, val2);
}

TEST_F(PTreeTests, getPTreeInfo)
{
    using std::placeholders::_1;
    IIdGeneratorPtr idgen = std::make_shared<IdGenerator>();
    PTree ptree(idgen);

    auto root = ptree.getNodeByPath("/");
    auto fcs = root->createProperty<Node>("FCS");
    auto sens = root->createProperty<Node>("SENSOR");
    auto aile = fcs->createProperty<Node>("AILERON");
    auto acel = sens->createProperty<Node>("ACCELEROMETER");
    auto ther = sens->createProperty<Node>("THERMOMETER");
    aile->createProperty<Value>("VALUE");
    aile->createProperty<Value>("OTHERVALUE");
    acel->createProperty<Value>("VALUE");
    ther->createProperty<Value>("VALUE");

    ptree.getPTreeInfo();
}

TEST_F(PTreeTests, getPropertyByPath)
{
    using std::placeholders::_1;    
    IIdGeneratorPtr idgen = std::make_shared<IdGenerator>();
    PTree ptree(idgen);

    auto root = ptree.getNodeByPath("/");
    auto fGen = root->createProperty<Node>("FCS");
    auto sGen = fGen->createProperty<Node>("AILERON");
    auto val = sGen->createProperty<Value>("VALUE");
    val->setValue<uint32_t>(420);

    auto val2 = ptree.getPropertyByPath<Value>("/FCS/AILERON/VALUE");
    EXPECT_EQ(val, val2);
}

} // namespace core
} // namespace ptree