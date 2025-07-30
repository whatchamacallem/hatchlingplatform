import gdb
import gdb.printing

class HxArrayPrinter:
    """Pretty printer for hxarray structure"""

    def __init__(self, val):
        self.val = val
        # Get the data pointers and capacity
        self.data = val['m_data']
        self.end = val['m_end']
        self.capacity = int(val['m_capacity'])

        # Infer the element type by dereferencing the data pointer
        # (this handles both typed and void pointers)
        if self.data and self.data != 0:
            self.type = self.data.dereference().type
        else:
            self.type = gdb.lookup_type('int')  # default fallback

    def to_string(self):
        size = int(self.end - self.data) if self.data != self.end else 0
        return "hxarray<{}> of size {}/{}".format(
            self.type, size, self.capacity)

    def children(self):
        m_begin = self.val['m_begin']
        m_end = self.val['m_end']

        if not m_begin or int(m_end) < int(m_begin): # Check for null or invalid range
            return # No children to yield

        elem_type = self.val.type.template_argument(0)
        num_elements = (int(m_end) - int(m_begin)) // elem_type.sizeof

        for i in range(num_elements):
            element_ptr = m_begin + i * elem_type.sizeof
            # Consider adding a try-except block here for robust error handling
            try:
                element_val = element_ptr.dereference()
                yield f"[{i}]", element_val
            except gdb.MemoryError:
                # Handle cases where memory cannot be read (e.g., out of bounds)
                yield f"[{i}]", "<unreadable memory>"
                break # Stop iterating if we hit unreadable memory

    def display_hint(self):
        return "array"

def build_pretty_printer():
    pp = gdb.printing.RegexpCollectionPrettyPrinter("hxarray_printer")
    # Match both hxarray and hxallocator types
    pp.add_printer('hxarray', '^hxarray$', HxArrayPrinter)
    pp.add_printer('hxallocator', '^hxallocator$', HxArrayPrinter)
    return pp

gdb.printing.register_pretty_printer(
    gdb.current_objfile(),
    build_pretty_printer(),
    replace=True)
