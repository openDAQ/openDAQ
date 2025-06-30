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


/// <summary>Search-Factory functions of SearchFilters in all openDAQ libraries.</summary>
public static class SearchFactory
{
    #region CoreTypes library

    /// <summary>Creates a search filter that accepts all components. &quot;Visit children&quot; always returns <c>true</c>.</summary>
    public static SearchFilter Any()
    {
        SearchFilter obj = CoreTypesFactory.CreateAnySearchFilter();
        return obj;
    }

    /// <summary>Creates a &quot;conjunction&quot; search filter that combines 2 filters, accepting a component only if both filters accept it.
    /// &quot;Visit children&quot; returns <c>true</c> only if both filters do so.</summary>
    /// <param name="left">The first argument of the conjunction operation.</param>
    /// <param name="right">The second argument of the conjunction operation.</param>
    public static SearchFilter And(SearchFilter left, SearchFilter right)
    {
        SearchFilter obj = CoreTypesFactory.CreateAndSearchFilter(left, right);
        return obj;
    }

    /// <summary>Creates a &quot;disjunction&quot; search filter that combines 2 filters, accepting a component if any of the two filters accepts it.
    /// &quot;Visit children&quot; returns <c>true</c> if any of the two filters accepts does so.</summary>
    /// <param name="left">The first argument of the disjunction operation.</param>
    /// <param name="right">The second argument of the disjunction operation.</param>
    public static SearchFilter Or(SearchFilter left, SearchFilter right)
    {
        SearchFilter obj = CoreTypesFactory.CreateOrSearchFilter(left, right);
        return obj;
    }

    /// <summary>Creates a search filter that negates the &quot;accepts component&quot; result of the filter provided as construction argument.
    /// Does not negate the &quot;Visit children&quot; result.</summary>
    /// <param name="filter">The filter of which results should be negated.</param>
    public static SearchFilter Not(SearchFilter filter)
    {
        SearchFilter obj = CoreTypesFactory.CreateNotSearchFilter(filter);
        return obj;
    }

    ///// <summary>Creates a custom search filter with a user-defined &quot;accepts&quot; and &quot;Visit children&quot; function.</summary>
    ///// <param name="acceptsFunction">The function to be called when &quot;accepts component&quot; is called. Should return <c>true</c> or <c>false</c>.</param>
    ///// <param name="visitFunction">The function to be called when &quot;Visit children&quot; is called. Should return <c>true</c> or <c>false</c>.</param>
    //public static SearchFilter Custom(Function acceptsFunction, Function visitFunction = null)
    //{
    //    SearchFilter obj = CoreTypesFactory.CreateCustomSearchFilter(acceptsFunction, visitFunction);
    //    return obj;
    //}

    /// <summary>Creates a search filter that indicates that the tree traversal method should recursively search the tree.</summary>
    /// <remarks>This filter constructor should always be the final filter wrapper, and should not be used as a constructor argument
    /// for another filter.</remarks>
    /// <param name="filter">The filter to be wrapped with a &quot;recursive&quot; flag.</param>
    public static SearchFilter Recursive(SearchFilter filter)
    {
        SearchFilter obj = CoreTypesFactory.CreateRecursiveSearchFilter(filter);
        return obj;
    }

    #endregion CoreTypes library

    #region CoreObjects library

    /// <summary>Creates a search filter that accepts only visible properties.</summary>
    /// <returns>The &apos;SearchFilter&apos; object.</returns>
    public static SearchFilter VisibleProperty()
    {
        SearchFilter obj = CoreObjectsFactory.CreateVisiblePropertyFilter();
        return obj;
    }

    /// <summary>Creates a search filter that accepts only read-only properties.</summary>
    /// <returns>The &apos;SearchFilter&apos; object.</returns>
    public static SearchFilter ReadOnlyProperty()
    {
        SearchFilter obj = CoreObjectsFactory.CreateReadOnlyPropertyFilter();
        return obj;
    }

    /// <summary>Creates a search filter that accepts properties whose names match the specified pattern.</summary>
    /// <returns>The &apos;SearchFilter&apos; object.</returns>
    /// <param name="regex">A regular expression pattern used to match property names.</param>
    public static SearchFilter NameProperty(string regex)
    {
        SearchFilter obj = CoreObjectsFactory.CreateNamePropertyFilter(regex);
        return obj;
    }

    #endregion CoreObjects library

    #region OpenDAQ library

    /// <summary>Creates a search filter that accepts only visible components. &quot;Visit children&quot; returns <c>true</c>
    /// only if the component being evaluated is visible. </summary>
    public static SearchFilter Visible()
    {
        SearchFilter obj = OpenDAQFactory.CreateVisibleSearchFilter();
        return obj;
    }

    /// <summary>Creates a search filter that accepts components that have all the required tags. &quot;Visit children&quot;
    /// always returns <c>true</c>.</summary>
    /// <param name="requiredTags">A list of strings containing the tags that a component must have to be accepted.</param>
    public static SearchFilter RequireTags(params string[] requiredTags)
    {
        var filterTags = CoreTypesFactory.CreateList<BaseObject>();
        foreach (var tag in requiredTags)
        {
            filterTags.Add(tag);
        }
        SearchFilter obj = OpenDAQFactory.CreateRequiredTagsSearchFilter(filterTags);
        return obj;
    }

    /// <summary>Creates a search filter that accepts components that do not have any of the excluded tags. &quot;Visit children&quot;
    /// always returns <c>true</c>.</summary>
    /// <param name="excludedTags">A list of strings containing the tags that a component must not have to be accepted.</param>
    public static SearchFilter ExcludeTags(IList<string> excludedTags)
    {
        var filterTags = CoreTypesFactory.CreateList<BaseObject>();
        foreach (var tag in excludedTags)
        {
            filterTags.Add(tag);
        }
        SearchFilter obj = OpenDAQFactory.CreateExcludedTagsSearchFilter(filterTags);
        return obj;
    }

    /// <summary>Creates a search filter that accepts components that implement the interface with the given interface ID. &quot;Visit children&quot;
    /// always returns <c>true</c>.</summary>
    /// <param name="intfId">The interface ID that should be implemented by accepted components.</param>
    public static SearchFilter InterfaceId(Guid intfId)
    {
        SearchFilter obj = OpenDAQFactory.CreateInterfaceIdSearchFilter(intfId);
        return obj;
    }

    /// <summary>Creates a search filter that accepts components with the specified local ID. &quot;Visit children&quot;
    /// always returns <c>true</c>.</summary>
    /// <param name="localId">The local ID of the accepted components.</param>
    public static SearchFilter LocalId(string localId)
    {
        SearchFilter obj = OpenDAQFactory.CreateLocalIdSearchFilter(localId);
        return obj;
    }

    #endregion OpenDAQ library
}
