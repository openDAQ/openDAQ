/*
 * Copyright 2022-2026 openDAQ d.o.o.
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

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>

#include <coretypes/filesystem.h>
#include <opendaq/function_block_impl.h>
#include <opendaq/opendaq.h>

#include <file_recorder_module/common.h>
#include <file_recorder_module/signal_file_reader.h>

BEGIN_NAMESPACE_OPENDAQ_FILE_RECORDER_MODULE

/*!
 * @brief A function block which replays a `.daqrec` file recorded by FileRecorderFbImpl into an
 *     output signal.
 *
 * The file is read incrementally by a background thread, so a recording much larger than available
 * memory can be replayed. The thread paces itself according to PlaybackMode:
 *
 * - `FixedSampleRate` emits samples evenly at the rate given by the SampleRate property, and
 *   builds a fresh linear domain to match. Any domain the recording carries is ignored, so this
 *   mode also works for recordings of signals which had no domain signal.
 *
 * - `RecordedDomain` takes both the pacing and the domain values from the recording. The wall
 *   clock interval between two samples is the interval their domain values describe, so pauses in
 *   the recording are reproduced as pauses in the playback. SampleRate is not used.
 *
 * Samples are grouped into output packets no longer than OutputPacketIntervalMs, and a pause
 * longer than that always ends a packet, so a gap is reproduced as a wait between packets rather
 * than being smeared across one.
 */
class FilePlayerFbImpl final : public FunctionBlock
{
public:
    /*!
     * @brief The type ID of this function block.
     */
    static constexpr const char* TYPE_ID = "FilePlayer";

    /*!
     * @brief Selects what determines the interval between replayed samples.
     */
    enum class PlaybackMode
    {
        /*!
         * @brief Samples are emitted evenly at the rate given by SampleRate, with a freshly built
         *     linear domain. The recorded domain, if any, is ignored.
         */
        FixedSampleRate = 0,

        /*!
         * @brief Samples are emitted at the intervals their recorded domain values describe, and
         *     carry those domain values.
         */
        RecordedDomain = 1,
    };

    /*!
     * @brief Contains constants for the names of properties supported by this function block.
     */
    struct Props
    {
        /*!
         * @brief The path of the recording to replay.
         */
        static constexpr const char* FILE_PATH = "FilePath";

        /*!
         * @brief The PlaybackMode to replay with.
         */
        static constexpr const char* PLAYBACK_MODE = "PlaybackMode";

        /*!
         * @brief The rate in hertz at which to emit samples. Used by, and visible in, the
         *     `FixedSampleRate` mode only.
         */
        static constexpr const char* SAMPLE_RATE = "SampleRate";

        /*!
         * @brief Whether the replayed domain starts at the moment playback starts rather than at
         *     the moment the recording was made. Used by, and visible in, the `RecordedDomain`
         *     mode only.
         *
         * The intervals between samples are the recorded ones either way; only the absolute
         * position of the domain differs. Shifting keeps the replayed data out of the past, which
         * is what consumers such as the renderer expect, and it lets each pass of a looped
         * playback continue the domain forwards instead of jumping back.
         */
        static constexpr const char* SHIFT_DOMAIN_TO_NOW = "ShiftDomainToNow";

        /*!
         * @brief Whether to start the recording over when its end is reached.
         */
        static constexpr const char* LOOP = "Loop";

        /*!
         * @brief The longest span of samples to place in one output packet, in milliseconds.
         */
        static constexpr const char* OUTPUT_PACKET_INTERVAL_MS = "OutputPacketIntervalMs";

        /*!
         * @brief A procedure property which starts playback.
         */
        static constexpr const char* START_PLAYBACK = "StartPlayback";

        /*!
         * @brief A procedure property which stops playback.
         */
        static constexpr const char* STOP_PLAYBACK = "StopPlayback";
    };

    /*!
     * @brief Creates and returns a type object describing this function block.
     */
    static FunctionBlockTypePtr createType();

    /*!
     * @brief Creates a new function block.
     * @param context The openDAQ context object.
     * @param parent The component object which will contain this function block.
     * @param localId The local identifier of this function block.
     * @param config A property object containing configuration data for this function block.
     */
    FilePlayerFbImpl(const ContextPtr& context,
                     const ComponentPtr& parent,
                     const StringPtr& localId,
                     const PropertyObjectPtr& config);

    ~FilePlayerFbImpl() override;

    /*!
     * @brief Opens the configured file, publishes the output signal's descriptors, and starts the
     *     playback thread. Does nothing if playback is already running.
     *
     * Failures to open or interpret the file are reported through the component status rather
     * than thrown, so that setting the property from a client does not raise.
     */
    void startPlayback();

    /*!
     * @brief Stops playback and joins the playback thread. Does nothing if playback is not
     *     running.
     */
    void stopPlayback();

protected:
    /*!
     * @brief Stops playback when the function block is deactivated.
     */
    void activeChanged() override;

    /*!
     * @brief Stops playback before the function block is removed, so that the thread does not
     *     outlive the signals it sends to.
     */
    void removed() override;

private:
    void initProperties();
    void initSignals();

