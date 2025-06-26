import clang.cindex

clang.cindex.Config.set_library_file("/usr/lib/llvm-18/lib/libclang.so.1")
index = clang.cindex.Index.create()
tu = index.parse(
    "include/hx/hatchling_pch.hpp",
    args=["-std=c++17", "-DHX_RELEASE=0", "-fdiagnostics-absolute-paths", "-Iinclude"]
)

def is_project_header(cursor):
    # Only include headers from your project (adjust as needed)
    header = cursor.location.file
    header_str = str(header) if header is not None else ""
    return (
        header is not None
        and "include/hx/" in header_str
        and "include/hx/internal/" not in header_str
    )

def is_public_function(cursor):
    return (
        cursor.kind == clang.cindex.CursorKind.FUNCTION_DECL # type: ignore
        and cursor.linkage == clang.cindex.LinkageKind.EXTERNAL # type: ignore
        and is_project_header(cursor)
    )

def is_public_class(cursor):
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
    return (
        cursor.kind == clang.cindex.CursorKind.CXX_METHOD # type: ignore
        and cursor.access_specifier == clang.cindex.AccessSpecifier.PUBLIC # type: ignore
    )

def is_public_constructor(cursor):
    return (
        cursor.kind == clang.cindex.CursorKind.CONSTRUCTOR # type: ignore
        and cursor.access_specifier == clang.cindex.AccessSpecifier.PUBLIC # type: ignore
    )

def is_public_destructor(cursor):
    return (
        cursor.kind == clang.cindex.CursorKind.DESTRUCTOR # type: ignore
        and cursor.access_specifier == clang.cindex.AccessSpecifier.PUBLIC # type: ignore
    )

def is_template(cursor):
    return (
        cursor.kind in (
            clang.cindex.CursorKind.CLASS_TEMPLATE, # type: ignore
            clang.cindex.CursorKind.FUNCTION_TEMPLATE, # type: ignore
            clang.cindex.CursorKind.CLASS_TEMPLATE_PARTIAL_SPECIALIZATION # type: ignore
        )
    )

def is_public_enum(cursor):
    return (
        cursor.kind == clang.cindex.CursorKind.ENUM_DECL # type: ignore
        and cursor.is_definition()
        and cursor.access_specifier in (
            clang.cindex.AccessSpecifier.PUBLIC, # type: ignore
            clang.cindex.AccessSpecifier.INVALID  # type: ignore # Top-level enums
        )
        and is_project_header(cursor)  # Only include project enums
    )

def get_pybind_function(cursor):
    # Use py::overload_cast for overloaded functions
    params = [a.type.spelling for a in cursor.get_arguments()]
    if params:
        args = ", ".join(params)
        return f'    m.def("{cursor.spelling}", py::overload_cast<{args}>(&{cursor.spelling}));'
    else:
        return f'    m.def("{cursor.spelling}", py::overload_cast<>(&{cursor.spelling}));'

def get_pybind_method(cursor, class_name):
    params = [a.type.spelling for a in cursor.get_arguments()]
    if params:
        args = ", ".join(params)
        return f'        .def("{cursor.spelling}", py::overload_cast<{args}>(&{class_name}::{cursor.spelling}))'
    else:
        return f'        .def("{cursor.spelling}", py::overload_cast<>(&{class_name}::{cursor.spelling}))'

def get_pybind_constructor(cursor, class_name):
    # Support any kind of constructor, select by parameter count
    params = list(cursor.get_arguments())
    param_types = [a.type.spelling for a in params]
    param_count = len(param_types)
    if param_count == 0:
        return f'        .def(py::init<>())'
    else:
        args = ", ".join(param_types)
        # Use py::init<...>() and a lambda to select by parameter count
        # The lambda is only needed if you want to disambiguate, but py::init<...>() is enough for pybind11
        return f'        .def(py::init<{args}>())  // {param_count} params'

def get_function_signature(cursor):
    params = tuple(a.type.spelling for a in cursor.get_arguments())
    return (cursor.spelling, params)

def get_method_signature(cursor):
    params = tuple(a.type.spelling for a in cursor.get_arguments())
    return (cursor.spelling, params)

def get_pybind_class(cursor):
    class_name = cursor.spelling
    lines = [f'    py::class_<{class_name}>(m, "{class_name}")']
    ctors = []
    methods = []
    enums = []
    seen_methods = set()
    for m in cursor.get_children():
        if is_template(m):
            continue
        if is_public_constructor(m):
            sig = get_method_signature(m)
            if sig not in seen_methods:
                ctors.append(get_pybind_constructor(m, class_name))
                seen_methods.add(sig)
        elif is_public_method(m):
            sig = get_method_signature(m)
            if sig not in seen_methods:
                methods.append(get_pybind_method(m, class_name))
                seen_methods.add(sig)
        elif is_public_enum(m):
            enums.append(get_pybind_enum(m, class_name))  # Pass class_name for nested enums
    if not ctors:
        return None  # Skip emitting this class
    lines.extend(ctors)
    lines.extend(methods)
    lines.extend(enums)
    lines[-1] += ';'  # End the chain
    return "\n".join(lines)

def get_pybind_enum(cursor, parent_class=None):
    enum_name = cursor.spelling
    if parent_class:
        lines = [f'        .def(py::enum_<{parent_class}::{enum_name}>(m, "{enum_name}")']
        for c in cursor.get_children():
            if c.kind == clang.cindex.CursorKind.ENUM_CONSTANT_DECL: # type: ignore
                lines.append(f'            .value("{c.spelling}", {parent_class}::{enum_name}::{c.spelling})')
        lines[-1] += '.export_values())'
        return "\n".join(lines)
    else:
        lines = [f'    py::enum_<{enum_name}>(m, "{enum_name}")']
        for c in cursor.get_children():
            if c.kind == clang.cindex.CursorKind.ENUM_CONSTANT_DECL: # type: ignore
                lines.append(f'        .value("{c.spelling}", {enum_name}::{c.spelling})')
        lines[-1] += '.export_values();'
        return "\n".join(lines)

def visit(cursor, seen_functions=None):
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
            elif is_public_enum(c) and c.semantic_parent.kind != clang.cindex.CursorKind.CLASS_DECL:
                enums.append(get_pybind_enum(c))
            # Recurse into all children to find nested enums/classes/functions
            child_functions, child_classes, child_enums = visit(c, seen_functions)
            functions.extend(child_functions)
            classes.extend(child_classes)
            enums.extend(child_enums)
        except Exception as e:
            continue
    return functions, classes, enums

def main():
    print('#include <pybind11/pybind11.h>')
    print('namespace py = pybind11;')
    print('\nPYBIND11_MODULE(hatchling, m) {')
    functions, classes, enums = visit(tu.cursor)
    # Print enums first so they are available for classes/functions
    for e in enums:
        print(e)
    for c in classes:
        print(c)
    for f in functions:
        print(f)
    print('}')

main()
