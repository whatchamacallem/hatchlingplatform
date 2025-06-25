import sys, os

import clang.cindex

clang.cindex.Config.set_library_file("/usr/lib/llvm-18/lib/libclang.so.1")

index = clang.cindex.Index.create()

translation_unit = index.parse("include/hx/hatchling_pch.hpp", args=["-std=c++17",
	"-DHX_RELEASE=0", "-fdiagnostics-absolute-paths", "-Iinclude"])

def is_public_function(cursor):
    try:
        return (
            cursor.kind == clang.cindex.CursorKind.FUNCTION_DECL
            and cursor.linkage == clang.cindex.LinkageKind.EXTERNAL
            and not cursor.spelling.startswith("hxx_")  # skip internal
        )
    except Exception:
        # Skip cursors with unknown kind
        return False

def get_function_decl(cursor):
    result_type = cursor.result_type.spelling
    name = cursor.spelling
    params = []
    for arg in cursor.get_arguments():
        params.append(f"{arg.type.spelling} {arg.spelling}")
    param_str = ", ".join(params)
    return f"{result_type} {name}({param_str});"


def visit(cursor):
    for child in cursor.get_children():
        if is_public_function(child):
            print(get_function_decl(child))
        visit(child)


def print_all_visible(cursor, depth=0):
    indent = "  " * depth
    print(f"{indent}{cursor.kind} {cursor.spelling} ({cursor.displayname})")
    for child in cursor.get_children():
        print_all_visible(child, depth + 1)


visit(translation_unit.cursor)

print("🐉🐉🐉")
