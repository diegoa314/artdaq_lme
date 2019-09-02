#include "proto/artdaqapp.hh"
#include "artdaq/Application/LoadParameterSet.hh"

int main(int argc, char* argv[])
{
	fhicl::ParameterSet config_ps = LoadParameterSet<artdaq::artdaqapp::Config>(argc, argv,"routing_master", "This is the artdaq Routing Master application\nThe Routing Master receives tokens from the receivers, builds Routing Tables using those tokens and a Routing Policy plugin, then sends the routing tables to the senders.");

	artdaq::detail::TaskType task = artdaq::detail::TaskType::RoutingMasterTask;

	artdaq::artdaqapp::runArtdaqApp(task, config_ps);
}