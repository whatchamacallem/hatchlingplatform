# example.py
# This Python module imports and uses the C++ extension.

from example_cpp_ext import world_from_cpp

def greet(number):
    """
    Greets the world and calls the C++ extension with a given number.
    """
    print("Hello from Python!")
    # Call the C++ function and print its result
    cpp_message = world_from_cpp(number)
    print(cpp_message)

if __name__ == "__main__":
    # Example usage of the greet function
    print("--- Testing with 25.0 ---")
    greet(25.0)
    print("\n--- Testing with 100.0 ---")
    greet(100.0)
