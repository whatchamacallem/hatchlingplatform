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


#    def is_abstract_record(self):
#    def is_pure_virtual_method(self): # throw in constructor?

#    def get_array_element_type(self):
#    def get_array_size(self):
#    def get_size(self): # assert(sizeof == get_size())
#    def get_fields(self): # classes
#    def is_bitfield(self):
#    def get_bitfield_width(self):




import sys
import os
import clang.cindex
import ctypes
from typing import Dict, List, Set, Tuple, Optional
from clang.cindex import Index, TranslationUnit, Diagnostic, Cursor, CursorKind
from clang.cindex import TypeKind, Type, LinkageKind, AccessSpecifier, Config

# Verbose - 0: Normal status and errors. 1: Processing steps. 2: AST traversal.
VERBOSE = 2

# Path to the libclang shared library. TODO throws exceptions.
_libclang_path = "/usr/lib/llvm-18/lib/libclang.so.1"

# Exit codes.
_exit_bindings_generated = 0
_exit_error = 1

# Processed command line arguments.
_arg_compiler_flags: List[str] = []
_arg_lib_name: str = ""
_arg_header_files: List[str] = []
_arg_output_file: str = ""
_arg_dependency_file: str = ""

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
    TypeKind.VOID:      'ctypes.c_void',       # type: ignore # void
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

_counter = 0

def verbose(x: str) -> None:
    if VERBOSE >= 1:
        print(f" * {x}")

def verbose2(x: str) -> None:
    if VERBOSE >= 2:
        print(f" - {x}")

def parse_argv() -> bool:
    global _arg_compiler_flags
    global _arg_lib_name
    global _arg_header_files
    global _arg_output_file
    global _arg_dependency_file

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

    _arg_dependency_file = _arg_output_file + '.d.txt'

    return True

# cindex may have extra exception data.
def exception_debugger(e: Exception) -> None:
    for attribute_name in dir(e):
        try:
            attribute_value = getattr(e, attribute_name)
            print(f"  {attribute_name}: {attribute_value}")
        except:
            pass

    print(e)
    sys.exit(_exit_error)

def throw_cursor(c: Cursor, message: str) -> None:
    raise ValueError(c.displayname() + ': ' + message + '\n' + c.location())

def get_counter() -> int:
    global _counter
    _counter += 1
    return _counter

def is_project_header(cursor: Cursor) -> bool:
    """
    Returns true if the symbol is in a file that was on the command line.
    """
    header = cursor.location.file
    return (
        header is not None
        and any(h in str(header) for h in _arg_header_files)
    )

def is_template(cursor: Cursor) -> bool:
    """
    Templates are skipped for binding. TODO: These nodes could be cached
    and then evaluated when an explicit instantiation was encountered.
    (Technically the instantiation of a template is not a template.)
    """
    return (
        cursor.kind in (
            CursorKind.CLASS_TEMPLATE, # type: ignore
            CursorKind.FUNCTION_TEMPLATE, # type: ignore
            CursorKind.CLASS_TEMPLATE_PARTIAL_SPECIALIZATION # type: ignore
        )
    )

def is_namespace(cursor: Cursor) -> bool:
    """
    Returns True if the cursor is a namespace definition.
    """
    return (
        cursor.kind is CursorKind.NAMESPACE  # type: ignore
        and cursor.is_definition()
    )

def is_public_enum(cursor: Cursor) -> bool:
    """
    Returns True if the cursor is a public enum definition.
    """
    return (
        cursor.kind is CursorKind.ENUM_DECL # type: ignore
        and cursor.is_definition()
        and cursor.access_specifier in (
            AccessSpecifier.PUBLIC, # type: ignore
            AccessSpecifier.INVALID  # type: ignore # Top-level enums
        )
    )

def is_arg_va_list(cursor: Cursor) -> bool:
    """
    Returns True if any argument to the function uses va_list (variadic argument list).
    Such functions are skipped for binding.
    """
    return any("va_list" in a.type.spelling for a in cursor.get_arguments())

def is_public_function(cursor: Cursor) -> bool:
    """
    Returns True if the cursor is a public, non-variadic, non-template, project function.
    """
    return (
        cursor.kind is CursorKind.FUNCTION_DECL # type: ignore
        and cursor.linkage is LinkageKind.EXTERNAL # type: ignore
        and not cursor.type.is_function_variadic()
        and not is_arg_va_list(cursor)
    )

