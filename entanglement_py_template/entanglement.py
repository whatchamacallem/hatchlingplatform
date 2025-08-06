# SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
# SPDX-License-Identifier: MIT
# This file is licensed under the terms of the LICENSE.md file.

'''
entanglement.py: Automatic Binding Generator for C++ Projects

version: 0.0.1-pre-alpha

Usage:
	python entanglement.py <compiler_flags> <lib_name> <header_files>... <output_file>
		compiler_flags  - Flags to pass to clang. Can be in any order on command line.
		lib_name		- C/C++ library/.so name to bind everything to.
		header_files... - Path(s) to the C++ header file(s) to parse.
		output_file	 - Path to write the generated Python binding code. '''

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

# Processed command line arguments.
_arg_compiler_flags: List[str] = []
_arg_lib_name: str = ''
_arg_header_files: List[str] = []
_arg_output_file: str = ''

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

# ctypes has special type names for void*, char* wchar_t*.
_clang_to_ctypes_ptr: Dict[TypeKind, str] = {
	TypeKind.VOID: 		'_Ctypes.c_void_p',		# type: ignore # void*
	TypeKind.CHAR_S:	'_Ctypes.c_char_p',		# type: ignore # char*
	TypeKind.WCHAR:		'_Ctypes.c_wchar_p',	# type: ignore # wchar_t*
}

# ctypes has special type conversions for void*, char* wchar_t* when they are
# returned from a function. void* are returned as ints that have to be cast to
# the right kind of pointer.
_clang_to_ctypes_ptr_return: Dict[TypeKind, str] = {
	TypeKind.VOID:		'int',		# type: ignore # void*
	TypeKind.CHAR_S:	'bytes',	# type: ignore # char*
	TypeKind.WCHAR:		'str',		# type: ignore # wchar_t*
}

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

# The order symbols are sorted within a particular namespace.
_sort_order = [
	CursorKind.ENUM_DECL,     # type: ignore
	CursorKind.FUNCTION_DECL, # type: ignore
	CursorKind.STRUCT_DECL,   # type: ignore
	CursorKind.CLASS_DECL,    # type: ignore
	CursorKind.CONSTRUCTOR,   # type: ignore
	CursorKind.DESTRUCTOR,    # type: ignore
	CursorKind.CXX_METHOD     # type: ignore
]

class type_string_kind(enum.Enum):
	arg_hint = 0,
	return_hint = 1,
	ctypes_parameters = 2,
	ctypes_structure = 3

def verbose(verbose_level: int, x: str) -> None:
	'''
	Prints `x` if `verbose_level >= VERBOSE`.
	- `verbose_level` : [0..3] Ranges from "no news" to debug diagnostics.
	- `x` : The message to be printed (or not).'''
	if VERBOSE >= verbose_level:
		print(f' {'?*+-'[verbose_level]} {x}', file=sys.stderr)

