#include "artdaq-core-dune/Overlays/PruebaFragmento.hh"

#include "cetlib_except/exception.h"

#if 0
namespace {
  unsigned int pop_count (unsigned int n) {
    unsigned int c; 
    for (c = 0; n; c++) n &= n - 1; 
    return c;
  }
}
#endif

std::ostream& prueba::operator <<(std::ostream& os, PruebaFragmento const& f)
{
	os << "PruebaFragmento event size: "
		<< f.hdr_event_size()
		<< "\n";

	return os;
}
