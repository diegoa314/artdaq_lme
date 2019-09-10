#ifndef sipmdaq_Generators_HardwareInterface_ArduHardware_hh
#define sipmdaq_Generators_HardwareInterface_ArduHardware_hh

#include "artdaq-core-demo/Overlays/FragmentType.hh"
#include "artdaq-core-demo/Overlays/ArduFragment.hh"
#include "fhiclcpp/fwd.h"

#include <SerialStream.h>
#include <sstream>
#include <iostream>

using namespace LibSerial;

class ArduHardware
{

public:

  typedef sipm::ArduFragment::ardu_byte data_t;

  explicit ArduHardware(fhicl::ParameterSet const& p);
  ~ArduHardware();
  void conectar();
  void desconectar();
  void configurar();
  void long2bytes(uint32_t val, uint8_t* byte_array);
  void inicializarTomaDeDatos();
  void finalizarTomaDeDatos();
  void llenarBuffer(data_t* buffer, size_t* bytes_leidos);
  void reservarBuffer(data_t** buffer);
  void liberarBuffer(data_t* buffer);
  uint32_t getNumeroSerial() const;
  uint8_t getNumeroCanales() const;
  uint8_t getNumeroCanalesActivos() const;
  uint32_t getPerConteo() const;

private:

  bool tomaDatos;
  std::size_t conteoData;
  const uint32_t numeroSerial;
  const uint8_t numeroCanales;
  const uint8_t numeroCanalesActivos;
  const uint32_t perConteo;

  SerialStream Ardu;
};  

#endif
