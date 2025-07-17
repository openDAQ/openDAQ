##
# Showcases the openDAQ search filters. Search filters can be used in tree traversal methods such as
# `folder.getItems` to filter out any results that do not match the filter criteria. This example
# uses them to traverse the openDAQ simulator.
##

# TODO: aliases for search filters, eg. daq.Any()

import sys
sys.path.append("..")

import opendaq as daq
import daq_utils

def print_component_list(list_):
    for component in list_:
        print(component.global_id)

if __name__ == "__main__":
    simulator = daq_utils.setup_simulator()

    # The following component search filters exist:
    # - Any(): Matches any component
    # - Visible(): Matches visible components
    # - And(left, right): True if both left and right filters match
    # - Or(left, right): True if either the left or right filter match
    # - Not(filter): Negates the result
    # - Recursive(filter): Makes the search recurse down the component tree. Must be the outermost filter.
    # - LocalId(id): Matches components with the given ID
    # - InterfaceId(id): Matches components that implement the given interface ID
    # - RequiredTags(tags): Matches components that have the specified tags
    # - ExcludedTags(tags): Matches components that do not have the specified tags
    # - Custom(function, visit_func): Custom search function. The function takes as input a component and returns a boolean.
    #                                 The visit_func should return True if the search recursion should continue.

    # TODO: Interface ID search filters are not usable in Python

    # Find all invisible components recursively
    items = simulator.get_items(daq.RecursiveSearchFilter(daq.NotSearchFilter(daq.VisibleSearchFilter())))
    print('\nAll invisible items:')
    print_component_list(items)

    # Find all components with the tag "DeviceDomain"
    tags = ['DeviceDomain']
    items = simulator.get_items(daq.RecursiveSearchFilter(daq.RequiredTagsSearchFilter(tags)))
    print('\nAll items with the tag \"DeviceDomain\":')
    print_component_list(items)

    # Find all components with the ID "FB" or "Dev"
    or_filter = daq.OrSearchFilter(daq.LocalIdSearchFilter('FB'), daq.LocalIdSearchFilter('Dev'))
    items = simulator.get_items(daq.RecursiveSearchFilter(or_filter))
    print('\nAll items with the local ID \"FB\" or \"Dev\":')
    print_component_list(items)


    # TODO: This does not seem to work
    # simulator.channels[0].active = False
    # func = daq.Function(lambda comp : daq.IComponent.cast_from(comp).active == False)
    # visit_func = daq.Function(lambda : True)
    #
    # items = simulator.get_items(daq.RecursiveSearchFilter(daq.CustomSearchFilter(func, visit_func)))
    # print('\nAll inactive items:')
    # print_component_list(items)

