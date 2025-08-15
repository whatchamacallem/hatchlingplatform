# SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
# SPDX-License-Identifier: MIT
# This file is licensed under the terms of the LICENSE.md file.

_version = '0.0.3-dev'
_usage = f'''\
python3 entanglement.py <compiler_flags> <lib_name> <header_files>... <output_file>
version: {_version}

Generates Python bindings for a C++ .so using a clang command line and one or
more C++ headers which were annotated with <entanglement.h>. Raises usage/error
if arguments are invalid or processing fails.

Arguments:
	compiler_flags  - Flags to pass to clang. Can be in any order on command line.
	lib_name		- C/C++ library/.so name to bind everything to.
	header_files... - Path(s) to the C++ header file(s) to parse.
	output_file	    - Path to write the generated Python binding code.
'''

import enum, keyword, sys
from clang.cindex import AccessSpecifier, Config, Cursor, CursorKind, Diagnostic
from clang.cindex import Index, LinkageKind, StorageClass, TranslationUnit, Type, TypeKind
from datetime import datetime
from typing import Dict, List, NoReturn, Optional, Tuple

# Verbose - 0: No news is good news. 1 : Status messages. 2: Processing steps.
# 3: AST traversal. All messages go to stderr.
VERBOSE = 2

# XXX Path to the libclang shared library. Needs to be configurable.
_libclang_path = '/usr/lib/llvm-18/lib/libclang.so.1'

# List of compiler flags for the build process.
_arg_compiler_flags: List[str]

# Name of the .so library to be wrapped.
_arg_lib_name: str

# List of header files to be used.
_arg_header_files: List[str]

# Name of the output file to be generated.
_arg_output_file: str

# Mapping of Clang TypeKind to ctypes types for basic C types. Used to convert C
# types to their corresponding Python ctypes representations.
_clang_to_ctypes: Dict[TypeKind, str] = {
	TypeKind.BOOL:      	'_Ctypes.c_bool',       # type: ignore # bool
	TypeKind.CHAR_S:		'_Ctypes.c_char',       # type: ignore # char
	TypeKind.CHAR_U:		'_Ctypes.c_ubyte',      # type: ignore # unsigned char
	TypeKind.DOUBLE:		'_Ctypes.c_double',     # type: ignore # double
	TypeKind.FLOAT:     	'_Ctypes.c_float',      # type: ignore # float
	TypeKind.INT:       	'_Ctypes.c_int',		# type: ignore # int
	TypeKind.LONG:      	'_Ctypes.c_long',       # type: ignore # long
	TypeKind.LONGDOUBLE:	'_Ctypes.c_longdouble', # type: ignore # long double
	TypeKind.LONGLONG:  	'_Ctypes.c_longlong',   # type: ignore # long long
	TypeKind.SCHAR:     	'_Ctypes.c_byte',       # type: ignore # signed char
	TypeKind.SHORT:     	'_Ctypes.c_short',      # type: ignore # short
	TypeKind.UCHAR:     	'_Ctypes.c_ubyte',      # type: ignore # unsigned char
	TypeKind.UINT:      	'_Ctypes.c_uint',       # type: ignore # unsigned int
	TypeKind.ULONG:     	'_Ctypes.c_ulong',      # type: ignore # unsigned long
	TypeKind.ULONGLONG: 	'_Ctypes.c_ulonglong',  # type: ignore # unsigned long long
	TypeKind.USHORT:    	'_Ctypes.c_ushort',     # type: ignore # unsigned short
	TypeKind.VOID:      	'None',                 # type: ignore # void
	TypeKind.WCHAR:     	'_Ctypes.c_wchar',      # type: ignore # wchar_t
}

# Mapping of Clang TypeKind to ctypes types for pointer types.
# Used for special handling of pointer types like void*, char*, and wchar_t*.
_clang_to_ctypes_ptr: Dict[TypeKind, str] = {
	TypeKind.VOID: 		'_Ctypes.c_void_p',		# type: ignore # void*
	TypeKind.CHAR_S:	'_Ctypes.c_char_p',		# type: ignore # char*
	TypeKind.WCHAR:		'_Ctypes.c_wchar_p',	# type: ignore # wchar_t*
}

# Mapping of Clang TypeKind to Python types for function return values. Special
# handling for pointer types returned from functions, converting void* to int,
# char* to bytes, and wchar_t* to str.
_clang_to_ctypes_ptr_return: Dict[TypeKind, str] = {
	TypeKind.VOID:		'int',		# type: ignore # void*
	TypeKind.CHAR_S:	'bytes',	# type: ignore # char*
	TypeKind.WCHAR:		'str',		# type: ignore # wchar_t*
}

# Mapping of Clang TypeKind to Python types for type hints. Used to map C types
# to their equivalent Python types for type hinting purposes.
_clang_to_python: Dict[TypeKind, str] = {
	TypeKind.BOOL:			'bool',  # type: ignore
	TypeKind.CHAR_S:		'int',   # type: ignore
	TypeKind.CHAR_U:		'int',   # type: ignore
	TypeKind.DOUBLE:		'float', # type: ignore
	TypeKind.FLOAT:			'float', # type: ignore
	TypeKind.INT:			'int',   # type: ignore
	TypeKind.LONG:			'int',   # type: ignore
	TypeKind.LONGDOUBLE:	'float', # type: ignore
	TypeKind.LONGLONG:		'int',   # type: ignore
	TypeKind.SCHAR:			'int',   # type: ignore
	TypeKind.SHORT:			'int',   # type: ignore
	TypeKind.UCHAR:			'int',   # type: ignore
	TypeKind.UINT:			'int',   # type: ignore
	TypeKind.ULONG:			'int',   # type: ignore
	TypeKind.ULONGLONG:		'int',   # type: ignore
	TypeKind.USHORT:		'int',   # type: ignore
	TypeKind.VOID:			'None',  # type: ignore
	TypeKind.WCHAR:			'int',   # type: ignore
}

# _operator_name_map - A number of operators are missing. These are just the
# ones that have literal translations between languages. E.g. Python uses a cast
# to bool to implement && and ||. There is no assignment operator because it
# wouldn't be what was expected. '__pos__' and '__neg__' are handled elsewhere.
_operator_name_map: Dict[str, str] = {
	'operator+':   '__add__',
	'operator&':   '__and__',
	'operator()':  '__call__',
	'operator==':  '__eq__',
	'operator>=':  '__ge__',
	'operator>':   '__gt__',
	'operator[]':  '__getitem__',
	'operator&=':  '__iand__',
	'operator+=':  '__iadd__',
	'operator<<=': '__ilshift__',
	'operator*=':  '__imul__',
	'operator|=':  '__ior__',
	'operator%=':  '__imod__',
	'operator~':   '__invert__',
	'operator^=':  '__ixor__',
	'operator-=':  '__isub__',
	'operator/=':  '__itruediv__',
	'operator>>=': '__irshift__',
	'operator<=':  '__le__',
	'operator<<':  '__lshift__',
	'operator<':   '__lt__',
	'operator%':   '__mod__',
	'operator*':   '__mul__',
	'operator!=':  '__ne__',
	'operator|':   '__or__',
	'operator>>':  '__rshift__',
	'operator-':   '__sub__',
	'operator/':   '__truediv__',
	'operator^':   '__xor__'
}

