/*
 * Copyright 2022-2023 Blueberry d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include <opendaq/streaming_impl.h>
#include <opendaq/streaming.h>
#include <opendaq/streaming_ptr.h>
#include <gmock/gmock.h>
#include <coretypes/gmock/mock_ptr.h>
#include <opendaq/packet_factory.h>

struct MockStreaming : daq::Streaming
{
    typedef MockPtr<
        daq::IStreaming,
        daq::StreamingPtr,
        MockStreaming,
        const daq::StringPtr&
    > Strict;

    daq::StringPtr onGetSignalStreamingId(const daq::StringPtr& signalRemoteId) override
    {
        return signalRemoteId;
    }

    MOCK_METHOD(void, onSetActive, (bool active), (override));
    MOCK_METHOD(void, onAddSignal, (const daq::MirroredSignalConfigPtr& signal), (override));
    MOCK_METHOD(void, onRemoveSignal, (const daq::MirroredSignalConfigPtr& signal), (override));
    MOCK_METHOD(void, onSubscribeSignal, (const daq::StringPtr& signalRemoteId, const daq::StringPtr& domainSignalRemoteId), (override));
    MOCK_METHOD(void, onUnsubscribeSignal, (const daq::StringPtr& signalRemoteId, const daq::StringPtr& domainSignalRemoteId), (override));
    MOCK_METHOD(daq::EventPacketPtr, onCreateDataDescriptorChangedEventPacket, (const daq::StringPtr& signalRemoteId), (override));

    daq::MirroredSignalConfigPtr signal;

    MockStreaming(const daq::StringPtr& connectionString) : daq::Streaming(connectionString, nullptr)
    {
        using namespace testing;

        ON_CALL(*this, onSetActive)
            .WillByDefault(DoAll(
                Invoke([&](bool active) {})
            ));

        ON_CALL(*this, onAddSignal)
            .WillByDefault(DoAll(
                Invoke([&](const daq::MirroredSignalConfigPtr& signal)
                       {
                           this->signal = signal;
                       })
            ));

        ON_CALL(*this, onRemoveSignal)
            .WillByDefault(DoAll(
                Invoke([&](const daq::MirroredSignalConfigPtr& signal)
                       {
                           if (signal == this->signal)
                                this->signal = nullptr;
                       })
            ));

        ON_CALL(*this, onSubscribeSignal)
            .WillByDefault(DoAll(
                Invoke([&](const daq::StringPtr& signalRemoteId, const daq::StringPtr& /*domainSignalRemoteId*/)
                       {
                           if (signal.getRemoteId() == signalRemoteId)
                                signal.template asPtr<daq::IMirroredSignalPrivate>()->subscribeCompleted(this->connectionString);
                       })
            ));

        ON_CALL(*this, onUnsubscribeSignal)
            .WillByDefault(DoAll(
                Invoke([&](const daq::StringPtr& signalRemoteId, const daq::StringPtr& /*domainSignalRemoteId*/)
                       {
                           if (signal.getRemoteId() == signalRemoteId)
                                signal.template asPtr<daq::IMirroredSignalPrivate>()->unsubscribeCompleted(this->connectionString);
                       })
            ));

        ON_CALL(*this, onCreateDataDescriptorChangedEventPacket)
            .WillByDefault(DoAll(
                Invoke([&](const daq::StringPtr& signalRemoteId)
                       {
                           return daq::DataDescriptorChangedEventPacket(nullptr, nullptr);
                       })
            ));
    }
};
