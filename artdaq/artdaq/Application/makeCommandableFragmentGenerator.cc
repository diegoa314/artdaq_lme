#include "artdaq/Application/makeCommandableFragmentGenerator.hh"

#include "artdaq/Application/GeneratorMacros.hh"
#include "fhiclcpp/ParameterSet.h"
#include "cetlib/BasicPluginFactory.h"

std::unique_ptr<artdaq::CommandableFragmentGenerator>
artdaq::makeCommandableFragmentGenerator(std::string const& generator_plugin_spec,
										 fhicl::ParameterSet const& ps)
{
	static cet::BasicPluginFactory bpf("generator", "make");

	return bpf.makePlugin<std::unique_ptr<artdaq::CommandableFragmentGenerator>,
						  fhicl::ParameterSet const &>(generator_plugin_spec, ps);
}
