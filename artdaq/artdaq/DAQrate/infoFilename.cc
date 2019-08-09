#include "artdaq/DAQrate/infoFilename.hh"

#include <sstream>
#include <iomanip>

std::string
artdaq::infoFilename(std::string const& prefix, int rank, int run)
{
	std::ostringstream ost;
	ost
		<< prefix
		<< std::setfill('0')
		<< std::setw(4)
		<< run
		<< "_"
		<< std::setfill('0')
		<< std::setw(4)
		<< rank
		<< ".txt";
	return ost.str();
}
