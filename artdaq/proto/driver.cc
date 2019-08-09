//
// artdaqDriver is a program for testing the behavior of the generic
// RawInput source. Run 'artdaqDriver --help' to get a description of the
// expected command-line parameters.
//
//
// The current version generates simple data fragments, for testing
// that data are transmitted without corruption from the
// artdaq::Eventevent_manager through to the artdaq::RawInput source.
//
#define TRACE_NAME "artdaqDriver"

#include "art/Framework/Art/artapp.h"
#include "artdaq-core/Generators/FragmentGenerator.hh"
#include "artdaq-core/Data/Fragment.hh"
#include "artdaq-core/Utilities/ExceptionHandler.hh"
#include "artdaq/DAQdata/GenericFragmentSimulator.hh"

#include "artdaq/DAQdata/Globals.hh"
#include "artdaq-core/Generators/makeFragmentGenerator.hh"
#include "artdaq/Application/makeCommandableFragmentGenerator.hh"
#include "artdaq-utilities/Plugins/MetricManager.hh"
#include "artdaq-core/Core/SimpleMemoryReader.hh"
#include "cetlib/filepath_maker.h"
#include "fhiclcpp/ParameterSet.h"
#include "fhiclcpp/make_ParameterSet.h"
#include <boost/program_options.hpp>

#include <signal.h>
#include <iostream>
#include <memory>
#include <utility>
#include "artdaq/DAQrate/SharedMemoryEventManager.hh"
#include "artdaq/Application/LoadParameterSet.hh"

namespace  bpo = boost::program_options;

volatile int events_to_generate;
void sig_handler(int) { events_to_generate = -1; }

template<typename B, typename D>
std::unique_ptr<D>
dynamic_unique_ptr_cast(std::unique_ptr<B>& p);

