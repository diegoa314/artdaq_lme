#include "artdaq/Application/Routing/RoutingMasterPolicy.hh"
#include "artdaq/Application/Routing/PolicyMacros.hh"
#include "fhiclcpp/ParameterSet.h"
#include <cmath>

namespace artdaq
{
	/**
	 * \brief A RoutingMasterPolicy which tries to fully load the first receiver, then the second, and so on
	 */
	class CapacityTestPolicy : public RoutingMasterPolicy
	{
	public:
		/**
		 * \brief CapacityTestPolicy Constructor
		 * \param ps ParameterSet used to configure the CapacityTestPolicy
		 * 
		 * \verbatim
		 * CapacityTestPolicy accepts the following Parameters:
		 * "tokens_used_per_table_percent" (Default: 50): Percentage of available tokens to be used on each iteration.
		 * \endverbatim
		 */
		explicit CapacityTestPolicy(fhicl::ParameterSet ps);

		/**
		 * \brief Default virtual Destructor
		 */
		virtual ~CapacityTestPolicy() = default;

		/**
		 * \brief Apply the policy to the current tokens
		 * \return A detail::RoutingPacket containing the Routing Table
		 * 
		 * CapacityTestPolicy will assign available tokens from the first receiver, then the second, and so on
		 * until it has assigned tokens equal to the inital_token_count * tokens_used_per_table_percent / 100.
		 * The idea is that in steady-state, the load on the receivers should reflect the workload relative to
		 * the capacity of the system. (i.e. if you have 5 receivers, and 3 of them are 100% busy, then your load
		 * factor is approximately 60%.)
		 */
		detail::RoutingPacket GetCurrentTable() override;
	private:
		int tokenUsagePercent_;
	};

	CapacityTestPolicy::CapacityTestPolicy(fhicl::ParameterSet ps)
		: RoutingMasterPolicy(ps)
		, tokenUsagePercent_(ps.get<int>("tokens_used_per_table_percent", 50))
	{}

	detail::RoutingPacket CapacityTestPolicy::GetCurrentTable()
	{
		auto tokens = getTokensSnapshot();
		std::map<int, int> table;
		auto tokenCount = 0;
		for (auto token : *tokens)
		{
			table[token]++;
			tokenCount++;
		}
		tokens->clear();

		int tokensToUse = ceil(tokenCount * tokenUsagePercent_ / 100.0);
		auto tokensUsed = 0;

		detail::RoutingPacket output;
		for (auto r : table)
		{
			bool breakCondition = false;
			while (table[r.first] > 0) {
				output.emplace_back(detail::RoutingPacketEntry(next_sequence_id_++, r.first));
				table[r.first]--;
				tokensUsed++;
				if(tokensUsed >= tokensToUse)
				{
					breakCondition = true;
					break;
				}
			}
			if (breakCondition) break;
		}

		for (auto r : table)
		{
			for (auto i = 0; i < r.second; ++i)
			{
				tokens->push_back(r.first);
			}
		}
		addUnusedTokens(std::move(tokens));

		return output;
	}
}

DEFINE_ARTDAQ_ROUTING_POLICY(artdaq::CapacityTestPolicy)