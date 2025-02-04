#include "test_base.h"
#include <testutils/memcheck_listener.h>

#include <opendaq/opendaq.h>
#include <opendaq/deserialize_component_ptr.h>
#include <opendaq/component_deserialize_context_factory.h>

#include <memory>
#include <future>

using namespace daq;
using namespace daq::opendaq_native_streaming_protocol;

static SignalPtr deserializeSignal(const ContextPtr& context, const StringPtr& serializedSignal)
{
    const auto deserializer = JsonDeserializer();
    const auto deserializeContext = ComponentDeserializeContext(context, nullptr, nullptr, "sig");

    const SignalPtr signal = deserializer.deserialize(serializedSignal, deserializeContext, nullptr);
    return signal;
}

static StringPtr getDomainSignalId(const SignalPtr& signal)
{
    return signal.asPtr<IDeserializeComponent>(true).getDeserializedParameter("domainSignalId");
}

class StreamingProtocolAttributes : public ClientAttributesBase
{
public:
    std::promise< std::tuple<StringPtr, StringPtr> > signalAvailablePromise;
    std::future< std::tuple<StringPtr, StringPtr> > signalAvailableFuture;

    std::promise< std::tuple<StringPtr, StringPtr> > signalWithDomainAvailablePromise;
    std::future< std::tuple<StringPtr, StringPtr> > signalWithDomainAvailableFuture;

    std::promise< StringPtr > signalUnavailablePromise;
    std::future< StringPtr > signalUnavailableFuture;

    std::promise< std::tuple<StringPtr, PacketPtr> > packetReceivedPromise;
    std::future< std::tuple<StringPtr, PacketPtr> > packetReceivedFuture;

    std::promise< StringPtr > subscribedAckPromise;
    std::future< StringPtr > subscribedAckFuture;

    std::promise< StringPtr > unsubscribedAckPromise;
    std::future< StringPtr > unsubscribedAckFuture;

    std::promise< ClientReconnectionStatus > reconnectionStatusPromise;
    std::future< ClientReconnectionStatus > reconnectionStatusFuture;

    OnSignalAvailableCallback signalAvailableHandler;
    OnSignalAvailableCallback signalWithDomainAvailableHandler;
    OnSignalUnavailableCallback signalUnavailableHandler;
    OnPacketCallback packetHandler;
    OnSignalSubscriptionAckCallback signalSubscriptionAckHandler;
    OnReconnectionStatusChangedCallback reconnectionStatusChangedHandler;

    void setUp()
    {
        ClientAttributesBase::setUp();

        signalAvailablePromise = std::promise< std::tuple<StringPtr, StringPtr> >();
        signalAvailableFuture = signalAvailablePromise.get_future();

        signalWithDomainAvailablePromise = std::promise< std::tuple<StringPtr, StringPtr> >();
        signalWithDomainAvailableFuture = signalWithDomainAvailablePromise.get_future();

        signalUnavailablePromise = std::promise< StringPtr >();
        signalUnavailableFuture = signalUnavailablePromise.get_future();

        packetReceivedPromise = std::promise< std::tuple<StringPtr, PacketPtr> >();
        packetReceivedFuture = packetReceivedPromise.get_future();

        subscribedAckPromise = std::promise< StringPtr >();
        subscribedAckFuture = subscribedAckPromise.get_future();

        unsubscribedAckPromise = std::promise< StringPtr >();
        unsubscribedAckFuture = unsubscribedAckPromise.get_future();

        reconnectionStatusPromise = std::promise< ClientReconnectionStatus >();
        reconnectionStatusFuture = reconnectionStatusPromise.get_future();

        signalAvailableHandler = [this](const StringPtr& signalStringId,
                                        const StringPtr& serializedSignal)
        {
            signalAvailablePromise.set_value({signalStringId, serializedSignal});
        };

        signalWithDomainAvailableHandler = [this](const StringPtr& signalStringId,
                                                  const StringPtr& serializedSignal)
        {
            signalWithDomainAvailablePromise.set_value({signalStringId, serializedSignal});
        };

        signalUnavailableHandler = [this](const StringPtr& signalStringId)
        {
            signalUnavailablePromise.set_value(signalStringId);
        };

        packetHandler = [this](const StringPtr& signalStringId, const PacketPtr& packet)
        {
            packetReceivedPromise.set_value({signalStringId, packet});
        };

        signalSubscriptionAckHandler = [this](const StringPtr& signalStringId, bool subscribed)
        {
            if (subscribed)
                subscribedAckPromise.set_value(signalStringId);
            else
                unsubscribedAckPromise.set_value(signalStringId);
        };

        reconnectionStatusChangedHandler = [this](ClientReconnectionStatus status)
        {
            reconnectionStatusPromise.set_value(status);
        };
    }

