/*
 * Copyright 2022-2025 openDAQ d.o.o.
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
#include <ref_fb_module/common.h>
#include <opendaq/function_block_impl.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/event_packet_ptr.h>

#include "SFML/Window.hpp"
#include "SFML/Graphics.hpp"


BEGIN_NAMESPACE_REF_FB_MODULE

namespace VideoPlayer
{

template<typename T>
class SafeDeque
{
public:
    void pushBack(T&& value)
    {
        std::lock_guard<std::mutex> lock(mutex);
        deque.push_back(std::forward<T>(value));
    }

    bool tryPopFront(T& value)
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (deque.empty())
            return false;

        value = std::move(deque.front());
        deque.pop_front();
        return true;
    }

    void clear()
    {
        std::lock_guard<std::mutex> lock(mutex);
        deque.clear();
    }

private:
    std::deque<T> deque;
    mutable std::mutex mutex;
};

class VideoPlayerFbImpl final : public FunctionBlock
{
public:
    explicit VideoPlayerFbImpl(const ContextPtr& ctx, 
                            const ComponentPtr& parent, 
                            const StringPtr& localId, 
                            const PropertyObjectPtr& config);

    static FunctionBlockTypePtr CreateType();

    void onPacketReceived(const InputPortPtr& port) override;
    void handleDataPacket(const DataPacketPtr& packet);
    void handleEventPacket(const EventPacketPtr& packet);

private:
    void initProperties();
    void initInputPorts();
    void startRender();

    InputPortConfigPtr videoInputPort;

    std::unique_ptr<sf::RenderWindow> window;
    sf::Texture texture;
    sf::Sprite sprite;

    SafeDeque<DataPacketPtr> dataPackets;
};

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY, VideoPlayerFb, IFunctionBlock,
    IContext*, context,
    IString*, parentGlobalId,
    IString*, localId,
    IPropertyObject*, config
);

}

END_NAMESPACE_REF_FB_MODULE
