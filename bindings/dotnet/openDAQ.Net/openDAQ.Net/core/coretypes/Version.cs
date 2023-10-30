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


namespace Daq.Core.Types;


public class OpenDaqVersion
{
    public OpenDaqVersion(uint major, uint minor, uint revision)
    {
        this.Major = major;
        this.Minor = minor;
        this.Revision = revision;
    }

    public uint Major { get; }
    public uint Minor { get; }
    public uint Revision { get; }
}


#region Class Factory

public static partial class CoreTypesFactory
{
    [DllImport(CoreTypesDllInfo.FileName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void daqCoreTypesGetVersion(ref uint major, ref uint minor, ref uint revision);

    public static OpenDaqVersion GetVersion()
    {
        uint major = 0;
        uint minor = 0;
        uint revision = 0;

        daqCoreTypesGetVersion(ref major, ref minor, ref revision);
        OpenDaqVersion version = new OpenDaqVersion(major, minor, revision);
        return version;
    }
}

#endregion Class Factory
