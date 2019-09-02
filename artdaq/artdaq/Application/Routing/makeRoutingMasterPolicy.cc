#include "artdaq/Application/Routing/makeRoutingMasterPolicy.hh"

#include "artdaq/Application/Routing/PolicyMacros.hh"
#include "fhiclcpp/ParameterSet.h"
#include "cetlib/BasicPluginFactory.h"

std::unique_ptr<artdaq::RoutingMasterPolicy>
artdaq::makeRoutingMasterPolicy(std::string const& policy_plugin_spec,
										 fhicl::ParameterSet const& ps)
{
	static cet::BasicPluginFactory bpf("policy", "make");

	return bpf.makePlugin<std::unique_ptr<artdaq::RoutingMasterPolicy>,
		fhicl::ParameterSet const &>(policy_plugin_spec, ps);
}
