#define TRACE_NAME "shared_memory_reader_t"

#include "artdaq/ArtModules/detail/SharedMemoryReader.hh"
#include "artdaq/DAQrate/SharedMemoryEventManager.hh"

#include "art/Framework/Core/FileBlock.h"
//#include "art/Framework/Core/RootDictionaryManager.h"
#include "art/Framework/IO/Sources/SourceHelper.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/EventPrincipal.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/RunPrincipal.h"
#include "art/Framework/Principal/SubRunPrincipal.h"
//#include "art/Persistency/Provenance/BranchIDListHelper.h"
#include "art/Persistency/Provenance/MasterProductRegistry.h"
#include "art/Persistency/Provenance/ProductMetaData.h"
#include "canvas/Persistency/Provenance/EventID.h"
#include "canvas/Persistency/Provenance/FileFormatVersion.h"
#include "canvas/Persistency/Provenance/ModuleDescription.h"
#include "canvas/Persistency/Provenance/Parentage.h"
#include "canvas/Persistency/Provenance/ProcessConfiguration.h"
#include "canvas/Persistency/Provenance/RunID.h"
#include "canvas/Persistency/Provenance/SubRunID.h"
#include "canvas/Persistency/Provenance/Timestamp.h"
#include "canvas/Utilities/Exception.h"
#include "art/Version/GetReleaseVersion.h"
#include "artdaq-core/Data/Fragment.hh"
#include "artdaq-core/Utilities/configureMessageFacility.hh"
#include "fhiclcpp/make_ParameterSet.h"

#define BOOST_TEST_MODULE shared_memory_reader_t
#include <boost/test/auto_unit_test.hpp>

#include <iostream>
#include <memory>
#include <string>

/**
 * \brief MasterProductRegistry Test Fixture
 */
class MPRGlobalTestFixture
{
public:
	/**
	 * \brief MPRGlobalTestFixture Constructor
	 */
	MPRGlobalTestFixture();

	/**
	 * \brief BKmap_t associates a string with a art::BranchKey
	 */
	typedef std::map<std::string, art::BranchKey> BKmap_t;

	BKmap_t branchKeys_; ///< Keys in this test fixture
	std::map<std::string, std::unique_ptr<art::ProcessConfiguration>> processConfigurations_; ///< Configurations

	/**
	 * \brief Create the ProcessConfiguration for a single module art process
	 * \param tag Tag for the ProcessConfiguraiton
	 * \param processName Name of the process
	 * \param moduleParams ParameterSet for the single module
	 * \param release See art::ProcessConfiguration
	 * \param pass See art::ProcessConfiguration
	 * \return Pointer to created art::ProcessConfiguration object
	 */
	art::ProcessConfiguration*
		fake_single_module_process(std::string const& tag,
								   std::string const& processName,
								   fhicl::ParameterSet const& moduleParams,
								   std::string const& release = art::getReleaseVersion()
		);

	/**
	 * \brief Create a BranchDescription for a process
	 * \param tag Tag for the module_process
	 * \param processName Name of the process
	 * \param productInstanceName Name of the product
	 * \return Pointer to created art::BranchDescription object
	 */
	std::unique_ptr<art::BranchDescription>
		fake_single_process_branch(std::string const& tag,
								   std::string const& processName,
								   std::string const& productInstanceName = std::string());

	/**
	 * \brief Finalizes the ProductRegistry
	 */
	void finalize();

	art::MasterProductRegistry productRegistry_; ///< MasterProductRegistry instance
	//art::RootDictionaryManager rdm_;
};

MPRGlobalTestFixture::MPRGlobalTestFixture()
	:
	branchKeys_()
	, processConfigurations_()
	, productRegistry_()//,
//  rdm_()
{
	// We can only insert products registered in the MasterProductRegistry.
	productRegistry_.addProduct(fake_single_process_branch("hlt", "HLT"));
	productRegistry_.addProduct(fake_single_process_branch("prod", "PROD"));
	productRegistry_.addProduct(fake_single_process_branch("test", "TEST"));
	productRegistry_.addProduct(fake_single_process_branch("user", "USER"));
	productRegistry_.addProduct(fake_single_process_branch("rick", "USER2", "rick"));
}

void
MPRGlobalTestFixture::finalize()
{
	productRegistry_.setFrozen();
	art::ProductMetaData::create_instance(productRegistry_);
}