    void tearDown()
    {
        ClientAttributesBase::tearDown();
    }
};

class StreamingProtocolTest : public ProtocolTestBase
{
public:
    void SetUp() override
    {
        ProtocolTestBase::SetUp();

        signalSubscribedPromise = std::promise< SignalPtr >();
        signalSubscribedFuture = signalSubscribedPromise.get_future();
        signalSubscribedHandler =
            [this](const SignalPtr& signal)
        {
            signalSubscribedPromise.set_value(signal);
        };

        signalUnsubscribedPromise = std::promise< SignalPtr >();
        signalUnsubscribedFuture = signalUnsubscribedPromise.get_future();
        signalUnsubscribedHandler =
            [this](const SignalPtr& signal)
        {
            signalUnsubscribedPromise.set_value(signal);
        };

        setUpConfigProtocolServerCb = [](ConfigProtocolPacketCb sendPacketCb)
        {
            return [](const config_protocol::PacketBuffer& packetBuffer) {};
        };

        clientsCount = std::get<0>(GetParam());
        clients = std::vector<StreamingProtocolAttributes>(clientsCount);
        for (size_t i = 0; i < clients.size(); ++i)
        {
            clients[i].setUp();
        }
    }

    void TearDown() override
    {
        if (std::get<1>(GetParam())) // stop server first
        {
            stopServer();
            for (auto& client : clients)
                client.tearDown();
        }
        else // stop clients first
        {
            for (auto& client : clients)
                client.tearDown();
            stopServer();
        }
        ProtocolTestBase::TearDown();
    }

    std::shared_ptr<NativeStreamingClientHandler> createClient(StreamingProtocolAttributes& client,
                                                               OnSignalAvailableCallback signalAvailableHandler)
    {
        auto clientHandler = std::make_shared<NativeStreamingClientHandler>(
            client.clientContext, ClientAttributesBase::createTransportLayerConfig());

        clientHandler->setIoContext(client.ioContextPtrClient);
        clientHandler->setSignalAvailableHandler(signalAvailableHandler);
        clientHandler->setSignalUnavailableHandler(client.signalUnavailableHandler);
        clientHandler->setPacketHandler(client.packetHandler);
        clientHandler->setSignalSubscriptionAckCallback(client.signalSubscriptionAckHandler);
        clientHandler->setReconnectionStatusChangedCb(client.reconnectionStatusChangedHandler);

        return clientHandler;
    }

    void startServer(const ListPtr<ISignal>& signalsList)
    {
        startIoOperations();
        serverHandler = std::make_shared<NativeStreamingServerHandler>(serverContext,
                                                                       ioContextPtrServer,
                                                                       signalsList,
                                                                       signalSubscribedHandler,
                                                                       signalUnsubscribedHandler,
                                                                       setUpConfigProtocolServerCb);
        serverHandler->startServer(NATIVE_STREAMING_SERVER_PORT);
    }

    void stopServer()
    {
        stopIoOperations();
        serverHandler.reset();
    }

protected:
    std::vector<StreamingProtocolAttributes> clients;

    OnSignalSubscribedCallback signalSubscribedHandler;
    OnSignalUnsubscribedCallback signalUnsubscribedHandler;

