#include "artdaq/ArtModules/RootNetOutput.hh"

#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art/Persistency/Provenance/ProcessHistoryRegistry.h"
#include "art/Persistency/Provenance/ProductMetaData.h"

#include "canvas/Persistency/Provenance/BranchDescription.h"
#include "canvas/Persistency/Provenance/BranchKey.h"
#include "canvas/Persistency/Provenance/History.h"
#include "canvas/Persistency/Provenance/ParentageRegistry.h"
#include "canvas/Persistency/Provenance/ProcessConfiguration.h"
#include "canvas/Persistency/Provenance/ProcessConfigurationID.h"
#include "canvas/Persistency/Provenance/ProcessHistoryID.h"
#include "canvas/Persistency/Provenance/ProductList.h"
#include "canvas/Persistency/Provenance/ProductProvenance.h"
#include "canvas/Persistency/Provenance/RunAuxiliary.h"
#include "canvas/Persistency/Provenance/SubRunAuxiliary.h"
#include "canvas/Utilities/DebugMacros.h"
#include "canvas/Utilities/Exception.h"
#include "cetlib/column_width.h"
#include "cetlib/lpad.h"
#include "cetlib/rpad.h"
#include <algorithm>
# include <iterator>
#include "fhiclcpp/ParameterSet.h"
#include "fhiclcpp/ParameterSetID.h"
#include "fhiclcpp/ParameterSetRegistry.h"

#define TRACE_NAME "RootNetOutput"

#define TLVL_OPENFILE 5
#define TLVL_CLOSEFILE 6
#define TLVL_RESPONDTOCLOSEINPUTFILE 7
#define TLVL_RESPONDTOCLOSEOUTPUTFILE 8
#define TLVL_ENDJOB 9
#define TLVL_SENDINIT 10
#define TLVL_SENDINIT_VERBOSE1 32
#define TLVL_SENDINIT_VERBOSE2 33
#define TLVL_WRITEDATAPRODUCTS 11
#define TLVL_WRITEDATAPRODUCTS_VERBOSE 34
#define TLVL_WRITE 12
#define TLVL_WRITERUN 13
#define TLVL_WRITESUBRUN 14
#define TLVL_WRITESUBRUN_VERBOSE 35


#include "artdaq/DAQdata/Globals.hh"
#include "artdaq/ArtModules/NetMonTransportService.h"
#include "artdaq-core/Data/detail/ParentageMap.hh"
#include "artdaq/DAQdata/NetMonHeader.hh"

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <unistd.h>

#include <TClass.h>
#include <TMessage.h>
#include <TBufferFile.h>

art::RootNetOutput::
RootNetOutput(fhicl::ParameterSet const& ps)
	: OutputModule(ps)
	, initMsgSent_(false)
{
	TLOG(TLVL_DEBUG) << "Begin: RootNetOutput::RootNetOutput(ParameterSet const& ps)";
	ServiceHandle<NetMonTransportService> transport;
	transport->connect();
	TLOG(TLVL_DEBUG) << "End:   RootNetOutput::RootNetOutput(ParameterSet const& ps)";
}

art::RootNetOutput::
~RootNetOutput()
{
	TLOG(TLVL_DEBUG) << "Begin: RootNetOutput::~RootNetOutput()";
	ServiceHandle<NetMonTransportService> transport;
	transport->disconnect();
	TLOG(TLVL_DEBUG) << "End:   RootNetOutput::~RootNetOutput()";
}

void
art::RootNetOutput::
openFile(FileBlock const&)
{
	TLOG(TLVL_OPENFILE) << "Begin/End: RootNetOutput::openFile(const FileBlock&)";
}

void
art::RootNetOutput::
closeFile()
{
	TLOG(TLVL_CLOSEFILE) << "Begin/End: RootNetOutput::closeFile()";
}

void
art::RootNetOutput::
respondToCloseInputFile(FileBlock const&)
{
	TLOG(TLVL_RESPONDTOCLOSEINPUTFILE) << "Begin/End: RootNetOutput::"
		"respondToCloseOutputFiles(FileBlock const&)";
}

