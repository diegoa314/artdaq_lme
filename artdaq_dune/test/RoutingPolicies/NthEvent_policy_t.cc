#define BOOST_TEST_MODULE ( NthEvent_policy_t )
#include <boost/test/auto_unit_test.hpp>

#include "artdaq/Application/Routing/makeRoutingMasterPolicy.hh"
#include "fhiclcpp/ParameterSet.h"
#include "fhiclcpp/make_ParameterSet.h"


BOOST_AUTO_TEST_SUITE(NthEvent_policy_t)

BOOST_AUTO_TEST_CASE(Simple)
{
	fhicl::ParameterSet ps;
	fhicl::make_ParameterSet("receiver_ranks: [1,2,3,4] nth_event: 5 target_receiver: 3", ps);

	auto nth = artdaq::makeRoutingMasterPolicy("NthEvent", ps);

	BOOST_REQUIRE_EQUAL(nth->GetReceiverCount(), 4);
		
	// Extra token, and out of sequence
	nth->Reset();
	nth->AddReceiverToken(1, 1);
	nth->AddReceiverToken(2, 1);
	nth->AddReceiverToken(4, 1);
	nth->AddReceiverToken(2, 1);
	auto secondTable = nth->GetCurrentTable();
	BOOST_REQUIRE_EQUAL(secondTable.size(), 3);
	BOOST_REQUIRE_EQUAL(secondTable[0].destination_rank, 1);
	BOOST_REQUIRE_EQUAL(secondTable[0].sequence_id, 1);
	BOOST_REQUIRE_EQUAL(secondTable[1].destination_rank, 2);
	BOOST_REQUIRE_EQUAL(secondTable[1].sequence_id, 2);
	BOOST_REQUIRE_EQUAL(secondTable[2].destination_rank, 4);
	BOOST_REQUIRE_EQUAL(secondTable[2].sequence_id, 3);

	// Adding empty number of tokens
	nth->AddReceiverToken(1, 0);
	auto thirdTable = nth->GetCurrentTable();
	BOOST_REQUIRE_EQUAL(thirdTable.size(), 0);

	// Catching up to the extra token
	nth->AddReceiverToken(1, 2);
	nth->AddReceiverToken(3, 2);
	nth->AddReceiverToken(4, 2);
	auto fourthTable = nth->GetCurrentTable();
	BOOST_REQUIRE_EQUAL(fourthTable.size(), 4);
	BOOST_REQUIRE_EQUAL(fourthTable[0].destination_rank, 1);
	BOOST_REQUIRE_EQUAL(fourthTable[0].sequence_id, 4);
	BOOST_REQUIRE_EQUAL(fourthTable[1].destination_rank, 3);
	BOOST_REQUIRE_EQUAL(fourthTable[1].sequence_id, 5);
	BOOST_REQUIRE_EQUAL(fourthTable[2].destination_rank, 2);
	BOOST_REQUIRE_EQUAL(fourthTable[2].sequence_id, 6);
	BOOST_REQUIRE_EQUAL(fourthTable[3].destination_rank, 4);
	BOOST_REQUIRE_EQUAL(fourthTable[3].sequence_id, 7);

	// Catching up to the missing token
	nth->AddReceiverToken(2, 1);
	auto fifthTable = nth->GetCurrentTable();
	BOOST_REQUIRE_EQUAL(fifthTable.size(), 4);
	BOOST_REQUIRE_EQUAL(fifthTable[0].destination_rank, 1);
	BOOST_REQUIRE_EQUAL(fifthTable[0].sequence_id, 8);
	BOOST_REQUIRE_EQUAL(fifthTable[1].destination_rank, 2);
	BOOST_REQUIRE_EQUAL(fifthTable[1].sequence_id, 9);
	BOOST_REQUIRE_EQUAL(fifthTable[2].destination_rank, 3);
	BOOST_REQUIRE_EQUAL(fifthTable[2].sequence_id, 10);
	BOOST_REQUIRE_EQUAL(fifthTable[3].destination_rank, 4);
	BOOST_REQUIRE_EQUAL(fifthTable[3].sequence_id, 11);

	// Lots of "normal" tokens
	nth->AddReceiverToken(1, 10);
	nth->AddReceiverToken(2, 10);
	nth->AddReceiverToken(4, 10);
	auto sixthTable = nth->GetCurrentTable();
	BOOST_REQUIRE_EQUAL(sixthTable.size(), 3);
	BOOST_REQUIRE_EQUAL(sixthTable[0].destination_rank, 1);
	BOOST_REQUIRE_EQUAL(sixthTable[0].sequence_id, 12);
	BOOST_REQUIRE_EQUAL(sixthTable[1].destination_rank, 2);
	BOOST_REQUIRE_EQUAL(sixthTable[1].sequence_id, 13);
	BOOST_REQUIRE_EQUAL(sixthTable[2].destination_rank, 4);
	BOOST_REQUIRE_EQUAL(sixthTable[2].sequence_id, 14);

	// Still no "nth" token
	auto blankTable = nth->GetCurrentTable();
	BOOST_REQUIRE_EQUAL(blankTable.size(), 0);

	// Some "nth" tokens
	nth->AddReceiverToken(3, 3);
	auto seventhTable = nth->GetCurrentTable();
	BOOST_REQUIRE_EQUAL(seventhTable.size(), 15);
	BOOST_REQUIRE_EQUAL(seventhTable[0].destination_rank, 3);
	BOOST_REQUIRE_EQUAL(seventhTable[0].sequence_id, 15);
	BOOST_REQUIRE_EQUAL(seventhTable[1].destination_rank, 1);
	BOOST_REQUIRE_EQUAL(seventhTable[1].sequence_id, 16);
	BOOST_REQUIRE_EQUAL(seventhTable[2].destination_rank, 2);
	BOOST_REQUIRE_EQUAL(seventhTable[2].sequence_id, 17);
	BOOST_REQUIRE_EQUAL(seventhTable[3].destination_rank, 4);
	BOOST_REQUIRE_EQUAL(seventhTable[3].sequence_id, 18);
	BOOST_REQUIRE_EQUAL(seventhTable[4].destination_rank, 1);
	BOOST_REQUIRE_EQUAL(seventhTable[4].sequence_id, 19);
	BOOST_REQUIRE_EQUAL(seventhTable[5].destination_rank, 3);
	BOOST_REQUIRE_EQUAL(seventhTable[5].sequence_id, 20);
	BOOST_REQUIRE_EQUAL(seventhTable[6].destination_rank, 2);
	BOOST_REQUIRE_EQUAL(seventhTable[6].sequence_id, 21);
	BOOST_REQUIRE_EQUAL(seventhTable[7].destination_rank, 4);
	BOOST_REQUIRE_EQUAL(seventhTable[7].sequence_id, 22);
	BOOST_REQUIRE_EQUAL(seventhTable[8].destination_rank, 1);
	BOOST_REQUIRE_EQUAL(seventhTable[8].sequence_id, 23);
	BOOST_REQUIRE_EQUAL(seventhTable[9].destination_rank, 2);
	BOOST_REQUIRE_EQUAL(seventhTable[9].sequence_id, 24);
	BOOST_REQUIRE_EQUAL(seventhTable[10].destination_rank, 3);
	BOOST_REQUIRE_EQUAL(seventhTable[10].sequence_id, 25);
	BOOST_REQUIRE_EQUAL(seventhTable[11].destination_rank, 4);
	BOOST_REQUIRE_EQUAL(seventhTable[11].sequence_id, 26);
	BOOST_REQUIRE_EQUAL(seventhTable[12].destination_rank, 1);
	BOOST_REQUIRE_EQUAL(seventhTable[12].sequence_id, 27);
	BOOST_REQUIRE_EQUAL(seventhTable[13].destination_rank, 2);
	BOOST_REQUIRE_EQUAL(seventhTable[13].sequence_id, 28);
	BOOST_REQUIRE_EQUAL(seventhTable[14].destination_rank, 4);
	BOOST_REQUIRE_EQUAL(seventhTable[14].sequence_id, 29);

	
}

BOOST_AUTO_TEST_SUITE_END()
