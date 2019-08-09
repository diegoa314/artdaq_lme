#define TRACE_NAME "commander_test"

#include "artdaq/ExternalComms/CommanderInterface.hh"

#include "artdaq/ExternalComms/MakeCommanderPlugin.hh"
#include "artdaq/Application/LoadParameterSet.hh"

#include "artdaq/DAQdata/Globals.hh"
#include <boost/thread.hpp>

int main(int argc, char** argv)
{
	struct Config
	{
		fhicl::TableFragment<artdaq::CommanderInterface::Config> commanderPluginConfig; ///< Configuration for the CommanderInterface. See artdaq::CommanderInterface::Config

		fhicl::Atom<std::string> partition_number{ fhicl::Name{ "partition_number" }, fhicl::Comment{ "Partition to run in" }, "" };
	};
	artdaq::configureMessageFacility("transfer_driver");
	fhicl::ParameterSet config_ps = LoadParameterSet<Config>(argc, argv, "commander_test", "A test driver for CommanderInterface plugins");


	if (config_ps.has_key("partition_number")) artdaq::Globals::partition_number_ = config_ps.get<int>("partition_number");

	std::unique_ptr<artdaq::Commandable> cmdble(new artdaq::Commandable());

	auto commander = artdaq::MakeCommanderPlugin(config_ps, *cmdble.get());

	// Start server thread
	boost::thread commanderThread([&] { commander->run_server(); });
	while (!commander->GetStatus()) usleep(10000);
    sleep(1);

	uint64_t arg = 0;
	fhicl::ParameterSet pset;

	TLOG(TLVL_DEBUG) << "Sending init";
	std::string sts = commander->send_init(pset, arg, arg);
	TLOG(TLVL_DEBUG) << "init res=" << sts << ", sending soft_init";
	sts = commander->send_soft_init(pset,arg,arg);
	TLOG(TLVL_DEBUG) << "soft_init res=" << sts << ", sending legal_commands";

	sts = commander->send_legal_commands();
	TLOG(TLVL_DEBUG) << "legal_commands res=" << sts << ", sending meta_command";
	sts = commander->send_meta_command("test","test");
	TLOG(TLVL_DEBUG) << "meta_command res=" << sts << ", sending report";
	sts = commander->send_report("test");
	TLOG(TLVL_DEBUG) << "report res=" << sts << ", sending start";

	sts = commander->send_start(art::RunID(0x7357),arg,arg);
	TLOG(TLVL_DEBUG) << "start res=" << sts << ", sending status";
	sts = commander->send_status();
	TLOG(TLVL_DEBUG) << "status res=" << sts << ", sending pause";
	sts = commander->send_pause(arg,arg);
	TLOG(TLVL_DEBUG) << "pause res=" << sts << ", sending resume";
	sts = commander->send_resume(arg,arg);
	TLOG(TLVL_DEBUG) << "resume res=" << sts << ", sending rollover_subrun";
	sts = commander->send_rollover_subrun(arg);
	TLOG(TLVL_DEBUG) << "rollover_subrun res=" << sts << ", sending stop";
	sts = commander->send_stop(arg,arg);
	TLOG(TLVL_DEBUG) << "stop res=" << sts << ", sending reinit";
	sts = commander->send_reinit(pset, arg, arg);
	TLOG(TLVL_DEBUG) << "reinit res=" << sts << ", sending trace_set";

	sts = commander->send_trace_set("TRACE", "M", 0x7357);
	TLOG(TLVL_DEBUG) << "trace_set res=" << sts << ", sending trace_get";
	sts = commander->send_trace_get("TRACE");
	TLOG(TLVL_DEBUG) << "trace_get res=" << sts << ", sending register_monitor";

	sts = commander->send_register_monitor("test");
	TLOG(TLVL_DEBUG) << "register_monitor res=" << sts << ", sending unregister_monitor";
	sts = commander->send_unregister_monitor("test");
	TLOG(TLVL_DEBUG) << "unregister_monitor res=" << sts << ", sending shutdown";

	sts = commander->send_shutdown(arg);
	TLOG(TLVL_DEBUG) << "shutdown res=" << sts << ", DONE";

	if(commanderThread.joinable()) commanderThread.join();
}
