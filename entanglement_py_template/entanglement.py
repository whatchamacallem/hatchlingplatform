"""
entanglement.py: Automatic Binding Generator for C++ Projects

version: 0.0.1-pre-alpha

Usage:
    python entanglement.py <compiler_flags> <lib_name> <header_files>... <output_file>
        compiler_flags  - Flags to pass to clang. Can be in any order on command line.
        lib_name        - C/C++ library/.so name to bind everything to.
        header_files... - Path(s) to the C++ header file(s) to parse.
        output_file     - Path to write the generated Python binding code.
"""

import ctypes, os, sys
import clang.cindex
from typing import Dict, List, NoReturn, Optional, Set, Tuple
from clang.cindex import AccessSpecifier, Config, Cursor, CursorKind, Diagnostic
from clang.cindex import Index, LinkageKind, StorageClass, TranslationUnit, Type, TypeKind
from datetime import datetime

# Verbose - 0: Normal status and errors. 1: Processing steps. 2: AST traversal.
VERBOSE = 2

# Path to the libclang shared library. TODO throws exceptions.
_libclang_path = '/usr/lib/llvm-18/lib/libclang.so.1'

# Exit codes.
_exit_bindings_generated = 0
_exit_error = 1

# Processed command line arguments.
_arg_compiler_flags: List[str] = []
_arg_lib_name: str = ''
_arg_header_files: List[str] = []
_arg_output_file: str = ''

_clang_to_ctypes: Dict[TypeKind, str] = {
    TypeKind.BOOL:      'ctypes.c_bool',       # type: ignore # bool
    TypeKind.CHAR_S:    'ctypes.c_char',       # type: ignore # char
    TypeKind.CHAR_U:    'ctypes.c_ubyte',      # type: ignore # unsigned char
    TypeKind.DOUBLE:    'ctypes.c_double',     # type: ignore # double
    TypeKind.FLOAT:     'ctypes.c_float',      # type: ignore # float
    TypeKind.INT:       'ctypes.c_int',        # type: ignore # int
    TypeKind.LONG:      'ctypes.c_long',       # type: ignore # long
    TypeKind.LONGDOUBLE:'ctypes.c_longdouble', # type: ignore # long double
    TypeKind.LONGLONG:  'ctypes.c_longlong',   # type: ignore # long long
    TypeKind.SCHAR:     'ctypes.c_byte',       # type: ignore # signed char
    TypeKind.SHORT:     'ctypes.c_short',      # type: ignore # short
    TypeKind.UCHAR:     'ctypes.c_ubyte',      # type: ignore # unsigned char
    TypeKind.UINT:      'ctypes.c_uint',       # type: ignore # unsigned int
    TypeKind.ULONG:     'ctypes.c_ulong',      # type: ignore # unsigned long
    TypeKind.ULONGLONG: 'ctypes.c_ulonglong',  # type: ignore # unsigned long long
    TypeKind.USHORT:    'ctypes.c_ushort',     # type: ignore # unsigned short
    TypeKind.VOID:      'None',                # type: ignore # void
    TypeKind.WCHAR:     'ctypes.c_wchar',      # type: ignore # wchar_t
}

_clang_to_python: Dict[TypeKind, str] = {
    TypeKind.BOOL:       'bool',  # type: ignore
    TypeKind.CHAR_S:     'int',   # type: ignore
    TypeKind.CHAR_U:     'int',   # type: ignore
    TypeKind.DOUBLE:     'float', # type: ignore
    TypeKind.FLOAT:      'float', # type: ignore
    TypeKind.INT:        'int',   # type: ignore
    TypeKind.LONG:       'int',   # type: ignore
    TypeKind.LONGDOUBLE: 'float', # type: ignore
    TypeKind.LONGLONG:   'int',   # type: ignore
    TypeKind.SCHAR:      'int',   # type: ignore
    TypeKind.SHORT:      'int',   # type: ignore
    TypeKind.UCHAR:      'int',   # type: ignore
    TypeKind.UINT:       'int',   # type: ignore
    TypeKind.ULONG:      'int',   # type: ignore
    TypeKind.ULONGLONG:  'int',   # type: ignore
    TypeKind.USHORT:     'int',   # type: ignore
    TypeKind.VOID:       'None',  # type: ignore
    TypeKind.WCHAR:      'int',   # type: ignore
}

