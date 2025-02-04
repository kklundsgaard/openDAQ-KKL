#include <native_streaming_server_module/native_streaming_server_impl.h>
#include <coretypes/impl.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>
#include <opendaq/server_type_factory.h>
#include <opendaq/device_private.h>
#include <opendaq/streaming_info_factory.h>
#include <opendaq/reader_factory.h>
#include <opendaq/search_filter_factory.h>
#include <opendaq/custom_log.h>
#include <opendaq/event_packet_ids.h>

#include <native_streaming_protocol/native_streaming_server_handler.h>
#include <config_protocol/config_protocol_server.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_SERVER_MODULE

using namespace daq;
using namespace opendaq_native_streaming_protocol;

NativeStreamingServerImpl::NativeStreamingServerImpl(DevicePtr rootDevice, PropertyObjectPtr config, const ContextPtr& context)
    : Server(config, rootDevice, context, nullptr)
    , readThreadActive(false)
    , readThreadSleepTime(std::chrono::milliseconds(20))
    , ioContextPtr(std::make_shared<boost::asio::io_context>())
    , workGuard(ioContextPtr->get_executor())
    , logger(context.getLogger())
    , loggerComponent(logger.getOrAddComponent("NativeStreamingServerImpl"))
{
    startAsyncOperations();

    prepareServerHandler();
    const uint16_t port = config.getPropertyValue("NativeStreamingPort");
    serverHandler->startServer(port);

    StreamingInfoConfigPtr streamingInfo = StreamingInfo("daq.ns");
    streamingInfo.addProperty(IntProperty("Port", port));
    ErrCode errCode = this->rootDevice.asPtr<IDevicePrivate>()->addStreamingOption(streamingInfo);
    checkErrorInfo(errCode);

    this->context.getOnCoreEvent() += event(&NativeStreamingServerImpl::coreEventCallback);

    startReading();
}

NativeStreamingServerImpl::~NativeStreamingServerImpl()
{
    this->context.getOnCoreEvent() -= event(&NativeStreamingServerImpl::coreEventCallback);
    stopReading();
    stopAsyncOperations();
}

void NativeStreamingServerImpl::componentAdded(ComponentPtr& /*sender*/, CoreEventArgsPtr& eventArgs)
{
    ComponentPtr addedComponent = eventArgs.getParameters().get("Component");

    auto deviceGlobalId = rootDevice.getGlobalId().toStdString();
    auto addedComponentGlobalId = addedComponent.getGlobalId().toStdString();
    if (addedComponentGlobalId.find(deviceGlobalId) != 0)
        return;

    LOG_I("Added Component: {};", addedComponentGlobalId);

    if (addedComponent.supportsInterface<ISignal>())
    {
        serverHandler->addSignal(addedComponent.asPtr<ISignal>(true));
    }
    else if (addedComponent.supportsInterface<IFolder>())
    {
        auto nestedComponents = addedComponent.asPtr<IFolder>().getItems(search::Recursive(search::Any()));
        for (const auto& nestedComponent : nestedComponents)
            if (nestedComponent.supportsInterface<ISignal>())
            {
                LOG_I("Added Signal: {};", nestedComponent.getGlobalId());
                serverHandler->addSignal(nestedComponent.asPtr<ISignal>(true));
            }
    }
}

void NativeStreamingServerImpl::componentRemoved(ComponentPtr& sender, CoreEventArgsPtr& eventArgs)
{
    StringPtr removedComponentLocalId = eventArgs.getParameters().get("Id");

    auto deviceGlobalId = rootDevice.getGlobalId().toStdString();
    auto removedComponentGlobalId =
        sender.getGlobalId().toStdString() + "/" + removedComponentLocalId.toStdString();
    if (removedComponentGlobalId.find(deviceGlobalId) != 0)
        return;

    LOG_I("Component: {}; is removed", removedComponentGlobalId);
    serverHandler->removeComponentSignals(removedComponentGlobalId);
}

void NativeStreamingServerImpl::coreEventCallback(ComponentPtr& sender, CoreEventArgsPtr& eventArgs)
{
    switch (static_cast<CoreEventId>(eventArgs.getEventId()))
    {
        case CoreEventId::ComponentAdded:
            componentAdded(sender, eventArgs);
            break;
        case CoreEventId::ComponentRemoved:
            componentRemoved(sender, eventArgs);
            break;
        default:
            break;
    }
}

void NativeStreamingServerImpl::startAsyncOperations()
{
    ioThread = std::thread([this]()
                           {
                               ioContextPtr->run();
                               LOG_I("IO thread finished");
                           });
}

void NativeStreamingServerImpl::stopAsyncOperations()
{
    ioContextPtr->stop();
    if (ioThread.joinable())
    {
        ioThread.join();
        LOG_I("IO thread joined");
    }
}

