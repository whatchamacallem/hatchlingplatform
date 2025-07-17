# To run these tests go to the parent directory and execute:
#
#   python3 -m entanglement_py_template.entanglement_py_template_test

import unittest
from . import entanglement_py_template as ept

class test_suite1(unittest.TestCase):
    def setUp(self):
        """Set up test fixtures before each test."""
        self.class2_int = ept.ns0.class2(5)  # Initialize class2 with int
        self.class2_float = ept.ns0.class2(2.5, 3.5)  # Initialize class2 with floats
        self.class3 = ept.ns0.class3()  # Initialize class3

    def test_fn1(self):
        """Test all overloads of ns0.fn1 with expected return values."""
        # Test fn1() with no arguments (sum of g_a, g_b, g_c, g_f, g_g, g_h, initially 0)
        result = ept.ns0.fn1()
        self.assertIsInstance(result, float, "fn1() should return a float")
        self.assertEqual(result, 0.0, "fn1() should return 0.0 (sum of unset globals)")

        # Test fn1(a: int, b: int) (sets g_a, g_b, sums all)
        result = ept.ns0.fn1(1, 2)
        self.assertIsInstance(result, float, "fn1(int, int) should return a float")
        self.assertEqual(result, 3.0, "fn1(1, 2) should return 1 + 2 = 3.0")

    def test_fn2(self):
        """Test all overloads of ns0.fn2 with expected return values."""
        # Test fn2() with no arguments
        result = ept.ns0.fn2()
        self.assertIsInstance(result, float, "fn2() should return a float")
        self.assertEqual(result, 3.0, "fn2() should return 0.0 (default behavior)")

        # Test fn2(x: enum1) (returns input value)
        result = ept.ns0.fn2(ept.ns0.enum1(0))
        self.assertIsInstance(result, int, "fn2(enum1) should return an int")
        self.assertEqual(result, 0, "fn2(enum1) should return the input value (0)")

        # Test fn2(a: enum2, b: enum2) (returns True if equal)
        result = ept.ns0.fn2(ept.ns0.enum2.enum2_1, ept.ns0.enum2.enum2_1)
        self.assertIsInstance(result, bool, "fn2(enum2, enum2) should return a bool")
        self.assertTrue(result, "fn2(enum2, enum2) should return True for equal values")

        # Test fn2(a: enum3, b: enum3, c: enum3) (returns one of the inputs)
        result = ept.ns0.fn2(ept.ns0.enum3.enum3_1, ept.ns0.enum3.enum3_2, ept.ns0.enum3.enum3_3)
        self.assertIsInstance(result, int, "fn2(enum3, enum3, enum3) should return an int")
        self.assertIn(result, [-10, 0, 10], "fn2(enum3, enum3, enum3) should return one of the input values")

    def test_class1(self):
        """Test class1 constructor."""
        obj = ept.ns0.class1()
        self.assertIsInstance(obj, ept.ns0.class1, "class1 should instantiate correctly")

    def test_class2(self):
        """Test class2 constructors and fn3 method with expected values."""
        # Test class2(int) constructor and fn3
        self.assertIsInstance(self.class2_int.fn3(), int, "class2.fn3 should return an int")
        self.assertEqual(self.class2_int.fn3(), 5, "class2.fn3 should return m_x (5)")

        # Test class2(float, float) constructor and fn3
        self.assertIsInstance(self.class2_float.fn3(), int, "class2.fn3 should return an int")
        self.assertEqual(self.class2_float.fn3(), 2, "class2.fn3 should return casted m_x (int(2.5))")

    def test_class3(self):
        """Test class3 constructor, fn4 overloads, and fn5 with expected values."""
        # Test class3 constructor
        self.assertIsInstance(self.class3, ept.ns0.class3, "class3 should instantiate correctly")

        # Test fn4() with no arguments (sum of m_a, m_b, m_c, m_f, m_g, m_h, initially 0)
        result = self.class3.fn4()
        self.assertIsInstance(result, float, "class3.fn4() should return a float")
        self.assertEqual(result, 0.0, "fn4() should return 0.0 (sum of unset members)")

        # Test fn4(a: int, b: int) (sets m_a, m_b, sums all)
        result = self.class3.fn4(1, 2)
        self.assertIsInstance(result, float, "class3.fn4(int, int) should return a float")
        self.assertEqual(result, 3.0, "fn4(1, 2) should return 1 + 2 = 3.0")

    def test_enums(self):
        """Test enum1, enum2, and enum3 values."""
        # Test enum1 (empty)
        self.assertEqual(len(ept.ns0.enum1), 0, "enum1 should be empty")

        # Test enum2
        self.assertEqual(ept.ns0.enum2.enum2_1.value, 0, "enum2.enum2_1 should be 0")

        # Test enum3
        self.assertEqual(ept.ns0.enum3.enum3_1, -10, "enum3.enum3_1 should be -10")
        self.assertEqual(ept.ns0.enum3.enum3_2, 0, "enum3.enum3_2 should be 0")
        self.assertEqual(ept.ns0.enum3.enum3_3, 10, "enum3.enum3_3 should be 10")

if __name__ == '__main__':
    unittest.main()
