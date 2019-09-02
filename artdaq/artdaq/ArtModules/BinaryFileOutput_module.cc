

#define TRACE_NAME "BinaryFileOutput"

#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Core/OutputModule.h"
#include "art/Framework/Principal/EventPrincipal.h"
#include "art/Framework/Principal/RunPrincipal.h"
#include "art/Framework/Principal/SubRunPrincipal.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Persistency/Common/GroupQueryResult.h"
#include "art/Framework/IO/FileStatsCollector.h"
#include "art/Framework/IO/PostCloseFileRenamer.h"
#include "canvas/Utilities/DebugMacros.h"
#include "canvas/Utilities/Exception.h"
#include "fhiclcpp/ParameterSet.h"

#include "artdaq-core/Data/Fragment.hh"
#include "artdaq/DAQdata/Globals.hh"

#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <memory>
#include <unistd.h>
#include <stdio.h>

namespace art
{
	class BinaryFileOutput;
}

using art::BinaryFileOutput;
using fhicl::ParameterSet;

/**
 * \brief The BinaryFileOutput module streams art Events to a binary file, bypassing ROOT
 */
class art::BinaryFileOutput final: public OutputModule
{
public:
	/**
	 * \brief BinaryFileOutput Constructor
	 * \param ps ParameterSet used to configure BinaryFileOutput
	 *
	 * BinaryFileOutput accepts the same configuration parameters as art::OutputModule.
	 * It has the same name substitution code that RootOutput uses to uniquify names.
	 *
	 * BinaryFileOutput also expects the following Parameters:
	 * "fileName" (REQUIRED): Name of the file to write
	 * "directIO" (Default: false): Whether to use O_DIRECT
	 */
	explicit BinaryFileOutput(ParameterSet const& ps);

	/**
	 * \brief BinaryFileOutput Destructor
	 */
	virtual ~BinaryFileOutput();

private:
	void beginJob() override;

	void endJob() override;

	void write(EventPrincipal&) override;

	void writeRun(RunPrincipal&) override {};
	void writeSubRun(SubRunPrincipal&) override {};

	void initialize_FILE_();

	void deinitialize_FILE_();

	bool readParameterSet_(fhicl::ParameterSet const& pset);

private:
	std::string name_ = "BinaryFileOutput";
	std::string file_name_ = "/tmp/artdaqdemo.binary";
	bool do_direct_ = false;
	int fd_ = -1; // Used for direct IO
	std::unique_ptr<std::ofstream> file_ptr_ = {nullptr};
	art::FileStatsCollector fstats_;
};

art::BinaryFileOutput::
BinaryFileOutput(ParameterSet const& ps)
	: OutputModule(ps)
	, fstats_{ name_, processName() }
{
	TLOG(TLVL_DEBUG) << "Begin: BinaryFileOutput::BinaryFileOutput(ParameterSet const& ps)\n";
	readParameterSet_(ps);
	TLOG(TLVL_DEBUG) << "End: BinaryFileOutput::BinaryFileOutput(ParameterSet const& ps)\n";
}

art::BinaryFileOutput::
~BinaryFileOutput()
{
	TLOG(TLVL_DEBUG) << "Begin/End: BinaryFileOutput::~BinaryFileOutput()\n";
}

void
art::BinaryFileOutput::
beginJob()
{
	TLOG(TLVL_DEBUG) << "Begin: BinaryFileOutput::beginJob()\n";
	initialize_FILE_();
	TLOG(TLVL_DEBUG) << "End:   BinaryFileOutput::beginJob()\n";
}

void
art::BinaryFileOutput::
endJob()
{
	TLOG(TLVL_DEBUG) << "Begin: BinaryFileOutput::endJob()\n";
	deinitialize_FILE_();
	TLOG(TLVL_DEBUG) << "End:   BinaryFileOutput::endJob()\n";
}