# Nota Bene. ctypes.Structure uses these. So they are off limits.
ctypes_reserved: set[str] = {	"_alignment_", "_anonymous_", "_argtypes_", "_array_",
					"_as_parameter_", "_bases_", "_buffer_", "_checker_",
					"_class_", "_fields_", "_from_param_", "_func_ptr_",
					"_handle_", "_length_", "_name_", "_objects_", "_offset_",
					"_pack_", "_pointer_", "_restype_", "_size_", "_subclasses_",
					"_type_", "_values_" }

# Exception names appropriate for a command line tool.
class error(ValueError): ...
class usage(ValueError): ...

class type_string_kind(enum.Enum):
	'''
	Enum defining different contexts for type string generation. Used to specify
	the purpose of type strings (e.g., argument hints, return types, ctypes
	args, or structure definitions). '''
	arg_hint = 0,
	return_hint = 1,
	ctypes_args = 2,
	ctypes_struct = 3

def verbose(verbose_level: int, x: str) -> None:
	'''
	Prints message `x` if `verbose_level >= VERBOSE`.
	- `verbose_level` : [0..3] Ranges from "no news" to debug diagnostics.
	- `x` : The message to be printed (or not). '''
	if VERBOSE >= verbose_level:
		print(f' {'?*+-'[verbose_level]} {x}', file=sys.stderr)

def parse_argv(argv: List[str]) -> bool:
	'''
	Parses the command-line arguments into global constants. Returns `True` if
	parsing succeeds, `False` if insufficient non-switch arguments.
	- `argv` : List of command-line arguments as strings. '''
	global _arg_compiler_flags
	global _arg_lib_name
	global _arg_header_files
	global _arg_output_file

	_arg_compiler_flags = []
	non_flags: List[str] = []
	for arg in argv:
		if arg.startswith('-'):
			_arg_compiler_flags.append(arg)
		else:
			non_flags.append(arg)

	if len(non_flags) < 3:
		return False # Needed more non-switch args.

	# the first non-switch was the lib_name.
	_arg_lib_name = non_flags.pop(0)

	# the last non-switch was the output file.
	_arg_output_file = non_flags.pop()

	# the rest are headers.
	_arg_header_files = non_flags

	return True

def object_debugger(obj) -> None:
	'''
	Prints all accessible attributes of an object to stderr for debugging.
	cindex may have extra exception data.
	- `obj` : Any object to inspect and print attributes for. '''
	for attribute_name in dir(obj):
		try:
			attribute_value = getattr(obj, attribute_name)
			print(f'  {attribute_name}: {attribute_value}', file=sys.stderr)
		except:
			...

def raise_error(c: Cursor, message: str) -> NoReturn:
	'''
	Raises an Error. Formats the source code location and leaves the message to
	the caller.
	- `c` : Clang cursor indicating the source location of the error.
	- `message` : Error message to include in the raised exception. '''
	raise error(f'{c.location.file}:{c.location.line}:{c.location.column}: {message}\n')

def get_name(cursor: Cursor) -> str:
	'''
	Returns the cursor's name or a unique ID for anonymous cursors.
	- `cursor` : Clang cursor to extract the name from. '''
	return cursor.spelling if hasattr(cursor, 'spelling') and not cursor.is_anonymous() else '_ID' + str(hex(hash(cursor.get_usr())))[3:]

def get_internal_name(cursor: Cursor) -> str:
	'''
	Returns the mangled name of the function. C names are mangled as `_C0name`.
	- `cursor` : Clang cursor to extract the mangled name from. '''
	if cursor.mangled_name.startswith('_Z'):
		return cursor.mangled_name
	return f'_C0{cursor.spelling}'

def get_cxx_symbol_name(cursor: Cursor) -> str:
	'''
	Returns the name expected in the .so. Must be a function and C names are unchanged.
	- `cursor` : Clang cursor to determine the symbol name for. '''
	return cursor.mangled_name

def get_dunder_name(cursor: Cursor) -> str:
	'''
	In Python a dunder name is one that starts with double underscores. '''
	name = get_name(cursor)
	if cursor.kind is CursorKind.CONSTRUCTOR: # type: ignore
		name = '__init__'
	elif cursor.kind is CursorKind.DESTRUCTOR: # type: ignore
		name = '__del__'
	elif (cursor.kind is CursorKind.CXX_METHOD # type: ignore
			and name.startswith('operator')
			and len(name) > 8
			and not name[8].isidentifier()):

		if not name in _operator_name_map:
			raise_error(cursor, f'{name} unsupported.')
		dunder_name = _operator_name_map[name]
		if not any(True for _ in cursor.get_arguments()):
			if dunder_name == '__add__':
				return '__pos__'
			elif dunder_name == '__sub__':
				return '__neg__'
		return dunder_name
	return name

def is_annotated_entanglement(cursor: Cursor) -> bool:
	'''
	Checks if a cursor has the 'entanglement' annotation.
	- `cursor` : Clang cursor to check for annotations. '''
	for c in cursor.get_children():
		if c.kind == CursorKind.ANNOTATE_ATTR: # type: ignore
			if 'entanglement' == c.spelling:
				return True
	return False

def is_arg_va_list(cursor: Cursor) -> bool:
	'''
	Checks if a cursor's arguments include a va_list.
	- `cursor` : Clang cursor representing a function to check. '''
	return any("va_list" in a.type.spelling for a in cursor.get_arguments())

def is_staticmethod(cursor: Cursor) -> bool:
	'''
	Determines if a cursor represents a static method or free function.
	- `cursor` : Clang cursor to check. '''
	return cursor.kind is CursorKind.FUNCTION_DECL or ( # type: ignore
		cursor.kind is CursorKind.CXX_METHOD and cursor.storage_class == StorageClass.STATIC) # type: ignore

def calculate_namespace(cursor: Cursor) -> List[str]:
	'''
	Returns a list of namespace names in order from outermost to innermost.
	- `cursor` : Clang cursor to analyze. '''
	namespaces: List[str] = []
	current: Optional[Cursor] = cursor.semantic_parent
	while current and current.kind is not CursorKind.TRANSLATION_UNIT: # type: ignore
		if current.kind == CursorKind.LINKAGE_SPEC: # type: ignore
			return [] # Code declared extern "C" has no namespace.

		if hasattr(current, 'spelling') and not current.is_anonymous():
			namespaces.append(current.spelling)
		current = current.semantic_parent
	namespaces.reverse()
	return namespaces

