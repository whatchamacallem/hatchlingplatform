
# To run these tests go to the parent directory and execute:
#
#   python3 -m entanglement_py_template.entanglement_py_template_test

import unittest
from . import entanglement_py_template

class test_suite1(unittest.TestCase):
    def setUp(self):
        """Set up test fixtures before each test."""

    def test_fn1(self):
        x :float = entanglement_py_template.ns0.fn1(0, 3)
        print(f'?? {x}')


if __name__ == '__main__':
    unittest.main()