def parse_argv(argv: List[str]) -> bool:
	'''
	Parses the command-line arguments into global constants. Returns `True` if
	parsing succeeds, `False` if insufficient non-switch arguments.
	- `argv`: List of command-line arguments as strings.'''
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
	- `obj`: Any object to inspect and print attributes for.'''
	for attribute_name in dir(obj):
		try:
			attribute_value = getattr(obj, attribute_name)
			print(f'  {attribute_name}: {attribute_value}', file=sys.stderr)
		except:
			pass

def throw_cursor(c: Cursor, message: str) -> NoReturn:
	'''
	Formats the source code location and leaves the message to the caller.
	Raises a ValueError.
	- `c`: Clang cursor indicating the source location of the error.
	- `message`: Error message to include in the raised exception.'''
	raise ValueError(f'{c.location.file}:{c.location.line}:{c.location.column} Error: {message}\n')

def get_bare_name(cursor: Cursor) -> str:
	'''
	Returns the cursor's name or a unique ID for anonymous cursors.
	- `cursor`: Clang cursor to extract the name from.'''
	return cursor.spelling if not cursor.is_anonymous() else '_ID' + str(hex(hash(cursor.get_usr())))[3:]

def get_mangled_name(cursor: Cursor) -> str:
	'''
	Returns the mangled name of the cursor or its bare name if not mangled.
	- `cursor`: Clang cursor to extract the mangled name from.'''
	return cursor.mangled_name or get_bare_name(cursor)

def is_annotated_entanglement(cursor: Cursor) -> bool:
	'''
	Checks if a cursor has the 'entanglement' annotation.
	- `cursor`: Clang cursor to check for annotations.'''
	for c in cursor.get_children():
		if c.kind == CursorKind.ANNOTATE_ATTR: # type: ignore
			if 'entanglement' == c.spelling:
				return True
	return False

def is_arg_va_list(cursor: Cursor) -> bool:
	'''
	Checks if a cursor's arguments include a va_list.
	- `cursor`: Clang cursor representing a function to check.'''
	return any("va_list" in a.type.spelling for a in cursor.get_arguments())

def is_staticmethod(cursor: Cursor) -> bool:
	'''
	Determines if a cursor represents a static method or free function.
	- `cursor`: Clang cursor to check.'''
	return cursor.kind is CursorKind.FUNCTION_DECL or ( # type: ignore
		cursor.kind is CursorKind.CXX_METHOD and cursor.storage_class == StorageClass.STATIC) # type: ignore

def calculate_namespace(cursor: Cursor) -> List[str]:
	'''
	Returns a list of namespace names in order from outermost to innermost.
	- `cursor`: Clang cursor to analyze.'''
	namespaces: List[str] = []
	current: Optional[Cursor] = cursor.semantic_parent
	while current and current.kind is not CursorKind.TRANSLATION_UNIT: # type: ignore
		if current.kind == CursorKind.LINKAGE_SPEC: # type: ignore
			return [] # Code declared extern "C" has no namespace.

		if not current.is_anonymous():
			namespaces.append(current.spelling)
		current = current.semantic_parent
	namespaces.reverse()
	return namespaces

# Anonymous namespaces are traversed as they may contain implementation details
# like base class layouts that are needed by the bindings code. Anonymous enums
# and structs are given globally unique ids.
def calculate_python_package_path(cursor: Cursor) -> str:
	'''
	Returns a dot-separated string representing the Python package path.
	- `cursor`: Clang cursor to compute the path for.'''
	namespaces: List[str] = calculate_namespace(cursor)
	namespaces.append(get_bare_name(cursor))
	return '.'.join(namespaces)

def get_inheritance_depth(cursor: Cursor) -> int:
	'''
	Returns the number of inheritance levels (0 for no base class).
	- `cursor`: Clang cursor representing a class or struct.'''
	for child in cursor.get_children():
		if child.kind == CursorKind.CXX_BASE_SPECIFIER: # type: ignore
			return get_inheritance_depth(child.referenced) + 1
	return 0

def get_base_class(cursor: Cursor) -> str:
	'''
	Returns the Python package path of the base class or '_Ctypes.Structure' if none.
	- `cursor`: Clang cursor representing a class or struct.'''
	if sum(c.kind == CursorKind.CXX_BASE_SPECIFIER for c in cursor.get_children()) > 1: # type: ignore
		throw_cursor(cursor, 'Multiple inheritance unsupported by Python\'s ctypes module.')

	for child in cursor.get_children():
		if child.kind == CursorKind.CXX_BASE_SPECIFIER: # type: ignore
			return calculate_python_package_path(child.referenced)
	return '_Ctypes.Structure'

def has_vtable(cursor: Cursor):
	'''
	Returns `True` if the class has virtual methods and is the first virtual class, `False` otherwise.
	- `cursor`: Clang cursor representing a class or struct.'''
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
	- `cursor`: Clang cursor for error reporting.
	- `cpp_type_ref`: Clang type to convert.
	- `symbols`: Dictionary of known symbols for type resolution.
	- `result_kind`: Specifies the context (arg_hint, return_hint, ctypes_parameters, ctypes_structure).'''
	# Handle typedefs by resolving to the canonical type.
	cpp_type_canonical = cpp_type_ref.get_canonical()

	# Handle fundamental types.
	if cpp_type_canonical.kind in _clang_to_ctypes:
		if result_kind in (type_string_kind.ctypes_parameters,
					 		type_string_kind.ctypes_structure):
			return _clang_to_ctypes[cpp_type_canonical.kind]
		if result_kind is type_string_kind.arg_hint:
			if cpp_type_canonical.kind is TypeKind.WCHAR: # type: ignore
				return 'str' # Single characters are passed as strings in Python.
		return _clang_to_python[cpp_type_canonical.kind]

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
			throw_cursor(cursor, 'Only one pointer or reference allowed in an API type.')

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
						throw_cursor(cursor, "Return char and wchar_t by reference unsupported.")
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
		throw_cursor(cursor, f'Incomplete type: {cpp_type_ref.displayname}')

	if definition_cursor.kind in (CursorKind.ENUM_DECL, CursorKind.STRUCT_DECL, CursorKind.CLASS_DECL): # type: ignore
		py_name = calculate_python_package_path(definition_cursor)
		if not py_name in symbols:
			throw_cursor(definition_cursor, f'Missing definition for: {py_name}. Use ENTANGLEMENT_T.')

		if definition_cursor.kind == CursorKind.ENUM_DECL: # type: ignore
			if is_pointer:
				throw_cursor(definition_cursor, f'Cannot pass enums by pointer or reference. Use int.')
			if result_kind in (type_string_kind.ctypes_parameters,
					 			type_string_kind.ctypes_structure):
				# Just tell ctypes to marshal enums to their underlying type.
				return _clang_to_ctypes[definition_cursor.enum_type.kind]
			return 'int' # Use plain int for enum arg and return hints.

		# Classes and structs are usable directly by ctypes. These need to be
		# full path.
		if is_pointer:
			if result_kind is type_string_kind.ctypes_parameters:
				return f'_Ctypes.POINTER({py_name})'
			if result_kind is type_string_kind.ctypes_structure:
				# Using void here avoids a whole class definition dependency
				# graph situation that is intractable going from C++ to Python.
				return f'_Ctypes.c_void_p'
		return py_name

	throw_cursor(cursor, f'Unsupported definition kind {definition_cursor.kind}')