art::ProcessConfiguration*
MPRGlobalTestFixture::
fake_single_module_process(std::string const& tag,
						   std::string const& processName,
						   fhicl::ParameterSet const& moduleParams,
						   std::string const& release
)
{
	fhicl::ParameterSet processParams;
	processParams.put(processName, moduleParams);
	processParams.put<std::string>("process_name",
								   processName);
	auto emplace_pair =
		processConfigurations_.emplace(tag,
									   std::make_unique<art::ProcessConfiguration>(processName, processParams.id(), release, pass));
	return emplace_pair.first->second.get();
}

std::unique_ptr<art::BranchDescription>
MPRGlobalTestFixture::
fake_single_process_branch(std::string const& tag,
						   std::string const& processName,
						   std::string const& productInstanceName)
{
	std::string moduleLabel = processName + "dummyMod";
	std::string moduleClass("DummyModule");
	fhicl::ParameterSet modParams;
	modParams.put<std::string>("module_type", moduleClass);
	modParams.put<std::string>("module_label", moduleLabel);
	art::ProcessConfiguration* process =
		fake_single_module_process(tag, processName, modParams);
	art::ModuleDescription mod(modParams.id(),
							   moduleClass,
							   moduleLabel,
							   *process);
	art::TypeID dummyType(typeid(int));
	art::BranchDescription* result =
		new art::BranchDescription(
			art::InEvent,
			art::TypeLabel(dummyType,
						   productInstanceName),
			mod);


	branchKeys_.insert(std::make_pair(tag, art::BranchKey(*result)));
	return std::unique_ptr<art::BranchDescription>(result);
}

/**
 * \brief SharedMemoryReader Test Fixture
 */
struct ShmRTestFixture
{
	/**
	 * \brief ShmRTestFixture Constructor
	 */
	ShmRTestFixture()
	{
		static bool once(true);
		if (once)
		{
			artdaq::configureMessageFacility("shared_memory_reader_t");
			(void)reader(); // Force initialization.
			art::ModuleDescription md(fhicl::ParameterSet().id(),
									  "_NAMEERROR_",
									  "_LABELERROR_",
									  *gf().processConfigurations_["daq"]);
			// These _xERROR_ strings should never appear in branch names; they
			// are here as tracers to help identify any failures in coding.
			helper().registerProducts(gf().productRegistry_, md);
			gf().finalize();
			once = false;
		}
	}

	/**
	 * \brief Get a MPRGlobalTestFixture, creating a static instance if necessary
	 * \return MPRGlobalTestFixture object
	 */
	MPRGlobalTestFixture& gf()
	{
		static MPRGlobalTestFixture mpr;
		return mpr;
	}

	/**
	 * \brief Get an art::ProductRegistryHelper, creating a static instance if necessary
	 * \return art::ProductRegistryHelper object
	 */
	art::ProductRegistryHelper& helper()
	{
		static art::ProductRegistryHelper s_helper;
		return s_helper;
	}

	/**
	 * \brief Get an art::SourceHelper object, creating a static instance if necessary
	 * \return art::SourceHelper object
	 */
	art::SourceHelper& source_helper()
	{
		static std::unique_ptr<art::SourceHelper>
			s_source_helper;
		if (!s_source_helper)
		{
			fhicl::ParameterSet sourceParams;
			std::string moduleType{ "DummySource" };
			std::string moduleLabel{ "daq" };
			sourceParams.put<std::string>("module_type", moduleType);
			sourceParams.put<std::string>("module_label", moduleLabel);
			auto pc_ptr = gf().fake_single_module_process(moduleLabel,
														  "TEST",
														  sourceParams);
			art::ModuleDescription md(sourceParams.id(),
									  moduleType,
									  moduleLabel,
									  *pc_ptr);
			s_source_helper = std::make_unique<art::SourceHelper>(md);
		}
		return *s_source_helper;
	}

	/**
	 * \brief Gets the key for the shared memory segment
	 * \return Key of the shared memory segment
	 */
	uint32_t getKey()
	{
		static uint32_t key = static_cast<uint32_t>(std::hash<std::string>()("shared_memory_reader_t"));
		return key;
	}
	/**
	* \brief Gets the key for the broadcast shared memory segment
	* \return Key of the broadcast shared memory segment
	*/
	uint32_t getBroadcastKey()
	{
		static uint32_t key = static_cast<uint32_t>(std::hash<std::string>()("shared_memory_reader_t BROADCAST"));
		return key;
	}

