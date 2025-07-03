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
from typing import Dict, List, Set, Tuple, Optional, Any, Union

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

# Debug flag.
_verbose = True

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
    if _verbose:
        print(f" * {msg}")

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
            clang.cindex.AccessSpecifier.PUBLIC, # type: ignore
            clang.cindex.AccessSpecifier.INVALID  # type: ignore # Top-level enums
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
            clang.cindex.AccessSpecifier.PUBLIC, # type: ignore
            clang.cindex.AccessSpecifier.INVALID  # type: ignore # Top-level classes
        )
    )

def is_public_constructor(cursor: Cursor) -> bool:
    """
    Returns True if the cursor is a public constructor.
    """
    return (
        cursor.kind is CursorKind.CONSTRUCTOR # type: ignore
        and cursor.access_specifier is clang.cindex.AccessSpecifier.PUBLIC # type: ignore
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
        and cursor.access_specifier is clang.cindex.AccessSpecifier.PUBLIC # type: ignore
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
    return lines

def map_type(cpp_type: clang.cindex.Type, structs: Dict[str, str], enums: Dict[str, str]) -> Tuple[Any, str]:
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
    elif cpp_type.kind == clang.cindex.TypeKind.POINTER:
        pointee = cpp_type.get_pointee()
        base_type, _ = map_type(pointee, structs, enums)
        return (ctypes.POINTER(base_type), 'Any')
    return (ctypes.c_void_p, 'Any')  # Fallback

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
    lines.append(f"lib.{mangled_name}.restype = {return_type.__name__}")
    if arg_types:
        args = [arg[0].__name__ for arg in arg_types]
        lines.append(f"lib.{mangled_name}.argtypes = [{', '.join(args)}]")
    return lines

def format_method(cursor: Cursor, class_name: str, structs: Dict[str, str], enums: Dict[str, str], overload_index: int = 0) -> List[str]:
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

def format_constructor(cursor: Cursor, class_name: str, structs: Dict[str, str], enums: Dict[str, str], overload_index: int = 0) -> List[str]:
    """
    Generates binding lines for a class constructor.
    Includes overload decorator if needed.
    """
    mangled_name = get_mangled_name(cursor)
    arg_types = []
    for arg in cursor.get_arguments():
        ctypes_type, py_type = map_type(arg.type, structs, enums)
        arg_types.append((ctypes_type, py_type))

    lines: List[str] = []
    if overload_index > 0:
        lines.append(f"    @overload")

def format_class(cursor: Cursor, structs: Dict[str, str], enums: Dict[str, str], seen_signatures: Set[str]) -> List[str]:
    """
    Generates binding code for a class.
    Skips constructor bindings for abstract classes and those without public destructors.
    """
    class_name = cursor.spelling or f"class_{cursor.hash % 1000000}"
    doc_str = format_doc(cursor)
    lines: List[str] = []
    if doc_str:
        lines.append(doc_str)
    lines.append(f"class {structs.get(class_name, class_name)}(ctypes.Structure):")
    fields = []
    for child in cursor.get_children():
        if child.kind == CursorKind.FIELD_DECL:
            ctypes_type, _ = map_type(child.type, structs, enums)
            fields.append(f'("{child.spelling}", {ctypes_type.__name__})')
    if fields:
        lines.append(f"    _fields_ = [{', '.join(fields)}]")
    else:
        lines.append(f"    pass")

    # Collect constructors and methods for overload handling
    constructors = []
    methods = {}
    for child in cursor.get_children():
        if is_template(child) or is_pure_virtual_method(child):
            return []
        elif is_public_constructor(child):
            constructors.append(child)
        elif is_public_method(child):
            method_name = child.spelling
            if method_name not in methods:
                methods[method_name] = []
            methods[method_name].append(child)

    # Generate constructor bindings
    for i, child in enumerate(constructors):
        sig = get_mangled_name(child)
        if sig not in seen_signatures:
            lines.extend(format_constructor(child, class_name, structs, enums, i))
            seen_signatures.add(sig)

    # Generate method bindings with overloads
    for method_name, method_list in methods.items():
        for i, child in enumerate(method_list):
            sig = get_mangled_name(child)
            if sig not in seen_signatures:
                lines.extend(format_method(child, class_name, structs, enums, i))
                seen_signatures.add(sig)

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
    for c in cursor.get_children():
        try:
            if not is_project_header(c):
                continue
            if is_namespace(c):
                lines.extend(format_namespace(c, structs, enums, seen_functions))
            elif is_public_function(c):
                fn = calculate_signature(c)
                if fn not in seen_functions:
                    lines.extend(format_function(c, structs, enums, len([f for f in seen_functions if f.startswith(c.spelling)])))
                    seen_functions.add(fn)
            elif is_public_class(c):
                class_lines = format_class(c, structs, enums, seen_functions)
                if class_lines:
                    lines.extend(class_lines)
            elif is_public_enum(c) and c.semantic_parent.kind != CursorKind.CLASS_DECL: # type: ignore
                lines.extend(format_enum(c))
        except Exception as e:
            print(f"Unexpected error processing cursor {c.spelling}: {str(e)}")
            continue
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

            return has_changed
    except Exception as e:
        verbose("Not found: " + _arg_dependency_file)

    return True # deletion is modification.

def load_translation_unit_and_dependencies(header_file: str) -> Tuple[TranslationUnit, Set[str]]:
    index = clang.cindex.Index.create()
    translation_unit: Optional[TranslationUnit] = None
    try:
        translation_unit = index.parse(header_file, _arg_compiler_flags)
    except Exception as e:
        print(f"Error: {e} {header_file}")
        sys.exit(_exit_error)

    for diagnostic in translation_unit.diagnostics:
        print(diagnostic)

    if not translation_unit or (len(translation_unit.diagnostics) > 0
            and any(d.severity >= clang.cindex.Diagnostic.Error for d in translation_unit.diagnostics)):
        print("Parsing failed due to errors.")
        sys.exit(_exit_error)

    # Get all included files (direct and transitive)
    dependencies: Set[str] = set()
    for include in translation_unit.get_includes():
        dependencies.add(include.include.name)
    return translation_unit, dependencies

def write_dependencies(include_files: Set[str]) -> None:
    """
    Write new dependency file
    """
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

    verbose("Initializing Clang library.")
    clang.cindex.Config.set_library_file(_libclang_path)

    verbose(f"Checking dependencies for generating {_arg_output_file}...")
    if not check_dependencies_changed():
        print("Nothing changed. All done.")
        return _exit_nothing_changed

    # Begin data gathering loop
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

    # Write out gathered data.
    with open(_arg_output_file, "w") as f:
        f.write("\n".join(output_lines) if output_lines else "")
        print(f"Wrote {_arg_output_file}...")

    # Make the _arg_dependency_file older than the _arg_output_file.
    write_dependencies(dependencies)

    return _exit_bindings_generated

if __name__ == "__main__":
    sys.exit(main())
