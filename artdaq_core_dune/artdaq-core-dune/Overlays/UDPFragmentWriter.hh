#ifndef artdaq_core_dune_Overlays_UDPFragmentWriter_hh
#define artdaq_core_dune_Overlays_UDPFragmentWriter_hh

#include "artdaq-core/Data/Fragment.hh"
#include "artdaq-core-dune/Overlays/UDPFragment.hh"

namespace demo
{
	class UDPFragmentWriter;
}


/**
 * \brief Class derived from UDPFragment which allows writes to the data.
 * 
 * Class derived from UDPFragment which allows writes to the data.
 *  Note that for this reason it contains
 * non-const members which hide the const members in its parent class,
 * UDPFragment, including its reference to the artdaq::Fragment
 * object, artdaq_Fragment_, as well as its functions pointing to the
 * beginning and end of the line in the fragment, dataBegin() and
 * dataEnd(). This is necessary as the UDP data is not coming from
 * "hardware" but the UDP stack.
 */
class demo::UDPFragmentWriter: public demo::UDPFragment
{
public:


	/**
	 * \brief UDPFragmentWriter constructor
	 * \param f The artdaq::Fragment to overlay
	 * \throws cet::exception if the Fragment does not contain UDPFragment::Metadata and nothing else
	 */
	explicit UDPFragmentWriter(artdaq::Fragment& f);
	
	/**
	 * \brief Get a pointer to the start of the UDP payload
	 * \return A byte pointer to the start of the UDP payload
	 */
	uint8_t* dataBegin();

	/**
	 * \brief Get a pointer to the end of the UDP payload
	 * \return A byte pointer to the end of the UDP payload
	 */
	uint8_t* dataEnd();
	
	/**
	 * \brief Get a pointer to the UDPFragment::Header object for writing
	 * \return A pointer to the UDPFragment::Header object
	 */
	Header* header_()
	{
		assert(artdaq_Fragment_.dataSizeBytes() >= sizeof(Header) );
		return reinterpret_cast<Header *>(artdaq_Fragment_.dataBeginBytes());
	}

	/**
	 * \brief Setter for the Header::type field
	 * \param dataType Value to set to Header::type
	 */
	void set_hdr_type(Header::data_type_t dataType)
	{
		header_()->type = dataType & 0xF;
	}

	/**
	 * \brief Resize the UDP payload to the given number of bytes
	 * \param nBytes Number of bytes to request for the UDP payload
	 */
	void resize(size_t nBytes);

private:
	/**
	 * \brief Calculate the size of the UDPFragment payload in Header::data_t words
	 * \param nBytes Number of bytes in the UDP payload
	 * \return Number of Header::data_t words in the UDP payload
	 */
	static size_t calc_event_size_words_(size_t nBytes);

	/**
	 * \brief Calculate the number of Header::data_t words in a given number of bytes
	 * \param nBytes Number of bytes
	 * \return Number of Header::data_t words
	 */
	static size_t bytes_to_words_(size_t nBytes);

	// Note that this non-const reference hides the const reference in the base class
	artdaq::Fragment& artdaq_Fragment_;
};

inline demo::UDPFragmentWriter::UDPFragmentWriter(artdaq::Fragment& f) :
																UDPFragment(f)
																, artdaq_Fragment_(f)
{
	if (! f.hasMetadata() || f.dataSizeBytes() > 0)
	{
		throw cet::exception("Error in UDPFragmentWriter: Raw artdaq::Fragment object does not appear to consist of (and only of) its own header + the UDPFragment::Metadata object");
	}

	// Allocate space for the header
	artdaq_Fragment_.resizeBytes(sizeof(Header));
}


inline uint8_t* demo::UDPFragmentWriter::dataBegin()
{
	// Make sure there's data past the UDPFragment header
	assert(artdaq_Fragment_.dataSizeBytes() >= sizeof(Header) + sizeof(artdaq::Fragment::value_type) );
	return reinterpret_cast<uint8_t *>(header_() + 1);
}

inline uint8_t* demo::UDPFragmentWriter::dataEnd()
{
	return dataBegin() + udp_data_words() * bytes_per_word_();
}


inline void demo::UDPFragmentWriter::resize(size_t nBytes)
{
	artdaq_Fragment_.resizeBytes(sizeof(Header::data_t) * calc_event_size_words_(nBytes));
	header_()->event_size = calc_event_size_words_(nBytes);
}

inline size_t demo::UDPFragmentWriter::calc_event_size_words_(size_t nBytes)
{
	return bytes_to_words_(nBytes) + hdr_size_words();
}

inline size_t demo::UDPFragmentWriter::bytes_to_words_(size_t nBytes)
{
	auto mod(nBytes % bytes_per_word_());
	return (mod == 0) ?
			   nBytes / bytes_per_word_() :
			   nBytes / bytes_per_word_() + 1;
}

#endif /* artdaq_demo_Overlays_UDPFragmentWriter_hh */