void
art::RootNetOutput::
respondToCloseOutputFiles(FileBlock const&)
{
	TLOG(TLVL_RESPONDTOCLOSEOUTPUTFILE) << "Begin/End: RootNetOutput::"
		"respondToCloseOutputFiles(FileBlock const&)";
}

static
void
send_shutdown_message()
{
	// 4/1/2016, ELF: Apparently, all this function does is make things not work.
	// At this point in the state machine, RHandles and SHandles have already been
	// destructed. Calling sendMessage will cause SHandles to reconnect itself,
	// but the other end will never recieve the message.
}

void
art::RootNetOutput::
endJob()
{
	TLOG(TLVL_ENDJOB) << "Begin: RootNetOutput::endJob()";
	send_shutdown_message();
	TLOG(TLVL_ENDJOB) << "End:   RootNetOutput::endJob()";
}

//#pragma GCC push_options
//#pragma GCC optimize ("O0")
static
void
send_init_message()
{
	TLOG(TLVL_SENDINIT) << "Begin: RootNetOutput static send_init_message()";
	//
	//  Get the classes we will need.
	//
	//static TClass* string_class = TClass::GetClass("std::string");
	//if (string_class == nullptr) {
	//	throw art::Exception(art::errors::DictionaryNotFound) <<
	//		"RootNetOutput static send_init_message(): "
	//		"Could not get TClass for std::string!";
	//}
	static TClass* product_list_class = TClass::GetClass(
		"std::map<art::BranchKey,art::BranchDescription>");
	if (product_list_class == nullptr)
	{
		throw art::Exception(art::errors::DictionaryNotFound) <<
			"RootNetOutput static send_init_message(): "
			"Could not get TClass for "
			"map<art::BranchKey,art::BranchDescription>!";
	}
	//typedef std::map<const ProcessHistoryID,ProcessHistory> ProcessHistoryMap;
	//TClass* process_history_map_class = TClass::GetClass(
	//    "std::map<const art::ProcessHistoryID,art::ProcessHistory>");
	//FIXME: Replace the "2" here with a use of the proper enum value!
	static TClass* process_history_map_class = TClass::GetClass(
		"std::map<const art::Hash<2>,art::ProcessHistory>");
	if (process_history_map_class == nullptr)
	{
		throw art::Exception(art::errors::DictionaryNotFound) <<
			"RootNetOutput static send_init_message(): "
			"Could not get class for "
			"std::map<const art::Hash<2>,art::ProcessHistory>!";
	}
	//static TClass* parentage_map_class = TClass::GetClass(
	//    "std::map<const art::ParentageID,art::Parentage>");
	//FIXME: Replace the "5" here with a use of the proper enum value!
	static TClass* parentage_map_class = TClass::GetClass("art::ParentageMap");
	if (parentage_map_class == nullptr)
	{
		throw art::Exception(art::errors::DictionaryNotFound) <<
			"RootNetOutput static send_init_message(): "
			"Could not get class for ParentageMap.";
	}
	TLOG(TLVL_SENDINIT) << "parentage_map_class: " << (void*)parentage_map_class;

	//
	//  Construct and send the init message.
	//
	TBufferFile msg(TBuffer::kWrite);
	msg.SetWriteMode();
	//
	//  Stream the message type code.
	//
	TLOG(TLVL_SENDINIT) << "RootNetOutput static send_init_message(): Streaming message type code ...";
	msg.WriteULong(1);
	TLOG(TLVL_SENDINIT) << "RootNetOutput static send_init_message(): Finished streaming message type code.";

	//
	//  Stream the ParameterSetRegistry.
	//
	unsigned long ps_cnt = fhicl::ParameterSetRegistry::size();
	TLOG(TLVL_SENDINIT) << "RootNetOutput static send_init_message(): parameter set count: " + std::to_string(ps_cnt);
	msg.WriteULong(ps_cnt);
	TLOG(TLVL_SENDINIT) << "RootNetOutput static send_init_message(): Streaming parameter sets ...";
	for (
		auto I = std::begin(fhicl::ParameterSetRegistry::get()),
		E = std::end(fhicl::ParameterSetRegistry::get());
		I != E; ++I)
	{
		std::string pset_str = I->second.to_string();
		//msg.WriteObjectAny(&pset_str, string_class);
		msg.WriteStdString(pset_str);
	}
	TLOG(TLVL_SENDINIT) << "RootNetOutput static send_init_message(): Finished streaming parameter sets.";

	//
	//  Stream the MasterProductRegistry.
	//
	TLOG(TLVL_SENDINIT) << "RootNetOutput static send_init_message(): Streaming MasterProductRegistry ...";
	art::ProductList productList(
		art::ProductMetaData::instance().productList());
	msg.WriteObjectAny(&productList, product_list_class);
	TLOG(TLVL_SENDINIT) << "RootNetOutput static send_init_message(): Finished streaming MasterProductRegistry.";
	
	art::ProcessHistoryMap phr;
	for (auto const& pr : art::ProcessHistoryRegistry::get())
	{
		phr.emplace(pr);
	}
	//
	//  Dump the ProcessHistoryRegistry.
	//
	TLOG(TLVL_SENDINIT_VERBOSE2) << "RootNetOutput static send_init_message(): Dumping ProcessHistoryRegistry ...";
	//typedef std::map<const ProcessHistoryID,ProcessHistory>
	//    ProcessHistoryMap;
	TLOG(TLVL_SENDINIT_VERBOSE2) << "RootNetOutput static send_init_message(): phr: size: " << phr.size();
	for (auto I = phr.begin(), E = phr.end(); I != E; ++I)
	{
		std::ostringstream OS;
		I->first.print(OS);
		TLOG(TLVL_SENDINIT_VERBOSE2) << "RootNetOutput static send_init_message(): phr: id: '" << OS.str() << "'";
	}
	//
	//  Stream the ProcessHistoryRegistry.
	//
	TLOG(TLVL_SENDINIT) << "RootNetOutput static send_init_message(): Streaming ProcessHistoryRegistry ...";
	//typedef std::map<const ProcessHistoryID,ProcessHistory>
	//    ProcessHistoryMap;
	const art::ProcessHistoryMap& phm = phr;
	TLOG(TLVL_SENDINIT) << "RootNetOutput static send_init_message(): phm: size: " << phm.size();
	msg.WriteObjectAny(&phm, process_history_map_class);
	TLOG(TLVL_SENDINIT) << "RootNetOutput static send_init_message(): Finished streaming ProcessHistoryRegistry.";

	//
	//  Stream the ParentageRegistry.
	//
	TLOG(TLVL_SENDINIT) << "static send_init_message(): Streaming ParentageRegistry ..."
		<< (void*)parentage_map_class;
	art::ParentageMap parentageMap{};
	for (auto const& pr : art::ParentageRegistry::get())
	{
		parentageMap.emplace(pr.first, pr.second);
	}

	msg.WriteObjectAny(&parentageMap, parentage_map_class);

	TLOG(TLVL_SENDINIT) << "static send_init_message(): Finished streaming ParentageRegistry.";

	//
	//
	//  Send init message.
	//
	art::ServiceHandle<NetMonTransportService> transport;
	if (!transport.get())
	{
		TLOG(TLVL_ERROR) << "Could not get handle to NetMonTransportService!";
		return;
	}
	TLOG(TLVL_SENDINIT) << "RootNetOutput static send_init_message(): Sending the init message to " << transport->dataReceiverCount() << " data receivers ...";
	for (size_t idx = 0; idx < transport->dataReceiverCount(); ++idx)
	{
		transport->sendMessage(idx, artdaq::Fragment::InitFragmentType, msg);
	}
	TLOG(TLVL_SENDINIT) << "RootNetOutput static send_init_message(): Init message(s) sent.";

	TLOG(TLVL_SENDINIT) << "End:   RootNetOutput static send_init_message()";
}

