#include "proto/artdaqapp.hh"
#include "artdaq/Application/LoadParameterSet.hh"

int main(int argc, char* argv[])
{
	fhicl::ParameterSet config_ps = LoadParameterSet<artdaq::artdaqapp::Config>(argc, argv, "eventbuilder", "The EventBuilder application receives Fragments from BoardReaders, concatenating them to form RawEvents. RawEvents are processed by an art process which typically runs first-level filtering, then it sends events on to the DataLogger");
	artdaq::detail::TaskType task = artdaq::detail::TaskType::EventBuilderTask;

	artdaq::artdaqapp::runArtdaqApp(task, config_ps);
}