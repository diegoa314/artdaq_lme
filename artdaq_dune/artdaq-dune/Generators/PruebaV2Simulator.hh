#ifndef artdaq_dune_Generators_PruebaV2Simulator_hh
#define artdaq_dune_Generators_PruebaV2Simulator_hh

// Some C++ conventions used:

// -Append a "_" to every private member function and variable

#include "fhiclcpp/fwd.h"
#include "artdaq-core/Data/Fragment.hh"
#include "artdaq/Application/CommandableFragmentGenerator.hh"
#include "artdaq-core-dune/Overlays/PruebaV2Fragmento.hh"
#include "artdaq-core-dune/Overlays/FragmentType.hh"

#include "ToyHardwareInterface/PruebaV2HardwareInterface.hh"

#include <vector>
namespace prueba {	
	class PruebaV2Simulator: public artdaq::CommandableFragmentGenerator{
	public:
		explicit PruebaV2Simulator(fhicl::ParameterSet const& ps);
		virtual ~PruebaV2Simulator();
	private:
		bool getNext_(artdaq::FragmentPtrs & output) override;
		void start() override;
		void stop() override;
		void stopNoMutex() override;
		std::unique_ptr<PruebaV2HardwareInterface> hardware_interface_;
		artdaq::Fragment::timestamp_t timestamp_;
		int timestampScale_;
		PruebaV2Fragmento::Metadato metadato_;
		PruebaV2Fragmento::adc_t* readout_buffer_;
		demo::FragmentType fragment_type_;
	};
}
#endif