    std::promise< SignalPtr > signalSubscribedPromise;
    std::future< SignalPtr > signalSubscribedFuture;

    std::promise< SignalPtr > signalUnsubscribedPromise;
    std::future< SignalPtr > signalUnsubscribedFuture;

    SetUpConfigProtocolServerCb setUpConfigProtocolServerCb;
};

TEST_P(StreamingProtocolTest, CreateServerNoSignals)
{
    serverHandler = std::make_shared<NativeStreamingServerHandler>(serverContext,
                                                                   ioContextPtrServer,
                                                                   List<ISignal>(),
                                                                   signalSubscribedHandler,
                                                                   signalUnsubscribedHandler,
                                                                   setUpConfigProtocolServerCb);
}

TEST_P(StreamingProtocolTest, CreateClient)
{
    for (auto& client : clients)
    {
        client.clientHandler = createClient(client, client.signalAvailableHandler);
    }
}

TEST_P(StreamingProtocolTest, ClientConnectFailed)
{
    for (auto& client : clients)
    {
        client.clientHandler = createClient(client, client.signalAvailableHandler);
        ASSERT_FALSE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));
    }
}

TEST_P(StreamingProtocolTest, ConnectDisconnectNoSignals)
{
    startServer(List<ISignal>());

    for (auto& client : clients)
    {
        client.clientHandler = createClient(client, client.signalAvailableHandler);
        ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));
    }
}

TEST_P(StreamingProtocolTest, Reconnection)
{
    startServer(List<ISignal>());

    for (auto& client : clients)
    {
        client.clientHandler = createClient(client, client.signalAvailableHandler);
        ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));
    }

    stopServer();

    for (auto& client : clients)
    {
        ASSERT_EQ(client.reconnectionStatusFuture.wait_for(std::chrono::seconds(5)), std::future_status::ready);
        ASSERT_EQ(client.reconnectionStatusFuture.get(), ClientReconnectionStatus::Reconnecting);

        client.reconnectionStatusPromise = std::promise< ClientReconnectionStatus >();
        client.reconnectionStatusFuture = client.reconnectionStatusPromise.get_future();
    }

    startServer(List<ISignal>());

    for (auto& client : clients)
    {
        ASSERT_EQ(client.reconnectionStatusFuture.wait_for(std::chrono::seconds(5)), std::future_status::ready);
        ASSERT_EQ(client.reconnectionStatusFuture.get(), ClientReconnectionStatus::Restored);

        client.reconnectionStatusPromise = std::promise< ClientReconnectionStatus >();
        client.reconnectionStatusFuture = client.reconnectionStatusPromise.get_future();
    }
}

TEST_P(StreamingProtocolTest, ConnectDisconnectWithSignalDomainUnassigned)
{
    auto serverSignal = SignalWithDescriptor(serverContext, DataDescriptorBuilder().setSampleType(SampleType::Undefined).build(), nullptr, "signal");
    serverSignal.setName("signalName");
    serverSignal.setDescription("signalDescription");

    startServer(List<ISignal>(serverSignal));

    for (auto& client : clients)
    {
        client.clientHandler = createClient(client, client.signalWithDomainAvailableHandler);
        ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));

        ASSERT_EQ(client.signalWithDomainAvailableFuture.wait_for(timeout), std::future_status::ready);
        auto [clientSignalStringId, serializedSignal] = client.signalWithDomainAvailableFuture.get();
        SignalPtr clientSignal = deserializeSignal(client.clientContext, serializedSignal);
        ASSERT_EQ(clientSignalStringId, serverSignal.getGlobalId());
        ASSERT_TRUE(!getDomainSignalId(clientSignal).assigned());
        ASSERT_EQ(clientSignal.getDescriptor(), serverSignal.getDescriptor());
        ASSERT_EQ(clientSignal.getName(), serverSignal.getName());
        ASSERT_EQ(clientSignal.getDescription(), serverSignal.getDescription());
    }
}

