"""
generate_bindings.py: Automatic Binding Binding Generator for C++ Projects

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
    python generate_bindings.py <HEADER_FILE> <OUTPUT_FILE>

Adjust the CLANG_LIBRARY_FILE and CLANG_ARGS variables at the top of the script as needed
for your environment and project.
"""

import sys
import os
import clang.cindex

from typing import List, Set, Tuple, Optional
from clang.cindex import Cursor, TranslationUnit


# Path to the libclang shared library.
CLANG_LIBRARY_FILE = "/usr/lib/llvm-18/lib/libclang.so.1"

# Arguments passed to Clang when parsing the header file.
CLANG_ARGS: List[str] = [
    "-std=c++17",
    "-DHX_RELEASE=0",
    "-fdiagnostics-absolute-paths",
    "-Wall",
    "-I../include"
]

VERBOSE = True

def verbose(msg: str) -> None:
    if VERBOSE:
        print(f" * {msg}")

def is_project_header(cursor: Cursor) -> bool:
    """
    Returns True if the cursor's location is in a project-specific header file.
    Adjust the string checks as needed for your project's include structure.
    """
    header = cursor.location.file
    header_str = str(header) if header is not None else ""
    return (
        header is not None
        and "include/hx/" in header_str
        and "include/hx/detail/" not in header_str
    )

def uses_va_list(cursor: Cursor) -> bool:
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
        cursor.kind is clang.cindex.CursorKind.FUNCTION_DECL # type: ignore
        and cursor.linkage is clang.cindex.LinkageKind.EXTERNAL # type: ignore
        and not cursor.type.is_function_variadic()
        and not uses_va_list(cursor)
    )

def is_public_class(cursor: Cursor) -> bool:
    """
    Returns True if the cursor is a public class definition.
    """
    return (
        cursor.kind is clang.cindex.CursorKind.CLASS_DECL # type: ignore
        and cursor.is_definition()
        and cursor.access_specifier in (
            clang.cindex.AccessSpecifier.PUBLIC, # type: ignore
            clang.cindex.AccessSpecifier.INVALID  # type: ignore # Top-level classes
        )
    )

def is_public_method(cursor: Cursor) -> bool:
    """
    Returns True if the cursor is a public, non-variadic, non-template method.
    """
    return (
        cursor.kind is clang.cindex.CursorKind.CXX_METHOD # type: ignore
        and cursor.access_specifier is clang.cindex.AccessSpecifier.PUBLIC # type: ignore
        and not cursor.type.is_function_variadic()
        and not uses_va_list(cursor)
    )

def is_public_constructor(cursor: Cursor) -> bool:
    """
    Returns True if the cursor is a public constructor.
    """
    return (
        cursor.kind is clang.cindex.CursorKind.CONSTRUCTOR # type: ignore
        and cursor.access_specifier is clang.cindex.AccessSpecifier.PUBLIC # type: ignore
    )

def is_nonpublic_destructor(cursor: Cursor) -> bool:
    """
    Returns True if the cursor is a non-public destructor.
    """
    return (
        cursor.kind is clang.cindex.CursorKind.DESTRUCTOR # type: ignore
        and cursor.access_specifier != clang.cindex.AccessSpecifier.PUBLIC # type: ignore
    )

def is_template(cursor: Cursor) -> bool:
    """
    Returns True if the cursor is a template (class or function).
    Templates are skipped for binding.
    """
    return (
        cursor.kind in (
            clang.cindex.CursorKind.CLASS_TEMPLATE, # type: ignore
            clang.cindex.CursorKind.FUNCTION_TEMPLATE, # type: ignore
            clang.cindex.CursorKind.CLASS_TEMPLATE_PARTIAL_SPECIALIZATION # type: ignore
        )
    )

def is_public_enum(cursor: Cursor) -> bool:
    """
    Returns True if the cursor is a public enum definition.
    """
    return (
        cursor.kind is clang.cindex.CursorKind.ENUM_DECL # type: ignore
        and cursor.is_definition()
        and cursor.access_specifier in (
            clang.cindex.AccessSpecifier.PUBLIC, # type: ignore
            clang.cindex.AccessSpecifier.INVALID  # type: ignore # Top-level enums
        )
    )

def is_namespace(cursor: Cursor) -> bool:
    """
    Returns True if the cursor is a namespace definition.
    """
    return (
        cursor.kind is clang.cindex.CursorKind.NAMESPACE  # type: ignore
        and cursor.is_definition()
    )

def get_cursor_doc(cursor: Cursor) -> str:
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

