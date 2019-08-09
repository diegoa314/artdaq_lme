#include "art/Framework/IO/Sources/Source.h"
#include "artdaq/ArtModules/detail/SharedMemoryReader.hh"
#include "artdaq-core-demo/Overlays/FragmentType.hh"
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
	struct Source_generator<artdaq::detail::SharedMemoryReader<demo::makeFragmentTypeMap>>
	{
		static constexpr bool value = true; ///< Used to suppress use of file services on art Source
	};
}

/**
 * \brief The artdaq_demo namespace
 */
namespace demo
{
	/**
	 * \brief DemoInput is an art::Source using the detail::RawEventQueueReader class
	 */
	typedef art::Source< artdaq::detail::SharedMemoryReader<demo::makeFragmentTypeMap> > DemoInput;
}

DEFINE_ART_INPUT_SOURCE(demo::DemoInput)