# The order symbols are sorted within a particular namespace.
_sort_order = [
    CursorKind.ENUM_DECL, # type: ignore
    CursorKind.FUNCTION_DECL, # type: ignore
    CursorKind.STRUCT_DECL, # type: ignore
    CursorKind.CLASS_DECL, # type: ignore
    CursorKind.CONSTRUCTOR, # type: ignore
    CursorKind.DESTRUCTOR, # type: ignore
    CursorKind.CXX_METHOD # type: ignore
]

def verbose(verbose_level: int, x: str) -> None:
    if VERBOSE >= verbose_level:
        print(f' {'?*+---'[verbose_level]} {x}')

def parse_argv() -> bool:
    global _arg_compiler_flags
    global _arg_lib_name
    global _arg_header_files
    global _arg_output_file

    non_flags: List[str] = []
    for arg in sys.argv[1:]:
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

# cindex may have extra exception data.
def exception_debugger(e: Exception) -> None:
    for attribute_name in dir(e):
        try:
            attribute_value = getattr(e, attribute_name)
            print(f'  {attribute_name}: {attribute_value}')
        except:
            pass

    print(e)
    verbose(1, 'exit_error')
    exit(_exit_error)

# Format the source code location and leave the message to the caller.
def throw_cursor(c: Cursor, message: str) -> NoReturn:
    raise ValueError(f'{c.location.file}:{c.location.line}:{c.location.column} {message}\n')

def get_bare_name(cursor: Cursor) -> str:
    return cursor.spelling if not cursor.is_anonymous() else '_E_' +  str(hex(hash(cursor.get_usr())))[4:12]

def get_mangled_name(cursor: Cursor) -> str:
    return cursor.mangled_name or get_bare_name(cursor)

def is_annotated_entanglement(cursor: Cursor) -> bool:
    for c in cursor.get_children():
        if c.kind == clang.cindex.CursorKind.ANNOTATE_ATTR: # type: ignore
            if 'entanglement' == c.spelling:
                return True
    return False

def is_arg_va_list(cursor: Cursor) -> bool:
    return any("va_list" in a.type.spelling for a in cursor.get_arguments())

def emit_python_api_doc(tabs: str, cursor: Cursor) -> List[str]:
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
            line = line.replace('\\', '\\\\').replace('"', '\\"')
            cleaned.append(line)
        doc = f'\n{tabs}'.join(cleaned) if cleaned else ''
        return [f'{tabs}"""\n{tabs}{doc} """' if doc else ""]
    return [ ]

def calculate_namespace(cursor: Cursor) -> List[str]:
    namespaces: List[str] = []
    current: Optional[Cursor] = cursor.semantic_parent
    while current and current.kind is not CursorKind.TRANSLATION_UNIT: # type: ignore
        if not current.is_anonymous():
            namespaces.append(current.spelling)
        current = current.semantic_parent
    namespaces.reverse()
    return namespaces

# Anonymous namespaces are traversed as they may contain implementation details
# like base class layouts that are needed by the bindings code. Anonymous enums
# and structs are given globally unique ids. TODO operator names.
def calculate_python_package_path(cursor: Cursor) -> str:
    namespaces: List[str] = calculate_namespace(cursor)
    namespaces.append(get_bare_name(cursor))
    return '.'.join(namespaces)