def get_full_namespace(cursor: Cursor) -> str:
    """
    Returns the fully qualified namespace path for a cursor (e.g., 'outer::inner').
    Traverses semantic parents to build the namespace chain.
    """
    namespaces: List[str] = []
    current: Optional[Cursor] = cursor.semantic_parent
    while current and current.kind is clang.cindex.CursorKind.NAMESPACE: # type: ignore
        namespaces.append(current.spelling)
        current = current.semantic_parent
    return "::".join(reversed(namespaces)) if namespaces else ""

def get_bind_function(cursor: Cursor) -> str:
    """
    Generates a binding line for a free function.
    Includes the full namespace in the function reference if applicable.
    Includes docstring if available.
    """
    doc_str = get_cursor_doc(cursor)
    params: List[str] = [a.type.spelling for a in cursor.get_arguments()]
    ret_type = cursor.result_type.spelling
    args = ", ".join(params) if params else ""

    # Get the full namespace path
    namespace = get_full_namespace(cursor)
    namespace_prefix = f"{namespace}::" if namespace else ""

    return f'm.def("{cursor.spelling}", static_cast<{ret_type}(*)({args})>(&{namespace_prefix}{cursor.spelling}){doc_str});'

def get_bind_method(cursor: Cursor, class_name: str) -> str:
    """
    Generates a binding line for a class method.
    Handles constness and docstrings.
    """
    doc_str = get_cursor_doc(cursor)
    params: List[str] = [a.type.spelling for a in cursor.get_arguments()]
    ret_type = cursor.result_type.spelling
    const = " const" if cursor.is_const_method() else ""
    args = ", ".join(params) if params else ""
    return (
        f'.def("{cursor.spelling}", '
        f'static_cast<{ret_type} ({class_name}::*)({args}){const}>(&{class_name}::{cursor.spelling}){doc_str})'
    )

def get_bind_constructor(cursor: Cursor, class_name: str) -> str:
    """
    Generates a binding line for a class constructor.
    Handles overloaded constructors by parameter count.
    """
    params: List[Cursor] = list(cursor.get_arguments())
    param_types: List[str] = [a.type.spelling for a in params]
    args = ", ".join(param_types) if param_types else ""
    return f'.def(nanobind::init<{args}>())'

def get_function_signature(cursor: Cursor) -> Tuple[str, Tuple[str, ...]]:
    """
    Returns a tuple uniquely identifying a function by name and parameter types.
    Used to avoid duplicate bindings for overloaded functions.
    """
    params: Tuple[str, ...] = tuple(a.type.spelling for a in cursor.get_arguments())
    return (cursor.spelling, params)

def get_method_signature(cursor: Cursor) -> Tuple[str, Tuple[str, ...]]:
    """
    Returns a tuple uniquely identifying a method by name and parameter types.
    Used to avoid duplicate bindings for overloaded methods.
    """
    params: Tuple[str, ...] = tuple(a.type.spelling for a in cursor.get_arguments())
    return (cursor.spelling, params)

def get_bind_namespace(cursor: Cursor) -> str:
    """
    Generates binding code for a namespace, using a static nanobind::class_
    instead of a nanobind::module_.
    Binds public functions, enums, and nested namespaces as attributes of the class.
    """
    namespace_name = cursor.spelling
    doc_str = get_cursor_doc(cursor)
    # Define a nanobind::class_ for the namespace, using an empty struct as the type
    lines: List[str] = [f'nanobind::class_<nanobind::object>(m, "{namespace_name}"{doc_str})']
    functions: List[str] = []
    enums: List[str] = []
    seen_functions: Set[Tuple[str, Tuple[str, ...]]] = set()

    for m in cursor.get_children():
        if is_template(m):
            continue
        elif is_public_function(m):
            sig: Tuple[str, Tuple[str, ...]] = get_function_signature(m)
            if sig not in seen_functions:
                # Generate function binding using the namespace's class
                doc_m = get_cursor_doc(m)
                doc_m_str = f', R"doc({doc_m})doc"' if doc_m else ""
                params: List[str] = [a.type.spelling for a in m.get_arguments()]
                ret_type = m.result_type.spelling
                args = ", ".join(params) if params else ""
                namespace_prefix = get_full_namespace(m)
                namespace_prefix = f"{namespace_prefix}::" if namespace_prefix else ""
                functions.append(
                    f'.def_static("{m.spelling}", '
                    f'static_cast<{ret_type}(*)({args})>(&{namespace_prefix}{m.spelling}){doc_m_str})'
                )
                seen_functions.add(sig)
        elif is_public_enum(m):
            enum_name = m.spelling
            doc_m = get_cursor_doc(m)
            doc_m_str = f', R"doc({doc_m})doc"' if doc_m else ""
            enum_lines: List[str] = [f'nanobind::enum_<{enum_name}>({cursor.spelling}, "{enum_name}"{doc_m_str})']
            for c in m.get_children():
                if c.kind is clang.cindex.CursorKind.ENUM_CONSTANT_DECL: # type: ignore
                    value_doc = get_cursor_doc(c)
                    value_doc_str = f', R"doc({value_doc})doc"' if value_doc else ""
                    enum_lines.append(f'.value("{c.spelling}", {enum_name}::{c.spelling}{value_doc_str})')
            enum_lines[-1] += f'.export_values()'
            enums.append("\n".join(enum_lines))
        elif is_namespace(m):
            # Recursively process nested namespaces, binding to the current namespace's class
            nested_lines = get_bind_namespace(m)
            lines.append(nested_lines)

    lines.extend(functions)
    lines.extend(enums)
    return "\n".join(lines)

