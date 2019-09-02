#ifndef artdaq_core_dune_Overlays_UDPFragment_hh
#define artdaq_core_dune_Overlays_UDPFragment_hh

#include "artdaq-core/Data/Fragment.hh"

#include <ostream>

// Implementation of "UDPFragment", an artdaq::Fragment overlay class

namespace demo
{
	class UDPFragment;

	/// Let the "<<" operator dump the UDPFragment's data to stdout
	std::ostream& operator <<(std::ostream&, UDPFragment const&);
}

/**
 * \brief A Fragment designed to contain data received from the network in UDP datagrams
 */
class demo::UDPFragment
{
public:

	/**
	* \brief Metadata describing the contents of the UDPFragment
	*
	* The AsciiFragment::Metadata struct is used to store information about the
	* upstream environment from where the fragment came. In the case of UDPFragment,
	* this is the port and address at which the data was received.
	*/
	struct Metadata
	{
		typedef uint64_t data_t; ///< The fundamental unit of Metadata data

		data_t port : 16; ///< The port on which the data was received
		data_t address : 32; ///< The IPv4 address the data was received on
		data_t unused : 16; ///< Unused bits of the data_t field

		static size_t const size_words = 1ull; ///< Size of the UDPFragment::Metadata object, in units of Metadata::data_t
	};

	static_assert (sizeof(Metadata) == Metadata::size_words * sizeof(Metadata::data_t), "UDPFragment::Metadata size changed");

	/**
	* \brief The UDPFragment::Header contains information about the payload size and the "data type" of the UDP data.
	*
	* The UDPFragment::Header contains information about the payload size and the "data type" of the UDP data.
	* Currently-defined data types are Raw, String and JSON (defined in demo::DataType enum in artdaq-demo/Generators/UDPReceiver.hh).
	* Note that the UDPFragment::Header is distinct from the UDPFragment::Metadata and the Fragment::Header.
	* UDPFragment::Header is a convienience struct to decode the first 32 bytes of the data payload.
	*
	* Notice only the first 28 bits of the first 32-bit unsigned
	* integer in the Header is used to hold the event_size; this means
	* that you can't represent a fragment larger than 2**28 units of
	* data_t, or 1,073,741,824 bytes
	*/
	struct Header
	{
		typedef uint32_t data_t; ///< The fundamental unit of Header data

		typedef uint32_t event_size_t; ///< Type of the event_size field
		typedef uint32_t data_type_t; ///< Type of the type field

		event_size_t event_size : 28; ///< The size of the payload, in words
		event_size_t type : 4; ///< The type of the payload data, 0: Raw, 1: JSON, 2: String.

		static size_t const size_words = 1ul; ///< Size of the UDPFragment::Header, in units of Header::data_t
	};

	static_assert (sizeof(Header) == Header::size_words * sizeof(Header::data_t), "UDPFragment::Header size changed");

	/**
	* \brief The UDPFragment constructor
	* \param f The raw artdaq::Fragment object to overlay
	*
	* The constructor simply sets its const private member "artdaq_Fragment_" to refer to the artdaq::Fragment object
	*/
	explicit UDPFragment(artdaq::Fragment const& f) : artdaq_Fragment_(f) {}

	/**
	 * \brief Get the current value of the Header::event_size field
	 * \return The current value of the Header::event_size field (const)
	 */
	Header::event_size_t hdr_event_size() const { return header_()->event_size; }
	/**
	 * \brief Get the current value of the Header::type field
	 * \return The current value of the Header::type field (const)
	 */
	Header::data_type_t hdr_data_type() const { return header_()->type; }
	/**
	* \brief Gets the size_words variable from the artdaq::Header
	* \return The size of the Fragment payload
	*/
	static constexpr size_t hdr_size_words() { return Header::size_words; }

	/**
	 * \brief Get the number of words in the UDPFragment payload
	 * \return The number of Header::data_t words in the UDPFragment payload (const)
	 */
	size_t udp_data_words() const
	{
		return hdr_event_size() - hdr_size_words();
	}

	/**
	 * \brief Returns a const pointer to the start of the UDP payload
	 * \return const byte pointer to the start of the UDP payload
	 */
	uint8_t const* dataBegin() const
	{
		return reinterpret_cast<uint8_t const *>(header_() + 1);
	}

	/**
	 * \brief Returns a const pointer to the end of the UDP payload
	 * \return const byte pointer to the end of the UDP payload
	 */
	uint8_t const* dataEnd() const
	{
		return dataBegin() + udp_data_words() * bytes_per_word_();
	}

protected:
	
	/**
	 * \brief Translates between data words (type Header::data_t) and bytes
	 * \return The number of bytes in each Header::data_t word
	 */
	static constexpr size_t bytes_per_word_()
	{
		return sizeof(Header::data_t) / sizeof(uint8_t);
	}

	/**
	 * \brief Get a const pointer to the UDPFragment::Header
	 * \return A const pointer to the UDPFragment:Header
	 *
	 * header_() simply takes the address of the start of this overlay's
	 * data (i.e., where the UDPFragment::Header object begins) and
	 * casts it as a pointer to UDPFragment::Header
	 */
	Header const* header_() const
	{
		return reinterpret_cast<UDPFragment::Header const *>(artdaq_Fragment_.dataBeginBytes());
	}

private:

	artdaq::Fragment const& artdaq_Fragment_;
};

#endif /* artdaq_core_ots_Overlays_UDPFragment_hh */
