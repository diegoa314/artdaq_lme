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
	, nADCcounts_(ps.get<size_t>("nADCcounts", 40))
	, maxADCcounts_(ps.get<size_t>("maxADCcounts", 50000000))
	, change_after_N_seconds_(ps.get<size_t>("change_after_N_seconds",
											 std::numeric_limits<size_t>::max()))
	, nADCcounts_after_N_seconds_(ps.get<int>("nADCcounts_after_N_seconds",
											  nADCcounts_))
	, exception_after_N_seconds_(ps.get<bool>("exception_after_N_seconds",
											  false))
	, exit_after_N_seconds_(ps.get<bool>("exit_after_N_seconds",
										 false))
	, abort_after_N_seconds_(ps.get<bool>("abort_after_N_seconds",
										  false))
	, fragment_type_(demo::toFragmentType(ps.get<std::string>("fragment_type")))
	, maxADCvalue_(pow(2, NumADCBits()) - 1)
	, // MUST be after "fragment_type"
	throttle_usecs_(ps.get<size_t>("throttle_usecs", 100000))
	, usecs_between_sends_(ps.get<size_t>("usecs_between_sends", 0))
	, start_time_(fake_time_)
	, send_calls_(0)
	, serial_number_(0)

{ 
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"

/*Se verificaran sino se superan la cantidad maxima de cuentas.
* Como que nADC_counts_after_N_seconds es int, al compararlo con maxADCcounts que es unsgined
* se debe verificar que nADC sea positivo para 
* evitar errores en la comparacion*/
	if (nADCcounts_>maxADCcounts_ || (nADCcounts_after_N_seconds_>0 && nADCcounts_after_N_seconds_ > maxADCcounts_ )) {
		throw cet::exception("Hardware INterface")<< "nADCcounts o nADCcunts_after_N_seconds es mayor que maxADCcounts";
	}
	
	bool planned_disruption = nADCcounts_after_N_seconds_ != nADCcounts_ ||
		exception_after_N_seconds_ ||
		exit_after_N_seconds_ ||
		abort_after_N_seconds_;

	if (planned_disruption &&
		change_after_N_seconds_ == std::numeric_limits<size_t>::max())
	{
		throw cet::exception("HardwareInterface") << "A FHiCL parameter designed to create a disruption has been set, so \"change_after_N_seconds\" should be set as well";
	#pragma GCC diagnostic pop
	}

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

void PruebaHardwareInterface::FillBuffer(char* buffer, size_t*bytes_read) {
	if (taking_data_){
		usleep(throttle_usecs_); //se hace una pausa en el sistema
		auto elapsed_time_since_datataking_start=artdaq::TimeUtils::GetElapsedTime(start_time_); //el tiempo trnascurrido desde que se inicio
		if (elapsed_time_since_datataking_start < change_after_N_seconds_) {
			*bytes_read=sizeof(prueba::PruebaFragmento::Header)+nADCcounts_*sizeof(data_t); 
		}
		else {
			if (abort_after_N_seconds_) {
				std::abort(); //se aborta el programa
			}
			else if (exit_after_N_seconds_) {
				std::exit(1);	 
			}
			else if (exception_after_N_seconds_) {
				throw cet::exception("HardwareInterface")<<"Excepcion de prueba";
			}
			else if	(nADCcounts_after_N_seconds_>=0) {
				*bytes_read=sizeof(prueba::PruebaFragmento::Header)+nADCcounts_after_N_seconds_*sizeof(data_t);
			}
			else {
				//se colgo
				while(true) {};
			}
		}
	
		/*El tamaño del fragmento debe ser multiplo de Header::dato_t, unidad basica de dato*/
		assert(*bytes_read % sizeof(Prueba::PruebaFragmento::Header::dato_t) == 0);
		//Se crea el header con la direccion del buffer
		prueba::PruebaFragmento::Header* header=reinterpret_cast<prueba::PruebaFragmento::Header*>(buffer);			
		//llenamos el header
		header->periodo=100; //por poner un valor
		header->canales_act=3;
		header->tam_evento=*bytes_read/sizeof(prueba::PruebaFragmento::Header::dato_t);	
		data_t* adc_read=reinterpret_cast<data_t*>(header+1);
		//se generan los datos
		for (size_t i=0;i<nADCcounts_;i++) {
			adc_read[i]=i+65;
			TLOG(TLVL_INFO) << adc_read[i];
		}
	}
}

void PruebaHardwareInterface::AllocateBuffer (char** buffer) {
	*buffer=reinterpret_cast<char*>(new uint8_t[sizeof(prueba::PruebaFragmento::Header)+maxADCcounts_*sizeof(data_t)]);
	
}

void PruebaHardwareInterface::FreeReadoutBuffer(char* buffer) {
	delete[] buffer; 	
}

int PruebaHardwareInterface::SerialNumber() const {
	return 123;
}

int PruebaHardwareInterface::NumADCBits() const {
	return 8;
}	



	
	


