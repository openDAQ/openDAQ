#!/usr/bin/env python

import unittest
import opendaq_test
import opendaq as daq


def _make_instance():
    """Create a minimal instance that loads no native modules."""
    builder = daq.InstanceBuilder()
    builder.module_path = '[[none]]'
    return builder.build()


# ---------------------------------------------------------------------------
# Empty module — no function block types
# ---------------------------------------------------------------------------

class EmptyModule(daq.Module):
    MODULE_ID = 'EmptyTestModule'

    def __init__(self, context):
        super().__init__(
            context=context,
            name='EmptyTestModule',
            version=(1, 0, 0),
            id=EmptyModule.MODULE_ID,
        )


# ---------------------------------------------------------------------------
# Module with a minimal (no-op) function block
# ---------------------------------------------------------------------------

class EmptyFunctionBlock(daq.FunctionBlock):
    TYPE_ID = 'EmptyTestFunctionBlock'

    @staticmethod
    def create_function_block_type():
        return daq.FunctionBlockType(
            EmptyFunctionBlock.TYPE_ID,
            'EmptyTestFunctionBlock',
            'Empty test function block',
            daq.PropertyObject()
        )


class ModuleWithEmptyFB(daq.Module):
    MODULE_ID = 'ModuleWithEmptyFB'

    def __init__(self, context):
        super().__init__(
            context=context,
            name='ModuleWithEmptyFB',
            version=(1, 0, 0),
            id=ModuleWithEmptyFB.MODULE_ID,
        )

    def on_get_available_function_block_types(self):
        fb_type = EmptyFunctionBlock.create_function_block_type()
        return {EmptyFunctionBlock.TYPE_ID: fb_type}

    def on_create_function_block(self, id, parent, local_id, config):
        if id != EmptyFunctionBlock.TYPE_ID:
            return None
        return EmptyFunctionBlock(self.context, parent, local_id)


# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------

class TestPythonModuleEmpty(opendaq_test.TestCase):
    """Tests for a Python module that registers no function block types."""

    def test_empty_module_registers(self):
        """Empty module appears in the module manager's module list."""
        instance = _make_instance()
        instance.module_manager.add_python_module(EmptyModule(instance.context))

        ids = [m.module_info.id for m in instance.module_manager.modules]
        self.assertIn(EmptyModule.MODULE_ID, ids)

    def test_empty_module_has_no_fb_types(self):
        """Empty module contributes no function block types to the instance."""
        instance = _make_instance()
        instance.module_manager.add_python_module(EmptyModule(instance.context))

        self.assertNotIn(EmptyFunctionBlock.TYPE_ID,
                         instance.available_function_block_types)

    # def test_empty_module_multiple_registrations(self):
    #     """Two different empty modules can be registered simultaneously."""
    #     instance = _make_instance()

    #     class AnotherEmptyModule(daq.Module):
    #         def __init__(self, ctx):
    #             super().__init__(context=ctx, name='Another', version=(1, 0, 0), id='Another')

    #     instance.module_manager.add_python_module(EmptyModule(instance.context))
    #     instance.module_manager.add_python_module(AnotherEmptyModule(instance.context))

    #     ids = [m.module_info.id for m in instance.module_manager.modules]
    #     self.assertIn(EmptyModule.MODULE_ID, ids)
    #     self.assertIn('Another', ids)


class TestPythonModuleWithEmptyFB(opendaq_test.TestCase):
    """Tests for a Python module that provides an empty function block."""

    def test_fb_type_is_available(self):
        """The function block type from the Python module is visible on the instance."""
        print("Testing that a Python module's function block type is available on the instance...")
        instance = _make_instance()
        print("Adding Python module with empty function block...")
        instance.module_manager.add_python_module(ModuleWithEmptyFB(instance.context))

        self.assertIn(EmptyFunctionBlock.TYPE_ID,
                      instance.available_function_block_types)

    def test_add_empty_function_block(self):
        """A function block from the Python module can be added to the instance."""
        instance = _make_instance()
        instance.module_manager.add_python_module(ModuleWithEmptyFB(instance.context))

        fb = instance.add_function_block(EmptyFunctionBlock.TYPE_ID)
        self.assertIsNotNone(fb)
        self.assertEqual(len(instance.function_blocks), 1)

    # def test_add_multiple_empty_function_blocks(self):
    #     """Multiple instances of the same Python function block type can be added."""
    #     instance = _make_instance()
    #     instance.module_manager.add_python_module(ModuleWithEmptyFB(instance.context))

    #     instance.add_function_block(EmptyFunctionBlock.TYPE_ID)
    #     instance.add_function_block(EmptyFunctionBlock.TYPE_ID)
    #     self.assertEqual(len(instance.function_blocks), 2)

    def test_empty_function_block_lifecycle(self):
        """Function blocks are created and destroyed without crash or memory leak."""
        instance = _make_instance()
        instance.module_manager.add_python_module(ModuleWithEmptyFB(instance.context))

        instance.add_function_block(EmptyFunctionBlock.TYPE_ID)
        # instance goes out of scope here; tearDown verifies zero leaked objects


if __name__ == '__main__':
    unittest.main()
