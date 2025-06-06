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
using Daq.Core.Objects;


namespace Daq.Core.OpenDAQ;


// SearchFilter factory functions of the &apos;OpenDAQ&apos; library.
public static partial class OpenDAQFactory
{
    //ErrorCode createVisibleSearchFilter(daq.ISearchFilter** obj); cdecl;
    [DllImport(OpenDAQDllInfo.FileName, CallingConvention = CallingConvention.Cdecl)]
    private static extern ErrorCode createVisibleSearchFilter(out IntPtr obj);

    /// <summary>
    /// Creates a search filter that accepts only visible components. &quot;Visit children&quot; returns <c>true</c>
    /// only if the component being evaluated is visible.
    /// </summary>
    /// <param name="obj">The &apos;SearchFilter&apos; object.</param>
    public static ErrorCode CreateVisibleSearchFilter(out SearchFilter obj)
    {
        //initialize output argument
        obj = default;

        //native output argument
        IntPtr objPtr;

        //call native function
        ErrorCode errorCode = createVisibleSearchFilter(out objPtr);

        if (Result.Succeeded(errorCode))
        {
            //create object
            obj = new SearchFilter(objPtr, incrementReference: false);
        }

        return errorCode;
    }

    /// <summary>
    /// Creates a search filter that accepts only visible components. &quot;Visit children&quot; returns <c>true</c>
    /// only if the component being evaluated is visible.
    /// </summary>
    /// <returns>The &apos;SearchFilter&apos; object.</returns>
    public static SearchFilter CreateVisibleSearchFilter()
    {
        //native output argument
        IntPtr objPtr;

        //call native function
        ErrorCode errorCode = createVisibleSearchFilter(out objPtr);

        if (Result.Failed(errorCode))
        {
            throw new OpenDaqException(errorCode);
        }

        //create and return object
        return new SearchFilter(objPtr, incrementReference: false);
    }


    //ErrorCode createRequiredTagsSearchFilter(daq.ISearchFilter** obj, daq.IList* requiredTags); cdecl;
    [DllImport(OpenDAQDllInfo.FileName, CallingConvention = CallingConvention.Cdecl)]
    private static extern ErrorCode createRequiredTagsSearchFilter(out IntPtr obj, IntPtr requiredTags);

    /// <summary>
    /// Creates a search filter that accepts components that have all the required tags. &quot;Visit children&quot;
    /// always returns <c>true</c>.
    /// </summary>
    /// <param name="obj">The &apos;SearchFilter&apos; object.</param>
    /// <param name="requiredTags">A list of strings containing the tags that a component must have to be accepted.</param>
    public static ErrorCode CreateRequiredTagsSearchFilter(out SearchFilter obj, IListObject<BaseObject> requiredTags)
    {
        //initialize output argument
        obj = default;

        //native output argument
        IntPtr objPtr;

        //cast .NET argument to SDK object
        var requiredTagsPtr = (ListObject<BaseObject>)requiredTags;

        //call native function
        ErrorCode errorCode = createRequiredTagsSearchFilter(out objPtr, requiredTagsPtr);

        if (Result.Succeeded(errorCode))
        {
            //create object
            obj = new SearchFilter(objPtr, incrementReference: false);
        }

        return errorCode;
    }

    /// <summary>
    /// Creates a search filter that accepts components that have all the required tags. &quot;Visit children&quot;
    /// always returns <c>true</c>.
    /// </summary>
    /// <returns>The &apos;SearchFilter&apos; object.</returns>
    /// <param name="requiredTags">A list of strings containing the tags that a component must have to be accepted.</param>
    public static SearchFilter CreateRequiredTagsSearchFilter(IListObject<BaseObject> requiredTags)
    {
        //native output argument
        IntPtr objPtr;

        //cast .NET argument to SDK object
        var requiredTagsPtr = (ListObject<BaseObject>)requiredTags;

        //call native function
        ErrorCode errorCode = createRequiredTagsSearchFilter(out objPtr, requiredTagsPtr);

        if (Result.Failed(errorCode))
        {
            throw new OpenDaqException(errorCode);
        }

        //create and return object
        return new SearchFilter(objPtr, incrementReference: false);
    }


    //ErrorCode createExcludedTagsSearchFilter(daq.ISearchFilter** obj, daq.IList* excludedTags); cdecl;
    [DllImport(OpenDAQDllInfo.FileName, CallingConvention = CallingConvention.Cdecl)]
    private static extern ErrorCode createExcludedTagsSearchFilter(out IntPtr obj, IntPtr excludedTags);

    /// <summary>
    /// Creates a search filter that accepts components that do not have any of the excluded tags. &quot;Visit children&quot;
    /// always returns <c>true</c>.
    /// </summary>
    /// <param name="obj">The &apos;SearchFilter&apos; object.</param>
    /// <param name="excludedTags">A list of strings containing the tags that a component must not have to be accepted.</param>
    public static ErrorCode CreateExcludedTagsSearchFilter(out SearchFilter obj, IListObject<BaseObject> excludedTags)
    {
        //initialize output argument
        obj = default;

        //native output argument
        IntPtr objPtr;

        //cast .NET argument to SDK object
        var excludedTagsPtr = (ListObject<BaseObject>)excludedTags;

        //call native function
        ErrorCode errorCode = createExcludedTagsSearchFilter(out objPtr, excludedTagsPtr);

        if (Result.Succeeded(errorCode))
        {
            //create object
            obj = new SearchFilter(objPtr, incrementReference: false);
        }

        return errorCode;
    }

