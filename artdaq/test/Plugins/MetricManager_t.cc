#include "artdaq-utilities/Plugins/MetricManager.hh"
#include "artdaq-core/Utilities/TimeUtils.hh"
#include "artdaq-core/Utilities/configureMessageFacility.hh"

#define BOOST_TEST_MODULES MetricManager_t
#include "cetlib/quiet_unit_test.hpp"
#include "cetlib_except/exception.h"
#include "fhiclcpp/make_ParameterSet.h"

#include "tracemf.h"

BOOST_AUTO_TEST_SUITE(MetricManager_test)

BOOST_AUTO_TEST_CASE(Construct)
{
	artdaq::configureMessageFacility("MetricManager_t", true, true);
	TLOG_DEBUG("MetricManager_t") << "BEGIN TEST Construct" << TLOG_ENDL;
	artdaq::MetricManager mm;
	BOOST_REQUIRE_EQUAL(mm.Initialized(), false);
	BOOST_REQUIRE_EQUAL(mm.Running(), false);
	BOOST_REQUIRE_EQUAL(mm.Active(), false);
	BOOST_REQUIRE_EQUAL(mm.metricQueueEmpty(), true);
	BOOST_REQUIRE_EQUAL(mm.metricQueueSize(), 0);
	TLOG_DEBUG("MetricManager_t") << "END TEST Construct" << TLOG_ENDL;
}

BOOST_AUTO_TEST_CASE(Initialize)
{

	artdaq::configureMessageFacility("MetricManager_t", true, true);
	TLOG_DEBUG("MetricManager_t") << "BEGIN TEST Initialize" << TLOG_ENDL;
	artdaq::MetricManager mm;
	BOOST_REQUIRE_EQUAL(mm.Initialized(), false);
	BOOST_REQUIRE_EQUAL(mm.Running(), false);
	BOOST_REQUIRE_EQUAL(mm.Active(), false);
	BOOST_REQUIRE_EQUAL(mm.metricQueueEmpty(), true);
	BOOST_REQUIRE_EQUAL(mm.metricQueueSize(), 0);

	std::string testConfig = "msgFac: { level: 5 metricPluginType: msgFacility reporting_interval: 1.0}";
	fhicl::ParameterSet pset;
	fhicl::make_ParameterSet(testConfig, pset);

	mm.initialize(pset, "MetricManager_t");
	BOOST_REQUIRE_EQUAL(mm.Initialized(), true);
	BOOST_REQUIRE_EQUAL(mm.Running(), false);
	BOOST_REQUIRE_EQUAL(mm.Active(), false);


	mm.do_start();
	BOOST_REQUIRE_EQUAL(mm.Running(), true);
	BOOST_REQUIRE_EQUAL(mm.Active(), true);
	TLOG_DEBUG("MetricManager_t") << "END TEST Initialize" << TLOG_ENDL;
}

BOOST_AUTO_TEST_CASE(Initialize_WithError)
{

	artdaq::configureMessageFacility("MetricManager_t", true, true);
	TLOG_DEBUG("MetricManager_t") << "BEGIN TEST Initialize_WithError" << TLOG_ENDL;
	artdaq::MetricManager mm;
	BOOST_REQUIRE_EQUAL(mm.Initialized(), false);
	BOOST_REQUIRE_EQUAL(mm.Running(), false);
	BOOST_REQUIRE_EQUAL(mm.Active(), false);
	BOOST_REQUIRE_EQUAL(mm.metricQueueEmpty(), true);
	BOOST_REQUIRE_EQUAL(mm.metricQueueSize(), 0);

	std::string testConfig = "err: { level: 5 metricPluginType: nonExistentPluginType reporting_interval: 1.0}";
	fhicl::ParameterSet pset;
	fhicl::make_ParameterSet(testConfig, pset);

	mm.initialize(pset, "MetricManager_t");
	BOOST_REQUIRE_EQUAL(mm.Initialized(), true);
	BOOST_REQUIRE_EQUAL(mm.Running(), false);

	mm.do_start();
	BOOST_REQUIRE_EQUAL(mm.Running(), true);
	BOOST_REQUIRE_EQUAL(mm.Active(), false);
	TLOG_DEBUG("MetricManager_t") << "END TEST Initialize_WithError" << TLOG_ENDL;
}

