# To run these tests go to the parent directory and execute:
#
#   python3 -m entanglement_py_template.entanglement_py_template_test

import unittest

from . import entanglement_py_template as system_under_test

class run_all_tests(unittest.TestCase):
    def assert_instance_equal(self, result, expected_type, expected_value):
        self.assertIsInstance(result, expected_type, 'unit_test_assert is instance')
        self.assertEqual(result, expected_value, 'unit_test_assert is equal')

    def assert_instance_in(self, result, expected_type, expected_value_list):
        self.assertIsInstance(result, expected_type, 'unit_test_assert is instance')
        self.assertIn(result, expected_value_list, "unit_test_assert is in list")

    def test_function_roundtrip(self):
        self.assert_instance_equal(system_under_test.function_roundtrip_int8(-77), int, -77)
        self.assert_instance_equal(system_under_test.function_roundtrip_uint16(88), int, 88)
        self.assert_instance_equal(system_under_test.function_roundtrip_int32(-99), int, -99)
        self.assert_instance_equal(system_under_test.function_roundtrip_uint64(111), int, 111)

    def test_function_overload(self):
        self.assertEqual(system_under_test.function_overload(), None)
        self.assert_instance_equal(system_under_test.function_overload(1,2), int, -1)
        self.assert_instance_equal(system_under_test.function_overload(1,2,3,4), float, -2)





if __name__ == '__main__':
    unittest.main(verbosity=2)
