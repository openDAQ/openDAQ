/*
 * Copyright 2022-2024 openDAQ d.o.o.
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


namespace Daq.Core.Types;


/// <summary>Helper functions of the .NET Bindings.</summary>
internal static class CoreTypesHelper
{
    #region Arrays

    //using ArrayPool for our GC handles and jagged array pointers
    private static readonly System.Buffers.ArrayPool<GCHandle> _GCHandlePool            = System.Buffers.ArrayPool<GCHandle>.Shared;
    private static readonly System.Buffers.ArrayPool<IntPtr>   _JaggedArrayPointersPool = System.Buffers.ArrayPool<IntPtr>.Shared;

    /// <summary>
    /// Pins the sub-arrays in a jagged array so that the GC will not move in memory until freed.
    /// </summary>
    /// <typeparam name="TValue">The type of the values.</typeparam>
    /// <param name="jaggedArray">The jagged array.</param>
    /// <param name="pinnedHandles">Returns the pinned handles.</param>
    /// <param name="pinnedSubArrayPointers">Returns the pinned sub-array pointers.</param>
    /// <returns><c>true</c> when the sub-arrays have been pinned; otherwise <c>false</c>.</returns>
    public static bool PinJaggedArray<TValue>(TValue[][] jaggedArray, out GCHandle[] pinnedHandles, out IntPtr[] pinnedSubArrayPointers)
    {
        pinnedHandles       = null;
        pinnedSubArrayPointers = null;

        if (jaggedArray == null)
            return false;

        int jaggedArrayCount = jaggedArray.Length;

        if (jaggedArrayCount == 0)
            return false;

        pinnedHandles          = _GCHandlePool.Rent(jaggedArrayCount);
        pinnedSubArrayPointers = _JaggedArrayPointersPool.Rent(jaggedArrayCount);

        for (int i = 0; i < jaggedArrayCount; ++i)
        {
            pinnedHandles[i]          = GCHandle.Alloc(jaggedArray[i], GCHandleType.Pinned);
            pinnedSubArrayPointers[i] = pinnedHandles[i].AddrOfPinnedObject();
        }

        return true;
    }

    /// <summary>
    /// Frees the previously pinned sub-arrays.
    /// </summary>
    /// <param name="pinnedHandles">The pinned handles (<c>null</c>> on return).</param>
    /// <param name="pinnedSubArrayPointers">The pinned sub-array pointers (<c>null</c>> on return).</param>
    public static void FreeJaggedArray(ref GCHandle[] pinnedHandles, ref IntPtr[] pinnedSubArrayPointers)
    {
        if (pinnedSubArrayPointers != null)
        {
            _JaggedArrayPointersPool.Return(pinnedSubArrayPointers);
            pinnedSubArrayPointers = null;
        }

        if (pinnedHandles != null)
        {
            foreach (var pinnedHandle in pinnedHandles)
            {
                try
                {
                    pinnedHandle.Free();
                }
                catch
                {
                    //ignored intentionally
                }
            }

            _GCHandlePool.Return(pinnedHandles);
            pinnedHandles = null;
        }
    }

    /// <summary>
    /// Gets the length of the smallest array in a jagged array.
    /// </summary>
    /// <typeparam name="TValue">The type of the values.</typeparam>
    /// <param name="jaggedArray">The jagged array.</param>
    /// <returns>The length of the smallest array.</returns>
    public static int GetSmallestArrayLength<TValue>(TValue[][] jaggedArray)
    {
        int minLength = int.MaxValue;

        foreach (var array in jaggedArray)
        {
            if (array == null)
                continue;

            if (array.Length < minLength)
            {
                minLength = array.Length;
            }
        }

        return (minLength != int.MaxValue) ? minLength : 0;
    }

    #endregion Arrays
}