//#pragma GCC pop_options

void
art::RootNetOutput::
writeDataProducts(TBufferFile& msg, const Principal& principal,
	std::vector<BranchKey*>& bkv)
{
	TLOG(TLVL_WRITEDATAPRODUCTS) << "Begin: RootNetOutput::writeDataProducts(...)";
	//
	//  Fetch the class dictionaries we need for
	//  writing out the data products.
	//
	static TClass* branch_key_class = TClass::GetClass("art::BranchKey");
	if (branch_key_class == nullptr)
	{
		throw art::Exception(art::errors::DictionaryNotFound) <<
			"RootNetOutput::writeDataProducts(...): "
			"Could not get TClass for art::BranchKey!";
	}
	static TClass* prdprov_class = TClass::GetClass("art::ProductProvenance");
	if (prdprov_class == nullptr)
	{
		throw art::Exception(art::errors::DictionaryNotFound) <<
			"RootNetOutput::writeDataProducts(...): "
			"Could not get TClass for art::ProductProvenance!";
	}

	//
	//  Calculate the data product count.
	//
	unsigned long prd_cnt = 0;
	//std::map<art::BranchID, std::shared_ptr<art::Group>>::const_iterator
	for (auto I = principal.begin(), E = principal.end(); I != E; ++I)
	{
		auto const& productDescription = I->second->productDescription();
		auto const& refs = keptProducts()[productDescription.branchType()];
		bool found = false;
		for (auto const& ref : refs)
		{
			if (*ref == productDescription)
			{
				found = true;
				break;
			}
		}
		if (I->second->productUnavailable() || !found)
		{
			continue;
		}
		++prd_cnt;
	}
	//
	//  Write the data product count.
	//
	TLOG(TLVL_WRITEDATAPRODUCTS) << "RootNetOutput::writeDataProducts(...): Streaming product count: " + std::to_string(prd_cnt);
	msg.WriteULong(prd_cnt);
	TLOG(TLVL_WRITEDATAPRODUCTS) << "RootNetOutput::writeDataProducts(...): Finished streaming product count.";

	//
	//  Loop over the groups in the RunPrincipal and
	//  write out the data products.
	//
	// Note: We need this vector of keys because the ROOT I/O mechanism
	//       requires that each object inserted in the message has a
	//       unique address, so we force that by holding on to each
	//       branch key manufactured in the loop until after we are
	//       done constructing the message.
	//
	bkv.reserve(prd_cnt);
	//std::map<art::BranchID, std::shared_ptr<art::Group>>::const_iterator
	for (auto I = principal.begin(), E = principal.end(); I != E; ++I)
	{
		auto const& productDescription = I->second->productDescription();
		auto const& refs = keptProducts()[productDescription.branchType()];
		bool found = false;
		for (auto const& ref : refs)
		{
			if (*ref == productDescription)
			{
				found = true;
				break;
			}
		}
		if (I->second->productUnavailable() || !found)
		{
			continue;
		}
		const BranchDescription& bd(I->second->productDescription());
		bkv.push_back(new BranchKey(bd));
		TLOG(TLVL_WRITEDATAPRODUCTS_VERBOSE) << "RootNetOutput::writeDataProducts(...): Dumping branch key           of class: '"
			<< bkv.back()->friendlyClassName_
			<< "' modlbl: '"
			<< bkv.back()->moduleLabel_
			<< "' instnm: '"
			<< bkv.back()->productInstanceName_
			<< "' procnm: '"
			<< bkv.back()->processName_
			<< "'";
		TLOG(TLVL_WRITEDATAPRODUCTS) << "RootNetOutput::writeDataProducts(...): "
			"Streaming branch key         of class: '"
			<< bd.producedClassName()
			<< "' modlbl: '"
			<< bd.moduleLabel()
			<< "' instnm: '"
			<< bd.productInstanceName()
			<< "' procnm: '"
			<< bd.processName()
			<< "'";
		msg.WriteObjectAny(bkv.back(), branch_key_class);
		TLOG(TLVL_WRITEDATAPRODUCTS) << "RootNetOutput::writeDataProducts(...): "
			"Streaming product            of class: '"
			<< bd.producedClassName()
			<< "' modlbl: '"
			<< bd.moduleLabel()
			<< "' instnm: '"
			<< bd.productInstanceName()
			<< "' procnm: '"
			<< bd.processName()
			<< "'";

		OutputHandle oh = principal.getForOutput(bd.productID(), true);
		const EDProduct* prd = oh.wrapper();
		TLOG(TLVL_WRITEDATAPRODUCTS) << "Class for branch " << bd.wrappedName() << " is " << (void*)TClass::GetClass(bd.wrappedName().c_str());
		msg.WriteObjectAny(prd, TClass::GetClass(bd.wrappedName().c_str()));
		TLOG(TLVL_WRITEDATAPRODUCTS) << "RootNetOutput::writeDataProducts(...): "
			"Streaming product provenance of class: '"
			<< bd.producedClassName()
			<< "' modlbl: '"
			<< bd.moduleLabel()
			<< "' instnm: '"
			<< bd.productInstanceName()
			<< "' procnm: '"
			<< bd.processName()
			<< "'";
		const ProductProvenance* prdprov = I->second->productProvenancePtr().get();
		msg.WriteObjectAny(prdprov, prdprov_class);
	}
	TLOG(TLVL_WRITEDATAPRODUCTS) << "End:   RootNetOutput::writeDataProducts(...)";
}

