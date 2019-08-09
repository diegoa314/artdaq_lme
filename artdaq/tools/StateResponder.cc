#include "artdaq/Application/Commandable.hh"
#include "artdaq-core/Utilities/configureMessageFacility.hh"
#include "artdaq/DAQdata/Globals.hh"
#include "artdaq/ExternalComms/MakeCommanderPlugin.hh"
#include "artdaq/Application/LoadParameterSet.hh"

#include <boost/program_options.hpp>
#include <boost/lexical_cast.hpp>

#include <iostream>

int main(int argc, char* argv[])
{
	artdaq::configureMessageFacility("commandable");


	fhicl::ParameterSet config = LoadParameterSet<artdaq::CommanderInterface::Config>(argc, argv, "stateResponder", "This simple application sets up a CommanderInterface plugin and reports any received commands.");


	artdaq::setMsgFacAppName("Commandable", config.get<int>("id"));

	// create the Commandable object
	artdaq::Commandable commandable;
	
	auto commander = artdaq::MakeCommanderPlugin(config, commandable);
	commander->run_server();
}
