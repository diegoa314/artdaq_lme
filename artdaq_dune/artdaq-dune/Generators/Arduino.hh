#ifndef sipmdaq_Generator_Arduino_hh
#define sipmdaq_Generator_Arduino_hh

#include "fhiclcpp/fwd.h"
#include "artdaq-core/Data/Fragment.hh"
#include "artdaq/Application/CommandableFragmentGenerator.hh"
#include "artdaq-core-demo/Overlays/ArduFragment.hh"
#include "artdaq-core-demo/Overlays/FragmentType.hh"

#include "ToyHardwareInterface/ArduHardware.hh"

#include <vector>

namespace sipm
{

  class Arduino : public artdaq::CommandableFragmentGenerator
  {
  public:

   explicit Arduino(fhicl::ParameterSet const& p);
   virtual ~Arduino();

  private:

    bool getNext_(artdaq::FragmentPtrs& salida) override;
    void start() override;
    void stop() override;
    void stopNoMutex() override {};
    
    std::unique_ptr<ArduHardware> interfaz_hardware;
    artdaq::Fragment::timestamp_t estampa_temporal = 0;


    ArduFragment::Metadata metadata_;
    
    sipm::ArduFragment::ardu_byte* buffer_lectura;
    demo::FragmentType tipoFragmento;

  };
}

#endif