void
art::RootNetOutput::
write(EventPrincipal& ep)
{
	//
	//  Write an Event message.
	//
	TLOG(TLVL_WRITE) << "Begin: RootNetOutput::write(const EventPrincipal& ep)";
	if (!initMsgSent_)
	{
		send_init_message();
		initMsgSent_ = true;
	}
	//
	//  Get root classes needed for I/O.
	//
	static TClass* run_aux_class = TClass::GetClass("art::RunAuxiliary");
	if (run_aux_class == nullptr)
	{
		throw art::Exception(art::errors::DictionaryNotFound) <<
			"RootNetOutput::write(const EventPrincipal& ep): "
			"Could not get TClass for art::RunAuxiliary!";
	}
	static TClass* subrun_aux_class = TClass::GetClass("art::SubRunAuxiliary");
	if (subrun_aux_class == nullptr)
	{
		throw art::Exception(art::errors::DictionaryNotFound) <<
			"RootNetOutput::write(const EventPrincipal& ep): "
			"Could not get TClass for art::SubRunAuxiliary!";
	}
	static TClass* event_aux_class = TClass::GetClass("art::EventAuxiliary");
	if (event_aux_class == nullptr)
	{
		throw art::Exception(art::errors::DictionaryNotFound) <<
			"RootNetOutput::write(const EventPrincipal& ep): "
			"Could not get TClass for art::EventAuxiliary!";
	}
	static TClass* history_class = TClass::GetClass("art::History");
	if (history_class == nullptr)
	{
		throw art::Exception(art::errors::DictionaryNotFound) <<
			"RootNetOutput::write(const EventPrincipal& ep): "
			"Could not get TClass for art::History!";
	}
	//
	//  Setup message buffer.
	//
	TBufferFile msg(TBuffer::kWrite);
	msg.SetWriteMode();
	//
	//  Write message type code.
	//
	TLOG(TLVL_WRITE) << "RootNetOutput::write(const EventPrincipal& ep): Streaming message type code ...";
	msg.WriteULong(4);
	TLOG(TLVL_WRITE) << "RootNetOutput::write(const EventPrincipal& ep): Finished streaming message type code.";

	//
	//  Write RunAuxiliary.
	//
	TLOG(TLVL_WRITE) << "RootNetOutput::write(const EventPrincipal& ep): Streaming RunAuxiliary ...";
	msg.WriteObjectAny(&ep.subRunPrincipal().runPrincipal().aux(),
		run_aux_class);
	TLOG(TLVL_WRITE) << "RootNetOutput::write(const EventPrincipal& ep): Finished streaming RunAuxiliary.";

	//
	//  Write SubRunAuxiliary.
	//
	TLOG(TLVL_WRITE) << "RootNetOutput::write(const EventPrincipal& ep): Streaming SubRunAuxiliary ...";
	msg.WriteObjectAny(&ep.subRunPrincipal().aux(),
		subrun_aux_class);
	TLOG(TLVL_WRITE) << "RootNetOutput::write(const EventPrincipal& ep): Finished streaming SubRunAuxiliary.";

	//
	//  Write EventAuxiliary.
	//
	{
		TLOG(TLVL_WRITE) << "RootNetOutput::write(const EventPrincipal& ep): Streaming EventAuxiliary ...";
		msg.WriteObjectAny(&ep.aux(), event_aux_class);
		TLOG(TLVL_WRITE) << "RootNetOutput::write(const EventPrincipal& ep): Finished streaming EventAuxiliary.";
	}
	//
	//  Write History.
	//
	{
		TLOG(TLVL_WRITE) << "RootNetOutput::write(const EventPrincipal& ep): Streaming History ...";
		msg.WriteObjectAny(&ep.history(), history_class);
		TLOG(TLVL_WRITE) << "RootNetOutput::write(const EventPrincipal& ep): Finished streaming History.";
	}
	//
	//  Write data products.
	//
	std::vector<BranchKey*> bkv;
	writeDataProducts(msg, ep, bkv);
	//
	//  Send message.
	//
	{
		ServiceHandle<NetMonTransportService> transport;
		if (!transport.get())
		{
			TLOG(TLVL_ERROR) << "Could not get handle to NetMonTransportService!";
			return;
		}
		TLOG(TLVL_WRITE) << "RootNetOutput::write(const EventPrincipal& ep): Sending a message ...";
		transport->sendMessage(ep.id().event(), artdaq::Fragment::DataFragmentType, msg);
		TLOG(TLVL_WRITE) << "RootNetOutput::write(const EventPrincipal& ep): Message sent.";
	}
	//
	//  Delete the branch keys we created for the message.
	//
	for (auto I = bkv.begin(), E = bkv.end(); I != E; ++I)
	{
		delete *I;
		*I = 0;
	}
	TLOG(TLVL_WRITE) << "End:   RootNetOutput::write(const EventPrincipal& ep)";
}

