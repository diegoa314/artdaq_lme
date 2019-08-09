#ifndef artdaq_demo_BuildInfo_GetPackageBuildInfo_hh
#define artdaq_demo_BuildInfo_GetPackageBuildInfo_hh

#include "artdaq-core/Data/PackageBuildInfo.hh"

#include <string>

/**
* \brief Namespace used to differentiate the artdaq_demo version of GetPackageBuildInfo
* from other versions present in the system.
*/
namespace demo
{
	/**
	* \brief Wrapper around the demo::GetPackageBuildInfo::getPackageBuildInfo function
	*/
	struct GetPackageBuildInfo
	{
		/**
		* \brief Gets the version number and build timestmap for artdaq_demo
		* \return An artdaq::PackageBuildInfo object containing the version number and build timestamp for artdaq_demo
		*/
		static artdaq::PackageBuildInfo getPackageBuildInfo();
	};
}

#endif /* artdaq_demo_BuildInfo_GetPackageBuildInfo_hh */
