import gdb
import gdb.printing

class HxArrayPrinter:
    """Pretty printer for hxarray structure"""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        # Get size (difference between end pointer and data pointer)
        size = int(self.val['m_end_'] - self.val['m_data_'])

        # Get capacity - depends on whether we're using fixed or dynamic allocation
        if self.val.type.template_argument(1) == 0:
            capacity = int(self.val['m_capacity_'])
        else:
            capacity = self.val.type.template_argument(1)

        # Get element type
        elem_type = self.val.type.template_argument(0)

        return "{} / {} <{}>".format(size, capacity, elem_type)

def build_pretty_printer():
    pp = gdb.printing.RegexpCollectionPrettyPrinter("hxarray_printer")
    pp.add_printer('hxarray', '^hxarray<.*>$', HxArrayPrinter)
    return pp

gdb.printing.register_pretty_printer(gdb.current_objfile(), build_pretty_printer())
