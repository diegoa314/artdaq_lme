#define BOOST_TEST_MODULE NoOp_policy_t
#include <boost/test/auto_unit_test.hpp>

#include "artdaq/Application/Routing/makeRoutingMasterPolicy.hh"
#include "fhiclcpp/ParameterSet.h"
#include "fhiclcpp/make_ParameterSet.h"


BOOST_AUTO_TEST_SUITE(NoOp_policy_t)

BOOST_AUTO_TEST_CASE(Simple)
{
	fhicl::ParameterSet ps;
	fhicl::make_ParameterSet("receiver_ranks: [1,2,3,4]", ps);

	auto noop = artdaq::makeRoutingMasterPolicy("NoOp", ps);

	BOOST_REQUIRE_EQUAL(noop->GetReceiverCount(), 4);
	
	noop->Reset();
	noop->AddReceiverToken(1,1);
	noop->AddReceiverToken(3, 1);
	noop->AddReceiverToken(2, 1);
	noop->AddReceiverToken(4, 1);
	noop->AddReceiverToken(2, 1);
	auto secondTable = noop->GetCurrentTable();
	BOOST_REQUIRE_EQUAL(secondTable.size(), 5);
	BOOST_REQUIRE_EQUAL(secondTable[0].destination_rank, 1);
	BOOST_REQUIRE_EQUAL(secondTable[1].destination_rank, 3);
	BOOST_REQUIRE_EQUAL(secondTable[2].destination_rank, 2);
	BOOST_REQUIRE_EQUAL(secondTable[3].destination_rank, 4);
	BOOST_REQUIRE_EQUAL(secondTable[4].destination_rank, 2);
	BOOST_REQUIRE_EQUAL(secondTable[0].sequence_id, 1);
	BOOST_REQUIRE_EQUAL(secondTable[1].sequence_id, 2);
	BOOST_REQUIRE_EQUAL(secondTable[2].sequence_id, 3);
	BOOST_REQUIRE_EQUAL(secondTable[3].sequence_id, 4);
	BOOST_REQUIRE_EQUAL(secondTable[4].sequence_id, 5);

	noop->AddReceiverToken(1, 0);

	auto thirdTable = noop->GetCurrentTable();
	BOOST_REQUIRE_EQUAL(thirdTable.size(), 0);
}

BOOST_AUTO_TEST_SUITE_END()
