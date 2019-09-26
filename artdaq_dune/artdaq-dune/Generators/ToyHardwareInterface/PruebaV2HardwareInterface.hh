#ifndef artdaq_dune_Generators_ToyHardwareInterface_PruebaV2HardwareInterface_hh
#define artdaq_dune_Generators_ToyHardwareInterface_PruebaV2HardwareInterface_hh

#include "artdaq-core-dune/Overlays/FragmentType.hh"

#include "fhiclcpp/fwd.h"

class PruebaV2HardwareInterface {
public:
	typedef uint16_t adc_t;
	explicit PruebaV2HardwareInterface(fhicl::ParameterSet const & ps);
	void StartDatataking();
	void StopDatataking();
	void FillBuffer(adc_t* buffer, size_t* bytes_read) const; 
	void AllocateBuffer(adc_t** buffer ) const;
	void FreeReadoutBuffer(adc_t* buffer) const;
	int SerialNumber() ;
	int NumADCBits() ; //retorna el numero de bits usados por el generador
private:
	bool taking_data_;
	std::size_t nADCcounts_;
	double multiplicador_;
	double amplitud_;
	double freq_muestreo_;
};
#endif
		