def calculate_python_api_type_map(cursor: Cursor, cpp_type_ref: Type, symbols: Dict[str, List[Cursor]], is_return: bool, depth: int = 0) -> Tuple[str, str]:
    """ clang type -> (ctype, python)"""

    # Handle typedefs by resolving to the canonical type
    cpp_type = cpp_type_ref.get_canonical()
    if not cpp_type:
        throw_cursor(cursor, 'calculate_python_api_type_map get_canonical')

    # Handle fundamental types
    if cpp_type.kind in _clang_to_ctypes:
        if depth and not is_return: #xxx
            # Have type hints that allow pointers to be used with any array class.
            # Then _pass_array_to_ctypes provides runtime resolution and asserts.
            return (_clang_to_ctypes[cpp_type.kind], 'Any')
        return (_clang_to_ctypes[cpp_type.kind], _clang_to_python[cpp_type.kind])

    # Pointers and references. The ctypes documentation describes the C level
    # coercion issues involved here. Furthermore, treating references as pointers
    # is "implementation-defined behavior" that depends on the C++ ABI.
    if cpp_type.kind in (TypeKind.POINTER, TypeKind.LVALUEREFERENCE, TypeKind.RVALUEREFERENCE):  # type: ignore
        if depth:
            throw_cursor(cursor, "type_error Only one pointer or reference allowed in a type.")
        pointee = cpp_type.get_pointee()
        pointee_ctype, pointee_py_type = calculate_python_api_type_map(cursor, pointee, symbols, is_return, 1)
        return (f'ctypes.POINTER({pointee_ctype})', pointee_py_type)

    # If it isn't a fundamental type then there has to be a separate definition available.
    declaration_cursor = cpp_type.get_declaration()
    if not declaration_cursor or not declaration_cursor.is_definition():
        throw_cursor(cursor, 'type_error Incomplete type: {cpp_type_ref.displayname}')

    if cpp_type.kind in (CursorKind.ENUM_DECL, CursorKind.STRUCT_DECL, CursorKind.CLASS_DECL): # type: ignore
        py_name = calculate_python_package_path(declaration_cursor)

        if not py_name in symbols:
            throw_cursor(declaration_cursor, f'type_error Unregistered definition: {py_name}')

        if declaration_cursor.kind == CursorKind.ENUM_DECL: # type: ignore
            # Just tell ctypes to marshal enums to their underlying type.
            return (_clang_to_ctypes[declaration_cursor.enum_type.kind], py_name)

        # Classes and structs are usable directly by ctypes. These need to be full path.
        return (py_name, py_name)

    throw_cursor(cursor, 'type_error Unsupported declaration type.')

def is_staticmethod(cursor: Cursor) -> bool:
    return cursor.kind is CursorKind.FUNCTION_DECL or ( # type: ignore
        cursor.kind is CursorKind.CXX_METHOD and cursor.storage_class == StorageClass.STATIC) # type: ignore

def emit_python_api_function(namespace_tabs: str, cursor: Cursor, symbols: Dict[str, List[Cursor]], overloaded: bool) -> List[str]:
    arg_types : List[Tuple[str, str]] = []
    arg_index = 0
    for arg in cursor.get_arguments():
        _, py_type = calculate_python_api_type_map(cursor, arg.type, symbols, False)
        c_name = arg.spelling if arg.spelling else f'_E_{arg_index}'
        arg_types.append((c_name, py_type))
        arg_index = arg_index + 1
    _, return_py_type = calculate_python_api_type_map(cursor, cursor.result_type, symbols, True)

    lines: List[str] = ['']

    static_method = is_staticmethod(cursor)
    if static_method:
        lines.append(namespace_tabs + '@staticmethod')
    if overloaded:
        lines.append(namespace_tabs + '@overload')

    # TODO
    function_name = get_bare_name(cursor)
    if cursor.kind is CursorKind.CONSTRUCTOR: # type: ignore
        function_name = '__init__'
    elif cursor.kind is CursorKind.DESTRUCTOR: # type: ignore
        function_name = '__del__'

    self_parameter = 'self,' if not static_method else ''
    arg_hints = ','.join(f"{c_name}:'{py_type}'" for c_name, py_type in arg_types)
    return_hint = f"->'{return_py_type}'"
    lines.append(f'{namespace_tabs}def {function_name}({self_parameter}{arg_hints}){return_hint}:')

    function_tabs : str = namespace_tabs + '\t'
    if overloaded:
        lines += emit_python_api_doc(function_tabs, cursor)
        lines.append(function_tabs + '...')
    else:
        self_parameter = 'ctypes.byref(self),' if not static_method else ''
        mangled_name = get_mangled_name(cursor)
        arg_names = ','.join(c_name for c_name, _ in arg_types)
        lines += emit_python_api_doc(function_tabs, cursor)
        lines.append(f"{namespace_tabs}\treturn {mangled_name}({self_parameter}{arg_names}) # {cursor.displayname}")

    return lines


