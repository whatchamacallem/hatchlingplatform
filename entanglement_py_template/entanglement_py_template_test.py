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

	# Check C calling convention first.
	def test_function_roundtrip(self):
		self.assert_instance_equal(system_under_test.function_roundtrip_int8(-77), int, -77)
		self.assert_instance_equal(system_under_test.function_roundtrip_uint16(88), int, 88)
		self.assert_instance_equal(system_under_test.function_roundtrip_int32(-99), int, -99)
		self.assert_instance_equal(system_under_test.function_roundtrip_uint64(111), int, 111)

	# Generate overload group and selector.
	def test_function_overload(self):
		self.assertEqual(system_under_test.function_overload(), None)
		self.assert_instance_equal(system_under_test.function_overload(1,2), int, -1)
		self.assert_instance_equal(system_under_test.function_overload(1,2,3,4), float, -2)

	# Arrays of primitive types. These are important to numpy/mathematicians.
	def do_test_function_pointer(self, test_function, c_type, d_type, size :int, value: int):
		input_array = (c_type * size)()
		output_array = test_function(input_array, size, value)
		expected = [i + value for i in range(size)]

		# Confirm modification of array. Can iterate on a ctypes.Array with
		# modified values.
		self.assertEqual([int(x) for x in input_array], expected)

		# Can't iterate on a pointer but array access still works.
		self.assertEqual([output_array[i] for i in range(size)], expected)

		# Do it again with a ctypes pointer instead of an array.
		input_array_cast = ctypes.cast(input_array, ctypes.POINTER(c_type))
		test_function(input_array_cast, size, value * 2)
		expected2 = [i + value * 2 for i in range(size)]
		self.assertEqual([input_array_cast[i] for i in range(size)], expected2)

		# np.array.
		np_array_input = np.array([i for i in range(size)], dtype=d_type)
		np_array_output = test_function(np_array_input, size, value)

		# Confirm modification of array.
		self.assertEqual([np_array_input[i] for i in range(size)], expected)
		# Can't iterate on a pointer but array access still works.
		self.assertEqual([np_array_output[i] for i in range(size)], expected)

	def test_function_pointer_int8(self):
		self.do_test_function_pointer(system_under_test.function_pointer_int8, ctypes.c_int8, np.int8, 0, 0)
		self.do_test_function_pointer(system_under_test.function_pointer_int8, ctypes.c_int8, np.int8, 1, -7)
		self.do_test_function_pointer(system_under_test.function_pointer_int8, ctypes.c_int8, np.int8, 33, -10)

	def test_function_pointer_uint16(self):
		self.do_test_function_pointer(system_under_test.function_pointer_uint16, ctypes.c_uint16, np.uint16, 33, 10)

	def test_function_pointer_int32(self):
		self.do_test_function_pointer(system_under_test.function_pointer_int32, ctypes.c_int32, np.int32, 22, -20)

	def test_function_pointer_uint64(self):
		self.do_test_function_pointer(system_under_test.function_pointer_uint64, ctypes.c_uint64, np.uint64, 11, 30)

	def test_function_pointer_char(self):
		one_hundred_characters = (ctypes.c_char * 100)()
		result : bytes = system_under_test.function_pointer_char(one_hundred_characters)
		# These are both type "bytes".
		self.assert_instance_equal(one_hundred_characters.value.decode(), str, '🐉🐉🐉 A')
		self.assert_instance_equal(result.decode(), str, '🐉🐉🐉 A') # type: ignore

	def test_function_pointer_wchar(self):
		# Passing string buffers requires an array of ctypes wchar. Returning
		# wchar* results in a str. Python strings are immutable.  This is what
		# you get.
		one_hundred_characters = (ctypes.c_wchar * 100)()
		result : str = system_under_test.function_pointer_wchar(one_hundred_characters)
		self.assert_instance_equal(one_hundred_characters.value, str, '🐉🐉🐉 B')
		self.assert_instance_equal(result, str, '🐉🐉🐉 B')

	def test_function_pointer_void(self):
		one_int = (ctypes.c_int * 1)()
		result : int = system_under_test.function_pointer_void_to_int(one_int, 1, 7)

		# This is how a void C pointer is cast to a type.
		return_ptr = ctypes.cast(result, ctypes.POINTER(ctypes.c_int))

		self.assertEqual(one_int[0], 7)
		self.assertEqual(return_ptr[0], 7)

	def test_function_ref(self):
		# modify and return a char by reference.
		buf_bool = ctypes.c_bool(True)
		result_bool = system_under_test.function_ref_bool(buf_bool, False)
		self.assertEqual(buf_bool.value, False)
		# Return a reference, get a pointer.
		self.assertEqual(result_bool.contents.value, False)

		# modify and return a uint16 by reference.
		# XXX Using ctypes.c_uint16 here correctly breaks Pylance.
		buf_uint16 = ctypes.c_ushort(0)
		result_uint16 = system_under_test.function_ref_uint16(buf_uint16, 199)
		self.assertEqual(buf_uint16.value, 199)
		# Return a reference, get a pointer.
		self.assertEqual(result_uint16.contents.value, 199)

		# modify and return a wchar by reference.
		buf_wchar = ctypes.c_wchar('a')
		system_under_test.function_ref_wchar(buf_wchar, 'z')
		self.assertEqual(buf_wchar.value, 'z')

		# modify and return a uint64 by reference.
		# XXX Using ctypes.c_uint64 here correctly breaks Pylance.
		buf_uint64 = ctypes.c_ulong(66)
		result_uint64 = system_under_test.function_ref_uint64(buf_uint64, 66)
		self.assertEqual(buf_uint64.value, 66)
		self.assertEqual(result_uint64.contents.value, 66)


if __name__ == '__main__':
	unittest.main(verbosity=2)