BOOST_AUTO_TEST_CASE(Shutdown)
{
	artdaq::configureMessageFacility("MetricManager_t", true, true);
	TLOG_DEBUG("MetricManager_t") << "BEGIN TEST Shutdown" << TLOG_ENDL;
	artdaq::MetricManager mm;
	BOOST_REQUIRE_EQUAL(mm.Initialized(), false);
	BOOST_REQUIRE_EQUAL(mm.Running(), false);
	BOOST_REQUIRE_EQUAL(mm.Active(), false);
	BOOST_REQUIRE_EQUAL(mm.metricQueueEmpty(), true);
	BOOST_REQUIRE_EQUAL(mm.metricQueueSize(), 0);

	std::string testConfig = "msgFac: { level: 5 metricPluginType: msgFacility reporting_interval: 1.0}";
	fhicl::ParameterSet pset;
	fhicl::make_ParameterSet(testConfig, pset);

	mm.initialize(pset, "MetricManager_t");
	BOOST_REQUIRE_EQUAL(mm.Initialized(), true);
	BOOST_REQUIRE_EQUAL(mm.Running(), false);
	BOOST_REQUIRE_EQUAL(mm.Active(), false);


	mm.do_start();
	BOOST_REQUIRE_EQUAL(mm.Running(), true);
	BOOST_REQUIRE_EQUAL(mm.Active(), true);

	mm.do_stop();
	BOOST_REQUIRE_EQUAL(mm.Running(), false);
	BOOST_REQUIRE_EQUAL(mm.Initialized(), true);

	mm.shutdown();
	BOOST_REQUIRE_EQUAL(mm.Initialized(), false);
	TLOG_DEBUG("MetricManager_t") << "END TEST Shutdown" << TLOG_ENDL;
}

BOOST_AUTO_TEST_CASE(SendMetric_String)
{

	artdaq::configureMessageFacility("MetricManager_t", true, true);
	TLOG_DEBUG("MetricManager_t") << "BEGIN TEST SendMetric_String" << TLOG_ENDL;
	artdaq::MetricManager mm;
	BOOST_REQUIRE_EQUAL(mm.Initialized(), false);
	BOOST_REQUIRE_EQUAL(mm.Running(), false);
	BOOST_REQUIRE_EQUAL(mm.Active(), false);
	BOOST_REQUIRE_EQUAL(mm.metricQueueEmpty(), true);
	BOOST_REQUIRE_EQUAL(mm.metricQueueSize(), 0);

	std::string testConfig = "msgFac: { level: 5 metricPluginType: msgFacility reporting_interval: 1.0}";
	fhicl::ParameterSet pset;
	fhicl::make_ParameterSet(testConfig, pset);

	mm.initialize(pset, "MetricManager_t");
	BOOST_REQUIRE_EQUAL(mm.Initialized(), true);
	BOOST_REQUIRE_EQUAL(mm.Running(), false);
	BOOST_REQUIRE_EQUAL(mm.Active(), false);


	mm.do_start();
	BOOST_REQUIRE_EQUAL(mm.Running(), true);
	BOOST_REQUIRE_EQUAL(mm.Active(), true);


	mm.sendMetric("Test Metric", "This is a test", "Units", 2, artdaq::MetricMode::LastPoint);


	mm.do_stop();
	BOOST_REQUIRE_EQUAL(mm.Running(), false);
	BOOST_REQUIRE_EQUAL(mm.Initialized(), true);

	mm.shutdown();
	BOOST_REQUIRE_EQUAL(mm.Initialized(), false);
	TLOG_DEBUG("MetricManager_t") << "END TEST SendMetric_String" << TLOG_ENDL;
}

