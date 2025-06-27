"""
generate_bindings.py: Automatic Pybind11 Binding Generator for C++ Projects

This script automates the generation of Python bindings for C++ code using pybind11 and libclang.
It parses a given C++ header file, discovers public functions, classes, methods, constructors,
destructors, and enums defined in a project, and then emits C++ code that exposes these symbols
to Python via pybind11.

Key Features:
- Filters only project-specific public symbols (functions, classes, methods, enums) for binding.
    - See is_project_header.
- Skips internal headers, variadic functions, and templates to avoid ambiguous or unsafe bindings.
- Extracts and formats C++ documentation comments as Python docstrings for each binding.
- Handles class constructors, destructors, and methods, including overloaded signatures.
- Emits pybind11 code for enums, including nested enums within classes.

Usage:
    python generate_bindings.py <HEADER_FILE> <OUTPUT_FILE>

Adjust the CLANG_LIBRARY_FILE and CLANG_ARGS variables at the top of the script as needed
for your environment and project.
"""

import sys
import clang.cindex

# Path to the libclang shared library.
CLANG_LIBRARY_FILE = "/usr/lib/llvm-18/lib/libclang.so.1"

# Arguments passed to Clang when parsing the header file.
CLANG_ARGS = [
    "-std=c++17",
    "-DHX_RELEASE=0",
    "-fdiagnostics-absolute-paths",
    "-fno-exceptions",
    "-I../include"
]

VERBOSE = True

def is_project_header(cursor):
    """
    Returns True if the cursor's location is in a project-specific header file.
    Adjust the string checks as needed for your project's include structure.
    """
    header = cursor.location.file
    header_str = str(header) if header is not None else ""
    return (
        header is not None
        and "include/hx/" in header_str
        and "include/hx/internal/" not in header_str
    )

def uses_va_list(cursor):
    """
    Returns True if any argument to the function uses va_list (variadic argument list).
    Such functions are skipped for binding.
    """
    return any("va_list" in a.type.spelling for a in cursor.get_arguments())

def is_public_function(cursor):
    """
    Returns True if the cursor is a public, non-variadic, non-template, project function.
    """
    return (
        cursor.kind == clang.cindex.CursorKind.FUNCTION_DECL # type: ignore
        and cursor.linkage == clang.cindex.LinkageKind.EXTERNAL # type: ignore
        and is_project_header(cursor)
        and not cursor.type.is_function_variadic()
        and not uses_va_list(cursor)
    )

def is_public_class(cursor):
    """
    Returns True if the cursor is a public class definition in the project.
    """
    return (
        cursor.kind == clang.cindex.CursorKind.CLASS_DECL # type: ignore
        and cursor.is_definition()
        and cursor.access_specifier in (
            clang.cindex.AccessSpecifier.PUBLIC, # type: ignore
            clang.cindex.AccessSpecifier.INVALID  # type: ignore # Top-level classes
        )
        and is_project_header(cursor)
    )

def is_public_method(cursor):
    """
    Returns True if the cursor is a public, non-variadic, non-template method.
    """
    return (
        cursor.kind == clang.cindex.CursorKind.CXX_METHOD # type: ignore
        and cursor.access_specifier == clang.cindex.AccessSpecifier.PUBLIC # type: ignore
        and not cursor.type.is_function_variadic()
        and not uses_va_list(cursor)
    )

def is_public_constructor(cursor):
    """
    Returns True if the cursor is a public constructor.
    """
    return (
        cursor.kind == clang.cindex.CursorKind.CONSTRUCTOR # type: ignore
        and cursor.access_specifier == clang.cindex.AccessSpecifier.PUBLIC # type: ignore
    )

def is_nonpublic_destructor(cursor):
    """
    Returns True if the cursor is a non-public destructor.
    """
    return (
        cursor.kind == clang.cindex.CursorKind.DESTRUCTOR # type: ignore
        and cursor.access_specifier != clang.cindex.AccessSpecifier.PUBLIC # type: ignore
    )

def is_template(cursor):
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

def is_public_enum(cursor):
    """
    Returns True if the cursor is a public enum definition in the project.
    """
    return (
        cursor.kind == clang.cindex.CursorKind.ENUM_DECL # type: ignore
        and cursor.is_definition()
        and cursor.access_specifier in (
            clang.cindex.AccessSpecifier.PUBLIC, # type: ignore
            clang.cindex.AccessSpecifier.INVALID  # type: ignore # Top-level enums
        )
        and is_project_header(cursor)  # Only include project enums
    )