TEST_P(StreamingProtocolTest, ConnectDisconnectWithSignalDomainAssigned)
{
    auto serverDomainSignal =
        SignalWithDescriptor(serverContext, DataDescriptorBuilder().setSampleType(SampleType::Undefined).build(), nullptr, "domainSignal");
    auto serverSignal =
        SignalWithDescriptor(serverContext, DataDescriptorBuilder().setSampleType(SampleType::Undefined).build(), nullptr, "signal");
    serverSignal.setDomainSignal(serverDomainSignal);
    serverSignal.setName("signalName");
    serverSignal.setDescription("signalDescription");

    startServer(List<ISignal>(serverSignal));

    for (auto& client : clients)
    {
        client.clientHandler = createClient(client, client.signalWithDomainAvailableHandler);
        ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));

        ASSERT_EQ(client.signalWithDomainAvailableFuture.wait_for(timeout), std::future_status::ready);
        auto [clientSignalStringId, serializedSignal] = client.signalWithDomainAvailableFuture.get();
        SignalPtr clientSignal = deserializeSignal(client.clientContext, serializedSignal);
        ASSERT_EQ(clientSignalStringId, serverSignal.getGlobalId());
        ASSERT_EQ(clientSignal.getDescriptor(), serverSignal.getDescriptor());
        ASSERT_EQ(getDomainSignalId(clientSignal), serverDomainSignal.getGlobalId());
        ASSERT_EQ(clientSignal.getName(), serverSignal.getName());
        ASSERT_EQ(clientSignal.getDescription(), serverSignal.getDescription());
    }
}

TEST_P(StreamingProtocolTest, AddSignal)
{
    startServer(List<ISignal>());

    for (auto& client : clients)
    {
        client.clientHandler = createClient(client, client.signalAvailableHandler);
        ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));
    }

    auto serverSignal =
        SignalWithDescriptor(serverContext, DataDescriptorBuilder().setSampleType(SampleType::Undefined).build(), nullptr, "signal");
    serverSignal.setName("signalName");
    serverSignal.setDescription("signalDescription");
    serverHandler->addSignal(serverSignal);

    for (auto& client : clients)
    {
        ASSERT_EQ(client.signalAvailableFuture.wait_for(timeout), std::future_status::ready);
        auto [clientSignalStringId, serializedSignal] = client.signalAvailableFuture.get();
        SignalPtr clientSignal = deserializeSignal(client.clientContext, serializedSignal);
        ASSERT_EQ(clientSignalStringId, serverSignal.getGlobalId());
        ASSERT_EQ(clientSignal.getDescriptor(), serverSignal.getDescriptor());
        ASSERT_EQ(clientSignal.getName(), serverSignal.getName());
        ASSERT_EQ(clientSignal.getDescription(), serverSignal.getDescription());
    }
}

TEST_P(StreamingProtocolTest, RemoveSignal)
{
    auto serverSignal =
        SignalWithDescriptor(serverContext, DataDescriptorBuilder().setSampleType(SampleType::Undefined).build(), nullptr, "signal");

    startServer(List<ISignal>(serverSignal));

    for (auto& client : clients)
    {
        client.clientHandler = createClient(client, client.signalAvailableHandler);
        ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));

        ASSERT_EQ(client.signalAvailableFuture.wait_for(timeout), std::future_status::ready);
        auto [clientSignalStringId, serializedSignal] =
            client.signalAvailableFuture.get();
        ASSERT_EQ(clientSignalStringId, serverSignal.getGlobalId());
    }

    serverHandler->removeComponentSignals(serverSignal.getGlobalId());

    for (auto& client : clients)
    {
        ASSERT_EQ(client.signalUnavailableFuture.wait_for(timeout), std::future_status::ready);
        auto clientSignalStringId = client.signalUnavailableFuture.get();
        ASSERT_EQ(clientSignalStringId, serverSignal.getGlobalId());
    }
}

