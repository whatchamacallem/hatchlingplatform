# To run these tests go to the parent directory and execute:
#
#   python3 -m entanglement_py_template.entanglement_py_template_test

import unittest

from . import entanglement_py_template as package_under_test

class run_all_tests(unittest.TestCase):
    def assertInstanceEqual(self, result, expected_type, expected_value):
        self.assertIsInstance(result, expected_type, 'unit_test_assert is instance')
        self.assertEqual(result, expected_value, 'unit_test_assert is equal')

    def assert_instance_in(self, result, expected_type, expected_value_list):
        self.assertIsInstance(result, expected_type, 'unit_test_assert is instance')
        self.assertIn(result, expected_value_list, "unit_test_assert is in list")

    def setUp(self):
        pass

    def test_fn1(self):
        """Test all overloads of ns0.fn1 with expected return values."""
        # Test fn1() with no arguments (sum of g_a, g_b, g_c, g_f, g_g, g_h, initially 0)
        result = package_under_test.ns0.fn1()
        self.assertInstanceEqual(result, float, 0.0)

        # Test fn1(a: int, b: int) (sets g_a, g_b, sums all)
        result = package_under_test.ns0.fn1(1, 2)
        self.assertInstanceEqual(result, float, 3.0)



if __name__ == '__main__':
    unittest.main(verbosity=2)