def get_cursor_doc(cursor):
    """
    Extracts and formats the raw comment for use as a docstring.
    Strips comment markers and leading/trailing whitespace.
    """
    comment = cursor.raw_comment
    if comment:
        lines = comment.splitlines()
        cleaned = []
        for line in lines:
            line = line.strip()
            if line.startswith("///"):
                line = line[3:].strip()
            elif line.startswith("//"):
                line = line[2:].strip()
            elif line.startswith("/*"):
                line = line[2:].strip()
            elif line.endswith("*/"):
                line = line[:-2].strip()
            cleaned.append(line)
        return "\\n".join(cleaned) if len(cleaned) else ""
    return ""

def get_pybind_function(cursor):
    """
    Generates a pybind11 binding line for a free function.
    Includes docstring if available.
    """
    doc = get_cursor_doc(cursor)
    doc_str = f', R"doc({doc})doc"' if doc else ""
    params = [a.type.spelling for a in cursor.get_arguments()]
    ret_type = cursor.result_type.spelling
    args = ", ".join(params) if len(params) else ""
    return f'  m.def("{cursor.spelling}", static_cast<{ret_type}(*)({args})>(&{cursor.spelling}){doc_str});'

def get_pybind_method(cursor, class_name):
    """
    Generates a pybind11 binding line for a class method.
    Handles constness and docstrings.
    """
    doc = get_cursor_doc(cursor)
    doc_str = f', R"doc({doc})doc"' if doc else ""
    params = [a.type.spelling for a in cursor.get_arguments()]
    ret_type = cursor.result_type.spelling
    const = " const" if cursor.is_const_method() else ""
    args = ", ".join(params) if len(params) else ""
    return (
        f'  .def("{cursor.spelling}", '
        f'static_cast<{ret_type} ({class_name}::*)({args}){const}>(&{class_name}::{cursor.spelling}){doc_str})'
    )

def get_pybind_constructor(cursor, class_name):
    """
    Generates a pybind11 binding line for a class constructor.
    Handles overloaded constructors by parameter count.
    """
    params = list(cursor.get_arguments())
    param_types = [a.type.spelling for a in params]
    args = ", ".join(param_types) if len(param_types) else ""
    return f'  .def(py::init<{args}>())'

def get_function_signature(cursor):
    """
    Returns a tuple uniquely identifying a function by name and parameter types.
    Used to avoid duplicate bindings for overloaded functions.
    """
    params = tuple(a.type.spelling for a in cursor.get_arguments())
    return (cursor.spelling, params)

def get_method_signature(cursor):
    """
    Returns a tuple uniquely identifying a method by name and parameter types.
    Used to avoid duplicate bindings for overloaded methods.
    """
    params = tuple(a.type.spelling for a in cursor.get_arguments())
    return (cursor.spelling, params)

def get_pybind_class(cursor):
    """
    Generates pybind11 binding code for a class, including its constructors and methods.
    Nested enums are included within the class scope.
    Assumes a public destructor unless a protected or private one is found.
    """
    class_name = cursor.spelling
    doc = get_cursor_doc(cursor)
    doc_str = f', R"doc({doc})doc"' if doc else ""
    lines = [f'    py::class_<{class_name}>(m, "{class_name}"{doc_str})']
    ctors = []
    methods = []
    seen_methods = set()
    has_public_destructor = True
    for m in cursor.get_children():
        if is_template(m):
            continue
        elif is_public_constructor(m):
            sig = get_method_signature(m)
            if sig not in seen_methods:
                ctors.append(get_pybind_constructor(m, class_name))
                seen_methods.add(sig)
        elif is_nonpublic_destructor(m):
            has_public_destructor = False
        elif is_public_method(m):
            sig = get_method_signature(m)
            if sig not in seen_methods:
                methods.append(get_pybind_method(m, class_name))
                seen_methods.add(sig)
    if not ctors or not has_public_destructor:
        return None
    lines.extend(ctors)
    lines.extend(methods)
    lines.append('  .def("__del__", [](py::object self) { self.release(); });')
    return "\n".join(lines) if len(lines) else ""

