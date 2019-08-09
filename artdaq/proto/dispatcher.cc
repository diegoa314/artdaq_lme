#include "proto/artdaqapp.hh"
#include "artdaq/Application/LoadParameterSet.hh"

int main(int argc, char* argv[])
{
	fhicl::ParameterSet config_ps = LoadParameterSet<artdaq::artdaqapp::Config>(argc, argv, "dispatcher", "The Dispatcher process recevies events from a DataLogger or EventBuilder art process and makes them available to online monitors. Filters may be used by each online monitor to reduce network traffic.");
	artdaq::detail::TaskType task = artdaq::detail::TaskType::DispatcherTask;

	artdaq::artdaqapp::runArtdaqApp(task, config_ps);
}