def emit_python_api_overload_selector(namespace_tabs: str, overloads: List[Cursor], symbols: Dict[str, List[Cursor]]) -> List[str]:
    lines: List[str] = ['']
    static_method = False
    if any(is_staticmethod(cursor) for cursor in overloads):
        lines.append(namespace_tabs + "@staticmethod")
        static_method = True

    cursor0 = overloads[0]

    # TODO
    function_name = cursor0.spelling
    if cursor0.kind is CursorKind.CONSTRUCTOR: # type: ignore
        function_name = '__init__'
    elif cursor0.kind is CursorKind.DESTRUCTOR: # type: ignore
        function_name = '__del__'

    self_parameter = 'self,' if not static_method else ''
    lines += [  f'{namespace_tabs}def {function_name}({self_parameter}*args,**kwargs):',
                f"{namespace_tabs}\tassert not kwargs, 'keyword_arguments'",
                f'{namespace_tabs}\tmatch len(args):' ]

    # Group overloads by argument count
    arg_count_map: Dict[int, List[Cursor]] = {}
    for cursor in overloads:
        arg_count = len(list(cursor.get_arguments()))
        if arg_count not in arg_count_map:
            arg_count_map[arg_count] = []
        arg_count_map[arg_count].append(cursor)

    for arg_count, overload_group in sorted(arg_count_map.items()):
        if len(overload_group) > 1:
            # TODO Dispatch by argument count first and then by parameter type(s) second.
            for item in overload_group:
                print('error: ' + item.displayname)
            throw_cursor(overload_group[0], f"multiple_overloads {overload_group[0].spelling} args {arg_count}")

        assert overload_group
        cursor = overload_group[0]
        mangled_name = get_mangled_name(cursor)
        self_parameter = 'ctypes.byref(self), ' if not static_method else ''
        arg_list = ','.join(f'args[{i}]' for i in range(arg_count))
        lines += [
            f"{namespace_tabs}\t\tcase {arg_count}:",
            f"{namespace_tabs}\t\t\treturn {mangled_name}({self_parameter}{arg_list})"
        ]

    lines += [  f"{namespace_tabs}\t\tcase _:",
                f"{namespace_tabs}\t\t\tassert False, f'overload_resolution len {{len(args)}}'",
                f"{namespace_tabs}\t\t\tpass" ]
    return lines

def emit_python_api_enum(namespace_tabs: str, cursor: Cursor) -> List[str]:
    enum_name = get_bare_name(cursor)

    lines: List[str] = ['']
    if cursor.is_scoped_enum():
        lines.append(f'{namespace_tabs}class {enum_name}(enum.IntEnum):')
    else:
        lines.append(f'{namespace_tabs}class {enum_name}(enum.IntFlag):')

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
        throw_cursor(cursor, "Empty enums not allowed in Python: " + enum_name)

    # Put the named constants in the surrounding namespace.
    if not cursor.is_scoped_enum():
        for child in cursor.get_children():
            if child.kind == CursorKind.ENUM_CONSTANT_DECL: # type: ignore
                lines.append(f'{namespace_tabs}{child.spelling}={enum_name}.{child.spelling}')

    return lines

