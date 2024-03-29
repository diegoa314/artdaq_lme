#ifndef artdaq_dune_Generators_PruebaHardwareInterface_ToyHardwareInterface_hh
#define artdaq_dune_Generators_PruebaHardwareInterface_ToyHardwareInterface_hh

#include "artdaq-core-dune/Overlays/FragmentType.hh"

#include "fhiclcpp/fwd.h"

#include <random>
#include <chrono>

class PruebaHardwareInterface {
public:
	typedef uint8_t data_t; /*El tipo de dato para representar las
				*cuentas del ADC*/
	/*El constructor recibe un parameterset con las configuraciones*/
	explicit PruebaHardwareInterface(fhicl::ParameterSet const & ps);
	//StartDatataking avisa al hardware para comenzar el envio de datos	
	void StartDatataking();
	//StopDatataking realiza acciones del shutdown
	void StopDatataking();
	/*FillBuffer se encarga de llenar un buffer con los bytes 
 	*leidos. Buffer: Buffer a llenar, bytes_read: numero de bytes
	* a llenar */
	void FillBuffer(char* Buffer, size_t* bytes_read);
	/*AllocateBuffer solicita un buffer. buffer: Puntero al buffer
 	*(puntero a puntero) 	*/
	void AllocateBuffer(char** buffer);	
	/*FreeReadoutBuffer libera el buffer al hardware. buffer: buffer
 	*a liberar*/
	void FreeReadoutBuffer(char* buffer);
	/*SerialNumber retorna el numero serial de la placa simulada.
 	*Esta y las demas son funciones const porque no modifican ningun
	* miembro de esta clase. Es simplemente buena practica definirlas
	* asi 	*/
	int SerialNumber() const;
	/*NumADCBits retorna el numero de bits usados*/
	int NumADCBits() const;
	/*BoardType retorna el tipo de placa usado*/
	int BoardType() const;
private:
	bool taking_data_; //se toman datos?
	std::size_t nADCcounts_; //cantidad de cuentas ADC
	std::size_t maxADCcounts_; //cantidad max de cuentas ADC
	//los sgtes ni idea
	std::size_t change_after_N_seconds_;
	int nADCcounts_after_N_seconds_;
	bool exception_after_N_seconds_;
	bool exit_after_N_seconds_;
	bool abort_after_N_seconds_;
	demo::FragmentType fragment_type_;
	std::size_t maxADCvalue_;
	std::size_t throttle_usecs_;
	std::size_t usecs_between_sends_;
	//DistributionType distribution_type_;

	using time_type = decltype(std::chrono::steady_clock::now());

	const time_type fake_time_ = std::numeric_limits<time_type>::max();

	// Members needed to generate the simulated data
/*
	std::mt19937 engine_;
	std::unique_ptr<std::uniform_int_distribution<data_t>> uniform_distn_;
	std::unique_ptr<std::normal_distribution<double>> gaussian_distn_;
*/
	time_type start_time_;
	int send_calls_;
	int serial_number_;




};


#endif
