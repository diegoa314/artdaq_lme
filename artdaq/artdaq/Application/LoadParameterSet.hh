#ifndef artdaq_proto_LoadParameterSet_hh
#define artdaq_proto_LoadParameterSet_hh 1

// note: in header files (this LoadParameterSet.hh) consider if you want TRACE/LOG to use "name" from .cc or name for this file
#include "tracemf.h"
#include "fhiclcpp/make_ParameterSet.h"
#include "fhiclcpp/types/Table.h"
#include <boost/program_options.hpp>
#include <iostream>
namespace bpo = boost::program_options;

fhicl::ParameterSet LoadParameterSet(std::string const& psetOrFile)
{
	fhicl::ParameterSet pset;

	try
	{
		make_ParameterSet(psetOrFile, pset);
	}
	catch (fhicl::exception e)
	{
		if (getenv("FHICL_FILE_PATH") == nullptr)
			setenv("FHICL_FILE_PATH", ".", 0);
		cet::filepath_lookup_after1 lookup_policy("FHICL_FILE_PATH");
		make_ParameterSet(psetOrFile, lookup_policy, pset);
	}

	return pset;
}

template<typename C>
void PrintConfigurationToConsole(std::string name)
{
	fhicl::Table<C> config_description(fhicl::Name{ name });
	config_description.print_allowed_configuration(std::cout);
}

template<typename C>
fhicl::ParameterSet LoadParameterSet(int argc, char* argv[], std::string name, std::string description)
{
	std::ostringstream descstr;
	descstr << argv[0]
		<< " <-c <config>> <other-options> [<source-file>]+";
	bpo::options_description desc(descstr.str());
	desc.add_options()
		("config,c", bpo::value<std::string>(), "Configuration")
		("help,h", "produce help message");
	bpo::variables_map vm;
	try
	{
		bpo::store(bpo::command_line_parser(argc, argv).options(desc).run(), vm);
		bpo::notify(vm);
	}
	catch (bpo::error const & e)
	{
		TLOG_ERROR("LoadParameterSet") << "Exception from command line processing in " << argv[0]
			<< ": " << e.what() << "\n";
		exit(-1);
	}
	if (vm.count("help"))
	{
		std::cout << desc << std::endl;
		std::cout << description << std::endl;
		std::cout << "Sample FHiCL configuration for this application: " << std::endl;
		fhicl::Table<C> config_description(fhicl::Name{ name });
		config_description.print_allowed_configuration(std::cout);
		exit(1);
	}


	fhicl::ParameterSet pset;

	if (vm.count("config"))
	{
		std::string config = vm["config"].as<std::string>();

		if (config == "-" || config == "--")
		{
			TLOG_ERROR("LoadParameterSet") << "Reading configuration from standard input. Press Ctrl-D to end" << std::endl;
			std::stringstream ss;
			std::string line;
			while (std::getline(std::cin, line))
			{
				ss << line << std::endl;
			}
			std::cin.clear();

			make_ParameterSet(ss.str(), pset);
		}
		else
		{
			TLOG_DEBUG("LoadParameterSet") << config << std::endl;
			pset = LoadParameterSet(config);
		}
	}
	else
	{
		TLOG_ERROR("LoadParameterSet") << "Exception from command line processing in " << argv[0]
			<< ": no configuration given.\n"
			<< "For usage and an options list, please do '"
			<< argv[0] << " --help"
			<< "'.\n";
		exit(2);
	}
	return pset;
}
#endif //artdaq_proto_LoadParameterSet_hh
