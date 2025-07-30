import gdb # type: ignore
import gdb.printing # type: ignore
import traceback

#
# hxarray uses this allocation strategy:
#
# template<typename T_, size_t fixed_capacity_>
# class hxallocator {
# 	T_ m_data_[fixed_capacity_];
# };
# template<typename T_>
# class hxallocator<T_, 0> {
# 	m_capacity_;
# 	T_* m_data_;
# };
# template<typename T_, size_t capacity_=0>
# class hxarray : public hxallocator<T_, capacity_> {
# 	T_* m_end_;
# }
#

class HxArrayPrinter:
    """Pretty printer for hxarray structure"""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            m_data = self.val['m_data_']
            m_end = self.val['m_end_']

            if m_data.is_optimized_out or m_end.is_optimized_out:
                return "<optimized out>"

            if self.val.type.template_argument(1) == 0:
                capacity = int(self.val['m_capacity_'])
            else:
                capacity = self.val.type.template_argument(1)
            if capacity == 0:
                return "<unallocated>"

            # There are two different underlying implementations and this logic
            # works for both of them. This is Python, so we have int instead of
            # uintptr_t to calculate addresses with.
            if self.val.type.template_argument(1) != 0:
                m_data = int(m_data.address)
            else:
                m_data = int(m_data)
            m_end = int(m_end)

            elem_type = self.val.type.template_argument(0)
            size = int((m_end - m_data) / elem_type.sizeof)

            return "[{}] /{} <{}>".format(size, capacity, elem_type)
        except Exception as e:
            return f"{traceback.format_exc()}"

def build_pretty_printer():
    pp = gdb.printing.RegexpCollectionPrettyPrinter("hxarray_printer")
    # Match both hxarray<T_, 0> and hxarray<T_, N> patterns
    pp.add_printer('hxarray', '^hxarray<.*,.*>$', HxArrayPrinter)
    return pp

gdb.printing.register_pretty_printer(gdb.current_objfile(), build_pretty_printer())
