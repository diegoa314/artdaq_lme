#include "fhiclcpp/types/Atom.h"
#include "fhiclcpp/types/Table.h"
#include "fhiclcpp/types/OptionalAtom.h"
#include "fhiclcpp/types/OptionalTable.h"
#include "artdaq/ArtModules/NetMonTransportService.h"
#include "artdaq/ArtModules/RootNetOutput.hh"
#include "art/Framework/Core/OutputModule.h"
# include "art/Framework/IO/ClosingCriteria.h"

namespace art {
	/// <summary>
	/// Configuration of the services.scheduler block for artdaq art processes
	/// </summary>
	struct ServicesSchedulerConfig
	{ 
		/// "errorOnFailureToPut" (Default: false): This parameter is necessary for correct function of artdaq.Do not modify.
		fhicl::Atom<bool> errorOnFailureToPut{ fhicl::Name{"errorOnFailureToPut"}, fhicl::Comment{"This parameter is necessary for correct function of artdaq. Do not modify."}, false };
	};
	/// <summary>
	/// Configuration of the services block for artdaq art processes
	/// </summary>
	struct ServicesConfig
	{
		fhicl::Table<ServicesSchedulerConfig> scheduler{ fhicl::Name{"scheduler"} }; ///< Artdaq requires a services.scheduler parameter. See art::ServicesSchedulerConfig
		fhicl::OptionalTable<NetMonTransportService::Config> netMonTransportServiceInterface{ fhicl::Name{ "NetMonTransportServiceInterface" } }; ///< Configuration for the NetMonTranportService. See art::NetMonTransportService::Config
	};

	struct AnalyzersConfig {}; ///< \todo Fill in artdaq-provided Analyzer modules
	struct ProducersConfig {}; ///< Artdaq does not provide any producers
	struct FiltersConfig {}; ///< \todo Fill in artdaq-provided Filter modules

	/// <summary>
	/// Configuration of the physics block for artdaq art processes
	/// </summary>
	struct PhysicsConfig
	{
		fhicl::Table<AnalyzersConfig> analyzers{ fhicl::Name{"analyzers"} }; ///< Analyzer module configuration
		fhicl::Table<ProducersConfig> producers{ fhicl::Name{"producers"} }; ///< Producer module configuration
		fhicl::Table<FiltersConfig> filters{ fhicl::Name{"filters"} }; ///< Filter module configuration
		/// Output modules (configured in the outputs block) to use
		fhicl::Sequence<std::string> my_output_modules{ fhicl::Name{"my_output_modules"}, fhicl::Comment{"Output modules (configured in the outputs block) to use"} };
	};

	/// <summary>
	/// Confgiguration for ROOT output modules
	/// </summary>
	struct RootOutputConfig
	{
		using Name = fhicl::Name; ///< Parameter Name
		using Comment = fhicl::Comment; ///< Parameter Comment
		/// Configuration Parameter
		template <typename T> using Atom = fhicl::Atom<T>; 
		/// Optional Configuration Parameter
		template <typename T> using OptionalAtom = fhicl::OptionalAtom<T>;

		fhicl::TableFragment<art::OutputModule::Config> omConfig; ///< Configuration common to all OutputModules.
		Atom<std::string> catalog{ Name("catalog"), "" }; ///< ???
		OptionalAtom<bool> dropAllEvents{ Name("dropAllEvents") }; ///< Whether to drop all events ???
		Atom<bool> dropAllSubRuns{ Name("dropAllSubRuns"), false }; ///< Whether to drop all subruns ???
		OptionalAtom<bool> fastCloning{ Name("fastCloning") }; ///< Whether to try to use fastCloning on the file
		Atom<std::string> tmpDir{ Name("tmpDir"), "/tmp" }; ///< Temporary directory
		Atom<int> compressionLevel{ Name("compressionLevel"), 7 }; ///< Compression level to use. artdaq recommends <= 3
		Atom<int64_t> saveMemoryObjectThreshold{ Name("saveMemoryObjectThreshold"), -1l }; ///< ???
		Atom<int64_t> treeMaxVirtualSize{ Name("treeMaxVirtualSize"), -1 }; ///< ???
		Atom<int> splitLevel{ Name("splitLevel"), 99 }; ///< ???
		Atom<int> basketSize{ Name("basketSize"), 16384 }; ///< ???
		Atom<bool> dropMetaDataForDroppedData{ Name("dropMetaDataForDroppedData"), false }; ///< ???
		Atom<std::string> dropMetaData{ Name("dropMetaData"), "NONE" };///< Which metadata to drop (Default: "NONE")
		Atom<bool> writeParameterSets{ Name("writeParameterSets"), true };///< Write art ParameterSet to output file (Default: true)
		fhicl::Table<ClosingCriteria::Config> fileProperties{ Name("fileProperties") }; ///< When should the file be closed
		
		/// <summary>
		///  These keys should be ignored by the configuration validation processor
		/// </summary>
		struct KeysToIgnore
		{
			/// <summary>
			/// Get the keys to ignore
			/// </summary>
			/// <returns>Set of keys to ignore</returns>
			std::set<std::string> operator()() const
			{
				std::set<std::string> keys{ art::OutputModule::Config::KeysToIgnore::get() };
				keys.insert("results");
				return keys;
			}
		};

	};

	/// <summary>
	/// Configuration for the outputs block of artdaq art processes
	/// </summary>
	struct OutputsConfig
	{
		fhicl::OptionalTable<art::RootNetOutput::Config> rootNetOutput{ fhicl::Name{"rootNetOutput"} }; ///< For transferring data from EventBuilders to DataLoggers
		fhicl::OptionalTable<RootOutputConfig> normalOutput{ fhicl::Name{"normalOutput"} }; ///< Normal art/ROOT output
		fhicl::OptionalTable<RootOutputConfig> rootDAQOutFile{ fhicl::Name{"rootDAQOutFile"} }; ///< art/ROOT output where the filename can be specified at initialization (e.g. to /dev/null), for testing
	};

	/// <summary>
	/// Configuration for the source block of artdaq art processes
	/// </summary>
	struct SourceConfig
	{
		/// "module_type": Module type of source. Should be "RawInput", "NetMonInput", or an experiment-defined input type (e.g. "DemoInput")
		fhicl::Atom<std::string> module_type{ fhicl::Name{ "module_type" }, fhicl::Comment{ "Module type of source. Should be \"RawInput\", \"NetMonInput\", or an experiment-defined input type (e.g. \"DemoInput\")" } };
	};

	/// <summary>
	/// Required configuration for art processes started by artdaq, with artdaq-specific defaults where applicable
	/// </summary>
	struct Config
	{
		fhicl::Table<art::ServicesConfig> services{ fhicl::Name{"services"} }; ///< Services block
		fhicl::Table<art::PhysicsConfig> physics{ fhicl::Name{"physics"} }; ///< Physics block
		fhicl::Table<art::OutputsConfig> outputs{ fhicl::Name{"outputs"} }; ///< Outputs block
		fhicl::Table<art::SourceConfig> source{ fhicl::Name{"source"} }; ///< Source block
		/// "process_name" (Default: "DAQ"): Name of this art processing job
		fhicl::Atom<std::string> process_name{ fhicl::Name{"process_name" },fhicl::Comment{"Name of this art processing job"}, "DAQ" };
	};
}
