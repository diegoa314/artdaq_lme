#include "artdaq/ArtModules/BuildInfo_module.hh"

#include "artdaq/BuildInfo/GetPackageBuildInfo.hh"
#include "artdaq-core/BuildInfo/GetPackageBuildInfo.hh"
#include "artdaq-core-dune/BuildInfo/GetPackageBuildInfo.hh"
#include "artdaq-dune/BuildInfo/GetPackageBuildInfo.hh"

#include <string>

namespace demo
{
	/**
	 * \brief Instance name for the artdaq_demo version of BuildInfo module
	 */
	static std::string instanceName = "ArtdaqdemoBuildInfo";
	/**
	 * \brief ArtdaqDemoBuildInfo is a BuildInfo type containing information about artdaq_core, artdaq, artdaq_core_demo and artdaq_demo builds.
	 */
	typedef artdaq::BuildInfo<&instanceName, artdaqcore::GetPackageBuildInfo, artdaq::GetPackageBuildInfo, coredemo::GetPackageBuildInfo, demo::GetPackageBuildInfo> ArtdaqdemoBuildInfo;

	DEFINE_ART_MODULE(ArtdaqdemoBuildInfo)
}