def is_public_class(cursor: Cursor) -> bool:
    """
    Returns True if the cursor is a public class or struct definition.
    """
    return (
        cursor.kind in (CursorKind.CLASS_DECL, CursorKind.STRUCT_DECL)  # type: ignore
        and cursor.is_definition()
        and cursor.access_specifier in (
            AccessSpecifier.PUBLIC,  # type: ignore
            AccessSpecifier.INVALID   # type: ignore # Top-level classes/classes
        )
    )

def is_public_constructor(cursor: Cursor) -> bool:
    """
    Returns True if the cursor is a public constructor.
    """
    return (
        cursor.kind is CursorKind.CONSTRUCTOR # type: ignore
        and cursor.access_specifier is AccessSpecifier.PUBLIC # type: ignore
    )

def is_pure_virtual_method(cursor: Cursor) -> bool:
    """
    Returns True if the cursor is a pure virtual method (declared with = 0).
    """
    return (
        cursor.kind is CursorKind.CXX_METHOD  # type: ignore
        and cursor.is_pure_virtual_method()  # Check for pure virtual
    )

def is_static_method(cursor: Cursor) -> bool:
    """
    Returns True if the cursor is a static method.
    """
    return (
        cursor.kind is CursorKind.CXX_METHOD # type: ignore
        and cursor.is_static_method()
    )

def is_public_method(cursor: Cursor) -> bool:
    """
    Returns True if the cursor is a public, non-variadic, non-template method.
    """
    return (
        cursor.kind is CursorKind.CXX_METHOD # type: ignore
        and cursor.access_specifier is AccessSpecifier.PUBLIC # type: ignore
        and not cursor.type.is_function_variadic()
        and not is_arg_va_list(cursor)
    )

def get_mangled_name(cursor: Cursor) -> str:
    return cursor.mangled_name or cursor.spelling

def type_map(cpp_type: Type, classes: Dict[str, str], enums: Dict[str, str]) -> Tuple[str, str]:
    """ clang type -> (ctype, python, C)"""
    # Handle typedefs by resolving to the canonical type
    while cpp_type.kind == TypeKind.TYPEDEF:  # type: ignore
        cpp_type = cpp_type.get_canonical()

    # Fundamental types
    if cpp_type.kind in _clang_to_ctypes:
        return (_clang_to_ctypes[cpp_type.kind], _clang_to_python[cpp_type.kind])

    # Pointers and references. Treating references as pointers is "implementation-defined
    # behavior" that depends on the C++ ABIs. If it doesn't work then it is a language
    # extension provided by this tool that should be unsupported on your platform.
    if cpp_type.kind in (TypeKind.POINTER, TypeKind.LVALUEREFERENCE, TypeKind.RVALUEREFERENCE):  # type: ignore
        pointee = cpp_type.get_pointee()
        pointee_ctype, pointee_py_type = type_map(pointee, classes, enums)
        return (f"ctypes.POINTER({pointee_ctype})", pointee_py_type)

    # Structs, classes and enums.
    spelling = cpp_type.spelling
    if spelling in classes:
        return (spelling, spelling)  # Use class name as ctype and Python type.
    if spelling in enums:
        return (enums[spelling], spelling)  # Use ctype and enum name as Python type.

    raise ValueError(f"Unrecognized C++ type '{spelling}' may cause memory corruption")

def calculate_namespace_prefix(cursor: Cursor) -> str:
    """
    Returns the fully qualified namespace path for a cursor (e.g., 'outer::inner').
    Traverses semantic parents to build the namespace chain.
    """
    namespaces: List[str] = []
    current: Optional[Cursor] = cursor.semantic_parent
    while current and current.kind is CursorKind.NAMESPACE: # type: ignore
        namespaces.append(current.spelling)
        current = current.semantic_parent
    if namespaces:
        namespaces.append("")
    return "::".join(reversed(namespaces))

def format_doc(cursor: Cursor) -> List[str]:
    """
    Extracts and formats the raw comment for use as a docstring.
    Strips comment markers and leading/trailing whitespace.
    Escapes characters (\\n, \\", \\) for safe embedding in a Python string.
    """
    comment = cursor.raw_comment
    if comment:
        lines: List[str] = comment.splitlines()
        cleaned: List[str] = []
        for line in lines:
            line = line.strip()
            if line.startswith("///"):
                line = line[3:].strip()
            elif line.startswith("//"):
                line = line[2:].strip()
            line = line.replace('\\', '\\\\').replace('"', '\\"')
            cleaned.append(line)
        doc = "\n\t".join(cleaned) if cleaned else ""
        return [f'\t"""{doc}"""' if doc else ""]
    return [ ]