	/**
	 * \brief Get an artdaq::detail::SharedMemoryReader object, creating a static instance if necessary
	 * \return artdaq::detail::SharedMemoryReader object
	 */
	artdaq::detail::SharedMemoryReader<>& reader()
	{
		writer();
		fhicl::ParameterSet pset;
		pset.put("shared_memory_key", getKey());
		pset.put("broadcast_shared_memory_key", getBroadcastKey());
		pset.put("max_event_size_bytes", 0x100000);
		pset.put("buffer_count", 10);
		static artdaq::detail::SharedMemoryReader<>
			s_reader(pset,
					 helper(),
					 source_helper(),
					 gf().productRegistry_);
		static bool reader_initialized = false;
		if (!reader_initialized)
		{
			s_reader.fragment_type_map_[1] = "ABCDEF";
			helper().reconstitutes<artdaq::Fragments, art::InEvent>("daq", "ABCDEF");
			reader_initialized = true;
		}
		return s_reader;
	}

	/**
	 * \brief Get the instance of the SharedMemoryEventManager
	 * \return Static instance of the SharedMemoryEventManager
	 */
	artdaq::SharedMemoryEventManager& writer()
	{
		fhicl::ParameterSet pset;
		pset.put("shared_memory_key", getKey());
		pset.put("broadcast_shared_memory_key", getBroadcastKey());
		pset.put("max_event_size_bytes", 0x100000);
		pset.put("art_analyzer_count", 0);
		pset.put("stale_buffer_timeout_usec", 100000);
		pset.put("expected_fragments_per_event", 1);
		pset.put("buffer_count", 10);
		static artdaq::SharedMemoryEventManager
			s_writer(pset, pset);
		return s_writer;

	}
};

BOOST_FIXTURE_TEST_SUITE(shared_memory_reader_t, ShmRTestFixture)

namespace
{
	/**
	 * \brief Run a basic checkout of the SharedMemoryReader
	 * \param reader SharedMemoryReader instance
	 * \param writer SharedMemoryWriter instance
	 * \param run Run principal pointer
	 * \param subrun Subrun principal pointer
	 * \param eventid ID of event
	 */
	void basic_test(artdaq::detail::SharedMemoryReader<>& reader,
					artdaq::SharedMemoryEventManager& writer,
					std::unique_ptr<art::RunPrincipal>&& run,
					std::unique_ptr<art::SubRunPrincipal>&& subrun,
					art::EventID const& eventid)
	{
		BOOST_REQUIRE(run || subrun == nullptr); // Sanity check.
		std::vector<artdaq::Fragment::value_type> fakeData{ 1, 2, 3, 4 };
		artdaq::FragmentPtr
			tmpFrag(artdaq::Fragment::dataFrag(eventid.event(),
											   0,
											   fakeData.begin(),
											   fakeData.end()));
		tmpFrag->setUserType(1);

		writer.startRun(eventid.run());


		auto iter = tmpFrag->dataBegin();
		std::ostringstream str;
		str << "{";
		while (iter != tmpFrag->dataEnd())
		{
			str << *iter << ", ";
			++iter;

		}
		str << "}";
		TLOG(TLVL_DEBUG) << "Fragment to art: " << str.str();


		artdaq::FragmentPtr tempFrag;
		auto sts = writer.AddFragment(std::move(tmpFrag), 1000000, tempFrag);
		BOOST_REQUIRE_EQUAL(sts, true);

		while (writer.GetLockedBufferCount())
		{
			writer.sendMetrics();
			usleep(100000);
		}

		art::EventPrincipal* newevent = nullptr;
		art::SubRunPrincipal* newsubrun = nullptr;
		art::RunPrincipal* newrun = nullptr;
		bool rc = reader.readNext(run.get(), subrun.get(), newrun, newsubrun, newevent);
		BOOST_REQUIRE(rc);
		if (run.get() && run->run() == eventid.run())
		{
			BOOST_CHECK(newrun == nullptr);
		}
		else
		{
			BOOST_CHECK(newrun);
			BOOST_CHECK(newrun->id() == eventid.runID());
		}
		if (!newrun && subrun.get() && subrun->subRun() == eventid.subRun())
		{
			BOOST_CHECK(newsubrun == nullptr);
		}
		else
		{
			BOOST_CHECK(newsubrun);
			BOOST_CHECK(newsubrun->id() == eventid.subRunID());
		}
		BOOST_CHECK(newevent);
		BOOST_CHECK(newevent->id() == eventid);
		art::Event e(*newevent, art::ModuleDescription());
		art::Handle<std::vector<artdaq::Fragment>> h;
		e.getByLabel("daq", "ABCDEF", h);
		BOOST_CHECK(h.isValid());
		BOOST_CHECK(h->size() == 1);

		auto iter2 = h->front().dataBegin();
		std::ostringstream str2;
		str2 << "{";
		while (iter2 != h->front().dataEnd())
		{
			str2 << *iter2 << ", ";
			++iter2;

		}
		str2 << "}";
		TLOG(TLVL_DEBUG) << "Fragment from art: " << str2.str();

		BOOST_CHECK(std::equal(fakeData.begin(),
							   fakeData.end(),
							   h->front().dataBegin()));
		delete(newrun);
		delete(newsubrun);
		delete(newevent);
	}
}

