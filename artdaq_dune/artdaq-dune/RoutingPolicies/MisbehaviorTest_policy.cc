#include "artdaq/Application/Routing/RoutingMasterPolicy.hh"
#include "artdaq/Application/Routing/PolicyMacros.hh"
#include "artdaq/DAQdata/Globals.hh"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

namespace demo
{
	/**
	 * \brief A test RoutingMasterPolicy which does various "bad" things, determined by configuration
	 */
	class MisbehaviorTest : public artdaq::RoutingMasterPolicy
	{
	public:
		/**
		 * \brief MisbehaviorTest Constructor
		 * \param ps ParameterSet used to configure MisbehaviorTest
		 * 
		 * \verbatim
		 * Note that only one misbehavior can be configured at a time. MisbehaviorTest will work like NoOp_policy when not misbehaving
		 * MisbehaviorTest accepts the following Parameters:
		 * "misbehave_after_n_events" (Default: 1000): The threshold after which it will start misbehaving
		 * "misbehave_pause_time_ms" (Default: 0): If greater than 0, will pause this long before sending out table updates
		 * "misbehave_send_confliting_table_data" (Default: false): If true, will send a table that contains the same sequence ID being sent to two different EventBuilders
		 * "misbehave_send_corrupt_table_data" (Default: false): If true, will send a table that contains an entry created using rand(), rand()
		 * "misbehave_overload_event_builder" (Default: false): If true, will send a large number of events to one EventBuilder
		 * \endverbatim
		 */
		explicit MisbehaviorTest(fhicl::ParameterSet ps);

		/**
		 * \brief MisbehaviorTest default Destructor
		 */
		virtual ~MisbehaviorTest() = default;

		/**
		 * \brief Generate and return a Routing Table
		 * \return An artdaq::detail::RoutingPacket with the Routing Table information
		 */
		artdaq::detail::RoutingPacket GetCurrentTable() override;
	private:
		artdaq::Fragment::sequence_id_t misbehave_after_;
		size_t misbehave_pause_ms_;
		bool misbehave_conflicting_table_data_;
		bool misbehave_corrupt_table_data_;
		bool misbehave_overload_event_builder_;
	};

	MisbehaviorTest::MisbehaviorTest(fhicl::ParameterSet ps)
		: RoutingMasterPolicy(ps)
		, misbehave_after_(ps.get<size_t>("misbehave_after_n_events", 1000))
		, misbehave_pause_ms_(ps.get<size_t>("misbehave_pause_time_ms", 0))
		, misbehave_conflicting_table_data_(ps.get<bool>("misbehave_send_conflicting_table_data", false))
		, misbehave_corrupt_table_data_(ps.get<bool>("misbehave_send_corrupt_table_data", false))
		, misbehave_overload_event_builder_(ps.get<bool>("misbehave_overload_event_builder", false))
	{
		srand(time(0));
		auto count = (misbehave_conflicting_table_data_ ? 1 : 0) + (misbehave_corrupt_table_data_ ? 1 : 0) + (misbehave_overload_event_builder_ ? 1 : 0) + (misbehave_pause_ms_ > 0 ? 1 : 0);
		if (count > 1)
		{
			mf::LogWarning("MisbehaviorTest") << "Only one misbehavior is allowed at a time!";
			exit(3);
		}
	}

	artdaq::detail::RoutingPacket MisbehaviorTest::GetCurrentTable()
	{
		auto tokens = getTokensSnapshot();
		artdaq::detail::RoutingPacket output;

		auto half = tokens->size() / 2;
		size_t counter = 0;
		for(;counter < half;++counter)
		{
			output.emplace_back(artdaq::detail::RoutingPacketEntry(next_sequence_id_++, tokens->at(counter)));
		}

		if (next_sequence_id_ > misbehave_after_)
		{
			if (tokens->size() > 0)
			{
				if (misbehave_pause_ms_ > 0)
				{
					mf::LogError("MisbehaviorTest") << "Pausing for " << misbehave_pause_ms_ << " milliseconds before sending table update";
					usleep(misbehave_pause_ms_ * 1000);
				}
				if (misbehave_conflicting_table_data_)
				{
					mf::LogError("MisbehaviorTest") << "Adding conflicting data point to output";
					output.emplace_back(next_sequence_id_, tokens->at(counter) + 1);
				}
				if (misbehave_corrupt_table_data_)
				{
					mf::LogError("MisbehaviorTest") << "Adding random data point";
					output.emplace_back(seedAndRandom(), rand());
				}
				if (misbehave_overload_event_builder_)
				{
					mf::LogError("MisbehaviorTest") << "Sending 100 events in a row to Rank " << tokens->at(0);
					for (auto ii = 0; ii < 100; ++ii)
					{
						output.emplace_back(next_sequence_id_++, tokens->at(0));
					}
				}
				misbehave_after_ += misbehave_after_;
			}
		}

		for (;counter < tokens->size();++counter)
		{
			output.emplace_back(artdaq::detail::RoutingPacketEntry(next_sequence_id_++, tokens->at(counter)));
		}

		return output;
	}
}

DEFINE_ARTDAQ_ROUTING_POLICY(demo::MisbehaviorTest)