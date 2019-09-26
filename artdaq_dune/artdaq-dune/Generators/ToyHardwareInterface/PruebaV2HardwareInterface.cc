#include "artdaq-dune/Generators/ToyHardwareInterface/PruebaV2HardwareInterface.hh"
#define TRACE_NAME "PruebaV2HardwareInterface"
#include "artdaq/DAQdata/Globals.hh"
#include "artdaq-core-dune/Overlays/PruebaV2Fragmento.hh"
#include "artdaq-core-dune/Overlays/FragmentType.hh"

#include "fhiclcpp/ParameterSet.h"
#include "cetlib_except/exception.h"

#include <unistd.h>
#include <iostream>
#include <cstdlib>
#include <cmath>

PruebaV2HardwareInterface::PruebaV2HardwareInterface(fhicl::ParameterSet const& ps) :
	taking_data_(false)
	,nADCcounts_(ps.get<size_t>("nADCcounts",100))
	,multiplicador_(ps.get<double>("multiplicador",0.1))
	,amplitud_(ps.get<double>("amplitud",50))
	,freq_muestreo_(ps.get<double>("freq_muestreo",1e+6))  {}
	
void PruebaV2HardwareInterface::StartDatataking() {
	taking_data_=true;
}	

void PruebaV2HardwareInterface::StopDatataking() {
	taking_data_=false;
}

void PruebaV2HardwareInterface::FillBuffer(adc_t* buffer, size_t* bytes_read)const {
	if(taking_data_) {
		*bytes_read=sizeof(prueba::PruebaV2Fragmento::Header)+nADCcounts_*sizeof(adc_t);
		assert(*bytes_read%sizeof(prueba::PruebaV2Fragmento::Header::dato_t)==0);
		prueba::PruebaV2Fragmento::Header* header=reinterpret_cast<prueba::PruebaV2Fragmento::Header*>(buffer);
		header->freq_muestreo=freq_muestreo_;
		header->tam_evento=*bytes_read/sizeof(prueba::PruebaV2Fragmento::Header::dato_t);
		adc_t* adc_read=reinterpret_cast<adc_t*>(header+1);
		for(size_t i=0;i<nADCcounts_;i++) {
			adc_read[i]=amplitud_*log(multiplicador_*i);
		}
	}	
}

void PruebaV2HardwareInterface::AllocateBuffer(adc_t** buffer) const{
	*buffer=reinterpret_cast<adc_t*>(new adc_t[sizeof(prueba::PruebaV2Fragmento::Header::dato_t)+nADCcounts_*sizeof(adc_t)]);

}

void PruebaV2HardwareInterface::FreeReadoutBuffer(adc_t* buffer) const{
	delete[] buffer;
}

int PruebaV2HardwareInterface::SerialNumber()  {
	return 456;
}

int PruebaV2HardwareInterface::NumADCBits()  {
	return 10;
}








