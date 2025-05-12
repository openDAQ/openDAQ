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


//SearchFilter-Factory functions of the &apos;OpenDAQ&apos; library.
public static class SearchFactory
{
    /// <summary> Creates a search filter that accepts only visible components. "Visit children" returns <c>true</c>
    /// only if the component being evaluated is visible. </summary>
    public static SearchFilter Visible()
    {
        SearchFilter obj = OpenDAQFactory.CreateVisibleSearchFilter();
        return obj;
    }

    /// <summary> Creates a search filter that accepts components that have all the required tags. "Visit children"
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

    /// <summary> Creates a search filter that accepts components that do not have any of the excluded tags. "Visit children"
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

    ///// <summary> Creates a search filter that accepts components that implement the interface with the given interface ID. "Visit children"
    ///// always returns <c>true</c>.</summary>
    ///// <param name="intfId">The interface ID that should be implemented by accepted components.</param>
    //public static SearchFilter InterfaceId(IntfID intfId)
    //{
    //    SearchFilter obj = OpenDAQFactory.CreateInterfaceIdSearchFilter(intfId);
    //    return obj;
    //}

    /// <summary> Creates a search filter that accepts components with the specified local ID. "Visit children"
    /// always returns <c>true</c>.</summary>
    /// <param name="localId">The local ID of the accepted components.</param>
    public static SearchFilter LocalId(string localId)
    {
        SearchFilter obj = OpenDAQFactory.CreateLocalIdSearchFilter(localId);
        return obj;
    }

    /// <summary> Creates a search filter that accepts all components. "Visit children" always returns <c>true</c>.</summary>
    public static SearchFilter Any()
    {
        SearchFilter obj = OpenDAQFactory.CreateAnySearchFilter();
        return obj;
    }

    /// <summary> Creates a "conjunction" search filter that combines 2 filters, accepting a component only if both filters accept it.
    /// "Visit children" returns <c>true</c> only if both filters do so.</summary>
    /// <param name="left">The first argument of the conjunction operation.</param>
    /// <param name="right">The second argument of the conjunction operation.</param>
    public static SearchFilter And(SearchFilter left, SearchFilter right)
    {
        SearchFilter obj = OpenDAQFactory.CreateAndSearchFilter(left, right);
        return obj;
    }

    /// <summary> Creates a "disjunction" search filter that combines 2 filters, accepting a component if any of the two filters accepts it.
    /// "Visit children" returns <c>true</c> if any of the two filters accepts does so.</summary>
    /// <param name="left">The first argument of the disjunction operation.</param>
    /// <param name="right">The second argument of the disjunction operation.</param>
    public static SearchFilter Or(SearchFilter left, SearchFilter right)
    {
        SearchFilter obj = OpenDAQFactory.CreateOrSearchFilter(left, right);
        return obj;
    }

    /// <summary> Creates a search filter that negates the "accepts component" result of the filter provided as construction argument.
    /// Does not negate the "visit children" result.</summary>
    /// <param name="filter">The filter of which results should be negated.</param>
    public static SearchFilter Not(SearchFilter filter)
    {
        SearchFilter obj = OpenDAQFactory.CreateNotSearchFilter(filter);
        return obj;
    }

    ///// <summary> Creates a custom search filter with a user-defined "accepts" and "visit children" function.</summary>
    ///// <param name="acceptsFunction">The function to be called when "accepts component" is called. Should return <c>true</c> or <c>false</c>.</param>
    ///// <param name="visitFunction">The function to be called when "visit children" is called. Should return <c>true</c> or <c>false</c>.</param>
    //public static SearchFilter Custom(Function acceptsFunction, Function visitFunction = null)
    //{
    //    SearchFilter obj = OpenDAQFactory.CreateCustomSearchFilter(acceptsFunction, visitFunction);
    //    return obj;
    //}

    /// <summary> Creates a search filter that indicates that the tree traversal method should recursively search the tree.</summary>
    /// <remarks>This filter constructor should always be the final filter wrapper, and should not be used as a constructor argument
    /// for another filter.</remarks>
    /// <param name="filter">The filter to be wrapped with a "recursive" flag.</param>
    public static SearchFilter Recursive(SearchFilter filter)
    {
        SearchFilter obj = OpenDAQFactory.CreateRecursiveSearchFilter(filter);
        return obj;
    }
}