def get_pybind_enum(cursor):
    """
    Generates pybind11 binding code for an enum, either at module scope or nested within a class.
    Includes all enum constants as values.
    """
    enum_name = cursor.spelling
    doc = get_cursor_doc(cursor)
    doc_str = f', R"doc({doc})doc"' if doc else ""

    lines = [f'    py::enum_<{enum_name}>(m, "{enum_name}"{doc_str})']
    for c in cursor.get_children():
        if c.kind == clang.cindex.CursorKind.ENUM_CONSTANT_DECL: # type: ignore
            value_doc = get_cursor_doc(c)
            value_doc_str = f', R"doc({value_doc})doc"' if value_doc else ""
            lines.append(f'    .value("{c.spelling}", {enum_name}::{c.spelling}{value_doc_str})')
    lines[-1] += f'.export_values();'
    return "\n".join(lines) if len(lines) else ""

def visit(cursor, seen_functions=None):
    """
    Recursively visits AST nodes starting from the given cursor.
    Collects functions, classes, and enums that are public and project-specific.
    """
    if seen_functions is None:
        seen_functions = set()
    functions = []
    classes = []
    enums = []
    for c in cursor.get_children():
        try:
            if is_template(c):
                continue
            if is_public_function(c):
                sig = get_function_signature(c)
                if sig not in seen_functions:
                    functions.append(get_pybind_function(c))
                    seen_functions.add(sig)
            elif is_public_class(c):
                class_code = get_pybind_class(c)
                if class_code is not None:
                    classes.append(class_code)
            elif is_public_enum(c) and c.semantic_parent.kind != clang.cindex.CursorKind.CLASS_DECL: # type: ignore
                enums.append(get_pybind_enum(c))
            child_functions, child_classes, child_enums = visit(c, seen_functions)
            functions.extend(child_functions)
            classes.extend(child_classes)
            enums.extend(child_enums)
        except Exception as e:
            continue
    return functions, classes, enums

def status(msg):
    if VERBOSE:
        print(f"[generate_bindings] {msg}")

def main():
    """
    Main entry point for the script. Parses command-line arguments, initializes Clang,
    and generates binding code for the given header file.
    """
    if len(sys.argv) != 3:
        print(
            "Usage: python generate_bindings.py <HEADER_FILE> <OUTPUT_FILE>\n"
            "\n"
            "This tool parses a C++ header file using libclang and generates pybind11 binding code.\n"
            "\n"
            "Arguments:\n"
            "  <HEADER_FILE>   Path to the C++ header file to parse\n"
            "  <OUTPUT_FILE>   Path to write the generated C++ pybind11 binding code\n"
            "\n"
            "Make sure to adjust CLANG_LIBRARY_FILE and CLANG_ARGS at the top of this script if needed."
        )
        sys.exit(1)

    header_file = sys.argv[1]
    output_file = sys.argv[2]

    status(f"Using header file: {header_file}")
    status(f"Output will be written to: {output_file}")

    clang.cindex.Config.set_library_file(CLANG_LIBRARY_FILE)
    status("Initialized Clang library.")

    index = clang.cindex.Index.create()
    status("Parsing translation unit...")
    tu = index.parse(
        header_file,
        args=CLANG_ARGS
    )

    status("Processing diagnostics...")
    for diag in tu.diagnostics:
        print(diag)

    if not tu or (len(tu.diagnostics) > 0 and any(d.severity >= clang.cindex.Diagnostic.Error for d in tu.diagnostics)):
        status("Parsing failed due to errors.")
        sys.exit(1)

    status("Visiting AST and generating bindings...")
    output_lines = []
    output_lines.append('// hatchling python bindings generator')
    output_lines.append('#include <hx/hatchling_pch.hpp>')
    output_lines.append('#include <pybind11/pybind11.h>')
    output_lines.append('namespace py = pybind11;')
    output_lines.append('\nPYBIND11_MODULE(hatchling, m) {')
    functions, classes, enums = visit(tu.cursor)
    for e in enums:
        output_lines.append(e)
    for c in classes:
        output_lines.append(c)
    for f in functions:
        output_lines.append(f)
    output_lines.append('}\n')

    status("Writing output file...")

    with open(output_file, "w") as f:
        f.write("\n".join(output_lines) if len(output_lines) else "")

    status("Binding generation complete.")

if __name__ == "__main__":
    main()
