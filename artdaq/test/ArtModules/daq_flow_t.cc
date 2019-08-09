#define TRACE_NAME "daq_flow_t"

#include "art/Framework/Art/artapp.h"
#include "artdaq-core/Data/Fragment.hh"
#include "artdaq/DAQdata/GenericFragmentSimulator.hh"
#include "artdaq/DAQrate/SharedMemoryEventManager.hh"
#include "cetlib_except/exception.h"
#include "fhiclcpp/make_ParameterSet.h"
#include "artdaq/Application/LoadParameterSet.hh"

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

using artdaq::FragmentPtrs;
using artdaq::GenericFragmentSimulator;
using artdaq::SharedMemoryEventManager;
using std::size_t;


int main(int argc, char* argv[])
{
	struct Config {
		fhicl::TableFragment<artdaq::SharedMemoryEventManager::Config> shmem_config;
		fhicl::TableFragment<art::Config> art_config;
		fhicl::TableFragment<artdaq::GenericFragmentSimulator::Config> frag_gen_config;
	};
	auto pset = LoadParameterSet<Config>(argc, argv, "daq_flow_t", "daq_flow_t tests data from a GenericFragmentSimulator through art");
	int rc = -1;
	try
	{
		size_t const NUM_FRAGS_PER_EVENT = 5;
		SharedMemoryEventManager::run_id_t const RUN_ID = 2112;
		size_t const NUM_EVENTS = 100;
		pset.put("expected_fragments_per_event", NUM_FRAGS_PER_EVENT);
		pset.put("run_number", RUN_ID);
		pset.put("print_event_store_stats", true);
		pset.put("event_queue_wait_time", 10.0);
		pset.put("max_event_size_bytes", 0x100000);
		pset.put("buffer_count", 10);
		pset.put("send_init_fragments", false);

		auto temp = pset.to_string() + " source.waiting_time: 10";
		pset = fhicl::ParameterSet();
		fhicl::make_ParameterSet(temp, pset);
		// Eventually, this test should make a mixed-up streams of
		// Fragments; this has too clean a pattern to be an interesting
		// test of the EventStore's ability to deal with multiple events
		// simulatenously.
		GenericFragmentSimulator sim(pset);
		SharedMemoryEventManager events(pset, pset);
		events.startRun(RUN_ID);
		FragmentPtrs frags;
		size_t event_count = 0;
		while (frags.clear() , event_count++ < NUM_EVENTS && sim.getNext(frags))
		{
			TLOG(TLVL_DEBUG) << "Number of fragments: " << frags.size();
			assert(frags.size() == NUM_FRAGS_PER_EVENT);
			for (auto&& frag : frags)
			{
				assert(frag != nullptr);

				auto start_time = std::chrono::steady_clock::now();
				bool sts = false;
				auto loop_count = 0;
				while (!sts)
				{
					artdaq::FragmentPtr tempFrag;
					sts = events.AddFragment(std::move(frag), 1000000, tempFrag);
					if (!sts && event_count <= 10 && loop_count > 100)
					{
						TLOG(TLVL_ERROR) << "Fragment was not added after " << artdaq::TimeUtils::GetElapsedTime(start_time) << " s. Check art thread status!";
						events.endOfData();
						exit(1);
					}
					frag = std::move(tempFrag);
					if (!sts)
					{
						loop_count++;
						usleep(10000);
					}
				}
			}
		}

		bool endSucceeded = events.endOfData();
		if (endSucceeded)
		{
			rc = 0;
		}
		else
		{
			rc = 15;
		}
	}
	catch (cet::exception& x)
	{
		std::cerr << argv[0] << " failure\n" << x << std::endl;
		rc = 1;
	}
	catch (std::string& x)
	{
		std::cerr << argv[0] << " failure\n" << x << std::endl;
		rc = 2;
	}
	catch (char const* x)
	{
		std::cerr << argv[0] << " failure\n" << x << std::endl;
		rc = 3;
	}
	return rc;
}