    /*!
     * @brief Reads the properties into the cached members. Playback settings are latched when
     *     playback starts, so changing them mid-playback takes effect at the next start.
     */
    void readProperties();

    /*!
     * @brief Called when FilePath changes: restarts playback if it was running, so that the new
     *     file replaces the old one.
     */
    void onFilePathChanged();

    /*!
     * @brief Builds the linear domain descriptor used by the `FixedSampleRate` mode.
     */
    DataDescriptorPtr buildGeneratedDomainDescriptor() const;

    /*!
     * @brief Returns the domain descriptor to publish in the `RecordedDomain` mode: the recorded
     *     one, or a copy of it whose origin is the moment playback started.
     */
    DataDescriptorPtr buildRecordedDomainDescriptor(const DataDescriptorPtr& recorded) const;

    void threadMain();

    /*!
     * @brief Emits @p count samples of @p run starting at @p offset as one value packet and one
     *     domain packet.
     */
    void emitSlice(const SignalFileReader::Run& run, std::size_t offset, std::size_t count);

    /*!
     * @brief Builds the domain packet for a slice, according to the active playback mode and the
     *     run's domain kind.
     */
    DataPacketPtr createDomainPacket(const SignalFileReader::Run& run, std::size_t offset, std::size_t count);

    /*!
     * @brief Returns how many of @p run's samples starting at @p offset belong in one output
     *     packet: all of them in the `FixedSampleRate` mode, and in the `RecordedDomain` mode as
     *     many as fit within the packet interval before a gap or the interval itself ends it.
     */
    std::size_t sliceLength(const SignalFileReader::Run& run, std::size_t offset) const;

    /*!
     * @brief Blocks until the slice at @p offset is due to be emitted.
     * @returns False if playback was stopped while waiting.
     */
    bool waitForSlice(const SignalFileReader::Run& run, std::size_t offset);

    /*!
     * @brief Blocks until @p deadline, or until playback is stopped.
     * @returns False if playback was stopped.
     */
    bool waitUntil(std::chrono::steady_clock::time_point deadline);

    /*!
     * @brief The value added to every emitted integer tick: the rebasing which ShiftDomainToNow
     *     asks for, plus however far previous loop passes have advanced the domain.
     */
    Int emitShiftInt() const;

    /*!
     * @brief The floating-point counterpart of emitShiftInt().
     */
    double emitShiftDouble() const;

    /*!
     * @brief Rewinds the reader and advances the domain and pacing offsets so that the next pass
     *     continues where the previous one ended instead of jumping back in the domain.
     */
    void beginNextLoop();

    SignalConfigPtr outputSignal;
    SignalConfigPtr outputDomainSignal;

    // Latched from the properties when playback starts.
    fs::path filePath;
    PlaybackMode playbackMode = PlaybackMode::FixedSampleRate;
    double sampleRate = 1000.0;
    bool shiftDomainToNow = true;
    bool loopPlayback = false;
    std::chrono::nanoseconds packetInterval{};

    std::mutex threadMutex;
    std::condition_variable threadCv;
    bool stopRequested = false;
    std::thread thread;

    /*!
     * @brief True between a successful start and the stop which follows it. Written under the
     *     configuration lock.
     */
    bool playing = false;

    // The members below belong to the playback thread and are not shared.
    std::unique_ptr<SignalFileReader> reader;
    DataDescriptorPtr valueDescriptor;
    DataDescriptorPtr domainDescriptor;
    SampleType domainSampleType = SampleType::Undefined;
    bool domainIsFloatingPoint = false;
    double tickResolutionSeconds = 0.0;

    /*!
     * @brief The wall clock instant the first sample was emitted at, and the domain tick it
     *     carried: together they anchor every later sample's deadline. Set when the first sample
     *     of the first pass is about to be emitted.
     */
    std::chrono::steady_clock::time_point playbackStart;
    double firstTick = 0.0;
    Int firstTickInt = 0;
    bool anchored = false;

    /*!
     * @brief The recorded linear rule's delta, cached so that implicit packets do not re-read it
     *     from the descriptor for every packet. Zero when the recorded domain is not linear.
     */
    Int recordedDeltaInt = 0;
    double recordedDeltaDouble = 0.0;

    /*!
     * @brief The tick spacing to leave between the last sample of one loop pass and the first
     *     sample of the next. The linear rule's delta for an implicit domain, and the last
     *     interval seen for an explicit one.
     */
    Int lastStepInt = 0;
    double lastStepDouble = 0.0;

    /*!
     * @brief The domain tick of the most recently emitted sample.
     */
    Int lastTickInt = 0;
    double lastTick = 0.0;

    /*!
     * @brief Accumulated across loop passes, in domain ticks: added to every emitted tick and to
     *     every deadline so that each pass continues the previous one.
     */
    Int loopOffsetInt = 0;
    double loopOffsetDouble = 0.0;

    /*!
     * @brief The number of samples emitted so far, which drives both the deadlines and the
     *     implicit packet offsets of the `FixedSampleRate` mode.
     */
    std::uint64_t emittedSamples = 0;

    /*!
     * @brief The linear rule delta of the generated domain, in ticks.
     */
    Int generatedDelta = 0;
};

END_NAMESPACE_OPENDAQ_FILE_RECORDER_MODULE
