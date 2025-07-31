import gdb # type: ignore
import gdb.printing # type: ignore
import traceback

# hxarray uses this allocation strategy:
#
#	template<typename T_, size_t fixed_capacity_>
#	class hxallocator {
#		T_ m_data_[fixed_capacity_];
#	};
#	template<typename T_>
#	class hxallocator<T_, 0> {
#		int m_capacity_;
#		T_* m_data_;
#	};
#	template<typename T_, size_t capacity_=0>
#	class hxarray : public hxallocator<T_, capacity_> {
#		T_* m_end_;
#	}
#

class HxArrayPrinter:
    """
    Pretty printer for hxarray structure. There are two different underlying
    implementations and this logic works for both of them.
    """

    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            data = self.val['m_data_']
            end = self.val['m_end_']

            if data.is_optimized_out or end.is_optimized_out:
                return "<optimized out>"

            if self.val.type.template_argument(1) == 0:
                capacity = int(self.val['m_capacity_'])
            else:
                capacity = int(self.val.type.template_argument(1))
            if capacity == 0:
                return "<unallocated>"

            # There are two different underlying implementations and this logic
            # works for both of them. This is Python, so we have int instead of
            # uintptr_t to calculate addresses with.
            if self.val.type.template_argument(1) != 0:
                data = int(data.address)
            else:
                data = int(data)
            end = int(end)

			# Cache these for calculating children.
            self.elem_type = self.val.type.template_argument(0)
            self.size = int((end - data) / self.elem_type.sizeof)
            self.data = data

			# Format single line description.
            return "[{}] /{} <{}>".format(self.size, capacity, self.elem_type)
        except Exception as e:
            return f"{traceback.format_exc()}"

    def children(self):
        try:
            # Check if the array was found in to_string.
            if not hasattr(self, 'size'):
                return

			# Use integer address calculations.
            for i in range(self.size):
                int_ptr = self.data + i * self.elem_type.sizeof
                elem_ptr = gdb.Value(int_ptr).cast(self.elem_type.pointer())
                yield (f"[{i}]", elem_ptr.dereference())
        except Exception:
            return

    def display_hint(self):
        return "array"

def build_pretty_printer():
    pp = gdb.printing.RegexpCollectionPrettyPrinter("hxarray_printer")
    # Match both hxarray<T_, 0> and hxarray<T_, N> patterns
    pp.add_printer('hxarray', '^hxarray<.*,.*>$', HxArrayPrinter)
    return pp

gdb.printing.register_pretty_printer(gdb.current_objfile(), build_pretty_printer())
