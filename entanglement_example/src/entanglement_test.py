# SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
# SPDX-License-Identifier: MIT
# This file is licensed under the terms of the LICENSE.md file.
#
# See debugging.txt. entanglement_example.py has to be built.

import ctypes, numpy, unittest

# Do not require the package to be installed when testing.
from . import entanglement_example as system_under_test

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

	# Generate overload group and selector.
	def test_function_overload(self):
		self.assertEqual(system_under_test.function_overload(), None)
		self.assert_instance_equal(system_under_test.function_overload(1,2), int, -1)
		self.assert_instance_equal(system_under_test.function_overload(1,2,3,4), float, -2)

	# Arrays of fundamental types. These are important to numpy/mathematicians.
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

		# numpy.array.
		np_array_input = numpy.array([i for i in range(size)], dtype=d_type)
		np_array_output = test_function(np_array_input, size, value)

		# Confirm modification of array.
		self.assertEqual([np_array_input[i] for i in range(size)], expected)
		# Can't iterate on a pointer but array access still works.
		self.assertEqual([np_array_output[i] for i in range(size)], expected)

	def test_function_pointer_int8(self):
		self.do_test_function_pointer(system_under_test.function_pointer_int8, ctypes.c_int8, numpy.int8, 0, 0)
		self.do_test_function_pointer(system_under_test.function_pointer_int8, ctypes.c_int8, numpy.int8, 1, -7)
		self.do_test_function_pointer(system_under_test.function_pointer_int8, ctypes.c_int8, numpy.int8, 33, -10)

	def test_function_pointer_uint16(self):
		self.do_test_function_pointer(system_under_test.function_pointer_uint16, ctypes.c_uint16, numpy.uint16, 33, 10)

	def test_function_pointer_int32(self):
		self.do_test_function_pointer(system_under_test.function_pointer_int32, ctypes.c_int32, numpy.int32, 22, -20)

	def test_function_pointer_uint64(self):
		self.do_test_function_pointer(system_under_test.function_pointer_uint64, ctypes.c_uint64, numpy.uint64, 11, 30)

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

	def test_struct_fundamentals(self):
		struct_fundamentals = system_under_test.StructFundamentals()
		struct_fundamentals.m_bool = True
		struct_fundamentals.m_char0 = 3
		struct_fundamentals.m_char1 = 4
		struct_fundamentals.m_char2 = 5
		struct_fundamentals.m_int0 = 6
		struct_fundamentals.m_int1 = 7
		struct_fundamentals.m_uint2 = 8
		struct_fundamentals.m_double[0] = 9

		result = system_under_test.function_struct_fundamentals_multiply(struct_fundamentals, 3)

		self.assertEqual(struct_fundamentals.m_bool, True)
		# Plain char is handled as a single byte in a byte array.
		self.assertEqual(result.m_bool, False)
		self.assertEqual(struct_fundamentals.m_char0, b'\x03')
		self.assertEqual(result.m_char0, b'\x09')
		self.assertEqual(struct_fundamentals.m_char2, 5)
		self.assertEqual(result.m_char2, 15)
		self.assertEqual(struct_fundamentals.m_double[0], 9)
		self.assertEqual(result.m_double[0], 27)

	# tests .vtable as well.
	def test_pointer_fundamentals(self):
		pointer_fundamentals = system_under_test.StructPointerFundamentals()
		void_array = (ctypes.c_int * 1)(2)
		pointer_fundamentals.m_pvoid = ctypes.cast(void_array, ctypes.c_void_p)
		bool_array = (ctypes.c_bool * 1)(True)
		pointer_fundamentals.m_pbool = ctypes.cast(bool_array, ctypes.POINTER(ctypes.c_bool))
		float_array = (ctypes.c_float * 1)(3.0)
		pointer_fundamentals.m_pfloat = ctypes.cast(float_array, ctypes.POINTER(ctypes.c_float))

		result = system_under_test.function_struct_pointer_fundamentals_multiply(pointer_fundamentals, 5)
		# dereference pointer only once, uses memcpy to construct class.
		result = result.contents

		self.assertEqual(ctypes.cast(pointer_fundamentals.m_pvoid, ctypes.POINTER(ctypes.c_int))[0], 10)
		self.assertEqual(pointer_fundamentals.m_pbool[0], False)
		self.assertEqual(pointer_fundamentals.m_pfloat[0], 15)

		self.assertEqual(ctypes.cast(result.m_pvoid, ctypes.POINTER(ctypes.c_int))[0], 10)
		self.assertEqual(result.m_pbool[0], False)
		self.assertEqual(result.m_pfloat[0], 15)

		system_under_test.function_struct_pointer_fundamentals_multiply2(pointer_fundamentals, 2)
		self.assertEqual(pointer_fundamentals.m_pfloat[0], 30)

	def test_operators_one_to_one(self):
		# All operations return a matching string. A number of operators are
		# missing. These are just the ones that have literal translations
		# between languages. E.g. Python uses a cast to bool to implement && and
		# ||. There is no assignment operator because it wouldn't be what was
		# expected. Urinary operators are tested here but the result is not
		# explicit.
		a = system_under_test.OperatorTest()
		b = system_under_test.OperatorTest()

		self.assertEqual(a + b, '+')
		self.assertEqual(a & b, '&')
		self.assertEqual(a(0), '()')
		self.assertEqual(a == b, '==')
		self.assertEqual(a >= b, '>=')
		self.assertEqual(a > b, '>')
		self.assertEqual(a[0], '[]')
		a = system_under_test.OperatorTest()
		a &= b; self.assertEqual(a, '&=')
		a = system_under_test.OperatorTest()
		a += b; self.assertEqual(a, '+=')
		a = system_under_test.OperatorTest()
		a <<= b; self.assertEqual(a, '<<=')
		a = system_under_test.OperatorTest()
		a *= b; self.assertEqual(a, '*=')
		a = system_under_test.OperatorTest()
		a |= b; self.assertEqual(a, '|=')
		a = system_under_test.OperatorTest()
		a %= b; self.assertEqual(a, '%=')
		a = system_under_test.OperatorTest()
		self.assertEqual(~a, '~')
		a ^= b; self.assertEqual(a, '^=')
		a = system_under_test.OperatorTest()
		a -= b; self.assertEqual(a, '-=')
		a = system_under_test.OperatorTest()
		a /= b; self.assertEqual(a, '/=')
		a = system_under_test.OperatorTest()
		a >>= b; self.assertEqual(a, '>>=')
		a = system_under_test.OperatorTest()
		self.assertEqual(a <= b, '<=')
		self.assertEqual(a << b, '<<')
		self.assertEqual(a < b, '<')
		self.assertEqual(a % b, '%')
		self.assertEqual(a * b, '*')
		self.assertEqual(a != b, '!=')
		self.assertEqual(a | b, '|')
		self.assertEqual(a >> b, '>>')
		self.assertEqual(a - b, '-')
		self.assertEqual(+a, '+')
		self.assertEqual(-a, '-')
		self.assertEqual(a / b, '/')
		self.assertEqual(a ^ b, '^')

	def test_operators_logical(self):
		# Tests OperatorTest.__bool__ which returns true.
		a = system_under_test.OperatorTest()
		b = system_under_test.OperatorTest()

		# The logical operators return objects based on __bool__. Show they can
		# be used.
		self.assertEqual((a and b) == a, '==')
		self.assertEqual((a or b) == a, '==')
		self.assertTrue((not a) == False)

		if_a = False
		if a:
			if_a = True
		self.assertTrue(if_a)

	def test_namespaces(self):
		# See the C++ for test description.
		x = system_under_test.NameSpaceOne.NameSpaceOneClassThree()
		self.assertEqual(x.one_two.class_one_one(0), 10)
		self.assertEqual(x.one_two.class_one_two(0), 20)
		self.assertEqual(x.class_two_one(0), 40)
		self.assertEqual(x.class_one_three(0), 60)

		# Select between a class and a subclass as a first arg.
		one = system_under_test.NameSpaceOne.NameSpaceOneClassOne()
		two = system_under_test.NameSpaceOne.NameSpaceOneClassTwo()
		self.assertEqual(system_under_test.NameSpaceOne.namespace_one(one), 30)
		self.assertEqual(system_under_test.NameSpaceOne.namespace_one(two), 70)
		self.assertEqual(system_under_test.NameSpaceTwo.namespace_two(0), 50)

if __name__ == '__main__':
	unittest.main(verbosity=2)
