#include "artdaq/ArtModules/ArtdaqInput.hh"
#include "artdaq/ArtModules/NetMonWrapper.hh"

/**
 * \brief Namespace used for classes that interact directly with art
 */
namespace art
{
	/**
	 * \brief Trait definition (must precede source typedef).
	 */
	template <>
	struct Source_generator<ArtdaqInput<NetMonWrapper>>
	{
		static constexpr bool value = true; ///< dummy parameter
	};

	// Source declaration.
	/**
	 * \brief NetMonInput is an art::Source using an ArtdaqInput-wrapped NetMonWrapper
	 */
	typedef art::Source<ArtdaqInput<NetMonWrapper>> NetMonInput;
} // namespace art

DEFINE_ART_INPUT_SOURCE(art::NetMonInput)
