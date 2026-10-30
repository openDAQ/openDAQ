#include "multi_reader2_harness.h"

namespace daq::mr2test
{

class MultiReader2Scenario : public MultiReader2Test
{
};

// S2.01
TEST_F(MultiReader2Scenario, LoopReadingWaitsOutAnError)
{
    given().inputs(3);
    // step 2
    expect(drain(),
           {count(0).level(Ok).main(in1).domainChanged().resync().allDescChanged().size(in1, 8).size(in2, 8).size(in3, 8),
            count(0).noChanges()});
    write(in1, 0, 99);
    write(in2, 0, 99);
    write(in3, 0, 99);
    // step 6
    expect(drain(), {count(100).at(0).noChanges(), count(0).noChanges()});
    write(in1, 100, 199);
    write(in2, 100, 149);
    announce(in2, Value::structure());
    write(in3, 100, 199);
    // step 11
    expect(drain(),
           {count(50)
                .at(100)
                .level(Error)
                .mainNull()
                .inError(in2, InputError::ValueDescriptorInvalid)
                .msg("in2: value type not convertible to Float64")
                .size(in1, 0)
                .size(in2, 0)
                .size(in3, 0),
            count(0).noChanges()});
    write(in1, 200, 299);
    write(in3, 200, 299);
    // step 14
    expect(drain(), {count(0).noChanges()});
    announce(in2, Value(SampleType::Int32));
    write(in2, 300, 399);
    write(in1, 300, 399);
    write(in3, 300, 399);
    // step 19
    expect(
        drain(),
        {count(0).level(Ok).main(in1).domainChanged().resync().allDescChanged().healthy(in2).msg("").size(in1, 8).size(in2, 8).size(in3, 8),
         count(0).noChanges()});
    write(in1, 400, 499);
    write(in2, 400, 499);
    write(in3, 400, 499);
    // step 23
    expect(drain(), {count(100).at(400).noChanges(), count(0).noChanges()});
}

// S2.02
TEST_F(MultiReader2Scenario, HandlerReadingFollowsAutomaticMainInputMove)
{
    given().inputs(3);
    // step 2
    expect(configure(same().policy(Exclude)),
           ok().level(Ok).main(in1).domainChanged().resync().allDescChanged().size(in1, 8).size(in2, 8).size(in3, 8));
    handler(
        [&]
        {
            sender();
            drain();
        });
    write(in1, 0, 99);
    write(in2, 0, 99);
    write(in3, 0, 99);
    // step 7
    expect(idle(), calls(1, {ok().nullSender(), count(100).at(0).noChanges(), count(0).noChanges()}));
    announce(in3, Value::null());
    // step 9
    expect(idle(),
           calls(1,
                 {ok().nullSender(),
                  count(0).level(Warning).inError(in3, InputError::ValueDescriptorInvalid).msg("in3: no value descriptor").size(in3, 0),
                  count(0).noChanges()}));
    write(in1, 100, 199);
    write(in2, 100, 199);
    // step 12
    expect(idle(), calls(1, {ok().nullSender(), count(100).at(100).noChanges(), count(0).noChanges()}));
    blockWorker();
    announce(in1, Value::structure());
    write(in2, 200, 299);
    unblockWorker();
    // step 17
    expect(idle(),
           calls(1,
                 {ok().nullSender(),
                  count(0)
                      .main(in2)
                      .domainChanged()
                      .resync()
                      .descChanged(in2)
                      .inError(in1, InputError::ValueDescriptorInvalid)
                      .msg("in1: value type not convertible to Float64; in3: no value descriptor")
                      .size(in1, 0),
                  count(0).noChanges()}));
    write(in2, 300, 399);
    // step 19
    expect(idle(), calls(1, {ok().nullSender(), count(100).at(300).noChanges(), count(0).noChanges()}));
    announce(in1, Value(SampleType::Int32));
    // step 21
    expect(idle(),
           calls(1,
                 {ok().nullSender(),
                  count(0).descChanged(in1).healthy(in1).msg("in3: no value descriptor").size(in1, 8),
                  count(0).noChanges()}));
    write(in1, 400, 499);
    write(in2, 400, 499);
    // step 24
    expect(idle(), calls(1, {ok().nullSender(), count(100).at(400).noChanges(), count(0).noChanges()}));
    announce(in3, Value(SampleType::Int32));
    // step 26
    expect(idle(),
           calls(1, {ok().nullSender(), count(0).level(Ok).descChanged(in3).healthy(in3).msg("").size(in3, 8), count(0).noChanges()}));
    write(in1, 500, 599);
    write(in2, 500, 599);
    write(in3, 500, 599);
    // step 30
    expect(idle(), calls(1, {ok().nullSender(), count(100).at(500).noChanges(), count(0).noChanges()}));
}

// S2.03
TEST_F(MultiReader2Scenario, MainInputFromSettingWithRejectionByUnit)
{
    given().inputs(3).value(in1, Value().unit("V")).value(in2, Value().unit("V")).value(in3, Value().unit("A"));
    // step 2
    expect(configure(same().policy(Exclude).newAccepts(AcceptsRule::UnitV)),
           ok().level(Warning)
               .main(in1)
               .domainChanged()
               .resync()
               .descChanged(in1)
               .descChanged(in2)
               .inError(in3, InputError::ValueDescriptorInvalid)
               .msg("in3: value descriptor rejected by the owner")
               .size(in1, 8)
               .size(in2, 8)
               .size(in3, 0));
    handler([&] { drain(); });
    write(in1, 0, 99);
    write(in2, 0, 99);
    write(in3, 0, 99);
    // step 7
    expect(idle(), calls(1, {count(100).at(0).noChanges(), count(0).noChanges()}));
    // step 8
    expect(configure(same().main(in2)), ok().main(in2).domainChanged().resync().descChanged(in1).descChanged(in2));
    write(in1, 100, 199);
    write(in2, 100, 199);
    // step 11
    expect(idle(), calls(1, {count(100).at(100).noChanges(), count(0).noChanges()}));
    // step 12
    expect(configure(same().main(in3)), ok().level(Error).main(in3).size(in1, 0).size(in2, 0));
    write(in1, 200, 299);
    write(in2, 200, 299);
    // step 15
    expect(idle(), calls(0));
    // step 16
    expect(configure(same().main(in1)),
           ok().level(Warning).main(in1).domainChanged().resync().descChanged(in1).descChanged(in2).size(in1, 8).size(in2, 8));
    write(in1, 300, 399);
    write(in2, 300, 399);
    // step 19
    expect(idle(), calls(1, {count(100).at(300).noChanges(), count(0).noChanges()}));
    announce(in3, Value().unit("V"));
    // step 21
    expect(idle(), calls(1, {count(0).level(Ok).descChanged(in3).healthy(in3).msg("").size(in3, 8), count(0).noChanges()}));
    write(in1, 400, 499);
    write(in2, 400, 499);
    write(in3, 400, 499);
    // step 25
    expect(idle(), calls(1, {count(100).at(400).noChanges(), count(0).noChanges()}));
}

// S2.04
TEST_F(MultiReader2Scenario, FreePortSumJudgesDescriptorsAcrossInputs)
{
    given()
        .ports()
        .inputs(0)
        .policy(Exclude)
        .accepts(AcceptsRule::Sum)
        .value(in1, Value().unit("V"))
        .value(in2, Value().unit("V"))
        .value(in3, Value(SampleType::Int32).unit("A"));
    offerFreePort(in1);
    ownerReacts(OwnerReacts().cachesDescriptors().removesDisconnectedPorts());
    handler([&] { drain(); });
    connect(in1);  // s1 is in1's own signal
    offerFreePort(in2);
    // step 6
    expect(configure(same().add(in1)), ok().level(Ok).main(in1).domainChanged().resync().allDescChanged().size(in1, 8).inputs({in1}));
    write(in1, 0, 99);
    // step 8
    expect(idle(), calls(1, {count(100).at(0).noChanges(), count(0).noChanges()}));
    connect(in2);  // s2 is in2's own signal
    offerFreePort(in3);
    // step 11
    expect(configure(same().add(in2)), ok().domainChanged().resync().allDescChanged().size(in2, 8).inputs({in1, in2}));
    write(in1, 100, 199);
    write(in2, 100, 199);
    // step 14
    expect(idle(), calls(1, {count(100).at(100).noChanges(), count(0).noChanges()}));
    connect(in3);  // s3 is in3's own signal
    offerFreePort(in4);
    // step 17
    expect(configure(same().add(in3)),
           ok().level(Warning)
               .domainChanged()
               .resync()
               .descChanged(in1)
               .descChanged(in2)
               .inError(in3, InputError::ValueDescriptorInvalid)
               .msg("in3: value descriptor rejected by the owner")
               .size(in3, 0)
               .inputs({in1, in2, in3}));
    write(in1, 200, 299);
    write(in2, 200, 299);
    write(in3, 200, 299);
    // step 21
    expect(idle(), calls(1, {count(100).at(200).noChanges(), count(0).noChanges()}));
    announce(in1, Value().unit("A"));
    // step 23: in1's new unit judges in2 and in3 again in the same read
    expect(idle(),
           calls(1,
                 {count(0)
                      .descChanged(in1)
                      .descChanged(in3)
                      .inError(in2, InputError::ValueDescriptorInvalid)
                      .healthy(in3)
                      .msg("in2: value descriptor rejected by the owner")
                      .size(in2, 0)
                      .size(in3, 8),
                  count(0).noChanges()}));
    write(in1, 300, 399);
    write(in2, 300, 399);
    write(in3, 300, 399);
    // step 27
    expect(idle(), calls(1, {count(100).at(300).noChanges(), count(0).noChanges()}));
    disconnect(in3);
    // step 29
    expect(idle(),
           calls(1,
                 {count(0)
                      .inError(in3, InputError::Disconnected)
                      .msg("in2: value descriptor rejected by the owner; in3: not connected")
                      .size(in3, 0),
                  ok().domainChanged().resync().descChanged(in1).msg("in2: value descriptor rejected by the owner").inputs({in1, in2}),
                  fails(OPENDAQ_ERR_INVALIDPARAMETER).unchanged(),
                  count(0).noChanges()}));
    write(in1, 400, 499);
    // step 31
    expect(idle(), calls(1, {count(100).at(400).noChanges(), count(0).noChanges()}));
    announce(in2, Value().unit("A"));
    // step 33
    expect(idle(), calls(1, {count(0).level(Ok).descChanged(in2).healthy(in2).msg("").size(in2, 8), count(0).noChanges()}));
    write(in1, 500, 599);
    write(in2, 500, 599);
    // step 36
    expect(idle(), calls(1, {count(100).at(500).noChanges(), count(0).noChanges()}));
}

// S2.05
TEST_F(MultiReader2Scenario, TwoFixedPortsWithPinnedVoltagePort)
{
    given()
        .ports()
        .unconnected(in1)
        .unconnected(in2)
        .main(in1)
        .accepts(AcceptsRule::Power)
        .value(in1, Value().unit("V"))
        .value(in2, Value().unit("A"));
    ownerReacts(OwnerReacts().cachesUnlessError().warnsWhenOnlyDisconnected());
    handler([&] { drain(); });
    connect(in1);  // s1 is in1's own signal
    // step 4
    expect(
        idle(),
        calls(1,
              {count(0).level(Error).main(in1).inError(in2, InputError::Disconnected).msg("in2: not connected").size(in1, 0).size(in2, 0),
               count(0).noChanges()}));
    // step 5
    expectComponentStatus(Warning, "in2: not connected");
    write(in1, 0, 99);
    // step 7
    expect(idle(), calls(0));
    connect(in2);  // s2 is in2's own signal
    // step 9
    expect(idle(),
           calls(1,
                 {count(0).level(Ok).domainChanged().resync().allDescChanged().healthy(in2).msg("").size(in1, 8).size(in2, 8),
                  count(0).noChanges()}));
    write(in1, 100, 199);
    write(in2, 100, 199);
    // step 12
    expect(idle(), calls(1, {count(100).at(100).noChanges(), count(0).noChanges()}));
    announce(in1, Value().unit("mV"));
    // step 14
    expect(idle(),
           calls(1,
                 {count(0)
                      .level(Error)
                      .inError(in1, InputError::ValueDescriptorInvalid)
                      .msg("in1: value descriptor rejected by the owner")
                      .size(in1, 0)
                      .size(in2, 0),
                  count(0).noChanges()}));
    // step 15
    expectComponentStatus(Error, "in1: value descriptor rejected by the owner");
    write(in1, 200, 299);
    write(in2, 200, 299);
    // step 18
    expect(idle(), calls(0));
    announce(in1, Value().unit("V"));
    // step 20
    expect(idle(),
           calls(1,
                 {count(0).level(Ok).domainChanged().resync().allDescChanged().healthy(in1).msg("").size(in1, 8).size(in2, 8),
                  count(0).noChanges()}));
    write(in1, 300, 399);
    write(in2, 300, 399);
    // step 23
    expect(idle(), calls(1, {count(100).at(300).noChanges(), count(0).noChanges()}));
}

// S2.06
TEST_F(MultiReader2Scenario, PortCountAndMainInputFromProperties)
{
    given().ports().unconnected(in1).unconnected(in2).main(in1).policy(Exclude);
    ownerReacts(OwnerReacts().cachesUnlessError().setsComponentStatus());
    handler([&] { drain(); });
    connect(in1);  // s1 is in1's own signal
    // step 4
    expect(idle(),
           calls(1,
                 {count(0)
                      .level(Warning)
                      .main(in1)
                      .domainChanged()
                      .resync()
                      .descChanged(in1)
                      .inError(in2, InputError::Disconnected)
                      .msg("in2: not connected")
                      .size(in1, 8)
                      .size(in2, 0),
                  count(0).noChanges()}));
    connect(in2);  // s2 is in2's own signal
    // step 6
    expect(idle(), calls(1, {count(0).level(Ok).descChanged(in2).healthy(in2).msg("").size(in2, 8), count(0).noChanges()}));
    write(in1, 0, 99);
    write(in2, 0, 99);
    // step 9
    expect(idle(), calls(1, {count(100).at(0).noChanges(), count(0).noChanges()}));
    addPort(in3);  // the owner's NumInputs is 3
    // step 11
    expect(configure(same().add(in3)),
           ok().level(Warning)
               .domainChanged()
               .resync()
               .descChanged(in1)
               .descChanged(in2)
               .inError(in3, InputError::Disconnected)
               .msg("in3: not connected")
               .size(in3, 0)
               .inputs({in1, in2, in3}));
    write(in1, 100, 199);
    write(in2, 100, 199);
    // step 14
    expect(idle(), calls(1, {count(100).at(100).noChanges(), count(0).noChanges()}));
    connect(in3);  // s3 is in3's own signal
    // step 16
    expect(idle(), calls(1, {count(0).level(Ok).descChanged(in3).healthy(in3).msg("").size(in3, 8), count(0).noChanges()}));
    write(in1, 200, 299);
    write(in2, 200, 299);
    write(in3, 200, 299);
    // step 20
    expect(idle(), calls(1, {count(100).at(200).noChanges(), count(0).noChanges()}));
    // step 21
    // the owner's MainInput is Input2: the configure below
    // step 22
    expect(configure(same().main(in2)), ok().main(in2).domainChanged().resync().allDescChanged());
    write(in1, 300, 399);
    write(in2, 300, 399);
    write(in3, 300, 399);
    // step 26
    expect(idle(), calls(1, {count(100).at(300).noChanges(), count(0).noChanges()}));
    disconnect(in2);
    // step 28
    expect(
        idle(),
        calls(
            1,
            {count(0).level(Error).inError(in2, InputError::Disconnected).msg("in2: not connected").size(in1, 0).size(in2, 0).size(in3, 0),
             count(0).noChanges()}));
    write(in1, 400, 499);
    write(in3, 400, 499);
    // step 31
    expect(idle(), calls(0));
    connect(in2);  // s2 is in2's own signal
    // step 33
    expect(idle(),
           calls(1,
                 {count(0).level(Ok).domainChanged().resync().allDescChanged().healthy(in2).msg("").size(in1, 8).size(in2, 8).size(in3, 8),
                  count(0).noChanges()}));
    write(in1, 500, 599);
    write(in2, 500, 599);
    write(in3, 500, 599);
    // step 37
    expect(idle(), calls(1, {count(100).at(500).noChanges(), count(0).noChanges()}));
    // step 38
    // the owner's NumInputs is 2: the configure below
    // step 39
    expect(configure(same().remove(in3)), ok().domainChanged().resync().allDescChanged().inputs({in1, in2}));
    // step 40
    expectPort(in3, Listener::None, PacketReadyNotification::None);
    removePort(in3);
    write(in1, 600, 699);
    write(in2, 600, 699);
    // step 44
    expect(idle(), calls(1, {count(100).at(600).noChanges(), count(0).noChanges()}));
}

// S2.07
TEST_F(MultiReader2Scenario, StatusOnlyPollingThenLoopReading)
{
    given().ports().unconnected(in2);
    // step 2
    expect(read(0, nullData),
           count(0).level(Error).mainNull().inError(in2, InputError::Disconnected).msg("in2: not connected").size(in1, 0).size(in2, 0));
    // step 3
    expect(inputStatus(in2), ok().inError(in2, InputError::Disconnected).size(in2, 0));
    write(in1, 0, 99);
    // step 5
    expect(read(0, nullData), count(0).noChanges());
    // step 6
    expect(avail(), count(0));
    connect(in2);  // s2 is in2's own signal
    // step 8
    expect(read(0, nullData),
           count(0).level(Ok).main(in1).domainChanged().resync().allDescChanged().healthy(in2).msg("").size(in1, 8).size(in2, 8));
    write(in1, 100, 199);
    write(in2, 100, 199);
    // step 11
    expect(read(0, nullData), count(0).noChanges());
    // step 12
    expect(avail(), count(100));
    write(in1, 200, 299);
    write(in2, 200, 299);
    // step 15
    expect(avail(), count(200));
    // step 16
    expect(skip(200), count(200).at(100).noChanges());
    write(in1, 300, 399);
    write(in2, 300, 399);
    // step 19
    expect(readWithDomain(100), count(100).at(300).noChanges());
    disconnect(in2);
    // step 21
    expect(read(0, nullData),
           count(0).level(Error).mainNull().inError(in2, InputError::Disconnected).msg("in2: not connected").size(in1, 0).size(in2, 0));
    // step 22
    expect([&] { return inputStatus(in9); }, fails(OPENDAQ_ERR_INVALIDPARAMETER).unchanged());
    // step 23
    expect(read(0, nullData), count(0).noChanges());
    connect(in2);  // s2 is in2's own signal
    // step 25
    expect(read(0, nullData),
           count(0).level(Ok).main(in1).domainChanged().resync().allDescChanged().healthy(in2).msg("").size(in1, 8).size(in2, 8));
    write(in1, 400, 499);
    write(in2, 400, 499);
    // step 28
    expect(readWithDomain(100), count(100).at(400).noChanges());
    write(in1, 500, 599);
    write(in2, 500, 549);
    disconnect(in2);
    connect(in2);  // s2 is in2's own signal
    write(in2, 550, 599);
    // step 34
    expect(readWithDomain(100), count(100).at(500).noChanges());
    // step 35
    expect(read(0, nullData), count(0).noChanges());
}

// S2.08
TEST_F(MultiReader2Scenario, RecorderDrainWithTimestampsGapAndPause)
{
    given().ports().policy(Exclude).minReadCount(10).differingPackets();  // DATA.PACKET.3 is among the scenario's values
    handler([&] { recorderDrain(); });  // the recorder's rows and segments carry no expectation of the row
    write(in1, 0, 99);
    write(in2, 0, 99);
    // step 5
    expect(idle(),
           calls(1,
                 {count(0).level(Ok).main(in1).domainChanged().resync().allDescChanged().size(in1, 8).size(in2, 8),
                  count(100).at(0).noChanges(),
                  count(0).noChanges()}));
    write(in1, 100, 199);
    write(in2, 100, 149);
    // step 8
    expect(idle(), calls(1, {count(50).at(100).noChanges(), count(0).noChanges()}));
    gap(in2);
    // step 10
    expect(idle(), calls(1, {count(0).noChanges()}));
    write(in2, 200, 249);
    write(in1, 200, 299);
    // step 13
    expect(idle(), calls(1, {count(0).resync(), count(50).at(200).noChanges(), count(0).noChanges()}));
    write(in2, 250, 299);
    // step 15
    expect(idle(), calls(1, {count(50).at(250).noChanges(), count(0).noChanges()}));
    write(in1, 300, 399);
    write(in2, 300, 304);
    announce(in2, Value(SampleType::Int32));
    // step 19
    expect(idle(), calls(1, {count(0).resync().descChanged(in2), count(0).noChanges()}));
    write(in2, 305, 399);
    // step 21
    expect(idle(), calls(1, {count(95).at(305).noChanges(), count(0).noChanges()}));
    pauseRecording();
    write(in1, 400, 499);
    write(in2, 400, 499);
    // step 25
    expect(idle(), calls(1, {count(100).at(400).noChanges(), count(0).noChanges()}));
    resumeRecording();
    disconnect(in2);
    // step 28
    expect(idle(),
           calls(1,
                 {count(0).level(Warning).inError(in2, InputError::Disconnected).msg("in2: not connected").size(in2, 0),
                  count(0).noChanges()}));
    write(in1, 500, 599);
    // step 30
    expect(idle(), calls(1, {count(100).at(500).noChanges(), count(0).noChanges()}));
    connect(in2);  // s2 is in2's own signal
    // step 32
    expect(idle(), calls(1, {count(0).level(Ok).descChanged(in2).healthy(in2).msg("").size(in2, 8), count(0).noChanges()}));
    write(in1, 600, 699);
    write(in2, 600, 699);
    // step 35
    expect(idle(), calls(1, {count(100).at(600).noChanges(), count(0).noChanges()}));
}

}