def calculate_python_package_path(cursor: Cursor) -> str:
	'''
	Returns a dot-separated string representing the Python package path.
	Anonymous namespaces are traversed as they may contain implementation details
	like base class layouts that are needed by the bindings code. Anonymous enums
	and structs are given globally unique ids.
	- `cursor` : Clang cursor to compute the path for. '''
	namespaces: List[str] = calculate_namespace(cursor)
	namespaces.append(get_dunder_name(cursor))
	return '.'.join(namespaces)

def get_inheritance_generation(cursor: Cursor) -> int:
	'''
	Returns `1` for a class/struct with no parent and `1+N` for a class/struct
	with `N` parents. Returns `0` to signal all other kinds.
	- `cursor` : Clang cursor. '''

	if cursor.kind in (CursorKind.CLASS_DECL, CursorKind.STRUCT_DECL): # type: ignore
		for child in cursor.get_children():
			if child.kind == CursorKind.CXX_BASE_SPECIFIER: # type: ignore
				return get_inheritance_generation(child.referenced) + 1
		return 1 # Base class.
	return 0 # Fundamental or other type.

def get_base_class(cursor: Cursor) -> str:
	'''
	Returns the Python package path of the base class or `_Ctypes.Structure` if
	none.
	- `cursor` : Clang cursor representing a class or struct. '''
	if sum(c.kind == CursorKind.CXX_BASE_SPECIFIER for c in cursor.get_children()) > 1: # type: ignore
		raise_error(cursor, 'Multiple inheritance unsupported by Python\'s ctypes module.')

	for child in cursor.get_children():
		if child.kind == CursorKind.CXX_BASE_SPECIFIER: # type: ignore
			return calculate_python_package_path(child.referenced)
	return '_Ctypes.Structure'

def has_vtable(cursor: Cursor):
	'''
	Returns `True` if the class has virtual methods and is the first virtual
	class, `False` otherwise.
	- `cursor` : Clang cursor representing a class or struct. '''
	has_virtual = False
	for c in cursor.get_children():
		if c.kind in (	CursorKind.CONSTRUCTOR, # type: ignore
						CursorKind.DESTRUCTOR,  # type: ignore
						CursorKind.CXX_METHOD): # type: ignore

			if c.is_virtual_method():
				has_virtual = True
				break

	if has_virtual:
		for c in cursor.get_children():
			if c.kind == CursorKind.CXX_BASE_SPECIFIER: # type: ignore
				if has_vtable(c.referenced):
					return False # Not first virtual class

	return has_virtual

def calculate_type_string(cursor: Cursor, cpp_type_ref: Type, symbols: Dict[str, List[Cursor]], result_kind: type_string_kind) -> str:
	'''
	Returns a string representing the type as a Python or ctypes type.
	- `cursor` : Clang cursor for error reporting.
	- `cpp_type_ref` : Clang type to convert.
	- `symbols` : Dictionary of known symbols for type resolution.
	- `result_kind` : Specifies the context (arg_hint, return_hint,
	  ctypes_args, ctypes_struct). '''

	# Handle typedefs by resolving to the canonical type.
	cpp_type_canonical = cpp_type_ref.get_canonical()

	# Handle fundamental types.
	if cpp_type_canonical.kind in _clang_to_ctypes:
		if result_kind in (type_string_kind.ctypes_args,
					 		type_string_kind.ctypes_struct):
			return _clang_to_ctypes[cpp_type_canonical.kind]
		if result_kind is type_string_kind.arg_hint:
			if cpp_type_canonical.kind is TypeKind.WCHAR: # type: ignore
				return 'str' # Single characters are passed as strings in Python.
		return _clang_to_python[cpp_type_canonical.kind]

	# Arrays. These are for class and struct layout only. C/C++ functions should
	# pass arrays as pointers, ctypes.Array or numpy's ndarray. entanglement.py
	# does not provide additional Python interfaces to manipulate C++ arrays.
	if cpp_type_canonical.kind is TypeKind.CONSTANTARRAY:		  # type: ignore
		if not result_kind is type_string_kind.ctypes_struct:
			raise_error(cursor, 'Constant arrays supported in class and struct layouts only.')

		array_element_string = calculate_type_string(cursor, cpp_type_canonical.get_array_element_type(), symbols, result_kind)
		return f'{array_element_string} * {cpp_type_canonical.get_array_size()}'

	# Pointers and references. The ctypes documentation describes the C level
	# coercion issues involved here. Furthermore, treating references as
	# pointers is "implementation-defined behavior" that depends on the C++ ABI.
	is_pointer = False
	if cpp_type_canonical.kind in ( TypeKind.POINTER,		  # type: ignore
									TypeKind.LVALUEREFERENCE,  # type: ignore
									TypeKind.RVALUEREFERENCE): # type: ignore
		pointee_type = cpp_type_canonical.get_pointee()
		pointee_type_canonical = pointee_type.get_canonical()

		# Max 1 pointer or reference per-type. There is no Pythonian way to map
		# multiple levels of pointer indirection to a language that doesn't have
		# pointers.
		if pointee_type_canonical.kind in ( TypeKind.POINTER,		  # type: ignore
											TypeKind.LVALUEREFERENCE,  # type: ignore
											TypeKind.RVALUEREFERENCE): # type: ignore
			raise_error(cursor, 'Only one pointer or reference allowed in an API type.')

		# Special case handling for pointers and references to fundamental types.
		if pointee_type_canonical.kind in _clang_to_ctypes:
			if result_kind is type_string_kind.arg_hint:
				if cpp_type_canonical.kind is TypeKind.POINTER:  # type: ignore
					# Pointer args of fundamental type may be any kind of array.
					return '_Any'
				# Reference args take/modify a single ctypes element by
				# ctypes.byref().
				return _clang_to_ctypes[pointee_type_canonical.kind]

			if result_kind is type_string_kind.return_hint:
				if pointee_type_canonical.kind in _clang_to_ctypes_ptr_return:
					if cpp_type_canonical.kind in ( TypeKind.LVALUEREFERENCE,  # type: ignore
													TypeKind.RVALUEREFERENCE): # type: ignore
						# ctypes will convert these to "bytes" and "str" and eventually crash.
						raise_error(cursor, "Return char and wchar_t by reference unsupported.")
					return _clang_to_ctypes_ptr_return[pointee_type_canonical.kind]

				# Can't declare POINTER() return type due to 'Call expression
				# not allowed in type expression.'
				return '_Any'

			# Use the explicit ctypes pointer types when available.
			if pointee_type_canonical.kind in _clang_to_ctypes_ptr:
				return _clang_to_ctypes_ptr[pointee_type_canonical.kind]

			# Use the ctypes pointer type everywhere else.
			return f'_Ctypes.POINTER({_clang_to_ctypes[pointee_type_canonical.kind]})'

		# Signal that enums, structs and classes may need special handling.
		cpp_type_canonical = pointee_type_canonical
		is_pointer = True

	# If it isn't a fundamental type then there has to be a separate definition
	# available.
	definition_cursor = cpp_type_canonical.get_declaration()
	if not definition_cursor or not definition_cursor.is_definition():
		raise_error(cursor, f'Incomplete type: {cpp_type_ref.displayname}')

	if definition_cursor.kind in (CursorKind.ENUM_DECL, CursorKind.STRUCT_DECL, CursorKind.CLASS_DECL): # type: ignore
		py_name = calculate_python_package_path(definition_cursor)
		if not py_name in symbols:
			raise_error(definition_cursor, f'Missing definition for: {py_name}. Use ENTANGLEMENT_T.')

		if definition_cursor.kind == CursorKind.ENUM_DECL: # type: ignore
			if is_pointer:
				raise_error(definition_cursor, f'Cannot pass enums by pointer or reference. Use int.')
			if result_kind in (type_string_kind.ctypes_args,
					 			type_string_kind.ctypes_struct):
				# Just tell ctypes to marshal enums to their underlying type.
				return _clang_to_ctypes[definition_cursor.enum_type.kind]
			return 'int' # Use plain int for enum arg and return hints.

		# Classes and structs are usable directly by ctypes. These need to be
		# full path.
		if is_pointer:
			if result_kind is type_string_kind.ctypes_args:
				return f'_Ctypes.POINTER({py_name})'
			if result_kind is type_string_kind.ctypes_struct:
				# Using void here avoids a whole class definition dependency
				# graph situation that is intractable going from C++ to Python.
				return f'_Ctypes.c_void_p'
		return py_name

	raise_error(cursor, f'Unsupported definition kind {definition_cursor.kind}')

