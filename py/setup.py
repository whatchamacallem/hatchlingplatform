# setup.py
# This script is used by setuptools to build the Python package, including
# compiling the C++ extension.

from setuptools import setup, Extension

# Define the C++ extension module.
example_cpp_ext_module = Extension(
    'example_cpp_ext', # The name of the module in Python.
    sources=['example.cpp'],
    libraries=['m'],
    extra_compile_args=['-std=c++17']
)

setup(
    name='example-package',
    version='0.1.0',
    author='Your Name',
    author_email='your.email@example.com',
    description='Package description.',
    long_description=open('README.md').read(),
    long_description_content_type='text/markdown',
    url='https://github.com/yourusername/example-package',
    py_modules=['example_test'], # list of .py files
    ext_modules=[example_cpp_ext_module],
    install_requires=[], # runtime dependencies
    classifiers=[
#        "License :: OSI Approved :: MIT License",
        "Programming Language :: Python :: 3",
        "Operating System :: OS Independent",
    ],
    python_requires='>=3.7'
)