def format_enum(cursor: Cursor, enums: Dict[str, str]) -> List[str]:
    """Formats the enum binding code with type hints and constants."""
    enum_name = cursor.spelling or f"__enum_{get_counter()}"
    lines: List[str] = ['']

    # TODO pre-pass. Only built in types are valid here.
    enums[enum_name] = _clang_to_ctypes[cursor.enum_type.kind]

    if cursor.is_scoped_enum():
        lines.append(f"class {enum_name}(enum.Enum):")
    else:
        lines.append(f"class {enum_name}(enum.IntFlag):")

    lines += format_doc(cursor)

    is_pass = True
    for child in cursor.get_children():
        if child.kind == CursorKind.ENUM_CONSTANT_DECL: # type: ignore
            lines.append(f"\t{child.spelling}={child.enum_value}")
            is_pass = False

    if is_pass:
        lines.append("\tpass")

    # Put the named constants in the surrounding namespace.
    if not cursor.is_scoped_enum():
        for child in cursor.get_children():
            if child.kind == CursorKind.ENUM_CONSTANT_DECL: # type: ignore
                lines.append(f"{child.spelling}={enum_name}.{child.spelling}")

    return lines

def format_function(cursor: Cursor, classes: Dict[str, str], enums: Dict[str, str], overloaded: bool) -> List[str]:
    """
    Generates binding lines for a free function.
    Includes docstring and overload decorator if needed.
    """
    mangled_name = get_mangled_name(cursor)
    arg_types = []
    for arg in cursor.get_arguments():
        ctypes_type, py_type = type_map(arg.type, classes, enums)
        arg_types.append((ctypes_type, py_type))
    return_type, return_py_type = type_map(cursor.result_type, classes, enums)

    lines: List[str] = ['']

    if overloaded:
        lines.append(f"@overload")

    arg_hints = ', '.join(f'arg{i}: {py_type}' for i, (_, py_type) in enumerate(arg_types)) if arg_types else ''
    return_hint = f' -> {return_py_type}' if return_py_type != 'None' else ''
    lines.append(f"def {cursor.spelling}({arg_hints}){return_hint}:")

    if overloaded:
        lines[-1] += ' ...'
    else:
        lines += format_doc(cursor)
        lines.append(f"\treturn __clib.{mangled_name}({', '.join(f'arg{i}' for i in range(len(arg_types)))})")

    if arg_types:
        args = [arg[0] for arg in arg_types]
        lines.append(f"__clib.{mangled_name}.argtypes = [{', '.join(args)}]")
    lines.append(f"__clib.{mangled_name}.restype = {return_type}")

    return lines

def format_method(cursor: Cursor, classes: Dict[str, str], enums: Dict[str, str], overload_index: int = 0) -> List[str]:
    """
    Generates binding lines for a class method.
    Includes overload, classmethod, staticmethod, or abstractmethod decorators if needed.
    """
    mangled_name = get_mangled_name(cursor)
    arg_types = []
    for arg in cursor.get_arguments():
        ctypes_type, py_type = type_map(arg.type, classes, enums)
        arg_types.append((ctypes_type, py_type))
    return_type, return_py_type = type_map(cursor.result_type, classes, enums)

    lines: List[str] = ['']

    if arg_types:
        args = [arg[0] for arg in arg_types]
        lines.append(f"__clib.{mangled_name}.argtypes = [{', '.join(args)}]")
    lines.append(f"__clib.{mangled_name}.restype = {return_type}")

    if overload_index > 0:
        lines.append(f"@overload")
    if is_public_method(cursor):
        lines.append("@classmethod")
    elif is_static_method(cursor):
        lines.append("@staticmethod")

    arg_hints = ', '.join(f'arg{i}: {py_type}' for i, (_, py_type) in enumerate(arg_types)) if arg_types else ''
    return_hint = f' -> {return_py_type}' if return_py_type != 'None' else ''
    lines.append(f"def {cursor.spelling}({arg_hints}){return_hint}:")
    lines += format_doc(cursor)
    lines.append(f"\treturn __clib.{mangled_name}({', '.join(f'arg{i}' for i in range(len(arg_types)))})")
    return lines

