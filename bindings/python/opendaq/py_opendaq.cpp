#include "py_opendaq/py_opendaq.h"

void wrapDaqComponentOpenDaq(pybind11::module_ m)
{
    py::enum_<daq::SampleType>(m, "SampleType")
        .value("Invalid", daq::SampleType::Invalid)
        .value("Undefined", daq::SampleType::Undefined)
        .value("Float32", daq::SampleType::Float32)
        .value("Float64", daq::SampleType::Float64)
        .value("UInt8", daq::SampleType::UInt8)
        .value("Int8", daq::SampleType::Int8)
        .value("UInt16", daq::SampleType::UInt16)
        .value("Int16", daq::SampleType::Int16)
        .value("UInt32", daq::SampleType::UInt32)
        .value("Int32", daq::SampleType::Int32)
        .value("UInt64", daq::SampleType::UInt64)
        .value("Int64", daq::SampleType::Int64)
        .value("RangeInt64", daq::SampleType::RangeInt64)
        .value("ComplexFloat32", daq::SampleType::ComplexFloat32)
        .value("ComplexFloat64", daq::SampleType::ComplexFloat64)
        .value("Binary", daq::SampleType::Binary)
        .value("String", daq::SampleType::String);

    py::enum_<daq::ScaledSampleType>(m, "ScaledSampleType")
        .value("Invalid", daq::ScaledSampleType::Invalid)
        .value("Float32", daq::ScaledSampleType::Float32)
        .value("Float64", daq::ScaledSampleType::Float64);

    auto classIAllocator = declareIAllocator(m);
    auto classIRemovable = declareIRemovable(m);
    auto classIComponent = declareIComponent(m);
    auto classIFolder = declareIFolder(m);
    auto classIFolderConfig = declareIFolderConfig(m);
    auto classIDevice = declareIDevice(m);
    auto classIInstanceBuilder = declareIInstanceBuilder(m);
    auto classIConfigProvider = declareIConfigProvider(m);
    auto classIInstance = declareIInstance(m);
    auto classIContext = declareIContext(m);
    auto classIDeviceDomain = declareIDeviceDomain(m);
    auto classIDeviceInfo = declareIDeviceInfo(m);
    auto classIDeviceInfoConfig = declareIDeviceInfoConfig(m);
    auto classIDeviceType = declareIDeviceType(m);
    auto classIFunctionBlock = declareIFunctionBlock(m);
    auto classIChannel = declareIChannel(m);
    auto classIFunctionBlockType = declareIFunctionBlockType(m);
    auto classILogger = declareILogger(m);
    auto classILoggerComponent = declareILoggerComponent(m);
    auto classILoggerSink = declareILoggerSink(m);
    auto classILoggerThreadPool = declareILoggerThreadPool(m);
    auto classIModule = declareIModule(m);
    auto classIModuleManager = declareIModuleManager(m);
    auto classIReader = declareIReader(m);
    auto classIPacketReader = declareIPacketReader(m);
    auto classISampleReader = declareISampleReader(m);
    auto classIBlockReader = declareIBlockReader(m);
    auto classIStreamReader = declareIStreamReader(m);
    auto classTimeStreamReader = declareTimeStreamReader(m);
    auto classTimeTailReader = declareTimeTailReader(m);
    auto classTimeBlockReader = declareTimeBlockReader(m);
    auto classITailReader = declareITailReader(m);
    auto classIReaderStatus = declareIReaderStatus(m);
    auto classIAwaitable = declareIAwaitable(m);
    auto classIGraphVisualization = declareIGraphVisualization(m);
    auto classIScheduler = declareIScheduler(m);
    auto classITask = declareITask(m);
    auto classITaskGraph = declareITaskGraph(m);
    auto classIDataDescriptor = declareIDataDescriptor(m);
    auto classIDataDescriptorBuilder = declareIDataDescriptorBuilder(m);
    auto classIConnection = declareIConnection(m);
    auto classIPacketDestructCallback = declareIPacketDestructCallback(m);
    auto classIPacket = declareIPacket(m);
    auto classIDataPacket = declareIDataPacket(m);
    auto classIDataRule = declareIDataRule(m);
    auto classIDataRuleBuilder = declareIDataRuleBuilder(m);
    auto classIDimension = declareIDimension(m);
    auto classIDimensionBuilder = declareIDimensionBuilder(m);
    auto classIDimensionRule = declareIDimensionRule(m);
    auto classIDimensionRuleBuilder = declareIDimensionRuleBuilder(m);
    auto classIEventPacket = declareIEventPacket(m);
    auto classIInputPort = declareIInputPort(m);
    auto classIInputPortConfig = declareIInputPortConfig(m);
    auto classIInputPortNotifications = declareIInputPortNotifications(m);
    auto classIRange = declareIRange(m);
    auto classIScaling = declareIScaling(m);
    auto classIScalingBuilder = declareIScalingBuilder(m);
    auto classISignal = declareISignal(m);
    auto classISignalConfig = declareISignalConfig(m);
    auto classISignalEvents = declareISignalEvents(m);
    auto classITags = declareITags(m);
    auto classITagsPrivate = declareITagsPrivate(m);
    auto classIServer = declareIServer(m);
    auto classIServerType = declareIServerType(m);
    auto classIStreaming = declareIStreaming(m);
    auto classIStreamingInfo = declareIStreamingInfo(m);
    auto classIStreamingInfoConfig = declareIStreamingInfoConfig(m);
    auto classIMirroredSignalConfig = declareIMirroredSignalConfig(m);
    auto classISubscriptionEventArgs = declareISubscriptionEventArgs(m);
    auto classMockSignal = declareMockSignal(m);
    auto classISearchFilter = declareISearchFilter(m);
    auto classIComponentPrivate = declareIComponentPrivate(m);
    auto classIComponentStatusContainer = declareIComponentStatusContainer(m);
    auto classIComponentStatusContainerPrivate = declareIComponentStatusContainerPrivate(m);

    defineIAllocator(m, classIAllocator);
    defineIRemovable(m, classIRemovable);
    defineIComponent(m, classIComponent);
    defineIFolder(m, classIFolder);
    defineIFolderConfig(m, classIFolderConfig);
    defineIDevice(m, classIDevice);

    defineIInstanceBuilder(m, classIInstanceBuilder);
    defineIInstance(m, classIInstance);
    defineIConfigProvider(m, classIConfigProvider);
    defineIContext(m, classIContext);
    defineIDeviceDomain(m, classIDeviceDomain);
    defineIDeviceInfo(m, classIDeviceInfo);
    defineIDeviceInfoConfig(m, classIDeviceInfoConfig);
    defineIDeviceType(m, classIDeviceType);
    defineIFunctionBlock(m, classIFunctionBlock);
    defineIChannel(m, classIChannel);
    defineIFunctionBlockType(m, classIFunctionBlockType);
    defineILogger(m, classILogger);
    defineILoggerComponent(m, classILoggerComponent);
    defineILoggerSink(m, classILoggerSink);
    defineILoggerThreadPool(m, classILoggerThreadPool);
    defineIModule(m, classIModule);
    defineIModuleManager(m, classIModuleManager);
    defineIReader(m, classIReader);
    defineIPacketReader(m, classIPacketReader);
    defineISampleReader(m, classISampleReader);
    defineIBlockReader(m, classIBlockReader);
    defineIStreamReader(m, classIStreamReader);
    defineTimeStreamReader(m, classTimeStreamReader);
    defineTimeTailReader(m, classTimeTailReader);
    defineTimeBlockReader(m, classTimeBlockReader);
    defineITailReader(m, classITailReader);
    defineIReaderStatus(m, classIReaderStatus);
    defineIAwaitable(m, classIAwaitable);
    defineIGraphVisualization(m, classIGraphVisualization);
    defineIScheduler(m, classIScheduler);
    defineITask(m, classITask);
    defineITaskGraph(m, classITaskGraph);
    defineIDataDescriptor(m, classIDataDescriptor);
    defineIDataDescriptorBuilder(m, classIDataDescriptorBuilder);
    defineIConnection(m, classIConnection);
    defineIPacketDestructCallback(m, classIPacketDestructCallback);
    defineIPacket(m, classIPacket);
    defineIDataPacket(m, classIDataPacket);
    defineIDataRule(m, classIDataRule);
    defineIDataRuleBuilder(m, classIDataRuleBuilder);
    defineIDimension(m, classIDimension);
    defineIDimensionBuilder(m, classIDimensionBuilder);
    defineIDimensionRule(m, classIDimensionRule);
    defineIDimensionRuleBuilder(m, classIDimensionRuleBuilder);
    defineIEventPacket(m, classIEventPacket);
    defineIInputPort(m, classIInputPort);
    defineIInputPortConfig(m, classIInputPortConfig);
    defineIInputPortNotifications(m, classIInputPortNotifications);
    defineIRange(m, classIRange);
    defineIScaling(m, classIScaling);
    defineIScalingBuilder(m, classIScalingBuilder);
    defineISignal(m, classISignal);
    defineISignalConfig(m, classISignalConfig);
    defineISignalEvents(m, classISignalEvents);
    defineITags(m, classITags);
    defineITagsPrivate(m, classITagsPrivate);
    defineIServer(m, classIServer);
    defineIServerType(m, classIServerType);
    defineIStreaming(m, classIStreaming);
    defineIStreamingInfo(m, classIStreamingInfo);
    defineIStreamingInfoConfig(m, classIStreamingInfoConfig);
    defineIMirroredSignalConfig(m, classIMirroredSignalConfig);
    defineISubscriptionEventArgs(m, classISubscriptionEventArgs);
    defineMockSignal(m, classMockSignal);
    defineISearchFilter(m, classISearchFilter);
    defineIComponentPrivate(m, classIComponentPrivate);
    defineIComponentStatusContainer(m, classIComponentStatusContainer);
    defineIComponentStatusContainerPrivate(m, classIComponentStatusContainerPrivate);

    m.def("Instance", []() { return daq::Instance(".").detach(); });
}
