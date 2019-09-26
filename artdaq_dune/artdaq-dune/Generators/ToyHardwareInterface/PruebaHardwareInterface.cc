#include "artdaq-dune/Generators/ToyHardwareInterface/PruebaHardwareInterface.hh"
#define TRACE_NAME "PruebaHardwareInterface"
#include "artdaq/DAQdata/Globals.hh"
#include "artdaq-core-dune/Overlays/PruebaFragmento.hh"
#include "artdaq-core-dune/Overlays/FragmentType.hh"

#include "fhiclcpp/ParameterSet.h"
#include "cetlib_except/exception.h"

#include <random>
#include <unistd.h>
#include <iostream>
#include <cstdlib>
#include <cmath>

#define PI 3.14159265

PruebaHardwareInterface::PruebaHardwareInterface(fhicl::ParameterSet const& ps) :
	taking_data_(false)
	, nADCcounts_(ps.get<size_t>("nADCcounts", 100))
	, maxADCcounts_(ps.get<size_t>("maxADCcounts", 50000000))
	, frecuencia_(ps.get<double>("frecuencia", 20000))
	, amplitud_(ps.get<double>("amplitud",50))
	, fragment_type_(demo::toFragmentType(ps.get<std::string>("fragment_type")))
	, maxADCvalue_(pow(2, NumADCBits()) - 1)
	, // MUST be after "fragment_type"
	throttle_usecs_(ps.get<size_t>("throttle_usecs", 100000))
	, usecs_between_sends_(ps.get<size_t>("usecs_between_sends", 0))
	, start_time_(fake_time_)
	, send_calls_(0)
	, serial_number_(0){
		periodo_muestreo_=1;
}



//se dice al hardware para enviar o parar el envio de datos
void PruebaHardwareInterface::StartDatataking() {
	taking_data_=true;
	send_calls_=0;
}			

void PruebaHardwareInterface::StopDatataking() {
	taking_data_=false;
	start_time_=fake_time_; //toma el valor de fake_time_ que es el maximo valor posible	
}

void PruebaHardwareInterface::FillBuffer(data_t* buffer, size_t*bytes_read) {
	if (taking_data_){
		*bytes_read=sizeof(prueba::PruebaFragmento::Header)+nADCcounts_*sizeof(data_t); 
		/*El tamaño del fragmento debe ser multiplo de Header::dato_t, unidad basica de dato*/
		assert(*bytes_read % sizeof(Prueba::PruebaFragmento::Header::dato_t) == 0);
		//Se crea el header con la direccion del buffer
		prueba::PruebaFragmento::Header* header=reinterpret_cast<prueba::PruebaFragmento::Header*>(buffer);			
		//llenamos el header
		header->periodo=periodo_muestreo_; //por poner un valor
		header->canales_act=3;
		header->freq_muestreo=100;
		header->tam_evento=*bytes_read/sizeof(prueba::PruebaFragmento::Header::dato_t);	
		data_t* adc_read=reinterpret_cast<data_t*>(header+1);
		
		//se generan los datos
		for (size_t i=0;i<nADCcounts_;i++) {
			adc_read[i]=(uint8_t)(amplitud_*(1+sin(2*PI*frecuencia_*i)));
			
			TLOG(TLVL_INFO) << adc_read[i];
			
		}
	}
	periodo_muestreo_=periodo_muestreo_+nADCcounts_;
}

void PruebaHardwareInterface::AllocateBuffer (data_t** buffer) {
	*buffer=reinterpret_cast<data_t*>(new uint8_t[sizeof(prueba::PruebaFragmento::Header)+nADCcounts_*sizeof(data_t)]);
	
}

void PruebaHardwareInterface::FreeReadoutBuffer(data_t* buffer) {
	delete[] buffer; 	
}

int PruebaHardwareInterface::SerialNumber() const {
	return 123;
}

int PruebaHardwareInterface::NumADCBits() const {
	return 8;
}	



	
	