def emit_python_api_doc(tabs: str, cursor: Cursor) -> List[str]:
	'''
	Generates Python docstrings list from a cursor's raw comments. Returns an
	empty list if none.
	- `tabs`: String of tab characters for indentation.
	- `cursor`: Clang cursor containing comments to process.'''
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
		if not doc or doc.isspace():
			return [ ]
		return [f"{tabs}'''\n{tabs}{doc} '''"]
	return [ ]

def emit_ctypes_function_args(cursor: Cursor, symbols: Dict[str, List[Cursor]], overloaded: bool) -> str:
	'''
	Returns a comma-separated string of ctypes function call arguments with
	shims for pointers and references.
	- `cursor`: Clang cursor representing the function.
	- `symbols`: Dictionary of known symbols for type resolution.
	- `overloaded`: Boolean indicating if the function is overloaded.'''
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
			pointee_c_type = calculate_type_string(cursor, pointee_type, symbols, type_string_kind.ctypes_parameters)
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

def emit_python_api_function(namespace_tabs: str, cursor: Cursor, symbols: Dict[str, List[Cursor]], overloaded: bool) -> List[str]:
	'''
	Returns a list of strings forming a Python API function definition.
	- `namespace_tabs`: String of tab characters for indentation.
	- `cursor`: Clang cursor representing the function.
	- `symbols`: Dictionary of known symbols for type resolution.
	- `overloaded`: Boolean indicating if the function is overloaded.'''
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

	# XXX operator names??
	function_name = get_bare_name(cursor)
	if cursor.kind is CursorKind.CONSTRUCTOR: # type: ignore
		function_name = '__init__'
	elif cursor.kind is CursorKind.DESTRUCTOR: # type: ignore
		function_name = '__del__'

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
	self_parameter = 'self,' if not static_method else ''
	arg_hints = ','.join(f"{arg_name}:'{py_type}'" for arg_name, py_type in arg_types)
	return_hint = f"->'{return_py_type}'"
	lines.append(f'{namespace_tabs}def {function_name}({self_parameter}{arg_hints}){return_hint}:')

	# Function body.
	function_tabs : str = namespace_tabs + '\t'
	lines += emit_python_api_doc(function_tabs, cursor)
	if overloaded:
		lines.append(function_tabs + '...')
	else:
		# Function body calls ctypes function. Assemble ctypes function call
		# args.
		arg_str = emit_ctypes_function_args(cursor, symbols, overloaded)

		# Assemble return statement.
		mangled_name = get_mangled_name(cursor)
		self_parameter = '_Ctypes.byref(self),' if not static_method else ''
		lines.append(f"{namespace_tabs}\treturn {mangled_name}({self_parameter}{arg_str}) # {cursor.displayname}")

	return lines

