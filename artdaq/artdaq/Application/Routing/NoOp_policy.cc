#include "artdaq/Application/Routing/RoutingMasterPolicy.hh"
#include "artdaq/Application/Routing/PolicyMacros.hh"
#include "fhiclcpp/ParameterSet.h"
#include "tracemf.h"
#define TRACE_NAME "NoOp_policy"
namespace artdaq
{
	/**
	 * \brief A RoutingMasterPolicy which simply assigns Sequence IDs to tokens in the order they were received
	 */
	class NoOpPolicy : public RoutingMasterPolicy
	{
	public:
		/**
		 * \brief NoOpPolicy Constructor
		 * \param ps ParameterSet used to configure the NoOpPolicy
		 * 
		 * NoOpPolicy takes no additional Parameters at this time
		 */
		explicit NoOpPolicy(fhicl::ParameterSet ps) : RoutingMasterPolicy(ps) {}

		/**
		 * \brief Default virtual Destructor
		 */
		virtual ~NoOpPolicy() = default;

		/**
		 * \brief Using the tokens received so far, create a Routing Table
		 * \return A detail::RoutingPacket containing the Routing Table
		 */
		detail::RoutingPacket GetCurrentTable() override;
	};

	detail::RoutingPacket NoOpPolicy::GetCurrentTable()
	{
		TLOG(12) << "NoOpPolicy::GetCurrentTable start";
		auto tokens = getTokensSnapshot();
		detail::RoutingPacket output;
		for(auto token : *tokens.get())
		{
			output.emplace_back(detail::RoutingPacketEntry(next_sequence_id_++, token));
		}

		TLOG(12) << "NoOpPolicy::GetCurrentTable return";
		return output;
	}
}

DEFINE_ARTDAQ_ROUTING_POLICY(artdaq::NoOpPolicy)