def emit_python_api_doc(tabs: str, cursor: Cursor) -> List[str]:
	'''
	Generates Python docstrings list from a cursor's raw comments. Returns an
	empty list if none.
	- `tabs` : String of tab characters for indentation.
	- `cursor` : Clang cursor containing comments to process. '''
	name = get_name(cursor)
	if cursor.kind in (	CursorKind.FUNCTION_DECL, # type: ignore
						CursorKind.CONSTRUCTOR, # type: ignore
						CursorKind.DESTRUCTOR, # type: ignore
						CursorKind.CXX_METHOD): # type: ignore
		name = f'{cursor.result_type.spelling} {cursor.displayname}'

	comment = cursor.raw_comment
	if comment:
		lines: List[str] = comment.splitlines()
		cleaned: List[str] = []
		for line in lines:
			line = line.strip()
			if line.startswith('///'):
				line = line[3:].strip()
			elif line.startswith('//'):
				line = line[2:].strip()
			line = line.replace('\\', '\\\\').replace("'", "\\'")
			cleaned.append(line)
		doc = f'\n{tabs}'.join(cleaned)
		if doc and not doc.isspace():
			return [f"{tabs}'''\n{tabs}### `{name}`\n{tabs}{doc} '''"]

	return [f"{tabs}''' ### `{name}` '''"]

def emit_ctypes_function_args(cursor: Cursor, symbols: Dict[str, List[Cursor]], overloaded: bool) -> str:
	'''
	Returns a comma-separated string of ctypes function call arguments with
	shims for pointers and references.
	- `cursor` : Clang cursor representing the function.
	- `symbols` : Dictionary of known symbols for type resolution.
	- `overloaded` : Boolean indicating if the function is overloaded. '''
	arg_list = []
	arg_index = 0
	for arg in cursor.get_arguments():
		arg_type_kind = arg.type.kind
		if overloaded:
			arg_name = f'_Args[{arg_index}]'
		else:
			arg_name = arg.spelling if arg.spelling else f'_Arg{arg_index}'
		if arg_type_kind == TypeKind.POINTER: # type: ignore
			pointee_type = arg.type.get_pointee()
			pointee_c_type = calculate_type_string(cursor, pointee_type, symbols, type_string_kind.ctypes_args)
			if pointee_c_type != 'None':
				arg_list.append(f'_Pointer_shim({arg_name}, {pointee_c_type})')
			else:
				arg_list.append(arg_name)
		elif arg_type_kind in (TypeKind.LVALUEREFERENCE, TypeKind.RVALUEREFERENCE): # type: ignore
			arg_list.append(f'_Ctypes.byref({arg_name})')
		else:
			arg_list.append(arg_name)
		arg_index += 1
	return ','.join(arg_list)

def emit_python_api_overload_arg0_isinstance(cursor: Cursor, symbols: Dict[str, List[Cursor]], overloaded: bool) -> str:
	'''
	This check is used to distinguish between overloaded functions with the same
	number of args by comparing the class or struct used by the first arg. The
	intention is to be able to support the normal C++ constructor overloads. It
	should also be possible to correctly specialize overloads for subclasses.
	However, this is only intended to work when a class or struct is being
	passed and may fail to disambiguate the cases normally handled just fine by
	_Pointer_shim. There is also no attempt made to try to disambiguate more
	than one fundamental type. Returns a boolean expression determining if the
	arg is an instance of the expected ctype. Does not support ctypes.Array,
	ctypes.POINTER or numpy.ndarray.
	- `cursor` : Clang cursor representing the function.
	- `symbols` : Dictionary of known symbols for type resolution.
	- `overloaded` : Boolean indicating if the function is overloaded. '''

	assert cursor.get_arguments(), 'Impossible to overload 0 args.'

	arg = next(cursor.get_arguments())
	if overloaded:
		arg_name = '_Args[0]'
	else:
		arg_name = arg.spelling if arg.spelling else '_Arg0'

	arg_type_kind = arg.type.kind
	if arg_type_kind in (TypeKind.POINTER, TypeKind.LVALUEREFERENCE, TypeKind.RVALUEREFERENCE): # type: ignore
		pointee_type = arg.type.get_pointee()
		pointee_c_type = calculate_type_string(cursor, pointee_type, symbols, type_string_kind.ctypes_args)
		return f'isinstance({arg_name}, {pointee_c_type})'

	c_type = calculate_type_string(cursor, arg.type, symbols, type_string_kind.ctypes_args)
	return f'isinstance({arg_name}, {c_type})'

