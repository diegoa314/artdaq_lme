#include "artdaq/DAQrate/detail/FragCounter.hh"
#include "canvas/Utilities/Exception.h"

using artdaq::detail::FragCounter;

#define BOOST_TEST_MODULE FragCounter_t
#include <boost/test/auto_unit_test.hpp>

BOOST_AUTO_TEST_SUITE(FragCounter_test)

	BOOST_AUTO_TEST_CASE(Construct)
	{
		FragCounter f1;
	}

	BOOST_AUTO_TEST_CASE(nSlots)
	{
		FragCounter f1;
		BOOST_REQUIRE_EQUAL(f1.nSlots(), 0ul);
	}

	BOOST_AUTO_TEST_CASE(Apply)
	{
		FragCounter f;
		f.incSlot(0);
		BOOST_REQUIRE_EQUAL(f.slotCount(0), 1ul);
		f.incSlot(1, 4);
		BOOST_REQUIRE_EQUAL(f.slotCount(1), 4ul);
		f.incSlot(1);
		BOOST_REQUIRE_EQUAL(f.slotCount(1), 5ul);
		f.incSlot(1, 2);
		BOOST_REQUIRE_EQUAL(f.slotCount(1), 7ul);
		BOOST_REQUIRE_EQUAL(f.slotCount(2), 0ul);
		BOOST_REQUIRE_EQUAL(f.count(), 8ul);
	}

	BOOST_AUTO_TEST_CASE(ApplyWithOffset)
	{
		FragCounter f;
		f.incSlot(4);
		BOOST_REQUIRE_EQUAL(f.slotCount(4), 1ul);
		f.incSlot(5, 4);
		BOOST_REQUIRE_EQUAL(f.slotCount(5), 4ul);
		f.incSlot(5);
		BOOST_REQUIRE_EQUAL(f.slotCount(5), 5ul);
		f.incSlot(5, 2);
		BOOST_REQUIRE_EQUAL(f.slotCount(5), 7ul);
		BOOST_REQUIRE_EQUAL(f.slotCount(6), 0ul);
		BOOST_REQUIRE_EQUAL(f.count(), 8ul);
	}

BOOST_AUTO_TEST_SUITE_END()
