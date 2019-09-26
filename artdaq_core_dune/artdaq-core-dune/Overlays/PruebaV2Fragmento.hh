#ifndef artdaqdune_Overlays_PruebaV2Fragmento
#define artdaqdune_Overlays_PruebaV2Fragmento

#include "artdaq-core/Data/Fragment.hh"
#include <ostream>
namespace prueba {
	class PruebaV2Fragmento;	
	std::ostream & operator << (std::ostream&, PruebaV2Fragmento const &);
}
class prueba::PruebaV2Fragmento {
public:
	struct Metadato {
		typedef uint16_t dato_t;
		dato_t serial:6;
		dato_t n_adc_bits:10;
		static size_t const tam_palabra=1u;
	}; 
	static_assert(sizeof(Metadato)==Metadato::tam_palabra*sizeof(Metadato::dato_t),"El tamaño del metadato ha cambiado");
	typedef uint16_t adc_t;
	struct Header {
		typedef uint32_t dato_t;
		dato_t freq_muestreo: 8;
		dato_t esc_vertical:8;
		dato_t tam_evento:8;
		dato_t unused:8;
		static size_t const tam_palabra=1u;
	};
	static_assert(sizeof(Header)==Header::tam_palabra*sizeof(Header::dato_t), "El tamaño del Header ha cambiado");
	explicit PruebaV2Fragmento(artdaq::Fragment const& f):fragmento(f){}
	Header::dato_t hdr_event_size() const {
		return header_()->tam_evento;
	}
	Header::dato_t hdr_freq_muestreo() const {
		return header_()->freq_muestreo;
	}
	Header::dato_t hdr_esc_vertical() const {
		return header_()->esc_vertical;
	}
	adc_t const* dataBeginADCs() const {
		return reinterpret_cast<adc_t const*>(header_()+1);
	}
private:
	artdaq::Fragment const & fragmento;
	Header const * header_() const{
		return reinterpret_cast<PruebaV2Fragmento::Header const*>(fragmento.dataBeginBytes());
	}
};











#endif