def format_constructor(cursor: Cursor, classes: Dict[str, str], enums: Dict[str, str], overload_index: int = 0) -> List[str]:
    """
    Generates binding lines for a class constructor.
    Includes overload decorator if needed.
    """
    mangled_name = get_mangled_name(cursor)
    arg_types = []
    for arg in cursor.get_arguments():
        ctypes_type, py_type = type_map(arg.type, classes, enums)
        arg_types.append((ctypes_type, py_type))

    lines: List[str] = ['']
    lines.append(f"__clib.{mangled_name}.restype = None")
    if arg_types:
        args = [arg[0] for arg in arg_types]
        lines.append(f"__clib.{mangled_name}.argtypes = [{', '.join(args)}]")
    if overload_index > 0:
        lines.append(f"@overload")

    arg_hints = ', '.join(f'arg{i}: {py_type}' for i, (_, py_type) in enumerate(arg_types)) if arg_types else ''
    lines.append(f"def __init__(self, {arg_hints}) -> None:")
    lines += format_doc(cursor)
    lines.append(f"\t__clib.{mangled_name}({', '.join(f'arg{i}' for i in range(len(arg_types)))})")
    return lines

def generate_overload_selector(name: str, overloads: List[Cursor]) -> List[str]:
    """
    Generates a wrapper function to select between overloads based on argument count.
    Raises compile-time errors for ambiguous or missing overloads.
    TODO No support for overloading static and non-static members together.
    """
    # Group overloads by argument count
    arg_count_map: Dict[int, List[Cursor]] = {}
    for cursor in overloads:
        arg_count = len(list(cursor.get_arguments()))
        if arg_count not in arg_count_map:
            arg_count_map[arg_count] = []
        arg_count_map[arg_count].append(cursor)

    lines: List[str] = ['']
    if any(is_public_method(cursor) for cursor in overloads):
        lines.append("@classmethod")
    elif any(is_static_method(cursor) for cursor in overloads):
        lines.append("@staticmethod")

    lines.append(f"def {name}(*args: Any, **kwargs: Any) -> Any:")
    lines.append('\tmatch len(args):')

    # TODO Dispatch by argument count first and then by first parameter type second.
    for arg_count, overload_group in sorted(arg_count_map.items()):
        if not overload_group:
            continue
        if len(overload_group) > 1:
            raise ValueError(f"Multiple overloads for {name} with {arg_count} arguments: cannot disambiguate")
        cursor = overload_group[0]
        mangled_name = get_mangled_name(cursor)
        arg_list = ', '.join(f'args[{i}]' for i in range(arg_count))
        lines.append(f"\t\tcase {arg_count}:")
        lines.append(f"\t\t\treturn __clib.{mangled_name}({arg_list})")

    lines.append(f"\t\tcase _:")
    lines.append(f"\t\t\traise ValueError(f'overload_resolution {name} with {{len(args)}} arguments')")
    return lines

def format_class(cursor: Cursor, classes: Dict[str, str], enums: Dict[str, str], seen_signatures: Set[str]) -> List[str]:
    """
    Generates binding code for a class and registers it in classes.
    Skips constructor bindings for abstract classes and those without public destructors.
    """
    class_name = cursor.spelling
    verbose2(f"class {class_name}:")

    # TODO ctypes doesn't support .vtables directly. One solution would be to
    # use undefined behavior to force it to do so.
    has_pure_virtual = any(is_pure_virtual_method(child) for child in cursor.get_children())
    if has_pure_virtual:
        return []

    lines: List[str] = ['']
    lines.append(f"class {class_name}(ctypes.Structure):")
    lines += format_doc(cursor)
    fields = []
    for child in cursor.get_children():
        if child.kind == CursorKind.FIELD_DECL: # type: ignore
            ctypes_type, _ = type_map(child.type, classes, enums)
            fields.append(f'\t\t("{child.spelling}", {ctypes_type}),')
    if fields:
        lines.append("\t_fields_ = [")
        lines += fields
        lines.append("\t]")
    else:
        lines.append(f"\tpass")

    # Collect constructors and methods for overload handling
    constructors = []
    methods: Dict[str, List[Cursor]] = {}
    for child in cursor.get_children():
        if is_template(child):
            verbose2(f"skipping template {child.spelling}")
            return []
        elif is_public_enum(child):
            enum_lines = format_enum(child, enums)
            lines.extend(enum_lines)
        elif is_public_constructor(child):
            constructors.append(child)
        elif is_public_method(child):
            method_name = child.spelling
            if method_name not in methods:
                methods[method_name] = []
            methods[method_name].append(child)

    constructor_overloads = []
    for i, child in enumerate(constructors):
        sig = get_mangled_name(child)
        if sig not in seen_signatures:
            constructor_lines = format_constructor(child, classes, enums, i)
            lines.extend(constructor_lines)
            constructor_overloads.append(child)
            seen_signatures.add(sig)
    if constructor_overloads:
        lines.extend(generate_overload_selector('__init__', constructor_overloads))

    # Generate method bindings with overloads
    for method_name, method_list in methods.items():
        method_overloads = []
        for i, child in enumerate(method_list):
            sig = get_mangled_name(child)
            if sig not in seen_signatures:
                method_lines = format_method(child, classes, enums, i)
                lines.extend(method_lines)
                method_overloads.append(child)
                seen_signatures.add(sig)
        if method_overloads:
            lines.extend(generate_overload_selector(method_name, method_overloads))

    # TODO. Register the class in classes
    classes[class_name] = class_name  # Map to the ctypes.Structure subclass name

    return lines