def emit_python_api_class(namespace_tabs: str, cursor: Cursor) -> List[str]:
    verbose(2, f"emit_python_api_class {get_bare_name(cursor)}:")

    lines: List[str] = [f'{namespace_tabs}class {get_bare_name(cursor)}(ctypes.Structure):']
    lines += emit_python_api_doc(namespace_tabs + '\t', cursor)
    return lines

def emit_python_api(symbols: Dict[str, List[Cursor]], sorted_symbols: List[List[Cursor]], api: List[str]) -> None:

    current_namespace : List[str] = []

    # The cursors have been sorted according to their python identifier. This
    # means their encapsulating namespaces (from namespaces, structs and classes)
    # will be visited in "collation order." This is used to produce push and pop
    # operations when the namespace stack changes. Those produce a Python class
    # hierarchy.
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

        # Cursors for namespaces are not being selected. Instead missing namespace
        # declarations are only being added as needed.
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
    for cursor_list in sorted_symbols:
        cursor0 = cursor_list[0]
        if cursor0.kind in (CursorKind.CLASS_DECL, CursorKind.STRUCT_DECL): # type: ignore
            assert len(cursor_list) == 1

            structure_list.append(f'{calculate_python_package_path(cursor0)}._fields_ = [')

            for child in cursor0.get_children():
                if child.kind == CursorKind.FIELD_DECL: # type: ignore
                    ctypes_type, _ = calculate_python_api_type_map(child, child.type, symbols, False)
                    structure_list.append(f'\t("{child.spelling}", {ctypes_type}),')

            structure_list.append("]")

def emit_symbol_table(symbols: Dict[str, List[Cursor]], sorted_symbols: List[List[Cursor]], symbol_table: List[str]) -> None:
    for cursor_list in sorted_symbols:
        if cursor_list[0].kind in ( CursorKind.FUNCTION_DECL, # type: ignore
                                    CursorKind.CONSTRUCTOR,   # type: ignore
                                    CursorKind.DESTRUCTOR,    # type: ignore
                                    CursorKind.CXX_METHOD):   # type: ignore
            for cursor in cursor_list:
                # Get types
                arg_types : List[str]= []
                for arg in cursor.get_arguments():
                    ctypes_type, _ = calculate_python_api_type_map(arg, arg.type, symbols, False)
                    arg_types.append(ctypes_type)
                return_type, _ = calculate_python_api_type_map(cursor, cursor.result_type, symbols, True)

                # format them
                mangled_name = get_mangled_name(cursor)
                comment = f' # {cursor.displayname}'
                self_ptr = 'ctypes.c_void_p,' if cursor.kind != CursorKind.FUNCTION_DECL else '' # type: ignore

                symbol_table += [
                    f"{mangled_name}=_E_cdll.{mangled_name}{comment}",
                    f"{mangled_name}.argtypes=[{self_ptr}{','.join(arg_types)}]",
                    f"{mangled_name}.restype={return_type}"
                ]

# Gather symbols by their python path. They will have to work together. This is
# where symbols get dropped due to the ODR rule.
def add_symbol(cursor: Cursor, symbols: Dict[str, List[Cursor]]) -> None:
    if not (cursor.is_anonymous()
            or not cursor.spelling.startswith('_')
            or not (len(cursor.spelling) > 1 and cursor.spelling[1].isupper())):
        throw_cursor(cursor, 'Leading underscores followed by capital letters reserved by implementation.')

    sym : str = calculate_python_package_path(cursor)
    if sym not in symbols:
        assert sym
        symbols[sym] = [ cursor ]
    elif not any(c.get_usr().encode() == cursor.get_usr().encode() for c in symbols[sym]):
        symbols[sym].append(cursor)

# This is the final output order for the python api. Symbols are sorted first by
# namespace, then by cursor kind and then by name, if any. The symbols object
# is built up using the Python path instead of exposing the sort key. This keeps
# the details of the sort local to this function.
def sort_symbols(symbols: Dict[str, List[Cursor]], sorted_symbols: List[List[Cursor]]) -> None:
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