TEST_P(StreamingProtocolTest, RemoveSignalOfParent)
{
    const auto folder = Folder(serverContext, nullptr, "folder");
    auto serverSignal =
        SignalWithDescriptor(serverContext, DataDescriptorBuilder().setSampleType(SampleType::Undefined).build(), folder, "signal");

    startServer(List<ISignal>(serverSignal));

    for (auto& client : clients)
    {
        client.clientHandler = createClient(client, client.signalAvailableHandler);
        ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));

        ASSERT_EQ(client.signalAvailableFuture.wait_for(timeout), std::future_status::ready);
        auto [clientSignalStringId, serializedSignal] =
            client.signalAvailableFuture.get();
        ASSERT_EQ(clientSignalStringId, serverSignal.getGlobalId());
    }

    serverHandler->removeComponentSignals(folder.getGlobalId());

    for (auto& client : clients)
    {
        ASSERT_EQ(client.signalUnavailableFuture.wait_for(timeout), std::future_status::ready);
        auto clientSignalStringId = client.signalUnavailableFuture.get();
        ASSERT_EQ(clientSignalStringId, serverSignal.getGlobalId());
    }
}

TEST_P(StreamingProtocolTest, SignalSubscribeUnsubscribe)
{
    StringPtr clientSignalStringId, serializedSignal;
    DataDescriptorPtr clientSignalDescriptor;

    auto serverSignal =
        SignalWithDescriptor(serverContext, DataDescriptorBuilder().setSampleType(SampleType::Undefined).build(), nullptr, "signal");
    startServer(List<ISignal>(serverSignal));

    for (auto& client : clients)
    {
        client.clientHandler = createClient(client, client.signalAvailableHandler);
        ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));

        ASSERT_EQ(client.signalAvailableFuture.wait_for(timeout), std::future_status::ready);
        std::tie(clientSignalStringId, serializedSignal) = client.signalAvailableFuture.get();
        client.clientHandler->subscribeSignal(clientSignalStringId);
        ASSERT_EQ(client.subscribedAckFuture.wait_for(timeout), std::future_status::ready);
        ASSERT_EQ(client.subscribedAckFuture.get(), clientSignalStringId);
    }

    ASSERT_EQ(signalSubscribedFuture.wait_for(timeout), std::future_status::ready);
    ASSERT_EQ(signalSubscribedFuture.get(), serverSignal);

    for (auto& client : clients)
    {
        client.clientHandler->unsubscribeSignal(clientSignalStringId);
        ASSERT_EQ(client.unsubscribedAckFuture.wait_for(timeout), std::future_status::ready);
        ASSERT_EQ(client.unsubscribedAckFuture.get(), clientSignalStringId);
    }
    ASSERT_EQ(signalUnsubscribedFuture.wait_for(timeout), std::future_status::ready);
    ASSERT_EQ(signalUnsubscribedFuture.get(), serverSignal);
}

TEST_P(StreamingProtocolTest, RemoveSubscribedSignal)
{
    StringPtr clientSignalStringId, serializedSignal;
    DataDescriptorPtr clientSignalDescriptor;

    auto serverSignal =
        SignalWithDescriptor(serverContext, DataDescriptorBuilder().setSampleType(SampleType::Undefined).build(), nullptr, "signal");
    startServer(List<ISignal>(serverSignal));

    for (auto& client : clients)
    {
        client.clientHandler = createClient(client, client.signalAvailableHandler);
        ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));

        ASSERT_EQ(client.signalAvailableFuture.wait_for(timeout), std::future_status::ready);
        std::tie(clientSignalStringId, serializedSignal) = client.signalAvailableFuture.get();
        client.clientHandler->subscribeSignal(clientSignalStringId);
    }

    ASSERT_EQ(signalSubscribedFuture.wait_for(timeout), std::future_status::ready);
    ASSERT_EQ(signalSubscribedFuture.get(), serverSignal);

    serverHandler->removeComponentSignals(serverSignal.getGlobalId());

    ASSERT_EQ(signalUnsubscribedFuture.wait_for(timeout), std::future_status::ready);
    ASSERT_EQ(signalUnsubscribedFuture.get(), serverSignal);
}

