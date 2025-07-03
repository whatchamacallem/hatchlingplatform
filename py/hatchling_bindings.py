"""
hatchling_bindings.py: Automatic Binding Generator for C++ Projects

This script automates the generation of Python bindings for C++ code using ctypes and libclang.
It parses a given C++ header file, discovers public functions, classes, methods, constructors,
destructors, and enums defined in a project, and then emits Python code that exposes these symbols
to Python via ctypes.

Key Features:
- Filters only project-specific public symbols (functions, classes, methods, enums) for binding.
- Skips internal headers, variadic functions, and templates to avoid ambiguous or unsafe bindings.
- Extracts and formats C++ documentation comments as Python docstrings for each binding.
- Handles class constructors, destructors, and methods, including overloaded signatures.
- Emits ctypes code for enums, including nested enums within classes.

Usage:
    python hatchling_bindings.py <compiler_flags> <module_name> <header_files>... <output_file>
        compiler_flags  - Flags to pass to clang. Can be in any order on command line.
        module_name     - Module name to bind everything to.
        header_files... - Path(s) to the C++ header file(s) to parse.
        output_file     - Path to write the generated Python binding code.

Adjust the _libclang_path at the top of the script as needed for your environment and project.
"""

import sys
import os
import clang.cindex
import ctypes
from typing import Dict, List, Set, Tuple, Optional, Any
from clang.cindex import Index, TranslationUnit, Diagnostic, Cursor, CursorKind
from clang.cindex import TypeKind, Type, LinkageKind, Config

# Path to the libclang shared library. TODO.
_libclang_path = "/usr/lib/llvm-18/lib/libclang.so.1"

# Mapping C++ types to ctypes and Python type hints
TYPE_MAP = {
    'void': (ctypes.c_void_p, 'None'),
    'bool': (ctypes.c_bool, 'bool'),
    'char': (ctypes.c_char, 'int'),
    'unsigned char': (ctypes.c_ubyte, 'int'),
    'short': (ctypes.c_short, 'int'),
    'unsigned short': (ctypes.c_ushort, 'int'),
    'int': (ctypes.c_int, 'int'),
    'unsigned int': (ctypes.c_uint, 'int'),
    'long': (ctypes.c_long, 'int'),
    'unsigned long': (ctypes.c_ulong, 'int'),
    'long long': (ctypes.c_longlong, 'int'),
    'unsigned long long': (ctypes.c_ulonglong, 'int'),
    'float': (ctypes.c_float, 'float'),
    'double': (ctypes.c_double, 'float'),
    'char*': (ctypes.c_char_p, 'bytes'),
    'const char*': (ctypes.c_char_p, 'bytes'),
    'void*': (ctypes.c_void_p, 'Any'),
    'int*': (ctypes.POINTER(ctypes.c_int), 'Any'),
    'float*': (ctypes.POINTER(ctypes.c_float), 'Any'),
    'double*': (ctypes.POINTER(ctypes.c_double), 'Any'),
}

# Verbose 0: Normal status and errors. 1: Processing steps. 2: AST traversal.
_verbose = 2

# Exit codes.
_exit_bindings_generated = 0
_exit_error = 1
_exit_nothing_changed = 2

# Processed command line arguments.
_arg_compiler_flags: List[str] = []
_arg_module_name: str = ""
_arg_header_files: List[str] = []
_arg_output_file: str = ""
_arg_dependency_file: str = ""

def verbose(msg: str) -> None:
    if _verbose >= 1:
        print(f" * {msg}")

def verbose2(msg: str) -> None:
    if _verbose >= 2:
        print(f" - {msg}")

def parse_argv() -> bool:
    global _arg_compiler_flags
    global _arg_module_name
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

    # the first non-switch was the module_name.
    _arg_module_name = non_flags.pop(0)

    # the last non-switch was the output file.
    _arg_output_file = non_flags.pop()

    # the rest are headers.
    _arg_header_files = non_flags

    _arg_dependency_file = _arg_output_file + '.d.txt'

    return True

# TODO: make more flexible and efficient.
def is_project_header(cursor: Cursor) -> bool:
    """
    Returns True if the cursor's location is in a project-specific header file.
    Adjust the string checks as needed for your project's include structure.
    """
    header = cursor.location.file
    header_str = str(header) if header is not None else ""
    return (
        header is not None
        and any(h in header_str for h in _arg_header_files)
    )

