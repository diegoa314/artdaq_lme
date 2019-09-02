#include "proto/artdaqapp.hh"
#include "artdaq/Application/LoadParameterSet.hh"

int main(int argc, char* argv[])
{
	fhicl::ParameterSet config_ps = LoadParameterSet<artdaq::artdaqapp::Config>(argc, argv, "boardreader", "The BoardReader is responsible for reading out one or more pieces of connected hardware, creating Fragments and sending them to the EventBuilders.");
	artdaq::detail::TaskType task = artdaq::detail::TaskType::BoardReaderTask;

	artdaq::artdaqapp::runArtdaqApp(task, config_ps);
}