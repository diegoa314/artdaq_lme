#ifndef artdaq_Application_CompositeDriver_hh
#define artdaq_Application_CompositeDriver_hh

#include "fhiclcpp/fwd.h"
#include "artdaq/Application/CommandableFragmentGenerator.hh"
#include "artdaq-core/Data/Fragment.hh"
#include <vector>

namespace artdaq
{
	/**
	 * \brief CompositeDriver handles a set of lower-level generators
	 * 
	 * When multiple CommandableFragmentGenerators are needed by a single BoardReader, CompositeDriver may be used to provide
	 * a single interface to all of them.
	 */
	class CompositeDriver : public CommandableFragmentGenerator
	{
	public:
		/**
		 * \brief CompositeDriver Constructor
		 * \param ps ParameterSet used to configure CompositeDriver
		 * 
		 * \verbatim
		 * CompositeDriver accepts the following Parameters:
		 * "generator_config_list" (REQUIRED): A FHiCL sequence of FHiCL tables, each one configuring
		 * a CommandableFragmentGenerator instance.
		 * \endverbatim
		 */
		explicit CompositeDriver(fhicl::ParameterSet const& ps);

		/**
		 * \brief Destructor. Calls the destructors for each configured CommandableFragmentGenerator
		 */
		virtual ~CompositeDriver() noexcept;

		/**
		 * \brief Start all configured CommandableFragmentGenerators
		 */
		void start() override;

		/**
		 * \brief Call non-locked stop methods for all configured CommandableFragmentGenerators
		 */
		void stopNoMutex() override;

		/**
		 * \brief Call stop methods for all configured CommandableFragmentGenerators. Currently handled by stopNoMutex
		 */
		void stop() override;

		/**
		 * \brief Pause all configured CommandableFragmentGenerators
		 */
		void pause() override;

		/**
		 * \brief Resume all configured CommandableFragmentGenerators
		 */
		void resume() override;

	private:
		std::vector<artdaq::Fragment::fragment_id_t> fragmentIDs() override;

		bool getNext_(artdaq::FragmentPtrs& output) override;

		bool makeChildGenerator_(fhicl::ParameterSet const&);

		std::vector<std::unique_ptr<CommandableFragmentGenerator>> generator_list_;
		std::vector<bool> generator_active_list_;
	};
}
#endif /* artdaq_Application_CompositeDriver_hh */