BOOST_AUTO_TEST_CASE(SendMetrics)
{
	artdaq::configureMessageFacility("MetricManager_t", true, true);
	TLOG_DEBUG("MetricManager_t") << "BEGIN TEST SendMetrics" << TLOG_ENDL;
	artdaq::MetricManager mm;
	BOOST_REQUIRE_EQUAL(mm.Initialized(), false);
	BOOST_REQUIRE_EQUAL(mm.Running(), false);
	BOOST_REQUIRE_EQUAL(mm.Active(), false);
	BOOST_REQUIRE_EQUAL(mm.metricQueueEmpty(), true);
	BOOST_REQUIRE_EQUAL(mm.metricQueueSize(), 0);

	std::string testConfig = "msgFac: { level: 5 metricPluginType: msgFacility reporting_interval: 1.0}";
	fhicl::ParameterSet pset;
	fhicl::make_ParameterSet(testConfig, pset);

	mm.initialize(pset, "MetricManager_t");
	BOOST_REQUIRE_EQUAL(mm.Initialized(), true);
	BOOST_REQUIRE_EQUAL(mm.Running(), false);
	BOOST_REQUIRE_EQUAL(mm.Active(), false);


	mm.do_start();
	BOOST_REQUIRE_EQUAL(mm.Running(), true);
	BOOST_REQUIRE_EQUAL(mm.Active(), true);


	mm.sendMetric("Test Metric LastPoint", 1, "Units", 2, artdaq::MetricMode::LastPoint);
	mm.sendMetric("Test Metric LastPoint", 5, "Units", 2, artdaq::MetricMode::LastPoint);
	TLOG_INFO("MetricManager_t") << "LastPoint Metric should have value 5" << TLOG_ENDL;

	mm.sendMetric("Test Metric Accumulate", 4, "Units", 2, artdaq::MetricMode::Accumulate);
	mm.sendMetric("Test Metric Accumulate", 5, "Units", 2, artdaq::MetricMode::Accumulate);
	TLOG_INFO("MetricManager_t") << "Accumulate metric should have value 9" << TLOG_ENDL;
	mm.sendMetric("Test Metric Average", 1, "Units", 2, artdaq::MetricMode::Average);
	mm.sendMetric("Test Metric Average", 3, "Units", 2, artdaq::MetricMode::Average);
	TLOG_INFO("MetricManager_t") << "Average metric should have value 2" << TLOG_ENDL;



	mm.do_stop();
	BOOST_REQUIRE_EQUAL(mm.Running(), false);
	BOOST_REQUIRE_EQUAL(mm.Initialized(), true);

	mm.shutdown();
	BOOST_REQUIRE_EQUAL(mm.Initialized(), false);
	TLOG_DEBUG("MetricManager_t") << "END TEST SendMetrics" << TLOG_ENDL;
}

