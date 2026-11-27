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


using Daq.Core.Types;


namespace Daq.Core.OpenDAQ;


/// <summary>
/// A <see cref="MultiReader2"/> that reads into <c>TValue[][]</c> buffers, one per input in slot order.
/// </summary>
/// <remarks>
/// The value read type is the <see cref="SampleType"/> of <typeparamref name="TValue"/>. The buffer sizes follow the
/// last status with changes: a read never crosses such a status, so the samples it returns follow the descriptors
/// reported before it. Until the first status arrives a read is a 0-count status probe.
/// Configure, reevaluate and skip through this class, not through a <see cref="MultiReader2"/> reference, so it sees
/// every status.
/// </remarks>
public sealed class MultiReader2<TValue> : MultiReader2
    where TValue : unmanaged
{
    private readonly object _sync = new();
    private int[] _elementsPerSample;

    internal MultiReader2(IntPtr nativePointer, bool incrementReference)
        : base(nativePointer, incrementReference)
    {
    }

    /// <summary>
    /// The number of <typeparamref name="TValue"/> elements in one sample of each input, in slot order; 0 for an input
    /// that a read does not write. Empty until the first status.
    /// </summary>
    public int[] ElementsPerSample
    {
        get
        {
            lock (_sync)
                return _elementsPerSample?.ToArray() ?? Array.Empty<int>();
        }
    }

    /// <summary>
    /// Allocates one buffer of <paramref name="count"/> samples per input that a read writes, null for the others.
    /// Allocate again after a status with changes.
    /// </summary>
    public TValue[][] CreateBuffers(nuint count)
    {
        return ElementsPerSample.Select(elements => elements > 0 ? new TValue[(int)count * elements] : null).ToArray();
    }

    /// <summary>Applies the params as <see cref="MultiReader2.Configure"/>, with the value read type set to the type of <typeparamref name="TValue"/>.</summary>
    public new MultiReader2Status Configure(MultiReader2Params @params)
    {
        @params.ValueReadType = OpenDAQFactory.GetMultiReader2ReadType<TValue>();
        lock (_sync)
            return Track(base.Configure(@params), always: true);
    }

    /// <inheritdoc cref="MultiReader2.Reevaluate"/>
    public new MultiReader2Status Reevaluate()
    {
        lock (_sync)
            return Track(base.Reevaluate(), always: true);
    }

    /// <inheritdoc cref="MultiReader2.SkipSamples"/>
    public new MultiReader2Status SkipSamples(ref nuint count, nuint timeoutMs, out long packetOffset)
    {
        lock (_sync)
            return Track(base.SkipSamples(ref count, timeoutMs, out packetOffset), always: false);
    }

    /// <summary>
    /// Reads up to <paramref name="count"/> aligned samples into <paramref name="data"/>, as <see cref="MultiReader2.Read"/>.
    /// </summary>
    /// <param name="data">One buffer per input in slot order; null for an input a read does not write (see <see cref="ElementsPerSample"/>).</param>
    /// <param name="count">In: the requested count, 0 or at least MinReadCount, lowered to what the buffers hold; out: the count read.</param>
    /// <param name="packetOffset">The main tick of the first sample, relative to the main domain rule start.</param>
    /// <param name="domain">Optional buffer for the main input's domain ticks.</param>
    /// <returns>The status of this read.</returns>
    public MultiReader2Status Read(TValue[][] data, ref nuint count, out long packetOffset, long[] domain = null)
    {
        lock (_sync)
        {
            var elementsPerSample = _elementsPerSample;
            if (elementsPerSample == null)
                count = 0;
            else if (count > 0)
                count = Capacity(data, domain, elementsPerSample, count);

            var handles = new List<GCHandle>();
            try
            {
                var pointers = new IntPtr[elementsPerSample?.Length ?? 0];
                for (int i = 0; count > 0 && i < pointers.Length; ++i)
                {
                    if (elementsPerSample[i] == 0)
                        continue;
                    var handle = GCHandle.Alloc(data[i], GCHandleType.Pinned);
                    handles.Add(handle);
                    pointers[i] = handle.AddrOfPinnedObject();
                }

                IntPtr domainPtr = IntPtr.Zero;
                if (count > 0 && domain != null)
                {
                    var handle = GCHandle.Alloc(domain, GCHandleType.Pinned);
                    handles.Add(handle);
                    domainPtr = handle.AddrOfPinnedObject();
                }

                unsafe
                {
                    fixed (IntPtr* pointersPtr = pointers)
                    {
                        var dataPtr = count > 0 ? (IntPtr)pointersPtr : IntPtr.Zero;
                        return Track(base.Read(dataPtr, domainPtr, ref count, 0, out packetOffset), always: false);
                    }
                }
            }
            finally
            {
                foreach (var handle in handles)
                    handle.Free();
            }
        }
    }

    private static nuint Capacity(TValue[][] data, long[] domain, int[] elementsPerSample, nuint count)
    {
        if (data == null || data.Length < elementsPerSample.Length)
            throw new ArgumentException($"Expected one buffer per input ({elementsPerSample.Length}).", nameof(data));

        for (int i = 0; i < elementsPerSample.Length; ++i)
        {
            if (elementsPerSample[i] == 0)
                continue;
            if (data[i] == null)
                throw new ArgumentException($"No buffer for input {i}, which the read writes.", nameof(data));
            count = Math.Min(count, (nuint)(data[i].Length / elementsPerSample[i]));
        }

        if (domain != null)
            count = Math.Min(count, (nuint)domain.Length);
        return count;
    }

    private MultiReader2Status Track(MultiReader2Status status, bool always)
    {
        if (always || _elementsPerSample == null || status.HasChanges)
            _elementsPerSample = ElementsOf(status);
        return status;
    }

    private static int[] ElementsOf(MultiReader2Status status)
    {
        int size = Marshal.SizeOf<TValue>();
        bool valid = status.Status != ComponentStatus.Error;
        using var inputs = status.Inputs;
        var elements = new int[inputs.Count];
        for (int i = 0; i < elements.Length; ++i)
        {
            using var input = inputs[i];
            if (valid && input.Error == MultiReader2InputError.None)
                elements[i] = (int)input.SampleSize / size;
        }
        return elements;
    }
}


