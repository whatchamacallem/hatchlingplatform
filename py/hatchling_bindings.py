"""
hatchling_bindings.py: Automatic Binding Binding Generator for C++ Projects

This script automates the generation of Python bindings for C++ code using nanobind and libclang.
It parses a given C++ header file, discovers public functions, classes, methods, constructors,
destructors, and enums defined in a project, and then emits C++ code that exposes these symbols
to Python via nanobind.

Key Features:
- Filters only project-specific public symbols (functions, classes, methods, enums) for binding.
- Skips internal headers, variadic functions, and templates to avoid ambiguous or unsafe bindings.
- Extracts and formats C++ documentation comments as Python docstrings for each binding.
- Handles class constructors, destructors, and methods, including overloaded signatures.
- Emits nanobind code for enums, including nested enums within classes.

Usage:
    python hatchling_bindings.py <compiler_flags> <module_name> <header_files>... <output_file>
        compiler_flags  - Flags to pass to clang. Can be in any order on command line.
        module_name     - Module name to bind everything to.
        header_files... - Path(s) to the C++ header file(s) to parse.
        output_file     - Path to write the generated C++ binding code.

Adjust the _libclang_path at the top of the script as needed for your environment and project.
"""

import sys
import os
import clang.cindex

from typing import List, Set, Tuple, Optional
from clang.cindex import Cursor, CursorKind, TranslationUnit, LinkageKind

# Path to the libclang shared library. TODO.
_libclang_path = "/usr/lib/llvm-18/lib/libclang.so.1"

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
        and all(h in header_str for h in _arg_header_files)
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

def calculate_signature(cursor: Cursor) -> Tuple[str, Tuple[str, ...]]:
    """
    Returns a tuple uniquely identifying a function/method by name and parameter types.
    Used to avoid duplicate bindings for overloads.
    """
    params: Tuple[str, ...] = tuple(a.type.spelling for a in cursor.get_arguments())
    return (calculate_namespace_prefix(cursor) + cursor.spelling, params)