void NativeStreamingServerImpl::prepareServerHandler()
{
    auto signalSubscribedHandler = [this](const SignalPtr& signal)
    {
        std::scoped_lock lock(readersSync);
        addReader(signal);
    };
    auto signalUnsubscribedHandler = [this](const SignalPtr& signal)
    {
        std::scoped_lock lock(readersSync);
        removeReader(signal);
    };

    // The Callback establishes a new native configuration server for each connected client
    // and transfers ownership of the configuration server to the transport layer session
    SetUpConfigProtocolServerCb createConfigServerCb =
        [rootDevice = rootDevice](ConfigProtocolPacketCb sendConfigPacketCb)
    {
        auto configServer = std::make_shared<config_protocol::ConfigProtocolServer>(rootDevice, sendConfigPacketCb);
        ConfigProtocolPacketCb processConfigRequestCb =
            [configServer, sendConfigPacketCb](const config_protocol::PacketBuffer& packetBuffer)
        {
            auto replyPacketBuffer = configServer->processRequestAndGetReply(packetBuffer);
            sendConfigPacketCb(replyPacketBuffer);
        };
        return processConfigRequestCb;
    };

    serverHandler = std::make_shared<NativeStreamingServerHandler>(context,
                                                                   ioContextPtr,
                                                                   rootDevice.getSignals(search::Recursive(search::Any())),
                                                                   signalSubscribedHandler,
                                                                   signalUnsubscribedHandler,
                                                                   createConfigServerCb);
}

PropertyObjectPtr NativeStreamingServerImpl::createDefaultConfig()
{
    constexpr Int minPortValue = 0;
    constexpr Int maxPortValue = 65535;

    auto defaultConfig = PropertyObject();

    const auto websocketPortProp = IntPropertyBuilder("NativeStreamingPort", 7420)
        .setMinValue(minPortValue)
        .setMaxValue(maxPortValue)
        .build();
    defaultConfig.addProperty(websocketPortProp);

    return defaultConfig;
}

ServerTypePtr NativeStreamingServerImpl::createType()
{
    auto configurationCallback = [](IBaseObject* input, IBaseObject** output) -> ErrCode
    {
        PropertyObjectPtr propObjPtr;
        ErrCode errCode = wrapHandlerReturn(&NativeStreamingServerImpl::createDefaultConfig, propObjPtr);
        *output = propObjPtr.detach();
        return errCode;
    };

    return ServerType(
        "openDAQ Native Streaming",
        "openDAQ Native Streaming server",
        "Publishes device structure over openDAQ native configuration protocol and streams data over openDAQ native streaming protocol",
        configurationCallback);
}

void NativeStreamingServerImpl::onStopServer()
{
    stopReading();
    ErrCode errCode = rootDevice.asPtr<IDevicePrivate>()->removeStreamingOption(String("daq.ns"));
    checkErrorInfo(errCode);
    serverHandler->stopServer();
}

void NativeStreamingServerImpl::startReading()
{
    readThreadActive = true;
    this->readThread = std::thread([this]()
    {
        this->startReadThread();
        LOG_I("Reading thread finished");
    });
}

void NativeStreamingServerImpl::stopReading()
{
    readThreadActive = false;
    if (readThread.joinable())
    {
        readThread.join();
        LOG_I("Reading thread joined");
    }

    signalReaders.clear();
}

void NativeStreamingServerImpl::startReadThread()
{
    while (readThreadActive)
    {
        {
            std::scoped_lock lock(readersSync);
            for (const auto& [signal, reader] : signalReaders)
            {
                PacketPtr packet = reader.read();
                while (packet.assigned())
                {
                    serverHandler->sendPacket(signal, packet);
                    packet = reader.read();
                }
            }
        }

        std::this_thread::sleep_for(readThreadSleepTime);
    }
}

void NativeStreamingServerImpl::createReaders()
{
    signalReaders.clear();
    auto signals = rootDevice.getSignals(search::Recursive(search::Any()));

    for (const auto& signal : signals)
    {
        addReader(signal);
    }
}

void NativeStreamingServerImpl::addReader(SignalPtr signalToRead)
{
    auto it = std::find_if(signalReaders.begin(),
                           signalReaders.end(),
                           [&signalToRead](const std::pair<SignalPtr, PacketReaderPtr>& element)
                           {
                               return element.first == signalToRead;
                           });
    if (it != signalReaders.end())
        return;

    LOG_I("Add reader for signal {}", signalToRead.getGlobalId());
    auto reader = PacketReader(signalToRead);
    signalReaders.push_back(std::pair<SignalPtr, PacketReaderPtr>({signalToRead, reader}));
}

void NativeStreamingServerImpl::removeReader(SignalPtr signalToRead)
{
    auto it = std::find_if(signalReaders.begin(),
                           signalReaders.end(),
                           [&signalToRead](const std::pair<SignalPtr, PacketReaderPtr>& element)
                           {
                               return element.first == signalToRead;
                           });
    if (it == signalReaders.end())
        return;

    LOG_I("Remove reader for signal {}", signalToRead.getGlobalId());
    signalReaders.erase(it);
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY, NativeStreamingServer, daq::IServer,
    daq::DevicePtr, rootDevice,
    PropertyObjectPtr, config,
    const ContextPtr&, context
)

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_SERVER_MODULE
