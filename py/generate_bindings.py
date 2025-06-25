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

def get_function_decl(cursor):
    params = [f"{a.type.spelling} {a.spelling}" for a in cursor.get_arguments()]
    return f"{cursor.result_type.spelling} {cursor.spelling}({', '.join(params)});"

def get_method_decl(cursor):
    params = [f"{a.type.spelling} {a.spelling}" for a in cursor.get_arguments()]
    const = " const" if cursor.is_const_method() else ""
    return f"  {cursor.result_type.spelling} {cursor.spelling}({', '.join(params)}){const};"

def get_constructor_decl(cursor):
    params = [f"{a.type.spelling} {a.spelling}" for a in cursor.get_arguments()]
    return f"  {cursor.spelling}({', '.join(params)});"

def get_destructor_decl(cursor):
    return f"  ~{cursor.semantic_parent.spelling}();"

def get_class_decl(cursor, parent=None):
    if is_template(cursor):
        return ""  # Skip template classes
    name = f"{parent}::{cursor.spelling}" if parent else cursor.spelling
    decl = [f"class {name} {{"]
    for m in cursor.get_children():
        if is_template(m):
            continue  # Skip template methods/constructors/destructors/classes
        if is_public_constructor(m):
            decl.append(get_constructor_decl(m))
        elif is_public_destructor(m):
            decl.append(get_destructor_decl(m))
        elif is_public_method(m):
            decl.append(get_method_decl(m))
        elif is_public_class(m):
            inner = get_class_decl(m, name)
            if inner:
                decl.append(inner)
    decl.append("};")
    return "\n".join(decl)

def visit(cursor, parent=None):
    for c in cursor.get_children():
        if is_template(c):
            continue  # Skip template functions/classes
        if is_public_function(c):
            print(get_function_decl(c))
        elif is_public_class(c):
            decl = get_class_decl(c, parent)
            if decl:
                print(decl)

visit(tu.cursor)
print("🐉🐉🐉")
