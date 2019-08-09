#ifndef artdaq_DAQrate_infoFilename_hh
#define artdaq_DAQrate_infoFilename_hh

#include <string>

namespace artdaq
{
	/**
	 * \brief Generate a filename using the given parameters
	 * \param prefix Prefix for the file name
	 * \param rank Rank of the application
	 * \param run Run number
	 * \return prefix + run (4 digits) + "_" + rank (4 digits) + ".txt";
	 */
	std::string infoFilename(std::string const& prefix, int rank, int run);
}
#endif /* artdaq_DAQrate_infoFilename_hh */
