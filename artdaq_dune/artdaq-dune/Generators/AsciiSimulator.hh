#ifndef artdaq_dune_Generators_AsciiSimulator_hh
#define artdaq_dune_Generators_AsciiSimulator_hh

#include "fhiclcpp/fwd.h"
#include "artdaq-core/Data/Fragment.hh"
#include "artdaq/Application/CommandableFragmentGenerator.hh"
#include "artdaq-core-dune/Overlays/AsciiFragment.hh"
#include "artdaq-core-dune/Overlays/FragmentType.hh"

#include <random>
#include <vector>
#include <atomic>

namespace demo
{
	/**
	 * \brief Generates ASCIIFragments filled with user-specified ASCII strings
	 * 
	 * AsciiSimulator is a simple type of fragment generator intended to be
	 * studied by new users of artdaq as an example of how to create such
	 * a generator in the "best practices" manner. Derived from artdaq's
	 * CommandableFragmentGenerator class, it can be used in a full DAQ
	 * simulation, generating ASCII strings used for data validataion.
	 */
	class AsciiSimulator : public artdaq::CommandableFragmentGenerator
	{
	public:
		/**
		 * \brief AsciiSimulator Constructor
		 * \param ps fhicl::ParameterSet to configure AsciiSimulator. AsciiSimulator accepts the following configuration parameters:
		 * "throttle_usecs", how long to pause at the beginning of each call to getNext_, "string1" and "string2", strings to alternately put into the AsciiFragment
		 */
		explicit AsciiSimulator(fhicl::ParameterSet const& ps);

	private:

		/**
		 * \brief getNext_ is where the AsciiSimulator creates new AsciiFragments.
		 * \param output artdaq::FragmentPtrs object. New Fragments should be appended to this list
		 * \return true if the generator was not stopped
		 *  
		 * The "getNext_" function is used to implement user-specific
		 * functionality; it's a mandatory override of the pure virtual
		 * getNext_ function declared in CommandableFragmentGenerator
		 */
		bool getNext_(artdaq::FragmentPtrs& output) override;

		// Explicitly declare that there is nothing special to be done
		// by the start, stop, and stopNoMutex methods in this class
		void start() override {} ///< No special start actions necessary
		void stop() override {} ///< No special stop actions necessary
		void stopNoMutex() override {} ///< No special stop actions necessary

		// FHiCL-configurable variables. Note that the C++ variable names
		// are the FHiCL variable names with a "_" appended
		
		std::size_t const throttle_usecs_; ///< Sleep at start of each call to getNext_(), in us

		// Members needed to generate the simulated data
		std::string string1_; ///< The first string to generate. Alternates with string2_ in output data
		std::string string2_; ///< The second string to generate. Alternates with string1_ in output data
	};
}

#endif /* artdaq_demo_Generators_AsciiSimulator_hh */