BOOST_AUTO_TEST_CASE(nonempty_event)
{
	art::EventID eventid(2112, 1, 3);
	art::Timestamp now;
	basic_test(reader(), writer(),
			   std::unique_ptr<art::RunPrincipal>(source_helper().makeRunPrincipal(eventid.run(), now)),
			   std::unique_ptr<art::SubRunPrincipal>(source_helper().makeSubRunPrincipal(eventid.run(), eventid.subRun(), now)),
			   eventid);
}

BOOST_AUTO_TEST_CASE(first_event)
{
	art::EventID eventid(2112, 1, 3);
	art::Timestamp now;
	basic_test(reader(), writer(),
			   nullptr,
			   nullptr,
			   eventid);
}

BOOST_AUTO_TEST_CASE(new_subrun)
{
	art::EventID eventid(2112, 1, 3);
	art::Timestamp now;
	basic_test(reader(), writer(),
			   std::unique_ptr<art::RunPrincipal>(source_helper().makeRunPrincipal(eventid.run(), now)),
			   std::unique_ptr<art::SubRunPrincipal>(source_helper().makeSubRunPrincipal(eventid.run(), 0, now)),
			   eventid);
}

BOOST_AUTO_TEST_CASE(new_run)
{
	art::EventID eventid(2112, 1, 3);
	art::Timestamp now;
	basic_test(reader(), writer(),
			   std::unique_ptr<art::RunPrincipal>(source_helper().makeRunPrincipal(eventid.run() - 1, now)),
			   std::unique_ptr<art::SubRunPrincipal>(source_helper().makeSubRunPrincipal(eventid.run() - 1,
																						 eventid.subRun(),
																						 now)),
			   eventid);
}

BOOST_AUTO_TEST_CASE(end_of_data)
{
	// Tell 'reader' the name of the file we are to read. This is pretty
	// much irrelevant for SharedMemoryReader, but we'll stick to the
	// interface demanded by Source<T>...
	std::string const fakeFileName("no such file exists");
	art::FileBlock* pFile = nullptr;
	reader().readFile(fakeFileName, pFile);
	BOOST_CHECK(pFile);
	BOOST_CHECK(pFile->fileFormatVersion() == art::FileFormatVersion(1, "RawEvent2011"));
	BOOST_CHECK(pFile->tree() == nullptr);

	BOOST_CHECK(!pFile->fastClonable());
	// Test the end-of-data handling. Reading an end-of-data should result in readNext() returning false,
	// and should return null pointers for new-run, -subrun and -event.
	// Prepare our 'previous run/subrun/event'..
	art::RunID runid(2112);
	art::SubRunID subrunid(2112, 1);
	art::EventID eventid(2112, 1, 3);
	art::Timestamp now;
	std::unique_ptr<art::RunPrincipal> run(source_helper().makeRunPrincipal(runid.run(), now));
	std::unique_ptr<art::SubRunPrincipal> subrun(source_helper().makeSubRunPrincipal(runid.run(), subrunid.subRun(), now));
	std::unique_ptr<art::EventPrincipal> event(source_helper().makeEventPrincipal(runid.run(),
																				  subrunid.subRun(),
																				  eventid.event(),
																				  now));
	writer().endOfData();
	art::EventPrincipal* newevent = nullptr;
	art::SubRunPrincipal* newsubrun = nullptr;
	art::RunPrincipal* newrun = nullptr;
	bool rc = reader().readNext(run.get(), subrun.get(), newrun, newsubrun, newevent);
	BOOST_CHECK(!rc);
	BOOST_CHECK(newrun == nullptr);
	BOOST_CHECK(newsubrun == nullptr);
	BOOST_CHECK(newevent == nullptr);
}

BOOST_AUTO_TEST_SUITE_END()
