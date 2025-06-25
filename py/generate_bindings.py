import clang.cindex

clang.cindex.Config.set_library_file("/usr/lib/llvm-18/lib/libclang.so.1")
index = clang.cindex.Index.create()
tu = index.parse(
    "include/hx/hatchling_pch.hpp",
    args=["-std=c++17", "-DHX_RELEASE=0", "-fdiagnostics-absolute-paths", "-Iinclude"]
)

def is_public_function(cursor):
    return (
        cursor.kind == clang.cindex.CursorKind.FUNCTION_DECL
        and cursor.linkage == clang.cindex.LinkageKind.EXTERNAL
        and not cursor.spelling.startswith("hxx_")
    )

def is_public_class(cursor):
    return (
        cursor.kind == clang.cindex.CursorKind.CLASS_DECL
        and cursor.is_definition()
        and cursor.access_specifier in (
            clang.cindex.AccessSpecifier.PUBLIC,
            clang.cindex.AccessSpecifier.INVALID  # Top-level classes
        )
        and not cursor.spelling.startswith("hxx_")
    )

def is_public_method(cursor):
    return (
        cursor.kind == clang.cindex.CursorKind.CXX_METHOD
        and cursor.access_specifier == clang.cindex.AccessSpecifier.PUBLIC
        and not cursor.spelling.startswith("hxx_")
    )

def is_public_constructor(cursor):
    return (
        cursor.kind == clang.cindex.CursorKind.CONSTRUCTOR
        and cursor.access_specifier == clang.cindex.AccessSpecifier.PUBLIC
        and not cursor.spelling.startswith("hxx_")
    )

def is_public_destructor(cursor):
    return (
        cursor.kind == clang.cindex.CursorKind.DESTRUCTOR
        and cursor.access_specifier == clang.cindex.AccessSpecifier.PUBLIC
        and not cursor.spelling.startswith("hxx_")
    )

def is_template(cursor):
    return (
        cursor.kind in (
            clang.cindex.CursorKind.CLASS_TEMPLATE,
            clang.cindex.CursorKind.FUNCTION_TEMPLATE,
            clang.cindex.CursorKind.CLASS_TEMPLATE_PARTIAL_SPECIALIZATION
        )
    )

def get_pybind_function(cursor):
    return f'    m.def("{cursor.spelling}", &{cursor.spelling});'

def get_pybind_method(cursor, class_name):
    return f'        .def("{cursor.spelling}", &{class_name}::{cursor.spelling})'

def get_pybind_constructor(cursor, class_name):
    # Only supports default constructor for simplicity
    params = list(cursor.get_arguments())
    if not params:
        return f'        .def(py::init<>())'
    else:
        # For non-default constructors, you may need to generate py::init<Args...>()
        args = ", ".join([a.type.spelling for a in params])
        return f'        .def(py::init<{args}>())'

def get_pybind_class(cursor):
    class_name = cursor.spelling
    lines = [f'    py::class_<{class_name}>(m, "{class_name}")']
    has_ctor = False
    for m in cursor.get_children():
        if is_template(m):
            continue
        if is_public_constructor(m):
            lines.append(get_pybind_constructor(m, class_name))
            has_ctor = True
        elif is_public_method(m):
            lines.append(get_pybind_method(m, class_name))
    if not has_ctor:
        lines.append('        // No public constructor')
    lines[-1] += ';'  # End the chain
    return "\n".join(lines)

def visit(cursor):
    functions = []
    classes = []
    for c in cursor.get_children():
        if is_template(c):
            continue
        if is_public_function(c):
            functions.append(get_pybind_function(c))
        elif is_public_class(c):
            classes.append(get_pybind_class(c))
    return functions, classes

def main():
    print('#include <pybind11/pybind11.h>')
    print('namespace py = pybind11;')
    print('\nPYBIND11_MODULE(hatchling, m) {')
    functions, classes = visit(tu.cursor)
    for f in functions:
        print(f)
    for c in classes:
        print(c)
    print('}')

main()