def format_namespace(cursor: Cursor, classes: Dict[str, str], enums: Dict[str, str], seen_functions: Set[str]) -> List[str]:
    """
    Recursively visits AST nodes starting from the given cursor.
    Collects functions, classes, and enums that are public and project-specific.
    """

    lines: List[str] = []
    function_overloads: Dict[str, List[Cursor]] = {}
    namespace = calculate_namespace_prefix(cursor)

    for c in cursor.get_children():
        try:
            if not c.spelling or not is_project_header(c):
                continue
            if is_namespace(c):
                verbose2(f"namespace_enter {c.spelling}")
                lines.extend(format_namespace(c, classes, enums, seen_functions))
                verbose2(f"namespace_exit {c.spelling}")
            elif is_public_function(c):
                fn = get_mangled_name(c)
                if fn not in seen_functions:
                    func_name = c.spelling
                    if func_name not in function_overloads:
                        function_overloads[func_name] = []
                    function_overloads[func_name].append(c)
                    seen_functions.add(fn)
            elif is_public_class(c):
                class_lines = format_class(c, classes, enums, seen_functions)
                if class_lines:
                    lines.extend(class_lines)
            elif is_public_enum(c):
                lines.extend(format_enum(c, enums))
            else:
                verbose2(f"skipped {c.spelling}")
        except Exception as e:
            print(f"Unexpected error processing cursor {c.spelling}: {str(e)}")
            continue

    # Generate function overload selectors
    for func_name, overloads in function_overloads.items():
        if len(overloads) > 1:
            for cursor in overloads:
                lines.extend(format_function(cursor, classes, enums, True))
            lines.extend(generate_overload_selector(func_name, overloads))
        else:
            lines.extend(format_function(overloads[0], classes, enums, False))


    return lines





# The fields of an anonymous enum are available from their parent. As such the
# path can be the parents path. Anonymous namespaces must also be retained as
# they may contain implementation details like base class layouts that are
# needed by the bindings code.
def format_python_package_path(cursor: Cursor) -> str:
    """ Generate the Python package path for the symbol. """
    namespaces: List[str] = []
    current: Optional[Cursor] = cursor
    while current and current.kind is CursorKind.NAMESPACE: # type: ignore
        if current.spelling:
            namespaces.append(current.spelling)
        current = current.semantic_parent
    return '.'.join(reversed(namespaces))

def print_symbols(symbols: Dict[str, List[Cursor]]) -> None:
    for _, cursor_list in symbols.items():
        for cursor in cursor_list:
            print(f"{cursor.displayname} {len(cursor_list)}")
            break

def add_symbol(cursor: Cursor, symbols: Dict[str, List[Cursor]]) -> None:
    """ Add the symbol to the symbol table. """
    usr : str = cursor.get_usr()
    if usr not in symbols:
        assert usr
        symbols[usr] = [ cursor ]
    else:
        symbols[usr].append(cursor)
    verbose(f'added {cursor.displayname}')

def gather_symbols_of_interest(cursor: Cursor, symbols: Dict[str, List[Cursor]]) -> None:
    """
    Traverse nodes of interest accumulating cursors by namespace qualified name.
    Quality can be checked beyond simple examination once they are gathered.
    """

