#ifndef ARTDAQ_PROTO_ARTDAQAPP_HH
#define ARTDAQ_PROTO_ARTDAQAPP_HH

#include "artdaq/DAQdata/Globals.hh"

#include "artdaq/Application/TaskType.hh"
#include "artdaq-core/Utilities/configureMessageFacility.hh"
#include "artdaq/BuildInfo/GetPackageBuildInfo.hh"
#include "artdaq/Application/BoardReaderApp.hh"
#include "artdaq/Application/EventBuilderApp.hh"
#include "artdaq/Application/DataLoggerApp.hh"
#include "artdaq/Application/DispatcherApp.hh"
#include "artdaq/Application/RoutingMasterApp.hh"
#include "artdaq/ExternalComms/MakeCommanderPlugin.hh"

#include <sys/prctl.h>

namespace artdaq {
	/// <summary>
	/// Class representing an artdaq application. Used by all "main" functions to start artdaq.
	/// </summary>
	class artdaqapp
	{
	public:
		/// <summary>
		/// Configuration of artdaqapp. May be used for parameter validation
		/// </summary>
		struct Config
		{
			/// "application_name" (Default: artdaq::detail::TaskTypeToString(task)): Name to use for metrics and logging
			fhicl::Atom<std::string> application_name{ fhicl::Name{ "application_name" }, fhicl::Comment{ "Name to use for metrics and logging" }, "BoardReader" };
			/// "replace_image_name" (Default: false): Replace the application image name with application_name
			fhicl::Atom<bool> replace_image_name{ fhicl::Name{ "replace_image_name" }, fhicl::Comment{ "Replace the application image name with application_name" }, false };
			/// "rank": The "rank" of the application, used for configuring data transfers
			fhicl::Atom<int> rank{ fhicl::Name{ "rank" }, fhicl::Comment{ "The \"rank\" of the application, used for configuring data transfers" } };
			fhicl::TableFragment<artdaq::CommanderInterface::Config> commanderPluginConfig; ///< Configuration for the CommanderInterface. See artdaq::CommanderInterface::Config
			/// "auto_run" (Default: false): Whether to automatically start a run
			fhicl::Atom<bool> auto_run{ fhicl::Name{"auto_run"}, fhicl::Comment{"Whether to automatically start a run"}, false };
			/// "run_number" (Default: 101): Run number to use for automatic run
			fhicl::Atom<int> run_number{ fhicl::Name{"run_number"}, fhicl::Comment{"Run number to use for automatic run"}, 101 };
			/// "transition_timeout" (Default: 30): Timeout to use for automatic transitions
			fhicl::Atom<uint64_t> transition_timeout{ fhicl::Name{"transition_timeout"}, fhicl::Comment{"Timeout to use for automatic transitions"}, 30 };
			fhicl::TableFragment<artdaq::PortManager::Config> portsConfig; ///< Configuration for artdaq Ports
		};
		using Parameters = fhicl::WrappedTable<Config>;

		/// <summary>
		/// Run an artdaq Application
		/// </summary>
		/// <param name="task">Task type of the application</param>
		/// <param name="config_ps">"Startup" parameter set. See artdaq::artdaqapp::Config</param>
		static void runArtdaqApp(detail::TaskType task, fhicl::ParameterSet const& config_ps)
		{
			app_name = config_ps.get<std::string>("application_name", detail::TaskTypeToString(task));
			portMan->UpdateConfiguration(config_ps);

			if (config_ps.get<bool>("replace_image_name", config_ps.get<bool>("rin", false)))
			{
				int s;
				s = prctl(PR_SET_NAME, app_name.c_str(), NULL, NULL, NULL);
				if (s != 0)
				{
					std::cerr << "Could not replace process image name with " << app_name << "!" << std::endl;
					exit(1);
				}
			}

			std::string mf_app_name = artdaq::setMsgFacAppName(app_name, config_ps.get<int>("id"));
			artdaq::configureMessageFacility(mf_app_name.c_str());

			if (config_ps.has_key("rank"))
			{
				my_rank = config_ps.get<int>("rank");
			}
			TLOG_DEBUG(app_name + "Main") << "Setting application name to " << app_name;

			// 23-May-2018, KAB: added lookup of the partition number from the command line arguments.
			if (config_ps.has_key("partition_number"))
			{
				artdaq::Globals::partition_number_ = config_ps.get<int>("partition_number");
			}
			TLOG_DEBUG(app_name + "Main") << "Setting partition number to " << artdaq::Globals::partition_number_;

			TLOG_DEBUG(app_name + "Main") << "artdaq version " <<
				artdaq::GetPackageBuildInfo::getPackageBuildInfo().getPackageVersion()
				<< ", built " <<
				artdaq::GetPackageBuildInfo::getPackageBuildInfo().getBuildTimestamp();

			artdaq::setMsgFacAppName(app_name, config_ps.get<int>("id"));

			std::unique_ptr<artdaq::Commandable> comm(nullptr);
			switch (task)
			{
			case(detail::BoardReaderTask):
				comm.reset(new BoardReaderApp());
				break;
			case(detail::EventBuilderTask):
				comm.reset(new EventBuilderApp());
				break;
			case(detail::DataLoggerTask):
				comm.reset(new DataLoggerApp());
				break;
			case(detail::DispatcherTask):
				comm.reset(new DispatcherApp());
				break;
			case(detail::RoutingMasterTask):
				comm.reset(new RoutingMasterApp());
				break;
			default:
				return;
			}

			auto auto_run = config_ps.get<bool>("auto_run", false);
			if (auto_run)
			{
				int run = config_ps.get<int>("run_number", 101);
				uint64_t timeout = config_ps.get<uint64_t>("transition_timeout", 30);
				uint64_t timestamp = 0;

				comm->do_initialize(config_ps, timeout, timestamp);
				comm->do_start(art::RunID(run), timeout, timestamp);

				TLOG_INFO(app_name + "Main") << "Running XMLRPC Commander. To stop, either Control-C or " << std::endl
					<< "xmlrpc http://`hostname`:" << config_ps.get<int>("id") << "/RPC2 daq.stop" << std::endl
					<< "xmlrpc http://`hostname`:" << config_ps.get<int>("id") << "/RPC2 daq.shutdown";
			}

			auto commander = artdaq::MakeCommanderPlugin(config_ps, *comm.get());
			commander->run_server();
			artdaq::Globals::CleanUpGlobals();
		}

	};
}

#endif // ARTDAQ_PROTO_ARTDAQAPP_HH
