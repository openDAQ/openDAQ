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


using Daq.Core.Types;


namespace Daq.Core.Objects;


// SearchFilter factory functions of the &apos;CoreObjects&apos; library.
public static partial class CoreObjectsFactory
{
    //ErrorCode createVisiblePropertyFilter(daq.ISearchFilter** obj); cdecl;
    [DllImport(CoreObjectsDllInfo.FileName, CallingConvention = CallingConvention.Cdecl)]
    private static extern ErrorCode createVisiblePropertyFilter(out IntPtr obj);

    /// <summary>Creates a search filter that accepts only visible properties.</summary>
    /// <param name="obj">The &apos;SearchFilter&apos; object.</param>
    public static ErrorCode CreateVisiblePropertyFilter(out SearchFilter obj)
    {
        //initialize output argument
        obj = default;

        //native output argument
        IntPtr objPtr;

        //call native function
        ErrorCode errorCode = createVisiblePropertyFilter(out objPtr);

        if (Result.Succeeded(errorCode))
        {
            //create object
            obj = new SearchFilter(objPtr, incrementReference: false);
        }

        return errorCode;
    }

    /// <summary>Creates a search filter that accepts only visible properties.</summary>
    /// <returns>The &apos;SearchFilter&apos; object.</returns>
    public static SearchFilter CreateVisiblePropertyFilter()
    {
        //native output argument
        IntPtr objPtr;

        //call native function
        ErrorCode errorCode = createVisiblePropertyFilter(out objPtr);

        if (Result.Failed(errorCode))
        {
            throw new OpenDaqException(errorCode);
        }

        //create and return object
        return new SearchFilter(objPtr, incrementReference: false);
    }


    //ErrorCode createReadOnlyPropertyFilter(daq.ISearchFilter** obj); cdecl;
    [DllImport(CoreObjectsDllInfo.FileName, CallingConvention = CallingConvention.Cdecl)]
    private static extern ErrorCode createReadOnlyPropertyFilter(out IntPtr obj);

    /// <summary>Creates a search filter that accepts only read-only properties.</summary>
    /// <param name="obj">The &apos;SearchFilter&apos; object.</param>
    public static ErrorCode CreateReadOnlyPropertyFilter(out SearchFilter obj)
    {
        //initialize output argument
        obj = default;

        //native output argument
        IntPtr objPtr;

        //call native function
        ErrorCode errorCode = createReadOnlyPropertyFilter(out objPtr);

        if (Result.Succeeded(errorCode))
        {
            //create object
            obj = new SearchFilter(objPtr, incrementReference: false);
        }

        return errorCode;
    }

    /// <summary>Creates a search filter that accepts only read-only properties.</summary>
    /// <returns>The &apos;SearchFilter&apos; object.</returns>
    public static SearchFilter CreateReadOnlyPropertyFilter()
    {
        //native output argument
        IntPtr objPtr;

        //call native function
        ErrorCode errorCode = createReadOnlyPropertyFilter(out objPtr);

        if (Result.Failed(errorCode))
        {
            throw new OpenDaqException(errorCode);
        }

        //create and return object
        return new SearchFilter(objPtr, incrementReference: false);
    }

    //ErrorCode createNamePropertyFilter(daq.ISearchFilter** obj, daq.IString* regex); cdecl;
    [DllImport(CoreObjectsDllInfo.FileName, CallingConvention = CallingConvention.Cdecl)]
    private static extern ErrorCode createNamePropertyFilter(out IntPtr obj, IntPtr regex);

    /// <summary>Creates a search filter that accepts properties whose names match the specified pattern.</summary>
    /// <param name="obj">The &apos;SearchFilter&apos; object.</param>
    /// <param name="regex">A regular expression pattern used to match property names.</param>
    public static ErrorCode CreateNamePropertyFilter(out SearchFilter obj, string regex)
    {
        //initialize output argument
        obj = default;

        //native output argument
        IntPtr objPtr;

        //cast .NET argument to SDK object
        using var regexPtr = (StringObject)regex;

        //call native function
        ErrorCode errorCode = createNamePropertyFilter(out objPtr, regexPtr);

        if (Result.Succeeded(errorCode))
        {
            //create object
            obj = new SearchFilter(objPtr, incrementReference: false);
        }

        return errorCode;
    }

    /// <summary>Creates a search filter that accepts properties whose names match the specified pattern.</summary>
    /// <returns>The &apos;SearchFilter&apos; object.</returns>
    /// <param name="regex">A regular expression pattern used to match property names.</param>
    public static SearchFilter CreateNamePropertyFilter(string regex)
    {
        //native output argument
        IntPtr objPtr;

        //cast .NET argument to SDK object
        using var regexPtr = (StringObject)regex;

        //call native function
        ErrorCode errorCode = createNamePropertyFilter(out objPtr, regexPtr);

        if (Result.Failed(errorCode))
        {
            throw new OpenDaqException(errorCode);
        }

        //create and return object
        return new SearchFilter(objPtr, incrementReference: false);
    }
}