def gather_symbols_of_interest(cursor: Cursor, symbols: Dict[str, List[Cursor]]) -> None:
    verbose(3, f'visiting {cursor.kind.name} {cursor.displayname}')

    if cursor.kind in (CursorKind.TRANSLATION_UNIT, CursorKind.NAMESPACE): # type: ignore
        pass # fallthrough

    elif not is_annotated_entanglement(cursor):
        # Ignore everything but ENTANGLEMENT_TYPE/ENTANGLEMENT_LINK.
        return

    elif (cursor.kind in (
            CursorKind.CLASS_TEMPLATE, # type: ignore
            CursorKind.FUNCTION_TEMPLATE, # type: ignore
            CursorKind.CLASS_TEMPLATE_PARTIAL_SPECIALIZATION # type: ignore
        )):
        throw_cursor(cursor, f'unsupported_api {cursor.displayname} Templates are not supported.')

    elif cursor.kind is CursorKind.ENUM_DECL: # type: ignore
        if cursor.is_definition() and cursor.access_specifier in (
                AccessSpecifier.PUBLIC, # type: ignore
                AccessSpecifier.INVALID): # type: ignore # Top-level enums
            add_symbol(cursor, symbols)
        else:
            throw_cursor(cursor, f'unsupported_api {get_bare_name(cursor)} Enum is incomplete or private.')

    elif cursor.kind is CursorKind.FUNCTION_DECL: # type: ignore
        if (cursor.linkage is LinkageKind.EXTERNAL # type: ignore
                and not cursor.type.is_function_variadic()
                and not is_arg_va_list(cursor)):
            add_symbol(cursor, symbols)
        else:
            throw_cursor(cursor, f'unsupported_api {cursor.displayname} Functions must be extern and not variadic.')

    elif cursor.kind in (CursorKind.CLASS_DECL, CursorKind.STRUCT_DECL): # type: ignore
        if (cursor.is_definition() and not cursor.is_anonymous() and cursor.access_specifier in (
                AccessSpecifier.PUBLIC, # type: ignore
                AccessSpecifier.INVALID)): # type: ignore # Top-level classes/classes
            add_symbol(cursor, symbols)
        else:
            throw_cursor(cursor, f'unsupported_api {get_bare_name(cursor)} Structs and classes must be complete, not be anonymous and be public.')

    elif cursor.kind in (CursorKind.CONSTRUCTOR, CursorKind.DESTRUCTOR, CursorKind.CXX_METHOD): # type: ignore
        if (cursor.access_specifier is AccessSpecifier.PUBLIC # type: ignore
                and cursor.linkage is LinkageKind.EXTERNAL # type: ignore
                and not cursor.type.is_function_variadic()
                and not is_arg_va_list(cursor)):
            add_symbol(cursor, symbols)
        else:
            throw_cursor(cursor, f'unsupported_api {cursor.displayname} Methods must be public, extern and not variadic.')

    else:
        throw_cursor(cursor, f'unsupported_api Unsupported {cursor.kind.name} {get_bare_name(cursor)}')

    # fallthrough to children.
    for c in cursor.get_children():
        gather_symbols_of_interest(c, symbols)

def load_translation_unit(header_file: str) -> TranslationUnit:
    index = Index.create()
    translation_unit: Optional[TranslationUnit] = None
    try:
        translation_unit = index.parse(header_file, _arg_compiler_flags,
            options=clang.cindex.TranslationUnit.PARSE_SKIP_FUNCTION_BODIES)
    except Exception as e:
        exception_debugger(e)

    if translation_unit:
        for diagnostic in translation_unit.diagnostics:
            print(diagnostic)

    if not translation_unit or any(
            d.severity >= Diagnostic.Error for d in translation_unit.diagnostics):
        verbose(1, 'exit_error')
        exit(_exit_error)

    return translation_unit

