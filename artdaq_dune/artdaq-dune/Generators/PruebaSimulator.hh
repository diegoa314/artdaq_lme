#ifndef artdaq_dune_Generators_PruebaSimulator_hh
#define artdaq_dune_Generators_PruebaSimulator_hh

// Some C++ conventions used:

// -Append a "_" to every private member function and variable

#include "fhiclcpp/fwd.h"
#include "artdaq-core/Data/Fragment.hh"
#include "artdaq/Application/CommandableFragmentGenerator.hh"
#include "artdaq-core-dune/Overlays/PruebaFragment.hh"
#include "artdaq-core-dune/Overlays/FragmentType.hh"

#include "ToyHardwareInterface/ToyHardwareInterface.hh"

#include <random>
#include <vector>
#include <atomic>
namespace prueba {
	//la clase siempre debe heredar de commandablefragmentgenerator
	class PruebaSimulator : public artdaq::CommandableFragmentGenerator{
	public:
		/*El constructor de la clase recibe una referencia al 
		 * parameterset el cual contiene informacion sobre las 
		 * configuraciones.
 		 * Se tiene un constructor explicito para que no haga
 		 * una conversion implicita de los argumentos*/
		explicit PruebaSimulator(fhicl::ParameterSet const & ps);
		/*El destructor de la clase que es llamado con shutdown
		 * Debe ser virtual, ya que tenemos al menos una funcion
		 * virtual*/
		virtual ~PruebaSimulator();
 	private:
		/*Las funciones getNext_, start, stop y stopNoMutex deben
 		*ser sobrescrita al ser funciones virtuales puras.
		* GetNext_ retorna true si la toma de 
		* datos debe continuar. Recibe como parametro un vector
		* de fragmentos*/
		bool getNext_(artdaq::FragmentPtrs & output) override;	
		void start() override;
		void stop() override;
		void stopNoMutex() override;
		
		std::unique_ptr<PruebaHardwareInterface> hardware_interface_;
		//estampa temporal que el generador debe agregar al frag
		artdaq::Fragment::timestamp_t timestamp_; 
		int timestampScale; //escala de la estampa
		PruebaFragmento::Metadato metadato_;
		/*readout_buffer_ es un puntero que apunta al buffer
 		* que es llenado por el hardware interface*/
		char* readout_buffer_;
		//tipo de fragmento
		FragmentType fragment_type_;
	 
	};		
}


#endif