def is_pure_virtual_method(cursor: Cursor) -> bool:
    """
    Returns True if the cursor is a pure virtual method (declared with = 0).
    """
    return (
        cursor.kind is clang.cindex.CursorKind.CXX_METHOD  # type: ignore
        and cursor.is_pure_virtual_method()  # Check for pure virtual
    )

def get_bind_class(cursor: Cursor) -> Optional[str]:
    """
    Generates binding code for a class, including its constructors and methods.
    Nested enums are included within the class scope.
    Skips constructor bindings for abstract classes (those with pure virtual methods).
    Assumes a public destructor unless a protected or private one is found.
    """
    class_name = cursor.spelling
    doc_str = get_cursor_doc(cursor)
    lines: List[str] = [f'nanobind::class_<{class_name}>(m, "{class_name}"{doc_str})']
    ctors: List[str] = []
    methods: List[str] = []
    seen_methods: Set[Tuple[str, Tuple[str, ...]]] = set()
    has_public_destructor = True
    for m in cursor.get_children():
        if is_pure_virtual_method(m):
            return None
        elif is_template(m):
            continue
        elif is_public_constructor(m):
            sig: Tuple[str, Tuple[str, ...]] = get_method_signature(m)
            if sig not in seen_methods:
                ctors.append(get_bind_constructor(m, class_name))
                seen_methods.add(sig)
        elif is_nonpublic_destructor(m):
            has_public_destructor = False
        elif is_public_method(m):
            sig = get_method_signature(m)
            if sig not in seen_methods:
                methods.append(get_bind_method(m, class_name))
                seen_methods.add(sig)
    if not ctors or not has_public_destructor:
        return None
    lines.extend(ctors)
    lines.extend(methods)
    lines.append('.def("__del__", [](nanobind::object self) { self.release(); });')
    return "\n".join(lines)

def get_bind_enum(cursor: Cursor) -> str:
    """
    Generates binding code for an enum, either at module scope or nested within a class.
    Includes all enum constants as values.
    """
    enum_name = cursor.spelling
    doc_str = get_cursor_doc(cursor)
    lines: List[str] = [f'nanobind::enum_<{enum_name}>(m, "{enum_name}"{doc_str})']
    for c in cursor.get_children():
        if c.kind is clang.cindex.CursorKind.ENUM_CONSTANT_DECL: # type: ignore
            value_doc = get_cursor_doc(c)
            value_doc_str = f', R"doc({value_doc})doc"' if value_doc else ""
            lines.append(f'.value("{c.spelling}", {enum_name}::{c.spelling}{value_doc_str})')
    lines[-1] += f'.export_values();'
    return "\n".join(lines)

def visit(cursor: Cursor, seen_functions: Optional[Set[Tuple[str, Tuple[str, ...]]]] = None) -> List[str]:
    """
    Recursively visits AST nodes starting from the given cursor.
    Collects functions, classes, and enums that are public and project-specific.
    """
    if seen_functions is None:
        seen_functions = set()
    lines: List[str] = []
    for c in cursor.get_children():
        try:
            if not is_project_header(c) or is_template(c):
                continue
            if is_namespace(c):
                continue
                ns = get_bind_namespace(c)
                lines.append(ns)
            elif is_public_function(c):
                fn: Tuple[str, Tuple[str, ...]] = get_function_signature(c)
                if fn not in seen_functions:
                    lines.append(get_bind_function(c))
                    seen_functions.add(fn)
            elif is_public_class(c):
                t: Optional[str] = get_bind_class(c)
                if t:
                    lines.append(t)
            elif is_public_enum(c) and c.semantic_parent.kind != clang.cindex.CursorKind.CLASS_DECL: # type: ignore
                lines.append(get_bind_enum(c))
            child_lines: List[str] = visit(c, seen_functions)
            lines.extend(child_lines)
        except Exception as e:
            print(f"Error processing cursor {c.spelling}: {str(e)}")
            continue
    return lines