TEST_P(StreamingProtocolTest, InitialEventPacket)
{
    const auto valueDescriptor = DataDescriptorBuilder().setSampleType(SampleType::Float32).build();
    auto initialEventPacket = DataDescriptorChangedEventPacket(valueDescriptor, nullptr);
    auto serverSignal = SignalWithDescriptor(serverContext, valueDescriptor, nullptr, "signal");

    startServer(List<ISignal>(serverSignal));

    for (auto& client : clients)
    {
        client.clientHandler = createClient(client, client.signalAvailableHandler);
        ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));

        ASSERT_EQ(client.packetReceivedFuture.wait_for(timeout), std::future_status::ready);
        auto [signalId, packet] = client.packetReceivedFuture.get();
        ASSERT_EQ(signalId, serverSignal.getGlobalId());
        ASSERT_EQ(packet, initialEventPacket);
    }
}

TEST_P(StreamingProtocolTest, SendEventPacket)
{
    const auto valueDescriptor = DataDescriptorBuilder().setSampleType(SampleType::Float32).build();
    auto initialEventPacket = DataDescriptorChangedEventPacket(valueDescriptor, nullptr);
    auto serverSignal = SignalWithDescriptor(serverContext, valueDescriptor, nullptr, "signal");

    startServer(List<ISignal>(serverSignal));

    for (auto& client : clients)
    {
        client.clientHandler = createClient(client, client.signalAvailableHandler);
        ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));

        ASSERT_EQ(client.signalAvailableFuture.wait_for(timeout), std::future_status::ready);
        auto [clientSignalStringId, serializedSignal] =
            client.signalAvailableFuture.get();

        // wait for initial event packet
        ASSERT_EQ(client.packetReceivedFuture.wait_for(timeout), std::future_status::ready);
        // reset packet future / promise
        client.packetReceivedPromise = std::promise< std::tuple<StringPtr, PacketPtr> >();
        client.packetReceivedFuture = client.packetReceivedPromise.get_future();

        client.clientHandler->subscribeSignal(clientSignalStringId);
        ASSERT_EQ(client.subscribedAckFuture.wait_for(timeout), std::future_status::ready);
    }

    ASSERT_EQ(signalSubscribedFuture.wait_for(timeout), std::future_status::ready);

    const auto newValueDescriptor = DataDescriptorBuilder().setSampleType(SampleType::Binary).build();
    auto serverEventPacket = DataDescriptorChangedEventPacket(newValueDescriptor, nullptr);
    serverHandler->sendPacket(serverSignal, serverEventPacket);

    for (auto& client : clients)
    {
        ASSERT_EQ(client.packetReceivedFuture.wait_for(timeout), std::future_status::ready);
        auto [signalId, packet] = client.packetReceivedFuture.get();
        ASSERT_EQ(signalId, serverSignal.getGlobalId());
        ASSERT_EQ(packet, serverEventPacket);
    }
}

TEST_P(StreamingProtocolTest, SendPacketsNoSubscribers)
{
    const auto valueDescriptor = DataDescriptorBuilder().setSampleType(SampleType::Float32).build();
    auto serverSignal = SignalWithDescriptor(serverContext, valueDescriptor, nullptr, "signal");

    startServer(List<ISignal>(serverSignal));

    for (auto& client : clients)
    {
        client.clientHandler = createClient(client, client.signalAvailableHandler);
        ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));

        // wait for initial event packet
        ASSERT_EQ(client.packetReceivedFuture.wait_for(timeout), std::future_status::ready);

        // reset packet future / promise
        client.packetReceivedPromise = std::promise< std::tuple<StringPtr, PacketPtr> >();
        client.packetReceivedFuture = client.packetReceivedPromise.get_future();
    }

    auto serverEventPacket = DataDescriptorChangedEventPacket(valueDescriptor, nullptr);
    auto serverDataPacket = DataPacket(valueDescriptor, 100);

    serverHandler->sendPacket(serverSignal, serverEventPacket);
    serverHandler->sendPacket(serverSignal, serverDataPacket);

    // no subscribers - so packets wont be sent to clients
    for (auto& client : clients)
    {
        ASSERT_EQ(client.packetReceivedFuture.wait_for(timeout), std::future_status::timeout);
    }
}

