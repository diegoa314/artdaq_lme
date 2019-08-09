#ifndef artdaq_Application_Routing_RoutingMasterPolicy_hh
#define artdaq_Application_Routing_RoutingMasterPolicy_hh

#include "artdaq/DAQdata/Globals.hh"
#include "artdaq/DAQrate/detail/RoutingPacket.hh"
#include "artdaq-core/Data/Fragment.hh"

#include "fhiclcpp/fwd.h"
#include <mutex>
#include <deque>
#include <unordered_set>

namespace artdaq
{
	/**
	 * \brief The interface through which RoutingMasterCore obtains Routing Tables using received Routing Tokens
	 */
	class RoutingMasterPolicy
	{
	public:
		/**
		 * \brief RoutingMasterPolicy Constructor
		 * \param ps ParameterSet used to configure the RoutingMasterPolicy
		 * 
		 * \verbatim
		 * RoutingMasterPolicy accepts the following Parameters:
		 * "receiver_ranks" (REQUIRED): A list of integers indicating the ranks that the RoutingMasterPolicy should expect tokens from
		 * \endverbatim
		 */
		explicit RoutingMasterPolicy(fhicl::ParameterSet ps);
		/**
		 * \brief Default virtual Destructor
		 */
		virtual ~RoutingMasterPolicy() = default;

		/**
		 * \brief Generate a Routing Table using received tokens
		 * \return A detail::RoutingPacket containing the Routing Table
		 * 
		 * This function is pure virtual, it should be overridden by derived classes.
		 */
		virtual detail::RoutingPacket GetCurrentTable() = 0;

		/**
		 * \brief Get the number of configured receivers
		 * \return The size of the receiver_ranks list
		 */
		size_t GetReceiverCount() const { return receiver_ranks_.size(); }

		/**
		 * \brief Get the largest number of tokens that the RoutingMasterPolicy has seen at any one time
		 * \return The largest number of tokens that the RoutingMasterPolicy has seen at any one time
		 */
		size_t GetMaxNumberOfTokens() const { return max_token_count_; }

		/**
		 * \brief Add a token to the token list
		 * \param rank Rank that the token is from
		 * \param new_slots_free Number of slots that are now free (should usually be 1)
		 */
		void AddReceiverToken(int rank, unsigned new_slots_free);

		/**
		 * \brief Reset the policy, setting the next sequence ID to be used to 1
		 */
		void Reset() { next_sequence_id_ = 1; }
	protected:
		Fragment::sequence_id_t next_sequence_id_; ///< The next sequence ID to be assigned

		std::unique_ptr<std::deque<int>> getTokensSnapshot(); ///< Gets the current token list, used for building Routing Tables
		void addUnusedTokens(std::unique_ptr<std::deque<int>> tokens); ///< If necessary, return unused tokens to the token list, for subsequent updates
	private:
		mutable std::mutex tokens_mutex_;
		std::unordered_set<int> receiver_ranks_;
		std::deque<int> tokens_;
		size_t max_token_count_;

	};
}


#endif // artdaq_Application_Routing_RoutingMasterPolicy_hh