def is_template(cursor: Cursor) -> bool:
    """
    Returns True if the cursor is a template (class or function).
    Templates are skipped for binding.
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
    Returns True if the cursor is a public class definition.
    """
    return (
        cursor.kind is CursorKind.CLASS_DECL # type: ignore
        and cursor.is_definition()
        and cursor.access_specifier in (
            AccessSpecifier.PUBLIC, # type: ignore
            AccessSpecifier.INVALID  # type: ignore # Top-level classes
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
    """
    Get mangled name of a function or constructor.
    """
    return cursor.mangled_name or cursor.spelling

def format_doc(cursor: Cursor) -> str:
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
        doc = "\n".join(cleaned) if cleaned else ""
        return f'"""{doc}"""' if doc else ""
    return ""

def format_enum(cursor: Cursor) -> List[str]:
    """
    Generates binding code for an enum.
    Includes all enum constants as values.
    """
    enum_name = cursor.spelling or f"enum_{cursor.hash % 1000000}"
    lines: List[str] = [f"{enum_name} = ctypes.c_int"]
    doc_str = format_doc(cursor)
    if doc_str:
        lines.insert(0, doc_str)
    verbose2(f"enum {enum_name}: {lines}")
    return lines

def map_type(cpp_type: Type, structs: Dict[str, str], enums: Dict[str, str]) -> Tuple[Any, str]:
    """
    Maps C++ types to ctypes types and Python type hints.
    Handles basic types, pointers, structs, and enums.
    """
    spelling = cpp_type.spelling
    if spelling in TYPE_MAP:
        return TYPE_MAP[spelling]
    elif spelling in structs:
        return (structs[spelling], 'Any')
    elif spelling in enums:
        return (ctypes.c_int, 'int')
    elif cpp_type.kind == TypeKind.POINTER: # type: ignore
        pointee = cpp_type.get_pointee()
        base_type, _ = map_type(pointee, structs, enums)
        return (ctypes.POINTER(base_type), 'Any')
    raise ValueError(f"Unrecognized C++ type '{spelling}' may cause memory corruption")

def format_function(cursor: Cursor, structs: Dict[str, str], enums: Dict[str, str], overload_index: int = 0) -> List[str]:
    """
    Generates binding lines for a free function.
    Includes docstring and overload decorator if needed.
    """
    mangled_name = get_mangled_name(cursor)
    doc_str = format_doc(cursor)
    arg_types = []
    for arg in cursor.get_arguments():
        ctypes_type, py_type = map_type(arg.type, structs, enums)
        arg_types.append((ctypes_type, py_type))
    return_type, return_py_type = map_type(cursor.result_type, structs, enums)

    lines: List[str] = []
    if doc_str:
        lines.append(doc_str)
    if overload_index > 0:
        lines.append(f"@overload")
    arg_hints = ', '.join(f'arg{i}: {py_type}' for i, (_, py_type) in enumerate(arg_types)) if arg_types else ''
    return_hint = f' -> {return_py_type}' if return_py_type != 'None' else ''
    lines.append(f"def {cursor.spelling}({arg_hints}){return_hint}: ...")
    lines.append(f"    lib.{mangled_name}.restype = {return_type.__name__}")
    if arg_types:
        args = [arg[0].__name__ for arg in arg_types]
        lines.append(f"lib.{mangled_name}.argtypes = [{', '.join(args)}]")
    verbose2(f"function {cursor.spelling}: {lines}")
    return lines

def format_method(cursor: Cursor, structs: Dict[str, str], enums: Dict[str, str], overload_index: int = 0) -> List[str]:
    """
    Generates binding lines for a class method.
    Includes overload decorator if needed.
    """
    mangled_name = get_mangled_name(cursor)
    doc_str = format_doc(cursor)
    arg_types = []
    for arg in cursor.get_arguments():
        ctypes_type, py_type = map_type(arg.type, structs, enums)
        arg_types.append((ctypes_type, py_type))
    return_type, return_py_type = map_type(cursor.result_type, structs, enums)

    lines: List[str] = []
    if doc_str:
        lines.append(doc_str)
    if overload_index > 0:
        lines.append(f"    @overload")
    arg_hints = ', '.join(f'arg{i}: {py_type}' for i, (_, py_type) in enumerate(arg_types)) if arg_types else ''
    return_hint = f' -> {return_py_type}' if return_py_type != 'None' else ''
    lines.append(f"    def {cursor.spelling}({arg_hints}){return_hint}: ...")
    lines.append(f"        lib.{mangled_name}.restype = {return_type.__name__}")
    if arg_types:
        args = [arg[0].__name__ for arg in arg_types]
        lines.append(f"        lib.{mangled_name}.argtypes = [{', '.join(args)}]")
    verbose2(f"method {cursor.spelling}: {lines}")
    return lines

def format_constructor(cursor: Cursor, structs: Dict[str, str], enums: Dict[str, str], overload_index: int = 0) -> List[str]:
    """
    Generates binding lines for a class constructor.
    Includes overload decorator if needed.
    """
    mangled_name = get_mangled_name(cursor)
    doc_str = format_doc(cursor)
    arg_types = []
    for arg in cursor.get_arguments():
        ctypes_type, py_type = map_type(arg.type, structs, enums)
        arg_types.append((ctypes_type, py_type))

    lines: List[str] = []
    if doc_str:
        lines.append(f"    {doc_str}")
    if overload_index > 0:
        lines.append(f"    @overload")
    arg_hints = ', '.join(f'arg{i}: {py_type}' for i, (_, py_type) in enumerate(arg_types)) if arg_types else ''
    lines.append(f"    def __init__({arg_hints}) -> None: ...")
    lines.append(f"        lib.{mangled_name}.restype = None")
    if arg_types:
        args = [arg[0].__name__ for arg in arg_types]
        lines.append(f"        lib.{mangled_name}.argtypes = [{', '.join(args)}]")
    verbose2(f"constructor {cursor.spelling}: {lines}")
    return lines

def generate_overload_selector(name: str, overloads: List[Cursor], is_method: bool) -> List[str]:
    """
    Generates a wrapper function to select between overloads based on argument count.
    Raises compile-time errors for ambiguous or missing overloads.
    """
    # Group overloads by argument count
    arg_count_map: Dict[int, List[Cursor]] = {}
    for cursor in overloads:
        arg_count = len(list(cursor.get_arguments())) - (1 if is_method else 0)
        if arg_count not in arg_count_map:
            arg_count_map[arg_count] = []
        arg_count_map[arg_count].append(cursor)

    lines: List[str] = []
    lines.append(f"def {name}(" + ("self, " if is_method else "") + "*args, **kwargs) -> Any:")
    lines.append(f"    # Selects the appropriate overload for {name}")

    # Check for each possible argument count
    for arg_count, overload_group in sorted(arg_count_map.items()):
        if not overload_group:
            continue
        lines.append(f"    if len(args) == {arg_count}:")
        if len(overload_group) > 1:
            raise ValueError(f"Multiple overloads for {name} with {arg_count} arguments: cannot disambiguate")
        cursor = overload_group[0]
        mangled_name = get_mangled_name(cursor)
        arg_list = ', '.join(f'arg{i}' for i in range(arg_count))
        if is_method:
            lines.append(f"        return lib.{mangled_name}(self{', ' + arg_list if arg_list else ''})")
        else:
            lines.append(f"        return lib.{mangled_name}({arg_list})")

    lines.append(f"    raise ValueError('No matching overload for {name}" + " with {len(args)} arguments')")
    verbose2(f"selector {name}: {lines}")
    return lines

def format_class(cursor: Cursor, structs: Dict[str, str], enums: Dict[str, str], seen_signatures: Set[str]) -> List[str]:
    """
    Generates binding code for a class.
    Skips constructor bindings for abstract classes and those without public destructors.
    """
    class_name = cursor.spelling or f"class_{cursor.hash % 1000000}"
    verbose2(f"class {class_name}:")
    doc_str = format_doc(cursor)
    lines: List[str] = []
    if doc_str:
        lines.append(doc_str)
    lines.append(f"class {structs.get(class_name, class_name)}(ctypes.Structure):")
    fields = []
    for child in cursor.get_children():
        if child.kind == CursorKind.FIELD_DECL: # type: ignore
            ctypes_type, _ = map_type(child.type, structs, enums)
            fields.append(f'("{child.spelling}", {ctypes_type.__name__})')
    if fields:
        lines.append(f"    _fields_ = [{', '.join(fields)}]")
        verbose2(f"Class {class_name} fields: {fields}")
    else:
        lines.append(f"    pass")
        verbose2(f"Class {class_name} has no fields")

    # Collect constructors and methods for overload handling
    constructors = []
    methods: Dict[str, List[Cursor]] = {}
    for child in cursor.get_children():
        if is_template(child):
            verbose2(f"skipping template {child.spelling}")
            return []
        elif is_pure_virtual_method(child):
            verbose2(f"skipping virtual class {class_name} due to {child.spelling}")
            return []
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
            constructor_lines = format_constructor(child, structs, enums, i)
            lines.extend(constructor_lines)
            constructor_overloads.append(child)
            seen_signatures.add(sig)
    if constructor_overloads:
        lines.extend(generate_overload_selector('__init__', constructor_overloads, True))

    # Generate method bindings with overloads
    for method_name, method_list in methods.items():
        method_overloads = []
        for i, child in enumerate(method_list):
            sig = get_mangled_name(child)
            if sig not in seen_signatures:
                method_lines = format_method(child, structs, enums, i)
                lines.extend(method_lines)
                method_overloads.append(child)
                seen_signatures.add(sig)
        if method_overloads:
            lines.extend(generate_overload_selector(method_name, method_overloads, True))

    return lines

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

def calculate_signature(cursor: Cursor) -> str:
    """
    Returns the mangled name as the unique signature for a function/method.
    Used to avoid duplicate bindings for overloads.
    """
    return get_mangled_name(cursor)

def format_namespace(cursor: Cursor, structs: Dict[str, str], enums: Dict[str, str], seen_functions: Optional[Set[str]] = None) -> List[str]:
    """
    Recursively visits AST nodes starting from the given cursor.
    Collects functions, classes, and enums that are public and project-specific.
    """
    if seen_functions is None:
        seen_functions = set()
    lines: List[str] = []
    function_overloads: Dict[str, List[Cursor]] = {}
    namespace = calculate_namespace_prefix(cursor)
    verbose2(f"namespace {namespace}{cursor.spelling}")
    for c in cursor.get_children():
        try:
            if not is_project_header(c):
                verbose2(f"Skipping {c.spelling}, not in project header")
                continue
            if is_namespace(c):
                verbose2(f"namespace_enter {c.spelling}")
                lines.extend(format_namespace(c, structs, enums, seen_functions))
                verbose2(f"namespace_exit {c.spelling}")
            elif is_public_function(c):
                fn = calculate_signature(c)
                if fn not in seen_functions:
                    func_name = c.spelling
                    if func_name not in function_overloads:
                        function_overloads[func_name] = []
                    function_overloads[func_name].append(c)
                    seen_functions.add(fn)
            elif is_public_class(c):
                class_lines = format_class(c, structs, enums, seen_functions)
                if class_lines:
                    lines.extend(class_lines)
            elif is_public_enum(c) and c.semantic_parent.kind != CursorKind.CLASS_DECL: # type: ignore
                lines.extend(format_enum(c))
            else:
                verbose2(f"unknown {c.spelling}")
        except Exception as e:
            print(f"Unexpected error processing cursor {c.spelling}: {str(e)}")
            continue

    # Generate function overload selectors
    for func_name, overloads in function_overloads.items():
        lines.extend(format_function(overloads[0], structs, enums, 0))  # First overload
        for i, cursor in enumerate(overloads[1:], 1):
            lines.extend(format_function(cursor, structs, enums, i))  # Additional overloads
        if len(overloads) > 1:
            lines.extend(generate_overload_selector(func_name, overloads, False))

    return lines

def check_dependencies_changed() -> bool:
    """
    Check if the command line has changed or any included file has changed.
    N.B. Uses timestamps only. The _arg_dependency_file will be younger than the output
    file and will need to be touched again to make the build final. The
    output file will be listed as the first dependency file.
    """
    full_args = ' '.join(sys.argv)

    try:
        with open(_arg_dependency_file, 'r') as f:
            last_build_time: float = os.path.getmtime(_arg_dependency_file)
            lines: List[str] = f.read().splitlines()
            cached_args = lines[0] if lines else ""
            cached_dependencies: Set[str] = set(lines[1:])

            # Check if args match and dep file is newer than header and includes
            has_changed = False
            if full_args != cached_args:
                print("Args changed.")
                has_changed = True

            for file_path in _arg_header_files + [_arg_output_file] + list(cached_dependencies):
                if not os.path.exists(file_path) or os.path.getmtime(file_path) > last_build_time:
                    print("File changed: " + file_path)
                    has_changed = True

            verbose2(f"Dependencies changed: {has_changed}")
            return has_changed
    except Exception as e:
        verbose("Not found: " + _arg_dependency_file)

    return True # deletion is modification.

def load_translation_unit_and_dependencies(header_file: str) -> Tuple[TranslationUnit, Set[str]]:
    index = Index.create()
    verbose2(f"Parsing translation unit for {header_file}")
    translation_unit: Optional[TranslationUnit] = None
    try:
        translation_unit = index.parse(header_file, _arg_compiler_flags)
    except Exception as e:
        print(f"Error: {e} {header_file}")
        sys.exit(_exit_error)

    for diagnostic in translation_unit.diagnostics: # type: ignore
        print(diagnostic)

    if not translation_unit or (len(translation_unit.diagnostics) > 0
            and any(d.severity >= Diagnostic.Error for d in translation_unit.diagnostics)):
        print("Parsing failed due to errors.")
        sys.exit(_exit_error)

    dependencies: Set[str] = set()
    for include in translation_unit.get_includes():
        dependencies.add(include.include.name)
    return translation_unit, dependencies

def write_dependencies(include_files: Set[str]) -> None:
    """
    Write new dependency file
    """
    verbose2(f"Writing dependency file {_arg_dependency_file}")
    full_args = ' '.join(sys.argv)
    try:
        with open(_arg_dependency_file, 'w') as f:
            f.write(full_args + '\n')
            for dep in include_files:
                f.write(dep + '\n')
        verbose("Wrote " + _arg_dependency_file)
    except Exception as e:
        print(f"Error: Failed to write the dependency file: {e}")
        sys.exit(_exit_error)

def main() -> int:
    """
    Main entry point for the script. Parses command-line arguments, initializes Clang,
    and generates binding code for the given header file.
    """
    verbose2("main...")
    if not parse_argv():
        print("""\
Usage: python3 hatchling_bindings.py <compiler_flags> <module_name> <header_files>... <output_file>

This tool parses a C++ header file using libclang and generates binding code.

Arguments:
    compiler_flags  - Flags to pass to clang. Can be in any order on command line.
    module_name     - Module name to bind everything to.
    header_files... - Path(s) to the C++ header file(s) to parse.
    output_file     - Path to write the generated Python binding code.

Make sure to adjust _libclang_path at the top of this script if needed.
""")
        sys.exit(_exit_error)

    verbose("compiler_flags: " + ' '.join(_arg_compiler_flags))
    verbose("module_name: " + _arg_module_name)
    verbose("header_files: " + ' '.join(_arg_header_files))
    verbose("output_file: " + _arg_output_file)
    verbose("dependency_file: " + _arg_dependency_file)

    verbose(f"Setting libclang path: {_libclang_path}")
    Config.set_library_file(_libclang_path)

    verbose(f"Checking dependencies for generating {_arg_output_file}...")
    if not check_dependencies_changed():
        print("Nothing changed. All done.")
        return _exit_nothing_changed

    output_lines: List[str] = ["import ctypes", "from typing import Union, overload, Any", ""]
    output_lines.append(f"# Load the shared library")
    output_lines.append(f"lib = ctypes.CDLL('lib{_arg_module_name}.so')")
    output_lines.append("")
    output_headers: List[str] = []
    dependencies: Set[str] = set()
    structs: Dict[str, str] = {}
    enums: Dict[str, str] = {}

    for header_file in _arg_header_files:
        # prepend a full path include into the header.
        output_headers.append(f'#include <{os.path.abspath(header_file)}>')

        verbose(f"Visiting AST and generating bindings from {header_file}...")
        translation_unit, deps = load_translation_unit_and_dependencies(header_file)
        child_lines: List[str] = format_namespace(translation_unit.cursor, structs, enums, set())
        output_lines.extend(child_lines)
        dependencies |= deps
        verbose2(f"Added {len(child_lines)} lines from {header_file}, {len(deps)} dependencies")

    with open(_arg_output_file, "w") as f:
        f.write("\n".join(output_lines) if output_lines else "")
        print(f"Wrote {_arg_output_file}: {len(output_lines)} lines")

    write_dependencies(dependencies)
    verbose2("exit 0")
    return _exit_bindings_generated

if __name__ == "__main__":
    sys.exit(main())
