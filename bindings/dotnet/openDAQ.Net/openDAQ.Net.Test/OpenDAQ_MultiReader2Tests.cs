// Ignore Spelling: daqref

using Daq.Core.Types;
using Daq.Core.OpenDAQ;


namespace openDaq.Net.Test;


public class OpenDAQ_MultiReader2Tests : OpenDAQTestsBase
{
    [SetUp]
    public void Setup()
    {
        base.DontWarn();
    }

    private static Signal[] RefSignals(Instance instance, int count)
    {
        var device = instance.AddDevice("daqref://device0");
        var signals = device.GetChannels()
                            .SelectMany(channel => channel.GetSignals())
                            .Where(signal => signal.DomainSignal != null)
                            .Take(count)
                            .ToArray();
        Assert.That(signals, Has.Length.EqualTo(count));
        return signals;
    }

    private static bool WaitUntil(Func<bool> predicate, int timeoutMs = 3000)
    {
        var deadline = DateTime.UtcNow.AddMilliseconds(timeoutMs);
        while (!predicate())
        {
            if (DateTime.UtcNow > deadline)
                return false;
            Thread.Sleep(5);
        }
        return true;
    }

    private static MultiReader2Status ReadFirstStatus<TValue>(MultiReader2<TValue> reader)
        where TValue : unmanaged
    {
        nuint count = 100;
        var status = reader.Read(reader.CreateBuffers(count), ref count, out _);
        Assert.That(count, Is.EqualTo((nuint)0));
        Assert.That(status.HasChanges, Is.True);
        return status;
    }

    [Test]
    public void FirstReadReturnsTheConstructionStatus()
    {
        using var instance = OpenDAQFactory.Instance();
        var signals = RefSignals(instance, 2);
        var reader = OpenDAQFactory.CreateMultiReader2<double>(OpenDAQFactory.CreateMultiReader2Params(signals));

        Assert.That(reader.ElementsPerSample, Is.Empty);
        Assert.That(reader.CreateBuffers(10), Is.Empty);

        var status = ReadFirstStatus(reader);
        Assert.That(status.Status, Is.Not.EqualTo(ComponentStatus.Error));
        Assert.That(reader.ElementsPerSample, Is.EqualTo(new[] { 1, 1 }));
    }

    [Test]
    public void ReadsAlignedSamplesWithTicks()
    {
        using var instance = OpenDAQFactory.Instance();
        var signals = RefSignals(instance, 2);
        var reader = OpenDAQFactory.CreateMultiReader2<double>(OpenDAQFactory.CreateMultiReader2Params(signals));
        ReadFirstStatus(reader);

        Assert.That(WaitUntil(() => reader.AvailableCount >= 10), Is.True);

        nuint count = 10;
        var data = reader.CreateBuffers(count);
        var ticks = new long[count];
        var status = reader.Read(data, ref count, out long packetOffset, ticks);

        Assert.That(status.HasChanges, Is.False);
        Assert.That(count, Is.EqualTo((nuint)10));
        Assert.That(data.Select(buffer => buffer.Length), Is.All.EqualTo(10));
        long delta = ticks[1] - ticks[0];
        Assert.That(delta, Is.GreaterThan(0));
        for (int i = 1; i < ticks.Length; ++i)
            Assert.That(ticks[i] - ticks[i - 1], Is.EqualTo(delta));

        Assert.That(WaitUntil(() => reader.AvailableCount >= 1), Is.True);
        nuint next = 1;
        reader.SkipSamples(ref next, 0, out long nextOffset);
        Assert.That(next, Is.EqualTo((nuint)1));
        Assert.That(nextOffset, Is.EqualTo(packetOffset + 10 * delta));
    }

    [Test]
    public void ReadTypeFollowsTValue()
    {
        using var instance = OpenDAQFactory.Instance();
        var signals = RefSignals(instance, 2);
        var @params = OpenDAQFactory.CreateMultiReader2Params(signals);
        var reader = OpenDAQFactory.CreateMultiReader2<int>(@params);

        Assert.That(@params.ValueReadType, Is.EqualTo(SampleType.Int32));
        ReadFirstStatus(reader);
        Assert.That(reader.ElementsPerSample, Is.EqualTo(new[] { 1, 1 }));

        reader.Configure(@params);
        Assert.That(@params.ValueReadType, Is.EqualTo(SampleType.Int32));
    }

    [Test]
    public void NonNumericTypeIsRejected()
    {
        using var instance = OpenDAQFactory.Instance();
        var @params = OpenDAQFactory.CreateMultiReader2Params(RefSignals(instance, 1));

        Assert.Throws<ArgumentException>(() => OpenDAQFactory.CreateMultiReader2<char>(@params));
    }

    [Test]
    public void BuffersAreCheckedBeforeTheRead()
    {
        using var instance = OpenDAQFactory.Instance();
        var signals = RefSignals(instance, 2);
        var reader = OpenDAQFactory.CreateMultiReader2<double>(OpenDAQFactory.CreateMultiReader2Params(signals));
        ReadFirstStatus(reader);
        Assert.That(WaitUntil(() => reader.AvailableCount >= 10), Is.True);

        var missing = reader.CreateBuffers(10);
        missing[1] = null;
        nuint count = 10;
        Assert.Throws<ArgumentException>(() => reader.Read(missing, ref count, out _));
        Assert.Throws<ArgumentException>(() => reader.Read(new double[1][], ref count, out _));

        var shortBuffers = reader.CreateBuffers(4);
        count = 10;
        reader.Read(shortBuffers, ref count, out _);
        Assert.That(count, Is.EqualTo((nuint)4));
    }

    [Test]
    public void ConfigureUpdatesTheBufferLayout()
    {
        using var instance = OpenDAQFactory.Instance();
        var signals = RefSignals(instance, 2);
        var reader = OpenDAQFactory.CreateMultiReader2<double>(OpenDAQFactory.CreateMultiReader2Params(signals));
        ReadFirstStatus(reader);

        var status = reader.Configure(OpenDAQFactory.CreateMultiReader2Params(signals.Take(1)));

        Assert.That(status.Inputs.Count, Is.EqualTo(1));
        Assert.That(reader.ElementsPerSample, Is.EqualTo(new[] { 1 }));
    }

    [Test]
    public void Demo()
    {
        using var instance = OpenDAQFactory.Instance();
        var signals = RefSignals(instance, 2);

        var @params = OpenDAQFactory.CreateMultiReader2Params(signals);
        @params.ErrorPolicy = MultiReader2ErrorPolicy.Exclude;
        var reader = OpenDAQFactory.CreateMultiReader2<double>(@params);

        var data = reader.CreateBuffers(100);
        for (int i = 0; i < 20; ++i)
        {
            Thread.Sleep(50);

            nuint count = 100;
            var status = reader.Read(data, ref count, out long packetOffset);
            if (count > 0)
                Console.WriteLine($"{count} samples at tick {packetOffset}: {data[0][0]:F3} | {data[1][0]:F3}");
            if (status.HasChanges)
            {
                Console.WriteLine($"status {status.Status}: {status.StatusMessage}");
                data = reader.CreateBuffers(100);
            }
        }
    }
}
