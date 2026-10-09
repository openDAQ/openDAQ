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

#ifdef OPENDAQ_ENABLE_RENDERER

#include <thread>

#include <opendaq/data_descriptor_factory.h>
#include <opendaq/data_rule_factory.h>
#include <opendaq/dimension_factory.h>
#include <opendaq/opendaq.h>
#include <ref_fb_module/renderer_utils.h>

#include <testutils/memcheck_listener.h>

using namespace daq;
using namespace daq::modules::ref_fb_module::Renderer;

using RendererUtilsTest = testing::Test;

TEST_F(RendererUtilsTest, ConstantValueRangeAroundPositive)
{
    double min, max;
    constantValueRange(5.0, min, max);

    ASSERT_DOUBLE_EQ(min, 4.5);
    ASSERT_DOUBLE_EQ(max, 5.5);
}

TEST_F(RendererUtilsTest, ConstantValueRangeAroundNegative)
{
    double min, max;
    constantValueRange(-5.0, min, max);

    ASSERT_DOUBLE_EQ(min, -5.5);
    ASSERT_DOUBLE_EQ(max, -4.5);
}

TEST_F(RendererUtilsTest, ConstantValueRangeAroundZeroIsNotEmpty)
{
    double min, max;
    constantValueRange(0.0, min, max);

    // A zero-height range would divide by zero when mapping the value onto the plot.
    ASSERT_LT(min, max);
    ASSERT_DOUBLE_EQ(min, -1.0);
    ASSERT_DOUBLE_EQ(max, 1.0);
}

TEST_F(RendererUtilsTest, PlotItemHeightWithoutStrips)
{
    ASSERT_FLOAT_EQ(plotItemHeight(600.0f, 2, 0, 40.0f), 300.0f);
}

TEST_F(RendererUtilsTest, PlotItemHeightReservesStrips)
{
    ASSERT_FLOAT_EQ(plotItemHeight(600.0f, 2, 2, 40.0f), 260.0f);
}

TEST_F(RendererUtilsTest, PlotItemHeightWithoutPlots)
{
    ASSERT_FLOAT_EQ(plotItemHeight(600.0f, 0, 3, 40.0f), 0.0f);
}

TEST_F(RendererUtilsTest, PlotItemHeightNeverNegative)
{
    ASSERT_FLOAT_EQ(plotItemHeight(100.0f, 1, 4, 40.0f), 0.0f);
}

TEST_F(RendererUtilsTest, NullDescriptorDetection)
{
    ASSERT_TRUE(isNullDescriptor(nullptr));
    ASSERT_TRUE(isNullDescriptor(NullDataDescriptor()));

    const auto descriptor = DataDescriptorBuilder().setSampleType(SampleType::Int32).setRule(ConstantDataRule()).build();
    ASSERT_FALSE(isNullDescriptor(descriptor));
}

TEST_F(RendererUtilsTest, ConstantValueRangeSpansSeveralValues)
{
    double min, max;
    constantValueRange(2.0, 7.0, min, max);

    ASSERT_DOUBLE_EQ(min, 2.0);
    ASSERT_DOUBLE_EQ(max, 7.0);
}

TEST_F(RendererUtilsTest, ConstantValueRangeOfEqualValuesIsNotEmpty)
{
    double min, max;
    constantValueRange(5.0, 5.0, min, max);

    // Elements that are all the same span nothing, and a range of no width cannot be scaled.
    ASSERT_LT(min, max);
    ASSERT_DOUBLE_EQ(min, 4.5);
    ASSERT_DOUBLE_EQ(max, 5.5);
}

TEST_F(RendererUtilsTest, VectorDescriptorDetection)
{
    const auto scalar = DataDescriptorBuilder().setSampleType(SampleType::Int32).setRule(ConstantDataRule()).build();
    ASSERT_FALSE(isVectorDescriptor(scalar));
    ASSERT_FALSE(isVectorDescriptor(nullptr));

    const auto vector = DataDescriptorBuilder()
                            .setSampleType(SampleType::Int32)
                            .setDimensions(List<IDimension>(Dimension(LinearDimensionRule(1, 0, 4))))
                            .setRule(ConstantDataRule())
                            .build();
    ASSERT_TRUE(isVectorDescriptor(vector));
}

TEST_F(RendererUtilsTest, ElementValueReadsEachSampleType)
{
    const int32_t integers[] = {10, -20, 30};
    ASSERT_DOUBLE_EQ(elementValue(SampleType::Int32, integers, 0), 10.0);
    ASSERT_DOUBLE_EQ(elementValue(SampleType::Int32, integers, 1), -20.0);

    const double reals[] = {1.5, 2.5};
    ASSERT_DOUBLE_EQ(elementValue(SampleType::Float64, reals, 1), 2.5);

    const uint8_t bytes[] = {255, 1};
    ASSERT_DOUBLE_EQ(elementValue(SampleType::UInt8, bytes, 0), 255.0);

    // A sample type the renderer cannot draw reads as zero rather than as whatever the bytes happen to be.
    ASSERT_DOUBLE_EQ(elementValue(SampleType::Struct, integers, 0), 0.0);
}

// Opens a renderer window; run with --gtest_also_run_disabled_tests to look at it.
TEST_F(RendererUtilsTest, DISABLED_ShowConstantWithRealSignal)
{
    const auto instance = Instance();
    const auto refDevice = instance.addDevice("daqref://device0");

    const auto constFb = instance.addFunctionBlock("RefFBModuleConstantValue");
    constFb.setPropertyValue("Value", "3");

    const auto vectorFb = instance.addFunctionBlock("RefFBModuleConstantValue");
    vectorFb.setPropertyValue("Value", "1;4;9;16;9;4;1");

    const auto renderer = instance.addFunctionBlock("RefFBModuleRenderer");
    renderer.setPropertyValue("SingleXAxis", true);
    renderer.setPropertyValue("SingleYAxis", true);

    const auto deviceSignal = refDevice.getChannelsRecursive()[0].getSignals()[0];
    renderer.getInputPorts()[0].connect(deviceSignal);
    renderer.getInputPorts()[1].connect(constFb.getSignals()[0]);
    renderer.getInputPorts()[2].connect(vectorFb.getSignals()[0]);

    std::this_thread::sleep_for(std::chrono::seconds(10));

    const auto statusContainer = renderer.getStatusContainer();
    const StringPtr sharedMessage = statusContainer.getStatusMessage("ComponentStatus");
    std::cout << "shared-axis renderer message: " << sharedMessage.toStdString() << std::endl;
    ASSERT_NE(statusContainer.getStatus("ComponentStatus"), "Error") << sharedMessage.toStdString();

    renderer.setPropertyValue("SingleXAxis", false);
    std::this_thread::sleep_for(std::chrono::seconds(10));
    const StringPtr ownTileMessage = statusContainer.getStatusMessage("ComponentStatus");
    std::cout << "own-tile renderer message: " << ownTileMessage.toStdString() << std::endl;
    ASSERT_NE(statusContainer.getStatus("ComponentStatus"), "Error") << ownTileMessage.toStdString();

    instance.removeFunctionBlock(renderer);
}

#endif