def emit_python_api_function(namespace_tabs: str, cursor: Cursor, symbols: Dict[str, List[Cursor]], overloaded: bool) -> List[str]:
	'''
	Returns a list of strings forming a Python API function definition.
	- `namespace_tabs` : String of tab characters for indentation.
	- `cursor` : Clang cursor representing the function.
	- `symbols` : Dictionary of known symbols for type resolution.
	- `overloaded` : Boolean indicating if the function is overloaded. '''
	lines: List[str] = []

	if cursor.is_pure_virtual_method():
		# Ignore pure virtual methods. Python doesn't work that way.
		return lines

	# Decorators.
	static_method = is_staticmethod(cursor)
	if static_method:
		lines.append(namespace_tabs + '@_Staticmethod')
	if overloaded:
		lines.append(namespace_tabs + '@_Overload')

	function_name = get_dunder_name(cursor)

	# Arg type hints.
	arg_types : List[Tuple[str, str]] = []
	arg_index = 0
	for arg in cursor.get_arguments():
		py_type = calculate_type_string(cursor, arg.type, symbols, type_string_kind.arg_hint)
		arg_name = arg.spelling if arg.spelling else f'_Arg{arg_index}'
		arg_types.append((arg_name, py_type))
		arg_index = arg_index + 1
	return_py_type = calculate_type_string(cursor, cursor.result_type, symbols, type_string_kind.return_hint)

	# Assemble type hints.
	self_arg = 'self,' if not static_method else ''
	arg_hints = ','.join(f"{arg_name}:'{py_type}'" for arg_name, py_type in arg_types)
	return_hint = f"->'{return_py_type}'"
	lines.append(f'{namespace_tabs}def {function_name}({self_arg}{arg_hints}){return_hint}:')

	# Function body.
	function_tabs : str = namespace_tabs + '\t'
	lines += emit_python_api_doc(function_tabs, cursor)
	if overloaded:
		# This has to come after the doc string for an overload.
		lines.append(function_tabs + '...')
	else:
		# Function body calls ctypes function. Assemble ctypes function call
		# args.
		arg_str = emit_ctypes_function_args(cursor, symbols, overloaded)

		# Assemble return statement.
		internal_name = get_internal_name(cursor)
		self_arg = '_Ctypes.byref(self),' if not static_method else ''
		lines.append(f'{namespace_tabs}\treturn {internal_name}({self_arg}{arg_str}) # type: ignore')

	return lines

def emit_python_api_overload_selector(namespace_tabs: str, overloads: List[Cursor], symbols: Dict[str, List[Cursor]]) -> List[str]:
	'''
	Returns a list of strings forming the Python API overload selector function.
	- `namespace_tabs` : String of tab characters for indentation.
	- `overloads` : List of Clang cursors for overloaded functions.
	- `symbols` : Dictionary of known symbols for type resolution. '''
	lines: List[str] = []
	static_method = False
	if any(is_staticmethod(cursor) for cursor in overloads):
		lines.append(namespace_tabs + '@_Staticmethod')
		static_method = True

	function_name = get_dunder_name(overloads[0])

	# Boilerplate.
	self_arg = 'self,' if not static_method else ''
	lines += [  f'{namespace_tabs}def {function_name}({self_arg}*_Args,**_Kwargs):',
				f"{namespace_tabs}\tassert not _Kwargs, 'Keyword arguments.'",
				f'{namespace_tabs}\tmatch _Len(_Args):' ]

	# Group overloads by argument count.
	arg_count_map: Dict[int, List[Cursor]] = {}
	for cursor in overloads:
		if cursor.is_pure_virtual_method():
			# Ignore pure virtual methods. Python doesn't work that way.
			continue

		arg_count = len(list(cursor.get_arguments()))
		if arg_count not in arg_count_map:
			arg_count_map[arg_count] = [cursor]
		else:
			arg_count_map[arg_count].append(cursor)

	# Handle each group for each argument count.
	self_arg = '_Ctypes.byref(self),' if not static_method else ''
	for arg_count, overload_group in sorted(arg_count_map.items()):
		lines.append(f'{namespace_tabs}\t\tcase {arg_count}:')

		inheritance_generation_map: Dict[int, List[Cursor]] = {0:[]}
		for cursor in overload_group:
			generation = 0
			if arg_count:
				# This should all work with code that managed to compile.
				arg0_type = next(cursor.get_arguments()).type.get_canonical()
				if arg0_type.kind in (TypeKind.POINTER, TypeKind.LVALUEREFERENCE, TypeKind.RVALUEREFERENCE): # type: ignore
					arg0_type = arg0_type.get_pointee().get_canonical()

				generation = get_inheritance_generation(arg0_type.get_declaration())
			if generation not in inheritance_generation_map:
				inheritance_generation_map[generation] = [cursor]
			else:
				inheritance_generation_map[generation].append(cursor)

		if len(inheritance_generation_map[0]) > 1:
			raise_error(inheritance_generation_map[0][0],
				'Unable to disambiguate overloaded functions by arg count and class of the first arg.')

		counter = len(overload_group)
		for generation, generation_group in sorted(inheritance_generation_map.items(), reverse=True):
			for cursor in generation_group:
				internal_name = get_internal_name(cursor)
				arg_str = emit_ctypes_function_args(cursor, symbols, True)

				# Try using isinstance to select between overloads. This can be
				# allowed to fail without further overhead because ctypes will
				# safely throw an exception. Fundamental types are left for
				# last. See emit_python_api_overload_arg0_isinstance for docs.
				if counter > 1:
					counter -= 1
					assert generation > 0, 'isinstance checks are only for classes and structs.'
					arg0_selector = emit_python_api_overload_arg0_isinstance(cursor, symbols, True)
					lines.append(f'{namespace_tabs}\t\t\tif {arg0_selector}:')
					lines.append(f'{namespace_tabs}\t\t\t\treturn {internal_name}({self_arg}{arg_str}) # type: ignore')
				else:
					lines.append(f'{namespace_tabs}\t\t\treturn {internal_name}({self_arg}{arg_str}) # type: ignore')

	lines += [  f'{namespace_tabs}\t\tcase _:',
				f"{namespace_tabs}\t\t\tassert False, f'Arg count: {{_Len(_Args)}}'",
				f'{namespace_tabs}\t\t\t...' ]
	return lines

