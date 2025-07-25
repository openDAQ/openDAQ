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


using Daq.Core.OpenDAQ;
using Daq.Core.Types;

namespace Daq.Core.Objects;


/// <summary>Factory functions of the &apos;CoreObjects&apos; library.</summary>
/// <remarks>All functions are <c>static</c> so that you can import them directly by using the statement <c>using static Daq.Core.Objects.CoreObjectsFactory;</c></remarks>
public static partial class CoreObjectsFactory
{
    /// <summary>
    /// Initializes the openDAQ SDK.
    /// </summary>
    static CoreObjectsFactory()
    {
        // initialize the SDK (load all depending SDK libraries)
        _ = CoreTypesFactory.SdkVersion;
        _ = CoreObjectsFactory.SdkVersion;
    }

    //void daqCoreObjectsGetVersion(unsigned int* major, unsigned int* minor, unsigned int* revision); cdecl;
    [DllImport(CoreObjectsDllInfo.FileName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void daqCoreObjectsGetVersion(out int major, out int minor, out int revision);

    /// <summary>
    /// Gets the SDK version of the &apos;CoreObjects&apos; library.
    /// </summary>
    public static Version SdkVersion
    {
        get
        {
            //get the SDK version
            daqCoreObjectsGetVersion(out int major, out int minor, out int revision);

            //create and return object
            return new Version(major, minor, revision);
        }
    }
}