def emit_python_api_overload_selector(namespace_tabs: str, overloads: List[Cursor], symbols: Dict[str, List[Cursor]]) -> List[str]:
	'''
	Returns a list of strings forming the Python API overload selector function.
	- `namespace_tabs`: String of tab characters for indentation.
	- `overloads`: List of Clang cursors for overloaded functions.
	- `symbols`: Dictionary of known symbols for type resolution.'''
	lines: List[str] = []
	static_method = False
	if any(is_staticmethod(cursor) for cursor in overloads):
		lines.append(namespace_tabs + "@_Staticmethod")
		static_method = True

	# All cursors are the same type. Use the first one to identify them.
	cursor0 = overloads[0]

	# XXX operator names??
	function_name = cursor0.spelling
	if cursor0.kind is CursorKind.CONSTRUCTOR: # type: ignore
		function_name = '__init__'
	elif cursor0.kind is CursorKind.DESTRUCTOR: # type: ignore
		function_name = '__del__'

	# Boilerplate.
	self_parameter = 'self,' if not static_method else ''
	lines += [  f'{namespace_tabs}def {function_name}({self_parameter}*_Args,**_Kwargs):',
				f"{namespace_tabs}\tassert not _Kwargs, 'keyword_arguments'",
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
	for arg_count, overload_group in sorted(arg_count_map.items()):
		if len(overload_group) > 1:
			# XXX Dispatch by argument count first and then by parameter type(s) second.
			for item in overload_group:
				print('Error: ' + item.displayname, file=sys.stderr)
			throw_cursor(overload_group[0],
				f"Multiple overloads of {overload_group[0].spelling} with {arg_count} args.")

		cursor = overload_group[0]
		mangled_name = get_mangled_name(cursor)
		self_parameter = '_Ctypes.byref(self), ' if not static_method else ''
		arg_str = emit_ctypes_function_args(cursor, symbols, True)

		lines += [
			f"{namespace_tabs}\t\tcase {arg_count}:",
			f"{namespace_tabs}\t\t\treturn {mangled_name}({self_parameter}{arg_str})"
		]

	lines += [  f"{namespace_tabs}\t\tcase _:",
				f"{namespace_tabs}\t\t\tassert False, f'overload_resolution len {{_Len(_Args)}}'",
				f"{namespace_tabs}\t\t\tpass" ]
	return lines

def emit_python_api_enum(namespace_tabs: str, cursor: Cursor) -> List[str]:
	'''
	Returns a list of strings forming Python API enum definition.
	- `namespace_tabs`: String of tab characters for indentation.
	- `cursor`: Clang cursor representing an enum.'''
	enum_name = get_bare_name(cursor)

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
	for child in cursor.get_children():
		if child.kind == CursorKind.ENUM_CONSTANT_DECL: # type: ignore
			child_enum_value = child.enum_value if not force_unsigned else child.enum_value & 0xffffffffffffffff
			lines.append(f'{enum_tabs}{child.spelling}={child_enum_value}')
			is_empty = False
	if is_empty:
		throw_cursor(cursor, 'Empty enums not allowed in Python: ' + enum_name)

	# Put the named constants in the surrounding namespace.
	if not cursor.is_scoped_enum():
		for child in cursor.get_children():
			if child.kind == CursorKind.ENUM_CONSTANT_DECL: # type: ignore
				lines.append(f'{namespace_tabs}{child.spelling}={enum_name}.{child.spelling}')

	return lines

def emit_python_api_class(namespace_tabs: str, cursor: Cursor) -> List[str]:
	'''
	Returns a list of strings forming Python API class definition.
	- `namespace_tabs`: String of tab characters for indentation.
	- `cursor`: Clang cursor representing a class or struct.'''
	lines: List[str] = [f'{namespace_tabs}class {get_bare_name(cursor)}:']
	lines += emit_python_api_doc(namespace_tabs + '\t', cursor)
	return lines

def emit_python_api(symbols: Dict[str, List[Cursor]], sorted_symbols: List[List[Cursor]], api: List[str]) -> None:
	'''
	Generates the Python API section of the output script.
	- `symbols`: Dictionary of known symbols mapped to their cursors.
	- `sorted_symbols`: List of cursor lists sorted by namespace and kind.
	- `api`: List to append generated Python API lines to.'''
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
		# They keyword "pass" is being used to prevent errors.
		while (len(cursor_namespace) < len(current_namespace)
				or current_namespace != cursor_namespace[:len(current_namespace)]):
			api.append(f"{'\t' * len(current_namespace)}pass")
			leaving_namespace = current_namespace.pop()
			verbose(2, f'leaving_namespace {leaving_namespace}...')

		# Cursors for namespaces are not being selected. Instead missing
		# namespace declarations are only being added as needed.
		while len(cursor_namespace) > len(current_namespace):
			namespace_depth = len(current_namespace)
			next_namespace : str = cursor_namespace[namespace_depth]
			api.append(f"{'\t' * namespace_depth}class {next_namespace}:")
			current_namespace.append(next_namespace)
			verbose(2, f'entering_namespace {next_namespace}...')

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
			current_namespace += [ cursor0.spelling ]

def emit_structure_list(symbols: Dict[str, List[Cursor]], sorted_symbols: List[List[Cursor]], structure_list: List[str]) -> None:
	'''
	Generates the structure list section of the output script.
	- `symbols`: Dictionary of known symbols mapped to their cursors.
	- `sorted_symbols`: List of cursor lists sorted by namespace and kind.
	- `structure_list`: List to append generated structure definition lines to.'''
	# Determine the inheritance depth of the symbols.
	depth_sorted_symbols: Dict[int, List[List[Cursor]]] = { }

	for cursor_list in sorted_symbols:
		cursor0 = cursor_list[0]
		if cursor0.kind in (CursorKind.CLASS_DECL, CursorKind.STRUCT_DECL): # type: ignore
			depth = get_inheritance_depth(cursor0)
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

				name = get_bare_name(cursor0)
				base = get_base_class(cursor0)
				structure_list.append(f'class _T{counter}{name}({name}, {base}):')
				structure_list.append(f'\t_fields_ = [')

				if has_vtable(cursor0):
					structure_list.append(f"\t\t('_Vtable', _Ctypes.c_void_p),")

				for child in cursor0.get_children():
					if child.kind == CursorKind.FIELD_DECL: # type: ignore
						ctypes_type = calculate_type_string(child, child.type, symbols, type_string_kind.ctypes_structure)
						structure_list.append(f"\t\t('{child.spelling}', {ctypes_type}),")

				structure_list.append(f'\t]\n{name}=_T{counter}{name}')
				counter = counter + 1

				structure_list.append(f'assert _Ctypes.sizeof({name})=={cursor0.type.get_size()}')

def emit_symbol_table(symbols: Dict[str, List[Cursor]], sorted_symbols: List[List[Cursor]], symbol_table: List[str]) -> None:
	'''
	Generates the symbol table section of the output script.
	- `symbols`: Dictionary of known symbols mapped to their cursors.
	- `sorted_symbols`: List of cursor lists sorted by namespace and kind.
	- `symbol_table`: List to append generated symbol table lines to.'''
	for cursor_list in sorted_symbols:
		if cursor_list[0].kind in ( CursorKind.FUNCTION_DECL, # type: ignore
									CursorKind.CONSTRUCTOR,   # type: ignore
									CursorKind.DESTRUCTOR,	# type: ignore
									CursorKind.CXX_METHOD):   # type: ignore
			for cursor in cursor_list:
				# Get types
				arg_types : List[str]= []
				for arg in cursor.get_arguments():
					ctypes_type = calculate_type_string(arg, arg.type, symbols, type_string_kind.ctypes_parameters)
					arg_types.append(ctypes_type)
				return_type = calculate_type_string(cursor, cursor.result_type, symbols, type_string_kind.ctypes_parameters)

				# format them
				mangled_name = get_mangled_name(cursor)
				comment = f' # {cursor.displayname}'
				self_ptr = '_Ctypes.c_void_p,' if cursor.kind != CursorKind.FUNCTION_DECL else '' # type: ignore

				symbol_table += [
					f"{mangled_name}=_Cdll.{mangled_name}{comment}",
					f"{mangled_name}.argtypes=[{self_ptr}{','.join(arg_types)}]",
					f"{mangled_name}.restype={return_type}"
				]

def symbols_add(cursor: Cursor, symbols: Dict[str, List[Cursor]]) -> None:
	'''
	Adds a cursor to the symbols dictionary by its Python path. The symbols for
	each path will have to all be the same type. This is where symbols get
	dropped because they have already been seen in another translation unit.
	- `cursor`: Clang cursor to add.
	- `symbols`: Dictionary to store the cursor, mapped by Python path.'''
	if not cursor.is_anonymous():
		if cursor.spelling.startswith('__'):
			throw_cursor(cursor, '2 leading underscores reserved by Python.')
		if (cursor.spelling.startswith('_')
				and len(cursor.spelling) > 1 and cursor.spelling[1].isupper()):
			# The C/C++ standards also reserve the _[A-Z] prefix and the __
			# prefix will break due to Python's automatic renaming. This leaves
			# _[A-Z] as the only safe place to put the symbol table and shims.
			throw_cursor(cursor, '1 leading underscore followed by a capital letter reserved by entanglement.py.')
		if cursor.spelling in keyword.kwlist or cursor.spelling in keyword.softkwlist:
			throw_cursor(cursor, cursor.spelling + ' is a keyword in Python.')

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
	This keeps the details of the sort local to this function.
	- `symbols`: Dictionary of known symbols mapped to their cursors.
	- `sorted_symbols`: List to append sorted cursor lists to.'''
	symbols_by_sort_key: Dict[str, List[Cursor]] = { }

	for cursor_list in symbols.values():
		cursor0 = cursor_list[0]
		sort_key = calculate_namespace(cursor0)
		index : int = 0
		for i in range(len(_sort_order)):
			if _sort_order[i] == cursor0.kind:
				index = i + 1
				break
		sort_key.append(str(index))
		sort_key.append(get_bare_name(cursor0))
		sort_key_str = '.'.join(sort_key)
		assert not sort_key_str in symbols_by_sort_key
		symbols_by_sort_key[sort_key_str] = cursor_list

	for key in sorted(symbols_by_sort_key.keys()):
		sorted_symbols.append(symbols_by_sort_key[key])

def symbols_gather_required(cursor: Cursor, symbols: Dict[str, List[Cursor]]) -> None:
	'''
	Collects annotated symbols required for binding.
	- `cursor`: Clang cursor to analyze.
	- `symbols`: Dictionary to store collected cursors, mapped by Python path.'''
	verbose(3, f'ast_traversal {cursor.kind.name} {cursor.displayname}')

	if cursor.kind in (	CursorKind.TRANSLATION_UNIT, # type: ignore
						CursorKind.NAMESPACE, # type: ignore
						CursorKind.LINKAGE_SPEC): # type: ignore
		pass # fallthrough

	elif not is_annotated_entanglement(cursor):
		# Ignore everything but ENTANGLEMENT_T/ENTANGLEMENT.
		return

	elif (cursor.kind in (
			CursorKind.CLASS_TEMPLATE, # type: ignore
			CursorKind.FUNCTION_TEMPLATE, # type: ignore
			CursorKind.CLASS_TEMPLATE_PARTIAL_SPECIALIZATION # type: ignore
		)):
		throw_cursor(cursor, f'{cursor.displayname} Templates are not supported.')

	elif cursor.kind is CursorKind.ENUM_DECL: # type: ignore
		if cursor.is_definition() and cursor.access_specifier in (
				AccessSpecifier.PUBLIC, # type: ignore
				AccessSpecifier.INVALID): # type: ignore # Top-level enums
			symbols_add(cursor, symbols)
		else:
			throw_cursor(cursor, f'{get_bare_name(cursor)} Enum is incomplete or private.')

	elif cursor.kind is CursorKind.FUNCTION_DECL: # type: ignore
		if (cursor.linkage is LinkageKind.EXTERNAL # type: ignore
				and not cursor.type.is_function_variadic()
				and not is_arg_va_list(cursor)):
			symbols_add(cursor, symbols)
		else:
			throw_cursor(cursor, f'{cursor.displayname} Functions must be extern and not variadic.')

	elif cursor.kind in (CursorKind.CLASS_DECL, CursorKind.STRUCT_DECL): # type: ignore
		if (cursor.is_definition() and not cursor.is_anonymous() and cursor.access_specifier in (
				AccessSpecifier.PUBLIC, # type: ignore
				AccessSpecifier.INVALID)): # type: ignore # Top-level classes/classes
			symbols_add(cursor, symbols)
		else:
			throw_cursor(cursor, f'{get_bare_name(cursor)} Structs and classes must be complete, not be anonymous and be public.')

	elif cursor.kind in (CursorKind.CONSTRUCTOR, CursorKind.DESTRUCTOR, CursorKind.CXX_METHOD): # type: ignore
		if (cursor.access_specifier is AccessSpecifier.PUBLIC # type: ignore
				and cursor.linkage is LinkageKind.EXTERNAL # type: ignore
				and not cursor.type.is_function_variadic()
				and not is_arg_va_list(cursor)):
			symbols_add(cursor, symbols)
		else:
			throw_cursor(cursor, f'{cursor.displayname} Methods must be public, extern and not variadic.')

	else:
		throw_cursor(cursor, f'Unsupported {cursor.kind.name} {get_bare_name(cursor)}')

	# fallthrough to children.
	for c in cursor.get_children():
		symbols_gather_required(c, symbols)

def load_translation_unit(header_file: str) -> TranslationUnit:
	'''
	Loads and parses a C++ header file into a Clang translation unit. Returns
	the parsed translation unit or raises an error if parsing fails.
	- `header_file`: Path to the C++ header file to parse.'''
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
		raise ValueError(f'Error: Translation unit would not compile: {header_file}')

	return translation_unit

def assemble_output_file(python_api: List[str], structure_list: List[str], symbol_table: List[str]) -> List[str]:
	'''
	Returns a list of strings forming the complete script assembled from the
	API, structure, and symbol table sections.
	- `python_api`: List of strings for the Python API section.
	- `structure_list`: List of strings for the structure definitions.
	- `symbol_table`: List of strings for the symbol table.'''
	# ctypes.POINTER and ctypes.Array do not require special handling in
	# _Pointer_shim.
	return [
		f"''' entanglement.py {datetime.now()}",
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
_Len = len
_Staticmethod = staticmethod
_ValueError = ValueError

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
except Exception as e:
	__builtins__.print(f'missing_library {{e}}', file=_Sys.stderr)
	_Sys.exit(1)
"""
	] + symbol_table + [
'''
# 🐉🐉🐉
'''
	]

def main(argv: List[str]) -> None:
	'''See usage message in function body.'''
	if not parse_argv(argv):
		raise ValueError('''\
Usage: python3 entanglement.py <compiler_flags> <lib_name> <header_files>... <output_file>

Generates Python bindings for a C++ .so using a libclang command line, a C++
header and <entanglement.h>. Raises ValueError if arguments are invalid or
processing fails.

Arguments:
	compiler_flags  - Flags to pass to clang. Can be in any order on command line.
	lib_name		- C/C++ library/.so name to bind everything to.
	header_files... - Path(s) to the C++ header file(s) to parse.
	output_file	 - Path to write the generated Python binding code.
''')

	verbose(1, 'compiler_flags ' + ' '.join(_arg_compiler_flags))
	verbose(1, 'lib_name ' + _arg_lib_name)
	verbose(1, 'header_files ' + ' '.join(_arg_header_files))
	verbose(1, 'output_file ' + _arg_output_file)

	verbose(1, 'set_library_file ' +_libclang_path)
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
		raise ValueError('Error: No symbols found. Use -DENTANGLEMENT_PASS=1, ENTANGLEMENT_T and ENTANGLEMENT.')

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

	raise ValueError(f'Error: File not written: {_arg_output_file}')

# Allow exceptions to go uncaught. This provides stack traces for unexpected
# failures.
if __name__ == '__main__':
	main(sys.argv[1:])
