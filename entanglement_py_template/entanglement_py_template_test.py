# To run these tests go to the parent directory and execute:
#
#   python3 -m entanglement_py_template.entanglement_py_template_test

import ctypes, unittest
import numpy as np
from . import entanglement_py_template as system_under_test

class run_all_tests(unittest.TestCase):
    def assert_instance_equal(self, result, expected_type, expected_value):
        self.assertIsInstance(result, expected_type, 'unit_test_assert is instance')
        self.assertEqual(result, expected_value, 'unit_test_assert is equal')

    def assert_instance_in(self, result, expected_type, expected_value_list):
        self.assertIsInstance(result, expected_type, 'unit_test_assert is instance')
        self.assertIn(result, expected_value_list, "unit_test_assert is in list")

    def test_enums(self):
        t = (system_under_test.ENUM_C_STYLE_TWO_CONSTANTS_1
             | system_under_test.ENUM_C_STYLE_TWO_CONSTANTS_2)
        self.assert_instance_equal(t, int, 3)
        self.assert_instance_equal(system_under_test.ENUM_INT16_THREE_CONSTANTS_0,
                                   system_under_test.EnumInt16ThreeConstants, -32768)
        self.assert_instance_equal(system_under_test.ENUM_INT16_THREE_CONSTANTS_1,
                                   system_under_test.EnumInt16ThreeConstants, -1)
        self.assert_instance_equal(system_under_test.ENUM_INT16_THREE_CONSTANTS_2,
                                   system_under_test.EnumInt16ThreeConstants, 32767)
        self.assert_instance_equal(system_under_test.EnumScopedUInt64.ENUM_SCOPED_UINT64_0,
                                   system_under_test.EnumScopedUInt64, 12379813738877118345)
        self.assert_instance_equal(system_under_test.ANONYMOUS_ENUM_0, int, 0)

        # Iterate over EnumInt16ThreeConstants and show the sum is -2.
        total_value = 0
        for member in system_under_test.EnumInt16ThreeConstants:
            total_value += member.value
        self.assert_instance_equal(total_value, int, -2)

    def test_function_roundtrip(self):
        self.assert_instance_equal(system_under_test.function_roundtrip_int8(-77), int, -77)
        self.assert_instance_equal(system_under_test.function_roundtrip_uint16(88), int, 88)
        self.assert_instance_equal(system_under_test.function_roundtrip_int32(-99), int, -99)
        self.assert_instance_equal(system_under_test.function_roundtrip_uint64(111), int, 111)

    def test_function_overload(self):
        self.assertEqual(system_under_test.function_overload(), None)
        self.assert_instance_equal(system_under_test.function_overload(1,2), int, -1)
        self.assert_instance_equal(system_under_test.function_overload(1,2,3,4), float, -2)

    def do_test_function_pointer(self, test_function, c_type, size :int, value: int):
        input_array = (c_type * size)()
        output_array = test_function(input_array, size, value)
        expected = [i + value for i in range(size)]

        # Confirm modification of array. Can iterate on a ctypes.Array with
        # modified values.
        self.assertEqual([int(x) for x in input_array], expected)

        # Can't iterate on a pointer but array access still works.
        self.assertEqual([output_array[i] for i in range(size)], expected)

        # do it again with a pointer cast.
        input_array_cast = ctypes.cast(input_array, ctypes.POINTER(ctypes.c_int))
        test_function(input_array_cast, size, value * 2)
        expected2 = [i + value * 2 for i in range(size)]
        self.assertEqual([input_array_cast[i] for i in range(size)], expected2)

        # np.array.
        np_array_input = np.array([i for i in range(size)])
        np_array_output = test_function(np_array_input, size, value)
        np_array_expected = np.array([i + value for i in range(size)])

        # Confirm modification of array.
        self.assertEqual(np_array_input, np_array_expected)

        # Can't iterate on a pointer to numpy guts but array access still works.
        self.assertEqual([np_array_output[i] for i in range(size)], np_array_expected)

    def test_function_pointer_int8(self):
        self.do_test_function_pointer(system_under_test.function_pointer_int8, ctypes.c_int8, 0, 0)
        self.do_test_function_pointer(system_under_test.function_pointer_int8, ctypes.c_int8, 1, -7)
        self.do_test_function_pointer(system_under_test.function_pointer_int8, ctypes.c_int8, 33, -10)

    def test_function_pointer_uint16(self):
        self.do_test_function_pointer(system_under_test.function_pointer_uint16, ctypes.c_uint16, 33, 10)

    def test_function_pointer_int32(self):
        self.do_test_function_pointer(system_under_test.function_pointer_int32, ctypes.c_int32, 22, -20)

    def test_function_pointer_uint64(self):
        self.do_test_function_pointer(system_under_test.function_pointer_uint64, ctypes.c_uint64, 11, 30)

if __name__ == '__main__':
    unittest.main(verbosity=2)