BOOST_AUTO_TEST_CASE(MetricFlood)
{

	artdaq::configureMessageFacility("MetricManager_t", true, true);
	TLOG_DEBUG("MetricManager_t") << "BEGIN TEST MetricFlood" << TLOG_ENDL;
	artdaq::MetricManager mm;
	BOOST_REQUIRE_EQUAL(mm.Initialized(), false);
	BOOST_REQUIRE_EQUAL(mm.Running(), false);
	BOOST_REQUIRE_EQUAL(mm.Active(), false);
	BOOST_REQUIRE_EQUAL(mm.metricQueueEmpty(), true);
	BOOST_REQUIRE_EQUAL(mm.metricQueueSize(), 0);

	std::string testConfig = "msgFac: { level: 5 metricPluginType: msgFacility reporting_interval: 1.0}";
	fhicl::ParameterSet pset;
	fhicl::make_ParameterSet(testConfig, pset);

	mm.initialize(pset, "MetricManager_t");
	BOOST_REQUIRE_EQUAL(mm.Initialized(), true);
	BOOST_REQUIRE_EQUAL(mm.Running(), false);
	BOOST_REQUIRE_EQUAL(mm.Active(), false);


	mm.do_start();
	BOOST_REQUIRE_EQUAL(mm.Running(), true);
	BOOST_REQUIRE_EQUAL(mm.Active(), true);

	auto beforeOne = std::chrono::steady_clock::now();
	mm.sendMetric("Test Metric 1", 1, "Units", 2, artdaq::MetricMode::Accumulate);
	auto afterOne = std::chrono::steady_clock::now();

	sleep(2);

	auto beforeTen = std::chrono::steady_clock::now();
	for (auto ii = 1; ii <= 10; ++ii)
	{
		mm.sendMetric("Test Metric 10", ii, "Units", 2, artdaq::MetricMode::Accumulate);
	}
	auto afterTen = std::chrono::steady_clock::now();

	sleep(2);

	auto beforeOneHundred = std::chrono::steady_clock::now();
	for (auto ii = 1; ii <= 100; ++ii)
	{
		mm.sendMetric("Test Metric 100", ii, "Units", 2, artdaq::MetricMode::Accumulate);
	}
	auto afterOneHundred = std::chrono::steady_clock::now();

	sleep(2);

	auto beforeOneThousand = std::chrono::steady_clock::now();
	for (auto ii = 1; ii <= 1000; ++ii)
	{
		mm.sendMetric("Test Metric 1000", ii, "Units", 2, artdaq::MetricMode::Accumulate);
	}
	auto afterOneThousand = std::chrono::steady_clock::now();

	sleep(2);

	auto beforeTenThousand = std::chrono::steady_clock::now();
	for (auto ii = 1; ii <= 10000; ++ii)
	{
		mm.sendMetric("Test Metric 10000", ii, "Units", 2, artdaq::MetricMode::Accumulate);
	}
	auto afterTenThousand = std::chrono::steady_clock::now();


	auto beforeStop = std::chrono::steady_clock::now();
	mm.do_stop();
	BOOST_REQUIRE_EQUAL(mm.Running(), false);
	BOOST_REQUIRE_EQUAL(mm.Initialized(), true);
	auto afterStop = std::chrono::steady_clock::now();

	TLOG_INFO("MetricManager_t") << "Time for One Metric: " << artdaq::TimeUtils::GetElapsedTime(beforeOne, afterOne) << " s." << TLOG_ENDL;
	TLOG_INFO("MetricManager_t") << "Time for Ten Metrics: " << artdaq::TimeUtils::GetElapsedTime(beforeTen, afterTen) << " s." << TLOG_ENDL;
	TLOG_INFO("MetricManager_t") << "Time for One Hundred Metrics: " << artdaq::TimeUtils::GetElapsedTime(beforeOneHundred, afterOneHundred) << " s." << TLOG_ENDL;
	TLOG_INFO("MetricManager_t") << "Time for One Thousand Metrics: " << artdaq::TimeUtils::GetElapsedTime(beforeOneThousand, afterOneThousand) << " s." << TLOG_ENDL;
	TLOG_INFO("MetricManager_t") << "Time for Ten Thousand Metrics: " << artdaq::TimeUtils::GetElapsedTime(beforeTenThousand, afterTenThousand) << " s." << TLOG_ENDL;
	TLOG_INFO("MetricManager_t") << "Time for Stop Metrics: " << artdaq::TimeUtils::GetElapsedTime(beforeStop, afterStop) << " s." << TLOG_ENDL;
	
	mm.shutdown();
	BOOST_REQUIRE_EQUAL(mm.Initialized(), false);
	TLOG_DEBUG("MetricManager_t") << "END TEST MetricFlood" << TLOG_ENDL;
}

BOOST_AUTO_TEST_SUITE_END()