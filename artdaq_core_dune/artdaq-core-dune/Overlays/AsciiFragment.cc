#include "artdaq-core-dune/Overlays/AsciiFragment.hh"

#if 0
namespace {
  unsigned int pop_count (unsigned int n) {
	unsigned int c; 
	for (c = 0; n; c++) n &= n - 1; 
	return c;
  }
}
#endif

std::ostream& demo::operator <<(std::ostream& os, AsciiFragment const& f)
{
	os << "AsciiFragment event size: "
		<< f.hdr_event_size()
		<< ", line number: "
		<< f.hdr_line_number()
		<< "\n";

	return os;
}
