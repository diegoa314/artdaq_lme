#include "artdaq/ArtModules/BuildInfo_module.hh"

#include "artdaq/BuildInfo/GetPackageBuildInfo.hh"
#include "artdaq-core/BuildInfo/GetPackageBuildInfo.hh"
#include "artdaq-core-demo/BuildInfo/GetPackageBuildInfo.hh"
#include "artdaq-demo/BuildInfo/GetPackageBuildInfo.hh"

#include <string>

namespace demo
{
	/**
	 * \brief Instance name for the artdaq_demo version of BuildInfo module
	 */
	static std::string instanceName = "ArtdaqDemoBuildInfo";
	/**
	 * \brief ArtdaqDemoBuildInfo is a BuildInfo type containing information about artdaq_core, artdaq, artdaq_core_demo and artdaq_demo builds.
	 */
	typedef artdaq::BuildInfo<&instanceName, artdaqcore::GetPackageBuildInfo, artdaq::GetPackageBuildInfo, coredemo::GetPackageBuildInfo, demo::GetPackageBuildInfo> ArtdaqDemoBuildInfo;

	DEFINE_ART_MODULE(ArtdaqDemoBuildInfo)
}
