#ifndef artdaq_dune_Overlays_AsciiFragmentWriter_hh
#define artdaq_dune_Overlays_AsciiFragmentWriter_hh

#include "artdaq-core/Data/Fragment.hh"
#include "artdaq-core-dune/Overlays/AsciiFragment.hh"

namespace demo
{
	class AsciiFragmentWriter;
}


/**
 * \brief Class derived from AsciiFragment which allows writes to the data (for simulation purposes).
 * 
 * Class derived from AsciiFragment which allows writes to the data (for
 * simulation purposes). Note that for this reason it contains
 * non-const members which hide the const members in its parent class,
 * AsciiFragment, including its reference to the artdaq::Fragment
 * object, artdaq_Fragment_, as well as its functions pointing to the
 * beginning and end of the line in the fragment, dataBegin() and
 * dataEnd()
 */
class demo::AsciiFragmentWriter: public demo::AsciiFragment
{
public:


	/**
	 * \brief The constructor will expect the artdaq::Fragment object it's been
	 * passed to contain the artdaq::Fragment header + the
	 * AsciiFragment::Metadata object, otherwise it throws
	 * \param f artdaq::Fragment object to overlay
	 * \throws cet::exception if input Fragment does not contain an AsciiFragment::Metadata object
	 */
	explicit AsciiFragmentWriter(artdaq::Fragment& f);

	// These functions form overload sets with const functions from
	// demo::AsciiFragment

	/**
	 * \brief Return a non-const (for writing) pointer to the beginning of the Fragment payload
	 * \return A pointer to the beginning of the Fragment payload
	 */
	char* dataBegin();

	/**
	 * \brief Return a non-const (for writing) pointer to the end of the Fragment payload
	 * \return A pointer to the end of the Fragment payload
	 */
	char* dataEnd();

	/**
	 * \brief Returns a pointer to the AsciiFragment::Header object, non-const for writing
	 * \return A pointer to the AsciiFragment::Header object
	 */
	Header* header_()
	{
		assert(artdaq_Fragment_.dataSizeBytes() >= sizeof(Header) );
		return reinterpret_cast<Header *>(artdaq_Fragment_.dataBeginBytes());
	}

	/**
	 * \brief Sets the line number in the AsciiFragment::Header to the given (ASCII-encoded) value
	 * \param line_number Set the line number (ASCII-encoded) to this value
	 */
	void set_hdr_line_number(Header::line_number_t line_number)
	{
		header_()->line_number = line_number;
	}

	/**
	 * \brief Resize the Fragment so that it can contain nChars characters
	 * \param nChars Number of characters in resized Fragment
	 */
	void resize(size_t nChars);

private:
	static size_t calc_event_size_words_(size_t nChars);

	static size_t chars_to_words_(size_t nChars);

	// Note that this non-const reference hides the const reference in the base class
	artdaq::Fragment& artdaq_Fragment_;
};

// The constructor will expect the artdaq::Fragment object it's been
// passed to contain the artdaq::Fragment header + the
// AsciiFragment::Metadata object, otherwise it throws

inline demo::AsciiFragmentWriter::AsciiFragmentWriter(artdaq::Fragment& f) :
																	AsciiFragment(f)
																	, artdaq_Fragment_(f)
{
	if (! f.hasMetadata() || f.dataSizeBytes() > 0)
	{
		throw cet::exception("Error in AsciiFragmentWriter: Raw artdaq::Fragment object does not appear to consist of (and only of) its own header + the AsciiFragment::Metadata object");
	}

	// Allocate space for the header
	artdaq_Fragment_.resizeBytes(sizeof(Header));
}


inline char* demo::AsciiFragmentWriter::dataBegin()
{
	// Make sure there's data past the AsciiFragment header
	assert(artdaq_Fragment_.dataSizeBytes() >= sizeof(Header) + sizeof(artdaq::Fragment::value_type) );
	return reinterpret_cast<char*>(header_() + 1);
}

inline char* demo::AsciiFragmentWriter::dataEnd()
{
	return dataBegin() + total_line_characters();
}


inline void demo::AsciiFragmentWriter::resize(size_t nChars)
{
	artdaq_Fragment_.resizeBytes(sizeof(Header::data_t) * calc_event_size_words_(nChars));
	header_()->event_size = calc_event_size_words_(nChars);
}

inline size_t demo::AsciiFragmentWriter::calc_event_size_words_(size_t nChars)
{
	return chars_to_words_(nChars) + hdr_size_words();
}

inline size_t demo::AsciiFragmentWriter::chars_to_words_(size_t nChars)
{
	auto mod(nChars % chars_per_word_());
	return (mod == 0) ?
			   nChars / chars_per_word_() :
			   nChars / chars_per_word_() + 1;
}

#endif /* artdaq_demo_Overlays_AsciiFragmentWriter_hh */
