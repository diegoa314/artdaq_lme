#ifndef artdaq_ArtModules_TransferWrapper_hh
#define artdaq_ArtModules_TransferWrapper_hh


#include <string>
#include <memory>
#include <iostream>

#include "artdaq/TransferPlugins/TransferInterface.hh"
#include "artdaq/ExternalComms/CommanderInterface.hh"

#include <TBufferFile.h>

namespace fhicl
{
	class ParameterSet;
}

namespace artdaq
{
	class Fragment;

	/**
	 * \brief TransferWrapper wraps a TransferInterface so that it can be used in the ArtdaqInput class
	 * to make an art::Source
	 * 
	 * JCF, May-27-2016
	 *
	 * This is the class through which code that wants to access a
	 * transfer plugin (e.g., input sources, AggregatorCore, etc.) can do
	 * so. Its functionality is such that it satisfies the requirements
	 * needed to be a template in the ArtdaqInput class
	 */
	class TransferWrapper
	{
	public:

		/**
		 * \brief TransferWrapper Constructor
		 * \param pset ParameterSet used to configure the TransferWrapper
		 * 
		 * \verbatim
		 * TransferWrapper accepts the following Parameters:
		 * "timeoutInUsecs" (Default: 100000): The receive timeout
		 * "dispatcherHost" (REQUIRED): The hostname that the Dispatcher Aggregator is running on
		 * "dispatcherPort" (REQUIRED): The port that the Dispatcher Aggregator is running on
		 * "maxEventsBeforeInit" (Default: 5): How many non-Init events to receive before raising an error
		 * "allowedFragmentTypes" (Default: [226,227,229]): The Fragment type codes for expected Fragments
		 * "quitOnFragmentIntegrityProblem" (Default: true): If there is an inconsistency in the received Fragment, throw an exception and quit when true
		 * "debugLevel" (Default: 0): Enables some additional messages
		 * "transfer_plugin" (REQUIRED): Name of the TransferInterface plugin to load
		 * 
		 * This parameter set is also passed to TransferInterface, so any necessary Parameters for TransferInterface or the requested plugin
		 * should be included here.
		 * \endverbatim
		 */
		explicit TransferWrapper(const fhicl::ParameterSet& pset);

		/**
		 * \brief TransferWrapper Destructor
		 */
		virtual ~TransferWrapper();

		/**
		 * \brief Receive a Fragment from the TransferInterface, and send it to art
		 * \param[out] msg The message in art format
		 */
		void receiveMessage(std::unique_ptr<TBufferFile>& msg);

		/**
		 * \brief Receive the Init message from the TransferInterface, and send it to art
		 * \param[out] msg The message in art format
		 */
		void receiveInitMessage(std::unique_ptr<TBufferFile>& msg) { receiveMessage(msg); }

	private:

		void extractTBufferFile(const artdaq::Fragment&, std::unique_ptr<TBufferFile>&);

		void checkIntegrity(const artdaq::Fragment&) const;

		void unregisterMonitor();

		std::size_t timeoutInUsecs_;
		std::unique_ptr<TransferInterface> transfer_;
		std::unique_ptr<CommanderInterface> commander_;
		const std::string dispatcherHost_;
		const std::string dispatcherPort_;
		const std::string serverUrl_;
		const std::size_t maxEventsBeforeInit_;
		const std::vector<int> allowedFragmentTypes_;
		const bool quitOnFragmentIntegrityProblem_;
		bool monitorRegistered_;
	};
}

#endif /* artdaq_ArtModules_TransferWrapper_hh */
