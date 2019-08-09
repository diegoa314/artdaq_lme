#define TRACE_NAME "reconfigure_t"

#include "art/Framework/Art/artapp.h"
#include "artdaq-core/Data/Fragment.hh"
#include "artdaq/DAQdata/GenericFragmentSimulator.hh"
#include "artdaq/DAQrate/SharedMemoryEventManager.hh"
#include "cetlib_except/exception.h"
#include "fhiclcpp/ParameterSet.h"
#include "fhiclcpp/make_ParameterSet.h"
#include "artdaq/Application/LoadParameterSet.hh"

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

using artdaq::SharedMemoryEventManager;
using artdaq::FragmentPtrs;
using artdaq::GenericFragmentSimulator;
using fhicl::ParameterSet;
using std::size_t;


int main(int argc, char* argv[])
{
	artdaq::configureMessageFacility("reconfigure_t");
	struct Config
	{
		fhicl::TableFragment<artdaq::SharedMemoryEventManager::Config> shmem_config;
		fhicl::TableFragment<art::Config> art_config;
		fhicl::TableFragment<artdaq::GenericFragmentSimulator::Config> frag_gen_config;
	};
	auto pset = LoadParameterSet<Config>(argc, argv, "reconfigure_t", "reconfigure_t tests data from a GenericFragmentSimulator through art, then performs a reconfiguration and tests that the reconfiguration worked correctly.");
	int rc = -1;
	try
	{
		size_t const NUM_FRAGS_PER_EVENT = 5;
		SharedMemoryEventManager::run_id_t const RUN_ID = 2112;
		size_t const NUM_EVENTS = 100;
		// We may want to add ParameterSet parsing to this code, but right
		// now this will do...
		pset.put("expected_fragments_per_event", NUM_FRAGS_PER_EVENT);
		pset.put("run_number", RUN_ID);
		pset.put("print_event_store_stats", true);
		pset.put("max_event_size_bytes", 0x100000);
		pset.put("buffer_count",10);
		pset.put("send_init_fragments", false);

		auto temp = pset.to_string() + " source.waiting_time: 10";
		pset = fhicl::ParameterSet();
		fhicl::make_ParameterSet(temp, pset);
		// Eventually, this test should make a mixed-up streams of
		// Fragments; this has too clean a pattern to be an interesting
		// test of the EventStore's ability to deal with multiple events
		// simulatenously.
		GenericFragmentSimulator sim(pset);
		std::unique_ptr<SharedMemoryEventManager> events(new SharedMemoryEventManager(pset, pset));
		events->startRun(100);
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
					sts = events->AddFragment(std::move(frag), 1000000, tempFrag);
					if (!sts && event_count <= 10 && loop_count > 100)
					{
						TLOG(TLVL_ERROR) << "Fragment was not added after " << artdaq::TimeUtils::GetElapsedTime(start_time) << " s. Check art thread status!";
						events->endOfData();
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

		std::cout << "Ending first run..." << std::endl;
		bool endSucceeded = events->endOfData();
		if (endSucceeded)
		{
			rc = 0;

			size_t const NUM_EVENTS2 = 200;
			auto temp_config = pset.to_string() + " source.waiting_time: 10 physics.analyzers.frags.num_events_expected: " + std::to_string(NUM_EVENTS2);
			fhicl::ParameterSet sim_config2;
			fhicl::make_ParameterSet(temp_config, sim_config2);
			GenericFragmentSimulator sim2(sim_config2);
			events->ReconfigureArt(sim_config2);
			event_count = 0;
			while (frags.clear() , event_count++ < NUM_EVENTS2 && sim2.getNext(frags))
			{
				TLOG(TLVL_DEBUG) << "Number of fragments: " << frags.size();
				assert(frags.size() == NUM_FRAGS_PER_EVENT);
				for (auto&& frag : frags)
				{
					assert(frag != nullptr);
					bool sts = false;
					auto loop_count = 0;
					while (!sts)
					{
						artdaq::FragmentPtr tempFrag;
						sts = events->AddFragment(std::move(frag), 1000000, tempFrag);
						if (!sts && event_count <= 10 && loop_count < 100)
						{
							TLOG(TLVL_ERROR) << "Fragment was not added after 1s. Check art thread status!";
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

			bool endSucceeded2 = events->endOfData();
			if (endSucceeded2)
			{
				rc = 0;
			}
			else
			{
				rc = 16;
			}
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
