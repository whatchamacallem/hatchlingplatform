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
	Pretty printer for hxarray<T, capacity>. There are two different underlying
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
			if capacity < 0:
				return "<negative capacity>"

			# There are two different underlying implementations and this logic
			# works for both of them. This is Python, so we have int instead of
			# uintptr_t to calculate addresses with.
			if self.val.type.template_argument(1) != 0:
				data = int(data.address)
			else:
				data = int(data)
			end = int(end)

			elem_type = self.val.type.template_argument(0)
			size = int((end - data) / elem_type.sizeof)
			if size < 0:
				return "<negative size>"

			# Cache these for calculating children.
			self._elem_type = elem_type
			self._size = size
			self._data = data

			# Format single line description.

			basename = f'{self._elem_type}'.split(':')[-1]
			return "[{}/{}] {}".format(self._size, capacity, basename)
		except Exception as e:
			error = f"{traceback.format_exc()}"
			return error.split('\n', 1)[1]

	def raw_view(self):
		if self.val.type.template_argument(1) == 0:
			yield ('m_data_', self.val['m_data_'])
			yield ('m_capacity_', self.val['m_capacity_'])
		yield ('m_end_', self.val['m_end_'])

	def array_view(self):
		# Use integer byte calculations.
		for i in range(self._size):
			int_ptr = self._data + i * self._elem_type.sizeof
			elem_ptr = gdb.Value(int_ptr).cast(self._elem_type.pointer())
			yield (f"[{i}] {hex(int_ptr)}", elem_ptr.dereference())

	def children(self):
		try:
			# Check if the array was found in to_string.
			if hasattr(self, '_size'):
				if self._size > 10:
					# Move the raw view to the top of arrays over length 10.
					# This is to keep it from getting buried completely.
					yield from self.raw_view()
					yield from self.array_view()
				else:
					yield from self.array_view()
					yield from self.raw_view()
			else:
				yield from self.raw_view()

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