def main() -> int:
    """
    Main entry point for the script. Parses command-line arguments, initializes clang,
    and generates binding code for the given header file.
    """
    if not parse_argv():
        print("""\
Usage: python3 entanglement.py <compiler_flags> <lib_name> <header_files>... <output_file>

This tool parses a C++ header file using libclang and generates binding code.

Arguments:
    compiler_flags  - Flags to pass to clang. Can be in any order on command line.
    lib_name        - C/C++ library/.so name to bind everything to.
    header_files... - Path(s) to the C++ header file(s) to parse.
    output_file     - Path to write the generated Python binding code.
""")
        verbose(1, 'exit_error')
        exit(_exit_error)

    verbose(1, 'compiler_flags ' + ' '.join(_arg_compiler_flags))
    verbose(1, 'lib_name ' + _arg_lib_name)
    verbose(1, 'header_files ' + ' '.join(_arg_header_files))
    verbose(1, 'output_file ' + _arg_output_file)

    verbose(1, 'set_library_file ' +_libclang_path)
    Config.set_library_file(_libclang_path)

    # Merge and sort all the cursors of interest from all the translation units.
    # Using a Python class to implement namespaces requires merging the namespace
    # declarations. And Python's function overloading situationship requires
    # figuring out all of the function overloads in advance.
    symbols: Dict[str, List[Cursor]] = { }
    for header_file in _arg_header_files:
        verbose(1, f'load_translation_unit {header_file}...')
        translation_unit = load_translation_unit(header_file)
        gather_symbols_of_interest(translation_unit.cursor, symbols)

    if not symbols:
        print('exit_error No symbols found. Use -DENTANGLEMENT_PASS=1, ENTANGLEMENT_TYPE and ENTANGLEMENT_LINK.')
        exit(_exit_error)

    # Sort the symbols into their final order.
    sorted_symbols: List[List[Cursor]] = []
    sort_symbols(symbols, sorted_symbols)

    # Build the script sections
    python_api: List[str] = []
    structure_list: List[str] = []
    symbol_table: List[str] = []
    emit_python_api(symbols, sorted_symbols, python_api)
    emit_structure_list(symbols, sorted_symbols, structure_list)
    emit_symbol_table(symbols, sorted_symbols, symbol_table)

    # Assemble the script sections
    output_lines: List[str] = [
        f'""" {_arg_lib_name}',
        f'    {' '.join(sys.argv)}',
        f'    {datetime.now()} """',
        'import ctypes,enum,numpy,os,sys',
        'from typing import Any,overload',
        '',
        '# PYTHON_API',
        '',
        'def _E_array_to_ctypes(arr: Any, c_type) -> ctypes._Pointer:',
        "    assert isinstance(c_type, type), '_E_array_to_ctypes c_type not a type'",
        '    if isinstance(arr, ctypes.Array):',
        '        return arr',
        '    elif isinstance(arr, numpy.ndarray):',
        '        return arr.ctypes.data_as(ctypes.POINTER(c_type))',
        '    else:',
        "        raise ValueError('_E_array_to_ctypes Expected ctypes.Array or numpy.ndarray')",
        '',
    ] + python_api + [
        '',
        '# STRUCTURE_LIST',
        '',
    ] + structure_list + [
        '',
        '# SYMBOL_TABLE',
        '',
        f"_E_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), '{_arg_lib_name}')",
        'try:',
        '    _E_cdll = ctypes.CDLL(_E_path)',
        'except Exception as e:',
        f"    print(f'missing_library {{e}}', file=sys.stderr)",
        '    exit(1)',
    ] + symbol_table + [
        '',
        '# 🐉🐉🐉',
        '',
    ]

    # Write output
    with open(_arg_output_file, 'w') as f:
        f.write('\n'.join(output_lines) if output_lines else '')
        verbose(1, f'status Wrote {len(output_lines)} lines to {_arg_output_file}.')

    verbose(1, 'exit_ok')
    return _exit_bindings_generated

if __name__ == '__main__':
    exit(main())