void
art::BinaryFileOutput::
initialize_FILE_()
{
	std::string file_name = PostCloseFileRenamer{ fstats_ }.applySubstitutions(file_name_);
	if (do_direct_)
	{
		fd_ = open(file_name.c_str(), O_WRONLY | O_CREAT | O_DIRECT, 0660);
		TLOG(TLVL_TRACE) << "initialize_FILE_ fd_=" << fd_;
	}
	else
	{
		file_ptr_ = std::make_unique<std::ofstream>(file_name, std::ofstream::binary);
		TLOG(TLVL_TRACE) << "BinaryFileOutput::initialize_FILE_ file_ptr_=" << (void*)file_ptr_.get() << " errno=" << errno;
		//file_ptr_->rdbuf()->pubsetbuf(0, 0);
	}
	fstats_.recordFileOpen();
}

void
art::BinaryFileOutput::
deinitialize_FILE_()
{
	if (do_direct_)
	{
		close(fd_);
		fd_ = -1;
	}
	else
		file_ptr_.reset(nullptr);
	fstats_.recordFileClose();
}

bool
art::BinaryFileOutput::
readParameterSet_(fhicl::ParameterSet const& pset)
{
	TLOG(TLVL_DEBUG) << name_ << "BinaryFileOutput::readParameterSet_ method called with "
		<< "ParameterSet = \"" << pset.to_string()
		<< "\".";
	// determine the data sending parameters
	try
	{
		file_name_ = pset.get<std::string>("fileName");
	}
	catch (...)
	{
		TLOG(TLVL_ERROR) << name_
			<< "The fileName parameter was not specified "
			<< "in the BinaryFileOutput initialization PSet: \""
			<< pset.to_string() << "\".";
		return false;
	}
	do_direct_ = pset.get<bool>("directIO", false);
	// determine the data sending parameters
	return true;
}

#define USE_STATIC_BUFFER 0
#if USE_STATIC_BUFFER == 1
static unsigned char static_buffer[0xa00000];
#endif

void
art::BinaryFileOutput::
write(EventPrincipal& ep)
{
	using RawEvent = artdaq::Fragments;
	using RawEvents = std::vector<RawEvent>;
	using RawEventHandle = art::Handle<RawEvent>;
	using RawEventHandles = std::vector<RawEventHandle>;

	auto result_handles = std::vector<art::GroupQueryResult>();
	auto const& wrapped = art::WrappedTypeID::make<RawEvent>();
	result_handles = ep.getMany(wrapped, art::MatchAllSelector{});

	for (auto const& result_handle : result_handles)
	{
		auto const raw_event_handle = RawEventHandle(result_handle);

		if (!raw_event_handle.isValid())
			continue;

		for (auto const& fragment : *raw_event_handle)
		{
			auto sequence_id = fragment.sequenceID();
			auto fragid_id = fragment.fragmentID();
			TLOG(TLVL_TRACE) << "BinaryFileOutput::write seq=" << sequence_id
				<< " frag=" << fragid_id << " " << reinterpret_cast<const void*>(fragment.headerBeginBytes())
				<< " bytes=0x" << std::hex << fragment.sizeBytes() << " start";
			if (do_direct_)
			{
				ssize_t sts = ::write(fd_, reinterpret_cast<const char*>(fragment.headerBeginBytes()), fragment.sizeBytes());
				TLOG(5) << "BinaryFileOutput::write seq=" << sequence_id << " frag=" << fragid_id << " done sts=" << sts << " errno=" << errno;
			}
			else
			{
#          if USE_STATIC_BUFFER == 1
				file_ptr_->write((char*)static_buffer, fragment.sizeBytes());
#          else
				file_ptr_->write(reinterpret_cast<const char*>(fragment.headerBeginBytes()), fragment.sizeBytes());
#          endif
				TLOG(5) << "BinaryFileOutput::write seq=" << sequence_id << " frag=" << fragid_id << " done errno=" << errno;
			}
		}
	}
	fstats_.recordEvent(ep.id());
	return;
}

DEFINE_ART_MODULE(art::BinaryFileOutput)
