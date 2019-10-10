// For an explanation of this class, look at its header,
// ToySimulator.hh, as well as
// https://cdcvs.fnal.gov/redmine/projects/artdaq-demo/wiki/Fragments_and_FragmentGenerators_w_Toy_Fragments_as_Examples

#include "artdaq-dune/Generators/PruebaV2Simulator.hh"

#include "canvas/Utilities/Exception.h"

#include "artdaq/Application/GeneratorMacros.hh"
#include "artdaq-core/Utilities/SimpleLookupPolicy.hh"

#include "artdaq-core-dune/Overlays/PruebaV2Fragmento.hh"
#include "artdaq-core-dune/Overlays/FragmentType.hh"

#include "fhiclcpp/ParameterSet.h"

#include <fstream>
#include <iomanip>
#include <iterator>
#include <iostream>

#include <unistd.h>
#define TRACE_NAME "PruebaV2Simulator"
#include "tracemf.h"		// TRACE, TLOG*
#include "cetlib_except/exception.h"

prueba::PruebaV2Simulator::PruebaV2Simulator(fhicl::ParameterSet const& ps):
	CommandableFragmentGenerator(ps)
	, hardware_interface_(new PruebaV2HardwareInterface(ps))
	, timestamp_(0)
	, timestampScale_(ps.get<int>("timestampScale", 1))
	, metadato_({0,0})
	, readout_buffer_(nullptr)
	, fragment_type_(static_cast<decltype(fragment_type_)>(artdaq::Fragment::InvalidFragmentType))
{
	hardware_interface_->AllocateBuffer(&readout_buffer_);
	metadato_.serial=hardware_interface_->SerialNumber();
	metadato_.n_adc_bits=hardware_interface_->NumADCBits();
	fragment_type_=demo::toFragmentType("PRUEBAV2");
}

prueba::PruebaV2Simulator::~PruebaV2Simulator() {
	hardware_interface_->FreeReadoutBuffer(readout_buffer_);
}

bool prueba::PruebaV2Simulator::getNext_(artdaq::FragmentPtrs& frags) {
	if(should_stop()){
		return false;
	}	
	std::size_t bytes_read=0;
	hardware_interface_->FillBuffer(readout_buffer_, &bytes_read);
	artdaq::FragmentPtr fragptr (artdaq::Fragment::FragmentBytes(bytes_read, ev_counter(), fragment_id(), fragment_type_, metadato_, timestamp_));
	frags.emplace_back(std::move(fragptr));
	memcpy(frags.back()->dataBeginBytes(),readout_buffer_, bytes_read);
	artdaq::CommandableFragmentGenerator::ev_counter_inc();
	timestamp_+=timestampScale_;
	return true;
}
void prueba::PruebaV2Simulator::start() {
	hardware_interface_->StartDatataking();
	timestamp_=0;	
}	

void prueba::PruebaV2Simulator::stop() {
	hardware_interface_->StopDatataking();
}

void prueba::PruebaV2Simulator::stopNoMutex() {
	hardware_interface_->StopDatataking();
}

DEFINE_ARTDAQ_COMMANDABLE_GENERATOR(prueba::PruebaV2Simulator)

















