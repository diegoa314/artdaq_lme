#ifndef artdaq_core_dune_BuildInfo_GetPackageBuildInfo_hh
#define artdaq_core_dune_BuildInfo_GetPackageBuildInfo_hh

#include "artdaq-core/Data/PackageBuildInfo.hh"

#include <string>

/**
* \brief Namespace used to differentiate the artdaq_core_demo version of GetPackageBuildInfo
* from other versions present in the system.
*/
namespace coredune
{
	/**
	* \brief Wrapper around the coredemo::GetPackageBuildInfo::getPackageBuildInfo function
	*/
	struct GetPackageBuildInfo
	{
		/**
		* \brief Gets the version number and build timestmap for artdaq_core_demo
		* \return An artdaq::PackageBuildInfo object containing the version number and build timestamp for artdaq_core_demo
		*/
		static artdaq::PackageBuildInfo getPackageBuildInfo();
	};
}

#endif /* artdaq_core_demo_BuildInfo_GetPackageBuildInfo_hh */
