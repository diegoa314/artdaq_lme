// For an explanation of this class, look at its header,
// ToySimulator.hh, as well as
// https://cdcvs.fnal.gov/redmine/projects/artdaq-demo/wiki/Fragments_and_FragmentGenerators_w_Toy_Fragments_as_Examples

#include "artdaq-dune/Generators/PruebaSimulator.hh"

#include "canvas/Utilities/Exception.h"

#include "artdaq/Application/GeneratorMacros.hh"
#include "artdaq-core/Utilities/SimpleLookupPolicy.hh"

#include "artdaq-core-dune/Overlays/PruebaFragment.hh"
#include "artdaq-core-dune/Overlays/FragmentType.hh"

#include "fhiclcpp/ParameterSet.h"

#include <fstream>
#include <iomanip>
#include <iterator>
#include <iostream>

#include <unistd.h>
#define TRACE_NAME "PruebaSimulator"
#include "tracemf.h"		// TRACE, TLOG*
#include "cetlib_except/exception.h"

prueba::PruebaSimulator::PruebaSimulator(fhicl::ParameterSet const& ps)
	:
	CommandableFragmentGenerator(ps)
	, hardware_interface_(new PruebaHardwareInterface(ps))
	, timestamp_(0)
	, timestampScale_(ps.get<int>("timestamp_scale_factor", 1))
	, metadato_({ 0,0,0 })
	, readout_buffer_(nullptr)
	, fragment_type_(static_cast<decltype(fragment_type_)>(artdaq::Fragment::InvalidFragmentType)) //??
{
	//se reserva espacio en el buffer
	hardware_interface_->AllocateReadoutBuffer(&readout_buffer_);
	//se carga el metadato	
	metadato_.board_serial_number = hardware_interface_->SerialNumber() & 0xFFFF;
	metadato_.num_adc_bits = hardware_interface_->NumADCBits();
	//informacion de debugging
	TLOG(TLVL_INFO) << "Constructor: metadata_.unused = 0x" << std::hex << metadata_.unused << " sizeof(metadata_) = " << std::dec << sizeof(metadata_);
}

prueba::PruebaSimulator::~PruebaSimulator() {
	//en el destructor liberamos memoria
	hardware_interface_->FreeReadoutBuffer(readout_buffer_);
}

bool prueba::PruebaSimulator::getNext_(artdaq::FragmentPtrs& frags) 
{	
	/*FragmentPtrs se define en la clase /products/artdaq_core/
 	*v3_05/source/artdaq-core/Fragment.hh como:
	* typedef std::unique_ptr<Fragment> FragmentPtr;
	* typedef std::list<FragmentPtr> FragmentPtrs*/
		
	/*should_stop() esta definido en la clase commandable y 
 	* retorna el valor de la bandera should_stop_*/
	if (should_stop()) { 
		return false;
	}		 	
	std::size_t bytes_read=0 //cantidad de bytes a leer
	hardware_interface_->FillBuffer(readout_buffer_, &bytes_read);
	/* Para crear el fragmento se usa el static factory function
 	* de la clase Fragment.
 	*
 	* template <class T>
 	*static FragmentPtr FragmentBytes(std::size_t payload_size_in_		* bytes, sequence_id_t sequence_id,fragment_id_t fragment_id
	*, type_t type, const T& metadata,timestamp_t timestamp = Frag		* ment::InvalidTimestamp)
 	*
 	* \param sequence_id Sequence ID of Fragment
	* \param fragment_id Fragment ID of Fragment
 	* \param type Type of Fragment
 	* \param metadata Metadata object
 	* \param timestamp Timestamp of Fragment
 	* \return FragmentPtr to created Fragment
	*/
	artdaq::FragmentPtr fragptr artdaq::Fragment::FragmentBytes(bytes_read, ev_counter(), fragment_id(),fragment_type_, metadato_, timestamp);
	frags.emplace_back(std::move(fragptr)); //ver el concepto de mve
	//dataBeginBytes() retorna la direccion donde comienza el payload del fragmento
	memcpy(frags.back()->dataBeginBytes(),readout_buffer_, bytes_read);	
	eva_counter_inc();
	timestamp_+=timestampScale_;
		 	
}

void prueba::PruebaSimulator::start() {
	harware_interface_->StartDatataking();
	timestamp_=0;
}

void prueba::PruebaSimulator::stop() {
	hardware_interface_->StopDatataking;
}

//Macro definido en GeneratorMacros.hh
DEFINE_AARTDAQ_COMMANDABLE_GENERATOR(prueba::ToySimulator)



