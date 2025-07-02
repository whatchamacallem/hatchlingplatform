// example.cpp
// This C++ file defines a Python extension module.
// It includes a function that performs a math operation (sqrt)
// and demonstrates linking against the math library.

#include <Python.h> // Required for Python C API
#include <cmath>    // For std::sqrt function
#include <cstdio>   // For snprintf function

// Define the C++ function that will be exposed to Python.
// It takes a Python object (args) and returns a Python object.
static PyObject* example_world_from_cpp(PyObject* self, PyObject* args) {
    double input_number;

    // Parse the input arguments. "d" expects a double.
    // If parsing fails (e.g., non-numeric input), PyArg_ParseTuple returns 0 and sets a Python error.
    if (!PyArg_ParseTuple(args, "d", &input_number)) {
        return NULL; // Return NULL to indicate an error to Python
    }

    // Perform a mathematical operation: calculate the square root.
    // This demonstrates linking against the math library.
    double result = std::sqrt(input_number);

    // Format the result into a C-style string buffer.
    // snprintf is used for safety to prevent buffer overflows.
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "Hello from C++! sqrt(%f) = %f", input_number, result);

    // Convert the C string to a Python Unicode object and return it.
    // PyUnicode_FromString handles memory allocation for the Python string.
    return PyUnicode_FromString(buffer);
}

// Define the methods that this C++ module will expose to Python.
// Each entry is a PyMethodDef struct.
static PyMethodDef HelloMethods[] = {
    // { "Python_name", C_function_pointer, argument_parsing_flag, "docstring" }
    {"world_from_cpp", example_world_from_cpp, METH_VARARGS, "Returns a greeting from C++ with a math calculation."},
    {NULL, NULL, 0, NULL} // Sentinel: Marks the end of the methods array. Required.
};

// Define the Python module itself.
// This struct contains metadata about the module.
static struct PyModuleDef examplemodule = {
    PyModuleDef_HEAD_INIT, // Required for all module definitions.
    "example_cpp_ext",       // Name of the module as it will be imported in Python (e.g., ).
    "A simple C++ extension module demonstrating math library linking.", // Module documentation string.
    -1,                    // Size of per-interpreter state of the module, or -1 if the module keeps state in global variables.
    HelloMethods           // Pointer to the array of methods defined above.
};

// Module initialization function.
// This function is called by Python when the module is imported.
// The name must be PyInit_<module_name> where <module_name> is the name used in PyModuleDef.
PyMODINIT_FUNC PyInit_example_cpp_ext(void) {
    // Create and return the module object.
    return PyModule_Create(&examplemodule);
}