    /// <summary>
    /// Creates a search filter that accepts components that do not have any of the excluded tags. &quot;Visit children&quot;
    /// always returns <c>true</c>.
    /// </summary>
    /// <returns>The &apos;SearchFilter&apos; object.</returns>
    /// <param name="excludedTags">A list of strings containing the tags that a component must not have to be accepted.</param>
    public static SearchFilter CreateExcludedTagsSearchFilter(IListObject<BaseObject> excludedTags)
    {
        //native output argument
        IntPtr objPtr;

        //cast .NET argument to SDK object
        var excludedTagsPtr = (ListObject<BaseObject>)excludedTags;

        //call native function
        ErrorCode errorCode = createExcludedTagsSearchFilter(out objPtr, excludedTagsPtr);

        if (Result.Failed(errorCode))
        {
            throw new OpenDaqException(errorCode);
        }

        //create and return object
        return new SearchFilter(objPtr, incrementReference: false);
    }


    //ErrorCode createInterfaceIdSearchFilter(daq.ISearchFilter** obj, const daq.core.types.IntfID& intfId); cdecl;
    [DllImport(OpenDAQDllInfo.FileName, CallingConvention = CallingConvention.Cdecl)]
    private static extern ErrorCode createInterfaceIdSearchFilter(out IntPtr obj, Guid intfId);

    /// <summary>
    /// Creates a search filter that accepts components that implement the interface with the given interface ID. &quot;Visit children&quot;
    /// always returns <c>true</c>.
    /// </summary>
    /// <param name="obj">The &apos;SearchFilter&apos; object.</param>
    /// <param name="intfId">The interface ID that should be implemented by accepted components.</param>
    public static ErrorCode CreateInterfaceIdSearchFilter(out SearchFilter obj, Guid intfId)
    {
        //initialize output argument
        obj = default;

        //native output argument
        IntPtr objPtr;

        //call native function
        ErrorCode errorCode = createInterfaceIdSearchFilter(out objPtr, intfId);

        if (Result.Succeeded(errorCode))
        {
            //create object
            obj = new SearchFilter(objPtr, incrementReference: false);
        }

        return errorCode;
    }

    /// <summary>
    /// Creates a search filter that accepts components that implement the interface with the given interface ID. &quot;Visit children&quot;
    /// always returns <c>true</c>.
    /// </summary>
    /// <returns>The &apos;SearchFilter&apos; object.</returns>
    /// <param name="intfId">The interface ID that should be implemented by accepted components.</param>
    public static SearchFilter CreateInterfaceIdSearchFilter(Guid intfId)
    {
        //native output argument
        IntPtr objPtr;

        //call native function
        ErrorCode errorCode = createInterfaceIdSearchFilter(out objPtr, intfId);

        if (Result.Failed(errorCode))
        {
            throw new OpenDaqException(errorCode);
        }

        //create and return object
        return new SearchFilter(objPtr, incrementReference: false);
    }


    //ErrorCode createLocalIdSearchFilter(daq.ISearchFilter** obj, daq.IString* localId); cdecl;
    [DllImport(OpenDAQDllInfo.FileName, CallingConvention = CallingConvention.Cdecl)]
    private static extern ErrorCode createLocalIdSearchFilter(out IntPtr obj, IntPtr localId);

    /// <summary>
    /// Creates a search filter that accepts components with the specified local ID. &quot;Visit children&quot;
    /// always returns <c>true</c>.
    /// </summary>
    /// <param name="obj">The &apos;SearchFilter&apos; object.</param>
    /// <param name="localId">The local ID of the accepted components.</param>
    public static ErrorCode CreateLocalIdSearchFilter(out SearchFilter obj, string localId)
    {
        //initialize output argument
        obj = default;

        //native output argument
        IntPtr objPtr;

        //cast .NET argument to SDK object
        using var localIdPtr = (StringObject)localId;

        //call native function
        ErrorCode errorCode = createLocalIdSearchFilter(out objPtr, localIdPtr);

        if (Result.Succeeded(errorCode))
        {
            //create object
            obj = new SearchFilter(objPtr, incrementReference: false);
        }

        return errorCode;
    }

    /// <summary>
    /// Creates a search filter that accepts components with the specified local ID. &quot;Visit children&quot;
    /// always returns <c>true</c>.
    /// </summary>
    /// <returns>The &apos;SearchFilter&apos; object.</returns>
    /// <param name="localId">The local ID of the accepted components.</param>
    public static SearchFilter CreateLocalIdSearchFilter(string localId)
    {
        //native output argument
        IntPtr objPtr;

        //cast .NET argument to SDK object
        using var localIdPtr = (StringObject)localId;

        //call native function
        ErrorCode errorCode = createLocalIdSearchFilter(out objPtr, localIdPtr);

        if (Result.Failed(errorCode))
        {
            throw new OpenDaqException(errorCode);
        }

        //create and return object
        return new SearchFilter(objPtr, incrementReference: false);
    }
}