def format_doc(cursor: Cursor) -> str:
    """
    Extracts and formats the raw comment for use as a docstring.
    Strips comment markers and leading/trailing whitespace.
    Escapes characters (\\n, \\", \\) for safe embedding in a C string.
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
        doc = "\\n".join(cleaned) if cleaned else ""
        return f', R"doc({doc})doc"' if doc else ""
    return ""

def format_enum(cursor: Cursor, parent_handle: str) -> List[str]:
    """
    Generates binding code for an enum, either at module scope or nested within a class.
    Includes all enum constants as values.
    """
    enum_name = cursor.spelling
    doc_str = format_doc(cursor)
    lines: List[str] = [f'nanobind::enum_<{enum_name}>({parent_handle}, "{enum_name}"{doc_str})']
    for c in cursor.get_children():
        if c.kind is CursorKind.ENUM_CONSTANT_DECL: # type: ignore
            value_doc_str = format_doc(c)
            lines.append(f'.value("{c.spelling}", {enum_name}::{c.spelling}{value_doc_str})')
    lines[-1] += f'.export_values();'
    return lines

def format_function(cursor: Cursor, parent_handle: str) -> str:
    """
    Generates a binding line for a free function.
    Includes the full namespace in the function reference if applicable.
    Includes docstring if available.
    """
    doc_str = format_doc(cursor)
    params: List[str] = [a.type.spelling for a in cursor.get_arguments()]
    ret_type = cursor.result_type.spelling
    args = ", ".join(params) if params else ""

    # Get the full namespace path
    namespace_prefix = calculate_namespace_prefix(cursor)

    return f'{parent_handle}.def("{cursor.spelling}", static_cast<{ret_type}(*)({args})>(&{namespace_prefix}{cursor.spelling}){doc_str});'

def format_method(cursor: Cursor, class_namespace: str) -> str:
    """
    Generates a binding line for a class method. No trailing ;.
    Handles constness and docstrings.
    """
    doc_str = format_doc(cursor)
    params: List[str] = [a.type.spelling for a in cursor.get_arguments()]
    ret_type = cursor.result_type.spelling
    const = " const" if cursor.is_const_method() else ""
    args = ", ".join(params) if params else ""
    return (
        f'.def("{cursor.spelling}", '
        f'static_cast<{ret_type} ({class_namespace}::*)({args}){const}>(&{class_namespace}::{cursor.spelling}){doc_str})'
    )

def format_constructor(cursor: Cursor) -> str:
    """
    Generates a binding line for a class constructor. No trailing ;.
    Handles overloaded constructors by parameter count.
    """
    params: List[Cursor] = list(cursor.get_arguments())
    param_types: List[str] = [a.type.spelling for a in params]
    args = ", ".join(param_types) if param_types else ""
    return f'.def(nanobind::init<{args}>())'

def format_class(cursor: Cursor, parent_handle: str, seen_signatures: Set[Tuple[str, Tuple[str, ...]]]) -> List[str]:
    """
    Skips constructor bindings for abstract classes and those without public destructors.
    Assumes a default public destructor unless a protected or private one is found.
    """
    class_namespace = calculate_namespace_prefix(cursor) + cursor.spelling
    doc_str = format_doc(cursor)
    lines: List[str] = [f'nanobind::class_<{class_namespace}>({parent_handle}, "{cursor.spelling}"{doc_str})']

    for child in cursor.get_children():
        # A full instantiation of a template should not be dismissed as a
        # template. The language is clear about the difference between a template
        # and it's full instantiation. However support needs to be added for
        # declaring class/function bindings every time the binding API instantiates
        # a template class/function as part of the API.
        if is_template(child):
            continue
        elif is_pure_virtual_method(child):
            return [ ]
        elif is_public_constructor(child):
            sig: Tuple[str, Tuple[str, ...]] = calculate_signature(child)
            if sig not in seen_signatures:
                lines.append(format_constructor(child))
                seen_signatures.add(sig)
        elif is_public_method(child):
            sig = calculate_signature(child)
            if sig not in seen_signatures:
                lines.append(format_method(child, class_namespace))
                seen_signatures.add(sig)

    lines[-1] += ";"

    return lines

def format_namespace(cursor: Cursor, seen_functions: Optional[Set[Tuple[str, Tuple[str, ...]]]] = None) -> List[str]:
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
                continue
                ns = format_namespace(c)
                lines.append(ns)
            elif is_public_function(c):
                fn: Tuple[str, Tuple[str, ...]] = calculate_signature(c)
                if fn not in seen_functions:
                    lines.append(format_function(c))
                    seen_functions.add(fn)
            elif is_public_class(c):
                t: Optional[str] = format_class(c)
                if t:
                    lines.append(t)
            elif is_public_enum(c) and c.semantic_parent.kind != CursorKind.CLASS_DECL: # type: ignore
                lines.append(format_enum(c))



#            child_lines: List[str] = format_namespace(c, seen_functions)
#            lines.extend(child_lines)
        except Exception as e:
            print(f"Error processing cursor {c.spelling}: {str(e)}")
            continue
    return lines












def check_dependencies_changed() -> bool:

    """Check if the command line has changed or any included file has changed.
    N.B. Uses timestamps only. The _arg_dependency_file will be younger than the output
    file and will need to be touched again to make the build final. The
    output file will be listed as the first dependency file."""

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
    for include in translation_unit.format_includes():
        dependencies.add(include.include.name)

    return translation_unit, dependencies

def write_dependencies(include_files: Set[str]) -> None:
    """Write new dependency file"""

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
    output_file     - Path to write the generated C++ binding code.

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
    output_headers: List[str] = [ '#include <nanobind/nanobind.h>', ]
    output_lines: List[str] = [ f'NB_MODULE({_arg_module_name}, module_root_) ', '{' ]
    dependencies: Set[str] = set()

    for header_file in _arg_header_files:
        # prepend a full path include into the header.
        output_headers.append(f'#include <{os.path.abspath(header_file)}>')

        verbose(f"Visiting AST and generating bindings from {header_file}...")
        translation_unit, deps = load_translation_unit_and_dependencies(header_file)


            # calculate_namespace_prefix() -> a per-namespace seen list. just qualify symbol with namespace.
        child_lines: List[str] = format_namespace(translation_unit.cursor, "module_root_) xxx
        output_lines.extend(child_lines)
        dependencies |= deps








    output_lines.append('}\n')
    output_lines = output_headers + output_lines # prepend headers

    # Write out gathered data.
    with open(_arg_output_file, "w") as f:
        f.write("\n".join(output_lines) if output_lines else "")
        print(f"Wrote {_arg_output_file}...")

    # Make the _arg_dependency_file older than the _arg_output_file. This will
    # detect modification of the output file and regenerate it in that case.
    write_dependencies(dependencies)

    return _exit_bindings_generated

if __name__ == "__main__":
    sys.exit(main())