def emit_python_api_enum(namespace_tabs: str, cursor: Cursor) -> List[str]:
	'''
	Returns a list of strings forming Python API enum definition.
	- `namespace_tabs` : String of tab characters for indentation.
	- `cursor` : Clang cursor representing an enum. '''
	enum_name = get_name(cursor)

	lines: List[str] = []

	# A plain Python Enum is not an int and enforces validation that C does not
	# which breaks normal C semantics for an enum. IntEnum does provide normal C
	# semantics for an enum. Iterating over IntFlags breaks due to a bug when
	# there is a negative value. Otherwise it sounded cool for non-class enums.
	lines.append(f'{namespace_tabs}class {enum_name}(_Enum.IntEnum):')

	enum_tabs = namespace_tabs + '\t'
	lines += emit_python_api_doc(enum_tabs, cursor)

	# HACK. Fix unsigned 64-bit types as clib is reinterpreting them as signed.
	underlying_type = cursor.enum_type.get_canonical()
	force_unsigned = underlying_type.kind in (TypeKind.UINT, TypeKind.ULONG, TypeKind.ULONGLONG) # type: ignore

	is_empty = True
	for constant in cursor.get_children():
		if constant.kind == CursorKind.ENUM_CONSTANT_DECL: # type: ignore
			child_enum_value = constant.enum_value if not force_unsigned else constant.enum_value & 0xffffffffffffffff
			lines.append(f'{enum_tabs}{constant.spelling}={child_enum_value}')
			is_empty = False
	if is_empty:
		raise_error(cursor, 'Empty enums not allowed in Python: ' + enum_name)

	# Put the named constants in the surrounding namespace.
	if not cursor.is_scoped_enum():
		for constant in cursor.get_children():
			if constant.kind == CursorKind.ENUM_CONSTANT_DECL: # type: ignore
				lines.append(f'{namespace_tabs}{constant.spelling}={enum_name}.{constant.spelling}')

	return lines

def emit_python_api_class(namespace_tabs: str, cursor: Cursor) -> List[str]:
	'''
	Returns a list of strings forming Python API class definition.
	- `namespace_tabs` : String of tab characters for indentation.
	- `cursor` : Clang cursor representing a class or struct. '''
	lines: List[str] = [f'{namespace_tabs}class {get_name(cursor)}:']
	lines += emit_python_api_doc(namespace_tabs + '\t', cursor)
	return lines

def emit_python_api(symbols: Dict[str, List[Cursor]], sorted_symbols: List[List[Cursor]], api: List[str]) -> None:
	'''
	Generates the Python API section of the output script.
	- `symbols` : Dictionary of known symbols mapped to their cursors.
	- `sorted_symbols` : List of cursor lists sorted by namespace and kind.
	- `api` : List to append generated Python API lines to. '''
	current_namespace : List[str] = []

	# The cursors have been sorted according to their python identifier. This
	# means their encapsulating namespaces (from namespaces, structs and
	# classes) will be visited in "collation order." This is used to produce
	# push and pop operations when the namespace stack changes. Those produce a
	# Python class hierarchy.
	for cursor_list in sorted_symbols:
		# Expect all cursors in a list to be the same type as a condition of the
		# code having compiled. Anonymous cursors have been given unique names.
		assert all(cursor.kind == cursor_list[0].kind for cursor in cursor_list)

		cursor0 = cursor_list[0]
		cursor_namespace = calculate_namespace(cursor0)

		# Find the prefix of the current namespace needed by the next symbol.
		# This is Python so there is nothing to do but drop indentation.
		# They keyword ... is being used to prevent errors with empty classes.
		while (len(cursor_namespace) < len(current_namespace)
				or current_namespace != cursor_namespace[:len(current_namespace)]):
			api.append(f"{'\t' * len(current_namespace)}...")
			leaving_namespace = current_namespace.pop()
			verbose(2, f'leaving_namespace {leaving_namespace}')

		# Cursors for namespaces are not being selected. Instead missing
		# namespace declarations are only being added as needed.
		while len(cursor_namespace) > len(current_namespace):
			namespace_depth = len(current_namespace)
			next_namespace : str = cursor_namespace[namespace_depth]
			api.append(f"{'\t' * namespace_depth}class {next_namespace}:")
			current_namespace.append(next_namespace)
			verbose(2, f'entering_namespace {next_namespace}')

		namespace_depth = len(current_namespace)
		namespace_tabs = '\t' * namespace_depth

		verbose(2, f'emit_python_api {calculate_python_package_path(cursor0)} {cursor0.kind.name}')

		if cursor0.kind is CursorKind.ENUM_DECL: # type: ignore
			assert len(cursor_list) == 1
			api += emit_python_api_enum(namespace_tabs, cursor0)

		elif cursor0.kind in (CursorKind.FUNCTION_DECL, CursorKind.CONSTRUCTOR, CursorKind.DESTRUCTOR, CursorKind.CXX_METHOD): # type: ignore
			if len(cursor_list) > 1:
				for cursor in cursor_list:
					api += emit_python_api_function(namespace_tabs, cursor, symbols, True)
				api += emit_python_api_overload_selector(namespace_tabs, cursor_list, symbols)
			else:
				api += emit_python_api_function(namespace_tabs, cursor0, symbols, False)

		elif cursor0.kind in (CursorKind.CLASS_DECL, CursorKind.STRUCT_DECL): # type: ignore
			assert len(cursor_list) == 1
			api += emit_python_api_class(namespace_tabs, cursor0)
			current_namespace.append(cursor0.spelling)

def emit_structure_list(symbols: Dict[str, List[Cursor]], sorted_symbols: List[List[Cursor]], structure_list: List[str]) -> None:
	'''
	Generates the structure list section of the output script.
	- `symbols` : Dictionary of known symbols mapped to their cursors.
	- `sorted_symbols` : List of cursor lists sorted by namespace and kind.
	- `structure_list` : List to append generated structure definition lines to. '''

	# Determine the inheritance depth of the symbols.
	depth_sorted_symbols: Dict[int, List[List[Cursor]]] = { }

	for cursor_list in sorted_symbols:
		cursor0 = cursor_list[0]
		if cursor0.kind in (CursorKind.CLASS_DECL, CursorKind.STRUCT_DECL): # type: ignore
			depth = get_inheritance_generation(cursor0)
			if depth not in depth_sorted_symbols:
				depth_sorted_symbols[depth] = [ cursor_list ]
			else:
				depth_sorted_symbols[depth].append(cursor_list)

	# Visit by depth and then sort order.
	counter = 0
	for depth in sorted(depth_sorted_symbols.keys()):
		symbols_at_depth = depth_sorted_symbols[depth]
		for cursor_list in symbols_at_depth:
			cursor0 = cursor_list[0]
			if cursor0.kind in (CursorKind.CLASS_DECL, CursorKind.STRUCT_DECL): # type: ignore
				assert len(cursor_list) == 1

				name = get_name(cursor0)
				path = calculate_python_package_path(cursor0)
				base = get_base_class(cursor0)
				# Place the API lookup before the ctypes.Structure lookup for
				# speed. That shouldn't hurt the perf of parameter passing.
				structure_list.append(f'class _T{counter}{name}({path}, {base}):')
				structure_list.append(f'\t_fields_ = [')

				if has_vtable(cursor0):
					structure_list.append(f"\t\t('_Vtable', _Ctypes.c_void_p),")

				field_count = 0
				for field in cursor0.get_children():
					if field.kind == CursorKind.FIELD_DECL: # type: ignore
						ctypes_type = calculate_type_string(field, field.type, symbols, type_string_kind.ctypes_struct)
						bits = f', {field.get_bitfield_width()}' if field.is_bitfield() else ''
						structure_list.append(f"\t\t('{field.spelling}', {ctypes_type}{bits}),")
						field_count += 1

				if not field_count:
					# Use a 1 byte pad variable. ctypes was not matching empty
					# classes.
					raise_error(cursor0, 'Empty class/struct unsupported due to ctypes.')

				structure_list.append(f'\t]\n{path}=_T{counter}{name}')
				counter += 1

				structure_list.append(f'assert _Ctypes.sizeof({path})=={cursor0.type.get_size()}')

