#include "artdaq-demo/Generators/ToyHardwareInterface/ArduHardware.hh"
#include "artdaq/DAQdata/Globals.hh"

#include "artdaq-core-demo/Overlays/FragmentType.hh"
#include "artdaq-core-demo/Overlays/ArduFragment.hh"

#include "fhiclcpp/ParameterSet.h"
#include "cetlib/exception.h"

#include <unistd.h>
#include <cstdlib>

#include <string.h>

ArduHardware::ArduHardware(fhicl::ParameterSet const& p)
  :tomaDatos(false),
   conteoData(p.get<size_t>("conteoData",4)),
   numeroSerial(0),
   numeroCanales(2),
   numeroCanalesActivos(p.get<uint8_t>("CanalesActivos", 2)),
   perConteo(p.get<uint32_t>("perConteo",1000))
{

   Ardu.SetBaudRate(SerialStreamBuf::BAUD_57600);
   Ardu.SetCharSize(SerialStreamBuf::CHAR_SIZE_8);
   Ardu.SetNumOfStopBits(1);
   Ardu.SetVTime(20);
   Ardu.SetParity(SerialStreamBuf::PARITY_NONE);
   conectar();

}
  
void ArduHardware::inicializarTomaDeDatos(){

  tomaDatos = true;
  configurar();
  
}

void ArduHardware::finalizarTomaDeDatos(){

  tomaDatos = false;

}

void ArduHardware::reservarBuffer(data_t** buffer){

  size_t Bytes_a_Recibir = (size_t)(numeroCanalesActivos)*conteoData;
  *buffer =  reinterpret_cast<data_t*>(new uint8_t[sizeof(sipm::ArduFragment::Header) + Bytes_a_Recibir*sizeof(data_t)]);

}

void ArduHardware::liberarBuffer(data_t* buffer){

  delete[] buffer;

}

void ArduHardware::llenarBuffer(data_t* buffer, size_t* bytes_leidos){

  if(tomaDatos){
    
    size_t Bytes_a_Recibir = (size_t)(numeroCanalesActivos)*conteoData;
    *bytes_leidos = sizeof(sipm::ArduFragment::Header) + Bytes_a_Recibir*sizeof(data_t);
    assert(*bytes_leidos % sizeof(sipm::ArduFragment::Header::data_t) ==0);
    sipm::ArduFragment::Header* header = reinterpret_cast<sipm::ArduFragment::Header*>(buffer);
    header->tam_fragmento = *bytes_leidos / sizeof(sipm::ArduFragment::Header::data_t);
    header->per_conteo = getPerConteo();
    header->canales_activos = getNumeroCanalesActivos();
    data_t* bufferptr = reinterpret_cast<data_t*>(reinterpret_cast<sipm::ArduFragment::Header*>(buffer) + 1);
    for(size_t i = 0; i<Bytes_a_Recibir;i++){
      /*TLOG_INFO("Arduino") << "i_bytes: "
      << "Numero de bytes recepcionados = " << i
      << "." << TLOG_ENDL;*/
      bufferptr[i] = Ardu.get();

    }
  }
}

ArduHardware::~ArduHardware(){
 
   desconectar();
   Ardu.~SerialStream();
}

void ArduHardware::conectar(){

  Ardu.Open("/dev/ttyACM0");
    
    if(Ardu.IsOpen()){
      std::cout<<"Exito al abrir el puerto!"<<std::endl;
    }
    else{
      throw cet::exception("Fallo al abrir el puerto");
    }
}

void ArduHardware::desconectar(){

  Ardu.Close();

  if(!Ardu.IsOpen()){
     std::cout<<"Exito al cerrar el puerto!"<<std::endl;
  }
  else{
     throw cet::exception("Fallo al cerrar el puerto");
  }
}

void ArduHardware::configurar(){
    
  
  uint8_t perConteo_[4] = {0};
  long2bytes(perConteo,perConteo_);
  Ardu << 'r';
  Ardu << perConteo_;
  Ardu << numeroCanalesActivos;
    
}

void ArduHardware::long2bytes(uint32_t val, uint8_t* byte_array){
  union{
    uint32_t long_variable;
    char temp_array[4];
  }u;
  u.long_variable = val;
  memcpy(byte_array,u.temp_array,4);
}

uint32_t ArduHardware::getNumeroSerial() const {

  return numeroSerial;

}

uint8_t ArduHardware::getNumeroCanales() const {

  return numeroCanales;

}

uint8_t ArduHardware::getNumeroCanalesActivos() const {

  return numeroCanalesActivos;

}

uint32_t ArduHardware::getPerConteo() const {

  return perConteo;

}
 
