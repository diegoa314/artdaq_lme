#define BOOST_TEST_MODULE RoundRobin_policy_t
#include <boost/test/auto_unit_test.hpp>

#include "artdaq/Application/Routing/makeRoutingMasterPolicy.hh"
#include "fhiclcpp/ParameterSet.h"
#include "fhiclcpp/make_ParameterSet.h"


BOOST_AUTO_TEST_SUITE(RoundRobin_policy_t)

BOOST_AUTO_TEST_CASE(Simple)
{
	fhicl::ParameterSet ps;
	fhicl::make_ParameterSet("receiver_ranks: [1,2,3,4]", ps);

	auto rr = artdaq::makeRoutingMasterPolicy("RoundRobin", ps);

	BOOST_REQUIRE_EQUAL(rr->GetReceiverCount(), 4);
	
	rr->Reset();
	rr->AddReceiverToken(1, 1);
	rr->AddReceiverToken(3, 1);
	rr->AddReceiverToken(2, 1);
	rr->AddReceiverToken(4, 1);
	rr->AddReceiverToken(2, 1);
	auto secondTable = rr->GetCurrentTable();
	BOOST_REQUIRE_EQUAL(secondTable.size(), 4);
	BOOST_REQUIRE_EQUAL(secondTable[0].destination_rank, 1);
	BOOST_REQUIRE_EQUAL(secondTable[1].destination_rank, 2);
	BOOST_REQUIRE_EQUAL(secondTable[2].destination_rank, 3);
	BOOST_REQUIRE_EQUAL(secondTable[3].destination_rank, 4);
	BOOST_REQUIRE_EQUAL(secondTable[0].sequence_id, 1);
	BOOST_REQUIRE_EQUAL(secondTable[1].sequence_id, 2);
	BOOST_REQUIRE_EQUAL(secondTable[2].sequence_id, 3);
	BOOST_REQUIRE_EQUAL(secondTable[3].sequence_id, 4);

	rr->AddReceiverToken(1, 0);

	auto thirdTable = rr->GetCurrentTable();
	BOOST_REQUIRE_EQUAL(thirdTable.size(), 0);

	rr->AddReceiverToken(1, 2);
	rr->AddReceiverToken(2, 1);
	rr->AddReceiverToken(3, 1);
	rr->AddReceiverToken(4, 2);
	auto fourthTable = rr->GetCurrentTable();
	BOOST_REQUIRE_EQUAL(fourthTable.size(), 4);
	BOOST_REQUIRE_EQUAL(fourthTable[0].destination_rank, 1);

	rr->AddReceiverToken(3, 1);
	auto fifthTable = rr->GetCurrentTable();
	BOOST_REQUIRE_EQUAL(fifthTable.size(), 4);
	BOOST_REQUIRE_EQUAL(fifthTable[0].destination_rank, 1);
	
}

BOOST_AUTO_TEST_CASE(MinimumParticipants)
{
	fhicl::ParameterSet ps;
	fhicl::make_ParameterSet("receiver_ranks: [1,2,3,4] minimum_participants: 2", ps);

	auto rr = artdaq::makeRoutingMasterPolicy("RoundRobin", ps);

	BOOST_REQUIRE_EQUAL(rr->GetReceiverCount(), 4);

	rr->Reset();
	rr->AddReceiverToken(1, 1);
	rr->AddReceiverToken(3, 1);
	rr->AddReceiverToken(2, 1);
	rr->AddReceiverToken(4, 1);
	rr->AddReceiverToken(2, 1);
	auto secondTable = rr->GetCurrentTable();
	BOOST_REQUIRE_EQUAL(secondTable.size(), 4);
	BOOST_REQUIRE_EQUAL(secondTable[0].destination_rank, 1);
	BOOST_REQUIRE_EQUAL(secondTable[1].destination_rank, 2);
	BOOST_REQUIRE_EQUAL(secondTable[2].destination_rank, 3);
	BOOST_REQUIRE_EQUAL(secondTable[3].destination_rank, 4);
	BOOST_REQUIRE_EQUAL(secondTable[0].sequence_id, 1);
	BOOST_REQUIRE_EQUAL(secondTable[1].sequence_id, 2);
	BOOST_REQUIRE_EQUAL(secondTable[2].sequence_id, 3);
	BOOST_REQUIRE_EQUAL(secondTable[3].sequence_id, 4);

	rr->AddReceiverToken(1, 0);

	auto thirdTable = rr->GetCurrentTable();
	BOOST_REQUIRE_EQUAL(thirdTable.size(), 0);


	rr->AddReceiverToken(1, 1);
	auto fourthTable = rr->GetCurrentTable();
	BOOST_REQUIRE_EQUAL(fourthTable.size(), 2);

	BOOST_REQUIRE_EQUAL(fourthTable[0].destination_rank, 1);
	BOOST_REQUIRE_EQUAL(fourthTable[1].destination_rank, 2);
	BOOST_REQUIRE_EQUAL(fourthTable[0].sequence_id, 5);
	BOOST_REQUIRE_EQUAL(fourthTable[1].sequence_id, 6);

	rr->AddReceiverToken(1, 2);
	rr->AddReceiverToken(2, 2);
	rr->AddReceiverToken(3, 1);
	rr->AddReceiverToken(4, 2);
	auto fifthTable = rr->GetCurrentTable();
	BOOST_REQUIRE_EQUAL(fifthTable.size(), 7);
	BOOST_REQUIRE_EQUAL(fifthTable[0].destination_rank, 1);

	rr->AddReceiverToken(3, 1);
	auto sixthTable = rr->GetCurrentTable();
	BOOST_REQUIRE_EQUAL(sixthTable.size(), 0);

}

BOOST_AUTO_TEST_SUITE_END()