#    verbose2(f'visiting {cursor.kind.name} {cursor.displayname}')

    if cursor.kind in (CursorKind.TRANSLATION_UNIT, CursorKind.NAMESPACE): # type: ignore
        pass # fallthrough

    # TODO
    elif not is_project_header(cursor):
        return

    elif (cursor.kind in (
            CursorKind.CLASS_TEMPLATE, # type: ignore
            CursorKind.FUNCTION_TEMPLATE, # type: ignore
            CursorKind.CLASS_TEMPLATE_PARTIAL_SPECIALIZATION # type: ignore
        )):
        verbose2(f'skipping template {cursor.displayname}')
        return

    elif cursor.kind is CursorKind.ENUM_DECL: # type: ignore
        if cursor.is_definition() and cursor.access_specifier in (
                AccessSpecifier.PUBLIC, # type: ignore
                AccessSpecifier.INVALID): # type: ignore # Top-level enums
            add_symbol(cursor, symbols)
        else:
            verbose2(f'skipping enum {cursor.displayname}')
        return

    elif cursor.kind is CursorKind.FUNCTION_DECL: # type: ignore
        if (cursor.linkage is LinkageKind.EXTERNAL # type: ignore
                and not cursor.type.is_function_variadic()
                and not is_arg_va_list(cursor)):
            add_symbol(cursor, symbols)
        else:
            verbose2(f'skipping function {cursor.displayname}')
        return

    elif cursor.kind in (CursorKind.CLASS_DECL, CursorKind.STRUCT_DECL): # type: ignore
        if (cursor.is_definition() and cursor.access_specifier in (
                AccessSpecifier.PUBLIC, # type: ignore
                AccessSpecifier.INVALID)): # type: ignore # Top-level classes/classes
            add_symbol(cursor, symbols)
        else:
            verbose2(f'skipping class {cursor.displayname}')
            return
        # fallthrough

    elif cursor.kind in (CursorKind.CONSTRUCTOR, CursorKind.DESTRUCTOR, CursorKind.CXX_METHOD): # type: ignore
        if (cursor.access_specifier is AccessSpecifier.PUBLIC # type: ignore
                and not cursor.type.is_function_variadic()
                and not is_arg_va_list(cursor)):
            add_symbol(cursor, symbols)
        else:
            verbose2(f'skipping class method {cursor.displayname}')
        return

    else:
        verbose2(f'not gathering {cursor.kind.name} {cursor.displayname}')
        return

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

    for diagnostic in translation_unit.diagnostics: # type: ignore
        print(diagnostic)

    if not translation_unit or (len(translation_unit.diagnostics) > 0
            and any(d.severity >= Diagnostic.Error for d in translation_unit.diagnostics)):
        print("Parsing failed due to errors.")
        sys.exit(_exit_error)

    return translation_unit

def main() -> int:
    """
    Main entry point for the script. Parses command-line arguments, initializes Clang,
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
        sys.exit(_exit_error)

    verbose("compiler_flags: " + ' '.join(_arg_compiler_flags))
    verbose("lib_name: " + _arg_lib_name)
    verbose("header_files: " + ' '.join(_arg_header_files))
    verbose("output_file: " + _arg_output_file)
    verbose("dependency_file: " + _arg_dependency_file)

    verbose(f"Setting libclang path: {_libclang_path}")
    Config.set_library_file(_libclang_path)

    output_lines: List[str] = [
        f'""" Generated from {', '.join(_arg_header_files)}."""',
        'import ctypes, enum, os',
        'from typing import Any, overload',
        '',
        f"__clib = ctypes.CDLL(os.path.join(os.getcwd(),'{_arg_lib_name}'))",
        ''
    ]

    symbols: Dict[str, List[Cursor]] = { }
    for header_file in _arg_header_files:
        verbose(f"Gathering symbols from {header_file}...")
        translation_unit = load_translation_unit(header_file)
        gather_symbols_of_interest(translation_unit.cursor, symbols)

    print_symbols(symbols)

    exit(0)


    classes: Dict[str, str] = {} # check for declaration of classes and structs
    enums: Dict[str, str] = {} # check for declaration of enums
    seen_functions: Set[str] = set() # check for duplicates, not overloads

    for header_file in _arg_header_files:
        verbose(f"Visiting AST and generating bindings from {header_file}...")
        translation_unit = load_translation_unit(header_file)
        child_lines: List[str] = format_namespace(translation_unit.cursor, classes, enums, seen_functions)
        output_lines.extend(child_lines)
        verbose2(f"Added {len(child_lines)} lines from {header_file}")

    output_lines += ['if __name__ == "__main__":', '\tprint("🐉🐉🐉")', '']

    with open(_arg_output_file, "w") as f:
        f.write("\n".join(output_lines) if output_lines else "")
        print(f"Wrote {_arg_output_file}: {len(output_lines)} lines")

    verbose2("exit 0")
    return _exit_bindings_generated

if __name__ == "__main__":
    sys.exit(main())
