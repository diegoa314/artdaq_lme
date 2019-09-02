#include "art/Framework/IO/Sources/Source.h"
#include "artdaq/ArtModules/detail/SharedMemoryReader.hh"
#include "art/Framework/Core/InputSourceMacros.h"
#include "art/Framework/IO/Sources/SourceTraits.h"

#include <string>
using std::string;


namespace art
{
	/**
	* \brief  Specialize an art source trait to tell art that we don't care about
	* source.fileNames and don't want the files services to be used.
	*/
	template <>
	struct Source_generator<artdaq::detail::SharedMemoryReader<artdaq::Fragment::MakeSystemTypeMap>>
	{
		static constexpr bool value = true; ///< Used to suppress use of file services on art Source
	};
}
namespace artdaq
{
	/**
	 * \brief RawInput is a typedef of art::Source<detail::SharedMemoryReader>
	 */
	typedef art::Source<detail::SharedMemoryReader<artdaq::Fragment::MakeSystemTypeMap>> RawInput;
}

DEFINE_ART_INPUT_SOURCE(artdaq::RawInput)