TEST_P(StreamingProtocolTest, SendDataPacket)
{
    const auto valueDescriptor = DataDescriptorBuilder().setSampleType(SampleType::Float32).build();
    auto serverDataPacket = DataPacket(valueDescriptor, 100);
    auto serverSignal = SignalWithDescriptor(serverContext, valueDescriptor, nullptr, "signal");

    startServer(List<ISignal>(serverSignal));

    for (auto& client : clients)
    {
        client.clientHandler = createClient(client, client.signalAvailableHandler);
        ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));

        ASSERT_EQ(client.signalAvailableFuture.wait_for(timeout), std::future_status::ready);
        auto [clientSignalStringId, serializedSignal] =
            client.signalAvailableFuture.get();

        // wait for initial event packet
        ASSERT_EQ(client.packetReceivedFuture.wait_for(timeout), std::future_status::ready);
        // reset packet future / promise
        client.packetReceivedPromise = std::promise< std::tuple<StringPtr, PacketPtr> >();
        client.packetReceivedFuture = client.packetReceivedPromise.get_future();

        client.clientHandler->subscribeSignal(clientSignalStringId);
        ASSERT_EQ(client.subscribedAckFuture.wait_for(timeout), std::future_status::ready);
    }

    ASSERT_EQ(signalSubscribedFuture.wait_for(timeout), std::future_status::ready);

    serverHandler->sendPacket(serverSignal, serverDataPacket);

    for (auto& client : clients)
    {
        ASSERT_EQ(client.packetReceivedFuture.wait_for(timeout), std::future_status::ready);
        auto [signalId, packet] = client.packetReceivedFuture.get();
        ASSERT_EQ(signalId, serverSignal.getGlobalId());
        ASSERT_EQ(packet, serverDataPacket);
    }
}

TEST_P(StreamingProtocolTest, AddNotPublicSignal)
{
    startServer(List<ISignal>());

    for (auto& client : clients)
    {
        client.clientHandler = createClient(client, client.signalAvailableHandler);
        ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));
    }

    auto serverSignal =
        SignalWithDescriptor(serverContext, DataDescriptorBuilder().setSampleType(SampleType::Undefined).build(), nullptr, "signal");
    serverSignal.setName("signalName");
    serverSignal.setDescription("signalDescription");
    serverSignal.setPublic(false);
    serverHandler->addSignal(serverSignal);

    for (auto& client : clients)
    {
        ASSERT_EQ(client.signalAvailableFuture.wait_for(std::chrono::milliseconds(100)), std::future_status::timeout);
    }
}

TEST_P(StreamingProtocolTest, AddNotPublicSignalInConstructor)
{
    auto serverSignal =
        SignalWithDescriptor(serverContext, DataDescriptorBuilder().setSampleType(SampleType::Undefined).build(), nullptr, "signal");
    serverSignal.setName("signalName");
    serverSignal.setDescription("signalDescription");
    serverSignal.setPublic(false);

    startServer(List<ISignal>(serverSignal));

    for (auto& client : clients)
    {
        client.clientHandler = createClient(client, client.signalAvailableHandler);
        ASSERT_TRUE(client.clientHandler->connect(SERVER_ADDRESS, NATIVE_STREAMING_LISTENING_PORT));

        ASSERT_EQ(client.signalAvailableFuture.wait_for(std::chrono::milliseconds(100)), std::future_status::timeout);
    }
}

INSTANTIATE_TEST_SUITE_P(
    ProtocolTestGroup,
    StreamingProtocolTest,
    testing::Values(std::make_tuple(1, true),
                    std::make_tuple(1, false),
                    std::make_tuple(4, true),
                    std::make_tuple(4, false))
);
