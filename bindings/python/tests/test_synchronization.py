#!/usr/bin/env python

import opendaq_test
import opendaq
import unittest


class TestSynchronization(opendaq_test.TestCase):

    def test_clock_interface_is_the_default_source(self):
        instance = opendaq.Instance()
        sync = opendaq.Synchronization(instance.context.type_manager, 'testDevice')

        self.assertEqual(list(sync.interfaces.keys()), ['ClockSyncInterface'])
        self.assertEqual(list(sync.available_sources.keys()), ['ClockSyncInterface'])
        self.assertEqual(list(sync.reference_domain_ids), ['local:testDevice'])

        source = sync.source
        self.assertEqual(source.id, 'ClockSyncInterface')
        self.assertEqual(source.sync_type, 'local')
        self.assertEqual(source.reference_domain_id, 'local:testDevice')
        self.assertTrue(source.source_supported)
        self.assertEqual(source.mode, opendaq.SyncMode.Input)
        self.assertEqual(len(source.available_modes), 1)

    def test_clock_interface_statuses(self):
        instance = opendaq.Instance()
        sync = opendaq.Synchronization(instance.context.type_manager, 'testDevice')

        statuses = sync.source.status_container
        self.assertEqual(statuses.get_status('SynchronizationSourceStatus').name, 'Synced')
        self.assertEqual(statuses.get_status('SynchronizationRoleStatus').name, 'Input')

    def test_set_unknown_source_fails(self):
        instance = opendaq.Instance()
        sync = opendaq.Synchronization(instance.context.type_manager, 'testDevice')

        with self.assertRaises(Exception):
            sync.source = 'DoesNotExist'
        self.assertEqual(sync.source.id, 'ClockSyncInterface')

    def test_device_synchronization(self):
        instance = opendaq.Instance()
        device = instance.add_device('daqref://device0')

        sync = device.synchronization
        self.assertIsNotNone(sync)
        self.assertEqual(sync.source.id, 'ClockSyncInterface')
        self.assertEqual(sync.source.reference_domain_id, 'local:openDAQ_DevSer0')


if __name__ == '__main__':
    unittest.main()
