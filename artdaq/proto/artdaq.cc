#include "proto/artdaqapp.hh"
#include "artdaq/Application/LoadParameterSet.hh"

int main(int argc, char* argv[])
{
	struct Config
	{
		fhicl::TableFragment<artdaq::artdaqapp::Config> artdaqapp_config;
		fhicl::Atom<std::string> app_type{ fhicl::Name{"app_type"}, fhicl::Comment{"Type of the artdaq application to run"}, "" };
	};

	fhicl::ParameterSet config_ps = LoadParameterSet<Config>(argc, argv, "artdaq", "This meta-application may be configured to run any of the core artdaq processes through the \"app_type\" configuration parameter.");
	artdaq::detail::TaskType task = artdaq::detail::UnknownTask;

	if (config_ps.has_key("app_type"))
	{
		task = artdaq::detail::StringToTaskType(config_ps.get<std::string>("app_type", ""));
		if (task == artdaq::detail::TaskType::UnknownTask) {
			task = artdaq::detail::IntToTaskType(config_ps.get<int>("app_type"));
		}
	}
	else if (config_ps.has_key("application_type"))
	{
		task = artdaq::detail::StringToTaskType(config_ps.get<std::string>("application_type", ""));
		if (task == artdaq::detail::TaskType::UnknownTask) {
			task = artdaq::detail::IntToTaskType(config_ps.get<int>("application_type"));
		}
	}
	artdaq::artdaqapp::runArtdaqApp(task, config_ps);
}