def emit_symbol_table(symbols: Dict[str, List[Cursor]], sorted_symbols: List[List[Cursor]], symbol_table: List[str]) -> None:
	'''
	Generates the symbol table section of the output script.
	- `symbols` : Dictionary of known symbols mapped to their cursors.
	- `sorted_symbols` : List of cursor lists sorted by namespace and kind.
	- `symbol_table` : List to append generated symbol table lines to. '''
	for cursor_list in sorted_symbols:
		if cursor_list[0].kind in ( CursorKind.FUNCTION_DECL, # type: ignore
									CursorKind.CONSTRUCTOR,   # type: ignore
									CursorKind.DESTRUCTOR,	# type: ignore
									CursorKind.CXX_METHOD):   # type: ignore
			for cursor in cursor_list:
				# Get types
				arg_types : List[str]= []
				for arg in cursor.get_arguments():
					ctypes_type = calculate_type_string(arg, arg.type, symbols, type_string_kind.ctypes_args)
					arg_types.append(ctypes_type)
				return_type = calculate_type_string(cursor, cursor.result_type, symbols, type_string_kind.ctypes_args)

				# format them
				internal_name = get_internal_name(cursor)
				symbol_name = get_cxx_symbol_name(cursor)
				comment = f' # {cursor.result_type.spelling} {cursor.displayname}'
				self_ptr = '_Ctypes.c_void_p,' if cursor.kind != CursorKind.FUNCTION_DECL else '' # type: ignore

				symbol_table += [
					f"{internal_name}=_Cdll.{symbol_name}{comment}",
					f"{internal_name}.argtypes=[{self_ptr}{','.join(arg_types)}]",
					f"{internal_name}.restype={return_type}"
				]

def symbols_add(cursor: Cursor, symbols: Dict[str, List[Cursor]]) -> None:
	'''
	Adds a cursor to the symbols dictionary by its Python path. The symbols for
	each path will have to all be the same type. This is where symbols get
	dropped because they have already been seen in another translation unit.
	- `cursor` : Clang cursor to add.
	- `symbols` : Dictionary to store the cursor, mapped by Python path. '''
	if not cursor.is_anonymous():
		if (cursor.spelling.startswith('_')
				and len(cursor.spelling) > 1 and cursor.spelling[1].isupper()):
			# The C/C++ standards also reserve the _[A-Z] prefix and the __
			# prefix will break due to Python's automatic renaming. This leaves
			# _[A-Z] as the only safe place to put the symbol table and shims.
			raise_error(cursor, '1 leading underscore followed by a capital letter reserved by entanglement.py.')
		if cursor.spelling in keyword.kwlist or cursor.spelling in keyword.softkwlist:
			raise_error(cursor, cursor.spelling + ' is a keyword in Python.')
		if cursor.spelling in ctypes_reserved:
			raise_error(cursor, cursor.spelling + ' is reserved by ctypes.Structure.')

	sym : str = calculate_python_package_path(cursor)
	if sym not in symbols:
		symbols[sym] = [ cursor ]
	elif not any(c.get_usr() == cursor.get_usr() for c in symbols[sym]):
		symbols[sym].append(cursor)

def symbols_sort(symbols: Dict[str, List[Cursor]], sorted_symbols: List[List[Cursor]]) -> None:
	'''
	This is the final output order for the python api. Symbols are sorted first
	by namespace, then by cursor kind and then by name, if any. The symbols
	object is built up using the Python path instead of exposing the sort key.
	This keeps the details of the sort local to this function. Sorting by
	dunder name is required to build overload lists correctly.
	- `symbols` : Dictionary of known symbols mapped to their cursors.
	- `sorted_symbols` : List to append sorted cursor lists to. '''
	symbols_by_sort_key: Dict[str, List[Cursor]] = { }

	# Expects class methods to follow the class declaration.
	for cursor_list in symbols.values():
		cursor0 = cursor_list[0]
		sort_key = calculate_namespace(cursor0)
		sort_key.append(get_dunder_name(cursor0))
		sort_key_str = '.'.join(sort_key)
		assert not sort_key_str in symbols_by_sort_key
		symbols_by_sort_key[sort_key_str] = cursor_list

	for key in sorted(symbols_by_sort_key.keys()):
		sorted_symbols.append(symbols_by_sort_key[key])