void
art::RootNetOutput::
writeRun(RunPrincipal& rp)
{
	//
	//  Write an EndRun message.
	//
	TLOG(TLVL_WRITERUN) << "Begin: RootNetOutput::writeRun(const RunPrincipal& rp)";
	(void)rp;
	if (!initMsgSent_)
	{
		send_init_message();
		initMsgSent_ = true;
	}
#if 0
	//
	//  Fetch the class dictionaries we need for
	//  writing out the auxiliary information.
	//
	static TClass* run_aux_class = TClass::GetClass("art::RunAuxiliary");
	assert(run_aux_class != nullptr && "writeRun: Could not get TClass for art::RunAuxiliary!");
	//
	//  Begin preparing message.
	//
	TBufferFile msg(TBuffer::kWrite);
	msg.SetWriteMode();
	//
	//  Write message type code.
	//
	{
		TLOG(TLVL_WRITERUN) << "writeRun: streaming message type code ...";
		msg.WriteULong(2);
		TLOG(TLVL_WRITERUN) << "writeRun: finished streaming message type code.";
	}
	//
	//  Write RunAuxiliary.
	//
	{
		TLOG(TLVL_WRITERUN) << "writeRun: streaming RunAuxiliary ...";
		TLOG(TLVL_WRITERUN_VERBOSE) << "writeRun: dumping ProcessHistoryRegistry ...";
		//typedef std::map<const ProcessHistoryID,ProcessHistory>
		//    ProcessHistoryMap;
		art::ProcessHistoryMap const& phr =
			art::ProcessHistoryRegistry::get();
		TLOG(TLVL_WRITERUN_VERBOSE) << "writeRun: phr: size: " << phr.size();
		for (auto I = phr.begin(), E = phr.end(); I != E; ++I)
		{
			std::ostringstream OS;
			I->first.print(OS);
			TLOG(TLVL_WRITERUN_VERBOSE) << "writeRun: phr: id: '" << OS.str() << "'";
			OS.str("");
			TLOG(TLVL_WRITERUN_VERBOSE) << "writeRun: phr: data.size(): " << I->second.data().size();
			if (I->second.data().size())
			{
				I->second.data().back().id().print(OS);
				TLOG(TLVL_WRITERUN_VERBOSE) << "writeRun: phr: data.back().id(): '" << OS.str() << "'";
			}
		}
		if (!rp.aux().processHistoryID().isValid())
		{
			TLOG(TLVL_WRITERUN_VERBOSE) << "writeRun: ProcessHistoryID: 'INVALID'";
		}
		else
		{
			std::ostringstream OS;
			rp.aux().processHistoryID().print(OS);
			TLOG(TLVL_WRITERUN_VERBOSE) << "writeRun: ProcessHistoryID: '" << OS.str() << "'";
			OS.str("");
			const ProcessHistory& processHistory =
				ProcessHistoryRegistry::get(rp.aux().processHistoryID());
			if (processHistory.data().size())
			{
				// FIXME: Print something special on invalid id() here!
				processHistory.data().back().id().print(OS);
				TLOG(TLVL_WRITERUN_VERBOSE) << "writeRun: ProcessConfigurationID: '" << OS.str() << "'";
				OS.str("");
				TLOG(TLVL_WRITERUN_VERBOSE) << "writeRun: ProcessConfiguration: '" << processHistory.data().back();
			}
		}
		msg.WriteObjectAny(&rp.aux(), run_aux_class);
		TLOG(TLVL_WRITERUN) << "writeRun: streamed RunAuxiliary.";
	}
	//
	//  Write data products.
	//
	std::vector<BranchKey*> bkv;
	writeDataProducts(msg, rp, bkv);
	//
	//  Send message.
	//
	{
		ServiceHandle<NetMonTransportService> transport;
		if (!transport.get())
		{
			TLOG(TLVL_ERROR) << "Could not get handle to NetMonTransportService!";
			return;
		}
		TLOG(TLVL_WRITERUN) << "writeRun: sending a message ...";
		transport->sendMessage(0, artdaq::Fragment::EndOfRunFragmentType, msg);
		TLOG(TLVL_WRITERUN) << "writeRun: message sent.";
	}
	//
	//  Delete the branch keys we created for the message.
	//
	for (auto I = bkv.begin(), E = bkv.end(); I != E; ++I)
	{
		delete *I;
		*I = 0;
	}
#endif // 0
	TLOG(TLVL_WRITERUN) << "End:   RootNetOutput::writeRun(const RunPrincipal& rp)";
}