# Check if the command line has changed or any included file has changed.
# N.B. Uses timestamps only. The dep_file will be younger than the output
# file and will need to be touched again to make the build final. The
# output file will be listed as the first dependency file.
def load_translation_unit_if_changed(header_file: str, clang_args: List[str],
        output_file: str, dep_file: str) -> Optional[TranslationUnit]:

    full_args = f"{header_file} {' '.join(clang_args)} {output_file}"

    try:
        with open(dep_file, 'r') as f:
            last_build_time: float = os.path.getmtime(dep_file)
            lines: List[str] = f.read().splitlines()
            cached_args = lines[0] if lines else ""
            cached_deps: Set[str] = set(lines[1:])

            # Check if args match and dep file is newer than header and includes
            has_changed = False
            if full_args != cached_args:
                print("Args changed.")
                has_changed = True

            for file_path in [header_file, output_file] + list(cached_deps):
                if not os.path.exists(file_path) or os.path.getmtime(file_path) > last_build_time:
                    print("File changed: " + file_path)
                    has_changed = True

            if not has_changed:
                return None
    except IOError as e:
        verbose("Not found: " + dep_file)

    index = clang.cindex.Index.create()
    tu: TranslationUnit = index.parse(header_file, clang_args)

    for diag in tu.diagnostics:
        print(diag)

    if not tu or (len(tu.diagnostics) > 0 and any(d.severity >= clang.cindex.Diagnostic.Error for d in tu.diagnostics)):
        print("Parsing failed due to errors.")
        sys.exit(1)

    # Get all included files (direct and transitive)
    include_files: Set[str] = set()
    for inc in tu.get_includes():
        include_files.add(inc.include.name)

    # Write new dependency file
    try:
        with open(dep_file, 'w') as f:
            f.write(full_args + '\n')
            for dep in include_files:
                f.write(dep + '\n')
    except IOError as e:
        print(f"Error: Failed to write the dependency file: {e}")

    return tu

def main() -> int:
    """
    Main entry point for the script. Parses command-line arguments, initializes Clang,
    and generates binding code for the given header file.
    """
    if len(sys.argv) != 3:
        print(
            "Usage: python generate_bindings.py <HEADER_FILE> <OUTPUT_FILE>\n"
            "\n"
            "This tool parses a C++ header file using libclang and generates binding code.\n"
            "\n"
            "Arguments:\n"
            "  <HEADER_FILE>   Path to the C++ header file to parse\n"
            "  <OUTPUT_FILE>   Path to write the generated C++ binding code\n"
            "\n"
            "Make sure to adjust CLANG_LIBRARY_FILE and CLANG_ARGS at the top of this script if needed."
        )
        sys.exit(1)

    header_file = os.path.abspath(sys.argv[1])
    output_file = os.path.abspath(sys.argv[2])
    dependency_file = output_file + '.d.txt'

    verbose(f"Generating {output_file} from {header_file}")

    verbose("Initializing Clang library.")
    clang.cindex.Config.set_library_file(CLANG_LIBRARY_FILE)

    verbose("Checking dependency file and parsing if needed.")
    tu: Optional[TranslationUnit] = load_translation_unit_if_changed(header_file, CLANG_ARGS, output_file, dependency_file)
    if tu is None:
        print("Nothing changed. All done.")
        return 0

    verbose("Visiting AST and generating bindings.")
    output_lines: List[str] = [
        '// hatchling python bindings generator',
        f'#include <{header_file}>',
        '#include <nanobind/nanobind.h>',
        'NB_MODULE(hatchling, m) {'
    ]

    child_lines: List[str] = visit(tu.cursor)
    output_lines.extend(child_lines)
    output_lines.append('}\n')

    verbose("Writing output and then updating timestamp of dependency file...")
    with open(output_file, "w") as f:
        f.write("\n".join(output_lines) if output_lines else "")

    # Mark the dependency_file as being older than the output_file. This will
    # detect modification of the output file and regenerate it in that case.
    os.utime(dependency_file, None)

    print(f"Wrote {output_file}...")
    return 0

if __name__ == "__main__":
    exit(main())
