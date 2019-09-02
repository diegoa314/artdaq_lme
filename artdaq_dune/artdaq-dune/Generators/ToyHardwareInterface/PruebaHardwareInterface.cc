#include "artdaq-dune/Generators/ToyHardwareInterface/PruebaHardwareInterface.hh"
#define TRACE_NAME "PruebaHardwareInterface"
#include "artdaq/DAQdata/Globals.hh"
#include "artdaq-core-dune/Overlays/PruebaFragment.hh"
#include "artdaq-core-dune/Overlays/FragmentType.hh"

#include "fhiclcpp/ParameterSet.h"
#include "cetlib_except/exception.h"

#include <random>
#include <unistd.h>
#include <iostream>
#include <cstdlib>

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
	, distribution_type_(static_cast<DistributionType>(ps.get<int>("distribution_type")))
	, engine_(ps.get<int64_t>("random_seed", 314159))
	, uniform_distn_(new std::uniform_int_distribution<data_t>(0, maxADCvalue_))
	, gaussian_distn_(new std::normal_distribution<double>(0.5 * maxADCvalue_, 0.1 * maxADCvalue_))
	, start_time_(fake_time_)
	, send_calls_(0)
	, serial_number_((*uniform_distn_)(engine_))

{ /*Se verificaran sino se superan la cantidad maxima de cuentas.
* Como que nADC_counts_after_N_seconds es int, al compararlo con maxADCcounts que es unsgined
* se debe verificar que nADC sea positivo para 
* evitar errores en la comparacion*/
	if (nADCcounts_>maxADCcounts_ || (nADCcounts_after_N_seconds_>0 && nADCcounts_after_N_seconds > maxADCcounts) {
		throw cet::exception("Hardware INterface")<< "nADCcounts o nADCcunts_after_N_seconds es mayor que maxADCcounts";
	
	
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
void PruebaHardwareInterface::StartDatataking () {
	taking_data_=true;
	send_calls_=0;
}			

void PruebaHardwareInterface::StopDatataking () {
	taking_data_=false;
	start_time_=fake_time_; //toma el valor de fake_time_ que es el maximo valor posible	
}

void PruebaHardwareInterface::FillBuffer(char* Buffer, size_t*bytes_read) {
	if (data_taking_){
		usleep(throttle_usecs_); //se hace una pausa en el sistema
		auto elapsed_time_since_datataking_start=artdaq::TimeUtils::GetElapsedTime(start_time_); //el tiempo trnascurrido desde que se inicio
		if (elapsed_time_since_datataking_start < change_after_N_seconds_) {
			*bytes_read=sizeof(Prueba::PruebaFragment::Header)+nADCcounts_*sizeof(data_t); 
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
			else if	(nADC_counts_after_N_seconds_>=0) {
				*bytes_read=sizeof(Prueba::PruebaFragment::Header)+n_ADCcounts_after_N_seconds*sizeof(data_t);
			}
			else {
				//se colgo
				while(true) {};
			}
		}
	}
	/*El tamaño del fragmento debe ser multiplo de Header::dato_t, unidad basica de dato*/
	assert(*bytes_read % sizeof(Prueba::PruebaFragmento::Header::dato_t) == 0);
	//Se crea el header con la direccion del buffer
	Prueba::PruebaFragmento::Header* header=reinterpret_cast<Prueba:PruebaFragment::Header*>(buffer);			
	//llenamos el header
	header->periodo=100; //por poner un valor
	header->canales_act=3;
	header->tam_evento=*bytes_read/sizeof(Prueba::PruebaFragmento::Header::dato_t);
	data_t* adc_read=reinterpret_cast<data_t*>(header+1);

	