void
art::RootNetOutput::writeSubRun(SubRunPrincipal& srp)
{
	//
	//  Write an EndSubRun message.
	//
	TLOG(TLVL_WRITESUBRUN) << "Begin: RootNetOutput::writeSubRun(const SubRunPrincipal& srp)";
	if (!initMsgSent_)
	{
		send_init_message();
		initMsgSent_ = true;
	}
	//
	//  Fetch the class dictionaries we need for
	//  writing out the auxiliary information.
	//
	static TClass* subrun_aux_class = TClass::GetClass("art::SubRunAuxiliary");
	if (subrun_aux_class == nullptr)
	{
		throw art::Exception(art::errors::DictionaryNotFound) <<
			"RootNetOutput::writeSubRun: "
			"Could not get TClass for art::SubRunAuxiliary!";
	}
	//
	//  Begin preparing message.
	//
	TBufferFile msg(TBuffer::kWrite);
	msg.SetWriteMode();
	//
	//  Write message type code.
	//
	{
		TLOG(TLVL_WRITESUBRUN) << "RootNetOutput::writeSubRun: streaming message type code ...";
		msg.WriteULong(3);
		TLOG(TLVL_WRITESUBRUN) << "RootNetOutput::writeSubRun: finished streaming message type code.";
	}
	//
	//  Write SubRunAuxiliary.
	//
	{
		TLOG(TLVL_WRITESUBRUN) << "RootNetOutput::writeSubRun: streaming SubRunAuxiliary ...";

		TLOG(TLVL_WRITESUBRUN_VERBOSE) << "RootNetOutput::writeSubRun: dumping ProcessHistoryRegistry ...";
		//typedef std::map<const ProcessHistoryID,ProcessHistory>
		//    ProcessHistoryMap;
		for (auto I = std::begin(art::ProcessHistoryRegistry::get())
			, E = std::end(art::ProcessHistoryRegistry::get()); I != E; ++I)
		{
			std::ostringstream OS;
			I->first.print(OS);
			TLOG(TLVL_WRITESUBRUN_VERBOSE) << "RootNetOutput::writeSubRun: phr: id: '" << OS.str() << "'";
			OS.str("");
			TLOG(TLVL_WRITESUBRUN_VERBOSE) << "RootNetOutput::writeSubRun: phr: data.size():  " << I->second.data().size();
			if (I->second.data().size())
			{
				I->second.data().back().id().print(OS);
				TLOG(TLVL_WRITESUBRUN_VERBOSE) << "RootNetOutput::writeSubRun: phr: data.back().id(): '" << OS.str() << "'";
			}
		}
		if (!srp.aux().processHistoryID().isValid())
		{
			TLOG(TLVL_WRITESUBRUN_VERBOSE) << "RootNetOutput::writeSubRun: ProcessHistoryID: 'INVALID'";
		}
		else
		{
			std::ostringstream OS;
			srp.aux().processHistoryID().print(OS);
			TLOG(TLVL_WRITESUBRUN_VERBOSE) << "RootNetOutput::writeSubRun: ProcessHistoryID: '" << OS.str() << "'";
			OS.str("");
			ProcessHistory processHistory;
			ProcessHistoryRegistry::get(srp.aux().processHistoryID(), processHistory);
			if (processHistory.data().size())
			{
				// FIXME: Print something special on invalid id() here!
				processHistory.data().back().id().print(OS);
				TLOG(TLVL_WRITESUBRUN_VERBOSE) << "RootNetOutput::writeSubRun: ProcessConfigurationID: '" << OS.str() << "'";
				OS.str("");
				OS << processHistory.data().back();
				TLOG(TLVL_WRITESUBRUN_VERBOSE) << "RootNetOutput::writeSubRun: ProcessConfiguration: '" << OS.str();
			}
		}
		msg.WriteObjectAny(&srp.aux(), subrun_aux_class);
		TLOG(TLVL_WRITESUBRUN) << "RootNetOutput::writeSubRun: streamed SubRunAuxiliary.";
	}
	//
	//  Write data products.
	//
	std::vector<BranchKey*> bkv;
	writeDataProducts(msg, srp, bkv);
	//
	//  Send message.
	//
	ServiceHandle<NetMonTransportService> transport;
	if (!transport.get())
	{
		TLOG(TLVL_ERROR) << "Could not get handle to NetMonTransportService!";
		return;
	}
	TLOG(TLVL_WRITESUBRUN) << "RootNetOutput::writeSubRun: Sending the EndOfSubrun message";
	transport->sendMessage(0, artdaq::Fragment::EndOfSubrunFragmentType, msg);
	TLOG(TLVL_WRITESUBRUN) << "RootNetOutput::writeSubRun: EndOfSubrun message sent.";

	// Disconnecting will cause EOD fragments to be generated which will
	// allow components downstream to flush data and clean up.
	//transport->disconnect();

	//
	//  Delete the branch keys we created for the message.
	//
	for (auto I = bkv.begin(), E = bkv.end(); I != E; ++I)
	{
		delete *I;
		*I = 0;
	}
	TLOG(TLVL_WRITESUBRUN) << "End:   RootNetOutput::writeSubRun(const SubRunPrincipal& srp)";
}

DEFINE_ART_MODULE(art::RootNetOutput)
