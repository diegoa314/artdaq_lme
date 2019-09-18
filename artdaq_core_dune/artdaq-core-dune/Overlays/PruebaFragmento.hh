#ifndef artdaqdune_Overlays_PruebaFragmento
#define artdaqdune_Overlays_PruebaFragmento

#include "artdaq-core/Data/Fragment.hh"
#include <ostream>

namespace prueba {
	class PruebaFragmento;
	//sobrecarga del operador << para que se pueda imprimir informacion sobre PruebaFragmento
	std::ostream & operator << (std::ostream &, PruebaFragmento const &);
}

class prueba::PruebaFragmento {
public:
	/*en el metadato ponemos los datos relacionados al hardware	
	como el serial de la placa, numero de bits del adc y demas*/
	struct Metadato { 
		typedef uint32_t dato_t;
		dato_t serial:16;
		dato_t n_adc_bits:8; /*Define el numero de bits que 
		verdaderamente nos interesa de cada muestra */
		dato_t unused:8;
		static size_t const tam_palabra=1ul; //el tamaño del metadato en terminos de dato_t
	};
	static_assert(sizeof(Metadato)==Metadato::tam_palabra*sizeof(Metadato::dato_t), "El tamaño de palabra del metadato ha cambiado"); 
	
	typedef uint8_t adc_t; /*Esta variable define el numero de bits
	de cada muestra del ADC, no necesariamente se usaran todos 
	estos bits, algunos pueden ser siempre cero o informacion basura
	 */

	/*Definimos el header el cual contiene informacion sobre los datos
 * leidos en cada evento, como el periodo, el tamaño del dato a leer, 
 * canales activos*/
	struct Header {
		typedef uint32_t dato_t; //tamaño del header
		dato_t periodo: 8;
		dato_t canales_act: 8;
		dato_t tam_evento: 16; /* esto define el tamaño del 
		payload en terminos de dato_t. Esto significa que como
		maximo se tendran 2**16 dato_t o 2**16 * 32 bits de 
		payload. payload es header mas cuentas adc  */
		static size_t const tam_palabra=1ul;
	};
	static_assert(sizeof(Header)==sizeof(Header::dato_t)*Header::tam_palabra,"El tamaño del header ha cambiado");
	/* Se define el constructor de la clase que recibe como parametro
 * un Fragment y lo guarda en una variable privada*/
	explicit PruebaFragmento(artdaq::Fragment const& f):fragmento(f){}
	
	
	Header::dato_t hdr_tam_fragmento() const {
		return header_()->tam_evento;
	}
	static constexpr size_t hdr_size_words() { 
		return Header::tam_palabra; 
	}
	size_t total_adc_values() const
	{
		return (hdr_event_size() - hdr_size_words()) * adcs_per_word_();
	}
	
	adc_t const* dataBeginADCs() const
	{
		return reinterpret_cast<adc_t const *>(header_() + 1);
	}
	adc_t const* dataEndADCs() const
	{
		return dataBeginADCs() + total_adc_values();
	}

	Header::dato_t hdr_event_size() const { 
		return header_()->tam_evento; 
	}

	adc_t adc_value(uint32_t index)
	{
		// Simple way to handle index out of bounds - better ways are surely possible
		if (index >= total_adc_values()) { return 0xff; }
		return dataBeginADCs()[index];
	}

protected:
	Header const * header_() const {
		return reinterpret_cast<PruebaFragmento::Header const *>(fragmento.dataBeginBytes()); /*Lo que se hace aca es hacer que la direccion del 
	comienzo del Header coincida con la direccion del comienzo del
	buffer, conviritiendo el dataBeginBytes que da la direccion del
	buffer a un puntero del tipo Header */
	}
	
	static constexpr size_t adcs_per_word_()
	{
		return sizeof(Header::dato_t) / sizeof(adc_t);
	}

private:
	artdaq::Fragment const & fragmento; //referencia al fragmento


	

};
#endif

