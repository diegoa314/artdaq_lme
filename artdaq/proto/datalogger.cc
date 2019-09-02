#include "proto/artdaqapp.hh"
#include "artdaq/Application/LoadParameterSet.hh"

int main(int argc, char* argv[])
{
	fhicl::ParameterSet config_ps = LoadParameterSet<artdaq::artdaqapp::Config>(argc, argv, "datalogger", "The DataLogger is primarily responsible for writing events to disk in its attached art process. Events may be bunched if necessary for system performance (very small events may benefit).");
	artdaq::detail::TaskType task = artdaq::detail::TaskType::DataLoggerTask;

	artdaq::artdaqapp::runArtdaqApp(task, config_ps);
}