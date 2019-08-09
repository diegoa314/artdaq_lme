//
// Based off of "ds50driver", a program for testing DarkSide-50 input
// sources, "driver" is an executable designed to make it simple to
// test a fragment generator under development

// Run 'driver --help' to get a description of the
// expected command-line parameters.
//
//
// The current version generates data fragments and passes them
// to an EventStore instance and then to art, if desired.
//

#include "art/Framework/Art/artapp.h"
#include "artdaq-core/Data/Fragment.hh"
#include "artdaq/Application/CommandableFragmentGenerator.hh"
#include "artdaq/Application/makeCommandableFragmentGenerator.hh"
#include "artdaq/DAQrate/EventStore.hh"
#include "artdaq-core/Core/SimpleMemoryReader.hh"
#include "artdaq-core/Utilities/SimpleLookupPolicy.hh"
#include "cetlib/container_algorithms.h"
#include "cetlib/filepath_maker.h"
#include "fhiclcpp/ParameterSet.h"
#include "fhiclcpp/make_ParameterSet.h"

#include <boost/program_options.hpp>

#include <signal.h>
#include <iostream>
#include <memory>
#include <utility>
#include <limits>

using namespace fhicl;
namespace bpo = boost::program_options;


int main(int argc, char* argv[]) try
{
	// 15-Dec-2016, KAB: by consensus, we want people to use the "artdaqDriver"
	// program (which corresponds to "artdaq/proto/driver.cc").
	if (true)
	{
		std::cout << "****************************************" << std::endl;
		std::cout << "*** \"demo_driver\" has been deprecated (15-Dec-2016)." << std::endl;
		std::cout << "*** Please use the \"artdaqDriver\" application that is available from" << std::endl
			<< "*** the artdaq software package instead of \"demo_driver\"." << std::endl;
		exit(1);
	}

	// Get the input parameters via the boost::program_options library,
	// designed to make it relatively simple to define arguments and
	// issue errors if argument list is supplied incorrectly

	std::ostringstream descstr;
	descstr << argv[0] << " <-c <config-file>> <other-options>";

	bpo::options_description desc = descstr.str();

	desc.add_options()
		("config,c", bpo::value<std::string>(), "Configuration file.")
		("help,h", "produce help message");

	bpo::variables_map vm;

	try
	{
		bpo::store(bpo::command_line_parser(argc, argv).options(desc).run(), vm);
		bpo::notify(vm);
	}
	catch (bpo::error const& e)
	{
		std::cerr << "Exception from command line processing in " << argv[0]
			<< ": " << e.what() << "\n";
		return -1;
	}

	if (vm.count("help"))
	{
		std::cout << desc << std::endl;
		return 1;
	}
	if (!vm.count("config"))
	{
		std::cerr << "Exception from command line processing in " << argv[0]
			<< ": no configuration file given.\n"
			<< "For usage and an options list, please do '"
			<< argv[0] << " --help"
			<< "'.\n";
		return 2;
	}

	// Check the directories defined by the FHICL_FILE_PATH
	// environmental variable for the *.fcl file whose name was passed to
	// the command line. If not defined, look in the current directory.

	ParameterSet complete_pset;

	if (getenv("FHICL_FILE_PATH") == nullptr)
	{
		std::cerr
			<< "INFO: environment variable FHICL_FILE_PATH was not set. Using \".\"\n";
		setenv("FHICL_FILE_PATH", ".", 0);
	}

	artdaq::SimpleLookupPolicy lookup_policy("FHICL_FILE_PATH");

	make_ParameterSet(vm["config"].as<std::string>(), lookup_policy, complete_pset);

	ParameterSet fragment_receiver_pset = complete_pset.get<ParameterSet>("fragment_receiver");
	ParameterSet event_builder_pset = complete_pset.get<ParameterSet>("event_builder");

	// Use the "generator" parameter from the user-supplied *.fcl
	// configuration file to define our fragment generator

	std::unique_ptr<artdaq::CommandableFragmentGenerator> const
		gen(artdaq::makeCommandableFragmentGenerator(fragment_receiver_pset.get<std::string>("generator"),
		                                             fragment_receiver_pset));

	// The instance of the artdaq::EventStore object can either pass
	// events to a thread running Art, or to a small executable called
	// "SimpleMemoryReader"

	bool const want_artapp = event_builder_pset.get<bool>("use_art", false);

	std::ostringstream os;
	if (!want_artapp)
	{
		os << event_builder_pset.get<int>("events_expected_in_SimpleMemoryReader");
	}
	std::string oss = os.str();

	const char* args[2]{"SimpleMemoryReader", const_cast<char *>(oss.c_str())};

	int es_argc(want_artapp ? argc : 2);
	char** es_argv(want_artapp ? argv : const_cast<char**>(args));

	artdaq::EventStore::ART_CMDLINE_FCN*
		es_fcn(want_artapp ? &artapp : &artdaq::SimpleMemoryReaderApp);

	artdaq::EventStore store(event_builder_pset, event_builder_pset.get<size_t>("expected_fragments_per_event"),
	                         complete_pset.get<artdaq::EventStore::run_id_t>("run_number"),
	                         es_argc,
	                         es_argv,
	                         es_fcn);

	int events_to_generate = complete_pset.get<int>("events_to_generate", 0);
	int event_count = 0;
	artdaq::Fragment::sequence_id_t previous_sequence_id = 0;

	uint64_t timeout = 45;
	uint64_t timestamp = std::numeric_limits<uint64_t>::max();

	gen.get()->StartCmd(complete_pset.get<artdaq::EventStore::run_id_t>("run_number"), timeout, timestamp);

	artdaq::FragmentPtrs frags;

	while (events_to_generate >= 0 && gen->getNext(frags))
	{
		for (auto& val : frags)
		{
			std::cout << "Fragment: Seq ID: " << val->sequenceID() << ", Frag ID: " << val->fragmentID() << ", total size in bytes: " << val->size() * sizeof(artdaq::RawDataType) << std::endl;

			if (val->sequenceID() != previous_sequence_id)
			{
				++event_count;
				previous_sequence_id = val->sequenceID();
			}
			if (events_to_generate != 0 && event_count > events_to_generate)
				gen.get()->StopCmd(timeout, timestamp);

			store.insert(std::move(val));
		}
		frags.clear();

		if (events_to_generate != 0 && event_count >= events_to_generate)
			gen.get()->StopCmd(timeout, timestamp);
	}

	int readerReturnValue;
	bool endSucceeded = false;
	int attemptsToEnd = 1;
	endSucceeded = store.endOfData(readerReturnValue);
	while (! endSucceeded && attemptsToEnd < 3)
	{
		++attemptsToEnd;
		endSucceeded = store.endOfData(readerReturnValue);
	}
	if (! endSucceeded)
	{
		std::cerr << "Failed to shut down the reader and the event store "
			<< "because the endOfData marker could not be pushed "
			<< "onto the queue." << std::endl;
	}
	return readerReturnValue;
}

catch (std::string& x)
{
	std::cerr << "Exception (type string) caught in driver: " << x << "\n";
	return 1;
}

catch (char const* m)
{
	std::cerr << "Exception (type char const*) caught in driver: " << std::endl;
	if (m)
	{
		std::cerr << m;
	}
	else
	{
		std::cerr << "[the value was a null pointer, so no message is available]";
	}
	std::cerr << '\n';
}
