import time
import logging
import inspect
import functools
from typing import get_type_hints

# Allow other modules to configure logging coming from here.
logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)

VERBOSE = 1

# Cache signatures to avoid repeated computation
@functools.lru_cache(maxsize=128)
def get_cached_signature(func):
    return inspect.signature(func)

def debug_decorator(func):
    """Decorator to validate argument types and log execution time."""
    if not __debug__:
        return func

    @functools.wraps(func)
    def wrapper(*args, **kwargs):
        # Use cached signature for performance
        sig = get_cached_signature(func)
        try:
            bound_args = sig.bind(*args, **kwargs)
        except TypeError as e:
            raise TypeError(f"invalid_argument {func.__name__}: {e}")

        # Use get_type_hints for robust type checking
        type_hints = get_type_hints(func)
        for name, value in bound_args.arguments.items():
            if name in type_hints:
                expected_type = type_hints[name]
                if not isinstance(value, expected_type):
                    raise TypeError(
                        f"invalid_argument '{name}'='{value}' type {type(value).__name__} "
                        f"cannot be converted to {expected_type.__name__}"
                    )

        start_time = time.perf_counter() if VERBOSE >= 1 else 0.0

        result = func(*bound_args.args, **bound_args.kwargs)

        if VERBOSE >= 1:
            end_time = time.perf_counter()
            logger.info(f"{func.__name__} {((end_time - start_time) * 1000):.4f}ms")

        return result

    return wrapper