public static partial class OpenDAQFactory
{
    /// <summary>
    /// Creates a <see cref="MultiReader2{TValue}"/>, with the value read type of <paramref name="params"/> set to the
    /// type of <typeparamref name="TValue"/>.
    /// </summary>
    public static MultiReader2<TValue> CreateMultiReader2<TValue>(MultiReader2Params @params)
        where TValue : unmanaged
    {
        @params.ValueReadType = GetMultiReader2ReadType<TValue>();

        ErrorCode errorCode = createMultiReader2(out IntPtr objPtr, @params);
        if (Result.Failed(errorCode))
            throw new OpenDaqException(errorCode);

        return new MultiReader2<TValue>(objPtr, incrementReference: false);
    }

    /// <summary>Creates <see cref="MultiReader2Params"/> with the given inputs in slot order: all signals or all input ports.</summary>
    public static MultiReader2Params CreateMultiReader2Params(IEnumerable<Component> inputs)
    {
        var @params = CreateMultiReader2Params();
        @params.Inputs = CoreTypesFactory.CreateList(inputs.ToArray());
        return @params;
    }

    internal static SampleType GetMultiReader2ReadType<TValue>()
        where TValue : unmanaged
    {
        var sampleType = GetSampleType<TValue>();
        if (sampleType == SampleType.Undefined)
            throw new ArgumentException($"{typeof(TValue).Name} is not a numeric sample type.", nameof(TValue));
        return sampleType;
    }
}
