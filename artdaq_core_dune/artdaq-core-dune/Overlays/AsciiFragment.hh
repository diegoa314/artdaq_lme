#ifndef artdaq_dune_Overlays_AsciiFragment_hh
#define artdaq_dune_Overlays_AsciiFragment_hh

#include "artdaq-core/Data/Fragment.hh"
#include "cetlib_except/exception.h"

#include <ostream>
#include <vector>

// Implementation of "AsciiFragment", an artdaq::Fragment overlay class
// used for pedagogical purposes

/**
 * \brief The artdaq_demo namespace
 */
namespace demo
{
	class AsciiFragment;

	/**
	 * \brief Dumps the AsciiFragment's data (text) to given stream
	 * \return Reference to stream
	 */
	std::ostream& operator<<(std::ostream&, AsciiFragment const&);
}

/**
* \brief An artdaq::Fragment overlay class designed to hold string data for pedagogical purposes.
*
* AsciiFragment is an artdaq::Fragment overlay class designed to hold string data. It serves both
* as an educational tool for showing how Fragment overlays works, and as a way to showcase artdaq's
* data-handling ability, especially how the input data is replicated bit-for-bit in the output.
*/
class demo::AsciiFragment
{
public:

	/**
	 * \brief Metadata describing the contents of the AsciiFragment
	 * 
	 * The AsciiFragment::Metadata struct is used to store information about the
	 * upstream environment from where the fragment came. In the case of AsciiFragment,
	 * this is simply a count of the characters in the data payload.
	 */
	struct Metadata
	{
		/** "data_t" is a typedef of the fundamental unit of data the
		 * metadata structure thinks of itself as consisting of; it can give
		 * its size via the static "size_words" variable (
		 * AsciiFragment::Metadata::size_words )
		 */
		typedef char data_t;
		/**
		 * \brief Type used to represent the length in characters of the AsciiFragment data
		 */
		typedef uint32_t chars_in_line_t;

		chars_in_line_t charsInLine; ///< How many characters are in the data payload

		static size_t const size_words = 4ul; ///< Size of the Metadata object, in units of Metadata::data_t
	};

	static_assert (sizeof (Metadata) == Metadata::size_words * sizeof (Metadata::data_t), "AsciiFragment::Metadata size changed");

	/**
	 * \brief The AsciiFragment::Header contains information about the payload size and the "line number"
	 * of the data contained in the AsciiFragment.
	 * 
	 * The AsciiFragment::Header contains information about the payload size and the "line number"
	 * of the data contained in the AsciiFragment. Note that the AsciiFragment::Header is distinct from
	 * the AsciiFragment::Metadata and the Fragment::Header. AsciiFragment::Header is a convienience
	 * struct to decode the first 16 bytes of the data payload.
	 */
	struct Header
	{
		typedef char data_t; ///< The data type of the payload

		typedef uint64_t event_size_t; ///< Type for the event_size field
		typedef uint64_t line_number_t; ///< Type for the line_number field

		event_size_t event_size : 28; ///< The number of characters in the contained string
		event_size_t unused_1 : 36; ///< Unused

		line_number_t line_number; ///< The line number of the string (in ASCII). Equal to Event number in artdaq-demo/Generators/AsciiSimulator_generator.cc. 

		static size_t const size_words = 16ul; ///< Size of the Header object, in units of Header::data_t
	};

	static_assert (sizeof (Header) == Header::size_words * sizeof (Header::data_t), "AsciiFragment::Header size changed");
	
	/**
	 * \brief The AsciiFragment constructor
	 * \param f The raw artdaq::Fragment object to overlay
	 * 
	 * The constructor simply sets its const private member "artdaq_Fragment_" to refer to the artdaq::Fragment object
	 */
	explicit AsciiFragment(artdaq::Fragment const& f) : artdaq_Fragment_(f) {}

	// const getter functions for the data in the header

	/**
	 * \brief Gets the event_size from the AsciiFragment::Header
	 * \return The number of characters in the contained line
	 */
	Header::event_size_t hdr_event_size() const { return header_()->event_size; }
	/**
	 * \brief Gets the line_number from the AsciiFragment::Header
	 * \return The "line number" string of the AsciiFragment::Header
	 */
	Header::line_number_t hdr_line_number() const { return header_()->line_number; }
	/**
	 * \brief Gets the size_words variable from the artdaq::Header
	 * \return The size of the Fragment payload
	 */
	static constexpr size_t hdr_size_words() { return Header::size_words; }

	/// The number of characters in the line
	size_t total_line_characters() const
	{
		return (hdr_event_size() - hdr_size_words()) * chars_per_word_();
	}

	/// Start of the line, returned as a pointer to the char type
	char const* dataBegin() const
	{
		return reinterpret_cast<char const *>(header_() + 1);
	}

	/// End of the line, returned as a pointer to the char type
	char const* dataEnd() const
	{
		return dataBegin() + total_line_characters();
	}

protected:
	
	/**
	 * \brief Translates between the size of the character type and the artda::Fragment::value_type
	 * \return The number of characters in a artdaq::Fragment::value_type
	 */
	static constexpr size_t chars_per_word_()
	{
		return sizeof(Header::data_t) / sizeof(char);
	}
	
	/**
	 * \brief header_() simply takes the address of the start of this overlay's
	 * data (i.e., where the AsciiFragment::Header object begins) and
	 * casts it as a pointer to AsciiFragment::Header
	 * \return Pointer to the AsciiFragment::Header object in the data payload
	 */
	Header const* header_() const
	{
		return reinterpret_cast<AsciiFragment::Header const *>(artdaq_Fragment_.dataBeginBytes());
	}

private:

	artdaq::Fragment const& artdaq_Fragment_;
};

#endif /* artdaq_demo_Overlays_AsciiFragment_hh */