def symbols_gather_required(cursor: Cursor, symbols: Dict[str, List[Cursor]]) -> None:
	'''
	Collects annotated symbols required for binding.
	- `cursor` : Clang cursor to analyze.
	- `symbols` : Dictionary to store collected cursors, mapped by Python path. '''
	verbose(3, f'ast_traversal {cursor.kind.name} {cursor.displayname}')

	if cursor.kind in (	CursorKind.TRANSLATION_UNIT, # type: ignore
						CursorKind.NAMESPACE, # type: ignore
						CursorKind.LINKAGE_SPEC): # type: ignore
		... # fallthrough

	elif not is_annotated_entanglement(cursor):
		# Ignore everything but ENTANGLEMENT_T/ENTANGLEMENT.
		return

	elif (cursor.kind in (
			CursorKind.CLASS_TEMPLATE, # type: ignore
			CursorKind.FUNCTION_TEMPLATE, # type: ignore
			CursorKind.CLASS_TEMPLATE_PARTIAL_SPECIALIZATION # type: ignore
		)):
		raise_error(cursor, f'{cursor.displayname} Templates are not supported.')

	elif cursor.kind is CursorKind.ENUM_DECL: # type: ignore
		if cursor.is_definition() and cursor.access_specifier in (
				AccessSpecifier.PUBLIC, # type: ignore
				AccessSpecifier.INVALID): # type: ignore # Top-level enums
			symbols_add(cursor, symbols)
			return
		else:
			raise_error(cursor, f'{get_name(cursor)} Enum is incomplete or private.')

	elif cursor.kind is CursorKind.FUNCTION_DECL: # type: ignore
		if (cursor.linkage is LinkageKind.EXTERNAL # type: ignore
				and not cursor.type.is_function_variadic()
				and not is_arg_va_list(cursor)):
			symbols_add(cursor, symbols)
			return
		else:
			raise_error(cursor, f'{cursor.displayname} Functions must be extern and not variadic.')

	elif cursor.kind in (CursorKind.CLASS_DECL, CursorKind.STRUCT_DECL): # type: ignore
		if (cursor.is_definition() and not cursor.is_anonymous() and cursor.access_specifier in (
				AccessSpecifier.PUBLIC, # type: ignore
				AccessSpecifier.INVALID)): # type: ignore # Top-level classes/classes
			symbols_add(cursor, symbols)
		else:
			raise_error(cursor, f'{get_name(cursor)} Structs and classes must be complete, not be anonymous and be public.')

	elif cursor.kind in (CursorKind.CONSTRUCTOR, CursorKind.DESTRUCTOR, CursorKind.CXX_METHOD): # type: ignore
		if (cursor.access_specifier is AccessSpecifier.PUBLIC # type: ignore
				and cursor.linkage is LinkageKind.EXTERNAL # type: ignore
				and not cursor.type.is_function_variadic()
				and not is_arg_va_list(cursor)):
			symbols_add(cursor, symbols)
			return
		else:
			raise_error(cursor, f'{cursor.displayname} Methods must be public, extern and not variadic.')

	else:
		raise_error(cursor, f'Unsupported {cursor.kind.name} {get_name(cursor)}')

	# fallthrough to children.
	for c in cursor.get_children():
		symbols_gather_required(c, symbols)

def load_translation_unit(header_file: str) -> TranslationUnit:
	'''
	Loads and parses a C++ header file into a Clang translation unit. Returns
	the parsed translation unit or raises an error if parsing fails.
	- `header_file` : Path to the C++ header file to parse. '''
	index = Index.create()
	translation_unit: Optional[TranslationUnit] = None
	try:
		translation_unit = index.parse(header_file, _arg_compiler_flags,
			options=TranslationUnit.PARSE_SKIP_FUNCTION_BODIES)
	except Exception as e:
		object_debugger(e)
		raise e

	if translation_unit:
		for diagnostic in translation_unit.diagnostics:
			print(diagnostic, file=sys.stderr)

	if not translation_unit or any(
			d.severity >= Diagnostic.Error for d in translation_unit.diagnostics):
		raise error(f'Translation unit would not compile: {header_file}')

	return translation_unit

def assemble_output_file(python_api: List[str], structure_list: List[str], symbol_table: List[str]) -> List[str]:
	'''
	Returns a list of strings forming the complete script assembled from the
	API, structure, and symbol table sections.
	- `python_api` : List of strings for the Python API section.
	- `structure_list` : List of strings for the structure definitions.
	- `symbol_table` : List of strings for the symbol table. '''
	# ctypes.POINTER and ctypes.Array do not require special handling in
	# _Pointer_shim.
	return [
		f"''' entanglement.py {_version} {datetime.now()}",
		f'\tbindings for {_arg_lib_name}',
		f"\t{' '.join(sys.argv)} '''",
'''
import ctypes as _Ctypes
import enum as _Enum
import numpy as _Numpy
import os as _OS
import sys as _Sys
from typing import Any as _Any
from typing import overload as _Overload
_Exception = Exception
_Len = len
_Staticmethod = staticmethod

# PYTHON API ----------------------------------------------------------

def _Pointer_shim(_Obj: _Any, _CType):
	if isinstance(_Obj, _CType):
		return _Ctypes.byref(_Obj)
	elif isinstance(_Obj, _Numpy.ndarray):
		return _Numpy.ctypeslib.as_ctypes(_Obj)
	return _Obj
'''
	] + python_api + [
'''
# STRUCTURE LIST ------------------------------------------------------
'''
	] + structure_list + [
f"""
# SYMBOL TABLE --------------------------------------------------------

_Path = _OS.path.join(_OS.path.dirname(_OS.path.abspath(__file__)), '{_arg_lib_name}')
try:
	_Cdll = _Ctypes.CDLL(_Path)
except _Exception as _E:
	__builtins__.print(f'missing_library {{_E}}', file=_Sys.stderr)
	_Sys.exit(1)
"""
	] + symbol_table + [
'''
# 🐉🐉🐉
'''
	]

def main(argv: List[str]) -> None:
	'''See usage message at top of file. '''
	if not parse_argv(argv):
		raise usage(_usage)

	verbose(1, 'version:        ' + _version)
	verbose(1, 'compiler_flags: ' + ' '.join(_arg_compiler_flags))
	verbose(1, 'lib_name:       ' + _arg_lib_name)
	verbose(1, 'header_files:   ' + ' '.join(_arg_header_files))
	verbose(1, 'output_file:    ' + _arg_output_file)
	verbose(1, 'library_file:   ' + _libclang_path)

	Config.set_library_file(_libclang_path)

	# Merge and sort all the cursors of interest from all the translation units.
	# Using a Python class to implement namespaces requires merging the
	# namespace declarations. And Python's function overloading situationship
	# requires figuring out all of the function overloads in advance.
	symbols: Dict[str, List[Cursor]] = { }
	for header_file in _arg_header_files:
		verbose(1, 'load_translation_unit ' + header_file)
		translation_unit = load_translation_unit(header_file)
		symbols_gather_required(translation_unit.cursor, symbols)

	if not symbols:
		raise error('No symbols found. Use -DENTANGLEMENT_PASS=1, ENTANGLEMENT_T and ENTANGLEMENT.')

	# Sort the symbols into their final order.
	sorted_symbols: List[List[Cursor]] = []
	symbols_sort(symbols, sorted_symbols)

	# Build the script sections.
	python_api: List[str] = []
	structure_list: List[str] = []
	symbol_table: List[str] = []
	emit_python_api(symbols, sorted_symbols, python_api)
	emit_structure_list(symbols, sorted_symbols, structure_list)
	emit_symbol_table(symbols, sorted_symbols, symbol_table)

	# Assemble them into an output file.
	output_lines: List[str] = assemble_output_file(python_api, structure_list, symbol_table)

	# Write output.
	with open(_arg_output_file, 'w') as f:
		f.write('\n'.join(output_lines))
		verbose(1, f'status_message Wrote {len(output_lines)} lines to {_arg_output_file}.')
		return # success

	raise error(f'File not written: {_arg_output_file}')

# Allow exceptions to go uncaught. This provides stack traces for unexpected
# failures.
if __name__ == '__main__':
	main(sys.argv[1:])