int main(int argc, char * argv[]) try
{
	struct Config
	{
		fhicl::Atom<int> run_number{ fhicl::Name{"run_number"}, fhicl::Comment{"Run number to use for output file"}, 1 };
		fhicl::Atom<bool> debug_cout{ fhicl::Name{"debug_cout"}, fhicl::Comment{"Whether to print debug messages to console"}, false };
		fhicl::Atom<uint64_t> transition_timeout{ fhicl::Name{"transition_timeout"}, fhicl::Comment{"Timeout to use (in seconds) for automatic transitions"}, 30 };
		fhicl::Table<artdaq::CommandableFragmentGenerator::Config> generator{ fhicl::Name{ "fragment_receiver" } };
		fhicl::Table<artdaq::MetricManager::Config> metrics{ fhicl::Name{"metrics"} };
		fhicl::Table<artdaq::SharedMemoryEventManager::Config> event_builder{ fhicl::Name{"event_builder"} };
		fhicl::Atom<int> events_to_generate{ fhicl::Name{"events_to_generate"}, fhicl::Comment{"Number of events to generate and process"}, 0 };
		fhicl::TableFragment<art::Config> art_config;
	};
	auto pset = LoadParameterSet<Config>(argc, argv, "driver", "The artdaqDriver executable runs a Fragment Generator and an art process, acting as a \"unit integration\" test for a data source");

	int run = pset.get<int>("run_number", 1);
	bool debug = pset.get<bool>("debug_cout", false);
	uint64_t timeout = pset.get<uint64_t>("transition_timeout", 30);
	uint64_t timestamp = 0;

	artdaq::configureMessageFacility("artdaqDriver", true, debug);

	fhicl::ParameterSet fragment_receiver_pset = pset.get<fhicl::ParameterSet>("fragment_receiver");

	std::unique_ptr<artdaq::FragmentGenerator>
		gen(artdaq::makeFragmentGenerator(fragment_receiver_pset.get<std::string>("generator"),
			fragment_receiver_pset));

	std::unique_ptr<artdaq::CommandableFragmentGenerator> commandable_gen =
		dynamic_unique_ptr_cast<artdaq::FragmentGenerator, artdaq::CommandableFragmentGenerator>(gen);

	my_rank = 0;
	// pull out the Metric part of the ParameterSet
	fhicl::ParameterSet metric_pset;
	try {
		metric_pset = pset.get<fhicl::ParameterSet>("metrics");
	}
	catch (...) {} // OK if there's no metrics table defined in the FHiCL 

	if (metric_pset.is_empty()) {
		TLOG(TLVL_INFO) << "No metric plugins appear to be defined";
	}
	try {
		metricMan->initialize(metric_pset, "artdaqDriver");
		metricMan->do_start();
	}
	catch (...) {
	}
	artdaq::FragmentPtrs frags;
	//////////////////////////////////////////////////////////////////////
	// Note: we are constrained to doing all this here rather than
	// encapsulated neatly in a function due to the lieftime issues
	// associated with async threads and std::string::c_str().
	fhicl::ParameterSet event_builder_pset = pset.get<fhicl::ParameterSet>("event_builder");

	artdaq::SharedMemoryEventManager event_manager(event_builder_pset, pset);
	//////////////////////////////////////////////////////////////////////

	int events_to_generate = pset.get<int>("events_to_generate", 0);
	int event_count = 0;
	artdaq::Fragment::sequence_id_t previous_sequence_id = -1;

	if (commandable_gen) {
		commandable_gen->StartCmd(run, timeout, timestamp);
	}

	TLOG(50) << "driver main before event_manager.startRun";
	event_manager.startRun(run);

	// Read or generate fragments as rapidly as possible, and feed them
	// into the Eventevent_manager. The throughput resulting from this design
	// choice is likely to have the fragment reading (or generation)
	// speed as the limiting factor
	while ((commandable_gen && commandable_gen->getNext(frags)) ||
		(gen && gen->getNext(frags))) {
		TLOG(50) << "driver main: getNext returned frags.size()=" << frags.size() << " current event_count=" << event_count;
		for (auto & val : frags) {
			if (val->sequenceID() != previous_sequence_id) {
				++event_count;
				previous_sequence_id = val->sequenceID();
			}
			if (events_to_generate != 0 && event_count > events_to_generate) {
				if (commandable_gen) {
					commandable_gen->StopCmd(timeout, timestamp);
				}
				break;
			}

			auto start_time = std::chrono::steady_clock::now();
			bool sts = false;
			auto loop_count = 0;
			while (!sts)
			{
				artdaq::FragmentPtr tempFrag;
				sts = event_manager.AddFragment(std::move(val), 1000000, tempFrag);
				if (!sts && event_count <= 10 && loop_count > 100)
				{
					TLOG(TLVL_ERROR) << "Fragment was not added after " << artdaq::TimeUtils::GetElapsedTime(start_time) << " s. Check art thread status!";
					event_manager.endOfData();
					exit(1);
				}
				val = std::move(tempFrag);
				if (!sts)
				{
					loop_count++;
					//usleep(10000);
				}
			}
		}
		frags.clear();

		if (events_to_generate != 0 && event_count >= events_to_generate) {
			if (commandable_gen) {
				commandable_gen->StopCmd(timeout, timestamp);
			}
			break;
		}
	}

	if (commandable_gen) {
		commandable_gen->joinThreads();
	}

	TLOG(TLVL_INFO) << "Fragments generated, waiting for art to process them.";
	auto art_wait_start_time = std::chrono::steady_clock::now();
	auto last_delta_time = std::chrono::steady_clock::now();
	auto last_count = event_manager.size() - event_manager.WriteReadyCount(false);

	while (last_count > 0 && artdaq::TimeUtils::GetElapsedTime(last_delta_time) < 1.0)
	{
		auto this_count = event_manager.size() - event_manager.WriteReadyCount(false);
		if (this_count != last_count) {
			last_delta_time = std::chrono::steady_clock::now();
			last_count = this_count;
		}
		usleep(1000);
	}

	TLOG(TLVL_INFO) << "Ending Run, waited " << std::setprecision(2) << artdaq::TimeUtils::GetElapsedTime(art_wait_start_time) << " seconds for art to process events. (" << last_count << " buffers remain).";
	event_manager.endRun();
	usleep(artdaq::TimeUtils::GetElapsedTimeMicroseconds(art_wait_start_time)); // Wait as long again for EndRun message to go through

	TLOG(TLVL_INFO) << "Shutting down art";
	bool endSucceeded = false;
	int attemptsToEnd = 1;
	endSucceeded = event_manager.endOfData();
	while (!endSucceeded && attemptsToEnd < 3) {
		++attemptsToEnd;
		endSucceeded = event_manager.endOfData();
	}
	if (!endSucceeded) {
		TLOG(TLVL_ERROR) << "Failed to shut down the reader and the SharedMemoryEventManager "
			<< "because the endOfData marker could not be pushed "
			<< "onto the queue.";
	}

	metricMan->do_stop();
	return 0;
}
catch (std::string & x)
{
	std::cerr << "Exception (type string) caught in artdaqDriver: " << x << '\n';
	return 1;
}
catch (char const * m)
{
	std::cerr << "Exception (type char const*) caught in artdaqDriver: ";
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
catch (...) {
	artdaq::ExceptionHandler(artdaq::ExceptionHandlerRethrow::no,
		"Exception caught in artdaqDriver");
}


template<typename B, typename D>
std::unique_ptr<D>
dynamic_unique_ptr_cast(std::unique_ptr<B>& p)
{
	D* result = dynamic_cast<D*>(p.get());

	if (result) {
		p.release();
		return std::unique_ptr<D>(result);
	}
	return nullptr;
}
