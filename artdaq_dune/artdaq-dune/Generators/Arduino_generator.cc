#include "artdaq-demo/Generators/Arduino.hh"

#include "canvas/Utilities/Exception.h"

#include "artdaq/Application/GeneratorMacros.hh"
#include "artdaq-core/Utilities/SimpleLookupPolicy.hh"

#include "artdaq-core-demo/Overlays/ArduFragment.hh"
#include "artdaq-core-demo/Overlays/FragmentType.hh"

#include "cetlib/exception.h"
#include "fhiclcpp/ParameterSet.h"

#include <fstream>
#include <iomanip>
#include <iterator>
#include <iostream>
#include <chrono>

#include <unistd.h>
#include "trace.h"

sipm::Arduino::Arduino(fhicl::ParameterSet const& p)
  :CommandableFragmentGenerator(p),
   interfaz_hardware(new ArduHardware(p)),
   buffer_lectura(nullptr),
   tipoFragmento(static_cast<decltype(tipoFragmento)>(artdaq::Fragment::InvalidFragmentType))
{

   interfaz_hardware->reservarBuffer(&buffer_lectura);

   metadata_.num_serial_ardu = interfaz_hardware->getNumeroSerial();
   metadata_.num_canales = interfaz_hardware->getNumeroCanales();
   tipoFragmento = demo::toFragmentType("ARDU");
}

sipm::Arduino::~Arduino(){

   interfaz_hardware->liberarBuffer(buffer_lectura);
   interfaz_hardware->desconectar();

}

bool sipm::Arduino::getNext_(artdaq::FragmentPtrs& salida){

if(should_stop()) { return false;}

  std::size_t bytes_leidos = 0;

  interfaz_hardware-> llenarBuffer(buffer_lectura,&bytes_leidos);
  auto tiempo = std::chrono::steady_clock::now();
  estampa_temporal = tiempo.time_since_epoch().count();
  std::unique_ptr<artdaq::Fragment> fragptr(
					   artdaq::Fragment::FragmentBytes(bytes_leidos,
									   ev_counter(),
									   fragment_id(),
									   tipoFragmento,
									   metadata_,
									   estampa_temporal));
  salida.emplace_back(std::move(fragptr));
  memcpy(salida.back()->dataBeginBytes(), buffer_lectura, bytes_leidos);
  ev_counter_inc();
  TLOG_DEBUG("ARDUINO")<< "HOLA DESDE ARDU GEN" << TLOG_ENDL;
  return true;

}

void sipm::Arduino::start(){

  interfaz_hardware->inicializarTomaDeDatos();

}

void sipm::Arduino::stop(){

  interfaz_hardware->finalizarTomaDeDatos();

}

DEFINE_ARTDAQ_COMMANDABLE_GENERATOR(sipm::Arduino)
